/*
 * Joystick remapper for the input driver suite.
 * by Alexandre Hardy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <fcntl.h>
#include <sys/types.h>
#include <linux/input.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "mapper.h"

#define MAX_SIGNALS     16
#define MAX_EVENTS      32

#define MSECS(t)        (1000 * ((t) / HZ) + 1000 * ((t) % HZ) / HZ)

#define TIMEOUT         20 //(50 times a second)

#undef press
#undef release

struct shift_map {
    struct program_button_remap *button_press[KEY_MAX+1];
    struct program_button_remap *button_release[KEY_MAX+1];
    struct program_axis_remap *axes[ABS_MAX+1];
};

struct mapping {
    int fd; /* /dev/input/event* file descriptor */
    __uint16_t vendor;
    __uint16_t product;
    int jsnum;
    int mapped;
    //two maps, one for shifted, and one for not shifted
    struct shift_map map[2];
};

static int shifted=0;
static __uint16_t shift_vendor;
static __uint16_t shift_product;
static __uint16_t shift_button;
struct mapping *events[MAX_EVENTS];

//install after creating joysticks and code joystick
//our joysticks are vendor 0xff and product 0x01 and must not be handled
//our mice are vendor 0xff and product 0x02 and must not be handled
//our kbd are vendor 0xff and product 0x03 and must not be handled
//the code joystick is 0xff and product 0x00
//the code joystick must be handled by our code
void install_event_handlers() {
    int i, j, r;
    char name[256];
    struct input_id id;
    for (i=0; i<=MAX_EVENTS; i++) {
        sprintf(name, "%s%d", get_config(EVENT_DEV), i);
        int fd=open(name, O_RDONLY|O_NONBLOCK);
        if (fd<0) {
            events[i]=NULL;
        } else {
            r = ioctl(fd, EVIOCGID, &id);
            if (r < 0) {
                perror("Failed to get vendor and product id");
                exit(1);
            }
            if ((id.vendor==0x00ff)&&(id.product!=0x0000)) {
                close(fd);
                events[i]=NULL;
                continue;
            }
            char devname[256];
            memset(devname, 0, 256);
            r = ioctl(fd, EVIOCGNAME(255), devname);
            if (r < 0) {
                perror("Failed to get input device name");
                exit(1);
            }
            fprintf(stderr, "Found device %s (vendor=0x%04x, product=0x%04x)\n", devname, id.vendor, id.product);
            events[i]=(struct mapping *)malloc(sizeof(struct mapping));
            events[i]->fd=fd;
            events[i]->vendor=id.vendor;
            events[i]->product=id.product;
            events[i]->jsnum=-1;
            events[i]->mapped=0;

            for (j=0; j<KEY_MAX+1; j++) {
                events[i]->map[0].button_press[j]=NULL;
                events[i]->map[0].button_release[j]=NULL;
                events[i]->map[1].button_press[j]=NULL;
                events[i]->map[1].button_release[j]=NULL;
            }
            for (j=0; j<ABS_MAX+1; j++) {
                events[i]->map[0].axes[j]=NULL;
                events[i]->map[1].axes[j]=NULL;
            }
        }
    }
}


static void process_key(struct mapping *mapper, int key, int value);
static void process_axis(struct mapping *mapper, int axis, int value);
void poll_joystick_loop() {
    int i, j, n=0;
    struct pollfd polled_fds[MAX_EVENTS];
    struct mapping *poll_mapper[MAX_EVENTS];
    struct input_event ev[16];
    int rb;
    for (i=0; i<MAX_EVENTS; i++) {
        if (events[i] && (events[i]->mapped)) {
            poll_mapper[n]=events[i];
            polled_fds[n].fd=events[i]->fd;
            polled_fds[n++].events=POLLIN;
        }
    }

    poll(polled_fds, n, TIMEOUT);
    /* TODO: We may return early, we should indicate to program_run if a timertick is required */
    /* http://man7.org/linux/man-pages/man2/clock_gettime.2.html */
    for (i=0; i<n; i++) {
        if (polled_fds[i].revents&POLLIN) {
            rb=1;
            while (rb>0) {
                rb=read(polled_fds[i].fd, ev, sizeof(struct input_event)*16);
                if (rb>0)
                for (j=0; j<(int)(rb/sizeof(struct input_event)); j++) {
                    if (ev[j].type==EV_KEY) {
                        if ((ev[j].code >= BTN_MISC) && (ev[j].value != 2))
                            process_key(poll_mapper[i], ev[j].code, ev[j].value);
                    }
                    if (ev[j].type==EV_ABS)
                        process_axis(poll_mapper[i], ev[j].code, ev[j].value);
                }
            }
        }
    }
    program_run();
    repeat_mouse_move();
}

