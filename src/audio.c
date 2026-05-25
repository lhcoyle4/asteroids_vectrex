#include "audio.h"
#include "game.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAMPLE_RATE 44100

// We keep the raw audio buffers allocated, because Mix_QuickLoad_RAW does not copy them.
static int16_t *audio_buffers[SFX_COUNT];
static Mix_Chunk *mix_chunks[SFX_COUNT];
static int ufo_channel = -1;
static int thrust_channel = -1;

/**
 * 1. generate_laser
 * - High-pitch frequency sweep downwards (900Hz to 200Hz).
 * - Linear volume decay.
 * - Duration: ~0.15 seconds.
 * - Chiptune flavor: uses a custom combination of fundamental sine wave and
 *   odd harmonics (3rd harmonic) to create a warm, retro synth character.
 */
static int16_t* generate_laser(int *out_len) {
    if (!out_len) return NULL;
    float duration = 0.15f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = (int16_t*)malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    double phase = 0.0;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / duration;

        // Linear sweep from 900Hz to 200Hz
        float freq = 900.0f - 700.0f * progress;
        phase += 2.0 * M_PI * freq / SAMPLE_RATE;
        if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;

        // Linear volume decay
        float amp = 12000.0f * (1.0f - progress);

        // Mix fundamental (sine) and 3rd harmonic for a retro chiptune shape
        float wave = (float)(sin(phase) + 0.3 * sin(3.0 * phase));
        buf[i] = (int16_t)(wave * amp);
    }
    *out_len = len * sizeof(int16_t);
    return buf;
}

/**
 * 2. generate_noise
 * - White noise generation (scaled to float in [-1.0, 1.0]).
 * - Multiplied by exponential volume decay: amp = volume * exp(-decay * t).
 * - Used for general explosions.
 */
static int16_t* generate_noise(float duration, float decay, int *out_len) {
    if (!out_len) return NULL;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = (int16_t*)malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;

        // White noise scaled to float [-1.0, 1.0]
        float noise = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;

        // Exponential volume decay
        float amp = 16000.0f * (float)exp(-decay * t);

        buf[i] = (int16_t)(noise * amp);
    }
    *out_len = len * sizeof(int16_t);
    return buf;
}

/**
 * 3. generate_thrust
 * - Low rumble combining low frequencies (55Hz and 110Hz) with low-pass filtered noise.
 * - Loopable (exactly 0.25 seconds).
 * - Mathematics of loopability:
 *   For a duration of exactly 0.25 seconds (11025 samples at 44100Hz), the fundamental
 *   frequency must be a multiple of 1.0s / 0.25s = 4Hz to avoid phase discontinuities.
 *   Hence, we adjust 55Hz to 56Hz (14 cycles) and 110Hz to 112Hz (28 cycles).
 *   Additionally, the white noise is smoothed with a 1-pole low-pass filter to produce
 *   a warm, deep rocket rumble instead of harsh white hiss.
 */
static int16_t* generate_thrust(int *out_len) {
    if (!out_len) return NULL;
    float duration = 0.25f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = (int16_t*)malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    float f1 = 56.0f;  // Nearest multiple of 4Hz to 55Hz (completes exactly 14 cycles in 0.25s)
    float f2 = 112.0f; // Nearest multiple of 4Hz to 110Hz (completes exactly 28 cycles in 0.25s)

    double phase1 = 0.0;
    double phase2 = 0.0;
    float noise_lpf = 0.0f;

    for (int i = 0; i < len; i++) {
        phase1 += 2.0 * M_PI * f1 / SAMPLE_RATE;
        phase2 += 2.0 * M_PI * f2 / SAMPLE_RATE;

        // Keep phases wrapped to prevent float overflow / precision loss
        if (phase1 > 2.0 * M_PI) phase1 -= 2.0 * M_PI;
        if (phase2 > 2.0 * M_PI) phase2 -= 2.0 * M_PI;

        // White noise
        float noise = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;

        // 1-pole low pass filter: y[n] = y[n-1] + alpha * (x[n] - y[n-1])
        noise_lpf += 0.15f * (noise - noise_lpf);

        // Mix sine frequencies and low-pass filtered noise rumble
        float signal = (float)(sin(phase1) + 0.5 * sin(phase2) + 0.4 * noise_lpf);

        buf[i] = (int16_t)(signal * 8000.0f);
    }
    *out_len = len * sizeof(int16_t);
    return buf;
}

/**
 * 4. generate_beat
 * - A retro heartbeat drum beat.
 * - Sine wave starting at specified frequency with pitch sweep down (simulating analog kick drum behavior).
 * - Rapid exponential decay (exp(-15.0 * t)) for a punchy beat.
 */
static int16_t* generate_beat(float freq, int *out_len) {
    if (!out_len) return NULL;
    float duration = 0.12f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = (int16_t*)malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    double phase = 0.0;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / duration;

        // Pitch sweep downwards for a retro analog drum thump
        float current_freq = freq * (1.5f - 0.7f * progress);
        phase += 2.0 * M_PI * current_freq / SAMPLE_RATE;
        if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;

        // Rapid exponential decay
        float amp = 18000.0f * (float)exp(-15.0 * t);

        buf[i] = (int16_t)(sin(phase) * amp);
    }
    *out_len = len * sizeof(int16_t);
    return buf;
}

/**
 * 5. generate_ufo_loop
 * - A loopable space warble using Frequency Modulation (FM) synthesis.
 * - Carrier wave modulated by a slow LFO (600Hz carrier frequency, modulated by 8Hz modulator).
 * - Mathematics of loopability:
 *   For a duration of exactly 0.25 seconds:
 *   1) The modulator (8Hz) completes exactly 8 * 0.25 = 2.0 cycles (an integer).
 *   2) The carrier (600Hz) completes exactly 600 * 0.25 = 150.0 cycles (an integer).
 *   Because both frequencies complete an integer number of cycles, both the carrier
 *   phase and the modulator phase align perfectly at the loop boundary, preventing
 *   any clicks or phase discrepancies when looped.
 */
