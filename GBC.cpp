#include "GBC.h"
#include <string.h>
#include <stdio.h>

const int GB_FREQ = 4194304; // Hz
const int GB_LCD_WIDTH = 160;
const int GB_LCD_HEIGHT = 144;

const int GB_LCD_LY_MAX = 153;
const int GB_LCD_WX_MAX = 166;
const int GB_LCD_WY_MAX = 143;


GBC::GBC(u8 *rom_data, int rom_type) {
    rom = rom_data;
    type = rom_type;
    freq = GB_FREQ;

    reset_state();
}


GBC::~GBC(void) {
}

void GBC::reset_state(void) {
    AF = 0x01B0;
    BC = 0x0013;
    DE = 0x00D8;
    HL = 0x014D;
        
    sp = 0xFFFE;
    pc = 0x0100;

    if (type == 1) {
        AF = 0x1180;
        BC = 0x0000;
        DE = 0xff56;
        HL = 0x000d;
    }

    interrupts_master_enabled = 1;
    interrupts_enable  = 0x0;
    interrupts_request = 0x0;

    io_lcd_SCX  = 0x00;
    io_lcd_SCY  = 0x00;
    io_lcd_WX   = 0x00;
    io_lcd_WY   = 0x00;
    io_lcd_BGP  = 0xfc;
    io_lcd_OBP0 = 0xff;
    io_lcd_OBP1 = 0xff;
    io_lcd_LCDC = 0x91;
    io_lcd_LY   = 0x00;
    io_lcd_LYC  = 0x00;

    io_timer_DIV  = 0x00;
    io_timer_TIMA = 0x00;
    io_timer_TMA  = 0x00;
    io_timer_TAC  = 0x00;

    io_serial_data    = 0x00;
    io_serial_control = 0x00;

    memset(mem_HRAM, 0, 0xfffe - 0xff80 + 1);
    memset(mem_WRAM0, 0, 0xcfff - 0xc000 + 1);
}

void GBC::print_header_info(void) {
    printf("Title: %s\n", &rom[0x134]);
    
    printf("Cart type: ");
    u8 cart_type = rom[0x147];
    switch(cart_type) {
    case 0x00: printf("ROM ONLY\n"); break;
    case 0x01: printf("MBC1\n"); break;
    case 0x02: printf("MBC1+RAM\n"); break;
    case 0x03: printf("MBC1+RAM+BATTERY\n"); break;
    case 0x04: printf("MBC2\n"); break;
    case 0x06: printf("MBC2+BATTERY\n"); break;
    case 0x08: printf("ROM+RAM\n"); break;
    case 0x09: printf("ROM+RAM+BATTERY\n"); break;
    case 0x0b: printf("MMM01\n"); break;
    case 0x0c: printf("MMM01+RAM\n"); break;
    case 0x0d: printf("MMM01+RAM+BATTERY\n"); break;
    case 0x0f: printf("MBC3+TIMER+BATTERY\n"); break;
    case 0x10: printf("MBC3+TIMER+RAM+BATTERY\n"); break;
    case 0x11: printf("MBC3\n"); break;
    case 0x12: printf("MBC3+RAM\n"); break;
    case 0x13: printf("MBC3+RAM+BATTERY\n"); break;
    case 0x15: printf("MBC4\n"); break;
    case 0x16: printf("MBC4+RAM\n"); break;
    case 0x17: printf("MBC4+RAM+BATTERY\n"); break;
    case 0x19: printf("MBC5\n"); break;
    case 0x1a: printf("MBC5+RAM\n"); break;
    case 0x1b: printf("MBC5+RAM+BATTERY\n"); break;
    case 0x1c: printf("MBC5+RUMBLE\n"); break;
    case 0x1d: printf("MBC5+RUMBLE+RAM\n"); break;
    case 0x1e: printf("MBC5+RUMBLE+RAM+BATTERY\n"); break;
    case 0xfc: printf("POCKET CAMERA\n"); break;
    case 0xfd: printf("BANDAI TAMA5\n"); break;
    case 0xfe: printf("HuC3\n"); break;
    case 0xff: printf("HuC1+RAM+BATTERY\n"); break;
    }
}

void GBC::print_regs(void) {
    printf("\n\tAF\tBC\tDE\tHL\tSP\tPC\t\tLY\n");
    printf("\t%04x\t%04x\t%04x\t%04x\t%04x\t%04x\t\t%04x\n\n", AF, BC, DE, HL, sp, pc, io_lcd_LY);
}

