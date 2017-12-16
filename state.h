#ifndef STATE_H
#define STATE_H

#include "types.h"

void print_rom_header_info(u8* rom);
void read_file(char *filename, u8 **src, size_t *size);
struct gb_state *new_gb_state(u8 *bios, u8 *rom, size_t rom_inpsize,
        enum gb_type gb_type);
void init_emu_state(struct gb_state *s);
int state_save(char *filename, struct gb_state *s);
int state_load(char *filename, struct gb_state *s);

#endif
