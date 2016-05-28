#ifndef MMU_H
#define MMU_H

#include "types.h"

u8 mmu_read(struct gb_state *s, u16 location);
void mmu_write(struct gb_state *s, u16 location, u8 value);

#endif
