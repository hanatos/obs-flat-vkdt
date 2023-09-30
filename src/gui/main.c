#include "qvk/qvk.h"

#include "pipe/graph.h"
#include "pipe/graph-io.h"
#include "pipe/global.h"
#include "pipe/modules/api.h"
#include "db/thumbnails.h"
#include "core/log.h"
#include "core/signal.h"
#include "core/version.h"
#include "core/tools.h"
#include "gui/gui.h"
#include "gui/render.h"
#include "gui/view.h"
#include "db/db.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

dt_gui_t vkdt = {0};

int g_fullscreen = 0;

// from a stackoverflow answer. get the monitor that currently covers most of
// the window area.
static GLFWmonitor*
get_current_monitor(GLFWwindow *window)
{
  int bestoverlap = 0;
  GLFWmonitor *bestmonitor = NULL;

  int wx, wy, ww, wh;
  glfwGetWindowPos(window, &wx, &wy);
  glfwGetWindowSize(window, &ww, &wh);
  int nmonitors;
  GLFWmonitor **monitors = glfwGetMonitors(&nmonitors);

  for (int i = 0; i < nmonitors; i++)
  {
    const GLFWvidmode *mode = glfwGetVideoMode(monitors[i]);
    int mx, my;
    glfwGetMonitorPos(monitors[i], &mx, &my);
    int mw = mode->width;
    int mh = mode->height;

    int overlap =
      MAX(0, MIN(wx + ww, mx + mw) - MAX(wx, mx)) *
      MAX(0, MIN(wy + wh, my + mh) - MAX(wy, my));

    if (bestoverlap < overlap)
    {
      bestoverlap = overlap;
      bestmonitor = monitors[i];
    }
  }
  return bestmonitor;
}

// since in glfw, joysticks can only be polled and have no event interface
// (see this pull request: https://github.com/glfw/glfw/pull/1590)
// we need to look for changes in a busy loop in this dedicated thread.
// once we find activity, we'll step outside the glfwWaitEvents call by
// posting an empty event from here.
static void*
joystick_active(void *unused)
{
  uint8_t prev_butt[100] = {0};
  while(!glfwWindowShouldClose(qvk.window))
  {
    int res = 0;
    int axes_cnt = 0, butt_cnt = 0;
    const float   *axes = glfwGetJoystickAxes   (GLFW_JOYSTICK_1, &axes_cnt);
    const uint8_t *butt = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &butt_cnt);
    assert(butt_cnt < 100);
    assert(axes_cnt < 100);
    for(int i=0;i<butt_cnt;i++) if(butt[i] != prev_butt[i])
    {
      prev_butt[i] = butt[i];
      res = 1;
      break;
    }
    float restpos[20] = {0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f}; // rest positions of the dual shock 3
    for(int i=0;i<MIN(20,axes_cnt);i++) if(fabsf(axes[i] - restpos[i]) > 0.25)
    {
      res = 1;
      break;
    }
    if(res)
    {
      vkdt.wstate.busy = 20; // make sure we'll stay awake for a few frames
      glfwPostEmptyEvent();
    }
    struct timespec req = { .tv_sec = 0, .tv_nsec = 16000 }, rem;
    nanosleep(&req, &rem);
  }
  return 0;
}

static void
toggle_fullscreen()
{
  GLFWmonitor* monitor = get_current_monitor(qvk.window);
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  if(g_fullscreen)
  {
    glfwSetWindowPos(qvk.window, mode->width/8, mode->height/8);
    glfwSetWindowSize(qvk.window, mode->width/4 * 3, mode->height/4 * 3);
    g_fullscreen = 0;
  }
  else
  {
    glfwSetWindowPos(qvk.window, 0, 0);
    glfwSetWindowSize(qvk.window, mode->width, mode->height);
    g_fullscreen = 1;
  }
  dt_gui_recreate_swapchain();
  dt_gui_init_fonts();
}

static void
key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  const int grabbed = vkdt.wstate.grabbed;
  dt_view_keyboard(window, key, scancode, action, mods);
  if(!grabbed) // also don't pass on if we just ungrabbed
    dt_gui_imgui_keyboard(window, key, scancode, action, mods);

  if(key == GLFW_KEY_X && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
  {
    glfwSetWindowShouldClose(qvk.window, GLFW_TRUE);
  }
  else if(key == GLFW_KEY_F11 && action == GLFW_PRESS)
  {
    toggle_fullscreen();
  }
}

