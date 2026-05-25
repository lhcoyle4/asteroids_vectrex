#ifndef GAME_H
#define GAME_H
#include <SDL2/SDL.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 960

/* Initialize all game state. Call once after SDL window/renderer are ready. */
void game_init();

/* Process keypress events */
void game_handle_input(SDL_Event *event);

/* Update game physics and logic */
void game_update(float delta_time);

/* Draw all game objects and the HUD */
void game_render();

/* Pause / unpause the game */
void game_set_paused(int paused);

/* Called when the window gains or loses focus */
void game_focus_changed(int has_focus);

#endif /* GAME_H */