static int nsignals=0;
static int signals[MAX_SIGNALS];
static int sighead=0;
static int sigtail=0;
int no_signal(void) {
    return (nsignals==0);
}

int goto_next_signal(void) {
    int r=signals[sighead];
    nsignals--;
    sighead++;
    sighead=sighead%MAX_SIGNALS;
    return r;
}

void push_signal(int signal) {
    //silently drop signals if too many are sent
    if (nsignals>=MAX_SIGNALS) return;
    nsignals++;
    signals[sigtail]=signal;
    sigtail++;
    sigtail=sigtail%MAX_SIGNALS;
}

static void process_key(struct mapping *mapper, int key, int value) {
    int button=0;
    int i, j;
    struct program_button_remap **button_remap;

    if ((mapper->vendor==shift_vendor)||(mapper->product==shift_product)) {
        if (key==shift_button) {
            shifted=value;
            if (shifted!=0) shifted=1;
        }
    }

    if ((mapper->vendor!=0x00ff)||(mapper->product!=0x0000))
        code_notify_button(mapper->jsnum, key, value);
    if (value==1) button_remap=mapper->map[shifted].button_press;
    else if (value==0) button_remap=mapper->map[shifted].button_release;
    else return;
    if (button_remap==NULL) return;
    if (button_remap[key]==NULL) return;

    j=button_remap[key]->device&0x0F;
    switch (button_remap[key]->device&0xF0) {
        case DEVICE_JOYSTICK:
            if (button_remap[key]->type==TYPE_BUTTON) {
                for (i=0; (i<MAX_SEQUENCE) && (button_remap[key]->sequence[i]!=SEQUENCE_DONE); i++) {
                    button=button_remap[key]->sequence[i]&KEYMASK;
                    if (button_remap[key]->sequence[i]&RELEASEMASK) value=0;
                    else value=1;
                    press_joy_button(j, button, value);
                    if ((button_remap[key]->flags&FLAG_AUTO_RELEASE)&&(value==1)) {
                        press_joy_button(j, button, 0);
                    }
                }
            } else if (button_remap[key]->type==TYPE_AXIS) {
                //it is an axis
                if (value==1) value=32767;
                if (button_remap[key]->flags&FLAG_INVERT) value=-value;
                set_joy_axis(j, button_remap[key]->sequence[0], value);
            }
            break;
        case DEVICE_MOUSE:
            if (button_remap[key]->type==TYPE_BUTTON) {
                for (i=0; (i<MAX_SEQUENCE) && (button_remap[key]->sequence[i]!=SEQUENCE_DONE); i++) {
                    button=button_remap[key]->sequence[i]&KEYMASK;
                    if (button_remap[key]->sequence[i]&RELEASEMASK) value=0;
                    else value=1;
                    press_mouse_button(button, value);
                    if ((button_remap[key]->flags&FLAG_AUTO_RELEASE)&&(value==1)) {
                        press_mouse_button(button, 0);
                    }
                }
            } else if (button_remap[key]->type==TYPE_AXIS) {
                //it is an axis
                if (button_remap[key]->flags&FLAG_INVERT) value=-value;
                if (button_remap[key]->sequence[0]==ABS_X)
                    move_mouse_x(value * button_remap[key]->speed);
                if (button_remap[key]->sequence[0]==ABS_Y)
                    move_mouse_y(value * button_remap[key]->speed);
                if (button_remap[key]->sequence[0]==ABS_WHEEL)
                    move_mouse_wheel(value * button_remap[key]->speed);
            }
            break;
        case DEVICE_KBD:
            if (button_remap[key]->type==TYPE_BUTTON) {
                for (i=0; (i<MAX_SEQUENCE) && (button_remap[key]->sequence[i]!=SEQUENCE_DONE); i++) {
                    button=button_remap[key]->sequence[i]&KEYMASK;
                    if (button_remap[key]->sequence[i]&RELEASEMASK) value=0;
                    else value=1;
                    press_key(button, value);
                    if ((button_remap[key]->flags&FLAG_AUTO_RELEASE)&&(value==1)) {
                        press_key(button, 0);
                    }
                }
            }
            break;
    }
}

