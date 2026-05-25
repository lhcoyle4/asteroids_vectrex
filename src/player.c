#include "player.h"
#include "audio.h"
#include "vector_graphics.h"
#include "ui.h"
#include "game_data.h"
#include <math.h>
#include <stdlib.h>

int check_void_stone(void) {
    if (lives <= 1 && res_void_stone > 0) {
        res_void_stone--;
        time_stop_frames = 30;
        player.pos = player_pos_history[(pos_history_idx + POS_HISTORY_LEN - 100) % POS_HISTORY_LEN];
        player.invuln_timer = 2.0f;
        return 1;
    }
    return 0;
}

/** @brief Respawns the Autodyne at a random screen position.
 *  Grants 2 seconds of invulnerability to allow orientation after death. */
void reset_player(void)
{
    player.active = 1;
    /* Respawn at random position within the current visible screen area */
    float margin  = 80.0f;
    player.pos.x  = camera_pos.x + margin
                    + ((float)rand() / RAND_MAX) * (SCREEN_WIDTH  - margin * 2.0f);
    player.pos.y  = camera_pos.y + margin
                    + ((float)rand() / RAND_MAX) * (SCREEN_HEIGHT - margin * 2.0f);
    player.vel    = (Vec2){0.0f, 0.0f};
    player.angle  = 0.0f;
    player.invuln_timer = 2.0f; /* 2 seconds invulnerability */
}

/** @brief Executes an emergency hyperspace jump, teleporting the Autodyne to a random position.
 *  Carries a HYPERSPACE_DEATH_CHANCE probability of catastrophic misalignment (instant death).
 *  "To fold space without the Autarch's blessing is to risk annihilation." */
void trigger_hyperspace(void)
{
    if (!player.active) return;

    spawn_particles(player.pos, 10, (SDL_Color){150, 180, 255, 255});
    audio_play(SFX_EXPLOSION_SM);

    /* Catastrophic misalignment — FULIGIN classic risk */
    if (((float)rand() / RAND_MAX) < HYPERSPACE_DEATH_CHANCE) {
        if (check_void_stone()) return;
        player.active = 0;
        audio_play(SFX_EXPLOSION_LG);
        spawn_particles(player.pos, 30, (SDL_Color){255, 100, 100, 255});
        player_death_pos   = player.pos;
        player_death_angle = player.angle;
        player_death_timer = 1.8f;
        lives--;
        if (lives <= 0) {
            game_state = STATE_GAMEOVER;
            audio_stop(SFX_THRUST);
            audio_stop(SFX_UFO_LOOP);
        }
        return;
    }

    /* Hyperspace lands within the current visible screen area (not world origin) */
    float margin  = 80.0f;
    player.pos.x  = camera_pos.x + margin
                    + ((float)rand() / RAND_MAX) * (SCREEN_WIDTH  - margin * 2.0f);
    player.pos.y  = camera_pos.y + margin
                    + ((float)rand() / RAND_MAX) * (SCREEN_HEIGHT - margin * 2.0f);
    player.vel          = (Vec2){0.0f, 0.0f};
    player.invuln_timer = 0.5f;

    /* Hook massive white rectangle flash into hyperspace logic */
    hyperjump_flash_timer = 1.0f;
}

/** @brief Randomly selects 3 unique relics from the full pool for the player to choose. */
void spawn_upgrade_options()
{
    audio_stop(SFX_THRUST);
    audio_stop(SFX_UFO_LOOP);

    UpgradeType all_upgrades[UPGRADE_COUNT];
    for (int i = 0; i < UPGRADE_COUNT; i++) all_upgrades[i] = (UpgradeType)i;
    int total_available = UPGRADE_COUNT;

    /* Fisher-Yates partial shuffle — pick 3 unique relics */
    for (int i = 0; i < 3; i++) {
        int idx = i + rand() % (total_available - i);
        UpgradeType temp = all_upgrades[i];
        all_upgrades[i]  = all_upgrades[idx];
        all_upgrades[idx] = temp;
        upgrade_options[i] = all_upgrades[i];
    }
    selected_option = 0;
}

/** @brief Applies the chosen relic's effect to the Autodyne's capabilities.
 *  Each relic permanently alters player_upgrades until the run ends. */
