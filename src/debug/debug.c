#include "debug.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include "core/defines.h"
#include "core/cpu.h"
#include "core/mem.h"

#include "break.h"
#include "disasm.h"
#include "record.h"
#include "watch.h"

typedef enum {
    DEBUG_RUN,
    DEBUG_NEXT,
    DEBUG_FINISH
} DEBUG_MODE;

static DEBUG_MODE mode = DEBUG_NEXT;
static int active = 0;
static block_t* finish_block = NULL;
static int debug_scope = 0;

static int isword(char c) {
    return isprint(c) && !isspace(c);
}

static char* trim(char* in) {
    char* trimmed = in;
    for (; !isword(*trimmed) && *trimmed != '\0'; trimmed++) {}

    char* end = &in[strlen(in)-1];
    for (; !isword(*end); end--) {}

    if (*end != '\0') {
        *(end+1) = '\0';
    }
    return trimmed;
}

static char* word(char **in) {
    char* tmp = *in;
    for(; isword(**in); (*in)++) {}
    **in = '\0';
    (*in)++;
    return tmp;
}

static void cmd_break(char* tail) {
    char* subcmd = word(&tail);

    breakpoint_t bp;
    int valid_bp = 0;

    bp.type = BREAKPOINT_EVENT;

    if (!strcmp(subcmd, "joy-input")) {
        bp.event.type = EVENT_JOY_INPUT;
        valid_bp = 1;
    }
    else if (!strcmp(subcmd, "joy-noticed") || !strcmp(subcmd, "jn")) {
        bp.event.type = EVENT_JOY_NOTICED;
        valid_bp = 1;
    }
    else if (!strcmp(subcmd, "disable")) {
        char* subsubcmd = word(&tail);
        int i = atoi(subsubcmd);
        break_disable(i);
    }
    else if (!strcmp(subcmd, "pc")) {
        char* bpc = word(&tail);
        assert(*bpc != '\0');
        bp.type = BREAKPOINT_ADDRESS;
        bp.address.pc = strtol(bpc, NULL, 16);
        valid_bp = 1;
    }
    else if (!strcmp(subcmd, "clear")) {
        printf("Cleared all breakpoints\n", subcmd);
        break_clear();
        return;
    }
    else {
        printf("Unknown break command '%s'\n", subcmd);
        valid_bp = 0;
    }

    if (valid_bp) {
        break_enable(bp);
    }
}

static void cmd_disassemble(char* subcmd) {
    block_t* b = record_get_current_block();
    if (b) {
        printf("Disassembling %.4X..%.4X\n", b->begin, b->end);
        u16 a = b->begin;
        for (; a <= b->end;) {
            op_t op = disasm(a);
            printf("  %.4X(%.2X): %s\n", a, debug_read_byte(a), disasm_str(op));
            a += op_length(op);
        }
    }
    else {
        printf("Not in any block\n");
    }
}

static void cmd_finish(char* tail) {
    finish_block = record_get_current_block();
    if (finish_block) {
        mode = DEBUG_FINISH;
    }
    else {
        printf("Can't finish - not currently in any block\n");
    }
}

static void cmd_watch(char* tail) {
    char* subcmd = word(&tail);

    watchpoint_t watchpoint;

    if (!strcmp(subcmd, "mem-r")) {
        watchpoint.type = WATCHPOINT_MEM_R;
    }
    else if (!strcmp(subcmd, "mem-rw")) {
        watchpoint.type = WATCHPOINT_MEM_RW;
    }
    else if (!strcmp(subcmd, "mem-w")) {
        watchpoint.type = WATCHPOINT_MEM_W;
    }
    else if (!strcmp(subcmd, "clear")) {
        printf("Cleared all watches\n", subcmd);
        watch_clear();
        return;
    }
    else {
        printf("Unknown watch subcommand '%s'\n", subcmd);
        return;
    }

    switch (watchpoint.type) {
        case WATCHPOINT_MEM_R:
        case WATCHPOINT_MEM_RW:
        case WATCHPOINT_MEM_W:
        {
            char* begin = word(&tail);
            assert(*begin != '\0');

            char* end = word(&tail);
            if (*end == '\0') {
                end = begin;
            }

            watchpoint.mem.begin = strtol(begin, NULL, 16);
            watchpoint.mem.end = strtol(end, NULL, 16);
        }
        break;
    }

    watch_enable(watchpoint);
}

