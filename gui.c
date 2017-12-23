#include <assert.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include "gui.h"
#include "hwdefs.h"

static SDL_Renderer *renderer;
static SDL_Texture *texture;
static SDL_AudioDeviceID audio_dev;

static u16 *pixbuf;
static u8 *sndbuf;

static const int SAMPLE_RATE = 44100;
static const int CHANNELS = 2;
static const int BUFSIZE = 1024;

/* Called by SDL when it needs more samples. */
void audio_callback(void *userdata, u8 *stream, int len) {
    (void)userdata;
    assert(len == BUFSIZE*CHANNELS);
    memcpy(stream, sndbuf, len);
}

int audio_init(struct gb_state *s) {
    SDL_AudioSpec want, have;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        printf("SDL: failed to initialize sound: %s\n", SDL_GetError());
        return 1;
    }

    SDL_memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_U8;
    want.channels = 2;
    want.samples = BUFSIZE*CHANNELS;
    want.callback = audio_callback;
    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!audio_dev) {
        printf("SDL: failed to open sound device: %s\n", SDL_GetError());
        return 1;
    }
    sndbuf = malloc(BUFSIZE*CHANNELS);
    memset(sndbuf, 0, BUFSIZE*CHANNELS);

    audio_update(s);
    SDL_PauseAudioDevice(audio_dev, 0);
    return 0;
}

void audio_update(struct gb_state *s) {
    const double sample_freq = 1. * SAMPLE_RATE / BUFSIZE;

    if (s->io_sound_channel2_freq_hi & (1<<7)) {
        s->io_sound_channel2_freq_hi &= ~(1<<7);
        s->io_sound_enabled |= (1<<1);
    }

    u8 snd_enable = s->io_sound_enabled & (1<<7) ? 1 : 0;
    u8 ch4_enable = s->io_sound_enabled & (1<<3) ? 1 : 0;
    u8 ch3_enable = s->io_sound_enabled & (1<<2) ? 1 : 0;
    u8 ch2_enable = s->io_sound_enabled & (1<<1) ? 1 : 0;
    u8 ch1_enable = s->io_sound_enabled & (1<<0) ? 1 : 0;

    //printf("SND %d (%d %d %d %d)\n", snd_enable, ch1_enable, ch2_enable, ch3_enable, ch4_enable);

    if (!snd_enable)
        return;

    if (!ch1_enable && !ch2_enable && !ch3_enable && !ch4_enable)
        return;

    memset(sndbuf, 0, BUFSIZE*CHANNELS);

    if (ch2_enable) {
        static int env_step_cur = 0;
        static int env_step_start = 0;
        static int env_step_cyc_left = 0;
        static int env_running = 0;
        static int env_vol = 0;
        u8 ch2_len = s->io_sound_channel2_length_pattern & 0x3f;
        u8 ch2_duty = s->io_sound_channel2_length_pattern >> 6;
        u8 ch2_use_len = s->io_sound_channel2_freq_hi & (1<<6) ? 1 : 0;
        u8 ch2_vol = s->io_sound_channel2_envelope >> 4;
        u8 ch2_env_step = s->io_sound_channel2_envelope & 0x7;
        u8 ch2_env_inc = s->io_sound_channel2_envelope & (1<<3) ? 1 : 0;
        u16 ch2_freq_reg = s->io_sound_channel2_freq_lo |
                            ((s->io_sound_channel2_freq_hi & 7) << 8);
        u32 freq = 131072/(2048-ch2_freq_reg);
        u32 oscs_in_buf = freq / sample_freq;
        u32 osc_len = BUFSIZE / oscs_in_buf;
        u32 osc_high = osc_len * GB_SND_DUTY_PERC[ch2_duty];

        if (ch2_env_step && (!env_running || env_step_start != ch2_env_step)) {
            env_running = 1;
            env_step_cur = ch2_env_step;
            env_step_start = ch2_env_step;
            env_step_cyc_left = GB_SND_ENVSTEP_CYC * ch2_env_step;
            env_vol = ch2_vol;
        } else if (env_running) {
            /* TODO assumes we do this ones per frame */
            env_step_cyc_left -= GB_FREQ/60.;
            if (env_step_cyc_left <= 0) {
                env_step_cur--;
                env_vol += ch2_env_inc ? +1 : -1;
                env_vol &= 0xf;
                if (env_step_cur == 0) {
                    env_running = 0;
                } else {
                    env_step_cyc_left = GB_SND_ENVSTEP_CYC * env_step_cur;
                }
            }
        }

        u8 vol = env_running ? env_vol : ch2_vol;
        vol = 255 * vol / 16; /* Normalize to 0-255 */

        /* TODO: envelope, length, restart */
        if (vol) {
            printf("CH3 duty=%x (%u%%) freq=%u (%u) vol=%u sw=%u ul=%d\n",
                    ch2_duty, (int)(GB_SND_DUTY_PERC[ch2_duty]*100), freq,
                    ch2_freq_reg, vol, ch2_env_step, ch2_use_len);
            //printf(" oscs in buf: %u, len=%u samples, high=%u samples\n", oscs_in_buf, osc_len, osc_high);
            for (u32 i = 0; i < oscs_in_buf; i++)
                memset(&sndbuf[(i * osc_len)*2], vol, osc_high*2);
        }
    }
}