static int16_t* generate_ufo_loop(int *out_len) {
    if (!out_len) return NULL;
    float duration = 0.25f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = (int16_t*)malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    float fc = 600.0f;    // Carrier frequency
    float fm = 8.0f;      // Modulator frequency (LFO)
    float index = 150.0f; // Modulation index (freq deviation in Hz)

    double phase = 0.0;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;

        // FM formula: frequency(t) = fc + index * sin(2 * PI * fm * t)
        float freq = fc + index * (float)sin(2.0 * M_PI * fm * t);
        phase += 2.0 * M_PI * freq / SAMPLE_RATE;
        if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;

        buf[i] = (int16_t)(sin(phase) * 8000.0f);
    }
    *out_len = len * sizeof(int16_t);
    return buf;
}

/**
 * 6. generate_ufo_fire
 * - Alien laser firing sound.
 * - Modulated frequency sweep: frequency sweeps down from 1200Hz to 400Hz while
 *   being modulated by a fast 45Hz modulator (150Hz depth) to create a warbling alien effect.
 * - Volume decay is a blend of linear and exponential envelopes.
 */
static int16_t* generate_ufo_fire(int *out_len) {
    if (!out_len) return NULL;
    float duration = 0.22f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = (int16_t*)malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    double phase = 0.0;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / duration;

        // Modulated frequency sweep
        float base_freq = 1200.0f - 800.0f * progress;
        float mod = 150.0f * (float)sin(2.0 * M_PI * 45.0 * t);
        float freq = base_freq + mod;

        phase += 2.0 * M_PI * freq / SAMPLE_RATE;
        if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;

        // Blend of exponential and linear volume decay for a punchy synth shape
        float amp = 12000.0f * (float)exp(-6.0 * t) * (1.0f - progress);

        buf[i] = (int16_t)(sin(phase) * amp);
    }
    *out_len = len * sizeof(int16_t);
    return buf;
}

/**
 * 1. Quick, punchy energy weapon sound. Fast decay, combining a medium
 *    sine sweep with brief noise at the start.
 */
static int16_t* generate_turret_fire(int *out_len) {
    float duration = 0.15f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    float phase = 0.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / duration;

        // Sweep from 800 Hz down to 200 Hz
        float freq = 800.0f - 600.0f * progress;
        phase += 2.0f * (float)M_PI * freq / SAMPLE_RATE;

        // Sine wave with fast exponential decay
        float sine_amp = 14000.0f * expf(-18.0f * t);
        float sine_val = sinf(phase) * sine_amp;

        // Brief noise burst at the start (decays within 0.04s)
        float noise_val = 0.0f;
        if (t < 0.04f) {
            float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
            float noise_amp = 12000.0f * (1.0f - t / 0.04f);
            noise_val = noise * noise_amp;
        }

        float combined = sine_val + noise_val;
        // Clip protection
        if (combined > 32767.0f) combined = 32767.0f;
        if (combined < -32768.0f) combined = -32768.0f;

        buf[i] = (int16_t)combined;
    }

    *out_len = len * sizeof(int16_t);
    return buf;
}

/**
 * 2. Heavy plasma blaster. Deep, powerful sine wave starting low,
 *    sweeping down even lower, combined with some rumbling noise.
 */
static int16_t* generate_ufo_fire_lg(int *out_len) {
    float duration = 0.5f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    float phase = 0.0f;
    float noise_lpf = 0.0f; // State for low-pass filter to make rumble

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / duration;

        // Sweep from 150 Hz down to 50 Hz
        float freq = 150.0f - 100.0f * progress;
        phase += 2.0f * (float)M_PI * freq / SAMPLE_RATE;

        // Rich sine with a subharmonic (0.5x frequency) for heavy presence
        float sine_val = sinf(phase) + 0.4f * sinf(phase * 0.5f);
        float sine_amp = 18000.0f * (1.0f - progress);
        float wave = sine_val * sine_amp;

        // Rumbling noise: apply simple IIR low-pass filter (alpha = 0.1f)
        float raw_noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        noise_lpf += 0.1f * (raw_noise - noise_lpf);
        float rumble = noise_lpf * 10000.0f * expf(-6.0f * t);

        float combined = wave + rumble;
        if (combined > 32767.0f) combined = 32767.0f;
        if (combined < -32768.0f) combined = -32768.0f;

        buf[i] = (int16_t)combined;
    }

    *out_len = len * sizeof(int16_t);
    return buf;
}

/**
 * 3. High-speed light laser. Short high-pitched chirps (1500Hz to 800Hz in 0.08s).
 */
static int16_t* generate_ufo_fire_sm(int *out_len) {
    float duration = 0.08f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    float phase = 0.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / duration;

        // Frequency sweep from 1500 Hz down to 800 Hz
        float freq = 1500.0f - 700.0f * progress;
        phase += 2.0f * (float)M_PI * freq / SAMPLE_RATE;

        // Fast linear decay amplitude envelope
        float amp = 16000.0f * (1.0f - progress);
        float val = sinf(phase) * amp;

        buf[i] = (int16_t)val;
    }

    *out_len = len * sizeof(int16_t);
    return buf;
}

/**
 * 4. Kamikaze ship warning. Rapidly rising pitch siren (300Hz to 1200Hz)
 *    with pitch modulation (vibrato) and amplitude modulation.
 */
static int16_t* generate_ufo_fire_kamikaze(int *out_len) {
    float duration = 0.8f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    float phase = 0.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / duration;

        // Base siren pitch rises from 300 Hz to 1200 Hz
        float base_freq = 300.0f + 900.0f * progress;

        // 15 Hz vibrato pitch modulation (mod depth = 150 Hz)
        float lfo = sinf(2.0f * (float)M_PI * 15.0f * t);
        float freq = base_freq + 150.0f * lfo;

        phase += 2.0f * (float)M_PI * freq / SAMPLE_RATE;

        // 8 Hz amplitude modulation (tremolo pulse)
        float tremolo = 0.8f + 0.2f * sinf(2.0f * (float)M_PI * 8.0f * t);
        float amp = 14000.0f * tremolo;

        // Smooth fade-in and fade-out to prevent clicks
        float env = 1.0f;
        if (t < 0.05f) {
            env = t / 0.05f;
        } else if (t > duration - 0.05f) {
            env = (duration - t) / 0.05f;
        }

        float val = sinf(phase) * amp * env;
        buf[i] = (int16_t)val;
    }

    *out_len = len * sizeof(int16_t);
    return buf;
}

/**
 * 5. Heavy bomb drop. Deep thud with slow decay and exponential pitch drop.
 */
