#ifndef PLAYER_H
#define PLAYER_H

#include "entities.h"
#include "game_data.h"
#include "state.h"
#include <SDL2/SDL.h>

int check_void_stone(void);
void reset_player(void);
void trigger_hyperspace(void);
void spawn_upgrade_options(void);
void apply_upgrade(UpgradeType type);
void update_player_physics(float dt);
void update_player_bullets(float dt);

/* Exposed game.c static functions */
void spawn_particles(Vec2 pos, int count, SDL_Color color);
Vec2 calculate_external_forces(Vec2 pos);
float distance_sq(Vec2 p1, Vec2 p2);

#endif /* PLAYER_H */
