#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "keys.h"
#include "program.h"
#include "parser.h"
#include "dictionary.h"
#include "validkeys.h"

static int error=0;

static struct reserved reserved[]={
    {"shift", SHIFT},
    {"button", BUTTON},
    {"axis", AXIS},
    {"code", CODE},
    {"script", SCRIPT},
    {"include", INCLUDE},
    {"joysticks", JOYSTICKS},
    {"joystick", JOYSTICK},
    {NULL, 0}
};

static char *known_keys[]={
    "id",
    "vendor",
    "product",
    "src",
    "target",
    "axis",
    "plus",
    "minus",
    "device",
    "flags",
    "button",
    "axes",
    "buttons",
    "min",
    "max",
    "deadzone",
    "speed",
    NULL
};

FILE *pfile=NULL;
FILE *fmap=NULL;
int line=1;
int cpos=1;
static int nextchar=NONE;

static dictionary dict=NULL;
static struct program_button_remap map;
static struct program_axis_remap amap;
static struct scriptmap script;
struct program_code program;
struct program_button_remap buttons[MAX_ASSIGN];
struct program_axis_remap axes[MAX_ASSIGN];
struct scriptmap scriptassign[MAX_ASSIGN];
static int parse_err=0;
int nbuttons=0;
int naxes=0;
int nscript=0;
int njoysticks=0;
struct joystick joysticks[8];

static int base=0;

static char *id, *vendor, *product, *src, *target, *button, *device, *flags, *axis, *plus, *minus, *min, *max, *deadzone, *speed;
static char compile[256]="";

static int ishex(char *s) {
    char c;
    if (*s!='0') return 0;
    s++;
    if (*s!='x') return 0;
    s++;
    while (*s!='\0') {
        c=*s;
        if ((('A'<=c)&&(c<='F'))||
            (('a'<=c)&&(c<='f'))||
            (('0'<=c)&&(c<='9'))) s++;
        else return 0;
    }
    return 1;
}

static int isnum(char *s) {
    char c;
    while (*s!='\0') {
        c=*s;
        if (('0'<=c)&&(c<='9')) s++;
        else return 0;
    }
    return 1;
}

int numeric(char *s) {
    int r=0;
    int sign = 1;
    char msg[256];
    if (s==NULL) return 0;
    if (s[0] == '-') {
        sign = -1;
        s++;
    }
    if (ishex(s)) {
        r=strtol(s, NULL, 16);
    } else if (isnum(s)) {
        r=strtol(s, NULL, 10);
    } else {
        sprintf(msg, "Expected a number, got %s instead", s);
        report(msg);
        return 0;
    }
    return sign * r;
}

static int get_device(char *s) {
    char msg[256];
    if (s==NULL) report("Device expected!");
    if (strcmp(s, "joyaxis")==0) return DEVICE_JOYSTICK;
    if (strcmp(s, "joybtn")==0) return DEVICE_JOYSTICK;
    if (strcmp(s, "joystick")==0) return DEVICE_JOYSTICK;
    if (strcmp(s, "kbd")==0) return DEVICE_KBD;
    if (strcmp(s, "mouse")==0) return DEVICE_MOUSE;
    sprintf(msg, "Expecting a device type:joyaxis,joybtn,joystick,kbd,mouse. Found %s instead", s);
    report(msg);
    return 255;
}

static int get_type(char *s, dictionary d) {
    char *button, *axis, *plus, *minus;
    if (s==NULL) return 255;
    if (strcmp(s, "joyaxis")==0) return TYPE_AXIS;
    if (strcmp(s, "joybtn")==0) return TYPE_BUTTON;
    button=lookup_dictionary(d, "button");
    axis=lookup_dictionary(d, "axis");
    plus=lookup_dictionary(d, "plus");
    minus=lookup_dictionary(d, "minus");
    if ((button!=NULL)&&(axis!=NULL)) {
        report("Only one of the keys 'button' and 'axis' may be specified");
        return 255;
    }
    if ((plus!=NULL)&&(axis!=NULL)) {
        report("Only one of the keys 'plus' and 'axis' may be specified");
        return 255;
    }
    if ((minus!=NULL)&&(axis!=NULL)) {
        report("Only one of the keys 'minus' and 'axis' may be specified");
        return 255;
    }
    if (button!=NULL) return TYPE_BUTTON;
    if (plus!=NULL) return TYPE_BUTTON;
    if (minus!=NULL) return TYPE_BUTTON;
    if (axis!=NULL) return TYPE_AXIS;
    return 255;
}

