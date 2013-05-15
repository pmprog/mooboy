#include "io.h"
#include <stdio.h>
#include "lcd.h"
#include "maps.h"
#include "obj.h"
#include "cpu.h"
#include "mem.h"
#include "timers.h"
#include "sound.h"
#include "defines.h"
#include "util/defines.h"
#include "joy.h"

u8 io_read(u16 adr) {
    u8 r = adr &  0x00FF;

    switch(r) {
        case 0x00: return joy_read(); break;

        case 0x01: break;
        case 0x02: break;

        case 0x04: return timers.div; break;
        case 0x05: return timers.tima; break;
        case 0x06: return timers.tma; break;
        case 0x07: return timers.tac; break;
        case 0x0F: return cpu.irq; break;

        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x14: case 0x16: case 0x17: case 0x18:
        case 0x19: case 0x1A: case 0x1B: case 0x1C:
        case 0x1D: case 0x1E: case 0x20: case 0x21:
        case 0x22: case 0x23: case 0x24: case 0x25:
        case 0x26: case 0x30: case 0x31: case 0x32:
        case 0x33: case 0x34: case 0x35: case 0x36:
        case 0x37: case 0x38: case 0x39: case 0x3A:
        case 0x3B: case 0x3C: case 0x3D: case 0x3E:
        case 0x3F:
            return sound_read(r);
        break;

        case 0x15: case 0x1F: return 0x00; break;

        case 0x40: return lcd.c; break;
        case 0x41: return lcd.stat; break;
        case 0x42: return lcd.scy; break;
        case 0x43: return lcd.scx; break;
        case 0x44: return lcd.ly; break;
        case 0x45: return lcd.lyc; break;
        case 0x46: return 0x00; break;
        case 0x47: return lcd.bgp; break;
        case 0x48: return lcd.obp[0]; break;
        case 0x49: return lcd.obp[1]; break;
        case 0x4A: return lcd.wy; break;
        case 0x4B: return lcd.wx; break;

        case 0x4D: return cpu.freq_switch | (cpu.freq == DOUBLE_CPU_FREQ ? 0x80 : 0x00); break;

        case 0x4F: return ram.vrambank == ram.vrambanks[0] ? 0 : 1; break;

        case 0x51: return lcd.dma_source >> 8; break;
        case 0x52: return lcd.dma_source & 0xFF; break;
        case 0x53: return lcd.dma_dest >> 8; break;
        case 0x54: return lcd.dma_dest & 0xFF; break;
        case 0x55: return lcd.dma_length | lcd.dma_hblank_inactive; break;

        case 0x56: return 0x40; break;

        case 0x68: return lcd.bgps | lcd.bgpi; break;
        case 0x69: return lcd.bgpd[lcd.bgps]; break;
        case 0x6A: return lcd.obps | lcd.obpi; break;
        case 0x6B: return lcd.obpd[lcd.obps]; break;

        case 0x70: return ram.wrambank_index | 0xF8; break;

        default:;
            printf("Unknown IO read: %.2X\n", r);
    }

    return 0xFF; // Avoids nasty warnings, precious
}

void io_write(u16 adr, u8 val) {
    u8 r = adr & 0x00FF;
    switch(r) {
        case 0x00: joy_select_col(val); break;

        case 0x01: break;
        case 0x02: break;

        case 0x04: timers.div = 0x00; break;
        case 0x05: timers.tima = 0x00; break;
        case 0x06: timers.tma = val; break;
        case 0x07: timers.tac = val; break;
        case 0x0F: cpu.irq = val & 0x1F; break;

        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x14: case 0x16: case 0x17: case 0x18:
        case 0x19: case 0x1A: case 0x1B: case 0x1C:
        case 0x1D: case 0x1E: case 0x20: case 0x21:
        case 0x22: case 0x23: case 0x24: case 0x25:
        case 0x26: case 0x30: case 0x31: case 0x32:
        case 0x33: case 0x34: case 0x35: case 0x36:
        case 0x37: case 0x38: case 0x39: case 0x3A:
        case 0x3B: case 0x3C: case 0x3D: case 0x3E:
        case 0x3F:
            sound_write(r, val);
        break;

        case 0x15: case 0x1F: break;

        case 0x40:
            lcd.c = val;
            lcd_c_dirty();
            if(!(val & 0x80)) {
                lcd.ly = 0;
                lcd.cc = 0;
                lcd.stat &= 0xF8;
                printf("LCDC RESET\n");
            }
        break;
        case 0x41: lcd.stat = (lcd.stat & 0x03) | (val & 0x78); break;
        case 0x42: lcd.scy = val; break;
        case 0x43: lcd.scx = val; break;
        case 0x44: lcd.ly = 0x00; lcd.cc = 0;  break;
        case 0x45: lcd.lyc = val; break;
        case 0x46: lcd_dma(val); break;
        case 0x47:
            lcd.bgp = val;
            lcd_bgp_dirty();
        break;
        case 0x48:
            lcd.obp[0] = val;
            lcd_obp0_dirty();
        break;
        case 0x49:
            lcd.obp[1] = val;
            lcd_obp1_dirty();
        break;
        case 0x4A: lcd.wy = val; break;
        case 0x4B: lcd.wx = val; break;

        case 0x4D: cpu.freq_switch = val & 0x01;  break;

        case 0x4F: ram.vrambank = ram.vrambanks[val & 0x01]; break;

        case 0x51: lcd.dma_source = (lcd.dma_source & 0x00FF) | (val << 8); //printf("HDMA1 activity!\n");
        break;
        case 0x52: lcd.dma_source = (lcd.dma_source & 0xFF00) | (val & 0xF0); //printf("HDMA2 activity!\n");
        break;
        case 0x53: lcd.dma_dest = (lcd.dma_dest & 0x80FF) | ((val & 0x1F) << 8); //printf("HDMA3 activity!\n");
        break;
        case 0x54: lcd.dma_dest = (lcd.dma_dest & 0xFF00) | (val & 0xF0);// printf("HDMA4 activity!\n");
        break;
        case 0x55:
            //printf("HDMA %.4X => %.4X (%.2X)!\n", lcd.dma_source, lcd.dma_dest, val);
            lcd.dma_length = val & 0x7F;
            if(val & 0x80) {
                lcd.dma_hblank_inactive = 0x00;
            }
            else {
                if(lcd.dma_hblank_inactive) {
                    lcd_cgb_dma();
                    //printf(" ==> Performed!\n");
                    lcd.dma_length = 0x7F;
                }
                lcd.dma_hblank_inactive = 0x80;
            }
        break;

        case 0x56: break;

        case 0x68:
            lcd.bgps = val & 0x3F;
            lcd.bgpi = val & 0x80;
        break;
        case 0x69:
            lcd.bgpd[lcd.bgps] = val;
            lcd_bgpd_dirty(lcd.bgps);
            if(lcd.bgpi) {
                lcd.bgps++;
            }
            lcd.bgps &= 0x3F;
        break;
        case 0x6A:
            lcd.obps = val & 0x3F;
            lcd.obpi = val & 0x80;
        break;
        case 0x6B:
            lcd.obpd[lcd.obps] = val;
            lcd_obpd_dirty(lcd.obps);
            if(lcd.obpi) {
                lcd.obps++;
            }
            lcd.obps &= 0x3F;
        break;

        case 0x70:
            ram.wrambank_index = (val & 0x07) != 0 ? val & 0x07 : 0x01;
            ram.wrambank = ram.wrambanks[ram.wrambank_index];
        break;

        default:;
            printf("Unknown IO write: %.2X=%.2X\n", r, val);
    }
}