static int16_t* generate_ufo_fire_bomber(int *out_len) {
    float duration = 0.8f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    float phase = 0.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;

        // Rapid frequency drop from 340 Hz to 40 Hz
        float freq = 300.0f * expf(-20.0f * t) + 40.0f;
        phase += 2.0f * (float)M_PI * freq / SAMPLE_RATE;

        // Slow exponential decay for a lingering tail
        float amp = 20000.0f * expf(-4.0f * t);
        float val = sinf(phase) * amp;

        buf[i] = (int16_t)val;
    }

    *out_len = len * sizeof(int16_t);
    return buf;
}

/**
 * 6. Strange space beam. Glassy/futuristic sound utilizing
 *    phase modulation (FM synthesis) and amplitude ring modulation.
 */
static int16_t* generate_ufo_fire_eye(int *out_len) {
    float duration = 0.6f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / duration;

        // Modulator: 450 Hz wave
        float mod_phase = 2.0f * (float)M_PI * 450.0f * t;
        // Modulation index starting at 3.0 and decaying
        float mod_val = sinf(mod_phase) * 3.0f * (1.0f - progress);

        // Carrier: 600 Hz phase-modulated by modulator (FM)
        float car_freq = 600.0f;
        float car_phase = 2.0f * (float)M_PI * car_freq * t + mod_val;
        float signal = sinf(car_phase);

        // 25 Hz amplitude ring modulation
        float ring = sinf(2.0f * (float)M_PI * 25.0f * t);
        float amp = 16000.0f * (0.6f + 0.4f * ring) * expf(-2.0f * t);

        float val = signal * amp;
        buf[i] = (int16_t)val;
    }

    *out_len = len * sizeof(int16_t);
    return buf;
}

/**
 * 7. Creepy boss attack. Low frequency metallic screech/drone with sub-harmonics.
 *    Combines non-harmonic sines with rapid frequency modulation.
 */
static int16_t* generate_ufo_fire_eldritch(int *out_len) {
    float duration = 1.0f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    float phase1 = 0.0f;
    float phase2 = 0.0f;
    float phase3 = 0.0f;
    float screech_phase = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / duration;

        // Dissonant low frequency drone sines
        phase1 += 2.0f * (float)M_PI * 90.0f / SAMPLE_RATE;
        phase2 += 2.0f * (float)M_PI * 135.0f / SAMPLE_RATE;
        phase3 += 2.0f * (float)M_PI * 45.0f / SAMPLE_RATE; // Deep sub-harmonic

        // Rapid screech (high freq modulated by 45 Hz LFO)
        float screech_freq = 1200.0f + 300.0f * sinf(2.0f * (float)M_PI * 45.0f * t);
        screech_phase += 2.0f * (float)M_PI * screech_freq / SAMPLE_RATE;

        float drone = sinf(phase1) * 0.4f + sinf(phase2) * 0.3f + sinf(phase3) * 0.5f;
        // Screech amplitude pulses slowly
        float screech_pulse = 0.5f + 0.5f * sinf(2.0f * (float)M_PI * 5.0f * t);
        float screech = sinf(screech_phase) * 0.4f * screech_pulse;

        // Combine drone, screech, and metallic ring modulation
        float signal = drone + screech + 0.3f * drone * sinf(screech_phase);

        // Slow metallic envelope decay (quadratic to hold body)
        float amp = 12000.0f * (1.0f - progress * progress);
        
        // 0.05s fade-in to prevent initial pop
        float env = 1.0f;
        if (t < 0.05f) {
            env = t / 0.05f;
        }

        float combined = signal * amp * env;
        if (combined > 32767.0f) combined = 32767.0f;
        if (combined < -32768.0f) combined = -32768.0f;

        buf[i] = (int16_t)combined;
    }

    *out_len = len * sizeof(int16_t);
    return buf;
}

/**
 * 8. Chaotic noise bursts, rapid screeching or bitcrushed-style sweeps.
 *    Downsamples the rate dynamically (sweeping N-sample sample-and-hold)
 *    and quantizes amplitudes to mimic low-resolution retro registers.
 */
static int16_t* generate_ufo_fire_daemon(int *out_len) {
    float duration = 0.5f;
    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    float last_val = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / duration;

        // Dynamic downsampling factor (sweeps from 4 to 32 samples)
        int downsample_factor = 4 + (int)(28.0f * progress);

        if (i % downsample_factor == 0) {
            // Chaotic pitch sweep from 2000 Hz down to 300 Hz
            float freq = 2000.0f * expf(-4.0f * progress) + 300.0f;
            float base_phase = 2.0f * (float)M_PI * freq * t;
            
            // Mix sine wave with white noise
            float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
            float raw_signal = sinf(base_phase) * 0.6f + noise * 0.4f;

            // Quantize to 8 discrete amplitude levels (bitcrushed style)
            last_val = roundf(raw_signal * 3.0f) / 3.0f;
        }

        float amp = 14000.0f * (1.0f - progress);
        float combined = last_val * amp;

        if (combined > 32767.0f) combined = 32767.0f;
        if (combined < -32768.0f) combined = -32768.0f;

        buf[i] = (int16_t)combined;
    }

    *out_len = len * sizeof(int16_t);
    return buf;
}

#ifndef SAMPLE_RATE
#endif

/*
 * Rock Explosion Sound Generator
 * Generates low-pitched, heavy crumbling sounds with low-pass filtered noise and low frequency thuds.
 * - level: 0 for SM, 1 for MD, 2 for LG.
 * - out_len: pointer to write the output buffer length in bytes.
 */