static int parse_flags(char *s) {
    int flags=FLAG_NONE;
    char *p=s;
    int more=1;
    char msg[256];
    if (s==NULL) return flags;
    while (more) {
        while ((*p!='\0')&&(*p!=',')&&(*p!=';')) p++;
        if (*p=='\0') more=0;
        *p='\0';
        if (strcmp(s, "autorelease")==0) flags|=FLAG_AUTO_RELEASE;
        else if (strcmp(s, "release")==0) flags|=FLAG_RELEASE;
        else if (strcmp(s, "press")==0) flags|=FLAG_PRESS;
        else if (strcmp(s, "shift")==0) flags|=FLAG_SHIFT;
        else if (strcmp(s, "invert")==0) flags|=FLAG_INVERT;
        else if (strcmp(s, "binary")==0) flags|=FLAG_BINARY;
        else if (strcmp(s, "trinary")==0) flags|=FLAG_TRINARY;
        else {
            sprintf(msg, "Unknown flag %s", s);
            report(msg);
        }
        s=p+1;
    }
    return flags;
}

static void parse_sequence(__uint16_t *sequence, char *s, int base, int type) {
    char *p;
    int releaseflag=0;
    int value;
    int i;
    int n=0;
    char msg[256];
    if (s==NULL) {
        sequence[0]=SEQUENCE_DONE;
        return;
    }
    if (isnum(s)) {
        if ((base==DEVICE_JOYSTICK)&&(type==TYPE_BUTTON)) {
            base=BTN_JOYSTICK;
        } else if ((base==DEVICE_MOUSE)&&(type==TYPE_BUTTON)) {
            base=BTN_MOUSE;
        } else base=0;
        sequence[0]=numeric(s)+base;
        sequence[1]=SEQUENCE_DONE;
    } else {
        p=s;
        while (*p!='\0') {
            //skip whitespace
            while ((*p!='\0')&&(isspace(*p))) p++;
            s=p;
            while ((*p!='\0')&&(!(isspace(*p)))) p++;
            if (*p!='\0') {
                *p='\0';
                p++;
            }
            value=-1;
            if (strcmp(s, "REL")==0) releaseflag=RELEASEMASK;
            else {
                i=0;
                while (keymap[i].value!=-1) {
                    if (strcmp(keymap[i].key, s)==0)
                        value=keymap[i].value;
                    i++;
                }
                if (value==-1) {
                    parse_err=1;
                    sprintf(msg, "Unknown key %s", s);
                    report(msg);
                } else {
                    sequence[n]=value|releaseflag;
                    releaseflag=0;
                    n++;
                }
            }
            s=p;
        }
        sequence[n]=SEQUENCE_DONE;
    }
}

static void show_dictionary(dictionary dict) {
    dictionary d=dict;
    char *entry;
    entry=get_current(d);
    while (entry!=NULL) {
        printf("%s ", entry);
        d=next_entry(d);
        entry=get_current(d);
    }
}

static int has_required(dictionary dict, ...) {
    va_list ap;
    char *s;
    char *entry;
    char msg[256];

    va_start(ap, dict);
    s=va_arg(ap, char *);
    while (s!=NULL) {
        entry=lookup_dictionary(dict, s);
        if (entry==NULL) {
            sprintf(msg, "Missing key:%s", s);
            report(msg);
            printf("Defined keys: ");
            show_dictionary(dict);
            printf("\n");
            va_end(ap);
            return 0;
        }
        s=va_arg(ap, char *);
    }
    va_end(ap);
    return 1;
}

void report(char *message) {
    error=1;
    fprintf(stderr, "%d:%d %s.\n", line, cpos, message);
}

void reportline(int line, int cpos, char *message) {
    error=1;
    fprintf(stderr, "%d:%d %s.\n", line, cpos, message);
}

int peekchar() {
    if (nextchar!=NONE) return nextchar;
    nextchar=fgetc(pfile);
    return nextchar;
}

void eatchar() {
    if (nextchar=='\n') {
        line++;
        cpos=1;
    } else cpos++;
    if (nextchar!=EOF)
        nextchar=NONE;
}

int readchar() {
    int c=peekchar();
    eatchar();
    return c;
}

static struct token newline() {
    struct token t;
    t.line=line;
    t.pos=cpos;
    t.type=NL;
    strcpy(t.value, "\n");
    eatchar();
    return t;
}

