#include "enemies.h"
#include "game.h"
#include "entities.h"
#include "state.h"
#include "audio.h"
#include "game_data.h"
#include <math.h>
#include <stdlib.h>

/** @brief Spawns a friendly drone NPC at the given position.
 *  These sentinels of the Home Station will orbit the Autodyne and warn of danger. */
void spawn_npc(Vec2 pos)
{
    for (int i = 0; i < MAX_NPC; i++) {
        if (!npcs[i].active) {
            npcs[i].active       = 1;
            npcs[i].following    = 0;
            npcs[i].pos          = pos;
            npcs[i].vel          = (Vec2){0.0f, 0.0f};
            npcs[i].angle        = 0.0f;
            npcs[i].radius       = 10.0f;
            npcs[i].orbit_angle  =
                ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
            npcs[i].contact_timer = 0.0f;
            return;
        }
    }
}

/** @brief Selects and spawns an enemy vessel based on the current zone and drift level.
 *  Enemy types escalate with zone depth:
 *    Sizes 1-2: Saucers (Inner Belt and beyond)
 *    Size 3:    Vector Stalker  — kamikaze interceptor with predictive AI
 *    Size 4:    Boundary Weaver — sinusoidal sweep bomber
 *    Size 5:    Eye of the Void — gravitational harvester, pulls all matter
 *    Size 6:    Eldritch Tendril (Zone 2+) — drift-sinusoidal pursuer
 *    Size 7:    Daemon Sigil    (Zone 3+) — teleport-lurching horror
 *  Spawns UFO_SPAWN_RING_MIN to UFO_SPAWN_RING_MAX units from the Autodyne. */
void spawn_ufo(void)
{
    ufo.active = 1;

    /* Zone + level based enemy type selection */
    int r = rand() % 100;
    if (player_zone >= 3 && r < 15) {
        ufo.size   = 7;     /* Daemon Sigil (Zone 3+) */
        ufo.radius = 22.0f;
    } else if (player_zone >= 2 && r < 30) {
        ufo.size   = 6;     /* Eldritch Tendril (Zone 2+) */
        ufo.radius = 18.0f;
    } else if (level >= 4 && r < 45) {
        ufo.size   = 5;     /* Eye of the Void */
        ufo.radius = 24.0f;
    } else if (level >= 3 && r < 60) {
        ufo.size   = 4;     /* Boundary Weaver */
        ufo.radius = 20.0f;
    } else if (level >= 2 && r < 75) {
        ufo.size   = 3;     /* Vector Stalker */
        ufo.radius = 12.0f;
    } else {
        ufo.size   = (rand() % 100 < 40 + level * 5) ? 1 : 2;
        ufo.radius = (ufo.size == 2) ? 16.0f : 8.0f;
    }

    /* Spawn UFO_SPAWN_RING_MIN to UFO_SPAWN_RING_MAX units from the Autodyne */
    float angle = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
    float dist  = UFO_SPAWN_RING_MIN
                  + ((float)rand() / RAND_MAX) * (UFO_SPAWN_RING_MAX - UFO_SPAWN_RING_MIN);
    ufo.pos.x = player.pos.x + cosf(angle) * dist;
    ufo.pos.y = player.pos.y + sinf(angle) * dist;

    /* Set initial velocity generally towards the Autodyne */
    float base_speed = 100.0f + 25.0f * level;
    ufo.vel.x = (ufo.pos.x < player.pos.x) ? base_speed : -base_speed;
    ufo.target_y        = ufo.pos.y;
    ufo.vel.y           = 0.0f;
    ufo.fire_timer      = 1.0f;
    ufo.change_dir_timer = 1.0f;

    /* Setup model lines per vessel type */
    if (ufo.size == 3) {
        ufo.line_count = sizeof(kamikaze_lines) / sizeof(Line);
        for (int i = 0; i < ufo.line_count; i++)
            ufo.lines[i] = kamikaze_lines[i];
    } else if (ufo.size == 4) {
        ufo.line_count = sizeof(bomber_lines) / sizeof(Line);
        for (int i = 0; i < ufo.line_count; i++)
            ufo.lines[i] = bomber_lines[i];
    } else if (ufo.size == 5) {
        ufo.line_count = 0; /* custom procedural rendering */
    } else if (ufo.size == 6) {
        ufo.line_count = sizeof(eldritch_lines) / sizeof(Line);
        for (int i = 0; i < ufo.line_count; i++)
            ufo.lines[i] = eldritch_lines[i];
    } else if (ufo.size == 7) {
        ufo.line_count = sizeof(daemon_lines) / sizeof(Line);
        for (int i = 0; i < ufo.line_count; i++)
            ufo.lines[i] = daemon_lines[i];
    } else {
        ufo.line_count = sizeof(ufo_lines) / sizeof(Line);
        for (int i = 0; i < ufo.line_count; i++)
            ufo.lines[i] = ufo_lines[i];
    }

    audio_play(SFX_UFO_LOOP);
}

