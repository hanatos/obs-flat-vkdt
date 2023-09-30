#include "gui/view.h"
#include "gui/darkroom.h"
#include "gui/lighttable.h"
#include "gui/files.h"
#include "gui/render.h"
#include "gui/nodes.h"

int
dt_view_switch(dt_gui_view_t view)
{
  int err = 0;
  switch(vkdt.view_mode)
  {
    case s_view_darkroom:
      if(view != s_view_nodes) err = darkroom_leave();
      break;
    case s_view_lighttable:
      err = lighttable_leave();
      break;
    case s_view_files:
      err = files_leave();
      break;
    case s_view_nodes:
      err = nodes_leave();
      break;
    default:;
  }
  if(err) return err;
  dt_gui_view_t old_view = vkdt.view_mode;
  vkdt.view_mode = view;
  switch(vkdt.view_mode)
  {
    case s_view_darkroom:
      if(old_view != s_view_nodes) err = darkroom_enter();
      break;
    case s_view_lighttable:
      err = lighttable_enter();
      break;
    case s_view_files:
      err = files_enter();
      break;
    case s_view_nodes:
      err = nodes_enter();
      break;
    case s_view_cnt: // called on exit
      if(old_view == s_view_nodes) err = darkroom_leave(); // make sure to write .cfg too
      break;
    default:;
  }
  // TODO: reshuffle this stuff so we don't have to re-enter the old view?
  if(err)
  {
    vkdt.view_mode = old_view;
    return err;
  }
  return 0;
}

void
dt_view_mouse_button(GLFWwindow *window, int button, int action, int mods)
{
  // unfortunately this is always true:
  // if(dt_gui_imgui_want_mouse()) return;
  switch(vkdt.view_mode)
  {
  case s_view_darkroom:
    darkroom_mouse_button(window, button, action, mods);
    break;
  case s_view_lighttable:
    lighttable_mouse_button(window, button, action, mods);
    break;
  default:;
  }
}

void
dt_view_mouse_position(GLFWwindow *window, double x, double y)
{
  // unfortunately this is always true:
  // if(dt_gui_imgui_want_mouse()) return;
  switch(vkdt.view_mode)
  {
  case s_view_darkroom:
    darkroom_mouse_position(window, x, y);
    break;
  case s_view_lighttable:
    lighttable_mouse_position(window, x, y);
    break;
  default:;
  }
}

void
dt_view_mouse_scrolled(GLFWwindow *window, double xoff, double yoff)
{
  switch(vkdt.view_mode)
  {
  case s_view_darkroom:
    darkroom_mouse_scrolled(window, xoff, yoff);
    break;
  case s_view_lighttable:
    lighttable_mouse_scrolled(window, xoff, yoff);
    break;
  default:;
  }
}

void
dt_view_keyboard(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  if(!vkdt.wstate.grabbed && dt_gui_imgui_want_text()) return;
  switch(vkdt.view_mode)
  {
  case s_view_darkroom:
    darkroom_keyboard(window, key, scancode, action, mods);
    break;
  case s_view_lighttable:
    lighttable_keyboard(window, key, scancode, action, mods);
    break;
  default:;
  }
}

void
dt_view_process()
{
  switch(vkdt.view_mode)
  {
    case s_view_darkroom:
      darkroom_process();
      break;
    case s_view_nodes:
      nodes_process();
      break;
    default:;
  }
}

void
dt_view_pentablet_data(double x, double y, double z, double pressure, double pitch, double yaw, double roll)
{
  switch(vkdt.view_mode)
  {
    case s_view_darkroom:
      darkroom_pentablet_data(x, y, z, pressure, pitch, yaw, roll);
      break;
    default:;
  }
}

void
dt_view_pentablet_proximity(int enter)
{
  vkdt.wstate.pentablet_enabled = enter; // do this in any case to stay consistent
}
