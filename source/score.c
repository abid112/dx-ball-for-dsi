#include <nds.h>
#include <string.h>

#include "score.h"
#include "render.h"

/* 5x7 pixel font. Each glyph is 7 bytes; the low 5 bits of each byte
 * describe one row, MSB-leftmost. Unspecified entries default to zeros
 * (rendered as blank — fine for chars we don't care about).
 *
 * This font powers both the giant centered score (scale 8) and the
 * small bottom-of-screen credit (scale 1). */
static const u8 font5x7[128][7] = {
    [' '] = {0,    0,    0,    0,    0,    0,    0   },
    ['.'] = {0,    0,    0,    0,    0,    0,    0x04},
    [','] = {0,    0,    0,    0,    0,    0x04, 0x08},
    ['<'] = {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02},
    ['>'] = {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08},
    ['/'] = {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10},

    ['0'] = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},
    ['1'] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    ['2'] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},
    ['3'] = {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E},
    ['4'] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
    ['5'] = {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    ['6'] = {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    ['7'] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    ['8'] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    ['9'] = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},

    ['A'] = {0x0E, 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11},
    ['D'] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E},
    ['H'] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},

    ['a'] = {0,    0,    0x0E, 0x01, 0x0F, 0x11, 0x0F},
    ['b'] = {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x1E},
    ['c'] = {0,    0,    0x0E, 0x11, 0x10, 0x11, 0x0E},
    ['d'] = {0x01, 0x01, 0x0F, 0x11, 0x11, 0x11, 0x0F},
    ['e'] = {0,    0,    0x0E, 0x11, 0x1F, 0x10, 0x0E},
    ['f'] = {0x06, 0x09, 0x08, 0x1E, 0x08, 0x08, 0x08},
    ['g'] = {0,    0x0F, 0x11, 0x11, 0x0F, 0x01, 0x0E},
    ['h'] = {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x11},
    ['i'] = {0x04, 0,    0x0C, 0x04, 0x04, 0x04, 0x0E},
    ['l'] = {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    ['m'] = {0,    0,    0x1A, 0x15, 0x15, 0x15, 0x11},
    ['n'] = {0,    0,    0x1E, 0x11, 0x11, 0x11, 0x11},
    ['o'] = {0,    0,    0x0E, 0x11, 0x11, 0x11, 0x0E},
    ['p'] = {0,    0,    0x1E, 0x11, 0x11, 0x1E, 0x10},
    ['r'] = {0,    0,    0x16, 0x19, 0x10, 0x10, 0x10},
    ['s'] = {0,    0,    0x0F, 0x10, 0x0E, 0x01, 0x1E},
    ['t'] = {0x04, 0x04, 0x1E, 0x04, 0x04, 0x04, 0x03},
    ['u'] = {0,    0,    0x11, 0x11, 0x11, 0x11, 0x0F},
    ['v'] = {0,    0,    0x11, 0x11, 0x11, 0x0A, 0x04},
    ['w'] = {0,    0,    0x11, 0x11, 0x15, 0x15, 0x0A},
    ['y'] = {0,    0,    0x11, 0x11, 0x11, 0x0F, 0x01},
};

#define BG_W           256
#define BG_VISIBLE_H   192

#define DIGIT_SCALE    8
#define NUM_DIGITS     4

/* Layout of the 4-digit score block. */
#define DIGIT_W_PX     (5 * DIGIT_SCALE)              /* 40 */
#define DIGIT_H_PX     (7 * DIGIT_SCALE)              /* 56 */
#define DIGIT_GAP_PX   DIGIT_SCALE                    /*  8 */
#define SCORE_W_PX     (NUM_DIGITS * DIGIT_W_PX + (NUM_DIGITS - 1) * DIGIT_GAP_PX)
#define SCORE_X0       ((BG_W - SCORE_W_PX) / 2)
#define SCORE_Y0       ((BG_VISIBLE_H - DIGIT_H_PX) / 2)

/* Bottom-of-screen credit. */
#define CREDIT_SCALE   1
#define CREDIT_LINE1   "Developed by Abid Hasan."
#define CREDIT_LINE2   "His first Dsi game built with <3."
#define CREDIT_LINE1_Y 172
#define CREDIT_LINE2_Y 181

static int sLastScore = -1;

static void drawChar(u16 *bg, char ch, int x, int y, int scale, u16 color)
{
    if ((unsigned char)ch >= 128) return;
    const u8 *glyph = font5x7[(unsigned char)ch];
    for (int row = 0; row < 7; row++) {
        u8 bits = glyph[row];
        if (!bits) continue;
        for (int col = 0; col < 5; col++) {
            if (!(bits & (1 << (4 - col)))) continue;
            int px = x + col * scale;
            int py = y + row * scale;
            for (int dy = 0; dy < scale; dy++) {
                u16 *line = &bg[(py + dy) * BG_W + px];
                for (int dx = 0; dx < scale; dx++) line[dx] = color;
            }
        }
    }
}

static int textWidthPx(const char *s, int scale)
{
    int n = (int)strlen(s);
    if (n == 0) return 0;
    return n * 5 * scale + (n - 1) * scale;     /* 5px glyphs + 1px gaps */
}

static void drawText(u16 *bg, const char *s, int x, int y, int scale, u16 color)
{
    int step = 5 * scale + scale;
    for (int cx = x; *s; s++, cx += step) drawChar(bg, *s, cx, y, scale, color);
}

static void drawTextCentered(u16 *bg, const char *s, int y, int scale, u16 color)
{
    int x = (BG_W - textWidthPx(s, scale)) / 2;
    drawText(bg, s, x, y, scale, color);
}

static void clearAll(u16 *bg)
{
    for (int i = 0; i < BG_W * BG_VISIBLE_H; i++) bg[i] = 0;
}

static void clearScoreRect(u16 *bg)
{
    for (int y = SCORE_Y0; y < SCORE_Y0 + DIGIT_H_PX; y++) {
        u16 *line = &bg[y * BG_W + SCORE_X0];
        for (int x = 0; x < SCORE_W_PX; x++) line[x] = 0;
    }
}

static void drawCredit(u16 *bg)
{
    /* Slightly dimmer than pure white so it sits behind the score visually. */
    const u16 color = ARGB16(1, 22, 22, 22);
    drawTextCentered(bg, CREDIT_LINE1, CREDIT_LINE1_Y, CREDIT_SCALE, color);
    drawTextCentered(bg, CREDIT_LINE2, CREDIT_LINE2_Y, CREDIT_SCALE, color);
}

void scoreInit(void)
{
    sLastScore = -1;
    u16 *bg = renderScoreBgGfx();
    clearAll(bg);
    drawCredit(bg);
    scoreSet(0);
}

void scoreSet(int score)
{
    if (score == sLastScore) return;
    sLastScore = score;
    if (score < 0) score = 0;

    char buf[NUM_DIGITS + 1];
    int n = score;
    for (int i = NUM_DIGITS - 1; i >= 0; i--) {
        buf[i] = (char)('0' + (n % 10));
        n /= 10;
    }
    buf[NUM_DIGITS] = 0;

    u16 *bg = renderScoreBgGfx();
    clearScoreRect(bg);
    drawText(bg, buf, SCORE_X0, SCORE_Y0, DIGIT_SCALE, ARGB16(1, 31, 31, 31));
}
