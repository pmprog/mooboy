#include "lcd.h"
#include <string.h>
#include <stdio.h>
#include "cpu.h"
#include "sys/sys.h"
#include "emu.h"
#include "mem.h"
#include "defines.h"
#include "util/defines.h"
#include "obj.h"
#include "maps.h"

#define DUR_FULL_REFRESH 17556
#define DUR_MODE_0 51
#define DUR_MODE_2 20
#define DUR_MODE_3 43
#define DUR_SCANLINE 114
#define DUR_VBLANK 1140

#define SIF_HBLANK 0x08
#define SIF_VBLANK 0x10
#define SIF_OAM    0x20
#define SIF_LYC    0x40

#define STAT_SET_MODE(m)  (lcd.stat = (lcd.stat & 0xFC) | (m))
#define STAT_SET_CFLAG(c) (lcd.stat = (lcd.stat & 0xFB) | ((c) << 2))

#define TILE_BYTES 16
#define TILE_LINE_BYTES 2

#define TILES_INDEX_SIGNED 0
#define TILES_INDEX_UNSIGNED 1

#define MAP_WIDTH 256
#define MAP_HEIGHT 256
#define MAP_COLUMNS 32
#define MAP_ROWS 32


lcd_t lcd;

static void swap_fb() {
    u16 *tmp = lcd.clean_fb;
    lcd.clean_fb = lcd.working_fb;
    lcd.working_fb = tmp;
    memset(lcd.working_fb, 0x00, sizeof(lcd.fb)/2);
}

static void draw_line() {
    u8 x;
    u16 *pixel = &lcd.working_fb[lcd.ly * LCD_WIDTH];
    u8 maps_scan[LCD_WIDTH];
    u8 obj_scan[LCD_WIDTH];

    memset(maps_scan, 0x00, sizeof(maps_scan));
    memset(obj_scan, 0x00, sizeof(obj_scan));

    if(lcd.c & LCDC_OBJ_ENABLE_BIT) {
        lcd_scan_obj(obj_scan);
    }

    if(emu.hw == DMG_HW) {
        lcd_scan_maps(maps_scan);
        for(x = 0; x < LCD_WIDTH; x++, pixel++) {
            if(OBJ_PRIORITY(obj_scan[x])) {
                if(maps_scan[x] != 0) {
                    *pixel = lcd.bgp_map[maps_scan[x]];
                }
                else {
                    *pixel = lcd.obp_map[OBJ_PALETTE(obj_scan[x])][OBJ_DATA(obj_scan[x])];
                }
            }
            else {
                if(OBJ_DATA(obj_scan[x]) != 0) {
                    *pixel = lcd.obp_map[OBJ_PALETTE(obj_scan[x])][OBJ_DATA(obj_scan[x])];
                }
                else {
                    *pixel = lcd.bgp_map[maps_scan[x]];
                }
            }
        }
    }
    else {
        lcd_scan_maps(maps_scan);
        //printf("%.3i: ", lcd.ly);
        for(x = 0; x < LCD_WIDTH; x++, pixel++) {
            u8 bg_priority;

            if(lcd.c & LCDC_BG_ENABLE_BIT) {
                if(maps_scan[x] & MAPS_PRIORITY_BIT) {
                    bg_priority = 1;
                }
                else {
                    bg_priority = obj_scan[x] & OBJ_PRIORITY_BIT ? 1 : 0;
                }
            }
            else {
                bg_priority = 0;
            }

            //printf("%i", MAPS_PALETTE(maps_scan[x]));
            if(bg_priority) {
                if(MAPS_DATA(maps_scan[x]) != 0 || OBJ_DATA(obj_scan[x]) == 0) {
                    *pixel = lcd.bgpd_map[MAPS_PALETTE(maps_scan[x])][MAPS_DATA(maps_scan[x])];
                }
                else {
                    *pixel = lcd.obpd_map[OBJ_PALETTE(obj_scan[x])][OBJ_DATA(obj_scan[x])];
                }
            }
            else {
                if(OBJ_DATA(obj_scan[x]) != 0) {
                    *pixel = lcd.obpd_map[OBJ_PALETTE(obj_scan[x])][OBJ_DATA(obj_scan[x])];
                }
                else {
                    *pixel = lcd.bgpd_map[MAPS_PALETTE(maps_scan[x])][MAPS_DATA(maps_scan[x])];
                }
            }
        }
        //printf("\n");
    }
}

static inline void stat_irq(u8 flag) {
    if((lcd.c & LCDC_DISPLAY_ENABLE_BIT) && (lcd.stat & flag)) {
        cpu.irq |= IF_LCDSTAT;
//        if(flag == SIF_HBLANK) {
//            printf("HBLANK INT in LY %i\n", lcd.ly);
//        }
    }
}

static inline void hblank_dma() {
    u16 end;

  //  printf("%.4X %.4X => ", lcd.dma_source, lcd.dma_dest);
    for(end = lcd.dma_source + 0x10; lcd.dma_source < end; lcd.dma_source++, lcd.dma_dest++) {
        mem_write_byte(lcd.dma_dest, mem_read_byte(lcd.dma_source));
    }
   // printf("%.4X %.4X\n", lcd.dma_source, lcd.dma_dest);

    lcd.dma_length--;
    if(lcd.dma_length == 0x00) {
        lcd.dma_length = 0x7F;
        lcd.dma_hblank_inactive = 0x80;
    }
}