void apply_upgrade(UpgradeType type)
{
    switch (type) {
        /* relic: fires two additional forward bolts per shot */
        case UPGRADE_TRIPLE_SHOT:
            player_upgrades.triple_shot = 1;
            break;

        /* relic: grants one additional ricochet bounce to all projectiles */
        case UPGRADE_BOUNCE:
            player_upgrades.max_bounces += 1;
            break;

        /* relic: activates a regenerating energy barrier around the hull */
        case UPGRADE_SHIELD:
            player_upgrades.shield_active = 1;
            break;

        /* relic: reduces the weapon cycle cooldown by 30% */
        case UPGRADE_RAPID_FIRE:
            player_upgrades.fire_cooldown_mult *= 0.7f;
            break;

        /* relic: rounds pass through targets without being consumed */
        case UPGRADE_PIERCING:
            player_upgrades.piercing = 1;
            break;

        /* relic: boosts thruster output, increasing top speed by 20% */
        case UPGRADE_SPEED:
            player_upgrades.speed_mult *= 1.2f;
            break;

        /* relic: compresses hull cross-section, shrinking collision radius by 20% */
        case UPGRADE_SIZE_DOWN:
            player_upgrades.size_mult *= 0.8f;
            player.radius  *= 0.8f;
            break;

        /* relic: rounds acquire and track the nearest hostile */
        case UPGRADE_HOMING:
            player_upgrades.homing = 1;
            break;

        /* relic: extends the XP orb attraction field by 50 units */
        case UPGRADE_ORB_MAGNET:
            player_upgrades.magnet_radius += 50.0f;
            break;

        /* relic: mounts a rearward-facing cannon on the stern */
        case UPGRADE_REAR_GUN:
            player_upgrades.rear_gun = 1;
            break;

        /* relic: each shot fans into three diverging bolts on impact */
        case UPGRADE_SPLIT_SHOT:
            player_upgrades.split_shot = 1;
            break;

        /* relic: drift-core resonance multiplies XP yield by 50% */
        case UPGRADE_XP_BOOST:
            player_upgrades.xp_mult += 0.5f;
            break;

        /* relic: emergency life support restoration — grants +1 hull integrity */
        case UPGRADE_HEALTH_BOOST:
            lives++;
            break;

        /* relic: projects a phantom duplicate that mirrors all fire */
        case UPGRADE_MIRROR_IMAGE:
            player_upgrades.mirror_image = 1;
            break;

        /* relic: brief temporal displacement on collision — blinks through matter */
        case UPGRADE_PHASE_SHIFT:
            player_upgrades.phase_shift = 1;
            break;

        /* relic: hull plating absorbs heat, rendering fire damage negligible */
        case UPGRADE_THERMAL_HULL:
            player_upgrades.thermal_hull = 1;
            break;

        /* relic: warps a nearby foe to a random position in the drift */
        case UPGRADE_SINGULARITY_DISPLACER:
            player_upgrades.singularity_displacer = 1;
            break;

        /* relic: releases a lashing singularity tendril on near-miss */
        case UPGRADE_SINGULARITY_WHIP:
            player_upgrades.singularity_whip = 1;
            break;

        /* relic: destroyed targets trigger a chain detonation cascade */
        case UPGRADE_RESONANCE_CASCADE:
            player_upgrades.resonance_cascade = 1;
            break;

        /* relic: secondary weapon — launches a void vortex grenade */
        case UPGRADE_VORTEX_GRENADE:
            player_upgrades.vortex_grenade = 1;
            break;

        /* relic: deploys a self-targeting gun platform that orbits the ship */
        case UPGRADE_AUTO_TURRET:
            player_upgrades.auto_turret = 1;
            break;

        /* relic: each kill triggers a short-range plasma nova burst */
        case UPGRADE_NOVA_EXPLOSION:
            player_upgrades.nova_explosion = 1;
            break;

        default:
            break;
    }
}

/** @brief Updates player thrust, rotation, and hyperspace input state.
 *
 *  Handles both keyboard/mouse and gamepad input for all three control
 *  schemes (arcade, twin-stick).  Applies thrust force, external
 *  gravitational forces, friction, mirror-image positioning, auto-turret
 *  tick, and invulnerability blink.  Also manages the respawn countdown
 *  when the player is not active.
 */
