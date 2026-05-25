/*
 * vector_font.c
 *
 * Purpose: Custom vector/stroke font for rendering text as line segments.
 *          Each character is defined as a set of line segments in a
 *          normalized [0.0, 1.0] coordinate space.
 *
 * Author:  <placeholder>
 * Date:    2026-05-25
 */

#include "vector_font.h"
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

/* Maximum number of line segments per character glyph.
 * NOTE: Characters defined with more than MAX_CHAR_LINES segments will be
 * silently truncated to this limit. If new glyphs exceed this, increase the
 * constant and recompile. */
#define MAX_CHAR_LINES 8

/* Character spacing: each glyph advances by (scale * FONT_CHAR_SPACING). */
#define FONT_CHAR_SPACING 1.2f

typedef struct {
    float x1, y1, x2, y2;
} FontLine;

typedef struct {
    FontLine lines[MAX_CHAR_LINES];
    int line_count;
} FontChar;

static FontChar font_chars[256];
static FontChar g_unknown_char;   /* fallback square for unrecognized glyphs */
static int font_initialized = 0;

/**
 * @brief Registers line segment data for a character in a normalized
 *        [0.0, 1.0] coordinate space.  Called during vf_init() to populate
 *        the global font_chars table.
 *
 * @param c     The character to define.
 * @param count Number of line segments that follow (capped at MAX_CHAR_LINES).
 * @param ...   Variadic float pairs: x1, y1, x2, y2 for each segment,
 *              passed as doubles (C variadic promotion).
 */
static void define_char(char c, int count, ...)
{
    va_list args;
    va_start(args, count);
    FontChar fc;
    fc.line_count = count > MAX_CHAR_LINES ? MAX_CHAR_LINES : count;
    for (int i = 0; i < fc.line_count; i++) {
        fc.lines[i].x1 = (float)va_arg(args, double);
        fc.lines[i].y1 = (float)va_arg(args, double);
        fc.lines[i].x2 = (float)va_arg(args, double);
        fc.lines[i].y2 = (float)va_arg(args, double);
    }
    va_end(args);
    font_chars[(unsigned char)c] = fc;
}

/**
 * @brief Initializes the vector font system, defining all character glyphs.
 *        Safe to call multiple times; initialization only runs once.
 */
