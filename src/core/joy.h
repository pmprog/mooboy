#ifndef CORE_JOY_H
#define CORE_JOY_H

#include "defines.h"

#define JOY_BUTTON_RIGHT    0x01
#define JOY_BUTTON_LEFT     0x02
#define JOY_BUTTON_UP       0x04
#define JOY_BUTTON_DOWN     0x08
#define JOY_BUTTON_A        0x10
#define JOY_BUTTON_B        0x20
#define JOY_BUTTON_SELECT   0x40
#define JOY_BUTTON_START    0x80

#define JOY_STATE_PRESSED   0x00
#define JOY_STATE_RELEASED  0x01

#define SELECT_DIRECTION_BIT 0x10
#define SELECT_ACTION_BIT 0x20

typedef struct {
    u8 state;
    u8 col;
} joy_t;

extern joy_t joy;

void joy_reset();

void joy_set_button(u8 button, u8 state);
u8 joy_read();
void joy_select_col(u8 flag);

#endif