static void
mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  dt_view_mouse_button(window, button, action, mods);
  if(!vkdt.wstate.grabbed)
    dt_gui_imgui_mouse_button(window, button, action, mods);
}

static void
mouse_position_callback(GLFWwindow* window, double x, double y)
{
  dt_view_mouse_position(window, x, y);
}

static void
window_close_callback(GLFWwindow* window)
{
  glfwSetWindowShouldClose(qvk.window, GLFW_TRUE);
}

static void
window_size_callback(GLFWwindow* window, int width, int height)
{
  // window resized, need to rebuild our swapchain:
  dt_gui_recreate_swapchain();
  dt_gui_init_fonts();
}

static void
window_pos_callback(GLFWwindow* window, int xpos, int ypos)
{
  dt_gui_imgui_window_position(window, xpos, ypos);
}

static void
char_callback(GLFWwindow* window, unsigned int c)
{
  if(!vkdt.wstate.grabbed)
    dt_gui_imgui_character(window, c);
}

static void
scroll_callback(GLFWwindow *window, double xoff, double yoff)
{
  dt_view_mouse_scrolled(window, xoff, yoff);
  if(!vkdt.wstate.grabbed)
    dt_gui_imgui_scrolled(window, xoff, yoff);
}

#if VKDT_USE_PENTABLET==1
static void
pentablet_data_callback(double x, double y, double z, double pressure, double pitch, double yaw, double roll)
{
  dt_view_pentablet_data(x, y, z, pressure, pitch, yaw, roll);
}

static void
pentablet_proximity_callback(int entering)
{
  dt_view_pentablet_proximity(entering);
}

static void
pentablet_cursor_callback(unsigned int cursor)
{ }
#endif

static void
dt_gui_print_usage()
{
  printf("gui usage: vkdt [options] [file | directory]\n"
         "  options:\n"
         "       -d <log layer>   switch on log layer\n"
         "       -D <log layer>   mute log layer\n"
         "  log layers:\n"
         "  none qvk pipe gui db cli snd perf mem err all\n");
}