int gui_init(void) {
    SDL_Window *window;

    if (SDL_Init(SDL_INIT_VIDEO)) {
        printf("LCD: SDL failed to initialize: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow(GUI_WINDOW_TITLE,
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            GB_LCD_WIDTH * GUI_ZOOM, GB_LCD_HEIGHT * GUI_ZOOM, 0);
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

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, GB_LCD_WIDTH, GB_LCD_HEIGHT);
    if (!texture){
        printf("LCD: SDL could not create screen texture: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    pixbuf = malloc(GB_LCD_WIDTH * GB_LCD_HEIGHT * sizeof(u16));
    if (!pixbuf) {
        printf("LCD: could not allocate pixel buffer\n");
        SDL_Quit();
        return 1;
    }
    memset(pixbuf, 0, GB_LCD_WIDTH * GB_LCD_HEIGHT * sizeof(u16));

    return 0;
}

struct __attribute__((__packed__)) OAMentry {
    u8 y;
    u8 x;
    u8 tile;
    u8 flags;
};

u16 palette_get_col(u8 *palettedata, u8 palidx, u8 colidx) {
    u8 idx = palidx * 8 + colidx * 2;
    return palettedata[idx] | (palettedata[idx + 1] << 8);
}

u8 palette_get_gray(u8 palette, u8 colidx) {
    return (palette >> (colidx << 1)) & 0x3;
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

    int y = gb_state->io_lcd_LY;

    if (y >= GB_LCD_HEIGHT) /* VBlank */
        return;

    u8 use_col = gb_state->gb_type == GB_TYPE_CGB;

    u8 winmap_high       = (gb_state->io_lcd_LCDC & (1<<6)) ? 1 : 0;
    u8 win_enable        = (gb_state->io_lcd_LCDC & (1<<5)) ? 1 : 0;
    u8 bgwin_tilemap_low = (gb_state->io_lcd_LCDC & (1<<4)) ? 1 : 0;
    u8 bgmap_high        = (gb_state->io_lcd_LCDC & (1<<3)) ? 1 : 0;
    u8 obj_8x16          = (gb_state->io_lcd_LCDC & (1<<2)) ? 1 : 0;
    u8 obj_enable        = (gb_state->io_lcd_LCDC & (1<<1)) ? 1 : 0;
    u8 bg_enable         = (gb_state->io_lcd_LCDC & (1<<0)) ? 1 : 0;
    u8 bgwin_tilemap_unsigned = bgwin_tilemap_low;


    u16 bgwin_tilemap_addr = bgwin_tilemap_low ? 0x8000 : 0x9000;
    u16 bgmap_addr = bgmap_high ? 0x9c00 : 0x9800;
    u16 winmap_addr = winmap_high ? 0x9c00 : 0x9800;

    u8 *bgwin_tiledata = &gb_state->mem_VRAM[bgwin_tilemap_addr - 0x8000];
    u8 *obj_tiledata = &gb_state->mem_VRAM[0x8000 - 0x8000];
    u8 *bgmap = &gb_state->mem_VRAM[bgmap_addr - 0x8000];
    u8 *winmap = &gb_state->mem_VRAM[winmap_addr - 0x8000];

    u8 bg_scroll_x = gb_state->io_lcd_SCX;
    u8 bg_scroll_y = gb_state->io_lcd_SCY;
    u8 win_pos_x = gb_state->io_lcd_WX;
    u8 win_pos_y = gb_state->io_lcd_WY;

    u8 bgwin_palette = gb_state->io_lcd_BGP;
    u8 obj_palette1 = gb_state->io_lcd_OBP0;
    u8 obj_palette2 = gb_state->io_lcd_OBP1;

    u8 obj_tile_height = obj_8x16 ? 16 : 8;

    /* OAM scan - gather (max 10) objects on this line in cache */
    /* TODO: sort the objs so those with smaller x coord have higher prio */
    struct OAMentry *OAM = (struct OAMentry*)&gb_state->mem_OAM[0];
    struct OAMentry objs[10];
    int num_objs = 0;
    if (obj_enable)
        for (int i = 0; i < 40; i++) {
            if (y >= OAM[i].y - 16 && y < OAM[i].y - 16 + obj_tile_height)
                objs[num_objs++] = OAM[i];
            if (num_objs == 10)
                break;
        }


    /* Draw all background pixels of this line. */
    if (bg_enable) {
        for (int x = 0; x < GB_LCD_WIDTH; x++) {
            int bg_x = (x + bg_scroll_x) % 256,
                bg_y = (y + bg_scroll_y) % 256;
            int bg_tile_x = bg_x / 8,
                bg_tile_y = bg_y / 8;
            int bg_idx = bg_tile_x + bg_tile_y * 32;
            int bg_tileoff_x = bg_x % 8,
                bg_tileoff_y = bg_y % 8;

            u8 tile_idx_raw = bgmap[bg_idx];
            s16 tile_idx = bgwin_tilemap_unsigned ? (s16)(u16)tile_idx_raw :
                                                    (s16)(s8)tile_idx_raw;

            /* BG tile attrs are only available on CGB, and are at same location
             * as tile numbers but in bank 1 instead of 0. */
            u8 attr = use_col ?  bgmap[bg_idx + VRAM_BANKSIZE] : 0;
            u8 vram_bank = (attr & (1<<3)) ? 1 : 0;

            /* We have packed 2-bit color indices here, so the bits look like:
            * (each bit denoted by the pixel index in tile)
            * 01234567 01234567 89abcdef 89abcdef ...
            * So for the 9th pixel (which is px 1,1) we need bytes 2+3 (9/8*2 [+1])
            * and then shift both by 7 (8-9%8).
            */
            int bg_tileoff = bg_tileoff_x + bg_tileoff_y * 8;
            int shift = 7 - bg_tileoff % 8;
            int tiledata_off = tile_idx * 16 + bg_tileoff/8*2;
            if (vram_bank)
                tiledata_off += VRAM_BANKSIZE;
            u8 b1 = bgwin_tiledata[tiledata_off];
            u8 b2 = bgwin_tiledata[tiledata_off + 1];
            u8 colidx = ((b1 >> shift) & 1) |
                    (((b2 >> shift) & 1) << 1);

            u16 col = 0;
            if (use_col) {
                u8 palidx = attr & 7;
                col = palette_get_col(gb_state->io_lcd_BGPD, palidx, colidx);
            } else
                col = palette_get_gray(bgwin_palette, colidx);
            pixbuf[x + y * GB_LCD_WIDTH] = col;
        }
    } else {
        /* Background disabled - set all pixels to 0 */
        for (int x = 0; x < GB_LCD_WIDTH; x++)
            pixbuf[x + y * GB_LCD_WIDTH] = 0;
    }

    /* Draw the window for this line. */
    if (win_enable) {
        for (int x = 0; x < GB_LCD_WIDTH; x++) {
            int win_x = x - win_pos_x + 7,
                win_y = y - win_pos_y;
            int tile_x = win_x / 8,
                tile_y = win_y / 8;
            int tileoff_x = win_x % 8,
                tileoff_y = win_y % 8;

            if (win_x < 0 || win_y < 0)
                continue;

            u8 tile_idx_raw = winmap[tile_x + tile_y * 32];
            s16 tile_idx = bgwin_tilemap_unsigned ? (s16)(u16)tile_idx_raw :
                                                    (s16)(s8)tile_idx_raw;

            /* We have packed 2-bit color indices here, so the bits look like:
            * (each bit denoted by the pixel index in tile)
            * 01234567 01234567 89abcdef 89abcdef ...
            * So for the 9th pixel (which is px 1,1) we need bytes 2+3 (9/8*2 [+1])
            * and then shift both by 7 (8-9%8).
            */
            int tileoff = tileoff_x + tileoff_y * 8;
            int shift = 7 - tileoff % 8;
            u8 b1 = bgwin_tiledata[tile_idx * 16 + tileoff/8*2];
            u8 b2 = bgwin_tiledata[tile_idx * 16 + tileoff/8*2 + 1];
            u8 colidx = ((b1 >> shift) & 1) |
                       (((b2 >> shift) & 1) << 1);

            u16 col = 0;
            if (use_col)
                col = palette_get_col(gb_state->io_lcd_BGPD, 0, colidx);
            else
                col = palette_get_gray(bgwin_palette, colidx);
            pixbuf[x + y * GB_LCD_WIDTH] = col;
        }
    }

    /* Draw any sprites (objects) on this line. */
    for (int x = 0; x < GB_LCD_WIDTH; x++) {
        for (int i = 0; i < num_objs; i++) {
            int obj_tileoff_x = x - (objs[i].x - 8),
                obj_tileoff_y = y - (objs[i].y - 16);

            if (obj_tileoff_x < 0 || obj_tileoff_x >= 8)
                continue;

            if (objs[i].flags & (1<<5)) /* Flip x */
                obj_tileoff_x = 7 - obj_tileoff_x;
            if (objs[i].flags & (1<<6)) /* Flip y */
                obj_tileoff_y = obj_tile_height - 1 - obj_tileoff_y;

            int obj_tileoff = obj_tileoff_x + obj_tileoff_y * 8;
            int shift = 7 - obj_tileoff % 8;
            int tiledata_off = objs[i].tile * 16 + obj_tileoff/8*2;
            if (use_col && objs[i].flags & (1<<3))
                tiledata_off += VRAM_BANKSIZE;
            u8 b1 = obj_tiledata[tiledata_off];
            u8 b2 = obj_tiledata[tiledata_off + 1];
            u8 colidx = ((b1 >> shift) & 1) |
                       (((b2 >> shift) & 1) << 1);

            if (colidx != 0) {
                if (objs[i].flags & (1<<7)) /* OBJ-to-BG prio */
                    if (pixbuf[x + y * GB_LCD_WIDTH] > 0)
                        continue;
                u16 col = 0;
                if (use_col) {
                    u8 palidx = objs[i].flags & 7;
                    col = palette_get_col(gb_state->io_lcd_OBPD, palidx, colidx);
                } else {
                    u8 pal = objs[i].flags & (1<<4) ? obj_palette2 : obj_palette1;
                    col = palette_get_gray(pal, colidx);
                }
                pixbuf[x + y * GB_LCD_WIDTH] = col;
            }
        }
    }
}

void gui_render_frame(char use_colors) {
    u32 *pixels = NULL;
    int pitch;
    if (SDL_LockTexture(texture, NULL, (void*)&pixels, &pitch)) {
        printf("LCD: SDL could not lock screen texture: %s\n", SDL_GetError());
        exit(1);
    }

    if (use_colors) {
        /* The colors stored in pixbuf are two byte each, 5 bits per rgb
         * component: -bbbbbgg gggrrrrr. We need to extract these, scale these
         * values from 0-1f to 0-ff and put them in RGBA format. For the scaling
         * we'd have to multiply by 0xff/0x1f, which is 8.23, approx 8, which is
         * a shift by 3. */
        for (int y = 0; y < GB_LCD_HEIGHT; y++)
            for (int x = 0; x < GB_LCD_WIDTH; x++) {
                int idx = x + y * GB_LCD_WIDTH;
                u16 rawcol = pixbuf[idx];
                u32 r = ((rawcol >>  0) & 0x1f) << 3;
                u32 g = ((rawcol >>  5) & 0x1f) << 3;
                u32 b = ((rawcol >> 10) & 0x1f) << 3;
                u32 col = (r << 24) | (g << 16) | (b << 8) | 0xff;
                pixels[idx] = col;
            }
    } else {
        /* The colors stored in pixbuf already went through the palette
         * translation, but are still 2 bit monochrome. */
        u32 palette[] = { 0xffffffff, 0xaaaaaaaa, 0x66666666, 0x11111111 };
        for (int y = 0; y < GB_LCD_HEIGHT; y++)
            for (int x = 0; x < GB_LCD_WIDTH; x++) {
                int idx = x + y * GB_LCD_WIDTH;
                pixels[idx] = palette[pixbuf[idx]];
            }
    }

    SDL_UnlockTexture(texture);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_Delay(1000/120);
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
                gb_state->emu_state->dbg_break_next = 1;
                break;

            case SDLK_s:
                gb_state->emu_state->make_savestate = 1;
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
