#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "state.h"
#include "mmu.h"

void print_rom_header_info(u8* rom) {
    printf("Title: %s\n", &rom[0x134]);

    printf("Cart type: ");
    u8 cart_type = rom[ROMHDR_CARTTYPE];
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
    u8 cart_ROM = rom[ROMHDR_ROMSIZE];
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
    u8 cart_RAM = rom[ROMHDR_EXTRAMSIZE];
    switch(cart_RAM) {
    case 0x00: printf("None\n"); break;
    case 0x01: printf("2KB\n"); break;
    case 0x02: printf("8KB\n"); break;
    case 0x03: printf("32KB (4 banks)\n"); break;
    }
}


void read_file(char *filename, u8 **src, size_t *size) {
    FILE *fp;

    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to load file (\"%s\").\n", filename);
        #ifdef _MSC_VER
            __debugbreak();
        #endif
        exit(1);
    }

    /* Get the file size */
    fseek(fp, 0L, SEEK_END);
    long allocsize = ftell(fp);
    rewind(fp);

    *src = (u8 *)malloc(allocsize * sizeof(u8));
    if (*src == NULL) {
        printf("Error while allocating memory for reading file.");
        fclose(fp);
        return;
    }
    *size = fread(*src, sizeof(u8), allocsize, fp);
    fclose(fp);
}

struct gb_state *new_gb_state(u8 *bios, u8 *rom, size_t rom_inpsize,
        enum gb_type gb_type) {
    /* Cart features */
    int mbc = 0; /* Memory Bank Controller */
    int extram = 0;
    int battery = 0;
    int rtc = 0; /* Real time clock */
    int rom_banks = 0;
    int extram_banks = 0;
    int ram_banks = 0;
    int vram_banks = 0;

    /* Cart info from header */
    u8 cart_type = rom[ROMHDR_CARTTYPE];
    u8 rom_size = rom[ROMHDR_ROMSIZE];
    u8 extram_size = rom[ROMHDR_EXTRAMSIZE];

    struct gb_state *s = calloc(1, sizeof(struct gb_state));
    assert(s);

    assert(gb_type == GB_TYPE_GB);

    s->bios = bios;
    s->gb_type = gb_type;

    switch (cart_type) {
    case 0x00:                                              break;
    case 0x01: mbc = 1;                                     break;
    case 0x02: mbc = 1; extram = 1;                         break;
    case 0x03: mbc = 1; extram = 1; battery = 1;            break;
    case 0x04: mbc = 2;                                     break;
    case 0x06: mbc = 2;             battery = 1;            break;
    case 0x08:          extram = 1;                         break;
    case 0x09:          extram = 1; battery = 1;            break;
    case 0x0f: mbc = 3;             battery = 1; rtc = 1;   break;
    case 0x10: mbc = 3; extram = 1; battery = 1; rtc = 1;   break;
    case 0x11: mbc = 3;                                     break;
    case 0x12: mbc = 3; extram = 1;                         break;
    case 0x13: mbc = 3; extram = 1; battery = 1;            break;
    case 0x15: mbc = 4;                                     break;
    case 0x16: mbc = 4; extram = 1;                         break;
    case 0x17: mbc = 4; extram = 1; battery = 1;            break;
    case 0x19: mbc = 5;                                     break;
    case 0x1a: mbc = 5; extram = 1;                         break;
    case 0x1b: mbc = 5; extram = 1; battery = 1;            break;
    case 0x1c: mbc = 5;                                     break; /* rumble */
    case 0x1d: mbc = 5; extram = 1;                         break; /* rumble */
    case 0x1e: mbc = 5; extram = 1; battery = 1;            break; /* rumble */
    case 0x20: mbc = 6;                                     break;
    /* MMM01 unsupported */
    /* MBC7 Sensor not supported */
    /* Camera not supported */
    /* Bandai TAMA5 not supported */
    /* HuCn not supported */
    default:
        assert(!"Unsupported cartridge");
    }

    switch (rom_size) {
    case 0x00: rom_banks = 2;   break; /* 16K */
    case 0x01: rom_banks = 4;   break; /* 64K */
    case 0x02: rom_banks = 8;   break; /* 128K */
    case 0x03: rom_banks = 16;  break; /* 256K */
    case 0x04: rom_banks = 32;  break; /* 512K */
    case 0x05: rom_banks = 64;  break; /* 1M */
    case 0x06: rom_banks = 128; break; /* 2M */
    case 0x07: rom_banks = 256; break; /* 4M */
    case 0x52: rom_banks = 72;  break; /* 1.1M */
    case 0x53: rom_banks = 80;  break; /* 1.2M */
    case 0x54: rom_banks = 96;  break; /* 1.5M */
    default:
        assert(!"Unsupported ROM size");
    }

    switch (extram_size) {
    case 0x00: extram_banks = 0; break; /* None */
    case 0x01: extram_banks = 1; break; /* 2K */
    case 0x02: extram_banks = 1; break; /* 8K */
    case 0x03: extram_banks = 4; break; /* 32K */
    default:
        assert(!"Unsupported Ext RAM size");
    }

    assert((extram == 0 && extram_banks == 0) ||
           (extram == 1 && extram_banks > 0));

    /* Currently unsupported features. */
    //assert(mbc == 0 || mbc == 1);
    //assert(extram == 0);
    //assert(battery == 0);
    //assert(rtc == 0);

    if (gb_type == GB_TYPE_GB) {
        ram_banks = 2;
        vram_banks = 1;
    }

    s->mbc = mbc;
    s->has_extram = extram;
    s->has_battery = battery;
    s->has_rtc = rtc;

    s->mem_num_banks_rom = rom_banks;
    s->mem_num_banks_ram = ram_banks;
    s->mem_num_banks_extram = extram_banks;
    s->mem_num_banks_vram = vram_banks;

    s->mem_ROM = malloc(ROM_BANKSIZE * rom_banks);
    s->mem_RAM = malloc(RAM_BANKSIZE * ram_banks);
    s->mem_EXTRAM = malloc(EXTRAM_BANKSIZE * extram_banks);
    s->mem_VRAM = malloc(VRAM_BANKSIZE * vram_banks);

    memset(s->mem_ROM, 0, ROM_BANKSIZE * rom_banks);
    memcpy(s->mem_ROM, rom, rom_inpsize);

    return s;
}

