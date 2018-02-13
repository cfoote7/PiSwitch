#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "config.h"

char *current_config[CONFIG_MAX] = {
    /* UINPUT_DEV */
    "/dev/uinput",
    /* EVENT_DEV */
    "/dev/input/event",
    NULL,
};

char *cmdline_arg[CONFIG_MAX] = {
    /* UINPUT_DEV */
    "--uinput_dev",
    /* EVENT_DEV */
    "--event_dev",
    NULL,
};

char *get_config(int key) {
    if ((key >= CONFIG_MAX) || (key < 0)) {
        fprintf(stderr, "ERR: Invalid config key in code: %d\n", key);
        exit(1);
    }
    return current_config[key];
}

char *set_config(int key, char *value) {
    char *old;
    if ((key >= CONFIG_MAX) || (key < 0)) {
        fprintf(stderr, "ERR: Invalid config key in code: %d\n", key);
        exit(1);
    }
    old = current_config[key];
    current_config[key] = value;
    return old;
}

int match_config(char *arg) {
    int i=0;
    while (cmdline_arg[i] != NULL) {
        if (strcmp(cmdline_arg[i], arg) == 0)
            return i;
        i++;
    }
    return -1;
}

void cmdline_config(int argc, char *argv[]) {
    int i, index;
    for (i=0; i<argc; i++) {
        index = match_config(argv[i]);
        if (index >= 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing argument for: %s\n", argv[i]);
            }
            current_config[index] = argv[++i];
        }
    }
}