static int16_t* generate_expl_rock(int level, int *out_len) {
    float duration, decay_rate, cutoff, thud_start_freq, thud_decay;
    
    // Set parameters based on the level of explosion
    if (level == 0) { // Small
        duration = 0.4f;
        decay_rate = 8.0f;
        cutoff = 350.0f;
        thud_start_freq = 130.0f;
        thud_decay = 15.0f;
    } else if (level == 1) { // Medium
        duration = 0.8f;
        decay_rate = 4.5f;
        cutoff = 220.0f;
        thud_start_freq = 90.0f;
        thud_decay = 8.0f;
    } else { // Large
        duration = 1.6f;
        decay_rate = 2.5f;
        cutoff = 130.0f;
        thud_start_freq = 65.0f;
        thud_decay = 4.0f;
    }

    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = (int16_t*)malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    // Single-pole cascade (12dB/octave low-pass filter) coefficient
    float alpha = 2.0f * (float)M_PI * cutoff / SAMPLE_RATE;
    if (alpha > 1.0f) alpha = 1.0f;
    
    float lp1 = 0.0f;
    float lp2 = 0.0f;
    float thud_phase = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        
        // 1. White Noise Generation
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

        // 2. 2-Pole Low-Pass Filter
        lp1 += alpha * (noise - lp1);
        lp2 += alpha * (lp1 - lp2);
        float filtered_noise = lp2;

        // 3. Crumble modulation (amplitude modulation with an LFO to create rock texture)
        // A 14Hz LFO modulated by a 3.7Hz envelope creates the uneven crumbling peaks
        float crumble_lfo = sinf(2.0f * (float)M_PI * 14.0f * t) * cosf(2.0f * (float)M_PI * 3.7f * t);
        float crumble_mod = 0.6f + 0.4f * crumble_lfo;

        // Apply decay envelope to noise
        float noise_amp = 18000.0f * expf(-decay_rate * t) * crumble_mod;
        float noise_signal = filtered_noise * noise_amp;

        // 4. Low frequency thud (rapidly decaying pitch sweep)
        float current_freq = thud_start_freq * expf(-thud_decay * t);
        thud_phase += 2.0f * (float)M_PI * current_freq / SAMPLE_RATE;
        float thud_amp = 14000.0f * expf(-thud_decay * t);
        float thud_signal = sinf(thud_phase) * thud_amp;

        // Mix the low-passed rumble and the thud
        float mixed = noise_signal * 0.65f + thud_signal * 0.35f;

        // Apply a quick fade-in/fade-out to prevent clicks
        float fade_in = (t < 0.005f) ? (t / 0.005f) : 1.0f;
        float fade_out = 1.0f;
        if (t > duration - 0.05f) {
            fade_out = (duration - t) / 0.05f;
        }
        mixed *= fade_in * fade_out;

        // Hard clamping to prevent overflow
        if (mixed > 32767.0f) mixed = 32767.0f;
        if (mixed < -32768.0f) mixed = -32768.0f;
        
        buf[i] = (int16_t)mixed;
    }

    *out_len = len * sizeof(int16_t);
    return buf;
}

/*
 * Metal Explosion Sound Generator
 * Generates clanging, metallic ring-out using inharmonic modes and a high-passed strike transient.
 * - level: 0 for SM, 1 for MD, 2 for LG.
 * - out_len: pointer to write the output buffer length in bytes.
 */
static int16_t* generate_expl_metal(int level, int *out_len) {
    float base_freq, duration, base_decay;
    if (level == 0) { // Small
        base_freq = 600.0f;
        duration = 0.6f;
        base_decay = 7.0f;
    } else if (level == 1) { // Medium
        base_freq = 320.0f;
        duration = 1.2f;
        base_decay = 3.5f;
    } else { // Large
        base_freq = 160.0f;
        duration = 2.4f;
        base_decay = 1.5f;
    }

    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = (int16_t*)malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    // Inharmonic modes of a thin metallic plate
    #define NUM_METAL_MODES 7
    float modes[NUM_METAL_MODES] = {1.0f, 1.27f, 1.50f, 1.87f, 2.31f, 3.25f, 4.10f};
    float phases[NUM_METAL_MODES] = {0.0f};
    float amplitudes[NUM_METAL_MODES] = {1.0f, 0.8f, 0.7f, 0.5f, 0.4f, 0.3f, 0.2f};

    float hp_state = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;

        // 1. Strike transient: high-passed noise with immediate decay
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float hp_signal = noise - hp_state;
        hp_state += 0.25f * hp_signal; // High-pass cut ~10kHz

        float strike_amp = 15000.0f * expf(-120.0f * t);
        float strike_val = hp_signal * strike_amp;

        // 2. Ringing modes sum
        float resonance_val = 0.0f;
        for (int k = 0; k < NUM_METAL_MODES; k++) {
            float mode_freq = base_freq * modes[k];
            // Advance phase for this mode
            phases[k] += 2.0f * (float)M_PI * mode_freq / SAMPLE_RATE;
            // Higher modes decay faster
            float mode_decay = base_decay * (1.0f + 0.4f * k);
            float mode_amp = amplitudes[k] * expf(-mode_decay * t);
            resonance_val += sinf(phases[k]) * mode_amp;
        }
        float res_signal = resonance_val * 12000.0f;

        float mixed = strike_val + res_signal;

        // Anti-click fade
        float fade_in = (t < 0.001f) ? (t / 0.001f) : 1.0f;
        float fade_out = 1.0f;
        if (t > duration - 0.05f) {
            fade_out = (duration - t) / 0.05f;
        }
        mixed *= fade_in * fade_out;

        // Clamp
        if (mixed > 32767.0f) mixed = 32767.0f;
        if (mixed < -32768.0f) mixed = -32768.0f;

        buf[i] = (int16_t)mixed;
    }

    *out_len = len * sizeof(int16_t);
    return buf;
}

/*
 * Ice Explosion Sound Generator
 * Generates crisp, crackling high-frequency shattering using high-passed noise and randomized crackles.
 * - level: 0 for SM, 1 for MD, 2 for LG.
 * - out_len: pointer to write the output buffer length in bytes.
 */
typedef struct {
    float phase;
    float freq;
    float amp;
    float decay;
    int active;
} IceCrackle;

