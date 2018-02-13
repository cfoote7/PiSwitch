%{
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "dictionary.h"
#include "dictionary.c"
#include "program.h"
#include "keys.h"
#include "validkeys.h"

static dictionary dict=NULL;
static struct program_button_remap map;
static struct program_axis_remap amap;
static struct program_code program;
#define MAX_ASSIGN 1024
static struct program_button_remap buttons[MAX_ASSIGN];
static struct program_axis_remap axes[MAX_ASSIGN];
static int parse_err=0;
static int nbuttons=0;
static int naxes=0;
static int base=0;

static char *id, *vendor, *product, *src, *target, *button, *device, *flags, *axis, *plus, *minus; 
static char compile[256]="";

int ishex(char *s) {
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

int isnum(char *s) {
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
	if (s==NULL) return 0;
	if (ishex(s)) {
		r=strtol(s, NULL, 16);
	} else if (isnum(s)) {
		r=strtol(s, NULL, 10);
	} else {
		printf("expected a number, got %s instead\n", s);
		return 0;
	}
	return r;
}

int get_device(char *s) {
	if (s==NULL) printf("device expected!\n");
	if (strcmp(s, "joyaxis")==0) return DEVICE_JOYSTICK;
	if (strcmp(s, "joybtn")==0) return DEVICE_JOYSTICK;
	if (strcmp(s, "joystick")==0) return DEVICE_JOYSTICK;
	if (strcmp(s, "kbd")==0) return DEVICE_KBD;
	if (strcmp(s, "mouse")==0) return DEVICE_MOUSE;
	printf("Expecting a device type:joyaxis,joybtn,joystick,kbd,mouse. Found %s instead.\n", s);
	return 255;
}

int get_type(char *s, dictionary d) {
	char *button, *axis;
	if (s==NULL) return 255;
	if (strcmp(s, "joyaxis")==0) return TYPE_AXIS;
	if (strcmp(s, "joybtn")==0) return TYPE_BUTTON;
	button=lookup_dictionary(d, "button");
	axis=lookup_dictionary(d, "axis");
	if ((button!=NULL)&&(axis!=NULL)) {
		printf("Only one of the keys 'button' and 'axis' may be specified.\n");
		return 255;
	}
	if (button!=NULL) return TYPE_BUTTON;
	if (axis!=NULL) return TYPE_AXIS;
	return 255;
}

int parse_flags(char *s) {
	int flags=FLAG_NONE;
	char *p=s;
	int more=1;
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
		else printf("Unknown flag %s.\n", s);
		s=p+1;
	}
	return flags;
}

void parse_sequence(__s16 *sequence, char *s, int base, int type) {
	char *p;
	int releaseflag=0;
	int value;
	int i;
	int n=0;
	if (s==NULL) return;
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
					printf("Unknown key %s\n", s);
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

void show_dictionary(dictionary dict) {
	dictionary d=dict;
	char *entry;
	entry=get_current(d);
	while (entry!=NULL) {
		printf("%s ", entry);
		d=next_entry(d);
		entry=get_current(d);
	}
}

int has_required(dictionary dict, ...) {
	va_list ap;
	char *s;
	char *entry;

	va_start(ap, dict);
	s=va_arg(ap, char *);
	while (s!=NULL) {
		entry=lookup_dictionary(dict, s);
		if (entry==NULL) {
			printf("Missing key:%s\n", s);
			va_end(ap);
			return 0;
		}
		s=va_arg(ap, char *);
	}
	va_end(ap);
	return 1;
}
%}

%union {
	char string[256];
}

%token SHIFT 
%token BUTTON 
%token AXIS
%token CODE
%token <string> ID 
%token <string> VALUE
%token <string> STRING
%token NL 
%%

program:	lines						{}
;

lines:		/* empty */
		|lines axis					{}
		|lines code					{}
		|lines button					{}
		|lines shift					{}
		|lines NL					{}
		|error NL					{ parse_err=1; printf("error before newline!\n"); }
;

shift:		SHIFT valuepairs NL				{ 
    									printf("shift ");
									show_dictionary(dict);
									printf("\n");
									id=lookup_dictionary(dict, "id");
									vendor=lookup_dictionary(dict, "vendor");
									product=lookup_dictionary(dict, "product");
									src=lookup_dictionary(dict, "src");
									if ((id==NULL)&&((vendor==NULL)||(product==NULL))) {
										printf("Must have id, or vendor and product\n");
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
;

button:		BUTTON valuepairs NL				{ 
									id=lookup_dictionary(dict, "id");
									vendor=lookup_dictionary(dict, "vendor");
									product=lookup_dictionary(dict, "product");
									src=lookup_dictionary(dict, "src");
									target=lookup_dictionary(dict, "target");
									button=lookup_dictionary(dict, "button");
									axis=lookup_dictionary(dict, "axis");
									device=lookup_dictionary(dict, "device");
									flags=lookup_dictionary(dict, "flags");
									if ((id==NULL)&&((vendor==NULL)||(product==NULL))) {
										printf("Must have id, or vendor and product\n");
									} else {
										if (has_required(dict, "src", "target", NULL)) {
	    										printf("button ");
											show_dictionary(dict);
											printf("\n");
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
											map.srcbutton=numeric(src)+BTN_JOYSTICK;
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
;

axis:		AXIS valuepairs NL				{ 
    									printf("axis ");
									show_dictionary(dict);
									printf("\n");
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
									if ((id==NULL)&&((vendor==NULL)||(product==NULL))) {
										printf("Must have id, or vendor and product\n");
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
											if ((base==DEVICE_JOYSTICK)&&(amap.type==TYPE_BUTTON)) {
												base=BTN_JOYSTICK;
											} else if ((base==DEVICE_MOUSE)&&(amap.type==TYPE_BUTTON)) {
												base=BTN_MOUSE;
											} else base=0;
											amap.plus=numeric(plus)+base;
											amap.minus=numeric(minus)+base;
											amap.axis=numeric(axis);
											amap.flags=parse_flags(flags);
											axes[naxes]=amap;
											naxes++;
										}
									}
      									free_dictionary(dict);
									dict=NULL;
								}
;

code:		CODE STRING NL					{
    									printf("code %s\n", $2);
									strcpy(compile, $2);
   								}
;

valuepairs:	/* empty */
	  	|valuepairs valuepair				{}
;

valuepair:	ID VALUE					{
	 								dict=add_entry(dict, $1, $2);
	 							}
	 	|ID STRING					{
									dict=add_entry(dict, $1, $2);
								}
;

%%

#include "lex.map.c"

#include "programparse.h"
int main(int argc, char *argv[]) {
	int i, fd;
	char devname[256];
	int vendor=0;
	int product=0;
	struct program_button_remap map;
	struct program_axis_remap amap;
	
	++argv, --argc;  /* skip over program name */
	if ( argc > 0 )
		yyin = fopen( argv[0], "r" );
	else
		yyin = stdin;

	fd=open("/dev/mapper", O_RDWR);
	if (fd<0) {
		perror("Error programming joystick");
		return 0;
	}

	for (i=0; i<128; i++) {
		vendor=i;
		if (ioctl(fd, JOYMAP_VENDOR, &vendor)<0) {
			continue;
		}
		product=i;
		if (ioctl(fd, JOYMAP_PRODUCT, &product)<0) {
			continue;
		}
		sprintf(devname, " ");
		devname[0]=i;
		if (ioctl(fd, JOYMAP_NAME, devname)<0) {
			continue;
		}
		fprintf(stderr, "%d: Vendor=%x Product=%x [%s]\n", i, vendor, product, devname);	
	}
	yyparse();
	if (strlen(compile)>0) {
		if (parse_program(compile, &program)!=0) parse_err=1;
	}
	if (!parse_err) {
		ioctl(fd, JOYMAP_UNMAP);
		printf("%d button assignments.\n", nbuttons);
		for (i=0; i<nbuttons; i++) 
			write(fd, (char *)&buttons[i], sizeof(map));
		printf("%d axes assignments.\n", naxes);
		for (i=0; i<naxes; i++) 
			write(fd, (char *)&axes[i], sizeof(amap));
		ioctl(fd, JOYMAP_MAP);
	} else {
		printf("Error in map file, nothing done.\n");
	}	
	close(fd);
}
