#ifndef AUDIO_H
#define AUDIO_H

typedef enum {
    SFX_FIRE, SFX_EXPLOSION_LG, SFX_EXPLOSION_MD, SFX_EXPLOSION_SM,
    SFX_THRUST, SFX_BEAT1, SFX_BEAT2, SFX_UFO_FIRE, SFX_UFO_LOOP,
    SFX_TURRET_FIRE, SFX_UFO_FIRE_LG, SFX_UFO_FIRE_SM, SFX_UFO_FIRE_KAMIKAZE,
    SFX_UFO_FIRE_BOMBER, SFX_UFO_FIRE_EYE, SFX_UFO_FIRE_ELDRITCH, SFX_UFO_FIRE_DAEMON,
    SFX_EXPL_ROCK_LG, SFX_EXPL_ROCK_MD, SFX_EXPL_ROCK_SM,
    SFX_EXPL_METAL_LG, SFX_EXPL_METAL_MD, SFX_EXPL_METAL_SM,
    SFX_EXPL_ICE_LG, SFX_EXPL_ICE_MD, SFX_EXPL_ICE_SM,
    SFX_COUNT
} SoundEffect;

int audio_init();
void audio_play(SoundEffect sfx);
void audio_stop(SoundEffect sfx);
void audio_cleanup();
void audio_set_volume(int volume_percent);
void audio_update_volumes();
void audio_mute(int muted);
void audio_set_music_params(float combat, float spookiness, int paused, int gameplay, float dt);

#endif /* AUDIO_H */
