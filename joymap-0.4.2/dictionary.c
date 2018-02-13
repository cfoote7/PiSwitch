#include <stdlib.h>
#include <string.h>
#include "dictionary.h"

static entry *lookup_entry(dictionary d, char *key) {
    struct entry *e;
    if (d==NULL) return NULL;
    e=d;
    while (e!=NULL) {
        if (strcmp(e->key, key)==0) {
            return e;
        }
        e=e->next;
    }
    return NULL;
}

char *lookup_dictionary(dictionary d, char *key) {
    struct entry *e;
    if (d==NULL) return NULL;
    e=lookup_entry(d, key);
    if (e==NULL) return NULL;
    else return e->value;
}

dictionary add_entry(dictionary d, char *key, char *value) {
    struct entry *e;

    e=lookup_entry(d, key);
    if (e!=NULL) {
        e->value=strdup(value);
        return d;
    }

    e=(entry *)malloc(sizeof(entry));
    e->key=strdup(key);
    e->value=strdup(value);
    e->next=d;
    return e;
}

void free_dictionary(dictionary d) {
    struct entry *nd=NULL;
    while (d!=NULL) {
        nd=d->next;
        free(d->key);
        free(d->value);
        free(d);
        d=nd;
    }
}

static char key_value[1024];
char *get_current(dictionary d) {
    if (d==NULL) return NULL;
    strcpy(key_value, d->key);
    strcat(key_value, "=\"");
    strcat(key_value, d->value);
    strcat(key_value, "\"");
    return key_value;
}

dictionary next_entry(dictionary d) {
    if (d==NULL) return NULL;
    return d->next;
}