void update_player_physics(float dt)
{
    if (player.active) {
        int thrust_key_down = 0;
        int fire_key_down   = 0;
        float move_x = 0.0f;
        float move_y = 0.0f;

        /* ── Attract-mode AI pilot ─────────────────────────────────── */
        if (game_state == STATE_ATTRACT_GAMEPLAY) {
            float closest_dist = 9999999.0f;
            int   closest_idx  = -1;
            for (int i = 0; i < MAX_ASTEROIDS; i++) {
                if (asteroids[i].active) {
                    float dx = asteroids[i].pos.x - player.pos.x;
                    float dy = asteroids[i].pos.y - player.pos.y;
                    float d2 = dx*dx + dy*dy;
                    if (d2 < closest_dist) {
                        closest_dist = d2;
                        closest_idx  = i;
                    }
                }
            }
            if (closest_idx != -1) {
                float dx = asteroids[closest_idx].pos.x - player.pos.x;
                float dy = asteroids[closest_idx].pos.y - player.pos.y;
                player.angle = atan2f(dy, dx) + (float)M_PI_2;
                if (closest_dist > 150.0f * 150.0f) thrust_key_down = 1;
                if (closest_dist < 300.0f * 300.0f) fire_key_down   = 1;
            }
        } else {
            /* ── Human input ───────────────────────────────────────── */
            const Uint8 *keys = SDL_GetKeyboardState(NULL);
            int mx, my;
            Uint32 mouse_buttons = SDL_GetMouseState(&mx, &my);

            int   right_stick_active = 0;
            float rx_val = 0.0f, ry_val = 0.0f;

            /* Gamepad */
            if (g_controller) {
                float deadzone =
                    (float[]){0.1f, 0.2f, 0.35f}[settings_controller_deadzone]
                    * 32767.0f;
                int16_t lt =
                    SDL_GameControllerGetAxis(g_controller,
                                             SDL_CONTROLLER_AXIS_TRIGGERLEFT);
                int16_t rt =
                    SDL_GameControllerGetAxis(g_controller,
                                             SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

                if (settings_control_scheme == 0) {
                    /* Arcade: left-stick steers, triggers fire/thrust */
                    int16_t lx =
                        SDL_GameControllerGetAxis(g_controller,
                                                 SDL_CONTROLLER_AXIS_LEFTX);
                    if (fabsf((float)lx) > deadzone)
                        player.angle +=
                            ((float)lx / 32767.0f) * ROTATION_SPEED * dt * 2.5f;
                    if (lt > 8000
                        || SDL_GameControllerGetButton(
                               g_controller, ctrl_binds[CT_THRUST]))
                        thrust_key_down = 1;
                    if (rt > 8000
                        || SDL_GameControllerGetButton(
                               g_controller, ctrl_binds[CT_FIRE]))
                        fire_key_down = 1;
                    twin_stick_fire_active = 0;
                } else {
                    /* Twin-stick: left stick moves, right stick aims/fires */
                    int16_t lx =
                        SDL_GameControllerGetAxis(g_controller,
                                                 SDL_CONTROLLER_AXIS_LEFTX);
                    int16_t ly =
                        SDL_GameControllerGetAxis(g_controller,
                                                 SDL_CONTROLLER_AXIS_LEFTY);
                    float lmag = sqrtf((float)lx*(float)lx
                                       + (float)ly*(float)ly);
                    if (lmag > deadzone) {
                        move_x = (float)lx / 32767.0f;
                        move_y = (float)ly / 32767.0f;
                        thrust_key_down = 1;
                    }

                    int16_t rx =
                        SDL_GameControllerGetAxis(g_controller,
                                                 SDL_CONTROLLER_AXIS_RIGHTX);
                    int16_t ry =
                        SDL_GameControllerGetAxis(g_controller,
                                                 SDL_CONTROLLER_AXIS_RIGHTY);
                    float rmag = sqrtf((float)rx*(float)rx
                                       + (float)ry*(float)ry);
                    if (rmag > deadzone * 1.2f) {
                        rx_val = (float)rx / rmag;
                        ry_val = (float)ry / rmag;
                        right_stick_active    = 1;
                        twin_stick_fire_angle = atan2f((float)rx, -(float)ry);
                        twin_stick_fire_active = 1;
                        fire_key_down = 1;
                    } else {
                        twin_stick_fire_active = 0;
                    }

                    if (lt > 8000
                        || SDL_GameControllerGetButton(
                               g_controller, ctrl_binds[CT_THRUST]))
                        thrust_key_down = 1;
                    if (rt > 8000
                        || SDL_GameControllerGetButton(
                               g_controller, ctrl_binds[CT_FIRE]))
                        fire_key_down = 1;
                }
            }

            /* Keyboard / mouse */
            if (settings_control_scheme == 0) {
                /* Arcade keyboard: rotate left/right */
                if (keys[keybinds[KB_ROTATE_LEFT]]  || keys[SDL_SCANCODE_A])
                    player.angle -= ROTATION_SPEED * dt;
                if (keys[keybinds[KB_ROTATE_RIGHT]] || keys[SDL_SCANCODE_D])
                    player.angle += ROTATION_SPEED * dt;
                if (keys[keybinds[KB_THRUST]] || keys[SDL_SCANCODE_W]
                    || (settings_mouse_aim
                        && (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT))))
                    thrust_key_down = 1;
                if (keys[keybinds[KB_FIRE]] || keys[SDL_SCANCODE_SPACE]
                    || (settings_mouse_aim
                        && (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))))
                    fire_key_down = 1;

                if (settings_mouse_aim) {
                    float spx = player.pos.x - camera_pos.x;
                    float spy = player.pos.y - camera_pos.y;
                    float dx = (float)mx - spx;
                    float dy = (float)my - spy;
                    if (dx*dx + dy*dy > 9.0f)
                        player.angle = atan2f(dy, dx) + (float)M_PI_2;
                }
            } else {
                /* Twin-stick keyboard: WASD translates, mouse aims */
                float kb_x = 0.0f, kb_y = 0.0f;
                if (keys[SDL_SCANCODE_A] || keys[keybinds[KB_ROTATE_LEFT]])
                    kb_x -= 1.0f;
                if (keys[SDL_SCANCODE_D] || keys[keybinds[KB_ROTATE_RIGHT]])
                    kb_x += 1.0f;
                if (keys[SDL_SCANCODE_W] || keys[keybinds[KB_THRUST]])
                    kb_y -= 1.0f;
                if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])
                    kb_y += 1.0f;

                if (kb_x != 0.0f || kb_y != 0.0f) {
                    float len = sqrtf(kb_x*kb_x + kb_y*kb_y);
                    move_x = kb_x / len;
                    move_y = kb_y / len;
                    thrust_key_down = 1;
                }
                if (keys[keybinds[KB_FIRE]] || keys[SDL_SCANCODE_SPACE])
                    fire_key_down = 1;

                int   mouse_aim_active = 0;
                float mouse_dx = 0.0f, mouse_dy = 0.0f;
                if (settings_mouse_aim) {
                    float spx = player.pos.x - camera_pos.x;
                    float spy = player.pos.y - camera_pos.y;
                    float dx = (float)mx - spx;
                    float dy = (float)my - spy;
                    if (dx*dx + dy*dy > 9.0f) {
                        mouse_dx = dx;
                        mouse_dy = dy;
                        mouse_aim_active = 1;
                    }
                    if (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
                        thrust_key_down = 1;
                    if (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))
                        fire_key_down = 1;
                }

                /* Heading priority: right-stick > mouse > movement direction */
                if (right_stick_active)
                    player.angle = atan2f(rx_val, -ry_val);
                else if (mouse_aim_active)
                    player.angle = atan2f(mouse_dx, -mouse_dy);
                else if (move_x != 0.0f || move_y != 0.0f)
                    player.angle = atan2f(move_x, -move_y);
            }
        }

        /* ── Singularity Displacer: double-tap thrust to rift-jump ── */
        static Uint32 last_thrust_tap    = 0;
        static int    thrust_key_was_down = 0;
        is_thrusting = thrust_key_down;

        float thrust_angle = player.angle;
        if (settings_control_scheme == 1 && (move_x != 0.0f || move_y != 0.0f))
            thrust_angle = atan2f(move_x, -move_y);

        if (thrust_key_down && !thrust_key_was_down) {
            Uint32 now = SDL_GetTicks();
            if (player_upgrades.singularity_displacer
                && (now - last_thrust_tap) < 300) {
                Vec2 old_pos = player.pos;
                player.pos.x += sinf(thrust_angle) * 400.0f;
                player.pos.y -= cosf(thrust_angle) * 400.0f;
                spawn_particles(old_pos,    20, (SDL_Color){100, 100, 255, 255});
                spawn_particles(player.pos, 20, (SDL_Color){100, 100, 255, 255});
                audio_play(SFX_EXPLOSION_MD);
            }
            last_thrust_tap = now;
        }
        thrust_key_was_down = thrust_key_down;

        /* ── Thrust force + fuel burn ──────────────────────────────── */
        if (thrust_key_down) {
            player.vel.x += sinf(thrust_angle)
                            * THRUST_FORCE * player_upgrades.speed_mult * dt;
            player.vel.y -= cosf(thrust_angle)
                            * THRUST_FORCE * player_upgrades.speed_mult * dt;
            is_thrusting = 1;
            thrust_timer += dt;
            fuel_current -= FUEL_BURN_RATE * dt;
            if (fuel_current < 0.0f) fuel_current = 0.0f;
            audio_play(SFX_THRUST);
        } else {
            is_thrusting = 0;
            audio_stop(SFX_THRUST);
        }

        /* Passive fuel regen inside FUEL_REGEN_RADIUS of the home station */
        {
            float hx = player.pos.x, hy = player.pos.y;
            if (sqrtf(hx*hx + hy*hy) < FUEL_REGEN_RADIUS
                && fuel_current < fuel_max) {
                fuel_current += 8.0f * dt;
                if (fuel_current > fuel_max) fuel_current = fuel_max;
            }
        }

        /* ── Autofire cooldown ─────────────────────────────────────── */
        if (fire_cooldown_timer > 0.0f)
            fire_cooldown_timer -= dt;
        if (settings_autofire && fire_cooldown_timer <= 0.0f)
            fire_key_down = 1;

        /* ── Trail recording ───────────────────────────────────────── */
        player.trail_pos[player.trail_head] = player.pos;
        player.trail_ang[player.trail_head] = player.angle;
        player.trail_head = (player.trail_head + 1) % PHOS_TRAIL_LEN;

        /* ── External forces + friction + integration ──────────────── */
        Vec2 ext_f = calculate_external_forces(player.pos);
        player.vel.x += ext_f.x * dt;
        player.vel.y += ext_f.y * dt;
        player.vel.x *= powf(FRICTION, dt * 60.0f);
        player.vel.y *= powf(FRICTION, dt * 60.0f);
        player.pos.x += player.vel.x * dt;
        player.pos.y += player.vel.y * dt;

        /* ── Mirror image: phantom twin orbits 200 px behind the prow ─ */
        if (player_upgrades.mirror_image) {
            mirror_active_flag = 1;
            mirror_pos.x  = player.pos.x - sinf(player.angle) * 200.0f;
            mirror_pos.y  = player.pos.y + cosf(player.angle) * 200.0f;
            mirror_angle  = player.angle;
        } else {
            mirror_active_flag = 0;
        }

        /* ── Auto turret: fires a 4-way ring every 2 seconds ──────── */
        static float auto_turret_timer = 0.0f;
        if (player_upgrades.auto_turret) {
            auto_turret_timer += dt;
            if (auto_turret_timer >= 2.0f) {
                auto_turret_timer = 0.0f;
                for (int ring = 0; ring < 4; ring++) {
                    float ang = (float)ring * ((float)M_PI * 0.5f);
                    for (int bi = 0; bi < MAX_BULLETS; bi++) {
                        if (!bullets[bi].active) {
                            bullets[bi].active   = 1;
                            bullets[bi].pos      = player.pos;
                            bullets[bi].vel.x    = sinf(ang) * BULLET_SPEED * 0.57f;
                            bullets[bi].vel.y    = -cosf(ang) * BULLET_SPEED * 0.57f;
                            bullets[bi].life     = 1.8f;
                            bullets[bi].bounces  = 0;
                            bullets[bi].is_homing = 0;
                            bullets[bi].pierces  = player_upgrades.piercing;
                            break;
                        }
                    }
                }
            }
        } else {
            auto_turret_timer = 0.0f;
        }

        /* ── Primary weapon fire ───────────────────────────────────── */
        if (fire_key_down && fire_cooldown_timer <= 0.0f) {
            int   bolts_to_fire  = player_upgrades.triple_shot ? 3 : 1;
            float spread         = 0.25f;
            /* In twin-stick mode bolts travel along the right-stick axis */
            float base_fire_angle = twin_stick_fire_active
                                    ? twin_stick_fire_angle
                                    : player.angle;

            for (int b = 0; b < bolts_to_fire; b++) {
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (!bullets[i].active) {
                        float ang = base_fire_angle;
                        if (bolts_to_fire == 3) ang += (b - 1) * spread;

                        float nose_x = player.pos.x + sinf(ang) * 12.0f;
                        float nose_y = player.pos.y - cosf(ang) * 12.0f;

                        bullets[i].active   = 1;
                        bullets[i].life     = BULLET_LIFE;
                        bullets[i].bounces  = 0;
                        bullets[i].pierces  = 0;
                        bullets[i].pos      = (Vec2){nose_x, nose_y};
                        bullets[i].trail_head = 0;
                        for (int t = 0; t < PHOS_TRAIL_LEN; t++) {
                            bullets[i].trail_pos[t] = bullets[i].pos;
                            bullets[i].trail_ang[t] = 0.0f;
                        }
                        bullets[i].vel.x =
                            sinf(ang) * BULLET_SPEED
                            * player_upgrades.bullet_speed_mult
                            + player.vel.x * 0.3f;
                        bullets[i].vel.y =
                            -cosf(ang) * BULLET_SPEED
                            * player_upgrades.bullet_speed_mult
                            + player.vel.y * 0.3f;
                        bullets[i].color     = (SDL_Color){255, 255, 255, 255};
                        bullets[i].is_homing = player_upgrades.homing ? 1 : 0;
                        break;
                    }
                }
            }

            /* Rear gun fires from the aft thruster port */
            if (player_upgrades.rear_gun) {
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (!bullets[i].active) {
                        float rear_ang = base_fire_angle + (float)M_PI;
                        float rear_x   = player.pos.x
                                         - sinf(base_fire_angle) * 12.0f;
                        float rear_y   = player.pos.y
                                         + cosf(base_fire_angle) * 12.0f;
                        bullets[i].active    = 1;
                        bullets[i].life      = BULLET_LIFE;
                        bullets[i].bounces   = 0;
                        bullets[i].pierces   = 0;
                        bullets[i].pos       = (Vec2){rear_x, rear_y};
                        bullets[i].trail_head = 0;
                        for (int t = 0; t < PHOS_TRAIL_LEN; t++) {
                            bullets[i].trail_pos[t] = bullets[i].pos;
                            bullets[i].trail_ang[t] = 0.0f;
                        }
                        bullets[i].vel.x =
                            sinf(rear_ang) * BULLET_SPEED
                            * player_upgrades.bullet_speed_mult
                            + player.vel.x * 0.3f;
                        bullets[i].vel.y =
                            -cosf(rear_ang) * BULLET_SPEED
                            * player_upgrades.bullet_speed_mult
                            + player.vel.y * 0.3f;
                        bullets[i].color     = (SDL_Color){255, 180, 80, 255};
                        bullets[i].is_homing = 0;
                        break;
                    }
                }
            }

            audio_play(SFX_FIRE);
            fire_cooldown_timer =
                0.25f * player_upgrades.fire_cooldown_mult;
        }

        /* ── God mode cheat: unlimited lives and rapid triple fire ── */
        if (god_mode) {
            lives = 99;
            player_upgrades.triple_shot        = 1;
            player_upgrades.fire_cooldown_mult  = 0.2f;
            player_upgrades.magnet_radius       = 500.0f;
        }

        /* ── Invulnerability blink countdown ───────────────────────── */
        if (player.invuln_timer > 0.0f)
            player.invuln_timer -= dt;

    } else {
        /* ── Respawn countdown ─────────────────────────────────────── */
        static float spawn_delay = 2.2f;
        if (lives > 0) {
            spawn_delay -= dt;
            if (spawn_delay <= 0.6f && respawn_phase == 0) {
                respawn_phase = 1;
                respawn_blink = 0.0f;
            }
            if (spawn_delay <= 0.0f) {
                reset_player();
                spawn_delay   = 2.2f;
                respawn_phase = 0;
                respawn_blink = 0.0f;
            }
        }
    }
}

