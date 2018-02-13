#include <unistd.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <linux/uinput.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"

#define NUM_JOYSTICKS 10
#define WAIT_TIME 10

#define check_ioctl(x, y, z)\
    r = ioctl(x, y, z);\
    if (r < 0) {\
        fprintf(stderr, "Failed to perform ioctl\n");\
        perror("Error");\
        exit(1);\
    }

#define check_ioctl2(x, y)\
    r = ioctl(x, y);\
    if (r < 0) {\
        fprintf(stderr, "Failed to perform ioctl\n");\
        perror("Error");\
        exit(1);\
    }

void register_devices() {
    int i, j, r;
    struct uinput_user_dev dev;
    int fd[NUM_JOYSTICKS];
    for (i=0; i<NUM_JOYSTICKS; i++) {
        fd[i] = open(get_config(UINPUT_DEV), O_WRONLY);
        if (fd[i] < 0) {
            fprintf(stderr, "Failed to open %s:\n", get_config(UINPUT_DEV));
            perror("Error");
            exit(1);
        }
        check_ioctl(fd[i], UI_SET_EVBIT, EV_KEY);
        check_ioctl(fd[i], UI_SET_EVBIT, EV_ABS);
        check_ioctl(fd[i], UI_SET_ABSBIT, ABS_X);
        check_ioctl(fd[i], UI_SET_ABSBIT, ABS_Y);
        check_ioctl(fd[i], UI_SET_ABSBIT, ABS_RUDDER);
        check_ioctl(fd[i], UI_SET_KEYBIT, BTN_JOYSTICK+0);
        check_ioctl(fd[i], UI_SET_KEYBIT, BTN_JOYSTICK+1);

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
        r = write(fd[i], &dev, sizeof(dev));
        if (r != sizeof(dev)) {
            fprintf(stderr, "Failed to write device\n");
            perror("Error");
            exit(1);
        }

        check_ioctl2(fd[i], UI_DEV_CREATE);
    }

    sleep(WAIT_TIME);

    for (i=0; i<NUM_JOYSTICKS; i++) {
        check_ioctl2(fd[i], UI_DEV_DESTROY);
        close(fd[i]);
    }
}

int main(int argc, char *argv[]) {
    cmdline_config(argc, argv);
    if (fork()==0) {
        register_devices();
    }
    sleep(1);
    return 0;
}