static u8 step_mode(u8 m1) {
//    static int old_fc = 0;
//    static u32 old_cc = 0;

    u16 fc = lcd.cc % DUR_FULL_REFRESH;
    lcd.ly = fc / DUR_SCANLINE;
    STAT_SET_CFLAG(lcd.ly == lcd.lyc ? 1 : 0);
   // printf("%i\n", lcd.ly);
//    if(old_fc > fc) {
//        printf("%.8i %i %.5i %.10i\n", fc, cpu.nfcc, old_fc, old_cc);
//    }
//    old_fc = fc;
//    old_cc = cpu.nfcc;

    if(lcd.ly < 144) {
        u16 lc = fc % DUR_SCANLINE;
        if(lc < DUR_MODE_2)
            return 0x02;
        else if(lc < DUR_MODE_2 + DUR_MODE_3)
            return 0x03;
        else
            return 0x00;
    }
    else {
        return 0x01;
    }
}

void lcd_reset() {
    lcd.c = 0x91;
    lcd.stat = 0x82;
    lcd.scx = 0;
    lcd.scy = 0;
    lcd.ly = 0;
    lcd.lyc = 0;
    lcd.wx = 0;
    lcd.wy = 0;
    lcd.bgp = 0xFC;
    lcd.obp[0] = 0xFF;
    lcd.obp[1] = 0xFF;
    lcd.cc = 0;

    lcd.dma_source = 0x0000;
    lcd.dma_dest = 0x8000;
    lcd.dma_length = 0x00;
    lcd.dma_hblank_inactive = 0x80;

    memset(lcd.fb, 0x00, sizeof(lcd.fb));
    lcd.clean_fb = lcd.fb[0];
    lcd.working_fb = lcd.fb[1];

    memset(lcd.bgpd, 0x00, sizeof(lcd.bgpd));
    memset(lcd.obpd, 0x00, sizeof(lcd.obpd));

    lcd_c_dirty();
    lcd_bgp_dirty();
    lcd_obp0_dirty();
    lcd_obp1_dirty();
}

void lcd_step() {
    u16 m1, m2;

    lcd.cc += cpu.step_nf_cycles;

    m1 = lcd.stat & 0x03;
    m2 = step_mode(m1);
    STAT_SET_MODE(m2);

    if(m1 != m2) {
        switch(m2) {
            case 0x00:
                stat_irq(SIF_HBLANK);
                if(lcd.c & LCDC_DISPLAY_ENABLE_BIT) {
                    draw_line();
                }
                if(!lcd.dma_hblank_inactive) {
                    hblank_dma();
                }
            break;
            case 0x01:
                cpu.irq |= IF_VBLANK;
                stat_irq(SIF_VBLANK);
                sys_fb_ready();
                swap_fb();
                //SDL_Delay(100);
            break;
            case 0x02:
                stat_irq(SIF_OAM);
                if(lcd.ly == lcd.lyc) {
                    stat_irq(SIF_LYC);
                }
            break;
            case 0x03:
            break;
        }
    }
}

void lcd_dma(u8 v) {
    u8 b;
    u16 src;

    for(src = ((u16)v)<<8, b = 0; b < 0xA0; b++, src++) {
        ram.oam[b] = mem_read_byte(src);
    }
}

void lcd_cgb_dma() {
    u16 length, source, dest, end;

    length = (lcd.dma_length + 1) * 0x10;
    source = lcd.dma_source;
    dest = lcd.dma_dest;
    end = source + length;

    for(; source < end; source++, dest++) {
        mem_write_byte(dest, mem_read_byte(source));
    }

    lcd.dma_length = 0x7F;
    lcd.dma_hblank_inactive = 0x80;
}

void lcd_c_dirty() {
    lcd.bg_map = &ram.vrambanks[0][lcd.c & LCDC_BG_MAP_BIT ? 0x1C00 : 0x1800];
    lcd.wnd_map = &ram.vrambanks[0][lcd.c & LCDC_WND_MAP_BIT ? 0x1C00 : 0x1800];
    lcd.bg_attr_map = &ram.vrambanks[1][lcd.c & LCDC_BG_MAP_BIT ? 0x1C00 : 0x1800];
    lcd.wnd_attr_map = &ram.vrambanks[1][lcd.c & LCDC_WND_MAP_BIT ? 0x1C00 : 0x1800];
}

void lcd_bgp_dirty() {
    u8 rc;

    for(rc = 0; rc < 4; rc++) {
        lcd.bgp_map[rc] = (lcd.bgp & (0x3 << (rc<<1))) >> (rc<<1);
    }
}

static void obp_dirty(u8 obp, u8 *obp_map) {
    u8 rc;
    for(rc = 0; rc < 4; rc++) {
        obp_map[rc] = (obp & (0x3 << (rc<<1))) >> (rc<<1);
    }
}

void lcd_obp0_dirty() {
    obp_dirty(lcd.obp[0], lcd.obp_map[0]);
}

void lcd_obp1_dirty() {
    obp_dirty(lcd.obp[1], lcd.obp_map[1]);
}

static void pd_dirty(u16 map[8][4], u8 d, u8 s) {
    u16 palette, color;

    palette = s/8;
    color = (s/2)%4;

    if(s % 2 == 0) {
        map[palette][color] = (map[palette][color] & 0xFF00) | d;
    }
    else {
        map[palette][color] = (map[palette][color] & 0x00FF) | (d << 8);
    }
}

void lcd_bgpd_dirty(u8 bgps) {
    pd_dirty(lcd.bgpd_map, lcd.bgpd[bgps], bgps);
}

void lcd_obpd_dirty(u8 obps) {
    pd_dirty(lcd.obpd_map, lcd.obpd[obps], obps);
}
