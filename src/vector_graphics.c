#include "vector_graphics.h"
#include <math.h>
#include <stdlib.h>

static SDL_Renderer *vg_renderer = NULL;
static SDL_Texture *persistence_tex = NULL;
static int win_w, win_h;
static Vec2 vg_camera_offset = {0.0f, 0.0f};
static float vg_zoom = 1.0f;
static float vg_brightness = 1.0f;
static int vg_monochrome = 0;
static float vg_chromatic_aberration = 0.0f;

void vg_set_camera(Vec2 offset) {
    vg_camera_offset = offset;
}

void vg_set_zoom(float zoom) {
    vg_zoom = zoom;
}

void vg_set_brightness(int level) {
    vg_brightness = level / 4.0f;
}

void vg_set_monochrome(int enable) {
    vg_monochrome = enable;
}

void vg_set_chromatic_aberration(float strength) {
    vg_chromatic_aberration = strength;
}

void vg_init(SDL_Renderer *renderer, int width, int height) {
    vg_renderer = renderer;
    win_w = width;
    win_h = height;

    /* Try ABGR8888 first — friendlier to WebGL/little-endian. */
    persistence_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
                                        SDL_TEXTUREACCESS_TARGET, width, height);
    if (!persistence_tex) {
        SDL_Log("vg_init: ABGR8888 target texture failed (%s), trying RGBA8888", SDL_GetError());
        persistence_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_TARGET, width, height);
    }
    if (persistence_tex) {
        SDL_SetTextureBlendMode(persistence_tex, SDL_BLENDMODE_BLEND);
        SDL_Log("vg_init: persistence texture created OK (%dx%d)", width, height);
    } else {
        SDL_Log("vg_init: all texture formats failed (%s) — phosphor effect disabled", SDL_GetError());
    }
}

