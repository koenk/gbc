#include <assert.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include "gui.h"

static SDL_Renderer *renderer;
static SDL_Texture *texture;

static u8 *pixbuf;

int gui_init(void) {
    SDL_Window *window;

    if (SDL_Init(SDL_INIT_VIDEO)) {
        printf("LCD: SDL failed to initialize: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow(GUI_WINDOW_TITLE,
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            GUI_PX_WIDTH * GUI_ZOOM, GUI_PX_HEIGHT * GUI_ZOOM, 0);
    if (!window){
        printf("LCD: SDL could not create window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!window){
        printf("LCD: SDL could not create renderer: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, GUI_PX_WIDTH, GUI_PX_HEIGHT);
    if (!texture){
        printf("LCD: SDL could not create screen texture: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    pixbuf = malloc(GUI_PX_WIDTH * GUI_PX_HEIGHT);
    if (!pixbuf) {
        printf("LCD: could not allocate pixel buffer\n");
        SDL_Quit();
        return 1;
    }
    memset(pixbuf, 0, GUI_PX_WIDTH * GUI_PX_HEIGHT);

    return 0;
}

struct __attribute__((__packed__)) OAMentry {
    u8 y;
    u8 x;
    u8 tile;
    u8 flags;
};

u8 palette_get(u8 palette, u8 col) {
    return (palette >> (col << 1)) & 0x3;
}

void gui_render_current_line(struct gb_state *gb_state) {
    /*
     * Tile Data @ 8000-8FFF or 8800-97FF defines the pixels per Tile, which can
     * be used for the BG, window or sprite/object. 192 tiles max, 8x8px, 4
     * colors. Foreground tiles (sprites/objects) may only have 3 colors (0
     * being transparent). Each tile thus is 16 byte, 2 byte per line (2 bit per
     * px), first byte is lsb of color, second byte msb of color.
     *
     *
     * BG Map @ 9800-9BFF or 9C00-9FFF. 32 rows of 32 bytes each, each byte is
     * number of tile to be displayed (index into Tile Data, see below
     *
     * Window works similarly to BG
     *
     * Sprites or objects come from the Sprite Attribute table (OAM: Object
     * Attribute Memory) @ FE00-FE9F, 40 entries of 4 byte (max 10 per hline).
     *  byte 0: Y pos - 16
     *  byte 1: X pos - 8
     *  byte 2: Tile number, index into Tile data (see above)
     *
     */

    /* TODO for all things: palette? */

    int y = gb_state->io_lcd_LY;

    if (y >= GUI_PX_HEIGHT) /* VBlank */
        return;

    /* First render background tiles. */
    u8 bg_scroll_x = gb_state->io_lcd_SCX;
    u8 bg_scroll_y = gb_state->io_lcd_SCY;

    u8 bgwin_tilemap_low = (gb_state->io_lcd_LCDC & (1<<4)) ? 1 : 0;
    u8 bgwin_tilemap_unsigned = bgwin_tilemap_low;
    u8 bgmap_high = (gb_state->io_lcd_LCDC & (1<<3)) ? 1 : 0;
    u8 obj_8x16 = (gb_state->io_lcd_LCDC & (1<<2)) ? 1 : 0;
    u8 obj_enable = (gb_state->io_lcd_LCDC & (1<<1)) ? 1 : 0;

    u16 bgwin_tilemap_addr = bgwin_tilemap_low ? 0x8000 : 0x9000;
    u16 bgmap_addr = bgmap_high ? 0x9c00 : 0x9800;

    u8 *bgwin_tiledata = &gb_state->mem_VRAM[bgwin_tilemap_addr - 0x8000];
    u8 *obj_tiledata = &gb_state->mem_VRAM[0x8000 - 0x8000];
    u8 *bgmap = &gb_state->mem_VRAM[bgmap_addr - 0x8000];

    u8 bg_palette = gb_state->io_lcd_BGP;
    u8 obj_palette1 = gb_state->io_lcd_OBP0;
    u8 obj_palette2 = gb_state->io_lcd_OBP1;

    u8 obj_tile_height = obj_8x16 ? 16 : 8;

    /* OAM scan - gather (max 10) objects on this line in cache */
    struct OAMentry *OAM = (struct OAMentry*)&gb_state->mem_OAM[0];
    struct OAMentry objs[10];
    int num_objs = 0;
    if (obj_enable)
        for (int i = 0; i < 40; i++)
            if (y >= OAM[i].y - 16 && y < OAM[i].y - 16 + obj_tile_height)
                objs[num_objs++] = OAM[i];


    /* Draw all background pixels of this line. */
    for (int x = 0; x < GUI_PX_WIDTH; x++) {
        int bg_x = (x + bg_scroll_x) % 256,
            bg_y = (y + bg_scroll_y) % 256;
        int bg_tile_x = bg_x / 8,
            bg_tile_y = bg_y / 8;
        int bg_tileoff_x = bg_x % 8,
            bg_tileoff_y = bg_y % 8;

        u8 bg_tile_idx_raw = bgmap[bg_tile_x + bg_tile_y * 32];
        s16 bg_tile_idx = bgwin_tilemap_unsigned ? (s16)(u16)bg_tile_idx_raw :
                                                   (s16)(s8)bg_tile_idx_raw;

        /* We have packed 2-bit color indices here, so the bits look like:
         * (each bit denoted by the pixel index in tile)
         * 01234567 01234567 89abcdef 89abcdef ...
         * So for the 9th pixel (which is px 1,1) we need bytes 2+3 (9/8*2 [+1])
         * and then shift both by 7 (8-9%8).
         */
        int bg_tileoff = bg_tileoff_x + bg_tileoff_y * 8;
        int shift = 7 - bg_tileoff % 8;
        u8 b1 = bgwin_tiledata[bg_tile_idx * 16 + bg_tileoff/8*2];
        u8 b2 = bgwin_tiledata[bg_tile_idx * 16 + bg_tileoff/8*2 + 1];
        u8 colidx = ((b1 >> shift) & 1) |
                   (((b2 >> shift) & 1) << 1);

        u8 col = palette_get(bg_palette, colidx);
        pixbuf[x + y * GUI_PX_WIDTH] = colidx;
    }

    /* Draw any sprites (objects) on this line. */
    for (int x = 0; x < GUI_PX_WIDTH; x++) {
        for (int i = 0; i < num_objs; i++) {
            int obj_tileoff_x = x - (objs[i].x - 8),
                obj_tileoff_y = y - (objs[i].y - 16);

            if (obj_tileoff_x < 0 || obj_tileoff_x >= obj_tile_height)
                continue;

            /* TODO: flip, prio */

            int obj_tileoff = obj_tileoff_x + obj_tileoff_y * 8;
            int shift = 7 - obj_tileoff % 8;
            u8 b1 = obj_tiledata[objs[i].tile * 16 + obj_tileoff/8*2];
            u8 b2 = obj_tiledata[objs[i].tile * 16 + obj_tileoff/8*2 + 1];
            u8 colidx = ((b1 >> shift) & 1) |
                       (((b2 >> shift) & 1) << 1);

            if (colidx != 0) {
                u8 pal = objs[i].flags & (1<<4) ? obj_palette2 : obj_palette1;
                u8 col = palette_get(pal, colidx);
                pixbuf[x + y * GUI_PX_WIDTH] = col;
            }
        }
    }
}

void gui_render_frame(struct gb_state *gb_state) {
    u32 *pixels = NULL;
    int pitch;
    if (SDL_LockTexture(texture, NULL, (void*)&pixels, &pitch)) {
        printf("LCD: SDL could not lock screen texture: %s\n", SDL_GetError());
        exit(1);
    }

    /* The colors stored in pixbuf already went through the palette translation
     * when rendering each line. */
    u32 palette[] = { 0xffffffff, 0xaaaaaaaa, 0x66666666, 0x11111111 };
    for (int y = 0; y < GUI_PX_HEIGHT; y++)
        for (int x = 0; x < GUI_PX_WIDTH; x++) {
            int idx = x + y * GUI_PX_WIDTH;
            pixels[idx] = palette[pixbuf[idx]];
        }

    SDL_UnlockTexture(texture);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_Delay(1000/60);
}

int gui_handleinputs(struct gb_state *gb_state) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
            case SDLK_q:
                return 1;

            case SDLK_b:
                gb_state->dbg_break_next = 1;
                break;

            case SDLK_RETURN:    gb_state->io_buttons_buttons &= ~(1<<3); break;
            case SDLK_BACKSPACE: gb_state->io_buttons_buttons &= ~(1<<2); break;
            case SDLK_x:         gb_state->io_buttons_buttons &= ~(1<<1); break;
            case SDLK_z:         gb_state->io_buttons_buttons &= ~(1<<0); break;
            case SDLK_DOWN:      gb_state->io_buttons_dirs    &= ~(1<<3); break;
            case SDLK_UP:        gb_state->io_buttons_dirs    &= ~(1<<2); break;
            case SDLK_LEFT:      gb_state->io_buttons_dirs    &= ~(1<<1); break;
            case SDLK_RIGHT:     gb_state->io_buttons_dirs    &= ~(1<<0); break;
            }
            break;

        case SDL_KEYUP:
            switch (event.key.keysym.sym) {
            case SDLK_RETURN:    gb_state->io_buttons_buttons |= 1<<3; break;
            case SDLK_BACKSPACE: gb_state->io_buttons_buttons |= 1<<2; break;
            case SDLK_x:         gb_state->io_buttons_buttons |= 1<<1; break;
            case SDLK_z:         gb_state->io_buttons_buttons |= 1<<0; break;
            case SDLK_DOWN:      gb_state->io_buttons_dirs    |= 1<<3; break;
            case SDLK_UP:        gb_state->io_buttons_dirs    |= 1<<2; break;
            case SDLK_LEFT:      gb_state->io_buttons_dirs    |= 1<<1; break;
            case SDLK_RIGHT:     gb_state->io_buttons_dirs    |= 1<<0; break;
            }
            break;

        case SDL_QUIT:
            return 1;
        }
    }
    return 0;
}
