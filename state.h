#ifndef STATE_H
#define STATE_H

#include "types.h"

void print_rom_header_info(u8* rom);
int state_new_from_rom(struct gb_state *s, u8 *rom, size_t rom_size,
    enum gb_type rom_gb_type);
void init_emu_state(struct gb_state *s);
int state_save(struct gb_state *s, u8 **ret_state_buf,
        size_t *ret_state_size);
int state_load(struct gb_state *s, u8 *state_buf, size_t state_buf_size);

#endif
