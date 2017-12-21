#ifndef GUI_H
#define GUI_H

#include "types.h"

#define GUI_WINDOW_TITLE "KoenGB"
#define GUI_ZOOM      4

int audio_init(struct gb_state *s);
void audio_update(struct gb_state *s);

int gui_init(void);
void gui_render_current_line(struct gb_state *gb_state);
void gui_render_frame(struct gb_state *gb_state);
int gui_handleinputs(struct gb_state *gb_state);

#endif
