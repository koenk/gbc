#include "GBC.h"
#include <string.h>
#include <stdio.h>

#include "pause.h"

const int GB_FREQ = 4194304; // Hz
const int GB_LCD_WIDTH = 160;
const int GB_LCD_HEIGHT = 144;

const int GB_LCD_LY_MAX = 153;
const int GB_LCD_WX_MAX = 166;
const int GB_LCD_WY_MAX = 143;

const int GB_LCD_MODE_0_CLKS = 204;
const int GB_LCD_MODE_1_CLKS = 4560; 
const int GB_LCD_MODE_2_CLKS = 80;
const int GB_LCD_MODE_3_CLKS = 172;

const int cycles_per_instruction[] = { 
  // 0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
     4, 12,  8,  8,  4,  4,  8,  4, 20,  8,  8,  8,  4,  4,  8,  4, // 0
     4, 12,  8,  8,  4,  4,  8,  4, 12,  8,  8,  8,  4,  4,  8,  4, // 1
     8, 12,  8,  8,  4,  4,  8,  4,  8,  8,  8,  8,  4,  4,  8,  4, // 2
     8, 12,  8,  8, 12, 12, 12,  4,  8,  8,  8,  8,  4,  4,  8,  4, // 3
     4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4, // 4
     4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4, // 5
     4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4, // 6
     8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4,  4,  4,  8,  4, // 7
     4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4, // 8
     4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4, // 9
     4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4, // a
     4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  8,  4,  8,  4, // b
     8, 12, 12, 16, 12, 16,  8, 16,  8, 16, 12,  0, 12, 24,  8, 16, // c
     8, 12, 12,  4, 12, 16,  8, 16,  8, 16, 12,  4, 12,  4,  8, 16, // d
    12, 12,  8,  4,  4, 16,  8, 16, 16,  4, 16,  4,  4,  4,  8, 16, // e
    12, 12,  8,  4,  4, 16,  8, 16, 12,  8, 16,  4,  0,  4,  8, 16, // f
};

const int cycles_per_instruction_cb[] = {
  // 0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
     8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, // 0
     8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, // 1
     8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, // 2
     8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, // 3
     8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8, // 4
     8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8, // 5
     8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8, // 6
     8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8, // 7
     8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, // 8
     8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, // 9
     8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, // a
     8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, // b
     8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, // c
     8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, // d
     8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, // e
     8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8, // f
};


GBC::GBC(u8 *rom_data, int rom_type) {
    rom = rom_data;
    type = rom_type;
    freq = GB_FREQ;

    reset_state();
}


GBC::~GBC(void) {
}

void GBC::reset_state(void) {
    cycles = 0;

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

    lcd_mode_clks_left = 0;

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


    io_sound_enabled = 0xf1;
    io_sound_out_terminal = 0xf3;
    io_sound_terminal_control = 0x77;

    io_sound_channel1_sweep = 0x80;
    io_sound_channel1_length_pattern = 0xbf;
    io_sound_channel1_envelope = 0xf3;
    io_sound_channel1_freq_lo = 0x00;
    io_sound_channel1_freq_hi = 0xbf;

    io_sound_channel2_length_pattern = 0x3f;
    io_sound_channel2_envelope = 0x00;
    io_sound_channel2_freq_lo = 0x00;
    io_sound_channel2_freq_hi = 0xbf;

    io_sound_channel3_enabled = 0x7f;
    io_sound_channel3_length = 0xff;
    io_sound_channel3_level = 0x9f;
    io_sound_channel3_freq_lo = 0x00;
    io_sound_channel3_freq_hi = 0xbf;
    memset(io_sound_channel3_ram, 0, 0xf);

    io_sound_channel4_length = 0xff;
    io_sound_channel4_envelope = 0x00;
    io_sound_channel4_poly = 0x00;
    io_sound_channel4_consec_initial = 0xbf;
    

    mem_bank_rom = 1;
    mem_bank_ram = 0;
    mem_bank_wram = 1;

    memset(mem_RAM, 0, 4 * 0x2000);
    memset(mem_HRAM, 0, 0x7f);
    memset(mem_WRAM, 0, 8 * 0x1000);

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

    printf("ROM Size: ");
    u8 cart_ROM = rom[0x148];
    switch(cart_ROM) {
    case 0x00: printf("32KB (no banks)\n"); break;
    case 0x01: printf("64KB (4 banks)\n"); break;
    case 0x02: printf("128KB (8 banks)\n"); break;
    case 0x03: printf("256KB (16 banks)\n"); break;
    case 0x04: printf("512KB (32 banks)\n"); break;
    case 0x05: printf("1MB (64 banks)\n"); break;
    case 0x06: printf("2MB (128 banks)\n"); break;
    case 0x07: printf("4MB (256 banks)\n"); break;
    case 0x52: printf("1.1MB (72 banks)\n"); break;
    case 0x53: printf("1.2MB (80 banks)\n"); break;
    case 0x54: printf("1.5MB (96 banks)\n"); break;
    }

    printf("RAM Size: ");
    u8 cart_RAM = rom[0x149];
    switch(cart_RAM) {
    case 0x00: printf("None\n"); break;
    case 0x01: printf("2KB\n"); break;
    case 0x02: printf("8KB\n"); break;
    case 0x03: printf("32KB (4 banks)\n"); break;
    }
}