static int16_t* generate_expl_ice(int level, int *out_len) {
    float duration, noise_decay, hp_cutoff, base_prob, decay_prob;
    if (level == 0) { // Small
        duration = 0.4f;
        noise_decay = 8.0f;
        hp_cutoff = 3000.0f;
        base_prob = 0.15f;
        decay_prob = 6.0f;
    } else if (level == 1) { // Medium
        duration = 0.8f;
        noise_decay = 5.0f;
        hp_cutoff = 2400.0f;
        base_prob = 0.25f;
        decay_prob = 3.5f;
    } else { // Large
        duration = 1.6f;
        noise_decay = 3.0f;
        hp_cutoff = 1800.0f;
        base_prob = 0.35f;
        decay_prob = 2.0f;
    }

    int len = (int)(SAMPLE_RATE * duration);
    int16_t *buf = (int16_t*)malloc(len * sizeof(int16_t));
    if (!buf) {
        *out_len = 0;
        return NULL;
    }

    // High pass filter coefficient (1-pole)
    float alpha = 2.0f * (float)M_PI * hp_cutoff / SAMPLE_RATE;
    if (alpha > 1.0f) alpha = 1.0f;

    float hp1_state = 0.0f;
    float hp2_state = 0.0f;

    // Local array for active crackle grains
    #define MAX_CRACKLES 16
    IceCrackle crackles[MAX_CRACKLES];
    for (int c = 0; c < MAX_CRACKLES; c++) {
        crackles[c].active = 0;
    }

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;

        // 1. Dynamic Crackle Grain Triggering
        // Check every 100 samples (~2.27ms) to see if a crackle is spawned
        if (i % 100 == 0) {
            float prob = base_prob * expf(-decay_prob * t);
            float r = (float)rand() / RAND_MAX;
            if (r < prob) {
                // Find first free slot
                for (int c = 0; c < MAX_CRACKLES; c++) {
                    if (!crackles[c].active) {
                        crackles[c].active = 1;
                        crackles[c].phase = 0.0f;
                        // High pitch ice shards: 2500Hz to 9000Hz
                        crackles[c].freq = 2500.0f + 6500.0f * ((float)rand() / RAND_MAX);
                        // Randomized initial volume
                        crackles[c].amp = 4000.0f + 6000.0f * ((float)rand() / RAND_MAX);
                        // Very fast exponential decay rate (seconds^-1)
                        crackles[c].decay = 150.0f + 300.0f * ((float)rand() / RAND_MAX);
                        break;
                    }
                }
            }
        }

        // 2. Accumulate active crackles
        float crackle_sum = 0.0f;
        for (int c = 0; c < MAX_CRACKLES; c++) {
            if (crackles[c].active) {
                crackles[c].phase += 2.0f * (float)M_PI * crackles[c].freq / SAMPLE_RATE;
                float c_val = sinf(crackles[c].phase) * crackles[c].amp;
                crackle_sum += c_val;
                // Decay the grain amplitude exponentially
                crackles[c].amp *= expf(-crackles[c].decay / SAMPLE_RATE);
                if (crackles[c].amp < 1.0f) {
                    crackles[c].active = 0;
                }
            }
        }

        // 3. Crisp Noise Generation: 2-Pole High-Pass Filtered White Noise
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float hp1 = noise - hp1_state;
        hp1_state += alpha * hp1;
        float hp2 = hp1 - hp2_state;
        hp2_state += alpha * hp2;

        float noise_amp = 14000.0f * expf(-noise_decay * t);
        float noise_val = hp2 * noise_amp;

        // 4. Mix noise and crackles
        float mixed = noise_val + crackle_sum;

        // Fade out
        float fade_in = (t < 0.002f) ? (t / 0.002f) : 1.0f;
        float fade_out = 1.0f;
        if (t > duration - 0.05f) {
            fade_out = (duration - t) / 0.05f;
        }
        mixed *= fade_in * fade_out;

        // Clamp
        if (mixed > 32767.0f) mixed = 32767.0f;
        if (mixed < -32768.0f) mixed = -32768.0f;

        buf[i] = (int16_t)mixed;
    }

    *out_len = len * sizeof(int16_t);
    return buf;
}

/* Global external variables defined elsewhere in the game */
extern int settings_volume;
extern int settings_music_vol;
extern int settings_sfx_vol;
extern int settings_dynamic_range;
extern int settings_mute_unfocused;
extern SDL_Window *g_window;

/* SynthState structure to hold active synthesizer parameters */
typedef struct {
    double time;             // overall music time in seconds
    float menu_fade;         // 1.0 in menu, 0.0 in game
    float play_fade;         // 1.0 in game, 0.0 in menu
    float combat_level;      // 0.0 (quiet) to 1.0 (intense)
    float spookiness;        // 0.0 (near home) to 1.0 (abyss)
    float paused_fraction;   // 0.0 (playing) to 1.0 (paused)
    float lpf_state;         // low-pass filter state
} SynthState;

static SynthState synth_state = {
    .time = 0.0,
    .menu_fade = 1.0f,
    .play_fade = 0.0f,
    .combat_level = 0.0f,
    .spookiness = 0.0f,
    .paused_fraction = 0.0f,
    .lpf_state = 0.0f
};

/* Global mute state tracker */
static int audio_is_muted = 0;

/* Chord pad base frequencies: Am, F, Dm, E */
static const float pad_freqs[4][3] = {
    {220.0f, 261.63f, 329.63f}, // Am (A3, C4, E4)
    {174.61f, 220.0f, 261.63f}, // F  (F3, A3, C4)
    {146.83f, 174.61f, 220.0f}, // Dm (D3, F3, A3)
    {164.81f, 207.65f, 246.94f}  // E  (E3, G#3, B3)
};

/* --- Synthesizer helper functions --- */

/**
 * Renders a mellow pad sound for a specific chord index and time t.
 * Employs additive synthesis with sub-octaves for depth.
 */
static float eval_chord(int idx, double t) {
    float out = 0.0f;
    for (int i = 0; i < 3; i++) {
        double freq = (double)pad_freqs[idx][i];
        double phase = 2.0 * M_PI * freq * t;
        // Combine fundamental, sub-octave, and mild second harmonic
        out += (float)(sin(phase) + 0.4 * sin(phase * 0.5) + 0.2 * sin(phase * 2.0));
    }
    return out * 0.25f; // Normalise to prevent clipping
}

/**
 * Chord pads blending smoothly between the chords in pad_freqs.
 */
static float eval_pad(double t) {
    double chord_dur = 8.0;
    double trans_dur = 2.0;
    double cycle = 4.0 * chord_dur;
    double t_mod = fmod(t, cycle);
    int idx1 = (int)(t_mod / chord_dur) % 4;
    int idx2 = (idx1 + 1) % 4;
    double local_t = fmod(t_mod, chord_dur);
    
    float blend = 0.0f;
    if (local_t > (chord_dur - trans_dur)) {
        double trans_t = (local_t - (chord_dur - trans_dur)) / trans_dur;
        // Smoothstep interpolation
        blend = (float)(trans_t * trans_t * (3.0 - 2.0 * trans_t));
    }
    
    float val1 = eval_chord(idx1, t);
    float val2 = eval_chord(idx2, t);
    return (1.0f - blend) * val1 + blend * val2;
}

