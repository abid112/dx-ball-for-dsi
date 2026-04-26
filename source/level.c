#include "level.h"

Brick brickGrid[BRICK_ROWS][BRICK_COLS];

static const LevelDef levels[] = {
    {
        // Level 1: full 5x8 wall
        {
            {1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1},
        }
    },
};

#define LEVEL_COUNT ((int)(sizeof(levels) / sizeof(levels[0])))

void levelLoad(int index)
{
    if (index < 0 || index >= LEVEL_COUNT) index = 0;
    const LevelDef *lv = &levels[index];
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            brickGrid[r][c].alive = lv->layout[r][c];
        }
    }
}

int levelBricksAlive(void)
{
    int n = 0;
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (brickGrid[r][c].alive) n++;
        }
    }
    return n;
}

int levelCount(void)
{
    return LEVEL_COUNT;
}
