#include <nds.h>

#include "game.h"
#include "level.h"
#include "render.h"
#include "score.h"

#define PADDLE_Y       176
#define PADDLE_SPEED   2

#define BALL_START_VX  1
#define BALL_START_VY -2

#define INITIAL_LIVES  3
#define BRICK_SCORE    10

static GameState sState;

static int sPaddleX;
static int sBallX, sBallY;
static int sBallVx, sBallVy;
static int sBallLaunched;

static int sLives;
static int sScore;

static HudState hudFor(GameState st, int launched)
{
    switch (st) {
    case STATE_WIN:       return HUD_WIN;
    case STATE_GAME_OVER: return HUD_GAME_OVER;
    case STATE_PLAYING:
    default:              return launched ? HUD_PLAYING : HUD_READY;
    }
}

static void resetBallToPaddle(void)
{
    sBallLaunched = 0;
    sBallX = sPaddleX + PADDLE_W / 2 - BALL_W / 2;
    sBallY = PADDLE_Y - BALL_H;
    sBallVx = 0;
    sBallVy = 0;
}

void gameInit(void)
{
    sState = STATE_PLAYING;
    sPaddleX = (PLAY_W - PADDLE_W) / 2;
    sLives = INITIAL_LIVES;
    sScore = 0;
    levelLoad(0);
    resetBallToPaddle();
}

GameState gameGetState(void) { return sState; }
int       gameGetScore(void) { return sScore; }
int       gameGetLives(void) { return sLives; }

static void handlePaddleInput(u16 held)
{
    if (held & KEY_LEFT)  sPaddleX -= PADDLE_SPEED;
    if (held & KEY_RIGHT) sPaddleX += PADDLE_SPEED;
    if (sPaddleX < 0) sPaddleX = 0;
    if (sPaddleX + PADDLE_W > PLAY_W) sPaddleX = PLAY_W - PADDLE_W;
}

static void launchBall(void)
{
    sBallLaunched = 1;
    sBallVx = BALL_START_VX;
    sBallVy = BALL_START_VY;
}

static void bouncePaddle(void)
{
    sBallY = PADDLE_Y - BALL_H;
    sBallVy = -2;
    int hit = (sBallX + BALL_W / 2) - (sPaddleX + PADDLE_W / 2); /* -16..+15 */
    int v = hit / 8;            /* -2..+1 */
    if (v == 0) v = (hit < 0) ? -1 : 1;
    sBallVx = v;
}

static int collideBricks(void)
{
    int prevX = sBallX - sBallVx;
    int prevY = sBallY - sBallVy;

    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (!brickGrid[r][c].alive) continue;
            int bx = c * BRICK_W;
            int by = BRICK_AREA_TOP + r * BRICK_H;
            int overlap = (sBallX + BALL_W > bx) && (sBallX < bx + BRICK_W) &&
                          (sBallY + BALL_H > by) && (sBallY < by + BRICK_H);
            if (!overlap) continue;

            brickGrid[r][c].alive = 0;
            sScore += BRICK_SCORE;

            int wasOutsideY = (prevY + BALL_H <= by) || (prevY >= by + BRICK_H);
            int wasOutsideX = (prevX + BALL_W <= bx) || (prevX >= bx + BRICK_W);
            if (wasOutsideY)      sBallVy = -sBallVy;
            else if (wasOutsideX) sBallVx = -sBallVx;
            else                  sBallVy = -sBallVy;
            return 1;
        }
    }
    return 0;
}

static void updateBall(void)
{
    sBallX += sBallVx;
    sBallY += sBallVy;

    if (sBallX < 0)                 { sBallX = 0;                 sBallVx = -sBallVx; }
    if (sBallX + BALL_W > PLAY_W)   { sBallX = PLAY_W - BALL_W;   sBallVx = -sBallVx; }
    if (sBallY < 0)                 { sBallY = 0;                 sBallVy = -sBallVy; }

    /* Paddle. */
    if (sBallVy > 0 &&
        sBallY + BALL_H >= PADDLE_Y &&
        sBallY + BALL_H <= PADDLE_Y + PADDLE_H + 2 &&
        sBallX + BALL_W > sPaddleX &&
        sBallX < sPaddleX + PADDLE_W) {
        bouncePaddle();
    }

    collideBricks();

    if (levelBricksAlive() == 0) {
        sState = STATE_WIN;
        return;
    }

    if (sBallY > PLAY_H) {
        sLives--;
        if (sLives < 0) {
            sState = STATE_GAME_OVER;
        } else {
            resetBallToPaddle();
        }
    }
}

void gameUpdate(void)
{
    scanKeys();
    u16 held = keysHeld();
    u16 down = keysDown();

    if (sState == STATE_PLAYING) {
        handlePaddleInput(held);

        if (!sBallLaunched) {
            sBallX = sPaddleX + PADDLE_W / 2 - BALL_W / 2;
            sBallY = PADDLE_Y - BALL_H;
            if (down & KEY_A) launchBall();
        } else {
            updateBall();
        }
    } else {
        if (down & KEY_A) gameInit();
    }

    /* Push frame to renderer. */
    renderPaddle(sPaddleX, PADDLE_Y);
    int ballHidden = (sState != STATE_PLAYING);
    renderBall(sBallX, sBallY, ballHidden);
    renderBricks();
    renderHud(sLives, hudFor(sState, sBallLaunched));
    scoreSet(sScore);
}