int GBC::do_cycle(void) {
    // TODO:
    // * interrupts
    // * LY
    // * timer

    u8 op = rom[pc++];

    u8 temp1, temp2;
    s8 stemp;

    switch (op) {
    case 0x00: // NOP
        break;
    case 0x01: // LD BC, nnnn
        temp1 = rom[pc++];
        temp2 = rom[pc++];
        BC = temp1 | (temp2 << 8);
        break;
    case 0x18: // JR nn (offset)
        stemp = rom[pc++];
        pc += stemp;
        break;
    case 0x20: // JR NZ, nn (offset)
        if (!(AF & FLAG_Z)) {
            stemp = rom[pc++];
            pc += stemp;
        } else {
            pc++;
        }
        break;
    case 0x28: // JR Z, nn (offset)
        if (AF & FLAG_Z) {
            stemp = rom[pc++];
            pc += stemp;
        } else {
            pc++;
        }
        break;
    case 0x3e: // LD a, nn
        temp1 = rom[pc++];
        A(temp1);
        break;
    case 0x47: // LD B, A
        B(A());
        break;
    case 0xaf: // XOR a (so clear a)
        A(0);
        break;
    case 0xc3: // JP nnnn
        temp1 = rom[pc++];
        temp2 = rom[pc++];
        pc = temp1 | (temp2 << 8);
        break;
    case 0xcb: // EXTENDED INSTRUCTION
        op = rom[pc++];
        switch (op) {
        case 0x87: // RES 0, A (reset bit 0 to 0 in A)
            A(A() & ~(1 << 0));
            break;
        default:
            printf("Unknown extended opcode %x\n", op);
            return 1;
        }
        break;
    case 0xcd: // CALL nnnn
        temp1 = rom[pc++];
        temp2 = rom[pc++];

        mem_write(--sp, (pc & 0xff00) >> 8);
        mem_write(--sp,  pc & 0x00ff);

        pc = temp1 | (temp2 << 8);
        break;
    case 0xe0: // LD (ffnn), A
        temp1 = rom[pc++];
        mem_write(0xff00 | temp1, A());
        break;
    case 0xea: // LD (nnnn), A
        temp1 = rom[pc++];
        temp2 = rom[pc++];
        mem_write(temp1 | (temp2 << 8), A());
        break;
    case 0xf0: // LD A, (ffnn)
        temp1 = rom[pc++];
        A(mem_read(0xff00 | temp1));
        break;
    case 0xf3: // DI
        interrupts_master_enabled = 0;
        break;
    case 0xfe: // CP nn
        temp1 = rom[pc++];
        stemp = A() - temp1;
        F((stemp == 0 ? FLAG_Z : 0) | 
          FLAG_N |
          ((A() ^ temp1 ^ (stemp & 0xff)) & 0x10 ? FLAG_H : 0) |
          (stemp < 0 ? FLAG_C : 0));
        break;
    default:
        printf("Unknown opcode %x\n", op);
        return 1;
    }
    return 0;
}


void GBC::mem_write(u16 location, u16 value) {
    // MBC3

    printf("Mem write (%x) %x: ", location, value);
    switch (location & 0xf000) {
    case 0x0000: // 0000 - 1FFF
    case 0x1000:
        printf("RAM+Timer enable\n");
        break;
    case 0x2000: // 2000 - 3FFF
    case 0x3000:
        printf("ROM bank number\n");
        break;
    case 0x4000: // 4000 - 5FFF
    case 0x5000:
        printf("RAM bank number -OR- RTC register select\n");
        break;
    case 0x6000: // 6000 - 7FFF
    case 0x7000:
        printf("Latch clock data\n");
        break;
    case 0x8000: // 8000 - 9FFF
    case 0x9000:
        printf("VRAM\n");
        break;
    case 0xa000: // A000 - BFFF
    case 0xb000:
        printf("RAM (sw)/RTC\n");
        break;
    case 0xc000: // C000 - CFFF
        printf("W RAM B0  @%x\n", (location - 0xc000));
        mem_WRAM0[location - 0xc000] = value;
        break;
    case 0xd000: // D000 - DFFF
        printf("W RAM B1 (switchable)\n");
        break;
    case 0xe000: // E000 - FDFF
        printf("ECHO (0xc000 - 0xfdff) B0\n");
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
            case 0xff01:
                printf("Serial link data\n");
                io_serial_data = value;
                break;
            case 0xff02:
                printf("Serial link control\n");
                io_serial_control = value;
                break;
            case 0xff04:
                printf("Timer Divider\n");
                io_timer_DIV = 0x00;
                break;
            case 0xff05:
                printf("Timer Timer\n");
                io_timer_TIMA = value;
                break;
            case 0xff06:
                printf("Timer Modulo\n");
                io_timer_TMA = value;
                break;
            case 0xff07:
                printf("Timer Control\n");
                io_timer_TAC = value;
                break;
            case 0xff0f:
                printf("Int Req\n");
                interrupts_request = value;
                break;
            case 0xff40:
                printf("LCD Control\n");
                io_lcd_LCDC = value;
                break;
            case 0xff42:
                printf("Scroll Y\n");
                io_lcd_SCY = value;
                break;
            case 0xff43:
                printf("Scroll X\n");
                io_lcd_SCX = value;
                break;
            case 0xff44:
                printf("LCD LY\n");
                //io_lcd_LY = 0x0
                break;
            case 0xff45:
                printf("LCD LYC\n");
                io_lcd_LYC = value;
                break;
            case 0xff47:
                printf("Background palette\n");
                io_lcd_BGP = value;
                break;
            case 0xff48:
                printf("Object palette 0\n");
                io_lcd_OBP0 = value;
                break;
            case 0xff49:
                printf("Object palette 1\n");
                io_lcd_OBP1 = value;
                break;
            case 0xff4a:
                printf("Window Y\n");
                io_lcd_WY = value;
                break;
            case 0xff4b:
                printf("window X\n");
                io_lcd_WX = value;
                break;
            default:
                printf("UNKNOWN\n");
            }
            
            break;
        }

        if (location < 0xffff) { // FF80 - FFFE
            printf("HRAM  @%x\n", location - 0xff80);
            mem_HRAM[location - 0xff80] = value;
            break;
        }
        if (location == 0xffff) { // FFFF
            printf("Interrupt enable\n");
            interrupts_enable = value;
            break;
        }
    default:
        printf("INVALID LOCATION\n");
        break;
    }
}

