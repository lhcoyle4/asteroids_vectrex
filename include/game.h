/* fuligin/include/game.h
 * Public API for the FULIGIN game module.
 * Declares the game lifecycle functions called from main.c.
 *
 * Author: [author]
 * Date: 2025
 */

#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>

/* Canonical game resolution. All rendering and layout targets these dimensions. */
#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT  960

/**
 * @brief Called once after SDL window/renderer initialisation.
 *
 * Loads high scores from disk and initialises all game state — entities,
 * timers, RNG seed, and the starting screen. Must be called before any
 * ot