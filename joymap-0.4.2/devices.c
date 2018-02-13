#include <unistd.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <linux/uinput.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "config.h"
#include "clock.h"
#include "program.h"
#include "mapper.h"

#define NUM_JOYSTICKS 10
#define INTERVAL      50

struct jscal {
    int min;
    int max;
    int stable;
};

struct joystick_device {
    int axes;
    int buttons;
    int fd;
    struct jscal cal;
};

static struct joystick_device devices[NUM_JOYSTICKS]={
    {0,0,-1, {0,0,1000}},
    {0,0,-1, {0,0,1000}},
    {0,0,-1, {0,0,1000}},
    {0,0,-1, {0,0,1000}},
    {0,0,-1, {0,0,1000}},
    {0,0,-1, {0,0,1000}},
    {0,0,-1, {0,0,1000}},
    {0,0,-1, {0,0,1000}},
    {0,0,-1, {0,0,1000}},
    {0,0,-1, {0,0,1000}}
};

static int mouse_fd;
static int kbd_fd;
static int code_fd;
static int njoysticks;
static int x = 0;
static int y = 0;
static int dx = 0;
static int dy = 0;
static int adx = 0;
static int ady = 0;
static int adw = 0;
static int dwheel = 0;

void set_num_joysticks(int num) {
    njoysticks=num;
}

void set_num_axes(int js, int axes) {
    if ((js<0)||(js>NUM_JOYSTICKS)) return;
    devices[js].axes=axes;
}

void set_num_buttons(int js, int buttons) {
    if ((js<0)||(js>NUM_JOYSTICKS)) return;
    devices[js].buttons=buttons;
}

int valid_open(char *file, int flags) {
    int fd = open(file, flags);
    if (fd < 0) {
        fprintf(stderr, "Error opening %s\n", file);
        perror("Error opening file");
        exit(1);
    }
    return fd;
}

int safe_ioctl3(int fd, int cmd, int arg) {
    int r = ioctl(fd, cmd, arg);
    if (r < 0) {
        perror("Failed to execute ioctl");
        exit(1);
    }
    return r;
}

int safe_ioctl2(int fd, int cmd) {
    int r = ioctl(fd, cmd);
    if (r < 0) {
        perror("Failed to execute ioctl");
        exit(1);
    }
    return r;
}

int safe_write(int fd, void *data, int size) {
    int r = write(fd, data, size);
    if (r != size) {
        if (r < 0) {
            perror("Failed to execute ioctl");
            exit(1);
        }
        fprintf(stderr, "Short write\n");
        exit(1);
    }
    return r;
}