void GBC::print_regs(void) {
    printf("\n\tAF\tBC\tDE\tHL\tSP\tPC\t\tLY\n");
    printf("\t%04x\t%04x\t%04x\t%04x\t%04x\t%04x\t\t%04x\n\n", AF, BC, DE, HL, sp, pc, io_lcd_LY);
}

void GBC::handle_interrupts(void) {
    // Does NOT check for interupts enabled master.
    u8 interrupts = interrupts_enable & interrupts_request;

    printf("Executing interrupt %d.\n", interrupts);

    for (int i = 0; i < 5; i++) {
        if (interrupts & (1 << i)) {
            interrupts_master_enabled = 0;
            interrupts_request ^= 1 << i;

            mem_write(--sp, (pc & 0xff00) >> 8);
            mem_write(--sp,  pc & 0x00ff);

            pc = i * 0x8 + 0x40;

            return;
        }
    }
}

void GBC::handle_LCD(int op_cycles) {
    // The LCD goes through several states.
    // 0 = HBlank, 1 = VBlank, 2 = reading OAM, 3 = reading OAM and VRAM
    // 2 and 3 are between each HBlank
    // So the cycle goes like: 2330002330002330002330001111..1111233000...
    //                         OBBHHHOBBHHHOBBHHHOBBHHHVVVV..VVVVOBBHHH...
    // The entire cycle takes 70224 clks. (so that's about 60FPS)
    // HBlank takes about 201-207 cycles. VBlank 4560 clks.
    // 2 takes about 77-83 and 3 about 169-175 clks.

    lcd_mode_clks_left -= op_cycles;

    if (lcd_mode_clks_left < 0) {
        switch (io_lcd_STAT & 3) {
        case 0: // HBlank
            if (io_lcd_LY == 143) { // Go into VBlank (1)
                io_lcd_STAT = (io_lcd_STAT & 0xfc) | 1;
                lcd_mode_clks_left = GB_LCD_MODE_1_CLKS;
                interrupts_request |= 1 << 0;
            } else { // Back into OAM (2)
                io_lcd_STAT = (io_lcd_STAT & 0xfc) | 2;
                lcd_mode_clks_left = GB_LCD_MODE_2_CLKS;
            }
            io_lcd_LY = (io_lcd_LY + 1) % (GB_LCD_LY_MAX + 1);
            io_lcd_STAT = (io_lcd_STAT & 0xfb) | (io_lcd_LY == io_lcd_LYC);
            break;
        case 1: // VBlank, Back to OAM (2)
            io_lcd_STAT = (io_lcd_STAT & 0xfc) | 2;
            lcd_mode_clks_left = GB_LCD_MODE_2_CLKS;
            break;
        case 2: // OAM, onto OAM+VRAM (3)
            io_lcd_STAT = (io_lcd_STAT & 0xfc) | 3;
            lcd_mode_clks_left = GB_LCD_MODE_3_CLKS;
            break;
        case 3: // OAM+VRAM, let's HBlank (0)
            io_lcd_STAT = (io_lcd_STAT & 0xfc) | 0;
            lcd_mode_clks_left = GB_LCD_MODE_0_CLKS;
            break;
        }
    }
    
    if (io_lcd_STAT & (1 << 6) && io_lcd_STAT & (1 << 2)) // LY=LYC inter
        interrupts_request |= 1 << 1;

    if (io_lcd_STAT & (1 << 5) && io_lcd_STAT & 3 == 2) // Mode 2 inter
        interrupts_request |= 1 << 1;

    if (io_lcd_STAT & (1 << 4) && io_lcd_STAT & 3 == 1) // Mode 1 inter
        interrupts_request |= 1 << 1;

    if (io_lcd_STAT & (1 << 3) && io_lcd_STAT & 3 == 0) // Mode 0 inter
        interrupts_request |= 1 << 1;
}