void vg_apply_persistence(float fade_amount) {
    if (!persistence_tex) {
        /* Fallback: no offscreen texture — clear each frame so draws are fresh. */
        SDL_SetRenderDrawColor(vg_renderer, 0, 0, 0, 255);
        SDL_RenderClear(vg_renderer);
        return;
    }
    SDL_SetRenderTarget(vg_renderer, persistence_tex);

    // Dim the existing buffer for the "phosphor decay" effect
    SDL_SetRenderDrawBlendMode(vg_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(vg_renderer, 0, 0, 0, (Uint8)(fade_amount * 255));
    SDL_Rect rect = {0, 0, win_w, win_h};
    SDL_RenderFillRect(vg_renderer, &rect);

    SDL_SetRenderTarget(vg_renderer, NULL);
}

void vg_draw_shape(Shape *shape, Vec2 pos, float angle, float scale) {
    if (persistence_tex) SDL_SetRenderTarget(vg_renderer, persistence_tex);
    
    SDL_Color color = shape->color;
    if (vg_monochrome) {
        Uint8 gray = (Uint8)(0.299f * color.r + 0.587f * color.g + 0.114f * color.b);
        color.r = gray;
        color.g = gray;
        color.b = gray;
    }
    
    SDL_SetRenderDrawColor(vg_renderer, color.r, color.g, color.b, color.a);
    SDL_SetRenderDrawBlendMode(vg_renderer, SDL_BLENDMODE_ADD);

    float s = sinf(angle);
    float c = cosf(angle);

    for (int i = 0; i < shape->line_count; i++) {
        Line l = shape->lines[i];
        
        // Transform points
        float x1 = (l.p1.x * c - l.p1.y * s) * scale * vg_zoom + pos.x - vg_camera_offset.x;
        float y1 = (l.p1.x * s + l.p1.y * c) * scale * vg_zoom + pos.y - vg_camera_offset.y;
        float x2 = (l.p2.x * c - l.p2.y * s) * scale * vg_zoom + pos.x - vg_camera_offset.x;
        float y2 = (l.p2.x * s + l.p2.y * c) * scale * vg_zoom + pos.y - vg_camera_offset.y;

        if (vg_chromatic_aberration > 0.0f) {
            // Draw red pass offset left
            SDL_SetRenderDrawColor(vg_renderer, color.r, 0, 0, color.a);
            SDL_RenderDrawLine(vg_renderer, (int)(x1 - vg_chromatic_aberration), (int)y1, (int)(x2 - vg_chromatic_aberration), (int)y2);

            // Draw green pass centered
            SDL_SetRenderDrawColor(vg_renderer, 0, color.g, 0, color.a);
            SDL_RenderDrawLine(vg_renderer, (int)x1, (int)y1, (int)x2, (int)y2);

            // Draw blue pass offset right
            SDL_SetRenderDrawColor(vg_renderer, 0, 0, color.b, color.a);
            SDL_RenderDrawLine(vg_renderer, (int)(x1 + vg_chromatic_aberration), (int)y1, (int)(x2 + vg_chromatic_aberration), (int)y2);
        } else {
            SDL_RenderDrawLine(vg_renderer, (int)x1, (int)y1, (int)x2, (int)y2);
            
            // Draw a second time for a slight bloom effect
            SDL_SetRenderDrawColor(vg_renderer, color.r / 2, color.g / 2, color.b / 2, 100);
            SDL_RenderDrawLine(vg_renderer, (int)x1+1, (int)y1, (int)x2+1, (int)y2);
            SDL_RenderDrawLine(vg_renderer, (int)x1, (int)y1+1, (int)x2, (int)y2+1);
            SDL_SetRenderDrawColor(vg_renderer, color.r, color.g, color.b, color.a);
        }
    }

    if (persistence_tex) SDL_SetRenderTarget(vg_renderer, NULL);
}

void vg_draw_shape_trail(Shape *shape, Vec2 *trail_pos, float *trail_angle,
                         int trail_len, int trail_head, float scale,
                         float base_alpha, float decay) {
    if (persistence_tex) SDL_SetRenderTarget(vg_renderer, persistence_tex);
    SDL_SetRenderDrawBlendMode(vg_renderer, SDL_BLENDMODE_ADD);

    SDL_Color color = shape->color;
    if (vg_monochrome) {
        Uint8 gray = (Uint8)(0.299f * color.r + 0.587f * color.g + 0.114f * color.b);
        color.r = gray;
        color.g = gray;
        color.b = gray;
    }

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

        for (int i = 0; i < shape->line_count; i++) {
            Line l = shape->lines[i];
            float x1 = (l.p1.x * c - l.p1.y * s) * scale * vg_zoom + pos.x - vg_camera_offset.x;
            float y1 = (l.p1.x * s + l.p1.y * c) * scale * vg_zoom + pos.y - vg_camera_offset.y;
            float x2 = (l.p2.x * c - l.p2.y * s) * scale * vg_zoom + pos.x - vg_camera_offset.x;
            float y2 = (l.p2.x * s + l.p2.y * c) * scale * vg_zoom + pos.y - vg_camera_offset.y;

            if (vg_chromatic_aberration > 0.0f) {
                SDL_SetRenderDrawColor(vg_renderer, color.r, 0, 0, (Uint8)a);
                SDL_RenderDrawLine(vg_renderer, (int)(x1 - vg_chromatic_aberration), (int)y1, (int)(x2 - vg_chromatic_aberration), (int)y2);

                SDL_SetRenderDrawColor(vg_renderer, 0, color.g, 0, (Uint8)a);
                SDL_RenderDrawLine(vg_renderer, (int)x1, (int)y1, (int)x2, (int)y2);

                SDL_SetRenderDrawColor(vg_renderer, 0, 0, color.b, (Uint8)a);
                SDL_RenderDrawLine(vg_renderer, (int)(x1 + vg_chromatic_aberration), (int)y1, (int)(x2 + vg_chromatic_aberration), (int)y2);
            } else {
                SDL_SetRenderDrawColor(vg_renderer, color.r, color.g, color.b, (Uint8)a);
                SDL_RenderDrawLine(vg_renderer, (int)x1, (int)y1, (int)x2, (int)y2);
            }
        }
    }

    if (persistence_tex) SDL_SetRenderTarget(vg_renderer, NULL);
}

void vg_clear() {
    if (persistence_tex) {
        SDL_SetRenderTarget(vg_renderer, persistence_tex);
        SDL_SetRenderDrawColor(vg_renderer, 0, 0, 0, 255);
        SDL_RenderClear(vg_renderer);
        SDL_SetRenderTarget(vg_renderer, NULL);
    } else {
        SDL_SetRenderDrawColor(vg_renderer, 0, 0, 0, 255);
        SDL_RenderClear(vg_renderer);
    }
}

static int shake_dx = 0;
static int shake_dy = 0;

void vg_set_shake(int dx, int dy) {
    shake_dx = dx;
    shake_dy = dy;
}

void vg_present() {
    if (persistence_tex) {
        /* Blit offscreen buffer to screen (with optional screen-shake offset). */
        SDL_SetRenderDrawColor(vg_renderer, 0, 0, 0, 255);
        SDL_RenderClear(vg_renderer);
        if (shake_dx != 0 || shake_dy != 0) {
            SDL_Rect dest = {shake_dx, shake_dy, win_w, win_h};
            SDL_RenderCopy(vg_renderer, persistence_tex, NULL, &dest);
        } else {
            SDL_RenderCopy(vg_renderer, persistence_tex, NULL, NULL);
        }
    }
    /* If no persistence_tex, drawing went directly to the screen — just present. */
    SDL_RenderPresent(vg_renderer);
}