void update_ufo(float dt)
{
    if (!ufo.active) return;

    /* Kamikaze (size 3) faces its movement direction */
    {
        float vspd = sqrtf(ufo.vel.x * ufo.vel.x + ufo.vel.y * ufo.vel.y);
        if (ufo.size == 3 && vspd > 1.0f)
            ufo.angle = atan2f(ufo.vel.y, ufo.vel.x);
    }

    /* Trail recording */
    ufo.trail_pos[ufo.trail_head] = ufo.pos;
    ufo.trail_ang[ufo.trail_head] = ufo.angle;
    ufo.trail_head = (ufo.trail_head + 1) % PHOS_TRAIL_LEN;

    /* External forces (rift gravity, etc.) */
    Vec2 ext_f = calculate_external_forces(ufo.pos);
    ufo.vel.x += ext_f.x * dt;
    ufo.vel.y += ext_f.y * dt;

    if (ufo.size == 3 && player.active) {
        /*
         * Vector Stalker — predictive intercept:
         *   t = time-to-target at cruise speed 200 u/s
         *   p_target = player_pos + player_vel * t
         * The stalker sets velocity directly toward that future point so
         * it leads the player rather than chasing the current position.
         */
        float dx   = player.pos.x - ufo.pos.x;
        float dy   = player.pos.y - ufo.pos.y;
        float dist = sqrtf(dx*dx + dy*dy);
        float t    = dist / 200.0f;
        Vec2  p_target = {
            player.pos.x + player.vel.x * t,
            player.pos.y + player.vel.y * t
        };
        float tdx  = p_target.x - ufo.pos.x;
        float tdy  = p_target.y - ufo.pos.y;
        float td   = sqrtf(tdx*tdx + tdy*tdy);
        if (td > 0.1f) {
            ufo.vel.x = (tdx / td) * 200.0f;
            ufo.vel.y = (tdy / td) * 200.0f;
        }
        ufo.pos.x += ufo.vel.x * dt;
        ufo.pos.y += ufo.vel.y * dt;

    } else if (ufo.size == 4) {
        /*
         * Boundary Weaver — constant horizontal sweep with lane-tracking:
         * vel.x is fixed at spawn; Y converges on target_y with a
         * proportional correction factor of 2.0.
         */
        ufo.pos.x += ufo.vel.x * dt;
        float dy    = ufo.target_y - ufo.pos.y;
        ufo.pos.y  += dy * 2.0f * dt;

    } else if (ufo.size == 5) {
        /*
         * Eye of the Void — Gravitational Harvester:
         * Horizontal drift is intentionally slow (30% of vel.x).
         * Vertical sinusoidal oscillation uses game_time so the wave
         * is deterministic and frame-rate independent.
         * Frequency: sinf(game_time * 2.0f) matches the original
         * sinf(SDL_GetTicks() / 500.0f) at a constant 60 fps.
         */
        ufo.pos.x += ufo.vel.x * 0.3f * dt;
        ufo.pos.y += sinf(game_time * 2.0f) * 20.0f * dt;

    } else if (ufo.size == 6) {
        /*
         * Eldritch Tendril — sinusoidal drift with slow player approach:
         * A gentle attractive acceleration pulls it toward the player.
         * Two independent Lissajous frequencies (1.625 Hz, 1.125 Hz)
         * perturb velocity for an organic wandering motion.
         * game_time * 1.25f replaces SDL_GetTicks() / 800.0f.
         * Speed is hard-capped at 120 u/s.
         */
        if (player.active) {
            float dx = player.pos.x - ufo.pos.x;
            float dy = player.pos.y - ufo.pos.y;
            float d  = sqrtf(dx*dx + dy*dy);
            if (d > 0.1f) {
                ufo.vel.x += (dx / d) * 30.0f * dt;
                ufo.vel.y += (dy / d) * 30.0f * dt;
            }
        }
        float t6 = game_time * 1.25f;         /* = SDL_GetTicks()/800 at 60fps */
        ufo.vel.x += cosf(t6 * 1.3f) * 40.0f * dt;
        ufo.vel.y += sinf(t6 * 0.9f) * 40.0f * dt;
        float spd6 = sqrtf(ufo.vel.x * ufo.vel.x + ufo.vel.y * ufo.vel.y);
        if (spd6 > 120.0f) {
            ufo.vel.x = ufo.vel.x / spd6 * 120.0f;
            ufo.vel.y = ufo.vel.y / spd6 * 120.0f;
        }
        ufo.pos.x  += ufo.vel.x * dt;
        ufo.pos.y  += ufo.vel.y * dt;
        ufo.angle   = atan2f(ufo.vel.y, ufo.vel.x);

    } else if (ufo.size == 7) {
        /*
         * Daemon Sigil — erratic teleport-lurch pattern:
         * Spins at 3 rad/s continuously.  Every 0.8–2.0 s it leaps to a
         * random nearby position and resets velocity in that direction.
         * Velocity then decays with a 1.5 s time constant between lurches.
         */
        ufo.angle += 3.0f * dt;
        ufo.change_dir_timer -= dt;
        if (ufo.change_dir_timer <= 0.0f) {
            ufo.change_dir_timer = 0.8f + ((float)rand() / RAND_MAX) * 1.2f;
            float a7   = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
            float lurch = 80.0f + ((float)rand() / RAND_MAX) * 80.0f;
            ufo.pos.x += cosf(a7) * lurch;
            ufo.pos.y += sinf(a7) * lurch;
            ufo.vel.x  = cosf(a7) * 160.0f;
            ufo.vel.y  = sinf(a7) * 160.0f;
        }
        ufo.pos.x  += ufo.vel.x * dt;
        ufo.pos.y  += ufo.vel.y * dt;
        ufo.vel.x  *= (1.0f - 1.5f * dt);
        ufo.vel.y  *= (1.0f - 1.5f * dt);

    } else {
        /* Normal UFO (size 1 or 2): horizontal drift, random Y reversals */
        ufo.pos.x += ufo.vel.x * dt;
        ufo.change_dir_timer -= dt;
        if (ufo.change_dir_timer <= 0.0f) {
            ufo.change_dir_timer = 1.0f + ((float)rand() / RAND_MAX) * 1.5f;
            ufo.vel.y = (-1.0f + 2.0f * (rand() % 2)) * 60.0f;
        }
        ufo.pos.y += ufo.vel.y * dt;
    }

    /* Deactivate if the player has flown too far away */
    if (player.active
        && distance_sq(ufo.pos, player.pos) > 3500.0f * 3500.0f) {
        ufo.active = 0;
        audio_stop(SFX_UFO_LOOP);
        ufo_spawn_timer = 20.0f + ((float)rand() / RAND_MAX) * 15.0f;
        return;
    }

    /* ── UFO weapon fire ─────────────────────────────────────────── */
    ufo.fire_timer -= dt;
    if (ufo.fire_timer <= 0.0f && player.active) {
        /* Fire rate varies by vessel type */
        if      (ufo.size == 6) ufo.fire_timer = 2.2f;
        else if (ufo.size == 7) ufo.fire_timer = 0.7f;
        else                    ufo.fire_timer = (ufo.size == 2) ? 1.5f : 1.0f;

        for (int i = 0; i < MAX_UFO_BULLETS; i++) {
            if (!ufo_bullets[i].active) {
                ufo_bullets[i].active    = 1;
                ufo_bullets[i].life      = BULLET_LIFE;
                ufo_bullets[i].pos       = ufo.pos;
                ufo_bullets[i].trail_head = 0;
                for (int t = 0; t < PHOS_TRAIL_LEN; t++) {
                    ufo_bullets[i].trail_pos[t] = ufo.pos;
                    ufo_bullets[i].trail_ang[t] = 0.0f;
                }

                float fire_angle;
                if (ufo.size == 2) {
                    /* Wanderer fires in a random direction */
                    fire_angle = ((float)rand() / RAND_MAX)
                                 * 2.0f * (float)M_PI;
                } else {
                    /* All others aim at the player with accuracy scatter
                     * that decreases as level rises: offset ~ 0.4/level */
                    float dx = player.pos.x - ufo.pos.x;
                    float dy = player.pos.y - ufo.pos.y;
                    fire_angle  = atan2f(dx, -dy);
                    fire_angle += (-0.3f + 0.6f * ((float)rand() / RAND_MAX))
                                  * (4.0f / (3.0f + level));
                }

                ufo_bullets[i].vel.x =
                    sinf(fire_angle) * UFO_BULLET_SPEED;
                ufo_bullets[i].vel.y =
                    -cosf(fire_angle) * UFO_BULLET_SPEED;
                ufo_bullets[i].color = (SDL_Color){255, 120, 120, 255};
                audio_play(SFX_UFO_FIRE);
                break;
            }
        }
    }
}

