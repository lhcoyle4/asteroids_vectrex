# FULIGIN — C Coding Style Guide

*A style guide for a vector-graphics arcade game set at the end of the world.*
*Draws from NASA C Coding Standard, Linux Kernel Coding Style, Google C Style Guide, and Barr Group Embedded C Coding Standard.*

---

## 1. File Organization

Every `.c` and `.h` file begins with a block comment header:

```c
/*
 * game.c
 *
 * Core game logic: state machine, entity update, collision, and rendering.
 *
 * Author: <name>
 * Date:   2025-05-25
 */
```

One module per `.c`/`.h` pair. A module owns one coherent responsibility (e.g. `audio`, `vector_graphics`, `game`). Cross-module state is passed explicitly or through well-defined globals declared in one place and `extern`-referenced everywhere else.

**Include order** — system headers first, then SDL2, then local headers. Each group separated by a blank line:

```c
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "game.h"
#include "vector_graphics.h"
#include "audio.h"
```

Public declarations (types, constants, function prototypes) live in the `.h` file. Private declarations (static helpers, internal types) live at the top of the `.c` file. Never put a `static` function prototype in a header.

---

## 2. Naming Conventions

| Category | Style | Example |
|---|---|---|
| Constants / macros | `SCREAMING_SNAKE_CASE` | `MAX_ASTEROIDS`, `BULLET_SPEED` |
| Types (structs, enums, typedefs) | `PascalCase` | `ShipEntity`, `GameState`, `Particle` |
| Functions | `snake_case`, verb-first | `spawn_asteroid`, `game_update`, `audio_play_sfx` |
| Variables | `snake_case` | `delta_time`, `spawn_timer` |
| Boolean flags | `snake_case` with `is_`, `has_`, or `can_` prefix | `is_active`, `has_shield`, `can_fire` |

Never use a leading underscore (`_var` or `__var`). Those names are reserved by the C standard and POSIX.

Enum variants are prefixed with their type name to avoid collisions:

```c
typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAMEOVER
} GameState;
```

---

## 3. Function Design

Each function does one thing. If you need a comment to divide a function into named phases, split it into two functions instead.

**Length:** keep functions under ~100 lines. If a function is longer, look for a natural seam to extract a static helper.

**All file-internal helpers are `static`:**

```c
/* Declared as a forward declaration at the top of the .c file: */
static void wrap_position(Vec2 *pos);

/* Implemented further down: */
static void wrap_position(Vec2 *pos) {
    if (pos->x < 0.0f) pos->x += SCREEN_WIDTH;
    if (pos->x > SCREEN_WIDTH) pos->x -= SCREEN_WIDTH;
    if (pos->y < 0.0f) pos->y += SCREEN_HEIGHT;
    if (pos->y > SCREEN_HEIGHT) pos->y -= SCREEN_HEIGHT;
}
```

Forward-declare every `static` function in a block near the top of the `.c` file, before the first implementation. This keeps the reading order flexible without requiring forward-ordering of definitions.

**Public functions validate their inputs at entry:**

```c
void game_handle_input(SDL_Event *event) {
    if (!event) {
        SDL_Log("game_handle_input: NULL event pointer");
        return;
    }
    /* ... */
}
```

---

## 4. Documentation

**Public functions** use Doxygen-style block comments in the `.h` file:

```c
/**
 * @brief Spawn a new asteroid at a random screen edge.
 *
 * @param scale  Size multiplier: 1.0 = large, 0.5 = medium, 0.25 = small.
 * @param origin Position to split from, or NULL for a random edge spawn.
 * @return       Index of the new asteroid in the pool, or -1 if the pool is full.
 */
int spawn_asteroid(float scale, const Vec2 *origin);
```

**Static helpers** get a single-line brief:

```c
/** @brief Clamp a float value to [lo, hi]. */
static float clampf(float v, float lo, float hi);
```

**Complex algorithms** get a plain-English block comment placed *before* the code:

```c
/*
 * Broad-phase collision: for each bullet, only check asteroids whose
 * grid cell overlaps the bullet's cell. This reduces O(n*m) to roughly
 * O(n + m) for typical entity counts.
 */
static void check_bullet_asteroid_collisions(void) { /* ... */ }
```

**Do not comment every line.** Comment only non-obvious logic — the *why*, not the *what*. Code like `pos.x += vel.x * dt;` needs no comment.

**Magic numbers are forbidden** for any value with domain meaning. Use a named constant:

```c
/* Wrong */
if (score > 10000) grant_extra_life();

/* Right */
#define EXTRA_LIFE_THRESHOLD 10000
if (score > EXTRA_LIFE_THRESHOLD) grant_extra_life();
```

---

## 5. Error Handling

Every allocation is checked. Never silently discard a NULL return:

```c
ShipEntity *ship = malloc(sizeof(ShipEntity));
if (!ship) {
    SDL_Log("spawn_ship: malloc failed");
    return NULL;
}
```

Every SDL creation call is checked, with a descriptive log message:

