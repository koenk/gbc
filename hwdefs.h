#ifndef HWDEFS_H
#define HWDEFS_H

static const int GB_FREQ = 4194304; /* Hz */

static const int GB_LCD_WIDTH  = 160; /* px */
static const int GB_LCD_HEIGHT = 144; /* px */

static const int GB_LCD_LY_MAX = 153;

static const int GB_HDMA_BLOCK_CLKS = 8;    /* Per block of 0x10 bytes */

static const int GB_LCD_MODE_0_CLKS = 204;  /* H-Blank */
static const int GB_LCD_MODE_1_CLKS = 4560; /* V-Blank */
static const int GB_LCD_MODE_2_CLKS = 80;   /* OAM read */
static const int GB_LCD_MODE_3_CLKS = 172;  /* Line rendering */
static const int GB_LCD_FRAME_CLKS  = 70224; /* Total cycles per frame */

static const int GB_DIV_FREQ = 16384;  /* Hz */
static const int GB_TIMA_FREQS[] = { 4096, 262144, 65536, 16384 };  /* Hz */

static const double GB_SND_DUTY_PERC[] = { .125, .25, .50, .75 };
static const int GB_SND_ENVSTEP_CYC = GB_FREQ/64; /* n*(1/64)th seconds */

static const int ROMHDR_TITLE      = 0x134;
static const int ROMHDR_CGBFLAG    = 0x143;
static const int ROMHDR_CARTTYPE   = 0x147;
static const int ROMHDR_ROMSIZE    = 0x148;
static const int ROMHDR_EXTRAMSIZE = 0x149;

static const int ROM_BANKSIZE      = 0x4000; /* 16K */
static const int WRAM_BANKSIZE     = 0x1000; /* 4K */
static const int VRAM_BANKSIZE     = 0x2000; /* 8K */
static const int EXTRAM_BANKSIZE   = 0x2000; /* 8K */

static const int OAM_SIZE          = 0xa0;

#endif
