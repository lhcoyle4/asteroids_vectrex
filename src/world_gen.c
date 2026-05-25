#include "world_gen.h"
#include "game.h"
#include "audio.h"
#include "entities.h"
#include "state.h"
#include <math.h>
#include <stdlib.h>

extern Vec2 calculate_external_forces(Vec2 pos);
extern float distance_sq(Vec2 p1, Vec2 p2);
extern void spawn_ufo(void);

/** @brief Generates a randomized polygon shape for an asteroid.
 *  Creates 8-12 vertices in a rough circle with random radial perturbation
 *  to give each asteroid a unique silhouette. The Autodyne's pilots learn
 *  to read these shapes like runes. */
void init_asteroid_shape(AsteroidEntity *ast, int size)
{
    ast->size   = size;
    ast->radius = (size == 3) ? 35.0f : ((size == 2) ? 18.0f : 9.0f);

    int num_points  = 8 + rand() % 5; /* 8 to 12 vertices */
    ast->line_count = num_points;

    Vec2 points[16];
    for (int i = 0; i < num_points; i++) {
        float angle = i * 2.0f * (float)M_PI / num_points;
        float r     = ast->radius * (0.8f + 0.4f * ((float)rand() / RAND_MAX));
        points[i].x = r * cosf(angle);
        points[i].y = r * sinf(angle);
    }

    for (int i = 0; i < num_points; i++) {
        ast->lines[i].p1 = points[i];
        ast->lines[i].p2 = points[(i + 1) % num_points];
    }
}

/** @brief Finds a free slot in the asteroid pool and initializes it.
 *  Asteroids are the Void Stones that fill the drift lanes between zones.
 *  Higher levels increase the chance of shielded (metal) and crystalline variants.
 *  Spawned ASTEROID_SPAWN_RING_MIN to ASTEROID_SPAWN_RING_MAX units from a given position. */
void spawn_asteroid(Vec2 pos, int size)
{
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) {
            asteroids[i].active = 1;
            asteroids[i].pos    = pos;

            float angle       = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
            float speed_scale = 1.0f + (difficulty * 0.3f);
            float speed       = (30.0f + 50.0f * (4 - size) + 10.0f * level) * speed_scale;
            asteroids[i].vel.x    = cosf(angle) * speed;
            asteroids[i].vel.y    = sinf(angle) * speed;
            asteroids[i].angle    = angle;
            asteroids[i].rot_speed =
                (-1.5f + 3.0f * ((float)rand() / RAND_MAX)) * speed_scale;
            init_asteroid_shape(&asteroids[i], size);
            asteroids[i].has_shield =
                (level >= 3 && rand() % 100 < (15 + difficulty * 10));
            break;
        }
    }
}

/** @brief Spawns a chronicle orb (XP collectible) at the given position.
 *  Chronicle orbs encode fragments of the destroyed vessel's memory. */
void spawn_orb(Vec2 pos, int value)
{
    for (int i = 0; i < MAX_ORBS; i++) {
        if (!orbs[i].active) {
            orbs[i].active = 1;
            orbs[i].pos    = pos;
            float angle    = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
            float speed    = 20.0f + 30.0f * ((float)rand() / RAND_MAX);
            orbs[i].vel.x  = cosf(angle) * speed;
            orbs[i].vel.y  = sinf(angle) * speed;
            orbs[i].value  = value;
            orbs[i].life   = 10.0f;
            break;
        }
    }
}

/** @brief Updates asteroid physics, rotation, trail, and repositioning.
 *
 *  Each active asteroid is integrated with external forces and its rotation
 *  advanced.  Asteroids that drift beyond 3500 u from the player are
 *  teleported into a fresh spawn ring (2000-2500 u) to maintain density.
 *  If the active count falls below the target, a new large asteroid is
 *  spawned off-screen.  The Anomalous Void Rift is activated when the
 *  player reaches level 4.
 */
