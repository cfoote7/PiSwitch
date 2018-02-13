#include "program.h"
#include "program.tab.c"

int parse_program(char *program, struct program_code *code) {
	yyin=fopen(program, "r");
	if (yyin==NULL) {
		printf("File not found %s.\n", program);
		return 1;
	} else
		yyparse();
	return program_parse_err;
}
