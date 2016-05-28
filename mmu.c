#include <stdio.h>

#include "pause.h"
#include "mmu.h"

void mmu_write(struct gb_state *s, u16 location, u8 value) {
    // MBC3

    printf("Mem write (%x) %x: ", location, value);
    switch (location & 0xf000) {
    case 0x0000: // 0000 - 1FFF
    case 0x1000:
        printf("RAM+Timer enable\n");
        // Dummy, we always have those enabled.
        break;
    case 0x2000: // 2000 - 3FFF
    case 0x3000:
        printf("ROM bank number\n");
        s->mem_bank_rom = value;
        break;
    case 0x4000: // 4000 - 5FFF
    case 0x5000:
        printf("RAM bank number -OR- RTC register select\n");
        s->mem_ram_rtc_select = value;
        break;
    case 0x6000: // 6000 - 7FFF
    case 0x7000:
        printf("Latch clock data\n");
        if (s->mem_latch_rtc == 0x01 && value == 0x01) {
            // TODO... actually latch something?
            s->mem_latch_rtc = s->mem_latch_rtc;
        }
        s->mem_latch_rtc = value;
        break;
    case 0x8000: // 8000 - 9FFF
    case 0x9000:
        printf("VRAM\n");
        s->mem_VRAM[s->mem_bank_vram][location - 0x8000] = value;
        break;
    case 0xa000: // A000 - BFFF
    case 0xb000:
        printf("RAM (sw)/RTC\n");
        if (s->mem_ram_rtc_select < 0x04)
            s->mem_RAM[s->mem_ram_rtc_select][location - 0xa000] = value;
        else if (s->mem_ram_rtc_select >= 0x08 && s->mem_ram_rtc_select <= 0x0c)
            s->mem_RTC[s->mem_ram_rtc_select - 0xa008] = value;
        else
            pause();
        break;
    case 0xc000: // C000 - CFFF
        printf("WRAM B0  @%x\n", (location - 0xc000));
        s->mem_WRAM[0][location - 0xc000] = value;
        break;
    case 0xd000: // D000 - DFFF
        printf("WRAM B1-7 (switchable) @%x, bank %d\n", location - 0xd000,
                s->mem_bank_wram);
        s->mem_WRAM[s->mem_bank_wram][location - 0xd000] = value;
        break;
    case 0xe000: // E000 - FDFF
        printf("ECHO (0xc000 - 0xfdff) B0\n");
        pause();
        break;
    case 0xf000:
        if (location < 0xfe00) {
            printf("ECHO (0xc000 - 0xfdff) B0\n");
            break;
        }
        if (location < 0xfea0) { // FE00 - FE9F
            printf("Sprite attribute table\n");
            break;
        }
        if (location < 0xff00) { // FEA0 - FEFF
            printf("NOT USABLE\n");
            break;
        }
        if (location < 0xff80) { // FF00 - FF7F
            printf("I/O ports ");

            switch(location) {
            case 0xff00:
                printf("Joypad\n");
                s->io_buttons = value;
                break;
            case 0xff01:
                printf("Serial link data\n");
                s->io_serial_data = value;
                break;
            case 0xff02:
                printf("Serial link control\n");
                s->io_serial_control = value;
                break;
            case 0xff04:
                printf("Timer Divider\n");
                s->io_timer_DIV = 0x00;
                break;
            case 0xff05:
                printf("Timer Timer\n");
                s->io_timer_TIMA = value;
                break;
            case 0xff06:
                printf("Timer Modulo\n");
                s->io_timer_TMA = value;
                break;
            case 0xff07:
                printf("Timer Control\n");
                s->io_timer_TAC = value;
                break;
            case 0xff0f:
                printf("Int Req\n");
                s->interrupts_request = value;
                break;
            case 0xff10:
                printf("Sound channel 1 sweep\n");
                s->io_sound_channel1_sweep = value;
                break;
            case 0xff11:
                printf("Sound channel 1 length/pattern\n");
                s->io_sound_channel1_length_pattern = value;
                break;
            case 0xff12:
                printf("Sound channel 1 envelope\n");
                s->io_sound_channel1_envelope = value;
                break;
            case 0xff13:
                printf("Sound channel 1 freq lo\n");
                s->io_sound_channel1_freq_lo = value;
                break;
            case 0xff14:
                printf("Sound channel 1 freq hi\n");
                s->io_sound_channel1_freq_hi = value;
                break;
            case 0xff16:
                printf("Sound channel 2 length/pattern\n");
                s->io_sound_channel2_length_pattern = value;
                break;
            case 0xff17:
                printf("Sound channel 2 envelope\n");
                s->io_sound_channel2_envelope = value;
                break;
            case 0xff18:
                printf("Sound channel 2 freq lo\n");
                s->io_sound_channel2_freq_lo = value;
                break;
            case 0xff19:
                printf("Sound channel 2 freq hi\n");
                s->io_sound_channel2_freq_hi = value;
                break;
            case 0xff1a:
                printf("Sound channel 3 enabled\n");
                s->io_sound_channel3_enabled = value;
                break;
            case 0xff1b:
                printf("Sound channel 3 length\n");
                s->io_sound_channel3_length = value;
                break;
            case 0xff1c:
                printf("Sound channel 3 level\n");
                s->io_sound_channel3_level = value;
                break;
            case 0xff1d:
                printf("Sound channel 3 freq lo\n");
                s->io_sound_channel3_freq_lo = value;
                break;
            case 0xff1e:
                printf("Sound channel 3 freq hi\n");
                s->io_sound_channel3_freq_hi = value;
                break;
            case 0xff20:
                printf("Sound channel 4 length\n");
                s->io_sound_channel4_length = value;
                break;
            case 0xff21:
                printf("Sound channel 4 envelope\n");
                s->io_sound_channel4_envelope = value;
                break;
            case 0xff22:
                printf("Sound channel 4 polynomial counter\n");
                s->io_sound_channel4_poly = value;
                break;
            case 0xff23:
                printf("Sound channel 4 Counter/consecutive; Inital\n");
                s->io_sound_channel4_consec_initial = value;
                break;
            case 0xff24:
                printf("Sound channel control\n");
                s->io_sound_terminal_control = value;
                break;
            case 0xff25:
                printf("Sound output terminal\n");
                s->io_sound_out_terminal = value;
                break;
            case 0xff26:
                printf("Sound enabled flags\n");
                s->io_sound_enabled = value;
                break;
            case 0xff40:
                printf("LCD Control\n");
                s->io_lcd_LCDC = value;
                break;
            case 0xff41:
                printf("LCD Stat\n");
                s->io_lcd_STAT = value;
                break;
            case 0xff42:
                printf("Scroll Y\n");
                s->io_lcd_SCY = value;
                break;
            case 0xff43:
                printf("Scroll X\n");
                s->io_lcd_SCX = value;
                break;
            case 0xff44:
                printf("LCD LY\n");
                s->io_lcd_LY = 0x0;
                s->io_lcd_STAT = (s->io_lcd_STAT & 0xfb) | (s->io_lcd_LY == s->io_lcd_LYC);
                break;
            case 0xff45:
                printf("LCD LYC\n");
                s->io_lcd_LYC = value;
                s->io_lcd_STAT = (s->io_lcd_STAT & 0xfb) | (s->io_lcd_LY == s->io_lcd_LYC);
                break;
            case 0xff47:
                printf("Background palette\n");
                s->io_lcd_BGP = value;
                break;
            case 0xff48:
                printf("Object palette 0\n");
                s->io_lcd_OBP0 = value;
                break;
            case 0xff49:
                printf("Object palette 1\n");
                s->io_lcd_OBP1 = value;
                break;
            case 0xff4a:
                printf("Window Y\n");
                s->io_lcd_WY = value;
                break;
            case 0xff4b:
                printf("window X\n");
                s->io_lcd_WX = value;
                break;
            case 0xff4f:
                printf("VRAM Bank\n");
                s->mem_bank_vram = value & 1;
                break;
            case 0xff56:
                printf("Infrared\n");
                s->io_infrared = value;
                break;
            case 0xff68:
                printf("Background Palette Index\n");
                s->io_lcd_BGPI = value;
                break;
            case 0xff69:
                printf("Background Palette Data\n");
                s->io_lcd_BGPD = value;
                if (s->io_lcd_BGPI & (1 << 7))
                    s->io_lcd_BGPI = (((s->io_lcd_BGPI & 0x3f) + 1) & 0x3f) | (1 << 7);
                break;
            case 0xff6a:
                printf("Sprite Palette Index\n");
                s->io_lcd_OBPI = value;
                break;
            case 0xff6b:
                printf("Sprite Palette Data\n");
                s->io_lcd_OBPD = value;
                if (s->io_lcd_OBPI & (1 << 7))
                    s->io_lcd_OBPI = (((s->io_lcd_OBPI & 0x3f) + 1) & 0x3f) | (1 << 7);
                break;
            case 0xff70:
                printf("WRAM Bank\n");
                if (value == 0)
                    s->mem_bank_wram = 1;
                else if (value < 8)
                    s->mem_bank_wram = value;
                else
                    pause();
                break;
            default:
                printf("UNKNOWN\n");
                pause();
            }

            break;
        }

        if (location < 0xffff) { // FF80 - FFFE
            printf("HRAM  @%x\n", location - 0xff80);
            if (location == 0xffa0) {
                printf("HRAM FFA0: %x\n", value);
            }
            s->mem_HRAM[location - 0xff80] = value;
            break;
        }
        if (location == 0xffff) { // FFFF
            printf("Interrupt enable\n");
            s->interrupts_enable = value;
            break;
        }
    default:
        printf("INVALID LOCATION\n");
        pause();
        break;
    }
}