static struct token eof() {
    struct token t;
    t.line=line;
    t.pos=cpos;
    t.type=EOF;
    strcpy(t.value, "end of file");
    return t;
}

static void parse_comment() {
    //first character was #
    //read until newline
    int nc;
    nc=peekchar();
    while ((nc!='\n')&&(nc!=EOF)) {
        eatchar();
        nc=peekchar();
    }
}

static void skipwhite() {
    int nc=peekchar();
    while (isspace(nc)&&(nc!='\n')) {
        eatchar();
        nc=peekchar();
    }
}

static struct token parse_string() {
    //opening quote has been read
    //read until closing quote
    struct token t;
    int pos=0;
    int nc;
    t.line=line;
    t.pos=cpos;
    t.type=STRING;

    nc=readchar();
    nc=readchar();
    while (nc!='"') {
        if (nc==EOF) {
            report("Unexpected end of file while parsing a string literal");
            break;
        }
        t.value[pos++]=nc;
        nc=readchar();
    }
    t.value[pos]='\0';
    return t;
}

static struct token parse_value(struct token t, int pos) {
    int nc;
    t.type=VALUE;
    if (pos>=0) {
        nc=peekchar();
        while (isalnum(nc)||(nc==',')||(nc==';')||(nc=='-')) {
            t.value[pos++]=nc;
            eatchar();
            nc=peekchar();
        }
        t.value[pos]='\0';
    }
    nc=0;
    while (reserved[nc].token!=NULL) {
        if (strcmp(reserved[nc].token, t.value)==0) {
            t.type=reserved[nc].value;
            break;
        }
        nc++;
    }
    return t;
}

static struct token parse_id(struct token t, int pos) {
    int nc;
    t.type=ID;
    nc=peekchar();
    while (isalnum(nc)||(nc=='_')) {
        t.value[pos++]=nc;
        eatchar();
        nc=peekchar();
    }
    t.value[pos]='\0';
    nc=peekchar();
    if (nc!='=') {
        report("Expected an \"=\"");
    } else eatchar();

    return t;
}

static struct token parse_id_or_value() {
    struct token t;
    int pos=0;
    int nc;
    char message[256];
    t.line=line;
    t.pos=cpos;
    nc=peekchar();
    while (nc!=EOF) {
        if (nc=='_') return parse_id(t, pos);
        if (nc=='=') return parse_id(t, pos);
        if (nc==',') return parse_value(t, pos);
        if (nc==';') return parse_value(t, pos);
        if (isspace(nc)) {
            skipwhite();
            nc=peekchar();
            if (nc=='=') return parse_id(t, pos);
            t.value[pos]='\0';
            pos=-1;
            return parse_value(t, pos);
        }
        if (isalnum(nc)) {
            t.value[pos++]=nc;
        } else {
            sprintf(message, "Unexpected character \"%c\".", nc);
            report(message);
            eatchar();
            break;
        }
        t.value[pos]='\0';
        eatchar();
        nc=peekchar();
    }
    t.type=ID;
    report("Missing =");
    return t;
}

static struct token maptoken() {
    int skip=1;
    int nc;
    struct token t;
    char message[256];
    while (skip) {
        nc=peekchar();
        if (nc=='#')
            parse_comment();
        else if ((nc!='\n')&&isspace(nc))
            skipwhite();
        else skip=0;
    }
    if (nc=='\n') return newline();
    if (nc==EOF) return eof();
    if (nc=='"') return parse_string();
    if (nc==',') return parse_value(t,0);
    if (nc==';') return parse_value(t,0);
    if (nc=='_') return parse_value(t,0);
    if (nc=='-') return parse_value(t,0);
    if (isdigit(nc)) return parse_value(t,0);
    if (isalnum(nc)) return parse_id_or_value();
    sprintf(message, "Unexpected character \"%c\".", nc);
    report(message);
    t.type=ERROR;
    t.line=line;
    t.pos=cpos;
    t.value[0]=nc;
    t.value[1]='\0';
    eatchar();
    return t;
}

struct token (*tokenizer)()=NULL;
static struct token nexttoken;
struct token peektoken() {
    if (nexttoken.type!=NONE) return nexttoken;
    nexttoken=tokenizer();
    return nexttoken;
}

void eattoken() {
    if (nexttoken.type!=EOF) nexttoken.type=NONE;
}

struct token readtoken() {
    struct token t;
    t=peektoken();
    eattoken();
    return t;
}

