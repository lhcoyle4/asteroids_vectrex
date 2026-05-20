#ifndef AUDIO_H
#define AUDIO_H

typedef enum {
    SFX_FIRE,
    SFX_EXPLOSION_LG,
    SFX_EXPLOSION_MD,
    SFX_EXPLOSION_SM,
    SFX_THRUST,
    SFX_BEAT1,
    SFX_BEAT2,
    SFX_UFO_FIRE,
    SFX_UFO_LOOP,
    SFX_COUNT
} SoundEffect;

// Initialize procedural audio system
int audio_init();

// Play a sound effect (non-blocking)
void audio_play(SoundEffect sfx);

// Set overall volume (0 to 100)
void audio_set_volume(int volume_percent);

// Stop a looping sound effect
void audio_stop(SoundEffect sfx);

// Clean up audio resources
void audio_cleanup();

#endif
