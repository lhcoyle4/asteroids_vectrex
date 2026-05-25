/*
 * vector_graphics.h
 *
 * Purpose : Public interface for the FULIGIN vector-graphics renderer.
 *           Declare types and functions used to draw retro line-based shapes
 *           with phosphor persistence, chromatic aberration, screen shake,
 *           monochrome mode, and bloom effects.
 *
 * Author  : FULIGIN contributors
 * Date    : 2026-05-25
 */

#ifndef VECTOR_GRAPHICS_H
#define VECTOR_GRAPHICS_H

#include <SDL2/SDL.h>

/* ====== TYPES ====== */

/** @brief A 2-D point or vector in world space (floating-point). */
typedef struct { float x, y; } Vec2;

/** @brief A single line segment defined by two endpoints. */
typedef struct { Vec2 p1, p2; } Line;

/**
 * @brief A renderable shape: a collection of line segments with a base colour.
 *
 * All lines share the same colour; per-line colouring is not supported.
 * The lines array must remain valid for the lifeti