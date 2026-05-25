/* fuligin/include/audio.h
 * Public interface for the FULIGIN procedural audio engine.
 * All sound effects are synthesized at startup; no external audio files required.
 * Uses SDL2_mixer for channel management and playback.
 *
 * Author: [author]
 * Date: 2025
 */

#ifndef AUDIO_H
#define AUDIO_H

/* ===========================================================================
 * SOUND EFFECT ENUMERATION
 * =========================================================================== */

typedef enum {
    /* --- Player weapons --- */
    SFX_FIRE,

    /* --- Generic explosions (noise-based, three sizes) --- */
    SFX_EXPLOSION_LG, SFX_EXPLOSION_MD, SFX_EXPLOSION_SM,

    /* --- Looping player-ship sounds --- */
    SFX_THRUST,

    /* --- Heartbeat percussion (two pitches) --- */
    SFX_BEAT1, SFX_BEAT2,

    /* --- Basic enemy (UFO) sounds --- */
    SFX_UFO_FIR