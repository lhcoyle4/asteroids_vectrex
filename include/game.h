#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 960

// Initialize the game
void game_init();

// Handle keypress events
void game_handle_input(SDL_Event *event);

// Update game physics and logic
void game_update(float delta_time);

// Draw all game objects and text
void game_render();

// Set pause state
void game_set_paused(int paused);

#endif
