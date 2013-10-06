#ifndef CORE_LCD_H
#define CORE_LCD_H

#include "defines.h"
#include "hw.h"

#define TILE_WIDTH 8
#define TILE_HEIGHT 8

#define LCDC_BG_ENABLE_BIT      0x01
#define LCDC_OBJ_ENABLE_BIT     0x02
#define LCDC_OBJ_SIZE_BIT       0x04
#define LCDC_BG_MAP_BIT         0x08
#define LCDC_TILE_DATA_BIT      0x10
#define LCDC_WND_ENABLE_BIT     0x20
#define LCDC_WND_MAP_BIT        0x40
#define LCDC_DISPLAY_ENABLE_BIT 0x80

#define LCD_WIDTH 160
#define LCD_HEIGHT 144


typedef struct {
    u16 color;
    u8 color_id;
    u8 priority;
} scan_pixel_t;

typedef struct {
    scan_pixel_t scan_cache[256][256];
    int tile_dirty[32][32];
    u8 *tiles;
    u8 *attr;
} lcd_map_t;

typedef struct {
    u8 c;
    u8 stat;
    u8 scx, scy;
    u8 ly;
    u8 lyc;
    u8 wx, wy;
    u8 bgp, obp[2];
    u8 bgpd[0x40], obpd[0x40];
    u8 bgps, bgpi;
    u8 obps, obpi;

    u16 fb[2][144*160];
    u16 *clean_fb;
    u16 *working_fb;

    u16 hdma_source, hdma_dest;
    u8 hdma_length, hdma_inactive;

    lcd_map_t maps[2];

    u16 bgp_map[8][4];
    u16 obp_map[8][4];

    hw_event_t mode_event[4];
    hw_event_t vblank_line_event;
} lcd_t;


extern lcd_t lcd;


void lcd_reset();
void lcd_begin();

void lcd_dma(u8 v);
void lcd_gdma();
void lcd_hdma();

void lcd_enable();
void lcd_disable();
void lcd_set_lyc(u8 lyc);
void lcd_reset_ly();

void lcd_c_write(u8 val);
void lcd_vram_write(u16 adr, u8 val);

void lcd_update_map_pointers(u8 c);
void lcd_obp0_dirty();
void lcd_obp1_dirty();
void lcd_bgp_dirty();
void lcd_bgpd_dirty(u8 bgps);
void lcd_obpd_dirty(u8 obps);

#endif