void init_emu_state(struct gb_state *s) {
    s->emu_state = calloc(1, sizeof(struct emu_state));
    s->emu_state->quit = 0;
    s->emu_state->make_savestate = 0;
    s->emu_state->dbg_break_next = 0;
    s->emu_state->dbg_breakpoint = 0xffff;
}

int state_save(char *filename, struct gb_state *s) {
    FILE *fp;

    fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open state file (\"%s\").\n", filename);
        return 1;
    }

    u32 state_size = sizeof(struct gb_state);
    fwrite(&state_size, sizeof(state_size), 1, fp);
    fwrite(s, sizeof(struct gb_state), 1, fp);
    fwrite(s->mem_ROM, ROM_BANKSIZE * s->mem_num_banks_rom, 1, fp);
    fwrite(s->mem_RAM, RAM_BANKSIZE * s->mem_num_banks_ram, 1, fp);
    fwrite(s->mem_EXTRAM, EXTRAM_BANKSIZE * s->mem_num_banks_extram, 1, fp);
    fwrite(s->mem_VRAM, VRAM_BANKSIZE * s->mem_num_banks_vram, 1, fp);
    fclose(fp);
    return 0;
}

int state_load(char *filename, struct gb_state *s) {
    FILE *fp;

    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open state file (\"%s\").\n", filename);
        return 1;
    }

    u32 state_size;
    fread(&state_size, sizeof(state_size), 1, fp);
    if (state_size != sizeof(struct gb_state)) {
        fprintf(stderr, "Header mismatch for \"%s\": file statesize is %u "
                "byte, program statesize is %zu byte.\n", filename, state_size,
                sizeof(struct gb_state));
        return 1;
    }

    fread(s, sizeof(struct gb_state), 1, fp);
    s->mem_ROM = malloc(ROM_BANKSIZE * s->mem_num_banks_rom);
    s->mem_RAM = malloc(RAM_BANKSIZE * s->mem_num_banks_ram);
    s->mem_EXTRAM = malloc(EXTRAM_BANKSIZE * s->mem_num_banks_extram);
    s->mem_VRAM = malloc(VRAM_BANKSIZE * s->mem_num_banks_vram);
    fread(s->mem_ROM, ROM_BANKSIZE * s->mem_num_banks_rom, 1, fp);
    fread(s->mem_RAM, RAM_BANKSIZE * s->mem_num_banks_ram, 1, fp);
    fread(s->mem_EXTRAM, EXTRAM_BANKSIZE * s->mem_num_banks_extram, 1, fp);
    fread(s->mem_VRAM, VRAM_BANKSIZE * s->mem_num_banks_vram, 1, fp);
    fclose(fp);

    return 0;
}
