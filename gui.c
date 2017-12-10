#include <assert.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include "gui.h"

static SDL_Renderer *renderer;
static SDL_Texture *texture;

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

    return 0;
}

void gui_render_frame(struct gb_state *gb_state) {
    u32 *pixels = NULL;
    int pitch;
    if (SDL_LockTexture(texture, NULL, (void*)&pixels, &pitch)) {
        printf("LCD: SDL could not lock screen texture: %s\n", SDL_GetError());
        exit(1);
    }

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
    for (int y = 0; y < GUI_PX_HEIGHT; y++)
        SDL_memset(&pixels[y * GUI_PX_WIDTH], y, GUI_PX_WIDTH * 4);

    u32 palette[] = { 0xffffffff, 0xaaaaaaaa, 0x66666666, 0x11111111 };

    /* TODO for all things: palette? */

    /* First render background tiles. */
    /* TODO: scroll, selecting other tile data area */
    u8 *tiledata = &gb_state->mem_VRAM[0x8000-0x8000];
    u8 *bgmap = &gb_state->mem_VRAM[0x9800-0x8000];
    for (int tiley = 0; tiley < 32; tiley++) {
        if (tiley * 8 >= GUI_PX_HEIGHT)
            break;
        for (int tilex = 0; tilex < 32; tilex++) {
            if (tilex * 8 >= GUI_PX_WIDTH)
                break;
            int tilebasepx = tilex * 8 + tiley * 8 * GUI_PX_WIDTH;
            u8 tileidx = bgmap[tilex + tiley*32];
            for (int y = 0; y < 8; y++) {
                if (tiley * 8 + y >= GUI_PX_HEIGHT)
                    break;
                for (int x = 0; x < 8; x++) {
                    if (tilex * 8 + x >= GUI_PX_WIDTH)
                        break;

                    /* We have packed 2-bit color indices here, so the bits look
                     * like: (each bit denoted by the pixel index in tile)
                     *  01234567 01234567 89abcdef 89abcdef ...
                     * So for the 9th pixel (which is px 1,1) we need bytes 2+3
                     * (9/8*2 [+1]) and then shift both by 7 (8-9%8)
                     */
                    int i = x + y * 8;
                    int shift = 8 - i % 8;
                    u8 b1 = tiledata[tileidx * 16 + i/8*2];
                    u8 b2 = tiledata[tileidx * 16 + i/8*2 + 1];
                    u8 colidx = ((b1 >> shift) & 1) |
                               (((b2 >> shift) & 1) << 1);

                    u32 col = palette[colidx];
                    pixels[tilebasepx + x + y * GUI_PX_WIDTH] = col;

                }
            }
        }
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
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    return 1;
                break;
            case SDL_QUIT:
                return 1;
        }
    }
    return 0;
}
