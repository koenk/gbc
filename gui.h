#ifndef GUI_H
#define GUI_H

int gui_audio_init(int sample_rate, int channels, size_t sndbuf_size,
        uint8_t *sndbuf);

int gui_lcd_init(int width, int height, int zoom, char *wintitle);
void gui_lcd_render_frame(char use_colors, uint16_t *pixbuf);

enum gui_input_type {
    GUI_INPUT_NONE,
    GUI_INPUT_EXIT,
    GUI_INPUT_BUTTON_DOWN,
    GUI_INPUT_BUTTON_UP,
    GUI_INPUT_SAVESTATE,
    GUI_INPUT_DBGBREAK,
};

enum gui_input_button {
    GUI_BUTTON_LEFT,
    GUI_BUTTON_RIGHT,
    GUI_BUTTON_UP,
    GUI_BUTTON_DOWN,
    GUI_BUTTON_A,
    GUI_BUTTON_B,
    GUI_BUTTON_START,
    GUI_BUTTON_SELECT,
};

struct gui_input {
    enum gui_input_type type;
    enum gui_input_button button;
};

int gui_input_poll(struct gui_input *input);

#endif
