#ifndef LEVEL_H
#define LEVEL_H

#define BRICK_COLS       8
#define BRICK_ROWS       5
#define BRICK_W         32
#define BRICK_H          8
#define BRICK_AREA_TOP  24

typedef struct {
    unsigned char alive;
} Brick;

typedef struct {
    unsigned char layout[BRICK_ROWS][BRICK_COLS];
} LevelDef;

extern Brick brickGrid[BRICK_ROWS][BRICK_COLS];

void levelLoad(int index);
int  levelBricksAlive(void);
int  levelCount(void);

#endif
