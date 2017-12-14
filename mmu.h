#ifndef MMU_H
#define MMU_H

#include "types.h"

/*
 * Memory layout of the GameBoy:
 *  $0000-$3FFF	ROM (bank 0)
 *  $4000-$7FFF	ROM (bank 1..n, switchable)
 *  $8000-$9FFF	VRAM (bank 0 non-CGB, bank 0-1 on CGB)
 *  $A000-$BFFF	External RAM (cartridge, optional)
 *  $C000-$CFFF	Internal RAM (bank 0)
 *  $D000-$DFFF	Internal RAM (bank 1-7, switchable, CGB only)
 *  $E000-$FDFF	Echo RAM (reserved)
 *  $FE00-$FE9F	OAM - Object Attribute Memory
 *  $FEA0-$FEFF	Unusable
 *  $FF00-$FF7F	Hardware I/O Registers
 *  $FF80-$FFFE	Zero
 *  $FFFF	    Interrupt Enable Flag
 */

#define ROMHDR_CARTTYPE     0x147
#define ROMHDR_ROMSIZE      0x148
#define ROMHDR_EXTRAMSIZE   0x149

#define ROM_BANKSIZE        0x4000 /* 16K */
#define RAM_BANKSIZE        0x1000 /* 4K */
#define VRAM_BANKSIZE       0x2000 /* 8K */
#define EXTRAM_BANKSIZE     0x2000 /* 8K */

#define OAM_SIZE            0xa0


u8 mmu_read(struct gb_state *s, u16 location);
void mmu_write(struct gb_state *s, u16 location, u8 value);

u16 mmu_read16(struct gb_state *s, u16 location);
void mmu_write16(struct gb_state *s, u16 location, u16 value);
u16 mmu_pop16(struct gb_state *s);
void mmu_push16(struct gb_state *s, u16 value);

#endif
