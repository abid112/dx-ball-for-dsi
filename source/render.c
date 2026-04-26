#include <nds.h>
#include <stdio.h>
#include <string.h>

#include "render.h"
#include "level.h"

/* Sprite IDs on the main OAM:
 *  0       : ball
 *  1       : paddle
 *  2..41   : bricks (BRICK_ROWS * BRICK_COLS = 40)
 */
#define SPR_BALL    0
#define SPR_PADDLE  1
#define SPR_BRICK_BASE 2

static u16 *sPaddleGfx;
static u16 *sBallGfx;
static u16 *sBrickGfx[BRICK_ROWS];

static u16 *sScoreBgGfx;

/* Cached HUD state so we only redraw text when something changed. */
static int       sLastLives = -1;
static HudState  sLastHud   = (HudState)-1;

static void fillSprite(u16 *gfx, int pixels, u16 color)
{
    for (int i = 0; i < pixels; i++) gfx[i] = color;
}

void renderInit(void)
{
    powerOn(POWER_ALL_2D);
    lcdMainOnTop();

    /* Top screen: tiled BGs + sprites. */
    videoSetMode(MODE_0_2D);
    /* Bottom screen: bitmap BG for the big score. */
    videoSetModeSub(MODE_5_2D);

    /* VRAM banks.
     *
     * The text console's tile + map bases live in the first 128KB of main
     * BG memory (mapBase is 5 bits = 0..62KB), so VRAM_A must be mapped
     * to MAIN_BG at base 0x06000000. VRAM_B is then used for sprites,
     * pinned to 0x06400000 (the OBJ base the engine reads from) — its
     * default `VRAM_B_MAIN_SPRITE` would put it at 0x06420000 instead. */
    vramSetBankA(VRAM_A_MAIN_BG);                    /* main BG (console) */
    vramSetBankB(VRAM_B_MAIN_SPRITE_0x06400000);     /* main sprite gfx  */
    vramSetBankC(VRAM_C_SUB_BG);                     /* sub BG (score)   */

    /* Backdrop colors. */
    BG_PALETTE[0]     = RGB15(0, 0, 0);
    BG_PALETTE_SUB[0] = RGB15(0, 0, 0);

    /* Console on top screen, BG layer 0 (loads default 8x8 font + palette). */
    consoleInit(NULL, 0,
                BgType_Text4bpp, BgSize_T_256x256,
                31 /* mapBase */, 0 /* tileBase */,
                true  /* main display */,
                true  /* load default graphics */);

    /* Bitmap BG on the sub engine for the score. */
    int subBg = bgInitSub(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    sScoreBgGfx = bgGetGfxPtr(subBg);
    for (int i = 0; i < 256 * 256; i++) sScoreBgGfx[i] = 0;

    /* Sprite engine on main, bitmap-mode sprites. */
    oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);

    /* Allocate sprite gfx tiles and paint solid colors. */
    sPaddleGfx = oamAllocateGfx(&oamMain, SpriteSize_32x8, SpriteColorFormat_Bmp);
    sBallGfx   = oamAllocateGfx(&oamMain, SpriteSize_8x8,  SpriteColorFormat_Bmp);

    fillSprite(sPaddleGfx, 32 * 8, ARGB16(1, 31, 31, 31));   /* white */
    fillSprite(sBallGfx,    8 * 8, ARGB16(1, 31, 31, 31));

    /* One brick gfx tile per row, each row a different color. */
    static const u16 rowColors[BRICK_ROWS] = {
        ARGB16(1, 31,  4,  4),   /* red    */
        ARGB16(1, 31, 18,  0),   /* orange */
        ARGB16(1, 31, 31,  0),   /* yellow */
        ARGB16(1,  4, 28,  4),   /* green  */
        ARGB16(1,  6, 14, 31),   /* blue   */
    };
    for (int r = 0; r < BRICK_ROWS; r++) {
        sBrickGfx[r] = oamAllocateGfx(&oamMain, SpriteSize_32x8, SpriteColorFormat_Bmp);
        fillSprite(sBrickGfx[r], 32 * 8, rowColors[r]);
    }

    /* Hide every sprite slot we use until something tells us otherwise. */
    for (int i = 0; i < SPR_BRICK_BASE + BRICK_ROWS * BRICK_COLS; i++) {
        oamSet(&oamMain, i, 0, 192,   /* offscreen */
               0, 15, SpriteSize_8x8, SpriteColorFormat_Bmp,
               sBallGfx, -1, false, true /* hide */,
               false, false, false);
    }
}

void renderPaddle(int x, int y)
{
    oamSet(&oamMain, SPR_PADDLE, x, y,
           0, 15,
           SpriteSize_32x8, SpriteColorFormat_Bmp,
           sPaddleGfx, -1, false, false,
           false, false, false);
}

void renderBall(int x, int y, int hidden)
{
    oamSet(&oamMain, SPR_BALL, x, y,
           0, 15,
           SpriteSize_8x8, SpriteColorFormat_Bmp,
           sBallGfx, -1, false, hidden ? true : false,
           false, false, false);
}

void renderBricks(void)
{
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            int id = SPR_BRICK_BASE + r * BRICK_COLS + c;
            int alive = brickGrid[r][c].alive;
            int x = c * BRICK_W;
            int y = BRICK_AREA_TOP + r * BRICK_H;
            oamSet(&oamMain, id, x, y,
                   0, 15,
                   SpriteSize_32x8, SpriteColorFormat_Bmp,
                   sBrickGfx[r], -1, false, alive ? false : true,
                   false, false, false);
        }
    }
}

static void clearLine(int row)
{
    /* 32-char-wide blank, used to wipe a row of the console. */
    iprintf("\x1b[%d;0H                                ", row);
}

void renderHud(int lives, HudState state)
{
    if (lives != sLastLives) {
        iprintf("\x1b[0;0HLIVES: %d ", lives);
        sLastLives = lives;
    }

    if (state == sLastHud) return;
    sLastHud = state;

    /* Wipe message rows, redraw if needed. */
    clearLine(11);
    clearLine(12);
    clearLine(14);

    switch (state) {
    case HUD_PLAYING:
        break;
    case HUD_READY:
        iprintf("\x1b[14;6HPRESS A TO LAUNCH");
        break;
    case HUD_WIN:
        iprintf("\x1b[11;11H YOU WIN ");
        iprintf("\x1b[14;6HPRESS A TO RESTART");
        break;
    case HUD_GAME_OVER:
        iprintf("\x1b[11;11HGAME OVER");
        iprintf("\x1b[14;6HPRESS A TO RESTART");
        break;
    }
}

void renderFlush(void)
{
    oamUpdate(&oamMain);
}

u16* renderScoreBgGfx(void)
{
    return sScoreBgGfx;
}
