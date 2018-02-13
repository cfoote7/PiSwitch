%{
#include <string.h>
#include <math.h>
#include <stdio.h>

static int lineno=1;
%}

digit	[0-9]
id	[a-zA-Z_][A-Za-z0-9_]*
comment [#].*[\n]
white	[ \r\t]
newline	[\n]
int  	{digit}+
float	{int}"."{int}
num	[\+\-]?({float}|{int})
value	{int}
string  \"[^\"]*\"
%%
if		{ return IF;}
else		{ return ELSE;}
while		{ return WHILE;}
delay		{ return DELAY;}
wait		{ return WAIT;}
var		{ return VAR;}
signal		{ return PSIGNAL;}
press		{ return PRESS;}
release		{ return RELEASE;}
halt		{ return PHALT;}
thread		{ return PTHREAD;}
{comment}	{ lineno++;}
{white}		{}
{newline}	{ lineno++;}
{id}		{ 
			yytext[strlen(yytext)-1]='\0';
			strcpy(yylval.string,yytext);
			return ID;
		}
{string}	{ 	 
			yytext[strlen(yytext)-1]='\0';
			strcpy(yylval.string,yytext+1);
			return STRING; 
		}
{value}		{ 
			strcpy(yylval.string,yytext);
			return INT;
		}
.		{ return yytext[0];}
%%

int yywrap(void) {
	return 1;
}

void yyerror(char *s) {
	printf("Parse error(line %d):" , lineno);
	printf("%s\n", s);
}