int GBC::do_instruction(void) {
    // TODO:
    // * timer

    u8 op;
    u8 temp1, temp2;
    s8 stemp;
    int op_cycles;

    if (interrupts_master_enabled && interrupts_enable & interrupts_request) {
        handle_interrupts();
        return 0; // temp?
    }

    op = mem_read(pc++);
    op_cycles = cycles_per_instruction[op];
    if (op == 0xcb)
        op_cycles = cycles_per_instruction_cb[mem_read(pc + 1)];

    handle_LCD(op_cycles);
    cycles += op_cycles;

    switch (op) {
    case 0x00: // NOP
        break;
    case 0x01: // LD BC, nnnn
        temp1 = mem_read(pc++);
        temp2 = mem_read(pc++);
        BC = temp1 | (temp2 << 8);
        break;
    case 0x05: // DEC B
        B(B() - 1);
        F((B() == 0 ? FLAG_Z : 0) |
            FLAG_N |
            (B() & 0xf == 0xf ? FLAG_H : 0) |
            F() & FLAG_C);

        break;
    case 0x06: // LD B, nn
        temp1 = mem_read(pc++);
        B(temp1);
        break;
    case 0x0b: // DEC BC
        BC--;
        break;
    case 0x0c: // INC C
        C(C() + 1);
        F((C() == 0 ? FLAG_Z : 0) |
            0 |
            (C() & 0xf == 0xf ? FLAG_H : 0) |
            F() & FLAG_C);
        break;
    case 0x0e: // LD C, nn
        temp1 = mem_read(pc++);
        C(temp1);
        break;
    case 0x11: // LD DE, nnnn
        temp1 = mem_read(pc++);
        temp2 = mem_read(pc++);
        DE = temp1 | (temp2 << 8);
        break;
    case 0x15: // DEC D
        D(D() - 1);
        F((D() == 0 ? FLAG_Z : 0) |
            FLAG_N |
            (D() & 0xf == 0xf ? FLAG_H : 0) |
            F() & FLAG_C);
        break;
    case 0x16: // LD D, nn
        temp1 = mem_read(pc++);
        D(temp1);
        break;
    case 0x18: // JR nn (offset)
        stemp = mem_read(pc++);
        pc += stemp;
        break;
    case 0x1d: // DEC E
        E(E() - 1);
        F((E() == 0 ? FLAG_Z : 0) |
            FLAG_N |
            (E() & 0xf == 0xf ? FLAG_H : 0) |
            F() & FLAG_C);
        break;
    case 0x20: // JR NZ, nn (offset)
        if (!(AF & FLAG_Z)) {
            stemp = mem_read(pc++);
            pc += stemp;
        } else {
            pc++;
        }
        break;
    case 0x21: // LD HL, nnnn
        temp1 = mem_read(pc++);
        temp2 = mem_read(pc++);
        HL = temp1 | (temp2 << 8);
        break;
    case 0x22: // LDI (HL), A
        mem_write(HL, A());
        break;
    case 0x23: // INC HL
        HL++;
        break;
    case 0x26: // LD H, nn
        temp1 = mem_read(pc++);
        H(temp1);
        break;
    case 0x28: // JR Z, nn (offset)
        if (AF & FLAG_Z) {
            stemp = mem_read(pc++);
            pc += stemp;
        } else {
            pc++;
        }
        break;
    case 0x2a: // LDI A, (HL)
        A(mem_read(HL));
        break;
    case 0x31: // LD sp, nnnn
        temp1 = mem_read(pc++);
        temp2 = mem_read(pc++);
        sp = temp1 | (temp2 << 8);
        break;
    case 0x36: // LD (HL), nn
        temp1 = mem_read(pc++);
        mem_write(HL, temp1);
        break;
    case 0x3e: // LD a, nn
        temp1 = mem_read(pc++);
        A(temp1);
        break;
    case 0x3d: // DEC A
        A(A() - 1);
        F((A() == 0 ? FLAG_Z : 0) |
            FLAG_N |
            (A() & 0xf == 0xf ? FLAG_H : 0) |
            F() & FLAG_C);
        break;
    case 0x42: // LD B, D
        B(D());
        break;
    case 0x47: // LD B, A
        B(A());
        break;
    case 0x57: // LD D, A
        D(A());
        break;
    case 0x6b: // LD L, E
        L(E());
        break;
    case 0x78: // LD A, B
        A(B());
        break;
    case 0x7a: // LD A, D
        A(D());
        break;
    case 0xa7: // AND A
        F((A() == 0 ? FLAG_Z : 0) |
            0 |
            FLAG_H |
            0);
        break;
    case 0xaf: // XOR a (so clear a)
        A(0);
        break;
    case 0xb1: // OR C
        A(A() | C());
        F(A() == 0 ? FLAG_Z : 0);
        break;
    case 0xc1: // POP BC
        temp1 = mem_read(sp++);
        temp2 = mem_read(sp++);

        BC = temp1 | (temp2 << 8);
        break;
    case 0xc3: // JP nnnn
        temp1 = mem_read(pc++);
        temp2 = mem_read(pc++);
        pc = temp1 | (temp2 << 8);
        break;
    case 0xc5: // PUSH BC
        mem_write(--sp, (BC & 0xff00) >> 8);
        mem_write(--sp,  BC & 0x00ff);
        break;
    case 0xc8: // RET Z
        if (F() & FLAG_Z) {
            temp1 = mem_read(sp++);
            temp2 = mem_read(sp++);
            pc = temp1 | (temp2 << 8);
        }
        break;
    case 0xc9: // RET
        temp1 = mem_read(sp++);
        temp2 = mem_read(sp++);
        pc = temp1 | (temp2 << 8);
        break;
    case 0xca: // JP Z, nnnn
        if (AF & FLAG_Z) {
            temp1 = mem_read(pc++);
            temp2 = mem_read(pc++);
            pc = temp1 | (temp2 << 8);
        } else {
            pc += 2;
        }
        break;
    case 0xcb: // EXTENDED INSTRUCTION
        op = mem_read(pc++);
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
        temp1 = mem_read(pc++);
        temp2 = mem_read(pc++);

        mem_write(--sp, (pc & 0xff00) >> 8);
        mem_write(--sp,  pc & 0x00ff);

        pc = temp1 | (temp2 << 8);
        break;
    case 0xd1: // POP DE
        temp1 = mem_read(sp++);
        temp2 = mem_read(sp++);

        DE = temp1 | (temp2 << 8);
        break;
    case 0xd5: // PUSH DE
        mem_write(--sp, (DE & 0xff00) >> 8);
        mem_write(--sp,  DE & 0x00ff);
        break;
    case 0xe0: // LD (ffnn), A
        temp1 = mem_read(pc++);
        mem_write(0xff00 | temp1, A());
        break;
    case 0xe1: // POP HL
        temp1 = mem_read(sp++);
        temp2 = mem_read(sp++);

        HL = temp1 | (temp2 << 8);
        break;
    case 0xe2: // LD 0xff00+C, A
        mem_write(0xff00 + C(), A());
        break;
    case 0xe5: // PUSH HL
        mem_write(--sp, (HL & 0xff00) >> 8);
        mem_write(--sp,  HL & 0x00ff);
        break;
    case 0xe6: // AND nn
        temp1 = mem_read(pc++);
        A(A() & temp1);
        F((A() == 0) ? FLAG_Z : 0 |
          0 |
          FLAG_H | 
          0);
        break;
    case 0xea: // LD (nnnn), A
        temp1 = mem_read(pc++);
        temp2 = mem_read(pc++);
        mem_write(temp1 | (temp2 << 8), A());
        break;
    case 0xf0: // LD A, (ffnn)
        temp1 = mem_read(pc++);
        A(mem_read(0xff00 | temp1));
        break;
    case 0xf3: // DI
        interrupts_master_enabled = 0;
        break;
    case 0xf5: // PUSH AF
        mem_write(--sp, (AF & 0xff00) >> 8);
        mem_write(--sp,  AF & 0x00ff);
        break;
    case 0xfa: // LD A, (nnnn)
        temp1 = mem_read(pc++);
        temp2 = mem_read(pc++);
        A(mem_read(temp1 | (temp2 << 8)));
        break;
    case 0xfb: // EI
        interrupts_master_enabled = true;
        break;
    case 0xfe: // CP nn
        temp1 = mem_read(pc++);
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


void GBC::mem_write(u16 location, u8 value) {
    // MBC3

    printf("Mem write (%x) %x: ", location, value);
    switch (location & 0xf000) {
    case 0x0000: // 0000 - 1FFF
    case 0x1000:
        printf("RAM+Timer enable\n");
        pause();
        break;
    case 0x2000: // 2000 - 3FFF
    case 0x3000:
        printf("ROM bank number\n");
        mem_bank_rom = value;
        break;
    case 0x4000: // 4000 - 5FFF
    case 0x5000:
        printf("RAM bank number -OR- RTC register select\n");
        pause();
        break;
    case 0x6000: // 6000 - 7FFF
    case 0x7000:
        printf("Latch clock data\n");
        pause();
        break;
    case 0x8000: // 8000 - 9FFF
    case 0x9000:
        printf("VRAM\n");
        break;
    case 0xa000: // A000 - BFFF
    case 0xb000:
        printf("RAM (sw)/RTC\n");
        pause();
        break;
    case 0xc000: // C000 - CFFF
        printf("WRAM B0  @%x\n", (location - 0xc000));
        mem_WRAM[0][location - 0xc000] = value;
        break;
    case 0xd000: // D000 - DFFF
        printf("WRAM B1-7 (switchable) @%x, bank %d\n", location - 0xd000, mem_bank_wram);
        mem_WRAM[mem_bank_wram][location - 0xd000] = value;
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
            case 0xff10:
                printf("Sound channel 1 sweep\n");
                io_sound_channel1_sweep = value;
                break;
            case 0xff11:
                printf("Sound channel 1 length/pattern\n");
                io_sound_channel1_length_pattern = value;
                break;
            case 0xff12:
                printf("Sound channel 1 envelope\n");
                io_sound_channel1_envelope = value;
                break;
            case 0xff13:
                printf("Sound channel 1 freq lo\n");
                io_sound_channel1_freq_lo = value;
                break;
            case 0xff14:
                printf("Sound channel 1 freq hi\n");
                io_sound_channel1_freq_hi = value;
                break;
            case 0xff16:
                printf("Sound channel 2 length/pattern\n");
                io_sound_channel2_length_pattern = value;
                break;
            case 0xff17:
                printf("Sound channel 2 envelope\n");
                io_sound_channel2_envelope = value;
                break;
            case 0xff18:
                printf("Sound channel 2 freq lo\n");
                io_sound_channel2_freq_lo = value;
                break;
            case 0xff19:
                printf("Sound channel 2 freq hi\n");
                io_sound_channel2_freq_hi = value;
                break;
            case 0xff1a:
                printf("Sound channel 3 enabled\n");
                io_sound_channel3_enabled = value;
                break;
            case 0xff1b:
                printf("Sound channel 3 length\n");
                io_sound_channel3_length = value;
                break;
            case 0xff1c:
                printf("Sound channel 3 level\n");
                io_sound_channel3_level = value;
                break;
            case 0xff1d:
                printf("Sound channel 3 freq lo\n");
                io_sound_channel3_freq_lo = value;
                break;
            case 0xff1e:
                printf("Sound channel 3 freq hi\n");
                io_sound_channel3_freq_hi = value;
                break;
            case 0xff20:
                printf("Sound channel 4 length\n");
                io_sound_channel4_length = value;
                break;
            case 0xff21:
                printf("Sound channel 4 envelope\n");
                io_sound_channel4_envelope = value;
                break;
            case 0xff22:
                printf("Sound channel 4 polynomial counter\n");
                io_sound_channel4_poly = value;
                break;
            case 0xff23:
                printf("Sound channel 4 Counter/consecutive; Inital\n");
                io_sound_channel4_consec_initial = value;
                break;
            case 0xff24:
                printf("Sound channel control\n");
                io_sound_terminal_control = value;
                break;
            case 0xff25:
                printf("Sound output terminal\n");
                io_sound_out_terminal = value;
                break;
            case 0xff26:
                printf("Sound enabled flags\n");
                io_sound_enabled = value;
                break;
            case 0xff40:
                printf("LCD Control\n");
                io_lcd_LCDC = value;
                break;
            case 0xff41:
                printf("LCD Stat\n");
                io_lcd_STAT = value;
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
                io_lcd_LY = 0x0;
                io_lcd_STAT = (io_lcd_STAT & 0xfb) | (io_lcd_LY == io_lcd_LYC);
                break;
            case 0xff45:
                printf("LCD LYC\n");
                io_lcd_LYC = value;
                io_lcd_STAT = (io_lcd_STAT & 0xfb) | (io_lcd_LY == io_lcd_LYC);
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
                pause();
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
        pause();
        break;
    }
}

u8 GBC::mem_read(u16 location) {
    // MBC3

    //printf("Mem read (%x): ", location);

    switch (location & 0xf000) {
    case 0x0000: // 0000 - 3FFF
    case 0x1000:
    case 0x2000:
    case 0x3000:
        //printf("CA ROM fixed\n");
        return rom[location];
    case 0x4000: // 4000 - 7FFF
    case 0x5000:
    case 0x6000:
    case 0x7000:
        printf("CA ROM switchable, bank %d, %4x\n", mem_bank_rom, mem_bank_rom * 0x4000 + (location - 0x4000));
        return rom[mem_bank_rom * 0x4000 + (location - 0x4000)];
        break;
    case 0x8000: // 8000 - 9FFF
    case 0x9000:
        printf("VRAM\n");
        break;
    case 0xa000: // A000 - BFFF
    case 0xb000:
        printf("RAM (sw)/RTC\n");
        pause();
        break;
    case 0xc000: // C000 - CFFF
        printf("WRAM B0  @%x\n", (location - 0xc000));
        return mem_WRAM[0][location - 0xc000];
    case 0xd000: // D000 - DFFF
        printf("WRAM B1-7 (switchable) @%x, bank %d\n", location - 0xd000, mem_bank_wram);
        return mem_WRAM[mem_bank_wram][location - 0xd000];
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
            case 0xff10:
                printf("Sound channel 1 sweep\n");
                return io_sound_channel1_sweep;
            case 0xff12:
                printf("Sound channel 1 envelope\n");
                return io_sound_channel1_envelope;
            case 0xff1a:
                printf("Sound channel 3 enabled\n");
                return io_sound_channel3_enabled;
            case 0xff1c:
                printf("Sound channel 3 level\n");
                return io_sound_channel3_level;
            case 0xff25:
                printf("Sound output terminal\n");
                return io_sound_out_terminal;
            case 0xff26:
                printf("Sound enabled flags\n");
                return io_sound_enabled;
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
            pause();
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
        pause();
    }
    return 0;
}