/** @brief Advances all active player bullets and handles their lifecycle.
 *
 *  Applies external gravitational forces, homing steering, movement
 *  integration, and bounce/cull logic.  Bullets that exceed the cull
 *  radius (1300 u from the player) or exhaust their lifetime are
 *  deactivated.
 */
void update_player_bullets(float dt)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;

        /* Trail recording */
        bullets[i].trail_pos[bullets[i].trail_head] = bullets[i].pos;
        bullets[i].trail_ang[bullets[i].trail_head] = 0.0f;
        bullets[i].trail_head = (bullets[i].trail_head + 1) % PHOS_TRAIL_LEN;

        bullets[i].life -= dt;
        if (bullets[i].life <= 0.0f) {
            bullets[i].active = 0;
            continue;
        }

        /* External gravitational forces (Void Rifts, home-station gravity) */
        Vec2 ext_f = calculate_external_forces(bullets[i].pos);
        bullets[i].vel.x += ext_f.x * dt;
        bullets[i].vel.y += ext_f.y * dt;

        /* Homing steering: turn toward nearest asteroid */
        if (bullets[i].is_homing) {
            float min_d2 = 1e18f;
            Vec2  home_target = bullets[i].pos;
            for (int ha = 0; ha < MAX_ASTEROIDS; ha++) {
                if (!asteroids[ha].active) continue;
                float hdx = asteroids[ha].pos.x - bullets[i].pos.x;
                float hdy = asteroids[ha].pos.y - bullets[i].pos.y;
                float hd2 = hdx*hdx + hdy*hdy;
                if (hd2 < min_d2) { min_d2 = hd2; home_target = asteroids[ha].pos; }
            }
            float hdx  = home_target.x - bullets[i].pos.x;
            float hdy  = home_target.y - bullets[i].pos.y;
            float hdist = sqrtf(hdx*hdx + hdy*hdy);
            if (hdist > 1.0f) {
                float str = 280.0f;
                bullets[i].vel.x += (hdx / hdist) * str * dt;
                bullets[i].vel.y += (hdy / hdist) * str * dt;
                float bspd = sqrtf(bullets[i].vel.x * bullets[i].vel.x
                                   + bullets[i].vel.y * bullets[i].vel.y);
                float max_spd = BULLET_SPEED * player_upgrades.bullet_speed_mult;
                if (bspd > max_spd) {
                    bullets[i].vel.x = bullets[i].vel.x / bspd * max_spd;
                    bullets[i].vel.y = bullets[i].vel.y / bspd * max_spd;
                }
            }
        }

        bullets[i].pos.x += bullets[i].vel.x * dt;
        bullets[i].pos.y += bullets[i].vel.y * dt;

        /* Bouncy bullets: reflect off the current camera viewport edges */
        if (player_upgrades.max_bounces > 0
            && bullets[i].bounces < player_upgrades.max_bounces) {
            int bounced = 0;
            if (bullets[i].pos.x < camera_pos.x) {
                bullets[i].pos.x = camera_pos.x;
                bullets[i].vel.x = -bullets[i].vel.x;
                bounced = 1;
            } else if (bullets[i].pos.x > camera_pos.x + SCREEN_WIDTH) {
                bullets[i].pos.x = camera_pos.x + SCREEN_WIDTH;
                bullets[i].vel.x = -bullets[i].vel.x;
                bounced = 1;
            }
            if (bullets[i].pos.y < camera_pos.y) {
                bullets[i].pos.y = camera_pos.y;
                bullets[i].vel.y = -bullets[i].vel.y;
                bounced = 1;
            } else if (bullets[i].pos.y > camera_pos.y + SCREEN_HEIGHT) {
                bullets[i].pos.y = camera_pos.y + SCREEN_HEIGHT;
                bullets[i].vel.y = -bullets[i].vel.y;
                bounced = 1;
            }
            if (bounced) bullets[i].bounces++;
        } else {
            /* Non-bouncy: cull when far from player */
            if (player.active
                && distance_sq(bullets[i].pos, player.pos)
                   > 1300.0f * 1300.0f) {
                bullets[i].active = 0;
            }
        }
    }
}

