#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "config.h"
#include "dictionary.h"
#include "program.h"
#include "keys.h"
#include "validkeys.h"
#include "parser.h"
#include "mapper.h"
#include "clock.h"

#define TIMEOUT         20 //(50 times a second)

int main(int argc, char *argv[]) {
    int i;
    int parse_err;
    int mult=1;
    int div=1;
    int ofs=0;
    int timeout;
    int start;
    int stop;

    cmdline_config(argc, argv);
    ++argv, --argc;  /* skip over program name */

    if (strcmp(argv[0], "-8")==0) {
        ++argv, --argc;
        mult=256;
        div=1;
        ofs=32767;
    }

    if (strcmp(argv[0], "-d")==0) {
        ++argv, --argc;
        set_dynamic_calibrate(1);
    }

    if ( argc > 0 )
        fmap = fopen(argv[0], "r");
    else
        fmap = stdin;

    if (fmap==NULL) {
        perror("Failed to open map");
        return 1;
    }

    program.program=PROGRAM_CODE;
    program.code[0]=HALT;
    parse_err=parse_map();
    if (!parse_err) {
        printf("%d joysticks.\n", njoysticks);
        set_num_joysticks(njoysticks);
        for (i=0; i<njoysticks; i++) {
            set_num_axes(i, joysticks[i].axes);
            set_num_buttons(i, joysticks[i].buttons);
            printf("joystick%d axes=%d buttons=%d.\n", i, joysticks[i].axes, joysticks[i].buttons);
        }

        register_devices();
        set_scale_factor(mult, div, ofs);
        install_event_handlers();
        printf("%d button assignments.\n", nbuttons);
        for (i=0; i<nbuttons; i++)
            remap_button(&buttons[i]);
        printf("%d axes assignments.\n", naxes);
        for (i=0; i<naxes; i++)
            remap_axis(&axes[i]);
        for (i=0; i<nscript; i++) {
            set_joystick_number(scriptassign[i].vendor, scriptassign[i].product, scriptassign[i].device);
        }
        code_set_program(&program);
    } else {
        printf("Error in map file, nothing done.\n");
        return 1;
    }

    timeout = TIMEOUT;
    while (1) {
        start = clock_millis();
        poll_joystick_loop(timeout);
        stop = clock_millis();
        timeout -= stop - start;
        /* We silently ignore skipped iterations .... */
        while (timeout <= 0)
            timeout += TIMEOUT;
    }

    mapper_code_uninstall();
    unregister_devices();
    return 0;
}
