%{
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "program.h"
#include "keys.h"
#include "validkeys.h"

static int program_parse_err=0;
%}

%union {
	char string[256];
	int num;
}

%token IF 
%token ELSE 
%token WHILE 
%token DELAY
%token WAIT
%token VAR 
%token PSIGNAL
%token PRESS 
%token RELEASE
%token PHALT 
%token PTHREAD 
%token <string> ID 
%token <num> INT
%token <string> STRING
%%

program:	vardecs statements				{}
;

vardecs:	/* empty */
		|vardecs vardec					{}
;

statements:	/* empty */
		|statements statement				{}
;

vardec:		VAR ID ";"					{}
      		|VAR ID "[" INT "]" ";"				{}
;

statement:	ifstmnt						{}
	 	|whilestmnt					{}
		|delay						{}
		|wait						{}
		|signal						{}
		|press						{}
		|release					{}
		|halt						{}
		|thread						{}
		|assign						{}
		|empty						{}
		|incr						{}
		|decr						{}
		|error ";"					{}
;

empty:		";"
;

ifstmnt:	IF "(" expr ")" "{" statements "}" elsepart	{}
       		| IF "(" expr ")" statement elsepart		{} 
;

elsepart:	ELSE "{" statements "}"				{}
		| ELSE statement				{}
		| /*empty*/					{}
;

whilestmnt:	WHILE "(" expr ")" "{" statements "}"		{}
	  	| WHILE "(" expr ")" statement			{}
;

delay:		DELAY "(" expr ")" ";"				{}
;

wait:		WAIT "(" expr ")" ";"				{}
;

signal:		PSIGNAL "(" expr ")" ";"			{}
;

press:		PRESS "(" expr ")" ";"				{}
;

release:	RELEASE "(" expr ")" ";"			{}
;

incr:		ID"++;"						{}
;

decr:		ID"--;"						{}
;

halt:		PHALT ";"					{}
;

thread:		PTHREAD "{" statements "}"			{}
      		| PTHREAD ID "{" statements "}"			{}
;

assign:		lvalue "=" expr ";"				{}
      		|lvalue "+" "=" expr ";"			{}
      		|lvalue "-" "=" expr ";"			{}
      		|lvalue "*" "=" expr ";"			{}
      		|lvalue "/" "=" expr ";"			{}
;

lvalue:		ID						{}
      		|ID "[" expr "]"				{}
;

expr:		logic						{}
;

arithterm:	arithterm "+" term				{}
    		|arithterm "-" term				{}
		|term						{}
;

term:		term "*" factor					{}
    		| term "/" factor				{}
		| term "%" factor				{}
		| factor					{}
;

factor:		factor "&" "&" unit				{}
		| factor "|" "|" unit				{}
		| unit						{}
;

logic:		logic "=" "=" arithterm				{}
     		| logic "!" "=" arithterm			{}
     		| logic "<" "=" arithterm			{}
     		| logic ">" "=" arithterm			{}
     		| logic "<" arithterm				{}
     		| logic ">" arithterm				{}
		| arithterm					{}
;

unit:		ID						{}
    		| ID "[" expr "]"				{}
    		| "j" "s" INT "." "A" "[" expr "]"		{}
    		| "j" "s" INT "." "B" "[" expr "]"		{}
		| INT						{}
		| "-" unit					{}
		| "+" unit					{}
		| "!" unit					{}
		| "(" expr ")"					{}
%%

#include "lex.program.c"