void update_asteroids(float dt)
{
    int active_count = 0;

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) continue;
        active_count++;

        /* Trail recording before position update */
        asteroids[i].trail_pos[asteroids[i].trail_head] = asteroids[i].pos;
        asteroids[i].trail_ang[asteroids[i].trail_head] = asteroids[i].angle;
        asteroids[i].trail_head = (asteroids[i].trail_head + 1) % PHOS_TRAIL_LEN;

        asteroids[i].angle += asteroids[i].rot_speed * dt;

        /* External force integration (rift gravity, etc.) */
        Vec2 ext_f = calculate_external_forces(asteroids[i].pos);
        asteroids[i].vel.x += ext_f.x * dt;
        asteroids[i].vel.y += ext_f.y * dt;
        asteroids[i].pos.x += asteroids[i].vel.x * dt;
        asteroids[i].pos.y += asteroids[i].vel.y * dt;

        /* Teleport back into the spawn ring if too far from player */
        if (player.active) {
            float d2 = distance_sq(asteroids[i].pos, player.pos);
            if (d2 > 3500.0f * 3500.0f) {
                float ang  = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
                float dist = UFO_SPAWN_RING_MIN
                             + ((float)rand() / RAND_MAX)
                               * (UFO_SPAWN_RING_MAX - UFO_SPAWN_RING_MIN);
                asteroids[i].pos.x = player.pos.x + cosf(ang) * dist;
                asteroids[i].pos.y = player.pos.y + sinf(ang) * dist;
                /* Reset trail to avoid streak artefacts across the void */
                for (int t = 0; t < PHOS_TRAIL_LEN; t++)
                    asteroids[i].trail_pos[t] = asteroids[i].pos;
            }
        }
    }

    /* Maintain target asteroid density based on level and difficulty */
    int target = 12 + player_level * 3 + (difficulty * 3);
    if (target > MAX_ASTEROIDS - 4) target = MAX_ASTEROIDS - 4;

    if (player.active && active_count < target) {
        float ang  = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
        float dist = UFO_SPAWN_RING_MIN
                     + ((float)rand() / RAND_MAX)
                       * (UFO_SPAWN_RING_MAX - UFO_SPAWN_RING_MIN);
        Vec2 pos = {
            player.pos.x + cosf(ang) * dist,
            player.pos.y + sinf(ang) * dist
        };
        spawn_asteroid(pos, 3);
    }

    /* Cache active count for the beat system (read in update_spawning) */
    cached_active_asteroids = active_count;
}

/** @brief Manages spawn timers for UFOs, rifts, and the beat-pulse system.
 *
 *  When the UFO is inactive its spawn timer counts down; spawn_ufo() is
 *  called when it expires.  The Anomalous Void Rift is activated once the
 *  player reaches level 4.  The beat system drives the tempo-based "thump"
 *  sound that accelerates as fewer Void Stones remain on screen.
 */
void update_spawning(float dt)
{
    /* UFO spawn timer */
    if (!ufo.active) {
        ufo_spawn_timer -= dt;
        if (ufo_spawn_timer <= 0.0f)
            spawn_ufo();
    }

    /* Activate the Anomalous Void Rift at player level 4+ */
    if (player.active && player_level >= 4 && !rift.active) {
        rift.active = 1;
        float ang  = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
        float dist = 850.0f + ((float)rand() / RAND_MAX) * 250.0f;
        rift.pos.x      = player.pos.x + cosf(ang) * dist;
        rift.pos.y      = player.pos.y + sinf(ang) * dist;
        rift.radius     = 35.0f;
        rift.angle1     = 0.0f;
        rift.angle2     = 0.0f;
        rift.pulse_timer = 0.0f;
        rift.spawn_timer = 5.0f;
    }

    /*
     * Sound beat system: a two-tone pulse whose period compresses as the
     * number of active Void Stones falls.  speed_mult ∈ [0.2, 1.0] maps
     * to a delay of [0.25 s, 1.0 s].
     * cached_active_asteroids is written by update_asteroids() earlier
     * this frame.
     */
    if (cached_active_asteroids > 0) {
        float speed_mult = (float)cached_active_asteroids / (float)(4 + level);
        if (speed_mult < 0.2f) speed_mult = 0.2f;
        if (speed_mult > 1.0f) speed_mult = 1.0f;
        float beat_delay = 0.25f + 0.75f * speed_mult;

        beat_timer -= dt;
        if (beat_timer <= 0.0f) {
            cur_beat   = 1 - cur_beat;
            audio_play(cur_beat ? SFX_BEAT1 : SFX_BEAT2);
            beat_timer = beat_delay;
        }
    }
}