void register_devices() {
    int i, j;
    struct uinput_user_dev dev;
    for (i=0; i<NUM_JOYSTICKS; i++) {
        devices[i].fd = valid_open(get_config(UINPUT_DEV), O_WRONLY);
        safe_ioctl3(devices[i].fd, UI_SET_EVBIT, EV_KEY);
        safe_ioctl3(devices[i].fd, UI_SET_EVBIT, EV_ABS);
        for (j=0; j<devices[i].axes; j++)
            safe_ioctl3(devices[i].fd, UI_SET_ABSBIT, j);
        for (j=0; j<devices[i].buttons; j++)
            safe_ioctl3(devices[i].fd, UI_SET_KEYBIT, BTN_JOYSTICK+j);

        memset(&dev, 0, sizeof(dev));
        sprintf(dev.name, "JOYMAP Joystick %d", i);

        dev.id.bustype = BUS_VIRTUAL;
        dev.id.vendor = 0x00ff;
        dev.id.product = 0x0001;
        dev.id.version = 0x0000;
        dev.ff_effects_max = 0;

        for (j = 0; j < ABS_MAX + 1; j++) {
            dev.absmax[j] = 32767;
            dev.absmin[j] = -32767;
            dev.absfuzz[i] = 0;
            dev.absflat[j] = 0;
        }
        safe_write(devices[i].fd, &dev, sizeof(dev));

        safe_ioctl2(devices[i].fd, UI_DEV_CREATE);
    }

    //now the mouse
    memset(&dev, 0, sizeof(dev));
    mouse_fd = valid_open(get_config(UINPUT_DEV), O_WRONLY);
    safe_ioctl3(mouse_fd, UI_SET_EVBIT, EV_KEY);
    safe_ioctl3(mouse_fd, UI_SET_EVBIT, EV_REL);
    safe_ioctl3(mouse_fd, UI_SET_KEYBIT, BTN_LEFT);
    safe_ioctl3(mouse_fd, UI_SET_KEYBIT, BTN_MIDDLE);
    safe_ioctl3(mouse_fd, UI_SET_KEYBIT, BTN_RIGHT);
    safe_ioctl3(mouse_fd, UI_SET_RELBIT, REL_X);
    safe_ioctl3(mouse_fd, UI_SET_RELBIT, REL_Y);
    safe_ioctl3(mouse_fd, UI_SET_RELBIT, REL_WHEEL);

    sprintf(dev.name, "JOYMAP Mouse");
    dev.id.bustype = BUS_VIRTUAL;
    dev.id.vendor = 0x00ff;
    dev.id.product = 0x0002;
    dev.id.version = 0x0000;
    dev.ff_effects_max = 0;
    safe_write(mouse_fd, &dev, sizeof(dev));

    safe_ioctl2(mouse_fd, UI_DEV_CREATE);

    //now the keyboard
    memset(&dev, 0, sizeof(dev));
    kbd_fd = valid_open(get_config(UINPUT_DEV), O_WRONLY);
    safe_ioctl3(kbd_fd, UI_SET_EVBIT, EV_KEY);
    safe_ioctl3(kbd_fd, UI_SET_EVBIT, EV_REP);
    safe_ioctl3(kbd_fd, UI_SET_EVBIT, EV_MSC);
    safe_ioctl3(kbd_fd, UI_SET_EVBIT, EV_LED);
    safe_ioctl3(kbd_fd, UI_SET_LEDBIT, LED_NUML);
    safe_ioctl3(kbd_fd, UI_SET_LEDBIT, LED_CAPSL);
    safe_ioctl3(kbd_fd, UI_SET_LEDBIT, LED_SCROLLL);
    for (i=0; i<512; i++)
        safe_ioctl3(kbd_fd, UI_SET_KEYBIT, i);

    sprintf(dev.name, "JOYMAP Keyboard");
    dev.id.bustype = BUS_VIRTUAL;
    dev.id.vendor = 0x00ff;
    dev.id.product = 0x0002;
    dev.id.version = 0x0000;
    dev.ff_effects_max = 0;
    safe_write(kbd_fd, &dev, sizeof(dev));

    safe_ioctl2(kbd_fd, UI_DEV_CREATE);

    //and the code device
    memset(&dev, 0, sizeof(dev));
    code_fd = valid_open(get_config(UINPUT_DEV), O_WRONLY);
    safe_ioctl3(code_fd, UI_SET_EVBIT, EV_KEY);
    safe_ioctl3(code_fd, UI_SET_EVBIT, EV_ABS);
    for (j=0; j<MAX_AXES; j++)
        safe_ioctl3(code_fd, UI_SET_ABSBIT, j);
    // We have to stop at BTN_DIGI to be matched as a joystick device
    // That gives at most 32 buttons
    for (j=BTN_JOYSTICK; j<BTN_DIGI; j++)
        safe_ioctl3(code_fd, UI_SET_KEYBIT, j);

    sprintf(dev.name, "JOYMAP Code Device");

    dev.id.bustype = BUS_VIRTUAL;
    dev.id.vendor = 0x00ff;
    dev.id.product = 0x0000;
    dev.id.version = 0x0000;
    dev.ff_effects_max = 0;

    for (i = 0; i < ABS_MAX + 1; i++) {
        dev.absmax[i] = 32767;
        dev.absmin[i] = -32767;
        dev.absfuzz[i] = 0;
        dev.absflat[i] = 0;
    }
    safe_write(code_fd, &dev, sizeof(dev));

    safe_ioctl2(code_fd, UI_DEV_CREATE);

    mapper_code_install();
}

void unregister_devices();

void press_key(int code, int value) {
    struct input_event event;
    gettimeofday(&event.time, NULL);
    event.type=EV_KEY;
    event.code=code;
    event.value=value;
    safe_write(kbd_fd, &event, sizeof(event));
    event.type=EV_SYN;
    event.code=SYN_REPORT;
    event.value=0;
    safe_write(kbd_fd, &event, sizeof(event));
}

void release_keys(void) {
    int i;
    for (i=0; i<512; i++) {
        press_key(i, 0);
    }
}

void press_mouse_button(int code, int value) {
    struct input_event event;
    gettimeofday(&event.time, NULL);
    event.type=EV_KEY;
    event.code=code;
    event.value=value;
    safe_write(mouse_fd, &event, sizeof(event));
    event.type=EV_SYN;
    event.code=SYN_REPORT;
    event.value=0;
    safe_write(mouse_fd, &event, sizeof(event));
}

void set_mouse_pos(int cx, int cy) {
    dx=cx-x;
    dy=cy-y;
    move_mouse_x(dx);
    move_mouse_y(dy);
    x=cx;
    y=cy;
    dx=0;
    dy=0;
    adx=0;
    ady=0;
}

void move_mouse_x(int rdx) {
    dx = rdx;
}

void move_mouse_y(int rdy) {
    dy = rdy;
}

void move_mouse_wheel(int rdw) {
    dwheel = rdw;
}

void send_move_mouse_x(int dx) {
    struct input_event event;
    gettimeofday(&event.time, NULL);
    event.type=EV_REL;
    event.code=REL_X;
    event.value=dx;
    safe_write(mouse_fd, &event, sizeof(event));
    event.type=EV_SYN;
    event.code=SYN_REPORT;
    event.value=0;
    safe_write(mouse_fd, &event, sizeof(event));
}

