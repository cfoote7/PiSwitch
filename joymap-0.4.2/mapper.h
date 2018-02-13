#ifndef __mapper_h
#define __mapper_h

#include "program.h"
#define BUS_JOYMAP		0xEF

void poll_joystick_loop();
void install_event_handlers();
void register_devices();
void unregister_devices();
void press_key(int code, int value);
void press_joy_button(int j, int code, int value);
void set_joy_axis(int j, int axis, int value);
void remove_all_joysticks(void);
void set_num_joysticks(int num);
void set_num_axes(int js, int axes);
void set_num_buttons(int js, int buttons);
void code_button(int code, int value);
void code_axis(int axis, int value);
void press_mouse_button(int code, int value);
void move_mouse(int dx, int dy);
void move_mouse_y(int dy);
void move_mouse_x(int dx);
void move_mouse_wheel(int rdw);
void set_mouse_pos(int cx, int cy);
void code_set_program(struct program_code *code);
void code_reset(void);
void code_notify_button(int js, int key, int value);
void code_notify_axis(int js, int axis, int value);
int no_signal(void);
int goto_next_signal(void);
void push_signal(int signal);
void release_mouse_buttons(void);
void release_keys(void);
void remap_button(struct program_button_remap *btn);
void remap_axis(struct program_axis_remap *axis);
void set_joystick_number(__uint16_t vendor, __uint16_t product, int device);
int mapper_code_install(void);
int mapper_code_uninstall(void);
void program_run();
void printcode();
void repeat_mouse_move();
void set_scale_factor(int mult, int div, int ofs);
void set_dynamic_calibrate(int on);

#endif