int main(int argc, char *argv[])
{
  if(argc > 1)
  {
    if(!strcmp(argv[1], "--version"))
    {
      printf("vkdt "VKDT_VERSION" (c) 2020--2023 johannes hanika\n");
      exit(0);
    }
    else if(!strcmp(argv[1], "--help"))
    {
      printf("vkdt "VKDT_VERSION" (c) 2020--2023 johannes hanika\n");
      dt_tool_print_usage();
      dt_gui_print_usage();
      exit(0);
    }
    dt_tool_dispatch(argc, argv);
  }
  // init global things, log and pipeline:
  dt_log_init(s_log_err|s_log_gui);
  int lastarg = dt_log_init_arg(argc, argv);
  dt_pipe_global_init();
  threads_global_init();
  dt_set_signal_handlers();

  if(dt_gui_init())
  {
    dt_log(s_log_gui|s_log_err, "failed to init gui/swapchain");
    exit(1);
  }

  // start fullscreen on current monitor, we only know which one is that after
  // we created the window in dt_gui_init(). maybe should be a config option:
  toggle_fullscreen();

  glfwSetKeyCallback(qvk.window, key_callback);
  glfwSetWindowSizeCallback(qvk.window, window_size_callback);
  glfwSetWindowPosCallback(qvk.window, window_pos_callback);
  glfwSetMouseButtonCallback(qvk.window, mouse_button_callback);
  glfwSetCursorPosCallback(qvk.window, mouse_position_callback);
  glfwSetCharCallback(qvk.window, char_callback);
  glfwSetScrollCallback(qvk.window, scroll_callback);
  glfwSetWindowCloseCallback(qvk.window, window_close_callback);
#if VKDT_USE_PENTABLET==1
  glfwSetPenTabletDataCallback(pentablet_data_callback);
  glfwSetPenTabletCursorCallback(pentablet_cursor_callback);
  glfwSetPenTabletProximityCallback(pentablet_proximity_callback);
#endif

  vkdt.view_mode = s_view_cnt;
  // this is done so we can mess with the thumbnails struct in
  // an asynchronous manner in a background thread. it will know
  // it is not serving thumbnails but it only writes bc1 to disk.
  // also we have a temporary thumbnails struct and background threads
  // to create thumbnails, if necessary.
  // only width/height will matter here
  dt_thumbnails_init(&vkdt.thumbnail_gen, 400, 400, 0, 0);
  dt_thumbnails_init(&vkdt.thumbnails, 400, 400, 3000, 1ul<<30);
  dt_db_init(&vkdt.db);
  char *filename = 0;
  {
    char defpath[1024];
    const char *mru = dt_rc_get(&vkdt.rc, "gui/ruc_entry00", "null");
    if(strcmp(mru, "null")) snprintf(defpath, sizeof(defpath), "%s", mru);
    else snprintf(defpath, sizeof(defpath), "%s/Pictures", getenv("HOME"));
    if(argc > lastarg+1) filename = realpath(argv[lastarg+1], 0);
    else                 filename = realpath(defpath, 0);
  }
  struct stat statbuf = {0};
  if(filename) stat(filename, &statbuf);
  if(!filename || ((statbuf.st_mode & S_IFMT) == S_IFDIR))
  {
    vkdt.view_mode = s_view_lighttable;
    dt_db_load_directory(&vkdt.db, &vkdt.thumbnails, filename);
    dt_view_switch(s_view_lighttable);
    dt_thumbnails_cache_collection(&vkdt.thumbnail_gen, &vkdt.db, &glfwPostEmptyEvent);
  }
  else
  {
    if(dt_db_load_image(&vkdt.db, &vkdt.thumbnails, filename))
    {
      dt_log(s_log_err, "image could not be loaded!");
      goto out;
    }
    dt_db_selection_add(&vkdt.db, 0);
    dt_view_switch(s_view_darkroom);
  }
  dt_gui_read_tags();

  // joystick
  pthread_t joystick_thread;
  const int joystick_present = vkdt.wstate.have_joystick;
  if(joystick_present) pthread_create(&joystick_thread, 0, joystick_active, 0);

  // main loop
  double beg_rf = dt_time();
  const int frame_limiter = dt_rc_get_int(&vkdt.rc, "gui/frame_limiter", 0);
  vkdt.wstate.busy = 3;
  vkdt.graph_dev.frame = vkdt.state.anim_frame = 0;
  while(!glfwWindowShouldClose(qvk.window))
  {
    // block and wait for one event instead of polling all the time to save on
    // gpu workload. might need an interrupt for "render finished" etc. we might
    // do that via glfwPostEmptyEvent()
    if(vkdt.wstate.busy > 0) vkdt.wstate.busy--;
    // vkdt.wstate.busy = 100; // do these two lines instead if profiling in nvidia gfx insight or so.
    // vkdt.graph_dev.runflags = s_graph_run_record_cmd_buf;
    if(vkdt.state.anim_playing) // should redraw because animation is playing?
      vkdt.wstate.busy = vkdt.state.anim_max_frame == -1 ? 3 : vkdt.state.anim_max_frame - vkdt.state.anim_frame + 1;
    if(vkdt.wstate.busy > 0) glfwPostEmptyEvent();
    else vkdt.wstate.busy = 3;
    // should probably consider this instead:
    // https://github.com/bvgastel/imgui/commits/imgui-2749
    glfwWaitEvents();
    if(frame_limiter || (dt_log_global.mask & s_log_perf))
    { // artificially limit frames rate to frame_limiter milliseconds/frame as minimum.
      double end_rf = dt_time();
      dt_log(s_log_perf, "fps %.2g", 1.0/(end_rf - beg_rf));
      if(frame_limiter && end_rf - beg_rf < frame_limiter / 1000.0)
      {
        usleep(frame_limiter * 1000);
        continue;
      }
      beg_rf = end_rf;
    }

    dt_gui_render_frame_imgui();

    if(dt_gui_render() == VK_SUCCESS)
      dt_gui_present();
    else
      dt_gui_recreate_swapchain();

    dt_view_process();
    if(vkdt.graph_dev.gui_msg && vkdt.graph_dev.gui_msg[0]) dt_gui_notification(vkdt.graph_dev.gui_msg);
  }
  if(joystick_present) pthread_join(joystick_thread, 0);

  QVKL(&qvk.queue_mutex, vkDeviceWaitIdle(qvk.device));

out:
  // leave whatever view we're still in:
  dt_view_switch(s_view_cnt);

  threads_shutdown();
  threads_global_cleanup(); // join worker threads before killing their resources
  dt_thumbnails_cleanup(&vkdt.thumbnails);
  dt_thumbnails_cleanup(&vkdt.thumbnail_gen);
  dt_gui_cleanup();
  dt_db_cleanup(&vkdt.db);
  dt_pipe_global_cleanup();
  free(filename);
  exit(0);
}
