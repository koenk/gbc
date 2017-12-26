#ifndef AUDIO_H
#define AUDIO_H

#include "types.h"

static const int AUDIO_SAMPLE_RATE = 44100; /* Hz */
static const int AUDIO_CHANNELS = 2;
static const int AUDIO_SNDBUF_SIZE = 1024; /* Per channel */

int audio_init(struct gb_state *s);
void audio_update(struct gb_state *s);

#endif
