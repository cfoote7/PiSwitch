#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "program.h"
#include "parser.h"
#include "validkeys.h"

static int program_parse_err=0;

static struct reserved reserved[]={
    {"if", IF},
    {"else", ELSE},
    {"while", WHILE},
    {"delay", DELAY},
    {"wait", WAIT},
    {"signal", PSIGNAL},
    {"var", VAR},
    {"global", GLOBALVAR},
    {"press", PRESS},
    {"release", RELEASE},
    {"halt", PHALT},
    {"thread", PTHREAD},
    {NULL, 0}
};

//register assignments
static int assigned=RESERVED;

struct symbol {
    char name[256];
    int r; //assigned register
    int num; //number of entries (for an array)
    int type; //GP/CODEB/CODEA or JS
};

#define JS_FLAG (JS<<5)
#define GP_FLAG (GP<<5)
#define GLOBAL_FLAG (GLOBAL<<5)
#define CONST_FLAG (CONST<<5)
#define CODEA_FLAG (CODEA<<5)
#define CODEB_FLAG (CODEB<<5)
struct symbol symbol_table[MAX_SYMBOLS]={
    {"js0.a", 0x0000+0x1000, 64, JS_FLAG},
    {"js0.b", 0x0000, 128, JS_FLAG},
    {"js1.a", 0x0100+0x1000, 64, JS_FLAG},
    {"js1.b", 0x0100, 128, JS_FLAG},
    {"js2.a", 0x0200+0x1000, 64, JS_FLAG},
    {"js2.b", 0x0200, 128, JS_FLAG},
    {"js3.a", 0x0300+0x1000, 64, JS_FLAG},
    {"js3.b", 0x0300, 128, JS_FLAG},
    {"js4.a", 0x0400+0x1000, 64, JS_FLAG},
    {"js4.b", 0x0400, 128, JS_FLAG},
    {"js5.a", 0x0500+0x1000, 64, JS_FLAG},
    {"js5.b", 0x0500, 128, JS_FLAG},
    {"js6.a", 0x0600+0x1000, 64, JS_FLAG},
    {"js6.b", 0x0600, 128, JS_FLAG},
    {"js7.a", 0x0700+0x1000, 64, JS_FLAG},
    {"js7.b", 0x0700, 128, JS_FLAG},
    {"js8.a", 0x0800+0x1000, 64, JS_FLAG},
    {"js8.b", 0x0800, 128, JS_FLAG},
    {"js9.a", 0x0900+0x1000, 64, JS_FLAG},
    {"js9.b", 0x0900, 128, JS_FLAG},
    {"js10.a", 0x0A00+0x1000, 64, JS_FLAG},
    {"js10.b", 0x0A00, 128, JS_FLAG},
    {"js11.a", 0x0B00+0x1000, 64, JS_FLAG},
    {"js11.b", 0x0B00, 128, JS_FLAG},
    {"js12.a", 0x0C00+0x1000, 64, JS_FLAG},
    {"js12.b", 0x0C00, 128, JS_FLAG},
    {"js13.a", 0x0D00+0x1000, 64, JS_FLAG},
    {"js13.b", 0x0D00, 128, JS_FLAG},
    {"js14.a", 0x0E00+0x1000, 64, JS_FLAG},
    {"js14.b", 0x0E00, 128, JS_FLAG},
    {"js15.a", 0x0F00+0x1000, 64, JS_FLAG},
    {"js15.b", 0x0F00, 128, JS_FLAG},
    {"a", 0, MAX_AXES, CODEA_FLAG},
    {"b", 0, MAX_BUTTONS, CODEB_FLAG},
    {"firstscan", 0, 1, GP_FLAG},
    {"clocktick", 1, 1, GP_FLAG},
    {"xrel", 2, 1, GP_FLAG},
    {"yrel", 3, 1, GP_FLAG},
    {"zrel", 4, 1, GP_FLAG},
    {"timestamp", 5, 1, GP_FLAG},
    {"currentmode", 6, 1, GP_FLAG},
};

static char threadnames[8][256];

