#pragma once
#ifndef GBC_H
#define GBC_H

#include "types.h"

class GBC
{
public:
    GBC(u8 *rom, int type);
    ~GBC(void);

    void reset_state(void);
    void print_header_info(void);
    void print_regs(void);
    int do_cycle(void);

    u16 pc;
private:
    u16 mem_read(u16 location);
    void mem_write(u16 location, u16 value);

    inline u8 A(void) { return AF >> 8; };
    inline void A(u8 n) { AF = (AF & 0xff) | (n << 8); }
    inline u8 F(void) { return AF & 0xff; };
    inline void F(u8 n) { AF = (AF & 0xff00) | n; }
    inline u8 B(void) { return BC >> 8; };
    inline void B(u8 n) { BC = (BC & 0xff) | (n << 8); }

    static const u8 FLAG_C = 0x10;
    static const u8 FLAG_H = 0x20;
    static const u8 FLAG_N = 0x40;
    static const u8 FLAG_Z = 0x80;


    u8 *rom;
    int type;
    int freq;

    u16 AF, BC, DE, HL;
        
    u16 sp;

    bool interrupts_master_enabled;
    u8 interrupts_enable;
    u8 interrupts_request;

    u8 io_lcd_SCX;
    u8 io_lcd_SCY;
    u8 io_lcd_WX;
    u8 io_lcd_WY;
    u8 io_lcd_BGP;
    u8 io_lcd_OBP0;
    u8 io_lcd_OBP1;
    u8 io_lcd_LCDC;
    u8 io_lcd_LY;
    u8 io_lcd_LYC;

    u8 io_timer_DIV;
    u8 io_timer_TIMA;
    u8 io_timer_TMA;
    u8 io_timer_TAC;

    u8 io_serial_data;
    u8 io_serial_control;

    u8 mem_HRAM[0xfffe - 0xff80 + 1];
    u8 mem_WRAM0[0xcfff - 0xc000 + 1];
};

#endif
