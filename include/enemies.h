#ifndef ENEMIES_H
#define ENEMIES_H

#include "state.h"
#include "entities.h"

void spawn_ufo(void);
void update_ufo(float dt);
void update_ufo_bullets(float dt);
void spawn_npc(Vec2 pos);
void update_npcs(float dt);

Vec2 calculate_external_forces(Vec2 pos);
float distance_sq(Vec2 p1, Vec2 p2);

#endif
