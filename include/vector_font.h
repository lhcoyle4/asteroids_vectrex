/*
 * vector_font.h
 *
 * Purpose: Public interface for the vector/stroke font system.
 *          Characters are defined as sets of line segments in a normalized
 *          [0.0, 1.0] coordinate space and rendered via the vector graphics
 *          layer.
 *
 * Author:  <placeholder>
 * Date:    2026-05-25
 */

#ifndef VECTOR_FONT_H
#define VECTOR_FONT_H

#include "vector_graphics.h"

/**
 * @brief Initializes the vector font system and registers all character
 *        glyphs.  Safe to call multiple times; work is only performed on the
 *        first call.
 */
void 