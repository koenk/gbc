#ifndef HWDEFS_H
#define HWDEFS_H

static const int GB_FREQ = 4194304; /* Hz */

static const int GB_LCD_WIDTH  = 160; /* px */
static const int GB_LCD_HEIGHT = 144; /* px */

static const int GB_LCD_LY_MAX = 153;

static const int GB_LCD_MODE_0_CLKS = 204;
static const int GB_LCD_MODE_1_CLKS = 4560;
static const int GB_LCD_MODE_2_CLKS = 80;
static const int GB_LCD_MODE_3_CLKS = 172;

static const int GB_DIV_FREQ = 16384;  /* Hz */
static const int GB_TIMA_FREQS[] = { 4096, 262144, 65536, 16384 };  /* Hz */

#endif
