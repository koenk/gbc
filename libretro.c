/*
 * Frontend implementation of the emulator as a libretro core.
 *
 * By implementing the interface (as specified by libretro.h, provided by
 * libretro) the emulator can be compiled as a shared library that libretro
 * implementations such as RetroArch can load. libretro provides a uniform
 * interface for video, audio, input and file loading on numerous platforms.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libretro.h"

#include "hwdefs.h"
#include "types.h"
#include "emu.h"

struct gb_state gb_state;
struct player_input input;

/* Callbacks the core (we) can use to call intro libretro. */
static retro_environment_t env_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

void retro_set_environment(retro_environment_t cb) {
    env_cb = cb;

    struct retro_input_descriptor desc[] = {
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
    };
    env_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

    enum retro_pixel_format pixfmt = RETRO_PIXEL_FORMAT_0RGB1555;
    if (!env_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixfmt))
        fprintf(stderr, "Format RGB555 not supported!\n");
}
void retro_set_video_refresh(retro_video_refresh_t cb) {
    video_cb = cb;
}
void retro_set_audio_sample(retro_audio_sample_t cb) {
    audio_cb = cb;
}
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
    audio_batch_cb = cb;
}
void retro_set_input_poll(retro_input_poll_t cb) {
    input_poll_cb = cb;
}
void retro_set_input_state(retro_input_state_t cb) {
    input_state_cb = cb;
}

typedef uint16_t pixel_t;
static pixel_t *framebuf;
static size_t framebuf_size;

/* Library global initialization/deinitialization. */
void retro_init(void) {
    framebuf_size = GB_LCD_WIDTH * GB_LCD_HEIGHT * sizeof(pixel_t);
    framebuf = malloc(framebuf_size);
    memset(framebuf, 0, framebuf_size);
}
void retro_deinit(void) {
    free(framebuf);
    framebuf = NULL;
}

/* Must return RETRO_API_VERSION. Used to validate ABI compatibility when the
 * API is revised. */
unsigned retro_api_version(void) {
    return RETRO_API_VERSION;
}

/* Return static information about our core. */
void retro_get_system_info(struct retro_system_info *info) {
    info->library_name = "KoenGB";
    info->library_version = "0.1";
    info->valid_extensions = "gb|gbc";
    info->need_fullpath = true; /* TODO? */
    info->block_extract = false;
}

/* Gets information about (emulated) system audio/video timings and geometry. */
void retro_get_system_av_info(struct retro_system_av_info *info) {
    info->geometry.base_width = GB_LCD_WIDTH;
    info->geometry.base_height = GB_LCD_HEIGHT;
    info->geometry.max_width = GB_LCD_WIDTH;
    info->geometry.max_height = GB_LCD_HEIGHT;
    info->geometry.aspect_ratio = 1. * GB_LCD_WIDTH / GB_LCD_HEIGHT;

    info->timing.fps = 1. * GB_FREQ / GB_LCD_FRAME_CLKS;
    printf("Requesting %f fps\n", info->timing.fps);
    info->timing.sample_rate = 44100;
}

/* Sets device to be used for player 'port'. */
void retro_set_controller_port_device(unsigned port, unsigned device) {
    (void)port, (void)device;
}

/* Resets the current game. */
void retro_reset(void) {

}

void update_inputs(void) {
    unsigned port = 0;
    unsigned dev = RETRO_DEVICE_JOYPAD;
    unsigned idx = 0;

    input_poll_cb();

#define INP(id) input_state_cb(port, dev, idx, RETRO_DEVICE_ID_JOYPAD_ ##id)
    input.button_left   = INP(LEFT);
    input.button_right  = INP(RIGHT);
    input.button_up     = INP(UP);
    input.button_down   = INP(DOWN);
    input.button_a      = INP(A);
    input.button_b      = INP(B);
    input.button_start  = INP(START);
    input.button_select = INP(SELECT);
#undef INP

    emu_process_inputs(&gb_state, &input);
}

void render_frame(void) {
    if (gb_state.gb_type == GB_TYPE_CGB) {
        /* The gameboy uses a BGR555 format, so swap around colors. */
        for (int y = 0; y < GB_LCD_HEIGHT; y++)
            for (int x = 0; x < GB_LCD_WIDTH; x++) {
                int idx = x + y * GB_LCD_WIDTH;
                uint16_t rawcol = gb_state.emu_state->lcd_pixbuf[idx];
                uint32_t r = ((rawcol >>  0) & 0x1f) << 0;
                uint32_t g = ((rawcol >>  5) & 0x1f) << 0;
                uint32_t b = ((rawcol >> 10) & 0x1f) << 0;
                uint16_t col = (r << 10) | (g << 5) | (b << 0);
                framebuf[idx] = col;
            }
    } else {
        /* The colors stored in pixbuf already went through the palette
         * translation, but are still 2 bit monochrome. */
        uint32_t palette[] = { 0x6318, 0x4a52, 0x318c, 0x18c6 };
        for (int y = 0; y < GB_LCD_HEIGHT; y++)
            for (int x = 0; x < GB_LCD_WIDTH; x++) {
                int idx = x + y * GB_LCD_WIDTH;
                framebuf[idx] = palette[gb_state.emu_state->lcd_pixbuf[idx]];
            }
    }
    video_cb(framebuf, GB_LCD_WIDTH, GB_LCD_HEIGHT, GB_LCD_WIDTH * sizeof(pixel_t));
}

/* Runs the game for one video frame. */
void retro_run(void) {
    update_inputs();

    emu_step_frame(&gb_state);

    render_frame();
}

/* Returns size to serialize internal state (save state). */
size_t retro_serialize_size(void) {
    return 0;
}

/* Serializes internal state (save state). */
bool retro_serialize(void *data, size_t size) {
    (void)data, (void)size;
    return false; /* Fail */
}
bool retro_unserialize(const void *data, size_t size) {
    (void)data, (void)size;
    return false; /* Fail */
}

void retro_cheat_reset(void) {

}
void retro_cheat_set(unsigned index, bool enabled, const char *code) {
    (void)index, (void)enabled, (void)code;
}

/* Loads a game. */
bool retro_load_game(const struct retro_game_info *info) {
    printf("Loading %s\n", info->path);

    struct emu_args args;
    memset(&args, 0, sizeof(struct emu_args));
    args.rom_filename = (char*)info->path;

    if (emu_init(&gb_state, &args)) {
        fprintf(stderr, "Initialization failed\n");
        return false;
    }

    return true;
}

/* Loads a special kind of game. Should not be used except in extreme cases. */
bool retro_load_game_special(unsigned game_type,
        const struct retro_game_info *info, size_t num_info) {
    (void)game_type, (void)info, (void)num_info;
    return false;
}

/* Unloads a currently loaded game. */
void retro_unload_game(void) {
    /* TODO: save extram */
}

/* Gets region (i.e., country) of game. */
unsigned retro_get_region(void) {
    return RETRO_REGION_NTSC;
}

/* Gets region of memory (e.g., save RAM, RTC, RAM/VRAM). */
void *retro_get_memory_data(unsigned id) {
    (void)id;
    return NULL;
}
size_t retro_get_memory_size(unsigned id) {
    (void)id;
    return 0;
}