/**
 * Helper to evaluate a single voice of the menu arpeggiator with decay envelope.
 */
static float eval_arp_voice(double t, int step_offset, float attenuation) {
    double note_t = t - (double)step_offset * 0.25;
    if (note_t < 0.0) return 0.0f;
    
    int total_step = (int)(note_t / 0.25);
    int step = total_step % 8;
    int chord_idx = (int)(note_t / 8.0) % 4;
    
    float base_freq = 0.0f;
    float scale_factor = 1.0f;
    
    // Arpeggio pattern over the 3 chord notes
    switch (step) {
        case 0: base_freq = pad_freqs[chord_idx][0]; scale_factor = 1.0f; break;
        case 1: base_freq = pad_freqs[chord_idx][1]; scale_factor = 1.0f; break;
        case 2: base_freq = pad_freqs[chord_idx][2]; scale_factor = 1.0f; break;
        case 3: base_freq = pad_freqs[chord_idx][1]; scale_factor = 1.0f; break;
        case 4: base_freq = pad_freqs[chord_idx][0]; scale_factor = 2.0f; break;
        case 5: base_freq = pad_freqs[chord_idx][1]; scale_factor = 2.0f; break;
        case 6: base_freq = pad_freqs[chord_idx][2]; scale_factor = 1.0f; break;
        case 7: base_freq = pad_freqs[chord_idx][1]; scale_factor = 2.0f; break;
    }
    
    float freq = base_freq * scale_factor;
    double step_t = fmod(note_t, 0.25);
    
    // Exponential decay envelope
    float env = 0.0f;
    if (step_t < 0.01) {
        env = (float)(step_t / 0.01);
    } else {
        env = expf(-8.0f * (float)(step_t - 0.01));
    }
    
    double phase = 2.0 * M_PI * (double)freq * step_t;
    float wave = (float)(sin(phase) + 0.25 * sin(phase * 2.0));
    
    return wave * env * attenuation;
}

/**
 * Calm, slower arpeggio for menu screens, scaled by menu_fade.
 */
static float eval_menu_arp(double t) {
    if (synth_state.menu_fade <= 0.01f) return 0.0f;
    
    // Play arpeggio with delay feedback taps for spacey echo
    float voice1 = eval_arp_voice(t, 0, 1.0f);
    float voice2 = eval_arp_voice(t, 1, 0.40f);
    float voice3 = eval_arp_voice(t, 2, 0.15f);
    
    float mix = (voice1 + voice2 + voice3) * 0.22f;
    return mix * synth_state.menu_fade;
}

/**
 * Dark, deep ambient drone for the gameplay mode, growing spookier as distance increases.
 */
static float eval_play_drone(double t, float spookiness) {
    // Deep fundamental tone (A1 = 55Hz) with subtle detuned chorus
    double phase1 = 2.0 * M_PI * 55.0 * t;
    double phase2 = 2.0 * M_PI * 55.3 * t; 
    double phase3 = 2.0 * M_PI * 110.0 * t;
    
    float base_drone = (float)(sin(phase1) + 0.7 * sin(phase2) + 0.3 * sin(phase3));
    
    // Detuned tritonal and minor second components fade in with spookiness
    if (spookiness > 0.01f) {
        double spooky_phase1 = 2.0 * M_PI * 77.77 * t; // Tritone (detuned)
        double spooky_phase2 = 2.0 * M_PI * 58.27 * t; // Minor second (detuned)
        
        // Slow LFO to pulse the dissonance
        float lfo = (float)(0.5 + 0.5 * sin(2.0 * M_PI * 0.15 * t));
        float spooky_drone = (float)(sin(spooky_phase1) + sin(spooky_phase2)) * lfo;
        
        base_drone = (1.0f - spookiness * 0.5f) * base_drone + spookiness * 0.8f * spooky_drone;
    }
    
    return base_drone * 0.28f;
}

/**
 * Slow glides that scale with spookiness.
 * Utilises stateless closed-form integration of frequency sweep equations.
 */
static float eval_play_spooky_glides(double t, float spookiness) {
    if (spookiness <= 0.01f) return 0.0f;
    
    // Slide 1: 150Hz base, sweeps +/- 70Hz every 20s
    double f0_1 = 150.0;
    double A1 = 70.0;
    double flfo1 = 0.05;
    double phase1 = 2.0 * M_PI * f0_1 * t - (A1 / flfo1) * cos(2.0 * M_PI * flfo1 * t);
    
    // Slide 2: 250Hz base, sweeps +/- 120Hz every ~33s
    double f0_2 = 250.0;
    double A2 = 120.0;
    double flfo2 = 0.03;
    double phase2 = 2.0 * M_PI * f0_2 * t - (A2 / flfo2) * sin(2.0 * M_PI * flfo2 * t);
    
    // Slide 3: High-pitched whistling wind, sweeps +/- 300Hz every 50s
    double f0_3 = 800.0;
    double A3 = 300.0;
    double flfo3 = 0.02;
    double phase3 = 2.0 * M_PI * f0_3 * t - (A3 / flfo3) * cos(2.0 * M_PI * flfo3 * t);
    float lfo_wind = (float)(0.3 + 0.3 * sin(2.0 * M_PI * 0.1 * t));
    
    float wave1 = (float)sin(phase1);
    float wave2 = (float)sin(phase2);
    float wave3 = (float)sin(phase3) * lfo_wind;
    
    float mix = (wave1 + wave2 * 0.8f + wave3 * 0.3f) * 0.18f;
    return mix * spookiness;
}

/**
 * Helper to evaluate a single voice of the driving combat arpeggiator.
 */
