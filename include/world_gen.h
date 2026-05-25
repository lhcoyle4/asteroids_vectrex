#ifndef WORLD_GEN_H
#define WORLD_GEN_H

#include "entities.h"
#include "state.h"

void init_asteroid_shape(AsteroidEntity *ast, int size);
void spawn_asteroid(Vec2 pos, int size);
void spawn_orb(Vec2 pos, int value);
void update_asteroids(float dt);
void update_spawning(float dt);

#endif /* WORLD_GEN_H */