u8 mmu_read(struct gb_state *s, u16 location) {
    // MBC3

    //printf("Mem read (%x): ", location);

    switch (location & 0xf000) {
    case 0x0000: // 0000 - 3FFF
    case 0x1000:
    case 0x2000:
    case 0x3000:
        //printf("CA ROM fixed @ %4x\n", location);
        return s->rom[location];
    case 0x4000: // 4000 - 7FFF
    case 0x5000:
    case 0x6000:
    case 0x7000:
        if (location > 0x7e50)
            pause();
        printf("CA ROM switchable, bank %d, %4x\n", s->mem_bank_rom, s->mem_bank_rom * 0x4000 + (location - 0x4000));
        return s->rom[s->mem_bank_rom * 0x4000 + (location - 0x4000)];
        break;
    case 0x8000: // 8000 - 9FFF
    case 0x9000:
        printf("VRAM\n");
        return s->mem_VRAM[s->mem_bank_vram][location - 0x8000];
        break;
    case 0xa000: // A000 - BFFF
    case 0xb000:
        printf("RAM (sw)/RTC\n");
        if (s->mem_ram_rtc_select < 0x04)
            return s->mem_RAM[s->mem_ram_rtc_select][location - 0xa000];
        else if (s->mem_ram_rtc_select >= 0x08 && s->mem_ram_rtc_select <= 0x0c)
            return s->mem_RTC[s->mem_ram_rtc_select - 0xa008];

        pause();
        return 0;
    case 0xc000: // C000 - CFFF
        printf("WRAM B0  @%x\n", (location - 0xc000));
        return s->mem_WRAM[0][location - 0xc000];
    case 0xd000: // D000 - DFFF
        printf("WRAM B1-7 (switchable) @%x, bank %d\n", location - 0xd000, s->mem_bank_wram);
        return s->mem_WRAM[s->mem_bank_wram][location - 0xd000];
        break;
    case 0xe000: // E000 - FDFF
        printf("ECHO (0xc000 - 0xddff) B0\n");
        pause();
        break;
    case 0xf000:
        if (location < 0xfe00) {
            printf("ECHO (0xc000 - 0xddff) B0\n");
            break;
        }

        if (location < 0xfea0) { // FE00 - FE9F
            printf("Sprite attribute table\n");
            break;
        }

        if (location < 0xff00) { // FEA0 - FEFF
            printf("NOT USABLE\n");
            break;
        }

        if (location < 0xff80) { // FF00 - FF7F
            //printf("I/O ports ");
            switch (location) {
            case 0xff00:
                printf("Joypad\n");
                if ((s->io_buttons & (1 << 4)) == 0)
                    return (s->io_buttons & 0xf0) | (s->io_buttons_dirs & 0x0f);
                else if ((s->io_buttons & (1 << 5)) == 0)
                    return (s->io_buttons & 0xf0) | (s->io_buttons_buttons & 0x0f);
                return (s->io_buttons & 0xf0) | (s->io_buttons_buttons & 0x0f);
            case 0xff01:
                printf("Serial link data\n");
                return s->io_serial_data;
            case 0xff02:
                printf("Serial link control\n");
                return s->io_serial_control;
            case 0xff04:
                printf("Timer Divider\n");
                return s->io_timer_DIV;
            case 0xff05:
                printf("Timer Timer\n");
                return s->io_timer_TIMA;
            case 0xff06:
                printf("Timer Modulo\n");
                return s->io_timer_TMA;
            case 0xff07:
                printf("Timer Control\n");
                return s->io_timer_TAC;
            case 0xff0f:
                printf("Int Reqs\n");
                return s->interrupts_request;
            case 0xff10:
                printf("Sound channel 1 sweep\n");
                return s->io_sound_channel1_sweep;
            case 0xff12:
                printf("Sound channel 1 envelope\n");
                return s->io_sound_channel1_envelope;
            case 0xff1a:
                printf("Sound channel 3 enabled\n");
                return s->io_sound_channel3_enabled;
            case 0xff1c:
                printf("Sound channel 3 level\n");
                return s->io_sound_channel3_level;
            case 0xff25:
                printf("Sound output terminal\n");
                return s->io_sound_out_terminal;
            case 0xff26:
                printf("Sound enabled flags\n");
                return s->io_sound_enabled;
            case 0xff40:
                printf("LCD Control\n");
                return s->io_lcd_LCDC;
            case 0xff42:
                printf("Scroll Y\n");
                return s->io_lcd_SCY;
            case 0xff43:
                printf("Scroll X\n");
                return s->io_lcd_SCX;
            case 0xff44:
                //printf("LCD LY\n");
                return s->io_lcd_LY;
            case 0xff45:
                printf("LCD LYC\n");
                return s->io_lcd_LYC;
            case 0xff47:
                printf("Background palette\n");
                return s->io_lcd_BGP;
            case 0xff48:
                printf("Object palette 0\n");
                return s->io_lcd_OBP0;
            case 0xff49:
                printf("Object palette 1\n");
                return s->io_lcd_OBP1;
            case 0xff4a:
                printf("Window Y\n");
                return s->io_lcd_WY;
            case 0xff4b:
                printf("Window X\n");
                return s->io_lcd_WX;
            case 0xff4f:
                printf("VRAM Bank\n");
                return s->mem_bank_vram & 1;
            case 0xff56:
                printf("Infrared\n");
                return s->io_infrared;
            case 0xff70:
                printf("WRAM bank\n");
                return s->mem_bank_wram;
            }

            printf("UNKNOWN\n");
            pause();
        }
        if (location < 0xffff) { // FF80 - FFFE
            printf("HRAM  @%x (%d)\n", location - 0xff80, s->mem_HRAM[location - 0xff80]);
            return s->mem_HRAM[location - 0xff80];
        }
        if (location == 0xffff) { // FFFF
            printf("Interrupt enable\n");
            return s->interrupts_enable;
        }

    default:
        printf("INVALID LOCATION\n");
        pause();
    }
    return 0;
}
