/*
 * Fake joystick driver for Linux
 * by Alexandre Hardy
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "program.h"
#include "mapper.h"
#include "clock.h"
#include "keys.h"

#define INTERVAL    50
#define STACK_SIZE    1024
#define MAX_THREADS    8

#define debug(...)
//#define debug printf

static int installed=0;

static int timestamp=0;
static int tick=0;
static __uint64_t started=0;
static int executing=0;

//controller variables
static __int16_t code_analog[64];
//controller variables
static __uint8_t code_bit[128];
//controller variables
static __int16_t js_analog[16][64];
//controller variables
static __uint8_t js_bit[16][128];
static __int16_t currentmode=0;
static __int16_t firstscan=1;
static __int16_t clocktick=1;
static __int16_t xrel=0;
static __int16_t yrel=0;
static __int16_t zrel=0;
static __uint8_t code[MAX_CODE_SIZE];
//registers inaccessible to user
//allocated by compiler
static int status=0;        //0 = no valid program 1=valid program
static struct code_context {
    unsigned int ip;
    int registers[256]; //general purpose registers
    int sp;
    int stack[STACK_SIZE];
    int active; //is this thread active
} code_threads[MAX_THREADS];

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

struct code_context *code_thread;

void code_button(int code, int value);
void code_axis(int axis, int value);
void execute_script(void);

void code_notify_button(int js, int key, int value) {
    if ((js>=0)&&(js<16))
    if ((key-BTN_JOYSTICK>=0)&&(key-BTN_JOYSTICK<128)) {
        if (js_bit[js][key-BTN_JOYSTICK]!=value) {
            js_bit[js][key-BTN_JOYSTICK]=value;
            program_run();
        }
    }
}

void code_notify_axis(int js, int axis, int value) {
    if ((js>=0)&&(js<16))
    if ((axis>=0)&&(axis<64)) {
        if (js_analog[js][axis]!=value) {
            js_analog[js][axis]=value;
            program_run();
        }
    }
}

void code_set_program(struct program_code *program) {
    if (program==NULL) {
        status=0;
        return;
    }
    if (program->program!=PROGRAM_CODE) return;
    memcpy(code, program->code, MAX_CODE_SIZE);
    firstscan=1;
    currentmode=0;
    timestamp=0;
    status=1;
}

void code_reset(void) {
    int i, j;
    for (i=0; i<MAX_THREADS; i++) {
        for (j=0; j<256; j++) {
             code_threads[i].registers[j]=0;
        }
        code_threads[i].active=0;
    }
    code_threads[0].active=1;
    for (i=0; i<64; i++) {
        code_analog[i]=0;
    }
    for (i=0; i<128; i++) {
        code_bit[i]=0;
    }
    firstscan=1;
    currentmode=0;
    timestamp=0;
    started=0;
}

static void push(int x) {
    if (code_thread->sp<STACK_SIZE) code_thread->stack[code_thread->sp++]=x;
}

static int pop(void) {
    if (code_thread->sp<=0) return 0;
    else return code_thread->stack[--code_thread->sp];
}

static int get_value(int address, int type, int array) {
    int js;
    int ofs=0;
    if (array)
        ofs=code_thread->registers[OFSREG];
    switch (type) {
        case GP:
            if (address+ofs==FIRSTSCAN) return firstscan;
            if (address+ofs==CLOCKTICK) return clocktick;
            if (address+ofs==XREL) return xrel;
            if (address+ofs==YREL) return yrel;
            if (address+ofs==ZREL) return zrel;
            if (address+ofs==TIMESTAMP) return timestamp;
            if (address+ofs==CURRENTMODE) return currentmode;
            if ((address+ofs>=0)&&(address+ofs<256))
                return code_thread->registers[address+ofs];
            else
                return 0;
        case GLOBAL:
            if (address+ofs==FIRSTSCAN) return firstscan;
            if (address+ofs==CLOCKTICK) return clocktick;
            if (address+ofs==XREL) return xrel;
            if (address+ofs==YREL) return yrel;
            if (address+ofs==ZREL) return zrel;
            if (address+ofs==TIMESTAMP) return timestamp;
            if (address+ofs==CURRENTMODE) return currentmode;
            if ((address+ofs>=0)&&(address+ofs<256))
                return code_threads[0].registers[address+ofs];
            else
                return 0;
        case CODEA:
            if ((address+ofs>=0)&&(address+ofs<64))
                return code_analog[address+ofs];
            else
                return 0;
        case CODEB:
            if ((address+ofs>=0)&&(address+ofs<128))
                return code_bit[address+ofs];
            else
                return 0;
        case JS:
            js=(address&0x0F00)>>8;
            type=address&0xF000;
            address=address&0x00FF;
            if (type==0) {
                if ((address+ofs>=0)&&(address+ofs<128))
                    return js_bit[js][address+ofs];
                else
                    return 0;
            } else {
                if ((address+ofs>=0)&&(address+ofs<64))
                    return js_analog[js][address+ofs];
                else
                    return 0;
            }
        case CONST:
            return address;
        default:
            return 0;
    }
}

static void set_value(int address, int type, int value, int array) {
    int ofs=0;
    if (array)
        ofs=code_thread->registers[OFSREG];
    switch (type) {
        case GP:
            if (address==XREL) xrel=value;
            if (address==YREL) yrel=value;
            if (address==ZREL) zrel=value;
            if (address==CURRENTMODE) currentmode=value;
            if ((address+ofs>=0)&&(address+ofs<256))
                code_thread->registers[address+ofs]=value;
            break;
        case GLOBAL:
            if (address==XREL) xrel=value;
            if (address==YREL) yrel=value;
            if (address==ZREL) zrel=value;
            if (address==CURRENTMODE) currentmode=value;
            if ((address+ofs>=0)&&(address+ofs<256))
                code_threads[0].registers[address+ofs]=value;
            break;
        case CODEA:
            if ((address+ofs>=0)&&(address+ofs<64))
                code_axis(address+ofs, value);
            break;
        case CODEB:
            if ((address+ofs>=0)&&(address+ofs<128))
                code_button(address+ofs, value);
            break;
        case JS:
        case CONST:
            break;
        default:
            break;
    }
}

static int instr_exec=0;

static void execute_script_thread(struct code_context *new_thread, unsigned int new_ip, unsigned int retip);
void execute_script(void) {
    if (executing) return;
    executing=1;
    code_thread=&code_threads[0];
    code_thread->ip=0;
    code_thread->sp=0;
    code_thread->active=1;
    instr_exec=0;
    debug("\n\nstart thread\n");
    debug("timestamp %d\n", timestamp);
    execute_script_thread(code_thread,0,MAX_CODE_SIZE);
    firstscan=0;
    clocktick=0;
    executing=0;
}

static void execute_script_thread(struct code_context *new_thread, unsigned int new_ip, unsigned int retip) {
    int address=0, array=0;
    int type;
    int task;
    int st;
    int p1, p2;
    int value=0;
    unsigned int thread_start;
    unsigned int thread_exit;
    if (status!=1) return;
    if (!new_thread->active) {
        debug("Activating thread\n");
        *new_thread=*code_thread;
        new_thread->ip=new_ip;
        //code_thread->ip=retip;
        code_thread=new_thread;
        new_thread->active=1;
    } else {
        code_thread=new_thread;
    }
    while ((code_thread->ip<MAX_CODE_SIZE)&&(instr_exec<100000)) {
        task=code[code_thread->ip++];
        type=task >> 5;
        task=task&31;
        if (task>=PUSHA) array=1;
        else array=0;
        if ((task>=0)&&(task<=INCA))
        if (has_operand[task]) {
            address=*((unsigned short *)(code+code_thread->ip));
            value=get_value(address, type, array);
            code_thread->ip+=2;
            debug("Got operand %d address=%d type=%d\n",value, address, type);
        }
        instr_exec++;
        switch (task) {
            case HALT:
                debug("HALT\n");
                code_thread->ip=MAX_CODE_SIZE+1;
                code_thread->active=0;
                break;
            case JZ:
                debug("JZ\n");
                st=pop();
                if (st==0) code_thread->ip=value;
                break;
            case JUMP:
                debug("JUMP\n");
                code_thread->ip=value;
                break;
            case JUMPREL:
                debug("JUMPREL\n");
                code_thread->ip+=value;
                break;
            case ADD:
                debug("ADD\n");
                p1=pop();
                p2=pop();
                push(p2+p1);
                break;
            case SUB:
                debug("SUB\n");
                p1=pop();
                p2=pop();
                push(p2-p1);
                break;
            case MUL:
                debug("MUL\n");
                p1=pop();
                p2=pop();
                push(p2*p1);
                break;
            case DIV:
                debug("DIV\n");
                p1=pop();
                p2=pop();
                push(p2/p1);
                break;
            case LT:
                debug("LT\n");
                p1=pop();
                p2=pop();
                push((p2<p1)?1:0);
                break;
            case LE:
                debug("LE\n");
                p1=pop();
                p2=pop();
                push((p2<=p1)?1:0);
                break;
            case GT:
                debug("GT\n");
                p1=pop();
                p2=pop();
                push((p2>p1)?1:0);
                break;
            case GE:
                debug("GE\n");
                p1=pop();
                p2=pop();
                push((p2>=p1)?1:0);
                break;
            case EQ:
                debug("EQ\n");
                p1=pop();
                p2=pop();
                push((p2==p1)?1:0);
                break;
            case NEQ:
                debug("NEQ\n");
                p1=pop();
                p2=pop();
                push((p2!=p1)?1:0);
                break;
            case AND:
                debug("AND\n");
                p1=pop();
                p2=pop();
                push((p2&&p1)?1:0);
                break;
            case OR:
                debug("OR\n");
                p1=pop();
                p2=pop();
                push((p2||p1)?1:0);
                break;
            case NOT:
                debug("NOT\n");
                p1=pop();
                push((!p1)?1:0);
                break;
            case NEG:
                debug("NEG\n");
                p1=pop();
                push(-p1);
                break;
            case PUSH:
                debug("PUSH %d\n", value);
                push(value);
                break;
            case POP:
                value=pop();
                debug("POP %d\n", value);
                set_value(address, type, value, 0);
                break;
            case DEC:
                debug("DEC\n");
                set_value(address, type, value-1, 0);
                break;
            case INC:
                debug("INC\n");
                set_value(address, type, value+1, 0);
                break;
            case PUSHA:
                debug("PUSHA %d\n", value);
                push(value);
                break;
            case POPA:
                value=pop();
                debug("POPA %d\n", value);
                set_value(address, type, value, 1);
                break;
            case DECA:
                debug("DECA\n");
                set_value(address, type, value-1, 1);
                break;
            case INCA:
                debug("INCA\n");
                set_value(address, type, value+1, 1);
                break;
            case KEY:
                p2=pop();
                p1=pop();
                debug("KEY %d %d\n", p1, p2);
                press_key(p1, p2);
                break;
            case SIGNAL:
                debug("SIGNAL\n");
                value=pop();
                push_signal(value);
                break;
            case THREAD: //context switch, value is new thread, two constant jump targets
                //thread_start=*((unsigned short *)(code+code_thread->ip));
                //code_thread->ip+=2;
                debug("THREAD %d\n", value);
                thread_exit=*((unsigned short *)(code+code_thread->ip));
                code_thread->ip+=2;
                thread_start=code_thread->ip;
                if ((value>0)&&(value<MAX_THREADS)) {
                    execute_script_thread(&code_threads[value], thread_start, thread_exit);
                    code_thread=new_thread;
                    code_thread->ip=thread_exit;
                } else {
                    debug("Not a valid thread\n");
                    code_thread=new_thread;
                    code_thread->ip=thread_exit;
                }
                break;
            case YIELD:
                debug("YIELD\n");
                return;
                break;
            case JOIN: //stop thread
                debug("JOIN %d\n", value);
                if ((value>0)&&(value<MAX_THREADS)) {
                    code_thread[value].active=0;
                }
                break;
        }
    }
}

int mapper_code_install(void) {
    int i, j;
    if (installed) return 0;

    for (j=0; j<16; j++) {
        for (i=0; i<64; i++) {
            js_analog[j][i]=0;
        }
        for (i=0; i<128; i++) {
            js_bit[j][i]=0;
        }
    }

    installed=1;
    code_reset();
    return 0;
}

int mapper_code_uninstall(void) {
    if (installed) {
        installed=0;
    }
    return 0;
}

void code_button(int code, int value) {
    if (!installed) return;
    if (value!=code_bit[code]) {
        press_joy_button(255, BTN_JOYSTICK+code, value);
        code_bit[code]=value;
    }
}

void code_axis(int axis, int value) {
    if (!installed) return;
    if (value!=code_analog[axis]) {
        set_joy_axis(255, axis, value);
        code_analog[axis]=value;
    }
}

void program_run() {
    int new_tick;

    if (started == 0)
        started = clock_millis();

    timestamp = clock_millis() - started;
    new_tick = timestamp / INTERVAL;

    if (tick != new_tick)
        clocktick=1;
    else
        clocktick=0;
    tick = new_tick;

    debug("timertick %d\n", timestamp);
    execute_script();
}