static void process_axis(struct mapping *mapper, int axis, int value) {
    int button=0;
    int i, j;
    struct program_axis_remap **axes_remap;
    __uint16_t *sequence, *off;
    int release = 0;
//        fprintf(stderr, "Scaling check Axis %d val %d\n", axis, value);
//rescale axis 16 and 17 to full axis.
    if ((axis==16)||(axis==17)) {
//        fprintf(stderr, "Scaling Axis %d val %d", axis, value);
        if (value==1) value = 255;
        if (value==0) value = 0;
        if (value==-1) value = -255;
    }
    if ((mapper->vendor!=0x00ff)||(mapper->product!=0x0000))
        code_notify_axis(mapper->jsnum, axis, value);

    axes_remap=mapper->map[shifted].axes;
    if (axes_remap==NULL) return;
    if (axes_remap[axis]==NULL) return;

    if (axes_remap[axis]->min != axes_remap[axis]->max) {
        if (axes_remap[axis]->min < axes_remap[axis]->max) {
            if (value < axes_remap[axis]->min)
                value = axes_remap[axis]->min;
            if (value > axes_remap[axis]->max)
                value = axes_remap[axis]->max;
        } else {
            if (value > axes_remap[axis]->min)
                value = axes_remap[axis]->min;
            if (value < axes_remap[axis]->max)
                value = axes_remap[axis]->max;
        }
        // sigh. use floating point because I am too lazy (right now) to work out an overflow free integer version
        value = ((value - axes_remap[axis]->min) * 65536.0) / (axes_remap[axis]->max - axes_remap[axis]->min) - 32767;
        if (value < -32767) value = -32767;
        if (value > 32767) value = 32767;
    }

    if ((value >= -axes_remap[axis]->deadzone) && (value <= axes_remap[axis]->deadzone)) {
        value = 0;
    } else if (axes_remap[axis]->deadzone) {
        // we don't want a sudden jump in values. rescale it.
        if (value < 0)
            value = (value + axes_remap[axis]->deadzone) * 32767.0 / (32767 - axes_remap[axis]->deadzone);
        else
            value = (value - axes_remap[axis]->deadzone) * 32767.0 / (32767 - axes_remap[axis]->deadzone);
    }

    if (axes_remap[axis]->flags&FLAG_BINARY) {
        // only process the event if there is a binary change (differs in sign and non-zero)
        if ((axes_remap[axis]->saved_value<0) && (value<=0)) {
            return;
        }
        if ((axes_remap[axis]->saved_value>0) && (value>=0)) {
            return;
        }
        if ((axes_remap[axis]->saved_value==0) && (value==0)) {
            return;
        }
        axes_remap[axis]->saved_value=value;
        if (value == 0)
            //release any keys pressed
            release = 1;
    }

    if (axes_remap[axis]->flags&FLAG_TRINARY) {
        // only process the event if there is a binary change (differs in sign and non-zero)
        if ((axes_remap[axis]->saved_value<0) && (value<0)) {
            return;
        }
        if ((axes_remap[axis]->saved_value>0) && (value>0)) {
            return;
        }
        axes_remap[axis]->saved_value=value;
        if (value == 0)
            //release any keys pressed
            release = 1;
    }

    j=axes_remap[axis]->device&0x0F;

    if (axes_remap[axis]->type==TYPE_BUTTON) {
        if (value<0) {
            sequence=axes_remap[axis]->minus;
            off=axes_remap[axis]->plus;
        } else {
            sequence=axes_remap[axis]->plus;
            off=axes_remap[axis]->minus;
        }
    }

    switch (axes_remap[axis]->device&0xF0) {
        case DEVICE_JOYSTICK:
            if (axes_remap[axis]->type==TYPE_BUTTON) {
                //a joystick button
                for (i=0; (i<MAX_SEQUENCE) && (off[i]!=SEQUENCE_DONE); i++) {
                    button=off[i]&KEYMASK;
                    press_joy_button(j, button, 0);
                }
                for (i=0; (i<MAX_SEQUENCE) && (sequence[i]!=SEQUENCE_DONE); i++) {
                    button=sequence[i]&KEYMASK;
                    if ((release) || (sequence[i]&RELEASEMASK)) value=0;
                    else value=1;
                    press_joy_button(j, button, value);
                    if ((axes_remap[axis]->flags&FLAG_AUTO_RELEASE)&&(value==1)) {
                        press_joy_button(j, button, 0);
                    }
                }
            } else if (axes_remap[axis]->type==TYPE_AXIS) {
                //it is an axis
                if (axes_remap[axis]->flags&FLAG_INVERT) value=-value;
                set_joy_axis(j, axes_remap[axis]->axis, value);
            }
            break;
        case DEVICE_MOUSE:
            if (axes_remap[axis]->type==TYPE_BUTTON) {
                //a mouse button
                for (i=0; (i<MAX_SEQUENCE) && (off[i]!=SEQUENCE_DONE); i++) {
                    button=off[i]&KEYMASK;
                    press_mouse_button(button, 0);
                }
                for (i=0; (i<MAX_SEQUENCE) && (sequence[i]!=SEQUENCE_DONE); i++) {
                    button=sequence[i]&KEYMASK;
                    if ((release) || (sequence[i]&RELEASEMASK)) value=0;
                    else value=1;
                    press_mouse_button(button, value);
                    if ((axes_remap[axis]->flags&FLAG_AUTO_RELEASE)&&(value==1)) {
                        press_mouse_button(button, 0);
                    }
                }
            } else if (axes_remap[axis]->type==TYPE_AXIS) {
                //it is an axis, assume -32767 to 32767. Use min and max if that is not the case
                // make it a lot smaller
                value = value * ((float)(axes_remap[axis]->speed) / 32767.0);
                if (axes_remap[axis]->flags&FLAG_INVERT) value=-value;
                //if (value>0) value=1;
                //if (value<0) value=-1;
                if (axes_remap[axis]->axis==ABS_X)
                    move_mouse_x(value);
                if (axes_remap[axis]->axis==ABS_Y)
                    move_mouse_y(value);
                if (axes_remap[axis]->axis==ABS_WHEEL)
                    move_mouse_wheel(value);
            }
            break;
        case DEVICE_KBD:
            if (axes_remap[axis]->type==TYPE_BUTTON) {
                for (i=0; (i<MAX_SEQUENCE) && (axes_remap[axis]->sequence[i]!=SEQUENCE_DONE); i++) {
                    button=axes_remap[axis]->sequence[i]&KEYMASK;
                    if (axes_remap[axis]->sequence[i]&RELEASEMASK) value=0;
                    else value=1;
                    press_key(button, value);
                    if ((axes_remap[axis]->flags&FLAG_AUTO_RELEASE)&&(value==1)) {
                        press_key(button, 0);
                    }
                }
            }
            break;
    }
}