void vf_init(void)
{
    if (font_initialized) return;

    memset(font_chars, 0, sizeof(font_chars));

    /* ------------------------------------------------------------------
     * Uppercase letters
     * ------------------------------------------------------------------ */

    // A
    define_char('A', 3,
        0.0, 1.0, 0.5, 0.0,
        0.5, 0.0, 1.0, 1.0,
        0.25, 0.5, 0.75, 0.5
    );
    // B
    define_char('B', 7,
        0.0, 0.0, 0.0, 1.0,
        0.0, 0.0, 0.75, 0.0,
        0.75, 0.0, 0.75, 0.5,
        0.75, 0.5, 0.0, 0.5,
        0.0, 0.5, 0.85, 0.5,
        0.85, 0.5, 0.85, 1.0,
        0.85, 1.0, 0.0, 1.0
    );
    // C
    define_char('C', 3,
        1.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
        0.0, 1.0, 1.0, 1.0
    );
    // D
    define_char('D', 4,
        0.0, 0.0, 0.0, 1.0,
        0.0, 0.0, 0.7, 0.3,
        0.7, 0.3, 0.7, 0.7,
        0.7, 0.7, 0.0, 1.0
    );
    // E
    define_char('E', 4,
        0.0, 0.0, 0.0, 1.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.5, 0.75, 0.5,
        0.0, 1.0, 1.0, 1.0
    );
    // F
    define_char('F', 3,
        0.0, 0.0, 0.0, 1.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.5, 0.75, 0.5
    );
    // G
    define_char('G', 5,
        1.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
        0.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 0.5,
        1.0, 0.5, 0.6, 0.5
    );
    // H
    define_char('H', 3,
        0.0, 0.0, 0.0, 1.0,
        1.0, 0.0, 1.0, 1.0,
        0.0, 0.5, 1.0, 0.5
    );
    // I
    define_char('I', 3,
        0.5, 0.0, 0.5, 1.0,
        0.2, 0.0, 0.8, 0.0,
        0.2, 1.0, 0.8, 1.0
    );
    // J
    define_char('J', 3,
        0.8, 0.0, 0.8, 0.8,
        0.8, 0.8, 0.4, 1.0,
        0.4, 1.0, 0.0, 0.8
    );
    // K
    define_char('K', 3,
        0.0, 0.0, 0.0, 1.0,
        0.0, 0.5, 0.8, 0.0,
        0.0, 0.5, 0.8, 1.0
    );
    // L
    define_char('L', 2,
        0.0, 0.0, 0.0, 1.0,
        0.0, 1.0, 0.8, 1.0
    );
    // M
    define_char('M', 4,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 0.5, 0.5,
        0.5, 0.5, 1.0, 0.0,
        1.0, 0.0, 1.0, 1.0
    );
    // N
    define_char('N', 3,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 0.0
    );
    // O
    define_char('O', 4,
        0.0, 0.0, 1.0, 0.0,
        1.0, 0.0, 1.0, 1.0,
        1.0, 1.0, 0.0, 1.0,
        0.0, 1.0, 0.0, 0.0
    );
    // P
    define_char('P', 4,
        0.0, 0.0, 0.0, 1.0,
        0.0, 0.0, 1.0, 0.0,
        1.0, 0.0, 1.0, 0.5,
        1.0, 0.5, 0.0, 0.5
    );
    // Q
    define_char('Q', 5,
        0.0, 0.0, 1.0, 0.0,
        1.0, 0.0, 1.0, 1.0,
        1.0, 1.0, 0.0, 1.0,
        0.0, 1.0, 0.0, 0.0,
        0.6, 0.6, 1.0, 1.0
    );
    // R
    define_char('R', 5,
        0.0, 0.0, 0.0, 1.0,
        0.0, 0.0, 1.0, 0.0,
        1.0, 0.0, 1.0, 0.5,
        1.0, 0.5, 0.0, 0.5,
        0.3, 0.5, 1.0, 1.0
    );
    // S
    define_char('S', 5,
        1.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.5,
        0.0, 0.5, 1.0, 0.5,
        1.0, 0.5, 1.0, 1.0,
        1.0, 1.0, 0.0, 1.0
    );
    // T
    define_char('T', 2,
        0.5, 0.0, 0.5, 1.0,
        0.0, 0.0, 1.0, 0.0
    );
    // U
    define_char('U', 3,
        0.0, 0.0, 0.0, 1.0,
        0.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 0.0
    );
    // V
    define_char('V', 2,
        0.0, 0.0, 0.5, 1.0,
        0.5, 1.0, 1.0, 0.0
    );
    // W
    define_char('W', 4,
        0.0, 0.0, 0.25, 1.0,
        0.25, 1.0, 0.5, 0.5,
        0.5, 0.5, 0.75, 1.0,
        0.75, 1.0, 1.0, 0.0
    );
    // X
    define_char('X', 2,
        0.0, 0.0, 1.0, 1.0,
        1.0, 0.0, 0.0, 1.0
    );
    // Y
    define_char('Y', 3,
        0.0, 0.0, 0.5, 0.5,
        1.0, 0.0, 0.5, 0.5,
        0.5, 0.5, 0.5, 1.0
    );
    // Z
    define_char('Z', 3,
        0.0, 0.0, 1.0, 0.0,
        1.0, 0.0, 0.0, 1.0,
        0.0, 1.0, 1.0, 1.0
    );

    /* ------------------------------------------------------------------
     * Digits
     * ------------------------------------------------------------------ */

    // 0
    define_char('0', 5,
        0.0, 0.0, 1.0, 0.0,
        1.0, 0.0, 1.0, 1.0,
        1.0, 1.0, 0.0, 1.0,
        0.0, 1.0, 0.0, 0.0,
        1.0, 0.0, 0.0, 1.0
    );
    // 1
    define_char('1', 2,
        0.5, 0.0, 0.5, 1.0,
        0.3, 0.2, 0.5, 0.0
    );
    // 2
    define_char('2', 5,
        0.0, 0.0, 1.0, 0.0,
        1.0, 0.0, 1.0, 0.5,
        1.0, 0.5, 0.0, 0.5,
        0.0, 0.5, 0.0, 1.0,
        0.0, 1.0, 1.0, 1.0
    );
    // 3
    define_char('3', 4,
        0.0, 0.0, 1.0, 0.0,
        1.0, 0.0, 1.0, 1.0,
        1.0, 1.0, 0.0, 1.0,
        0.0, 0.5, 1.0, 0.5
    );
    // 4
    define_char('4', 3,
        0.0, 0.0, 0.0, 0.5,
        0.0, 0.5, 1.0, 0.5,
        1.0, 0.0, 1.0, 1.0
    );
    // 5
    define_char('5', 5,
        1.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.5,
        0.0, 0.5, 1.0, 0.5,
        1.0, 0.5, 1.0, 1.0,
        1.0, 1.0, 0.0, 1.0
    );
    // 6
    define_char('6', 5,
        1.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
        0.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 0.5,
        1.0, 0.5, 0.0, 0.5
    );
    // 7
    define_char('7', 2,
        0.0, 0.0, 1.0, 0.0,
        1.0, 0.0, 0.5, 1.0
    );
    // 8
    define_char('8', 5,
        0.0, 0.0, 1.0, 0.0,
        1.0, 0.0, 1.0, 1.0,
        1.0, 1.0, 0.0, 1.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.5, 1.0, 0.5
    );
    // 9
    define_char('9', 5,
        0.0, 0.0, 1.0, 0.0,
        1.0, 0.0, 1.0, 1.0,
        1.0, 1.0, 0.0, 1.0,
        0.0, 0.0, 0.0, 0.5,
        0.0, 0.5, 1.0, 0.5
    );

    /* ------------------------------------------------------------------
     * Symbols
     * ------------------------------------------------------------------ */

    // -
    define_char('-', 1,
        0.2, 0.5, 0.8, 0.5
    );
    // /
    define_char('/', 1,
        0.1, 1.0, 0.9, 0.0
    );
    // :
    define_char(':', 2,
        0.5, 0.3, 0.5, 0.35,
        0.5, 0.7, 0.5, 0.75
    );
    // .
    define_char('.', 1,
        0.5, 0.9, 0.5, 0.95
    );
    // <
    define_char('<', 2,
        0.8, 0.2, 0.2, 0.5,
        0.2, 0.5, 0.8, 0.8
    );
    // >
    define_char('>', 2,
        0.2, 0.2, 0.8, 0.5,
        0.8, 0.5, 0.2, 0.8
    );

    /* Pre-build the fallback square used for unrecognized characters. */
    FontChar unk;
    unk.line_count = 4;
    unk.lines[0] = (FontLine){0.0f, 0.0f, 1.0f, 0.0f};
    unk.lines[1] = (FontLine){1.0f, 0.0f, 1.0f, 1.0f};
    unk.lines[2] = (FontLine){1.0f, 