```c
g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
if (!g_renderer) {
    SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 1;
}
```

Functions that can fail return an error indicator. Use `int` with `0` for success and a negative value (or `NULL` for pointers) for failure. Do not use `void` return types for functions that allocate, open files, or call SDL create APIs.

---

## 6. Code Formatting

**Brace style:** K&R — opening brace on the same line as the control statement. `else` on the same line as the closing brace:

```c
if (asteroid->is_active) {
    update_asteroid(asteroid, dt);
} else {
    continue;
}
```

**Indentation:** 4 spaces. No tabs anywhere.

**Line length:** 100 characters maximum. Break long expressions at a logical operator or after a comma, indenting the continuation by 4 spaces:

```c
g_window = SDL_CreateWindow(
    "PERMADRIFT",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    SCREEN_WIDTH, SCREEN_HEIGHT,
    SDL_WINDOW_SHOWN
);
```

**Pointer declarator:** asterisk belongs with the variable name, not the type:

```c
int *ptr;          /* correct */
SDL_Event *event;  /* correct */
int* ptr;          /* wrong   */
```

**No trailing whitespace** on any line.

**Blank lines** separate logical sections within a function body. Do not use blank lines to pad short functions.

---

## 7. Dead Code Policy

Remove unused variables, functions, structs, and enum variants. Do not comment out old code — it rots, confuses readers, and pollutes diffs. If removing something would break a caller, fix the caller.

```c
/* Wrong — commented-out dead code */
// static void old_spawn_logic(void) { ... }

/* Right — it's gone. If it was needed, git history has it. */
```

If a function parameter is intentionally unused (e.g. a callback stub), suppress the warning with an explicit cast, not a comment:

```c
void game_focus_changed(int has_focus) {
    (void)has_focus; /* focus state is handled at the SDL event level */
}
```

---

## 8. Section Organization Within `.c` Files

Use banner dividers to separate logical groups within a `.c` file. Suggested order:

```c
/* =========================================================
 * INCLUDES
 * ========================================================= */

/* =========================================================
 * CONSTANTS
 * ========================================================= */

/* =========================================================
 * TYPES
 * ========================================================= */

/* =========================================================
 * FORWARD DECLARATIONS
 * ========================================================= */

/* =========================================================
 * MODULE-SCOPE STATE
 * ========================================================= */

/* =========================================================
 * IMPLEMENTATION
 * ========================================================= */
```

Module-scope state (file-scope `static` variables) goes in its own named section, not scattered among functions. Group related state together — e.g. all asteroid pool state in one block, all ship state in another.

---

## 9. Game-Specific Conventions

**Physics constants** are grouped in their own named block, never scattered inline:

```c
/* =========================================================
 * PHYSICS CONSTANTS
 * ========================================================= */
#define FRICTION        0.99f
#define ROTATION_SPEED  4.5f
#define THRUST_FORCE    350.0f
#define BULLET_SPEED    600.0f
#define BULLET_LIFE     1.2f
```

**UI layout values** (pixel positions, sizes, offsets) are named constants, never raw literals:

```c
#define HUD_SCORE_X     20
#define HUD_SCORE_Y     20
#define HUD_LIVES_X     20
#define HUD_LIVES_Y     50
#define TITLE_CENTER_Y  380
```

**Entity pools** use a consistent `is_active` flag. All pool iteration follows the same pattern:

```c
for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (!asteroids[i].is_active) continue;
    update_asteroid(&asteroids[i], dt);
}
```

**Time-based animation** always uses accumulated `game_time` or the `dt` argument passed to `game_update`. Never call `SDL_GetTicks()` inside game logic functions. `SDL_GetTicks()` belongs only in the main loop for delta-time computation.

```c
/* Wrong — frame-rate dependent, not portable to pause/timescale */
float flicker = sinf(SDL_GetTicks() * 0.005f);

/* Right — driven by accumulated game time, pauses correctly */
float flicker = sinf(game_time * 5.0f);
```

No frame-rate-dependent values. Any motion, decay, or animation that involves a rate must be multiplied by `dt`:

```c
asteroid->pos.x += asteroid->vel.x * dt;   /* correct */
asteroid->pos.x += asteroid->vel.x * 0.016f; /* wrong — assumes 60 fps */
```

---

## 10. Version Control Notes

**Commit messages** use imperative mood and describe *what* changed and briefly *why*:

```
Add mine-sweep powerup to upgrade pool

The shop lacked any defensive option at high difficulty. The mine-sweep
clears all active mines on pickup, giving the player a recoverable path
when the field becomes saturated.
```

```
Fix asteroid split velocity scaling by size

Large splits were inheriting full parent speed, making them harder than
small ones. Velocity is now divided by the child scale factor.
```

Dead code removal is its own atomic commit, separate from feature work:

```
Remove unused attract_mode_timer and legacy warp stubs

These were left over from the warp-menu refactor and no longer referenced
anywhere. Removed to keep the module state section clean.
```

Do not bundle dead code removal inside a feature commit — it makes the diff harder to review.