void send_move_mouse_wheel(int dwheel) {
    struct input_event event;
    gettimeofday(&event.time, NULL);
    event.type=EV_REL;
    event.code=REL_WHEEL;
    event.value=dwheel;
    safe_write(mouse_fd, &event, sizeof(event));
    event.type=EV_SYN;
    event.code=SYN_REPORT;
    event.value=0;
    safe_write(mouse_fd, &event, sizeof(event));
}

void send_move_mouse_y(int dy) {
    struct input_event event;
    gettimeofday(&event.time, NULL);
    event.type=EV_REL;
    event.code=REL_Y;
    event.value=dy;
    safe_write(mouse_fd, &event, sizeof(event));
    event.type=EV_SYN;
    event.code=SYN_REPORT;
    event.value=0;
    safe_write(mouse_fd, &event, sizeof(event));
}

void move_mouse(int rdx, int rdy) {
    move_mouse_x(rdx);
    move_mouse_y(rdy);
}

void repeat_mouse_move() {
    static __uint64_t last = 0;
    int mdx = 0, mdy = 0, mdw = 0;
    __uint64_t current, delta;

    if (last == 0) {
        last = clock_millis();
        return;
    }

    current = clock_millis();
    delta = current - last;

    if (dx || dy) {
        adx += delta * dx;
        ady += delta * dy;
    
        if (abs(adx) >= INTERVAL) {
            mdx = adx / INTERVAL;
            adx -= mdx * INTERVAL;
        }
    
        if (abs(ady) >= INTERVAL) {
            mdy = ady / INTERVAL;
            ady -= mdy * INTERVAL;
        }
    
        x += mdx;
        y += mdy;
        if ((mdx) || (mdy)) {
            send_move_mouse_x(mdx);
            send_move_mouse_y(mdy);
        }
    }

    if (dwheel) {
        adw += delta * dwheel;
        if (abs(adw) >= INTERVAL) {
            mdw = adw / INTERVAL;
            adw -= mdw * INTERVAL;
        }

        if (mdw)
            send_move_mouse_wheel(mdw);
    }

    last = current;
}

void release_mouse_buttons(void) {
    press_mouse_button(BTN_LEFT,0);
    press_mouse_button(BTN_RIGHT,0);
    press_mouse_button(BTN_MIDDLE,0);
}

void press_joy_button(int j, int code, int value) {
    struct input_event event;
    gettimeofday(&event.time, NULL);
    if (j==255) {
        event.type=EV_KEY;
        event.code=code;
        event.value=value;
        safe_write(code_fd, &event, sizeof(event));
        event.type=EV_SYN;
        event.code=SYN_REPORT;
        event.value=0;
        safe_write(code_fd, &event, sizeof(event));
        return;
    }
    if ((j<0)||(j>=NUM_JOYSTICKS)) return;
    event.type=EV_KEY;
    event.code=code;
    event.value=value;
    safe_write(devices[j].fd, &event, sizeof(event));
    event.type=EV_SYN;
    event.code=SYN_REPORT;
    event.value=0;
    safe_write(devices[j].fd, &event, sizeof(event));
}

static int scale_multiplier=1;
static int scale_divider=1;
static int scale_offset=0;
static int dcal=0;
static int cal_error=6500;

void set_scale_factor(int mult, int div, int ofs) {
    scale_multiplier=mult;
    scale_divider=div;
    scale_offset=ofs;
}

void set_dynamic_calibrate(int on) {
    dcal=on;
}

int rescale(int v) {
    return v*scale_multiplier/scale_divider-scale_offset;
}

int calibrate(int j, int v) {
    if (!dcal) return v;
    if (devices[j].cal.stable) {
        devices[j].cal.stable--;
        return v;
    }
    if ((devices[j].cal.min==devices[j].cal.max)&&(devices[j].cal.max==0)) {
        devices[j].cal.min=devices[j].cal.max=v;
        return v;
    }
    if (v<devices[j].cal.min) devices[j].cal.min=v;
    if (v>devices[j].cal.max) devices[j].cal.max=v;
    if (devices[j].cal.min==devices[j].cal.max)
        return v;
    v=(v-devices[j].cal.min)*65536/devices[j].cal.max-devices[j].cal.min-32767;
    v=(v*32768)/(32768-cal_error);
    return v;
}

void set_joy_axis(int j, int axis, int value) {
    struct input_event event;
    gettimeofday(&event.time, NULL);
    value = rescale(value);
    if (j==255) {
        event.type=EV_ABS;
        event.code=axis;
        event.value=value;
        safe_write(code_fd, &event, sizeof(event));
        event.type=EV_SYN;
        event.code=SYN_REPORT;
        event.value=0;
        safe_write(code_fd, &event, sizeof(event));
        return;
    }
    if ((j<0)||(j>=NUM_JOYSTICKS)) return;
    event.type=EV_ABS;
    event.code=axis;
    event.value=calibrate(j, value);
    safe_write(devices[j].fd, &event, sizeof(event));
    event.type=EV_SYN;
    event.code=SYN_REPORT;
    event.value=0;
    safe_write(devices[j].fd, &event, sizeof(event));
}
