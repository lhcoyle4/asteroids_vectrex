#ifndef VECTOR_GRAPHICS_H
#define VECTOR_GRAPHICS_H

#include <SDL2/SDL.h>

typedef struct { float x, y; } Vec2;
typedef struct { Vec2 p1, p2; } Line;
typedef struct { Line *lines; int line_count; SDL_Color color; } Shape;

void vg_init(SDL_Renderer *renderer, int width, int height);
void vg_draw_shape(Shape *shape, Vec2 pos, float angle, float scale);
void vg_draw_shape_trail(Shape *shape, Vec2 *trail_pos, float *trail_angle,
                         int trail_len, int trail_head, float scale,
                         float base_alpha, float decay);
void vg_set_camera(Vec2 offset);
void vg_set_zoom(float zoom);
void vg_apply_persistence(float fade_amount);
void vg_clear();
void vg_present();
void vg_set_shake(int dx, int dy);
void vg_set_brightness(int level);

#endif /* VECTOR_GRAPHICS_H */