static void name_thread(int thread, char *name) {
    if (thread>=8) {
        report("Too many threads");
        return;
    }
    strcpy(threadnames[thread], name);
}

static int get_thread(char *name) {
    int i;
    char msg[256];
    for (i=0; i<8; i++) {
        if (strcmp(name, threadnames[i])==0) return i;
    }
    sprintf(msg, "Unknown thread %s", name);
    report(msg);
    return 0;
}

static int query_thread(char *name) {
    int i;
    for (i=0; i<8; i++) {
        if (strcmp(name, threadnames[i])==0) return i;
    }
    return 0;
}

static int nsymbols=41;

static char *instructions[]={
    "HALT",
    "JZ",
    "JUMP",
    "JUMPREL",
    "ADD",
    "SUB",
    "MUL",
    "DIV",
    "LT",
    "LE",
    "GT",
    "GE",
    "EQ",
    "NEQ",
    "AND",
    "OR",
    "NOT",
    "PUSH",
    "POP",
    "DEC",
    "INC",
    "KEY",
    "SIGNAL",
    "THREAD",
    "YIELD",
    "NEG",
    "JOIN",
    "PUSHA",
    "POPA",
    "DECA",
    "INCA"
};

//for each instruction, does it have an operand
static char has_operand[]={
    0, /* HALT */
    1, /* JZ */
    1, /* JUMP */
    1, /* JUMPREL */
    0, /* ADD */
    0, /* SUB */
    0, /* MUL */
    0, /* DIV */
    0, /* LT */
    0, /* LE */
    0, /* GT */
    0, /* GE */
    0, /* EQ */
    0, /* NEQ */
    0, /* AND */
    0, /* OR */
    0, /* NOT */
    1, /* PUSH */
    1, /* POP */
    1, /* DEC */
    1, /* INC */
    0, /* KEY */
    0, /* SIGNAL */
    1, /* THREAD handles its own arguments */
    0, /* YIELD */
    0, /* NEG */
    1, /* JOIN */
    1, /* PUSHA */
    1, /* POPA */
    1, /* DECA */
    1, /* INCA */
};

static unsigned char *codeseg;
static int ip=0;
static void emit1(int instr) {
    codeseg[ip]=instr;
    ip++;
}

static void emitc(int instr, int c) {
    codeseg[ip]=instr|CONST_FLAG;
    ip++;
    *((unsigned short *)(codeseg+ip))=c;
    ip+=2;
}

static void emitaddress(int c) {
    *((unsigned short *)(codeseg+ip))=c;
    ip+=2;
}

static void emitt(int instr, int c) {
    codeseg[ip]=instr|GP_FLAG;
    ip++;
    *((unsigned short *)(codeseg+ip))=c;
    ip+=2;
}

static void emit2(int instr, int symbol, int value) {
    if (value!=0) {
        emitt(POP, OFSREG);
    }
    codeseg[ip]=instr|(symbol_table[symbol].type);
    ip++;
    *((unsigned short *)(codeseg+ip))=symbol_table[symbol].r;
    ip+=2;
}

void printcode() {
    int lip=0;
    int task;
    int type;
    int address;
    int js;
    int ret;
    while (lip<ip) {
        printf("%d: ", lip);
        task=codeseg[lip++];
        type=(unsigned char)task >> 5;
        task=task&31;
        if ((task>=0)&&(task<=INCA)) {
            if (has_operand[task]) {
                address=*((unsigned short *)(codeseg+lip));
                lip+=2;
            } else type=-1;
        } else type=-1;
        ret=-1;
        if (task==THREAD) {
            ret=*((unsigned short *)(codeseg+lip));
            lip+=2;
        }
        printf("%s ", instructions[task]);
        if (type==GP) {
            printf("r%d", address);
        }
        if (type==CONST) {
            printf("%d", address);
        }
        if (type==CODEA) {
            printf("A%d", address);
        }
        if (type==CODEB) {
            printf("B%d", address);
        }
        if (type==JS) {
            js=(address&0x0F00)>>8;
            type=address&0xF000;
            address=address&0x00FF;
            if (type)
                printf("js%d.A%d", js, address);
            else
                printf("js%d.B%d", js, address);
        }
        if (ret>0) printf(" ret=%d", ret);
        printf("\n");
    }
}

