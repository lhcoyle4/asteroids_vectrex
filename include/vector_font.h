#ifndef VECTOR_FONT_H
#define VECTOR_FONT_H

#include "vector_graphics.h"

// Initialize the vector font system
void vf_init();

// Draw a single character at (x, y) with a given scale and color
void vf_draw_char(char c, float x, float y, float scale, SDL_Color color);

// Draw a string at (x, y) with a given scale and color
void vf_draw_string(const char *str, float x, float y, float scale, SDL_Color color);

// Draw a string centered at x with a given scale and color
void vf_draw_string_centered(const char *str, float x, float y, float scale, SDL_Color color);

#endif
