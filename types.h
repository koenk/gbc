#pragma once
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define REG16_BC 0
#define REG16_DE 2
#define REG16_HL 4
#define REG16_AF 6
#define REG_B 1
#define REG_C 0
#define REG_D 3
#define REG_E 2
#define REG_H 5
#define REG_L 4
#define REG_A 7
#define REG_F 6

    /*
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
    inline u8 H(void) { return HL >> 8; };
    inline void H(u8 n) { HL = (HL & 0xff) | (n << 8); }
    inline u8 L(void) { return HL & 0xff; };
    inline void L(u8 n) { HL = (HL & 0xff00) | n; }

    static const u8 FLAG_C = 0x10;
    static const u8 FLAG_H = 0x20;
    static const u8 FLAG_N = 0x40;
    static const u8 FLAG_Z = 0x80;
    */

#define FLAG_C 0x10
#define FLAG_H 0x20
#define FLAG_N 0x40
#define FLAG_Z 0x80

enum gb_type {
    GB_TYPE_GB,
    GB_TYPE_CGB,
};

/* TODO split this up into module-managed components (cpu, mmu, ...) */
struct gb_state
{
    enum gb_type gb_type;
    u8* bios;
    u8 in_bios:1;

    int freq;
    int lcd_mode_clks_left;

    union {
        u8 regs[8];
        struct {
            u16 BC, DE, HL, AF;
        } reg16;
        struct { /* little-endian of x86 is not nice here. */
            u8 C, B, E, D, L, H, F, A;
        } reg8;
        struct __attribute__((packed)) {
            char padding[6];
            u8 pad1:1;
            u8 pad2:1;
            u8 pad3:1;
            u8 pad4:1;
            u8 CF:1;
            u8 HF:1;
            u8 NF:1;
            u8 ZF:1;
        } flags;
    };

    /* Lookup tables for the reg-index encoded in instructions to ptr to reg. */
    u8 *reg8_lut[9];
    u16 *reg16_lut[4];
    u16 *reg16s_lut[4];

    u16 sp;
    u16 pc;
    long cycles;

    u8 interrupts_master_enabled:1;
    u8 interrupts_enable;
    u8 interrupts_request;
    u8 halt_for_interrupts:1;

    u8 io_lcd_SCX;  // Scroll X
    u8 io_lcd_SCY;  // Scroll Y
    u8 io_lcd_WX;   // Window X
    u8 io_lcd_WY;   // Window Y
    u8 io_lcd_BGP;  // Background palette data (monochrome)
    u8 io_lcd_OBP0; // Object palette 0 data (monochrome)
    u8 io_lcd_OBP1; // Object palette 1 data (monochrome)
    u8 io_lcd_BGPI; // Background palette index
    u8 io_lcd_BGPD; // Background palette data
    u8 io_lcd_OBPI; // Sprite palette index
    u8 io_lcd_OBPD; // Sprite palette data
    u8 io_lcd_LCDC; // LCD Control. 0 = BG, 1 = OBJ, 2 = OBJ-size, 3 = BG tilemap, 4 = BG+Win tilemap, 5 = window, 6 = window tilemap, 7 = lcd enable
    u8 io_lcd_STAT; // LCD Status. 0-1 = mode, 2 = LYC==LY, 3 = M0 interrupt, 4 = M1 inter, 5 = M2 inter, 6 = LY=LYC inter
    u8 io_lcd_LY;   // LCD Y line
    u8 io_lcd_LYC;  // LCD Y line compare

    u8 io_timer_DIV;
    u8 io_timer_TIMA;
    u8 io_timer_TMA;
    u8 io_timer_TAC;

    u8 io_serial_data;
    u8 io_serial_control;

    u8 io_infrared;

    u8 io_buttons;
    u8 io_buttons_dirs;
    u8 io_buttons_buttons;


    u8 io_sound_enabled;
    u8 io_sound_out_terminal;
    u8 io_sound_terminal_control;

    u8 io_sound_channel1_sweep;
    u8 io_sound_channel1_length_pattern;
    u8 io_sound_channel1_envelope;
    u8 io_sound_channel1_freq_lo;
    u8 io_sound_channel1_freq_hi;

    u8 io_sound_channel2_length_pattern;
    u8 io_sound_channel2_envelope;
    u8 io_sound_channel2_freq_lo;
    u8 io_sound_channel2_freq_hi;

    u8 io_sound_channel3_enabled;
    u8 io_sound_channel3_length;
    u8 io_sound_channel3_level;
    u8 io_sound_channel3_freq_lo;
    u8 io_sound_channel3_freq_hi;
    u8 io_sound_channel3_ram[0xf];

    u8 io_sound_channel4_length;
    u8 io_sound_channel4_envelope;
    u8 io_sound_channel4_poly;
    u8 io_sound_channel4_consec_initial;


    int mem_bank_rom, mem_num_banks_rom;
    int mem_bank_ram, mem_num_banks_ram;
    int mem_bank_extram, mem_num_banks_extram;
    int mem_bank_vram, mem_num_banks_vram;
    u8 mem_ram_rtc_select;

    u8 *mem_ROM; /* Between 16K and 4M (banked) */
    u8 *mem_RAM; /* Internal RAM (WRAM), 8K non-CGB, 32K CGB (banked) */
    u8 *mem_EXTRAM; /* External (cartridge) RAM, optional, max 32K (banked) */
    u8 *mem_VRAM; /* Video RAM, 8K non-CGB, 16K CGB (banked) */
    u8 mem_HRAM[0x7f];

    u8 mem_latch_rtc;
    u8 mem_RTC[0x05]; // 0x08 - 0x0c

    /* Cartridge hardware (including memory bank controller) */
    int mbc;
    char has_extram;
    char has_battery;
    char has_rtc;
};


#endif
