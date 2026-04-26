#ifndef RENDER_H
#define RENDER_H

#include <nds.h>

#define PLAY_W      256
#define PLAY_H      192
#define PADDLE_W    32
#define PADDLE_H    8
#define BALL_W      8
#define BALL_H      8

void renderInit(void);

/* Per-frame: place sprites at requested positions. Pass hidden=true to hide. */
void renderPaddle(int x, int y);
void renderBall(int x, int y, int hidden);
void renderBricks(void);

/* Draw / update HUD text on top screen (lives + state messages). */
typedef enum {
    HUD_PLAYING,
    HUD_WIN,
    HUD_GAME_OVER,
    HUD_READY        /* ball waiting on paddle */
} HudState;

void renderHud(int lives, HudState state);

/* Push the OAM buffers for both engines. Call once per frame at VBlank. */
void renderFlush(void);

/* Bitmap pointer for the bottom-screen score BG (used by score module). */
u16* renderScoreBgGfx(void);

#endif
