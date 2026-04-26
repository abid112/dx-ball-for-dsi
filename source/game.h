#ifndef GAME_H
#define GAME_H

typedef enum {
    STATE_PLAYING,
    STATE_WIN,
    STATE_GAME_OVER
} GameState;

void      gameInit(void);
void      gameUpdate(void);
GameState gameGetState(void);
int       gameGetScore(void);
int       gameGetLives(void);

#endif
