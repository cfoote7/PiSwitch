#ifndef __dict
#define __dict
#include <stdlib.h>

typedef struct entry {
	char *key;
	char *value;
	struct entry * next;
} entry;

typedef struct entry *dictionary;

dictionary add_entry(dictionary d, char *key, char *value);
void free_dictionary(dictionary d);
char *lookup_dictionary(dictionary d, char *key);
char *get_current(dictionary d);
dictionary next_entry(dictionary d);
#endif
