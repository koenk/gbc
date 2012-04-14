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
    int do_instruction(void);

    u16 pc;
    long cycles;
private:
    u8 mem_read(u16 location);
    void mem_write(u16 location, u8 value);
    void handle_interrupts(void);
    void handle_LCD(int op_cycles);

    inline u8 A(void) { return AF >> 8; };
    inline void A(u8 n) { AF = (AF & 0xff) | (n << 8); }
    inline u8 F(void) { return AF & 0xff; };
    inline void F(u8 n) { AF = (AF & 0xff00) | n; }
    inline u8 B(void) { return BC >> 8; };
    inline void B(u8 n) { BC = (BC & 0xff) | (n << 8); }
    inline u8 C(void) { return BC & 0xff; };
    inline void C(u8 n) { BC = (BC & 0xff00) | n; }
    inline u8 D(void) { return DE >> 8; };
    inline void D(u8 n) { DE = (DE & 0xff) | (n << 8); }
    inline u8 E(void) { return DE & 0xff; };
    inline void E(u8 n) { DE = (DE & 0xff00) | n; }

    static const u8 FLAG_C = 0x10;
    static const u8 FLAG_H = 0x20;
    static const u8 FLAG_N = 0x40;
    static const u8 FLAG_Z = 0x80;


    u8 *rom;
    int type;
    int freq;

    int lcd_mode_clks_left;

    u16 AF, BC, DE, HL;
        
    u16 sp;

    bool interrupts_master_enabled;
    u8 interrupts_enable;
    u8 interrupts_request;

    u8 io_lcd_SCX;  // Scroll X
    u8 io_lcd_SCY;  // Scroll Y
    u8 io_lcd_WX;   // Window X
    u8 io_lcd_WY;   // Window Y
    u8 io_lcd_BGP;
    u8 io_lcd_OBP0;
    u8 io_lcd_OBP1;
    u8 io_lcd_LCDC; // LCD Status. 0-1 = mode, 2 = LYC==LY, 3 = M0 interrupt, 4 = M1 inter, 5 = M2 inter, 6 = LY=LYC inter
    u8 io_lcd_LY;   // LCD Y line
    u8 io_lcd_LYC;  // LCD Y line compare

    u8 io_timer_DIV;
    u8 io_timer_TIMA;
    u8 io_timer_TMA;
    u8 io_timer_TAC;

    u8 io_serial_data;
    u8 io_serial_control;

    int mem_bank_rom, mem_bank_ram, mem_bank_wram;

    u8 mem_RAM[4][0x2000];
    u8 mem_HRAM[0x7f];
    u8 mem_WRAM[8][0x1000];

};

#endif