static float eval_combat_voice(double t, int step_offset, float attenuation) {
    double note_t = t - (double)step_offset * 0.12;
    if (note_t < 0.0) return 0.0f;
    
    int total_step = (int)(note_t / 0.12);
    int step = total_step % 8;
    int chord_idx = (int)(note_t / 8.0) % 4;
    
    float base_freq = 0.0f;
    float scale_factor = 1.0f;
    
    // Driving minor/dissonant progression based on current chord notes
    switch (step) {
        case 0: base_freq = pad_freqs[chord_idx][0]; scale_factor = 0.5f; break; // Low root
        case 1: base_freq = pad_freqs[chord_idx][0]; scale_factor = 0.5f; break;
        case 2: base_freq = pad_freqs[chord_idx][1]; scale_factor = 0.5f; break;
        case 3: base_freq = pad_freqs[chord_idx][0]; scale_factor = 0.5f; break;
        case 4: base_freq = pad_freqs[chord_idx][2]; scale_factor = 1.0f; break;
        case 5: base_freq = pad_freqs[chord_idx][1]; scale_factor = 1.0f; break;
        case 6: base_freq = pad_freqs[chord_idx][2]; scale_factor = 1.0f; break;
        case 7: base_freq = pad_freqs[chord_idx][0]; scale_factor = 1.0f; break;
    }
    
    float freq = base_freq * scale_factor;
    double step_t = fmod(note_t, 0.12);
    
    // Aggressive snappy envelope
    float env = 0.0f;
    if (step_t < 0.005) {
        env = (float)(step_t / 0.005);
    } else {
        env = expf(-18.0f * (float)(step_t - 0.005));
    }
    
    double phase = 2.0 * M_PI * (double)freq * step_t;
    // Approximated sawtooth wave for bright aggressive character
    float wave = (float)(sin(phase) + 0.5 * sin(phase * 2.0) + 0.25 * sin(phase * 4.0));
    
    return wave * env * attenuation;
}

/**
 * Intense combat arpeggio that scales with combat_level.
 */
static float eval_play_combat_arp(double t, float combat_level) {
    if (combat_level <= 0.01f) return 0.0f;
    
    float voice1 = eval_combat_voice(t, 0, 1.0f);
    float voice2 = eval_combat_voice(t, 1, 0.35f);
    float voice3 = eval_combat_voice(t, 2, 0.15f);
    
    float mix = (voice1 + voice2 + voice3) * 0.18f;
    return mix * combat_level;
}

/**
 * Post-mix callback function registered with SDL_mixer.
 * Fades music signals, mixes them with SFX, and applies filters.
 */
static void mix_music_callback(void *udata, Uint8 *stream, int len) {
    (void)udata;
    if (!stream || len <= 0) return;
    
    int16_t *buf = (int16_t *)stream;
    int num_samples = len / (int)sizeof(int16_t);
    
    // Check window input focus if requested by settings
    int focused = 1;
    if (settings_mute_unfocused && g_window) {
        Uint32 flags = SDL_GetWindowFlags(g_window);
        if (!(flags & SDL_WINDOW_INPUT_FOCUS)) {
            focused = 0;
        }
    }
    
    // Master scaling factor to apply mute policies
    float master_scale = (audio_is_muted || !focused) ? 0.0f : 1.0f;
    
    // Compute volume coefficients
    float music_scale = (settings_volume / 100.0f) * (settings_music_vol / 100.0f) * master_scale;
    float sfx_scale = (settings_sfx_vol / 100.0f) * master_scale;
    
    double dt_sample = 1.0 / (double)SAMPLE_RATE;
    
    for (int i = 0; i < num_samples; i++) {
        // Retrieve and scale incoming SFX sample
        float sfx_val = (float)buf[i] * sfx_scale;
        
        double t = synth_state.time;
        
        // Generate music tracks
        float menu_arp_val = eval_menu_arp(t);
        float pad_val = eval_pad(t);
        
        float play_drone_val = eval_play_drone(t, synth_state.spookiness);
        float play_glides_val = eval_play_spooky_glides(t, synth_state.spookiness);
        float play_combat_val = eval_play_combat_arp(t, synth_state.combat_level);
        
        float menu_music = menu_arp_val + pad_val * 0.7f;
        float play_music = play_drone_val + play_glides_val + play_combat_val;
        
        // Crossfade menu and gameplay music
        float music_sample = menu_music * synth_state.menu_fade + play_music * synth_state.play_fade;
        
        // Convert to 16-bit range level and apply volume scaling
        float music_amplitude = music_sample * 8000.0f * music_scale;
        
        // Low-pass filter (LPF) updates dynamically when paused
        float alpha = 1.0f - 0.93f * synth_state.paused_fraction;
        synth_state.lpf_state += alpha * (music_amplitude - synth_state.lpf_state);
        float filtered_music = synth_state.lpf_state;
        
        // Add music signal onto the existing SFX
        float mixed_val = sfx_val + filtered_music;
        
        // Damp final output amplitude when paused (2x quieter)
        float damp_factor = 1.0f - 0.5f * synth_state.paused_fraction;
        mixed_val *= damp_factor;
        
        // Apply soft dynamic range compression
        if (settings_dynamic_range) {
            float abs_val = fabsf(mixed_val);
            float threshold = 16000.0f;
            if (abs_val > threshold) {
                float excess = abs_val - threshold;
                float compressed_abs = threshold + excess * 0.25f; // 4:1 compression ratio
                mixed_val = (mixed_val < 0.0f) ? -compressed_abs : compressed_abs;
            }
        }
        
        // Hard clipping/clamping
        if (mixed_val > 32767.0f) mixed_val = 32767.0f;
        if (mixed_val < -32768.0f) mixed_val = -32768.0f;
        
        buf[i] = (int16_t)mixed_val;
        
        // Advance clock
        synth_state.time += dt_sample;
    }
}

/* --- Public interface functions --- */

/**
 * Updates SynthState parameters using targets and delta time.
 * Automatically registers the post-mix callback if needed.
 */
