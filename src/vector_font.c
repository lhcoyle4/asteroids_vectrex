#include "vector_font.h"
#include <ctype.h>
#include <string.h>

#define MAX_CHAR_LINES 8

typedef struct {
    float x1, y1, x2, y2;
} FontLine;

typedef struct {
    FontLine lines[MAX_CHAR_LINES];
    int line_count;
} FontChar;

static FontChar font_chars[256];
static int font_initialized = 0;

static void define_char(char c, int count, ...) {
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

void vf_init() {
    if (font_initialized) return;

    memset(font_chars, 0, sizeof(font_chars));

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
    define_char('5', 5,
        0.0, 0.0, 1.0, 0.0,
        1.0, 0.0, 1.0, 1.0,
        1.0, 1.0, 0.0, 1.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.5, 1.0, 0.5
    );
    // Let's re-write '8' correctly (there was a typo define_char('5', ...) above)
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
    // < (lives)
    define_char('<', 2,
        0.8, 0.2, 0.2, 0.5,
        0.2, 0.5, 0.8, 0.8
    );
    // >
    define_char('>', 2,
        0.2, 0.2, 0.8, 0.5,
        0.8, 0.5, 0.2, 0.8
    );

    font_initialized = 1;
}

void vf_draw_char(char c, float x, float y, float scale, SDL_Color color) {
    if (!font_initialized) vf_init();

    // Map character
    c = toupper((unsigned char)c);
    FontChar fc = font_chars[(unsigned char)c];
    if (fc.line_count == 0 && c != ' ') {
        // Fallback to square
        define_char('?', 4,
            0.0, 0.0, 1.0, 0.0,
            1.0, 0.0, 1.0, 1.0,
            1.0, 1.0, 0.0, 1.0,
            0.0, 1.0, 0.0, 0.0
        );
        fc = font_chars['?'];
    }

    if (fc.line_count > 0) {
        // Build a temporary shape to use vg_draw_shape
        Line *lines = malloc(fc.line_count * sizeof(Line));
        for (int i = 0; i < fc.line_count; i++) {
            lines[i].p1.x = fc.lines[i].x1 - 0.5f;
            lines[i].p1.y = fc.lines[i].y1 - 0.5f;
            lines[i].p2.x = fc.lines[i].x2 - 0.5f;
            lines[i].p2.y = fc.lines[i].y2 - 0.5f;
        }

        Shape s;
        s.lines = lines;
        s.line_count = fc.line_count;
        s.color = color;

        // Shift pos to center since we subtracted 0.5f to center characters
        Vec2 pos = { x + scale * 0.5f, y + scale * 0.5f };
        vg_draw_shape(&s, pos, 0.0f, scale);

        free(lines);
    }
}

void vf_draw_string(const char *str, float x, float y, float scale, SDL_Color color) {
    float cur_x = x;
    int len = (int)strlen(str);
    for (int i = 0; i < len; i++) {
        if (str[i] != ' ') {
            vf_draw_char(str[i], cur_x, y, scale, color);
        }
        cur_x += scale * 1.2f; // Space between characters
    }
}

void vf_draw_string_centered(const char *str, float x, float y, float scale, SDL_Color color) {
    int len = (int)strlen(str);
    float width = (len * scale * 1.2f) - (scale * 0.2f);
    vf_draw_string(str, x - width / 2.0f, y, scale, color);
}