static void cmd_cpureg() {
    printf("AF [ A = %.2X ; F = %.2X ]\n", A, F);
    printf("BC [ B = %.2X ; C = %.2X ]\n", B, C);
    printf("DE [ D = %.2X ; E = %.2X ]\n", D, E);
    printf("HL [ H = %.2X ; L = %.2X ]\n", H, L);
    printf("SP = %.4X\n", SP);
    printf("PC = %.4X\n", PC);
}

static void cmd_memdump(char* tail) {
    char* startaddr = word(&tail);
    assert(*startaddr != '\0');
    char* endaddr = word(&tail);
    if (*endaddr == '\0') {
        endaddr = startaddr;
    }

    u16 curaddr = strtol(startaddr, NULL, 16);
    u16 finish = strtol(endaddr, NULL, 16);
    if (finish < curaddr) {
        finish = curaddr + 0x10;
    }
    u8 needaddrmarker = 1;
    int newlineat = 16;
    while (curaddr <= finish) {
         if (needaddrmarker != 0) {
             printf("%.4X | ", curaddr);
             newlineat = 16;
             needaddrmarker = 0;
         }
         printf("%.2X ", debug_read_byte(curaddr));
         newlineat--;
         if (newlineat == 0) {
             needaddrmarker = 1;
             printf("\n");
         }
         curaddr++;
    }
    printf("\n");
}

void debug_step() {
    int continue_emulation = !break_now();

    if (mode == DEBUG_NEXT) {
        continue_emulation = 0;
    }

    if (mode == DEBUG_FINISH) {
        if (PC < finish_block->begin || PC > finish_block->end) {
            continue_emulation = 0;
        }
    }

    while (!continue_emulation) {
        static char raw[256];

        printf("(%.4X): ", PC);

        {
            int i = 0;
            for(; i < sizeof(raw); i++) {
                char c = getc(stdin);
                if (c == '\n') {
                    break;
                } else {
                    raw[i] = c;
                }
            }
            raw[i] = '\0';
        }

        char* tail = raw;
        char* cmd = word(&tail);

        if (!strcmp("r", cmd)) {
            mode = DEBUG_RUN;
            continue_emulation = 1;
        }
        else if (!strcmp("n", cmd) || *cmd == '\0') {
            mode = DEBUG_NEXT;
            continue_emulation = 1;
            break;
        }
        else if (!strcmp("b", cmd)) {
            cmd_break(trim(tail));
        }
        else if (!strcmp("d", cmd)) {
            cmd_disassemble(trim(tail));
        }
        else if (!strcmp("f", cmd)) {
            cmd_finish(trim(tail));
            continue_emulation = 1;
        }
        else if (!strcmp("w", cmd)) {
            cmd_watch(trim(tail));
        }
        else if (!strcmp("c", cmd)) {
            cmd_cpureg();
        }
        else if (!strcmp("m", cmd)) {
            cmd_memdump(trim(tail));
        }
        else {
            printf("Unknown cmd '%s'\n", cmd);
        }
    }

    if (mode == DEBUG_FINISH) {
        if (PC >= finish_block->begin && PC <= finish_block->end) {
            printf("%.4X(%.2X): %s\n", PC, debug_read_byte(PC), disasm_str(disasm(PC)));
        }
    }
    if (mode == DEBUG_NEXT) {
        printf("%.4X(%.2X): %s\n", PC, debug_read_byte(PC), disasm_str(disasm(PC)));
    }
}

u8 debug_read_byte(u16 addr) {
    debug_scope_begin();
    u8 b = mem_read_byte(addr);
    debug_scope_end();
    return b;
}

u16 debug_read_word(u16 addr) {
    debug_scope_begin();
    u16 b = mem_read_word(addr);
    debug_scope_end();
    return b;
}

void debug_scope_begin() {
    debug_scope = 1;
}

void debug_scope_end() {
    debug_scope = 0;
}

int in_debug_scope() {
    return debug_scope;
}