/** @brief Advances all active enemy bullets.
 *
 *  Applies external forces, integrates position, and culls bullets that
 *  exceed their lifetime or stray further than 1300 u from the player.
 */
void update_ufo_bullets(float dt)
{
    for (int i = 0; i < MAX_UFO_BULLETS; i++) {
        if (!ufo_bullets[i].active) continue;

        /* Trail recording */
        ufo_bullets[i].trail_pos[ufo_bullets[i].trail_head] = ufo_bullets[i].pos;
        ufo_bullets[i].trail_ang[ufo_bullets[i].trail_head] = 0.0f;
        ufo_bullets[i].trail_head =
            (ufo_bullets[i].trail_head + 1) % PHOS_TRAIL_LEN;

        ufo_bullets[i].life -= dt;
        if (ufo_bullets[i].life <= 0.0f) {
            ufo_bullets[i].active = 0;
            continue;
        }

        Vec2 ext_f = calculate_external_forces(ufo_bullets[i].pos);
        ufo_bullets[i].vel.x += ext_f.x * dt;
        ufo_bullets[i].vel.y += ext_f.y * dt;
        ufo_bullets[i].pos.x += ufo_bullets[i].vel.x * dt;
        ufo_bullets[i].pos.y += ufo_bullets[i].vel.y * dt;

        if (player.active
            && distance_sq(ufo_bullets[i].pos, player.pos) > 1300.0f * 1300.0f)
            ufo_bullets[i].active = 0;
    }
}

