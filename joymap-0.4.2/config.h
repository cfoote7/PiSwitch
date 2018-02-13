#ifndef __config_h
#define __config_h

#define CONFIG_MAX 10
#define UINPUT_DEV 0
#define EVENT_DEV 1

char *get_config(int key);
char *set_config(int key, char *value);
void cmdline_config(int argc, char *argv[]);

#endif