void init_tokenizer() {
    nextchar=NONE;
    nexttoken.type=NONE;
    line=1;
    cpos=1;
}

static int known_key(char *s) {
    int i;
    i=0;
    while (known_keys[i]!=NULL) {
        if (strcmp(known_keys[i], s)==0) return 1;
        i++;
    }
    return 0;
}

static void parse_valuepairs() {
    struct token key;
    struct token value;
    char message[256];
    key=readtoken();
    while ((key.type!=NL)||(key.type!=EOF)) {
        if (key.type==NL) break;
        if (key.type==EOF) break;
        if (key.type==ID) {
            if (!known_key(key.value)) {
                sprintf(message, "Unknown key \"%s\"", key.value);
                reportline(key.line, key.pos, message);
            }
            value=readtoken();
            if ((value.type==STRING)||(value.type==VALUE)) {
                dict=add_entry(dict, key.value, value.value);
            } else {
                sprintf(message, "Unexpected token \"%s\"", value.value);
                reportline(value.line, value.pos, message);
            }
        } else {
            sprintf(message, "Unexpected token \"%s\"", key.value);
            reportline(key.line, key.pos, message);
        }
        key=readtoken();
    }
}

static void parse_shift() {
    struct token t;
    t=readtoken();
    parse_valuepairs();
    //printf("shift ");
    //show_dictionary(dict);
    //printf("\n");
    id=lookup_dictionary(dict, "id");
    vendor=lookup_dictionary(dict, "vendor");
    product=lookup_dictionary(dict, "product");
    src=lookup_dictionary(dict, "src");
    if ((id==NULL)&&((vendor==NULL)||(product==NULL))) {
        reportline(t.line, t.pos, "Must have id, or vendor and product");
    } else {
        if (has_required(dict, "src", NULL)) {
            map.program=PROGRAM_BUTTON_REMAP;
            if (id!=NULL)
                map.joystick=numeric(id);
            else
                map.joystick=255;
            map.vendor=numeric(vendor);
            map.product=numeric(product);
            map.srcbutton=numeric(src);
            map.type=TYPE_SHIFT;
            map.flags=FLAG_NONE;
            buttons[nbuttons]=map;
            nbuttons++;
        }
    }
    free_dictionary(dict);
    dict=NULL;
}

static void parse_script() {
    struct token t;
    t=readtoken();
    parse_valuepairs();
    //printf("script ");
    //show_dictionary(dict);
    //printf("\n");
    id=lookup_dictionary(dict, "id");
    vendor=lookup_dictionary(dict, "vendor");
    product=lookup_dictionary(dict, "product");
    device=lookup_dictionary(dict, "device");
    if ((id==NULL)&&((vendor==NULL)||(product==NULL))) {
        reportline(t.line, t.pos, "Must have id, or vendor and product");
    } else {
        if (has_required(dict, "device", NULL)) {
            if (id!=NULL)
                script.id=numeric(id);
            else
                script.id=-1;
            script.vendor=numeric(vendor);
            script.product=numeric(product);
            script.device=numeric(device);
            scriptassign[nscript]=script;
            nscript++;
        }
    }
    free_dictionary(dict);
    dict=NULL;
}

