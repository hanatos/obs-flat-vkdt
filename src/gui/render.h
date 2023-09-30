#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef __cplusplus
extern "C" {
#endif

// initialise imgui. has to go after glfw and vulkan surface have been inited.
// will steal whatever is needed from global qvk struct.
int dt_gui_init_imgui();

// initialise and upload fonts for current window height
void dt_gui_init_fonts();

// tear down imgui and free resources
void dt_gui_cleanup_imgui();

// poll imgui events from given event
// return non-zero if event should not be passed on
int dt_gui_poll_event_imgui(GLFWwindow *w, int key, int scancode, int action, int mods);

void dt_gui_imgui_keyboard(GLFWwindow *w, int key, int scancade, int action, int mods);
void dt_gui_imgui_mouse_button(GLFWwindow *w, int button, int action, int mods);
void dt_gui_imgui_mouse_position(GLFWwindow *w, double x, double y);
void dt_gui_imgui_character(GLFWwindow *w, int c);
void dt_gui_imgui_scrolled(GLFWwindow *w, double xoff, double yoff);
void dt_gui_imgui_window_position(GLFWwindow *w, int x, int y);

int dt_gui_imgui_want_mouse();
int dt_gui_imgui_want_keyboard();
int dt_gui_imgui_want_text();
int dt_gui_imgui_input_blocked(); // modal dialog open or something


void dt_gui_record_command_buffer_imgui(VkCommandBuffer cmd_buf);
// time-throttled access to nav input (keyboard/gamepad navigation)
float dt_gui_imgui_nav_input(int which);
float dt_gui_imgui_nav_button(int which);

// display context sensitive help:
// all inputs of the dualshock 3 controller
typedef enum dt_gamepadhelp_input_t
{ // axes
  dt_gamepadhelp_analog_stick_L = 0,
  dt_gamepadhelp_analog_stick_R,
  dt_gamepadhelp_L2,
  dt_gamepadhelp_R2,
  // pressure sensitive buttons
  dt_gamepadhelp_button_triangle,
  dt_gamepadhelp_button_circle,
  dt_gamepadhelp_button_cross,
  dt_gamepadhelp_button_square,
  dt_gamepadhelp_L1,
  dt_gamepadhelp_R1,
  dt_gamepadhelp_arrow_up,
  dt_gamepadhelp_arrow_left,
  dt_gamepadhelp_arrow_down,
  dt_gamepadhelp_arrow_right,
  // digital buttons
  dt_gamepadhelp_start,
  dt_gamepadhelp_select,
  dt_gamepadhelp_ps,
  dt_gamepadhelp_L3,
  dt_gamepadhelp_R3,
  dt_gamepadhelp_cnt
} dt_gamepadhelp_input_t;
void dt_gamepadhelp_set(dt_gamepadhelp_input_t which, const char *str);
void dt_gamepadhelp_clear();
void dt_gamepadhelp_push();
void dt_gamepadhelp_pop();

#ifdef __cplusplus
struct ImFont;
// 0 small, 1 medium, 2 large, 3 material symbols
ImFont *dt_gui_imgui_get_font(int which);
void dt_gamepadhelp();
}
#endif
