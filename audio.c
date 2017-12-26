#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "audio.h"
#include "hwdefs.h"

int audio_init(struct gb_state *s) {
    s->emu_state->audio_sndbuf = malloc(AUDIO_SNDBUF_SIZE * AUDIO_CHANNELS);
    if (!s->emu_state->audio_sndbuf)
        return 0;
    memset(s->emu_state->audio_sndbuf, 0, AUDIO_SNDBUF_SIZE * AUDIO_CHANNELS);
    return 0;
}

void audio_update(struct gb_state *s) {
    const double sample_freq = 1. * AUDIO_SAMPLE_RATE / AUDIO_SNDBUF_SIZE;

    u8 *sndbuf = s->emu_state->audio_sndbuf;

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

    memset(sndbuf, 0, AUDIO_SNDBUF_SIZE * AUDIO_CHANNELS);

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
        u32 osc_len = AUDIO_SNDBUF_SIZE / oscs_in_buf;
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
