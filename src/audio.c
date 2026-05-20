#include "audio.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAMPLE_RATE 44100

// We keep the raw audio buffers allocated, because Mix_QuickLoad_RAW does not copy them.
static int16_t *audio_buffers[SFX_COUNT];
static Mix_Chunk *mix_chunks[SFX_COUNT];
static int ufo_channel = -1;
static int thrust_channel = -1;

static int16_t* generate_laser(int *out_len) {
    float duration = 0.15f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = malloc(len * sizeof(int16_t));
    float phase = 0.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / duration;
        float freq = 900.0f - 700.0f * progress;
        phase += 2.0f * (float)M_PI * freq / SAMPLE_RATE;
        float amp = 12000.0f * (1.0f - progress);
        buf[i] = (int16_t)(sinf(phase) * amp);
    }
    *out_len = len * sizeof(int16_t);
    return buf;
}

static int16_t* generate_noise(float duration, float decay, int *out_len) {
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = malloc(len * sizeof(int16_t));
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float amp = 16000.0f * expf(-decay * t);
        buf[i] = (int16_t)(noise * amp);
    }
    *out_len = len * sizeof(int16_t);
    return buf;
}

static int16_t* generate_thrust(int *out_len) {
    float duration = 0.25f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = malloc(len * sizeof(int16_t));
    float phase = 0.0f;
    for (int i = 0; i < len; i++) {
        // Low rumble combining 55Hz and 110Hz, with some white noise
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        phase += 2.0f * (float)M_PI * 55.0f / SAMPLE_RATE;
        float signal = sinf(phase) + 0.5f * sinf(phase * 2.0f) + 0.2f * noise;
        buf[i] = (int16_t)(signal * 8000.0f);
    }
    *out_len = len * sizeof(int16_t);
    return buf;
}

static int16_t* generate_beat(float freq, int *out_len) {
    float duration = 0.12f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = malloc(len * sizeof(int16_t));
    float phase = 0.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / duration;
        phase += 2.0f * (float)M_PI * freq / SAMPLE_RATE;
        float amp = 18000.0f * (1.0f - progress) * (1.0f - progress);
        buf[i] = (int16_t)(sinf(phase) * amp);
    }
    *out_len = len * sizeof(int16_t);
    return buf;
}

static int16_t* generate_ufo_loop(int *out_len) {
    float duration = 0.4f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = malloc(len * sizeof(int16_t));
    float phase = 0.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        // FM synthesis: carrier is 600Hz, modulated by 8Hz sine wave
        float freq = 600.0f + 150.0f * sinf(2.0f * (float)M_PI * 8.0f * t);
        phase += 2.0f * (float)M_PI * freq / SAMPLE_RATE;
        buf[i] = (int16_t)(sinf(phase) * 8000.0f);
    }
    *out_len = len * sizeof(int16_t);
    return buf;
}

static int16_t* generate_ufo_fire(int *out_len) {
    float duration = 0.2f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = malloc(len * sizeof(int16_t));
    float phase = 0.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / duration;
        float freq = 1200.0f - 800.0f * progress;
        phase += 2.0f * (float)M_PI * freq / SAMPLE_RATE;
        float amp = 10000.0f * (1.0f - progress);
        buf[i] = (int16_t)(sinf(phase) * amp);
    }
    *out_len = len * sizeof(int16_t);
    return buf;
}

int audio_init() {
    // Initialize SDL Mixer
    if (Mix_OpenAudio(SAMPLE_RATE, AUDIO_S16SYS, 1, 1024) < 0) {
        SDL_Log("SDL_mixer could not initialize! SDL_mixer Error: %s", Mix_GetError());
        return 0;
    }

    int size;
    // Generate SFX data in memory
    audio_buffers[SFX_FIRE] = generate_laser(&size);
    mix_chunks[SFX_FIRE] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_FIRE], size);

    audio_buffers[SFX_EXPLOSION_LG] = generate_noise(1.0f, 4.0f, &size);
    mix_chunks[SFX_EXPLOSION_LG] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_EXPLOSION_LG], size);

    audio_buffers[SFX_EXPLOSION_MD] = generate_noise(0.6f, 6.0f, &size);
    mix_chunks[SFX_EXPLOSION_MD] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_EXPLOSION_MD], size);

    audio_buffers[SFX_EXPLOSION_SM] = generate_noise(0.3f, 10.0f, &size);
    mix_chunks[SFX_EXPLOSION_SM] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_EXPLOSION_SM], size);

    audio_buffers[SFX_THRUST] = generate_thrust(&size);
    mix_chunks[SFX_THRUST] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_THRUST], size);

    audio_buffers[SFX_BEAT1] = generate_beat(110.0f, &size);
    mix_chunks[SFX_BEAT1] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_BEAT1], size);

    audio_buffers[SFX_BEAT2] = generate_beat(90.0f, &size);
    mix_chunks[SFX_BEAT2] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_BEAT2], size);

    audio_buffers[SFX_UFO_FIRE] = generate_ufo_fire(&size);
    mix_chunks[SFX_UFO_FIRE] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_UFO_FIRE], size);

    audio_buffers[SFX_UFO_LOOP] = generate_ufo_loop(&size);
    mix_chunks[SFX_UFO_LOOP] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_UFO_LOOP], size);

    return 1;
}

void audio_play(SoundEffect sfx) {
    if (sfx < 0 || sfx >= SFX_COUNT || !mix_chunks[sfx]) return;

    if (sfx == SFX_UFO_LOOP) {
        if (ufo_channel == -1 || !Mix_Playing(ufo_channel)) {
            ufo_channel = Mix_PlayChannel(-1, mix_chunks[sfx], -1); // loop infinitely
        }
    } else if (sfx == SFX_THRUST) {
        if (thrust_channel == -1 || !Mix_Playing(thrust_channel)) {
            thrust_channel = Mix_PlayChannel(-1, mix_chunks[sfx], -1); // loop infinitely
        }
    } else {
        Mix_PlayChannel(-1, mix_chunks[sfx], 0);
    }
}

void audio_stop(SoundEffect sfx) {
    if (sfx == SFX_UFO_LOOP) {
        if (ufo_channel != -1 && Mix_Playing(ufo_channel)) {
            Mix_HaltChannel(ufo_channel);
            ufo_channel = -1;
        }
    } else if (sfx == SFX_THRUST) {
        if (thrust_channel != -1 && Mix_Playing(thrust_channel)) {
            Mix_HaltChannel(thrust_channel);
            thrust_channel = -1;
        }
    }
}

void audio_set_volume(int volume_percent) {
    if (volume_percent < 0) volume_percent = 0;
    if (volume_percent > 100) volume_percent = 100;
    Mix_Volume(-1, (volume_percent * MIX_MAX_VOLUME) / 100);
}

void audio_cleanup() {
    for (int i = 0; i < SFX_COUNT; i++) {
        if (mix_chunks[i]) {
            Mix_FreeChunk(mix_chunks[i]);
            mix_chunks[i] = NULL;
        }
        if (audio_buffers[i]) {
            free(audio_buffers[i]);
            audio_buffers[i] = NULL;
        }
    }
    Mix_CloseAudio();
}