static void expect(int type, char *token) {
    struct token t;
    char msg[256];

    t=readtoken();
    if (t.type!=type) {
        sprintf(msg, "%s expected before %s", token, t.value);
        reportline(t.line, t.pos, msg);
    }
}

static int symbol_exists(char *sym) {
    int i;
    for (i=0; i<nsymbols; i++) {
        if (strcmp(symbol_table[i].name, sym)==0) return i;
    }
    return -1;
}

static void add_symbol(struct token symbol, int num, int global) {
    int i;
    char msg[256];

    if (nsymbols>=MAX_SYMBOLS) {
        reportline(symbol.line, symbol.pos, "Symbol table exhausted");
        return;
    }
    for (i=0; i<nsymbols; i++) {
        if (strcmp(symbol_table[i].name, symbol.value)==0) {
            sprintf(msg, "Duplicate symbol \"%s\"", symbol.value);
            reportline(symbol.line, symbol.pos, msg);
            return;
        }
    }
    strcpy(symbol_table[nsymbols].name, symbol.value);
    symbol_table[nsymbols].r=assigned;
    assigned+=num;
    if (assigned>=256) {
        reportline(symbol.line, symbol.pos, "Registers exhausted");
    }
    symbol_table[nsymbols].num=num;
    if (global) {
        symbol_table[nsymbols].type=GLOBAL_FLAG;
        printf("Adding global %s to symbols with register r%d\n", symbol.value, symbol_table[nsymbols].r);
    } else {
        symbol_table[nsymbols].type=GP_FLAG;
        printf("Adding %s to symbols with register r%d\n", symbol.value, symbol_table[nsymbols].r);
    }
    nsymbols++;
}

static void parse_comment() {
    //first character was #
    //read until newline
    int nc;
    nc=readchar();
    while ((nc!='\n')&&(nc!=EOF))
        nc=readchar();
}