void audio_set_music_params(float combat, float spookiness, int paused, int gameplay, float dt) {
    static int post_mix_registered = 0;
    if (!post_mix_registered) {
        Mix_SetPostMix(mix_music_callback, NULL);
        post_mix_registered = 1;
    }
    
    // Interpolate menu_fade (1.0 in menu, 0.0 in game)
    float menu_target = gameplay ? 0.0f : 1.0f;
    if (synth_state.menu_fade < menu_target) {
        synth_state.menu_fade += 2.0f * dt;
        if (synth_state.menu_fade > menu_target) synth_state.menu_fade = menu_target;
    } else if (synth_state.menu_fade > menu_target) {
        synth_state.menu_fade -= 2.0f * dt;
        if (synth_state.menu_fade < menu_target) synth_state.menu_fade = menu_target;
    }
    
    // Interpolate play_fade (1.0 in game, 0.0 in menu)
    float play_target = gameplay ? 1.0f : 0.0f;
    if (synth_state.play_fade < play_target) {
        synth_state.play_fade += 2.0f * dt;
        if (synth_state.play_fade > play_target) synth_state.play_fade = play_target;
    } else if (synth_state.play_fade > play_target) {
        synth_state.play_fade -= 2.0f * dt;
        if (synth_state.play_fade < play_target) synth_state.play_fade = play_target;
    }
    
    // Interpolate combat_level
    if (synth_state.combat_level < combat) {
        synth_state.combat_level += 1.0f * dt;
        if (synth_state.combat_level > combat) synth_state.combat_level = combat;
    } else if (synth_state.combat_level > combat) {
        synth_state.combat_level -= 1.0f * dt;
        if (synth_state.combat_level < combat) synth_state.combat_level = combat;
    }
    
    // Interpolate spookiness
    if (synth_state.spookiness < spookiness) {
        synth_state.spookiness += 0.5f * dt;
        if (synth_state.spookiness > spookiness) synth_state.spookiness = spookiness;
    } else if (synth_state.spookiness > spookiness) {
        synth_state.spookiness -= 0.5f * dt;
        if (synth_state.spookiness < spookiness) synth_state.spookiness = spookiness;
    }
    
    // Interpolate paused_fraction
    float paused_target = paused ? 1.0f : 0.0f;
    if (synth_state.paused_fraction < paused_target) {
        synth_state.paused_fraction += 3.0f * dt;
        if (synth_state.paused_fraction > paused_target) synth_state.paused_fraction = paused_target;
    } else if (synth_state.paused_fraction > paused_target) {
        synth_state.paused_fraction -= 3.0f * dt;
        if (synth_state.paused_fraction < paused_target) synth_state.paused_fraction = paused_target;
    }
}

/**
 * Updates the master volume percentage and propagates to SDL_mixer.
 */
void audio_set_volume(int volume_percent) {
    if (volume_percent < 0) volume_percent = 0;
    if (volume_percent > 100) volume_percent = 100;
    settings_volume = volume_percent;
    Mix_Volume(-1, (volume_percent * MIX_MAX_VOLUME) / 100);
}

/**
 * Re-applies current global settings volume to standard audio output.
 */
void audio_update_volumes(void) {
    audio_set_volume(settings_volume);
}

/**
 * Sets the global music and sound effect mute state.
 */
void audio_mute(int muted) {
    audio_is_muted = muted;
    if (muted) {
        Mix_Volume(-1, 0);
    } else {
        Mix_Volume(-1, (settings_volume * MIX_MAX_VOLUME) / 100);
    }
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

    // Advanced UFO sounds
    audio_buffers[SFX_TURRET_FIRE] = generate_turret_fire(&size);
    mix_chunks[SFX_TURRET_FIRE] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_TURRET_FIRE], size);

    audio_buffers[SFX_UFO_FIRE_LG] = generate_ufo_fire_lg(&size);
    mix_chunks[SFX_UFO_FIRE_LG] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_UFO_FIRE_LG], size);

    audio_buffers[SFX_UFO_FIRE_SM] = generate_ufo_fire_sm(&size);
    mix_chunks[SFX_UFO_FIRE_SM] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_UFO_FIRE_SM], size);

    audio_buffers[SFX_UFO_FIRE_KAMIKAZE] = generate_ufo_fire_kamikaze(&size);
    mix_chunks[SFX_UFO_FIRE_KAMIKAZE] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_UFO_FIRE_KAMIKAZE], size);

    audio_buffers[SFX_UFO_FIRE_BOMBER] = generate_ufo_fire_bomber(&size);
    mix_chunks[SFX_UFO_FIRE_BOMBER] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_UFO_FIRE_BOMBER], size);

    audio_buffers[SFX_UFO_FIRE_EYE] = generate_ufo_fire_eye(&size);
    mix_chunks[SFX_UFO_FIRE_EYE] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_UFO_FIRE_EYE], size);

    audio_buffers[SFX_UFO_FIRE_ELDRITCH] = generate_ufo_fire_eldritch(&size);
    mix_chunks[SFX_UFO_FIRE_ELDRITCH] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_UFO_FIRE_ELDRITCH], size);

    audio_buffers[SFX_UFO_FIRE_DAEMON] = generate_ufo_fire_daemon(&size);
    mix_chunks[SFX_UFO_FIRE_DAEMON] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_UFO_FIRE_DAEMON], size);

    // Debris sounds
    audio_buffers[SFX_EXPL_ROCK_LG] = generate_expl_rock(2, &size);
    mix_chunks[SFX_EXPL_ROCK_LG] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_EXPL_ROCK_LG], size);

    audio_buffers[SFX_EXPL_ROCK_MD] = generate_expl_rock(1, &size);
    mix_chunks[SFX_EXPL_ROCK_MD] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_EXPL_ROCK_MD], size);

    audio_buffers[SFX_EXPL_ROCK_SM] = generate_expl_rock(0, &size);
    mix_chunks[SFX_EXPL_ROCK_SM] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_EXPL_ROCK_SM], size);

    audio_buffers[SFX_EXPL_METAL_LG] = generate_expl_metal(2, &size);
    mix_chunks[SFX_EXPL_METAL_LG] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_EXPL_METAL_LG], size);

    audio_buffers[SFX_EXPL_METAL_MD] = generate_expl_metal(1, &size);
    mix_chunks[SFX_EXPL_METAL_MD] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_EXPL_METAL_MD], size);

    audio_buffers[SFX_EXPL_METAL_SM] = generate_expl_metal(0, &size);
    mix_chunks[SFX_EXPL_METAL_SM] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_EXPL_METAL_SM], size);

    audio_buffers[SFX_EXPL_ICE_LG] = generate_expl_ice(2, &size);
    mix_chunks[SFX_EXPL_ICE_LG] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_EXPL_ICE_LG], size);

    audio_buffers[SFX_EXPL_ICE_MD] = generate_expl_ice(1, &size);
    mix_chunks[SFX_EXPL_ICE_MD] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_EXPL_ICE_MD], size);

    audio_buffers[SFX_EXPL_ICE_SM] = generate_expl_ice(0, &size);
    mix_chunks[SFX_EXPL_ICE_SM] = Mix_QuickLoad_RAW((Uint8*)audio_buffers[SFX_EXPL_ICE_SM], size);

    // Register synth callback
    Mix_SetPostMix(mix_music_callback, NULL);

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

void audio_cleanup() {
    Mix_SetPostMix(NULL, NULL);
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
