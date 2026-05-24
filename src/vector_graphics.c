#include "vector_graphics.h"
#include <math.h>
#include <stdlib.h>

static SDL_Renderer *vg_renderer = NULL;
static SDL_Texture *persistence_tex = NULL;
static int win_w, win_h;
static Vec2 vg_camera_offset = {0.0f, 0.0f};

void vg_set_camera(Vec2 offset) {
    vg_camera_offset = offset;
}

void vg_init(SDL_Renderer *renderer, int width, int height) {
    vg_renderer = renderer;
    win_w = width;
    win_h = height;

    persistence_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
    SDL_SetTextureBlendMode(persistence_tex, SDL_BLENDMODE_BLEND);
}

void vg_apply_persistence(float fade_amount) {
    SDL_SetRenderTarget(vg_renderer, persistence_tex);
    
    // Dim the existing buffer for the "phosphor decay" effect
    SDL_SetRenderDrawBlendMode(vg_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(vg_renderer, 0, 0, 0, (Uint8)(fade_amount * 255)); 
    SDL_Rect rect = {0, 0, win_w, win_h};
    SDL_RenderFillRect(vg_renderer, &rect);

    SDL_SetRenderTarget(vg_renderer, NULL);
}

void vg_draw_shape(Shape *shape, Vec2 pos, float angle, float scale) {
    SDL_SetRenderTarget(vg_renderer, persistence_tex);
    SDL_SetRenderDrawColor(vg_renderer, shape->color.r, shape->color.g, shape->color.b, shape->color.a);
    SDL_SetRenderDrawBlendMode(vg_renderer, SDL_BLENDMODE_ADD);

    float s = sinf(angle);
    float c = cosf(angle);

    for (int i = 0; i < shape->line_count; i++) {
        Line l = shape->lines[i];
        
        // Transform points
        float x1 = (l.p1.x * c - l.p1.y * s) * scale + pos.x - vg_camera_offset.x;
        float y1 = (l.p1.x * s + l.p1.y * c) * scale + pos.y - vg_camera_offset.y;
        float x2 = (l.p2.x * c - l.p2.y * s) * scale + pos.x - vg_camera_offset.x;
        float y2 = (l.p2.x * s + l.p2.y * c) * scale + pos.y - vg_camera_offset.y;

        SDL_RenderDrawLine(vg_renderer, (int)x1, (int)y1, (int)x2, (int)y2);
        
        // Draw a second time for a slight bloom effect
        SDL_SetRenderDrawColor(vg_renderer, shape->color.r / 2, shape->color.g / 2, shape->color.b / 2, 100);
        SDL_RenderDrawLine(vg_renderer, (int)x1+1, (int)y1, (int)x2+1, (int)y2);
        SDL_RenderDrawLine(vg_renderer, (int)x1, (int)y1+1, (int)x2, (int)y2+1);
        SDL_SetRenderDrawColor(vg_renderer, shape->color.r, shape->color.g, shape->color.b, shape->color.a);
    }

    SDL_SetRenderTarget(vg_renderer, NULL);
}

void vg_draw_shape_trail(Shape *shape, Vec2 *trail_pos, float *trail_angle,
                         int trail_len, int trail_head, float scale,
                         float base_alpha, float decay) {
    SDL_SetRenderTarget(vg_renderer, persistence_tex);
    SDL_SetRenderDrawBlendMode(vg_renderer, SDL_BLENDMODE_ADD);

    for (int step = 1; step < trail_len; step++) {
        float alpha = base_alpha * powf(decay, (float)step);
        if (alpha < 1.0f / 255.0f) break;

        int idx = (trail_head - step + trail_len * 256) % trail_len;
        Vec2 pos = trail_pos[idx];
        float angle = trail_angle[idx];
        if (pos.x == 0.0f && pos.y == 0.0f) continue;

        float s = sinf(angle);
        float c = cosf(angle);
        int a = (int)(alpha * 255.0f);
        if (a > 255) a = 255;

        SDL_SetRenderDrawColor(vg_renderer,
            shape->color.r, shape->color.g, shape->color.b, (Uint8)a);

        for (int i = 0; i < shape->line_count; i++) {
            Line l = shape->lines[i];
            float x1 = (l.p1.x * c - l.p1.y * s) * scale + pos.x - vg_camera_offset.x;
            float y1 = (l.p1.x * s + l.p1.y * c) * scale + pos.y - vg_camera_offset.y;
            float x2 = (l.p2.x * c - l.p2.y * s) * scale + pos.x - vg_camera_offset.x;
            float y2 = (l.p2.x * s + l.p2.y * c) * scale + pos.y - vg_camera_offset.y;
            SDL_RenderDrawLine(vg_renderer, (int)x1, (int)y1, (int)x2, (int)y2);
        }
    }

    SDL_SetRenderTarget(vg_renderer, NULL);
}

void vg_clear() {

    SDL_SetRenderTarget(vg_renderer, persistence_tex);
    SDL_SetRenderDrawColor(vg_renderer, 0, 0, 0, 255);
    SDL_RenderClear(vg_renderer);
    SDL_SetRenderTarget(vg_renderer, NULL);
}

static int shake_dx = 0;
static int shake_dy = 0;

void vg_set_shake(int dx, int dy) {
    shake_dx = dx;
    shake_dy = dy;
}

void vg_present() {
    SDL_SetRenderDrawColor(vg_renderer, 0, 0, 0, 255);
    SDL_RenderClear(vg_renderer);
    if (shake_dx != 0 || shake_dy != 0) {
        SDL_Rect dest = {shake_dx, shake_dy, win_w, win_h};
        SDL_RenderCopy(vg_renderer, persistence_tex, NULL, &dest);
    } else {
        SDL_RenderCopy(vg_renderer, persistence_tex, NULL, NULL);
    }
    SDL_RenderPresent(vg_renderer);
}
