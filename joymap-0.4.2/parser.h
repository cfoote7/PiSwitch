#ifndef __parser
#define __parser

#include <stdio.h>
#include <stdlib.h>
#include "program.h"

#define NONE	-255
#define SHIFT	256
#define BUTTON	257
#define AXIS	258
#define CODE	259
#define STRING	260
#define ID	261
#define VALUE	262
#define IF	263
#define ELSE	264
#define WHILE	265
#define DELAY	266
#define WAIT	267
#define VAR	268
#define PRESS	269
#define RELEASE	270
#define PHALT	271
#define PTHREAD	272
#define NL 	273	
#define INCLUDE	274	
#define PSIGNAL	275	
#define PEQ	276
#define PNE	277
#define PLE	278
#define PGE	279
#define PAND	280
#define POR	281
#define PLUSE	282
#define MINUSE	283
#define MULTE	284
#define DIVE	285
#define PINC	286
#define PDEC	287
#define SCRIPT	288
#define JOYSTICK 289
#define JOYSTICKS 290
#define GLOBALVAR 291
#define ERROR	10000

#define MAX_SYMBOLS	512

struct token {
	int line;
	int pos;
	int type;
	char value[256];
};

struct reserved {
	char *token;
	int value;
};

struct scriptmap {
	int id;
	int vendor;
	int product;
	int device;
};

extern FILE *fmap;
extern FILE *pfile;
extern int line;
extern int cpos;
extern struct token (*tokenizer)();
void report(char *message);
void reportline(int line, int cpos, char *message);
int peekchar();
void eatchar();
int readchar();
struct token peektoken();
void eattoken();
struct token readtoken();
void init_tokenizer();
int numeric(char *s);
int parse_map(void);
int parse_program(char *program, struct program_code *code);

extern struct program_code program;
#define MAX_ASSIGN 1024
extern struct program_button_remap buttons[MAX_ASSIGN];
extern struct program_axis_remap axes[MAX_ASSIGN];
extern struct scriptmap scriptassign[MAX_ASSIGN];
extern int nbuttons;
extern int naxes;
extern int nscript;
extern int njoysticks;

struct joystick {
	int axes;
	int buttons;
};

extern struct joystick joysticks[8];

#endif
