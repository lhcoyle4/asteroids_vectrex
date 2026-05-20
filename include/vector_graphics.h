#ifndef VECTOR_GRAPHICS_H
#define VECTOR_GRAPHICS_H

#include <SDL2/SDL.h>

typedef struct {
    float x, y;
} Vec2;

typedef struct {
    Vec2 p1, p2;
} Line;

typedef struct {
    Line *lines;
    int line_count;
    SDL_Color color;
} Shape;

// Initialize graphics system
void vg_init(SDL_Renderer *renderer, int width, int height);

// Draw a shape with translation and rotation
void vg_draw_shape(Shape *shape, Vec2 pos, float angle, float scale);

// Draw N ghost trail copies of a shape with exponential alpha decay
// trail_pos/trail_angle are ring-buffer arrays of length trail_len
// trail_head is the index of the most recent entry
void vg_draw_shape_trail(Shape *shape, Vec2 *trail_pos, float *trail_angle,
                         int trail_len, int trail_head, float scale,
                         float base_alpha, float decay);

// Apply persistence effect (the Vectrex glow/trail)
void vg_apply_persistence(float fade_amount);

// Clear the persistence buffer
void vg_clear();

// Present the graphics
void vg_present();

// Set screen shake offset
void vg_set_shake(int dx, int dy);

#endif