static void parse_button() {
    struct token t;
    int num;
    t=readtoken();
    parse_valuepairs();
    id=lookup_dictionary(dict, "id");
    vendor=lookup_dictionary(dict, "vendor");
    product=lookup_dictionary(dict, "product");
    src=lookup_dictionary(dict, "src");
    target=lookup_dictionary(dict, "target");
    button=lookup_dictionary(dict, "button");
    axis=lookup_dictionary(dict, "axis");
    device=lookup_dictionary(dict, "device");
    flags=lookup_dictionary(dict, "flags");
    speed=lookup_dictionary(dict, "speed");
    if ((id==NULL)&&((vendor==NULL)||(product==NULL))) {
        reportline(t.line, t.pos, "Must have id, or vendor and product");
    } else {
        if (has_required(dict, "src", "target", NULL)) {
            map.program=PROGRAM_BUTTON_REMAP;
            if (id!=NULL)
                map.joystick=numeric(id);
            else
                map.joystick=255;
            map.vendor=numeric(vendor);
            map.product=numeric(product);
            base=get_device(target);
            map.device=base+(numeric(device)&0xF);
            map.type=get_type(target,dict);
            if (base==DEVICE_JOYSTICK) {
                num=numeric(device);
                if (num>8) {
                    report("Maximum of 8 joysticks allowed");
                } else {
                    if (num>=njoysticks) {
                        njoysticks=num+1;
                    }
                    if (map.type==TYPE_AXIS) {
                        if (joysticks[num].axes<=numeric(axis))
                            joysticks[num].axes=numeric(axis)+1;
                    }
                    if (map.type==TYPE_BUTTON) {
                        if (joysticks[num].buttons<=numeric(button))
                            joysticks[num].buttons=numeric(button)+1;
                    }
                }
            }
            map.srcbutton=numeric(src)+BTN_JOYSTICK;
            map.speed=numeric(speed);
            if (map.speed==0)
                map.speed=8;
            map.flags=parse_flags(flags);
            if (button!=NULL)
                parse_sequence(map.sequence, button, base, map.type);
            else if (axis!=NULL)
                parse_sequence(map.sequence, axis, base, map.type);
            buttons[nbuttons]=map;
            nbuttons++;
            if (!((map.flags&FLAG_PRESS)||(map.flags&FLAG_RELEASE))&&
                (map.sequence[1]==SEQUENCE_DONE)) {
                if (map.sequence[0]&RELEASEMASK) {
                    map.sequence[0]&=~RELEASEMASK;
                } else {
                    map.sequence[0]|=RELEASEMASK;
                }
                map.flags|=FLAG_RELEASE;
                buttons[nbuttons]=map;
                nbuttons++;
            }
        }
    }
    free_dictionary(dict);
    dict=NULL;
}

static void parse_axis() {
    struct token t;
    int num;
    t=readtoken();
    parse_valuepairs();
    //printf("axis ");
    //show_dictionary(dict);
    //printf("\n");
    id=lookup_dictionary(dict, "id");
    vendor=lookup_dictionary(dict, "vendor");
    product=lookup_dictionary(dict, "product");
    src=lookup_dictionary(dict, "src");
    target=lookup_dictionary(dict, "target");
    axis=lookup_dictionary(dict, "axis");
    plus=lookup_dictionary(dict, "plus");
    minus=lookup_dictionary(dict, "minus");
    device=lookup_dictionary(dict, "device");
    flags=lookup_dictionary(dict, "flags");
    min=lookup_dictionary(dict, "min");
    max=lookup_dictionary(dict, "max");
    deadzone=lookup_dictionary(dict, "deadzone");
    speed=lookup_dictionary(dict, "speed");
    if ((id==NULL)&&((vendor==NULL)||(product==NULL))) {
        reportline(t.line, t.pos, "Must have id, or vendor and product");
    } else {
        if (has_required(dict, "src", "target", NULL)) {
            amap.program=PROGRAM_AXIS_REMAP;
            if (id!=NULL)
                amap.joystick=numeric(id);
            else
                amap.joystick=255;
            amap.vendor=numeric(vendor);
            amap.product=numeric(product);
            amap.srcaxis=numeric(src);
            base=get_device(target);
            amap.device=base+(numeric(device)&0xF);
            amap.type=get_type(target,dict);
            amap.saved_value = 0;
            if  (base==DEVICE_JOYSTICK) {
                num=numeric(device);
                if (num>8) {
                    report("Maximum of 8 joysticks allowed");
                } else {
                    if (num>=njoysticks) {
                        njoysticks=num+1;
                    }
                    if (amap.type==TYPE_AXIS) {
                        if (joysticks[num].axes<=numeric(axis))
                            joysticks[num].axes=numeric(axis)+1;
                    }
                    if (amap.type==TYPE_BUTTON) {
                        if (joysticks[num].buttons<=numeric(plus))
                            joysticks[num].buttons=numeric(plus)+1;
                        if (joysticks[num].buttons<=numeric(minus))
                            joysticks[num].buttons=numeric(minus)+1;
                    }
                }
            }
            if ((base==DEVICE_JOYSTICK)&&(amap.type==TYPE_BUTTON)) {
                base=BTN_JOYSTICK;
            } else if ((base==DEVICE_MOUSE)&&(amap.type==TYPE_BUTTON)) {
                base=BTN_MOUSE;
            } else base=0;
            parse_sequence(amap.plus, plus, base, amap.type);
            parse_sequence(amap.minus, minus, base, amap.type);
            amap.axis=numeric(axis);
            amap.flags=parse_flags(flags);
            amap.min=numeric(min);
            amap.max=numeric(max);
            amap.deadzone=numeric(deadzone);
            amap.speed=numeric(speed);
            if (amap.speed==0)
                amap.speed=32767;
            axes[naxes]=amap;
            naxes++;
        }
    }
    free_dictionary(dict);
    dict=NULL;
}

