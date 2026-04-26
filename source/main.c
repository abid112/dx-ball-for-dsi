#include <nds.h>

#include "render.h"
#include "score.h"
#include "game.h"

int main(void)
{
    renderInit();
    scoreInit();
    gameInit();

    while (1) {
        gameUpdate();
        swiWaitForVBlank();
        renderFlush();
    }

    return 0;
}