u16 GBC::mem_read(u16 location) {
    // MBC3

    //printf("Mem read (%x): ", location);

    switch (location & 0xf000) {
    case 0x0000: // 0000 - 3FFF
    case 0x1000:
    case 0x2000:
    case 0x3000:
        printf("CA ROM fixed\n");
        return rom[location];
    case 0x4000: // 4000 - 7FFF
    case 0x5000:
    case 0x6000:
    case 0x7000:
        printf("CA ROM switchable\n");
        break;
    case 0x8000: // 8000 - 9FFF
    case 0x9000:
        printf("VRAM\n");
        break;
    case 0xa000: // A000 - BFFF
    case 0xb000:
        printf("RAM (sw)/RTC\n");
        break;
    case 0xc000: // C000 - CFFF
        printf("W RAM B0  @%x\n", (location - 0xc000));
        return mem_WRAM0[location - 0xc000];
    case 0xd000: // D000 - DFFF
        printf("W RAM B1 (switchable)\n");
        break;
    case 0xe000: // E000 - FDFF
        printf("ECHO (0xc000 - 0xddff) B0\n");
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
            case 0xff01:
                printf("Serial link data\n");
                return io_serial_data;
            case 0xff02:
                printf("Serial link control\n");
                return io_serial_control;
            case 0xff04:
                printf("Timer Divider\n");
                return io_timer_DIV;
            case 0xff05:
                printf("Timer Timer\n");
                return io_timer_TIMA;
            case 0xff06:
                printf("Timer Modulo\n");
                return io_timer_TMA;
            case 0xff07:
                printf("Timer Control\n");
                return io_timer_TAC;
            case 0xff0f:
                printf("Int Reqs\n");
                return interrupts_request;
            case 0xff40:
                printf("LCD Control\n");
                return io_lcd_LCDC;
            case 0xff42:
                printf("Scroll Y\n");
                return io_lcd_SCY;
            case 0xff43:
                printf("Scroll X\n");
                return io_lcd_SCX;
            case 0xff44:
                //printf("LCD LY\n");
                return io_lcd_LY;
            case 0xff45:
                printf("LCD LYC\n");
                return io_lcd_LYC;
            case 0xff47:
                printf("Background palette\n");
                return io_lcd_BGP;
            case 0xff48:
                printf("Object palette 0\n");
                return io_lcd_OBP0;
            case 0xff49:
                printf("Object palette 1\n");
                return io_lcd_OBP1;
            case 0xff4a:
                printf("Window Y\n");
                return io_lcd_WY;
            case 0xff4b:
                printf("Window X\n");
                return io_lcd_WX;
            }
            
            printf("UNKNOWN\n");
        }
        if (location < 0xffff) { // FF80 - FFFE
            printf("HRAM  @%x", location - 0xff80);
            return mem_HRAM[location - 0xff80];
        }
        if (location == 0xffff) { // FFFF
            printf("Interrupt enable\n");
            return interrupts_enable;
        }
    
    default:
        printf("INVALID LOCATION\n");
    }
    return 0;
}