static void parse_joystick() {
    struct token t;
    char *axes; char *buttons;
    int num;
    t=readtoken();
    parse_valuepairs();
    axes=lookup_dictionary(dict, "axes");
    buttons=lookup_dictionary(dict, "buttons");
    device=lookup_dictionary(dict, "device");
    if (device==NULL) {
        reportline(t.line, t.pos, "Must have device");
    } else {
        num=numeric(device);
        if ((num<0)||(num>7)) reportline(t.line, t.pos, "Joystick must be 0-7");
        //printf("joystick ");
        //show_dictionary(dict);
        //printf("\n");
        if (num>=njoysticks) njoysticks=num+1;
        if (axes!=NULL)
            joysticks[num].axes=numeric(axes);
        else
            joysticks[num].axes=0;
        if (buttons!=NULL)
            joysticks[num].buttons=numeric(buttons);
        else
            joysticks[num].buttons=0;
    }
    free_dictionary(dict);
    dict=NULL;
}

static void parse_code() {
    struct token t;
    t=readtoken();
    t=readtoken();
    //printf("code ");
    if (t.type!=STRING) {
        reportline(t.line, t.pos, "String expected");
    } else strcpy(compile, t.value);
    t=readtoken();
    while ((t.type!=NL)&&(t.type!=EOF)) {
        reportline(t.line, t.pos, "No further token expected on this line");
    }
    //printf("\"%s\"\n", compile);
}

static void parse_joysticks() {
    struct token t;
    int num;
    t=readtoken();
    t=readtoken();
    printf("joysticks ");
    if (t.type!=VALUE) {
        reportline(t.line, t.pos, "Value expected");
    } else num=numeric(t.value);
    t=readtoken();
    while ((t.type!=NL)&&(t.type!=EOF)) {
        reportline(t.line, t.pos, "No further token expected on this line");
    }
    if (num<0) reportline(t.line, t.pos, "Only positive numbers allowed");
    if (num>8) reportline(t.line, t.pos, "Maximum 8 joysticks allowed");
    njoysticks=num;
    printf("\"%d\"\n", num);
}

static void parse_lines();
static void parse_include() {
    struct token t;
    char include[256]="";
    FILE *backup;
    t=readtoken();
    t=readtoken();
    //printf("include ");
    if (t.type!=STRING) {
        reportline(t.line, t.pos, "String expected");
    } else strcpy(include, t.value);
    t=readtoken();
    while ((t.type!=NL)&&(t.type!=EOF)) {
        reportline(t.line, t.pos, "No further token expected on this line");
    }
    //printf("\"%s\"\n", include);
    backup=pfile;
    pfile=fopen(include, "r");
    if (pfile==NULL) reportline(t.line, t.pos, "Could not find specified file");
    else parse_lines();
    pfile=backup;
}

static void parse_lines() {
    struct token t;
    char message[256];
    t=peektoken();
    while (t.type!=EOF) {
        if (t.type==NL) {
            t=readtoken();
            //do nothing
        } else if (t.type==EOF) {
            t=readtoken();
            //do nothing
        } else if (t.type==CODE) {
            parse_code();
        } else if (t.type==BUTTON) {
            parse_button();
        } else if (t.type==AXIS) {
            parse_axis();
        } else if (t.type==SHIFT) {
            parse_shift();
        } else if (t.type==SCRIPT) {
            parse_script();
        } else if (t.type==JOYSTICK) {
            parse_joystick();
        } else if (t.type==JOYSTICKS) {
            parse_joysticks();
        } else if (t.type==INCLUDE) {
            parse_include();
        } else {
            t=readtoken();
            error=1;
            if (t.type!=ERROR) {
                sprintf(message, "Unexpected token \"%s\"", t.value);
                reportline(t.line, t.pos, message);
                while ((t.type!=EOF)&&(t.type!=NL)) {
                    t=readtoken();
                }
            }
        }
        t=peektoken();
    }
}

int parse_map(void) {
    pfile=fmap;
    tokenizer=maptoken;
    init_tokenizer();
    parse_lines();
    free_dictionary(dict);
    dict=NULL;
    if (fmap!=stdin) fclose(fmap);
    if (strlen(compile)>0) {
        fprintf(stderr, "parsing program %s\n", compile);
        if (parse_program(compile, &program)!=0) error=1;
    }
    return error;
}

