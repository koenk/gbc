#ifndef GUI_H
#define GUI_H

#include "types.h"

#define GUI_WINDOW_TITLE "KoenGB"
#define GUI_PX_WIDTH  160
#define GUI_PX_HEIGHT 144
#define GUI_ZOOM      4

int gui_init(void);
void gui_render_frame(struct gb_state *gb_state);
int gui_handleinputs(struct gb_state *gb_state);

#endif