void remap_axis(struct program_axis_remap *axis) {
    struct mapping *mapper;
    int i, shifted, r;

    if (axis->program!=PROGRAM_AXIS_REMAP) return;
    if (axis->srcaxis>ABS_MAX) return;

    mapper=NULL;
    for (i=0; i<MAX_EVENTS; i++) {
        if (events[i])
            if ((events[i]->vendor==axis->vendor)&&
               (events[i]->product==axis->product))
                mapper=events[i];
    }

    if (mapper==NULL) return;
    if (mapper->mapped)
        r = 0;
    else
        r = ioctl(mapper->fd, EVIOCGRAB, 1);
    mapper->mapped=1;
    if (r < 0) {
        perror("Failed to grab device");
        fprintf(stderr, "Failed to lock device with vendor=0x%04x, product=0x%04x. Continuing anyway...\n", axis->vendor, axis->product);
    }

    shifted=0;
    if (axis->flags&FLAG_SHIFT) shifted=1;
    mapper->map[shifted].axes[axis->srcaxis]=axis;
}

void remap_button(struct program_button_remap *btn) {
    struct mapping *mapper;
    int i, shifted, r;

    if (btn->program!=PROGRAM_BUTTON_REMAP) return;
    if (btn->srcbutton>KEY_MAX) return;

    mapper=NULL;
    for (i=0; i<MAX_EVENTS; i++) {
        if (events[i])
            if ((events[i]->vendor==btn->vendor)&&
               (events[i]->product==btn->product))
                mapper=events[i];
    }

    if (mapper==NULL) return;
    if (mapper->mapped)
        r = 0;
    else
        r = ioctl(mapper->fd, EVIOCGRAB, 1);
    mapper->mapped=1;
    if (r < 0) {
        perror("Failed to grab device");
        fprintf(stderr, "Failed to lock device with vendor=0x%04x, product=0x%04x. Continuing anyway...\n", btn->vendor, btn->product);
    }

    shifted=0;
    if (btn->flags&FLAG_SHIFT) shifted=1;

    if (btn->type==TYPE_SHIFT) {
        shift_vendor=btn->vendor;
        shift_product=btn->product;
        shift_button=btn->srcbutton;
    } else {
        if (btn->flags&FLAG_RELEASE)
            mapper->map[shifted].button_release[btn->srcbutton]=btn;
        else
            mapper->map[shifted].button_press[btn->srcbutton]=btn;
    }
}

void set_joystick_number(__uint16_t vendor, __uint16_t product, int device) {
    struct mapping *mapper;
    int i;
    mapper=NULL;
    for (i=0; i<MAX_EVENTS; i++) {
        if (events[i])
            if ((events[i]->vendor==vendor)&&
               (events[i]->product==product))
                mapper=events[i];
    }

    if (mapper==NULL) return;
    mapper->mapped=1;
    mapper->jsnum=device;
}
