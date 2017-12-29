#ifndef GUI_H
#define GUI_H

#include "player_input.h"

int gui_audio_init(int sample_rate, int channels, size_t sndbuf_size,
        uint8_t *sndbuf);

int gui_lcd_init(int width, int height, int zoom, char *wintitle);
void gui_lcd_render_frame(char use_colors, uint16_t *pixbuf);


int gui_input_poll(struct player_input *input);

#endif