void update_npcs(float dt)
{
/* ── Friendly NPCs ────────────────────────────────────────────── */
    for (int i = 0; i < MAX_NPC; i++) {
        if (!npcs[i].active) continue;

        float dx   = player.pos.x - npcs[i].pos.x;
        float dy   = player.pos.y - npcs[i].pos.y;
        float dist = sqrtf(dx*dx + dy*dy);

        if (!npcs[i].following) {
            /* Idle drift: gentle sinusoidal circles */
            npcs[i].orbit_angle += 0.4f * dt;
            npcs[i].vel.x += cosf(npcs[i].orbit_angle) * 20.0f * dt;
            npcs[i].vel.y += sinf(npcs[i].orbit_angle) * 20.0f * dt;
            npcs[i].vel.x *= (1.0f - 2.0f * dt);
            npcs[i].vel.y *= (1.0f - 2.0f * dt);
            npcs[i].pos.x += npcs[i].vel.x * dt;
            npcs[i].pos.y += npcs[i].vel.y * dt;

            /* Start following if the player stays within 120 u for 2 s */
            if (dist < 120.0f && player.active) {
                npcs[i].contact_timer += dt;
                if (npcs[i].contact_timer > 2.0f) {
                    npcs[i].following    = 1;
                    npcs[i].orbit_angle  = atan2f(dy, dx) + (float)M_PI;
                }
            } else {
                npcs[i].contact_timer *= 0.95f;
            }
        } else {
            /* Orbit player at 85 u using spring-damper */
            float target_dist = 85.0f;
            npcs[i].orbit_angle += 1.1f * dt;
            float tx = player.pos.x + cosf(npcs[i].orbit_angle) * target_dist;
            float ty = player.pos.y + sinf(npcs[i].orbit_angle) * target_dist;
            float ex = tx - npcs[i].pos.x;
            float ey = ty - npcs[i].pos.y;
            npcs[i].vel.x += ex * 8.0f * dt;
            npcs[i].vel.y += ey * 8.0f * dt;
            npcs[i].vel.x *= (1.0f - 3.0f * dt);
            npcs[i].vel.y *= (1.0f - 3.0f * dt);
            npcs[i].pos.x += npcs[i].vel.x * dt;
            npcs[i].pos.y += npcs[i].vel.y * dt;

            if (!player.active || dist > 400.0f)
                npcs[i].following = 0;
        }
        npcs[i].angle = atan2f(npcs[i].vel.y, npcs[i].vel.x);

        /* NPC dies if struck by a player bullet */
        for (int b = 0; b < MAX_BULLETS; b++) {
            if (!bullets[b].active) continue;
            float bx = bullets[b].pos.x - npcs[i].pos.x;
            float by = bullets[b].pos.y - npcs[i].pos.y;
            if (sqrtf(bx*bx + by*by) < npcs[i].radius) {
                bullets[b].active = 0;
                npcs[i].active    = 0;
                spawn_particles(npcs[i].pos, 12, (SDL_Color){80, 255, 80, 255});
                audio_play(SFX_EXPLOSION_SM);
                break;
            }
        }
    }
}