static void skipwhite() {
    int nc=peekchar();
    while (isspace(nc)) {
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

static struct token parse_id() {
    struct token t;
    int pos=0;
    int nc;
    t.type=ID;
    t.line=line;
    t.pos=cpos;
    nc=peekchar();
    while (isalnum(nc)||(nc=='_')||(nc=='.')) {
        t.value[pos++]=nc;
        eatchar();
        nc=peekchar();
    }
    t.value[pos]='\0';

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

static struct token chartype(int nc) {
    struct token t;

    t.type=nc;
    t.line=line;
    t.pos=cpos;
    t.value[0]=nc; t.value[1]='\0';
    eatchar();
    return t;
}

static struct token eqchartype(int nc) {
    struct token t;
    int pc;

    t.type=nc;
    t.line=line;
    t.pos=cpos;
    t.value[0]=nc; t.value[1]='\0';
    eatchar();

    pc=peekchar();
    if (pc=='=') {
        t.value[1]='=';
        t.value[2]='\0';
        eatchar();
        switch(nc) {
            case '<': t.type=PLE; break;
            case '>': t.type=PGE; break;
            case '+': t.type=PLUSE; break;
            case '-': t.type=MINUSE; break;
            case '*': t.type=MULTE; break;
            case '/': t.type=DIVE; break;
            case '=': t.type=PEQ; break;
            case '!': t.type=PNE; break;
        }
    }
    if ((pc=='+')&&(nc=='+')) {
        t.value[1]='+';
        t.value[2]='\0';
        t.type=PINC;
        eatchar();
    }
    if ((pc=='-')&&(nc=='-')) {
        t.value[1]='-';
        t.value[2]='\0';
        t.type=PDEC;
        eatchar();
    }
    return t;
}

static struct token logchartype(int nc) {
    struct token t;
    int pc;

    t.type=nc;
    t.line=line;
    t.pos=cpos;
    t.value[0]=nc; t.value[1]='\0';
    eatchar();

    pc=peekchar();
    if ((pc=='&')&&(nc=='&')) {
        t.value[1]='+';
        t.value[2]='\0';
        t.type=PAND;
        eatchar();
    }
    if ((pc=='|')&&(nc=='|')) {
        t.value[1]='-';
        t.value[2]='\0';
        t.type=PAND;
        eatchar();
    }
    return t;
}
static struct token parse_num() {
    struct token t;
    int pos=0;
    int nc;
    t.type=VALUE;
    t.line=line;
    t.pos=cpos;
    nc=peekchar();
    while (isdigit(nc)||((pos==1)&&(nc=='x'))) {
        t.value[pos++]=nc;
        eatchar();
        nc=peekchar();
    }
    t.value[pos]='\0';
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

static struct token programtoken() {
    int skip=1;
    int nc;
    struct token t;
    char message[256];
    while (skip) {
        nc=peekchar();
        if (nc=='#')
            parse_comment();
        else if (isspace(nc))
            skipwhite();
        else skip=0;
    }
    if (nc==EOF) return eof();
    if (nc=='"') return parse_string();
    if (nc=='_') return parse_id();
    if (isalpha(nc)) return parse_id();
    if (isdigit(nc)) return parse_num();
    if (nc=='{') return chartype(nc);
    if (nc=='}') return chartype(nc);
    if (nc=='[') return chartype(nc);
    if (nc==']') return chartype(nc);
    if (nc=='(') return chartype(nc);
    if (nc==')') return chartype(nc);
    if (nc=='%') return chartype(nc);
    if (nc==';') return chartype(nc);
    if (nc==',') return chartype(nc);
    if (nc=='!') return eqchartype(nc);
    if (nc=='=') return eqchartype(nc);
    if (nc=='+') return eqchartype(nc);
    if (nc=='-') return eqchartype(nc);
    if (nc=='*') return eqchartype(nc);
    if (nc=='/') return eqchartype(nc);
    if (nc=='<') return eqchartype(nc);
    if (nc=='>') return eqchartype(nc);
    if (nc=='&') return logchartype(nc);
    if (nc=='|') return logchartype(nc);
    sprintf(message, "Unexpected character \"%c\".", nc);
    eatchar();
    report(message);
    t.type=ERROR;
    t.line=line;
    t.pos=cpos;
    t.value[0]=nc; t.value[1]='\0';
    return t;
}

static void seek_endstmt() {
    struct token t;
    t=peektoken();
    while ((t.type!=';')&&(t.type!='}')&&(t.type!=EOF)) {
        eattoken();
        t=peektoken();
    }
    eattoken();
}

static void seek_nextvar() {
    struct token t;
    t=peektoken();
    while ((t.type!=';')&&(t.type!=',')&&(t.type!=EOF)) {
        eattoken();
        t=peektoken();
    }
}

static int threaddepth=0;
static int threadnum=0;

static void parse_expression();
static void parse_statement();
static void parse_statements(int block);
static void parse_unit() {
    struct token t;
    struct token id;
    int sym;
    char msg[256];
    t=readtoken();
    switch (t.type) {
        case ID:
            id=t;
            sym=symbol_exists(id.value);
            if (sym<0) {
                sprintf(msg, "Unknown identifier %s", id.value);
                reportline(id.line, id.pos, msg);
            }
            t=peektoken();
            if (t.type=='[') {
                eattoken();
                parse_expression();
                expect(']', "]");
                emit2(PUSHA, sym, 1);
            } else
                emit2(PUSH, sym, 0);
            break;
        case VALUE:
            emitc(PUSH, numeric(t.value));
            break;
        case '-':
            parse_unit();
            emit1(NEG);
            break;
        case '+':
            parse_unit();
            break;
        case '!':
            parse_unit();
            emit1(NOT);
            break;
        case '(':
            parse_expression();
            expect(')', ")");
            break;
    }
}

static int is_bin_operator(int type) {
    if (type==PAND) return 1;
    if (type==POR) return 1;
    return 0;
}

static void parse_factor() {
    struct token t;
    parse_unit();

    t=peektoken();
    if (is_bin_operator(t.type)) {
        eattoken();
        parse_factor();
        switch (t.type) {
            case PAND:
                emit1(AND);
                break;
            case POR:
                emit1(OR);
                break;
            default:
                break;
        }
    }
}

static int is_mdm_operator(int type) {
    if (type=='*') return 1;
    if (type=='/') return 1;
    if (type=='%') return 1;
    return 0;
}

static void parse_term() {
    struct token t;
    int x=8;
    int y=9;
    parse_factor();

    t=peektoken();
    while (is_mdm_operator(t.type)) {
        eattoken();
        parse_factor();
        switch (t.type) {
            case '*':
                emit1(MUL);
                break;
            case '/':
                emit1(DIV);
                break;
            case '%':
                emitt(POP,y);
                emitt(POP,x);
                emitt(PUSH,x);
                emitt(PUSH,y);
                emitt(PUSH,x);
                emitt(PUSH,y);
                emit1(DIV);
                emit1(MUL);
                emit1(SUB);
                break;
            default:
                break;
        }
        t=peektoken();
    }
}

static int is_pm_operator(int type) {
    if (type=='+') return 1;
    if (type=='-') return 1;
    return 0;
}

static void parse_arithterm() {
    struct token t;
    parse_term();

    t=peektoken();
    while (is_pm_operator(t.type)) {
        eattoken();
        parse_term();
        switch (t.type) {
            case '+':
                emit1(ADD);
                break;
            case '-':
                emit1(SUB);
                break;
            default:
                break;
        }
        t=peektoken();
    }
}

static int is_logic_operator(int type) {
    if (type=='<') return 1;
    if (type=='>') return 1;
    if (type==PEQ) return 1;
    if (type==PNE) return 1;
    if (type==PLE) return 1;
    if (type==PGE) return 1;
    return 0;
}

static void parse_logic() {
    struct token t;
    parse_arithterm();

    t=peektoken();
    if (is_logic_operator(t.type)) {
        eattoken();
        parse_logic();
        switch (t.type) {
            case '<': emit1(LT);
                break;
            case '>': emit1(GT);
                break;
            case PEQ: emit1(EQ);
                break;
            case PNE: emit1(NEQ);
                break;
            case PGE: emit1(GE);
                break;
            case PLE: emit1(LE);
                break;
            default:
                break;
        }
    }
}

static void parse_expression() {
    parse_logic();
}

static void parse_condition() {
    expect('(', "(");
    parse_expression();
    expect(')', ")");
}

static void parse_var(int global) {
    struct token t;
    struct token id;
    int size;

    eattoken();
    id=peektoken();
    if (id.type==ID) {
        eattoken();
        t=peektoken();
        if (t.type=='[') {
            eattoken();
            t=peektoken();
            if (t.type!=VALUE) {
                reportline(t.line, t.pos, "Only constant array dimensions are allowed");
                size=1;
            } else size=numeric(t.value);
            if (t.type!=']') {
                eattoken();
                t=peektoken();
            }
            if (t.type!=']') {
                reportline(t.line, t.pos, "] expected");
                seek_nextvar();
            } else eattoken();
            add_symbol(id, size, global);
            t=peektoken();
            if (t.type==',') parse_var(global);
            else if (t.type==';') {
                eattoken();
                return;
            } else {
                reportline(t.line, t.pos, "Expected ;");
                seek_endstmt();
                return;
            }
        } else {
            add_symbol(id, 1, global);
            t=peektoken();
            if (t.type==',') parse_var(global);
            else if (t.type==';') {
                eattoken();
                return;
            } else {
                reportline(t.line, t.pos, "Expected ;");
                seek_endstmt();
                return;
            }
        }
    } else {
        reportline(id.line, id.pos, "Expected an identifier");
        seek_endstmt();
    }
}

static void parse_declarations() {
    struct token t;

    t=peektoken();
    while ((t.type==VAR) || (t.type==GLOBALVAR)) {
        if (t.type == VAR)
            parse_var(0);
        else
            parse_var(1);
        t=peektoken();
    }
}

static void parse_if() {
    struct token t;
    int start, f=0, fin=0, before;

    parse_condition();
    start=ip; emitc(JZ, f);
    t=peektoken();
    if (t.type=='{') {
        eattoken();
        parse_statements(1);
        expect('}', "}");
    } else {
        parse_statement();
    }
    t=peektoken();
    if (t.type==ELSE) {
        before=ip;
        emitc(JUMP, fin);
        f=ip;
        eattoken();
        t=peektoken();
        if (t.type=='{') {
            eattoken();
            parse_statements(1);
            t=peektoken();
            expect('}', "}");
        } else {
            parse_statement();
        }
        fin=ip;
        ip=start;
        emitc(JZ, f);
        ip=before;
        emitc(JUMP, fin);
        ip=fin;
    } else {
        fin=ip;
        ip=start;
        emitc(JZ, fin);
        ip=fin;
    }
}

static void parse_while() {
    struct token t;
    int start, fin=0, cond;
    start=ip;
    parse_condition();
    cond=ip;
    emitc(JZ, fin);
    t=peektoken();
    if (t.type=='{') {
        eattoken();
        parse_statements(1);
        t=peektoken();
        expect('}', "}");
    } else {
        parse_statement();
    }
    emitc(JUMP, start);
    fin=ip;
    ip=cond;
    emitc(JZ, fin);
    ip=fin;
}

static void parse_delay() {
    struct token t;
    char msg[256];
    int before=0, after=0, jump=0, reg=0;
    t=readtoken();
    if (t.type!='(') {
        sprintf(msg, "( expected before %s", t.value);
        reportline(t.line, t.pos, msg);
        seek_endstmt();
        return;
    }
    if (threaddepth==0) {
        reportline(t.line, t.pos, "delay only valid in thread\n");
        parse_expression();
    } else {
        reg=assigned++;
        emitt(PUSH,TIMESTAMP);
        emitt(POP,reg);
        before=ip;
        emitt(PUSH,TIMESTAMP);
        emitt(PUSH,reg);
        emit1(SUB);
        parse_expression();
        emit1(LT);
        jump=ip;
        emitc(JZ,after);
        emit1(YIELD);
        emitc(JZ,before);
        after=ip;
        ip=jump;
        emitc(JZ,after);
        ip=after;
    }
    t=readtoken();
    if (t.type!=')') {
        sprintf(msg, "( expected before %s", t.value);
        reportline(t.line, t.pos, msg);
        seek_endstmt();
        return;
    }
    expect(';', ";");
}

static void parse_signal() {
    struct token t;
    char msg[256];
    t=readtoken();
    if (t.type!='(') {
        sprintf(msg, "( expected before %s", t.value);
        reportline(t.line, t.pos, msg);
        seek_endstmt();
        return;
    }
    parse_expression();
    emit1(SIGNAL);
    t=readtoken();
    if (t.type!=')') {
        sprintf(msg, "( expected before %s", t.value);
        reportline(t.line, t.pos, msg);
        seek_endstmt();
        return;
    }
    expect(';', ";");
}

static void parse_wait() {
    struct token t;
    char msg[256];
    int before=0, after=0, test=0;
    t=readtoken();
    if (t.type!='(') {
        sprintf(msg, "( expected before %s", t.value);
        reportline(t.line, t.pos, msg);
        seek_endstmt();
        return;
    }
    test=ip;
    parse_expression();
    if (threaddepth==0) {
        reportline(t.line, t.pos, "wait only valid in thread\n");
    } else {
        emit1(NOT);
        before=ip;
        emitc(JZ,after);
        emit1(YIELD);
        emitc(JUMP,test);
        after=ip;
        ip=before;
        emitc(JZ,after);
        ip=after;
    }
    t=readtoken();
    if (t.type!=')') {
        sprintf(msg, "( expected before %s", t.value);
        reportline(t.line, t.pos, msg);
        seek_endstmt();
        return;
    }
    expect(';', ";");
}

static void parse_press() {
    struct token t;
    char msg[256];
    int key, valid;
    t=readtoken();
    if (t.type!='(') {
        sprintf(msg, "( expected before %s", t.value);
        reportline(t.line, t.pos, msg);
        seek_endstmt();
        return;
    }
    t=readtoken();
    if (t.type!=STRING) {
        reportline(t.line, t.pos, "Only strings are accepted as argument for press");
        seek_endstmt();
        return;
    }
    key=0;
    valid=0;
    while (keymap[key].value!=-1) {
        if (strcmp(keymap[key].key, t.value)==0) {
            valid=1;
            break;
        }
        key++;
    }
    if (!valid) {
        sprintf(msg, "Unknown key %s", t.value);
        reportline(t.line, t.pos, msg);
    }
    emitc(PUSH,keymap[key].value);
    emitc(PUSH,1);
    emit1(KEY);
    t=readtoken();
    if (t.type!=')') {
        sprintf(msg, "( expected before %s", t.value);
        reportline(t.line, t.pos, msg);
        seek_endstmt();
        return;
    }
    expect(';', ";");
}

static void parse_release() {
    struct token t;
    char msg[256];
    t=readtoken();
    int key, valid;
    if (t.type!='(') {
        sprintf(msg, "( expected before %s", t.value);
        reportline(t.line, t.pos, msg);
        seek_endstmt();
        return;
    }
    t=readtoken();
    if (t.type!=STRING) {
        reportline(t.line, t.pos, "Only strings are accepted as argument for release");
        seek_endstmt();
        return;
    }
    key=0;
    valid=0;
    while (keymap[key].value!=-1) {
        if (strcmp(keymap[key].key, t.value)==0) {
            valid=1;
            break;
        }
        key++;
    }
    if (!valid) {
        sprintf(msg, "Unknown key %s", t.value);
        reportline(t.line, t.pos, msg);
    }
    emitc(PUSH,keymap[key].value);
    emitc(PUSH,0);
    emit1(KEY);
    t=readtoken();
    if (t.type!=')') {
        sprintf(msg, "( expected before %s", t.value);
        reportline(t.line, t.pos, msg);
        seek_endstmt();
        return;
    }
    expect(';', ";");
}

static void parse_halt() {
    struct token name;
    name=peektoken();
    if (name.type==ID) {
        eattoken();
        get_thread(name.value);
        emitc(JOIN,threadnum);
    } else {
        emit1(HALT);
    }
    expect(';', ";");
}

void parse_thread() {
    struct token name;
    int thread;
    int end;
    int this;
    if (threaddepth>0)
        report("Cannot create a thread within a thread.");
    threaddepth++;
    threadnum++;
    this=threadnum;
    name=peektoken();
    if (name.type==ID) {
        eattoken();
        this=query_thread(name.value);
        if (this==0) {
            this=threadnum;
            name_thread(this, name.value);
        }
    }
    expect('{', "{");
    thread=ip;
    emitc(THREAD,this); ip+=2;
    parse_statements(1);
    emit1(HALT);
    end=ip;
    ip=thread;
    emitc(THREAD, this);
    emitaddress(end);
    ip=end;
    expect('}', "}");
    threaddepth--;
}

static void parse_assign(struct token id) {
    struct token t;
    char msg[256];
    int sym=symbol_exists(id.value);
    int ofs=0;
    if (sym<0) {
        sprintf(msg, "Unknown identifier %s", id.value);
        reportline(id.line, id.pos, msg);
    }
    t=readtoken();
    if (t.type=='[') {
        parse_expression();
        expect(']', "]");
        t=readtoken();
        ofs=1;
    }

    if (t.type==PINC) {
        if (ofs) emit2(INCA,sym,ofs);
        else emit2(INC,sym,ofs);
    } else if (t.type==PDEC) {
        if (ofs) emit2(DECA,sym,ofs);
        else emit2(DEC,sym,ofs);
    } else if (t.type==PLUSE) {
        //save ofs into register 8
        if (ofs) {emitt(POP, 8); emitt(PUSH, 8);}
        emit2(PUSH,sym,ofs);
        parse_expression();
        emit1(ADD);
        //restore from register 8
        if (ofs) emitt(PUSH,8);
        if (ofs) emit2(POPA,sym,ofs);
        else emit2(POP,sym,ofs);
    } else if (t.type==MINUSE) {
        //save ofs into register 8
        if (ofs) {emitt(POP,8); emitt(PUSH, 8);}
        emit2(PUSH,sym,ofs);
        parse_expression();
        emit1(SUB);
        //restore from register 8
        if (ofs) emitt(PUSH,8);
        if (ofs) emit2(POPA,sym,ofs);
        else emit2(POP,sym,ofs);
    } else if (t.type==MULTE) {
        //save ofs into register 8
        if (ofs) {emitt(POP,8); emitt(PUSH,8);}
        emit2(PUSH,sym,ofs);
        parse_expression();
        emit1(MUL);
        //restore from register 8
        if (ofs) emitt(PUSH,8);
        if (ofs) emit2(POPA,sym,ofs);
        else emit2(POP,sym,ofs);
    } else if (t.type==DIVE) {
        //save ofs into register 8
        if (ofs) {emitt(POP,8); emitt(PUSH,8);}
        emit2(PUSH,sym,ofs);
        parse_expression();
        emit1(DIV);
        //restore from register 8
        if (ofs) emitt(PUSH,8);
        if (ofs) emit2(POPA,sym,ofs);
        else emit2(POP,sym,ofs);
    } else if (t.type=='=') {
        //save ofs into register 8
        if (ofs) emitt(POP, 8);
        parse_expression();
        //restore from register 8
        if (ofs) emitt(PUSH, 8);
        if (ofs) emit2(POPA,sym,ofs);
        else emit2(POP,sym,ofs);
    } else {
        reportline(t.line, t.pos, "Expected \"=\", \"++\" or \"--\"");
        seek_endstmt();
        return;
    }
    expect(';', ";");
}

static void parse_statement() {
    struct token t;
    char msg[256];

    t=readtoken();
    if (t.type==';') {
        //do nothing
    } else if (t.type==IF) {
        parse_if();
    } else if (t.type==WHILE) {
        parse_while();
    } else if (t.type==DELAY) {
        parse_delay();
    } else if (t.type==WAIT) {
        parse_wait();
    } else if (t.type==VAR) {
        reportline(t.line, t.pos, "Variable declarations must come before statements");
        seek_endstmt();
    } else if (t.type==PRESS) {
        parse_press();
    } else if (t.type==RELEASE) {
        parse_release();
    } else if (t.type==SIGNAL) {
        parse_signal();
    } else if (t.type==PHALT) {
        parse_halt();
    } else if (t.type==PTHREAD) {
        parse_thread();
    } else if (t.type==ID) {
        parse_assign(t);
    } else {
        sprintf(msg, "Unexpected token %s", t.value);
        reportline(t.line, t.pos, msg);
        seek_endstmt();
    }
}

static void parse_statements(int block) {
    struct token t;

    t=peektoken();
    if (block) {
        while (t.type!='}') {
            parse_statement();
            t=peektoken();
            if (t.type == EOF) {
                expect('}', "}");
                break;
            }
        }
    } else {
        while (t.type!=EOF) {
            parse_statement();
            t=peektoken();
        }
    }
}

static void parse_script() {
    parse_declarations();
    parse_statements(0);
    emit1(HALT);
}

int parse_program(char *program, struct program_code *code) {
    FILE *cfile=fopen(program, "r");
    tokenizer=programtoken;
    if (cfile==NULL) {
        printf("File not found %s.\n", program);
        return 1;
    } else {
        pfile=cfile;
        codeseg=code->code;
        init_tokenizer();
        parse_script();
        fclose(pfile);
        code->program=PROGRAM_CODE;
        //printcode();
    }
    return program_parse_err;
}
