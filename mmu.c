#include <stdio.h>

#include "pause.h"
#include "mmu.h"

#if 1
#define MMU_DEBUG_W(fmt, ...) \
    do { \
        printf(" [MMU] [W] " fmt " @%x: %x\n", ##__VA_ARGS__, location, value); \
    } while(0)

#define MMU_DEBUG_R(fmt, ...) \
    do { \
        printf(" [MMU] [R] " fmt "\n", ##__VA_ARGS__); \
    } while(0)
#else
#define MMU_DEBUG_W(...)
#define MMU_DEBUG_R(...)
#endif

void mmu_write(struct gb_state *s, u16 location, u8 value) {
    //MMU_DEBUG_W("Mem write (%x) %x: ", location, value);
    switch (location & 0xf000) {
    case 0x0000: /* 0000 - 1FFF */
    case 0x1000:
        MMU_DEBUG_W("RAM+Timer enable");
        /* Dummy, we always have those enabled. */
        break;
    case 0x2000: /* 2000 - 3FFF */
    case 0x3000:
        MMU_DEBUG_W("ROM bank number");
        assert(value > 0);
        assert(value < s->mem_num_banks_rom);
        s->mem_bank_rom = value;
        break;
    case 0x4000: /* 4000 - 5FFF */
    case 0x5000:
        MMU_DEBUG_W("RAM bank number -OR- RTC register select");
        assert(value > 0);
        assert(value < s->mem_num_banks_ram); /* TODO: RTC 08-0C */
        s->mem_ram_rtc_select = value;
        break;
    case 0x6000: /* 6000 - 7FFF */
    case 0x7000:
        MMU_DEBUG_W("Latch clock data");
        if (s->mem_latch_rtc == 0x01 && value == 0x01) {
            /* TODO... actually latch something? */
            s->mem_latch_rtc = s->mem_latch_rtc;
        }
        s->mem_latch_rtc = value;
        break;
    case 0x8000: /* 8000 - 9FFF */
    case 0x9000:
        MMU_DEBUG_W("VRAM (B%d)", s->mem_bank_vram);
        s->mem_VRAM[s->mem_bank_vram * VRAM_BANKSIZE + location - 0x8000]
            = value;
        break;
    case 0xa000: /* A000 - BFFF */
    case 0xb000:
        MMU_DEBUG_W("EXTRAM (sw)/RTC (B%d)", s->mem_ram_rtc_select);
        if (s->mem_ram_rtc_select < 0x04)
            s->mem_RAM[s->mem_ram_rtc_select * EXTRAM_BANKSIZE + location - 0xa000] = value;
        else if (s->mem_ram_rtc_select >= 0x08 && s->mem_ram_rtc_select <= 0x0c)
            s->mem_RTC[s->mem_ram_rtc_select - 0xa008] = value;
        else
            pause();
        break;
    case 0xc000: /* C000 - CFFF */
        MMU_DEBUG_W("RAM B0");
        s->mem_RAM[location - 0xc000] = value;
        break;
    case 0xd000: /* D000 - DFFF */
        MMU_DEBUG_W("RAM B%d", s->mem_bank_ram);
        s->mem_RAM[s->mem_bank_ram * RAM_BANKSIZE + location - 0xd000] = value;
        break;
    case 0xe000: /* E000 - FDFF */
        MMU_DEBUG_W("ECHO (0xc000 - 0xfdff)");
        pause();
        break;
    case 0xf000:
        if (location < 0xfe00) {
            MMU_DEBUG_W("ECHO (0xc000 - 0xfdff)");
            break;
        }
        if (location < 0xfea0) { /* FE00 - FE9F */
            MMU_DEBUG_W("Sprite attribute table");
            break;
        }
        if (location < 0xff00) { /* FEA0 - FEFF */
            MMU_DEBUG_W("NOT USABLE");
            break;
        }
        if (location < 0xff80) { /* FF00 - FF7F */
            //MMU_DEBUG_W("I/O ports ");

            switch(location) {
            case 0xff00:
                MMU_DEBUG_W("Joypad");
                s->io_buttons = value;
                break;
            case 0xff01:
                MMU_DEBUG_W("Serial link data");
                s->io_serial_data = value;
                break;
            case 0xff02:
                MMU_DEBUG_W("Serial link control");
                s->io_serial_control = value;
                break;
            case 0xff04:
                MMU_DEBUG_W("Timer Divider");
                s->io_timer_DIV = 0x00;
                break;
            case 0xff05:
                MMU_DEBUG_W("Timer Timer");
                s->io_timer_TIMA = value;
                break;
            case 0xff06:
                MMU_DEBUG_W("Timer Modulo");
                s->io_timer_TMA = value;
                break;
            case 0xff07:
                MMU_DEBUG_W("Timer Control");
                s->io_timer_TAC = value;
                break;
            case 0xff0f:
                MMU_DEBUG_W("Int Req");
                s->interrupts_request = value;
                break;
            case 0xff10:
                MMU_DEBUG_W("Sound channel 1 sweep");
                s->io_sound_channel1_sweep = value;
                break;
            case 0xff11:
                MMU_DEBUG_W("Sound channel 1 length/pattern");
                s->io_sound_channel1_length_pattern = value;
                break;
            case 0xff12:
                MMU_DEBUG_W("Sound channel 1 envelope");
                s->io_sound_channel1_envelope = value;
                break;
            case 0xff13:
                MMU_DEBUG_W("Sound channel 1 freq lo");
                s->io_sound_channel1_freq_lo = value;
                break;
            case 0xff14:
                MMU_DEBUG_W("Sound channel 1 freq hi");
                s->io_sound_channel1_freq_hi = value;
                break;
            case 0xff16:
                MMU_DEBUG_W("Sound channel 2 length/pattern");
                s->io_sound_channel2_length_pattern = value;
                break;
            case 0xff17:
                MMU_DEBUG_W("Sound channel 2 envelope");
                s->io_sound_channel2_envelope = value;
                break;
            case 0xff18:
                MMU_DEBUG_W("Sound channel 2 freq lo");
                s->io_sound_channel2_freq_lo = value;
                break;
            case 0xff19:
                MMU_DEBUG_W("Sound channel 2 freq hi");
                s->io_sound_channel2_freq_hi = value;
                break;
            case 0xff1a:
                MMU_DEBUG_W("Sound channel 3 enabled");
                s->io_sound_channel3_enabled = value;
                break;
            case 0xff1b:
                MMU_DEBUG_W("Sound channel 3 length");
                s->io_sound_channel3_length = value;
                break;
            case 0xff1c:
                MMU_DEBUG_W("Sound channel 3 level");
                s->io_sound_channel3_level = value;
                break;
            case 0xff1d:
                MMU_DEBUG_W("Sound channel 3 freq lo");
                s->io_sound_channel3_freq_lo = value;
                break;
            case 0xff1e:
                MMU_DEBUG_W("Sound channel 3 freq hi");
                s->io_sound_channel3_freq_hi = value;
                break;
            case 0xff20:
                MMU_DEBUG_W("Sound channel 4 length");
                s->io_sound_channel4_length = value;
                break;
            case 0xff21:
                MMU_DEBUG_W("Sound channel 4 envelope");
                s->io_sound_channel4_envelope = value;
                break;
            case 0xff22:
                MMU_DEBUG_W("Sound channel 4 polynomial counter");
                s->io_sound_channel4_poly = value;
                break;
            case 0xff23:
                MMU_DEBUG_W("Sound channel 4 Counter/consecutive; Inital");
                s->io_sound_channel4_consec_initial = value;
                break;
            case 0xff24:
                MMU_DEBUG_W("Sound channel control");
                s->io_sound_terminal_control = value;
                break;
            case 0xff25:
                MMU_DEBUG_W("Sound output terminal");
                s->io_sound_out_terminal = value;
                break;
            case 0xff26:
                MMU_DEBUG_W("Sound enabled flags");
                s->io_sound_enabled = value;
                break;
            case 0xff40:
                MMU_DEBUG_W("LCD Control");
                s->io_lcd_LCDC = value;
                break;
            case 0xff41:
                MMU_DEBUG_W("LCD Stat");
                s->io_lcd_STAT = value;
                break;
            case 0xff42:
                MMU_DEBUG_W("Scroll Y");
                s->io_lcd_SCY = value;
                break;
            case 0xff43:
                MMU_DEBUG_W("Scroll X");
                s->io_lcd_SCX = value;
                break;
            case 0xff44:
                MMU_DEBUG_W("LCD LY");
                s->io_lcd_LY = 0x0;
                s->io_lcd_STAT = (s->io_lcd_STAT & 0xfb) | (s->io_lcd_LY == s->io_lcd_LYC);
                break;
            case 0xff45:
                MMU_DEBUG_W("LCD LYC");
                s->io_lcd_LYC = value;
                s->io_lcd_STAT = (s->io_lcd_STAT & 0xfb) | (s->io_lcd_LY == s->io_lcd_LYC);
                break;
            case 0xff46:
                MMU_DEBUG_W("DMA source=%.4x dest=0xfe00 (OAM)", value << 8);
                /* TODO */
                break;
            case 0xff47:
                MMU_DEBUG_W("Background palette");
                s->io_lcd_BGP = value;
                break;
            case 0xff48:
                MMU_DEBUG_W("Object palette 0");
                s->io_lcd_OBP0 = value;
                break;
            case 0xff49:
                MMU_DEBUG_W("Object palette 1");
                s->io_lcd_OBP1 = value;
                break;
            case 0xff4a:
                MMU_DEBUG_W("Window Y");
                s->io_lcd_WY = value;
                break;
            case 0xff4b:
                MMU_DEBUG_W("window X");
                s->io_lcd_WX = value;
                break;
            case 0xff4f:
                MMU_DEBUG_W("VRAM Bank");
                s->mem_bank_vram = value & 1;
                break;
            case 0xff56:
                MMU_DEBUG_W("Infrared");
                s->io_infrared = value;
                break;
            case 0xff68:
                MMU_DEBUG_W("Background Palette Index");
                s->io_lcd_BGPI = value;
                break;
            case 0xff69:
                MMU_DEBUG_W("Background Palette Data");
                s->io_lcd_BGPD = value;
                if (s->io_lcd_BGPI & (1 << 7))
                    s->io_lcd_BGPI = (((s->io_lcd_BGPI & 0x3f) + 1) & 0x3f) | (1 << 7);
                break;
            case 0xff6a:
                MMU_DEBUG_W("Sprite Palette Index");
                s->io_lcd_OBPI = value;
                break;
            case 0xff6b:
                MMU_DEBUG_W("Sprite Palette Data");
                s->io_lcd_OBPD = value;
                if (s->io_lcd_OBPI & (1 << 7))
                    s->io_lcd_OBPI = (((s->io_lcd_OBPI & 0x3f) + 1) & 0x3f) | (1 << 7);
                break;
            case 0xff70:
                MMU_DEBUG_W("RAM Bank");
                if (value == 0)
                    s->mem_bank_ram = 1;
                else if (value < 8)
                    s->mem_bank_ram = value;
                else
                    pause();
                break;
            default:
                MMU_DEBUG_W("UNKNOWN");
                pause();
            }

            break;
        }

        if (location < 0xffff) { /* FF80 - FFFE */
            MMU_DEBUG_W("HRAM ");
            if (location == 0xffa0) {
                MMU_DEBUG_W("HRAM FFA0: %x", value);
            }
            s->mem_HRAM[location - 0xff80] = value;
            break;
        }
        if (location == 0xffff) { /* FFFF */
            MMU_DEBUG_W("Interrupt enable");
            s->interrupts_enable = value;
            break;
        }
    default:
        MMU_DEBUG_W("INVALID LOCATION");
        pause();
        break;
    }
}

u8 mmu_read(struct gb_state *s, u16 location) {
    /*MMU_DEBUG_R("Mem read (%x): ", location); */
    if (s->in_bios && location < 0x100)
    {
        /*MMU_DEBUG_R("BIOS: %04x: %02x", location, s->bios[location]); */
        return s->bios[location];
    }

    switch (location & 0xf000) {
    case 0x0000: /* 0000 - 3FFF */
    case 0x1000:
    case 0x2000:
    case 0x3000:
        /*MMU_DEBUG_R("ROM B0 @ %4x", location); */
        return s->mem_ROM[location];
    case 0x4000: /* 4000 - 7FFF */
    case 0x5000:
    case 0x6000:
    case 0x7000:
        //MMU_DEBUG_R("ROM B%d, %4x", s->mem_bank_rom, s->mem_bank_rom * 0x4000 + (location - 0x4000));
        assert(s->mem_num_banks_rom > 0);
        assert(s->mem_bank_rom < s->mem_num_banks_rom);
        return s->mem_ROM[s->mem_bank_rom * 0x4000 + (location - 0x4000)];
        break;
    case 0x8000: /* 8000 - 9FFF */
    case 0x9000:
        MMU_DEBUG_R("VRAM");
        return s->mem_VRAM[s->mem_bank_vram * VRAM_BANKSIZE + location - 0x8000];
        break;
    case 0xa000: /* A000 - BFFF */
    case 0xb000:
        MMU_DEBUG_R("EXTRAM (sw)/RTC");
        if (s->mem_ram_rtc_select < 0x04)
            return s->mem_RAM[s->mem_ram_rtc_select * EXTRAM_BANKSIZE + location - 0xa000];
        else if (s->mem_ram_rtc_select >= 0x08 && s->mem_ram_rtc_select <= 0x0c)
            return s->mem_RTC[s->mem_ram_rtc_select - 0xa008];

        pause();
        return 0;
    case 0xc000: /* C000 - CFFF */
        MMU_DEBUG_R("RAM B0  @%x", (location - 0xc000));
        return s->mem_RAM[location - 0xc000];
    case 0xd000: /* D000 - DFFF */
        MMU_DEBUG_R("RAM B%d @%x", s->mem_bank_ram, location - 0xd000);
        return s->mem_RAM[s->mem_bank_ram * RAM_BANKSIZE + location - 0xd000];
        break;
    case 0xe000: /* E000 - FDFF */
        MMU_DEBUG_R("ECHO (0xc000 - 0xddff) B0");
        pause();
        break;
    case 0xf000:
        if (location < 0xfe00) {
            MMU_DEBUG_R("ECHO (0xc000 - 0xddff) B0");
            break;
        }

        if (location < 0xfea0) { /* FE00 - FE9F */
            MMU_DEBUG_R("Sprite attribute table");
            break;
        }

        if (location < 0xff00) { /* FEA0 - FEFF */
            MMU_DEBUG_R("NOT USABLE");
            break;
        }

        if (location < 0xff80) { /* FF00 - FF7F */
            /*MMU_DEBUG_R("I/O ports "); */
            switch (location) {
            case 0xff00:
                MMU_DEBUG_R("Joypad");
                if ((s->io_buttons & (1 << 4)) == 0)
                    return (s->io_buttons & 0xf0) | (s->io_buttons_dirs & 0x0f);
                else if ((s->io_buttons & (1 << 5)) == 0)
                    return (s->io_buttons & 0xf0) | (s->io_buttons_buttons & 0x0f);
                return (s->io_buttons & 0xf0) | (s->io_buttons_buttons & 0x0f);
            case 0xff01:
                MMU_DEBUG_R("Serial link data");
                return s->io_serial_data;
            case 0xff02:
                MMU_DEBUG_R("Serial link control");
                return s->io_serial_control;
            case 0xff04:
                MMU_DEBUG_R("Timer Divider");
                return s->io_timer_DIV;
            case 0xff05:
                MMU_DEBUG_R("Timer Timer");
                return s->io_timer_TIMA;
            case 0xff06:
                MMU_DEBUG_R("Timer Modulo");
                return s->io_timer_TMA;
            case 0xff07:
                MMU_DEBUG_R("Timer Control");
                return s->io_timer_TAC;
            case 0xff0f:
                MMU_DEBUG_R("Int Reqs");
                return s->interrupts_request;
            case 0xff10:
                MMU_DEBUG_R("Sound channel 1 sweep");
                return s->io_sound_channel1_sweep;
            case 0xff12:
                MMU_DEBUG_R("Sound channel 1 envelope");
                return s->io_sound_channel1_envelope;
            case 0xff1a:
                MMU_DEBUG_R("Sound channel 3 enabled");
                return s->io_sound_channel3_enabled;
            case 0xff1c:
                MMU_DEBUG_R("Sound channel 3 level");
                return s->io_sound_channel3_level;
            case 0xff25:
                MMU_DEBUG_R("Sound output terminal");
                return s->io_sound_out_terminal;
            case 0xff26:
                MMU_DEBUG_R("Sound enabled flags");
                return s->io_sound_enabled;
            case 0xff40:
                MMU_DEBUG_R("LCD Control (%04x: %02x)", location, s->io_lcd_LCDC);
                return s->io_lcd_LCDC;
            case 0xff42:
                MMU_DEBUG_R("Scroll Y");
                return s->io_lcd_SCY;
            case 0xff43:
                MMU_DEBUG_R("Scroll X");
                return s->io_lcd_SCX;
            case 0xff44:
                /*MMU_DEBUG_R("LCD LY"); */
                return s->io_lcd_LY;
            case 0xff45:
                MMU_DEBUG_R("LCD LYC");
                return s->io_lcd_LYC;
            case 0xff47:
                MMU_DEBUG_R("Background palette");
                return s->io_lcd_BGP;
            case 0xff48:
                MMU_DEBUG_R("Object palette 0");
                return s->io_lcd_OBP0;
            case 0xff49:
                MMU_DEBUG_R("Object palette 1");
                return s->io_lcd_OBP1;
            case 0xff4a:
                MMU_DEBUG_R("Window Y");
                return s->io_lcd_WY;
            case 0xff4b:
                MMU_DEBUG_R("Window X");
                return s->io_lcd_WX;
            case 0xff4f:
                MMU_DEBUG_R("VRAM Bank");
                return s->mem_bank_vram & 1;
            case 0xff56:
                MMU_DEBUG_R("Infrared");
                return s->io_infrared;
            case 0xff70:
                MMU_DEBUG_R("RAM bank");
                return s->mem_bank_ram;
            }

            MMU_DEBUG_R("UNKNOWN");
            pause();
        }
        if (location < 0xffff) { /* FF80 - FFFE */
            MMU_DEBUG_R("HRAM  @%x (%x)", location - 0xff80, s->mem_HRAM[location - 0xff80]);
            return s->mem_HRAM[location - 0xff80];
        }
        if (location == 0xffff) { /* FFFF */
            MMU_DEBUG_R("Interrupt enable");
            return s->interrupts_enable;
        }

    default:
        MMU_DEBUG_R("INVALID LOCATION");
        pause();
    }
    return 0;
}

u16 mmu_read16(struct gb_state *s, u16 location) {
    return mmu_read(s, location) | ((u16)mmu_read(s, location + 1) << 8);
}

void mmu_write16(struct gb_state *s, u16 location, u16 value) {
    mmu_write(s, location, value & 0xff);
    mmu_write(s, location + 1, value >> 8);
}

u16 mmu_pop16(struct gb_state *s) {
    u16 val = mmu_read16(s, s->sp);
    s->sp += 2;
    return val;
}

void mmu_push16(struct gb_state *s, u16 value) {
    s->sp -= 2;
    mmu_write16(s, s->sp, value);
}
