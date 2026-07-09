#include "levels.h"

const LevelDef LEVELS[] = {
    { "first", 1, 2,
      { {0,0,2,0}, {1,0,2,F_GOAL} }, 0, {} },

    { "aim", 2, 3,
      { {0,0,2,0}, {1,0,2,0}, {2,-1,4,F_GOAL} }, 0, {} },

    { "zigzag", 3, 4,
      { {0,0,2,0}, {1,0,2,0}, {1,-1,2,0}, {2,-1,2,F_GOAL} }, 0, {} },

    { "order", 3, 4,
      { {0,0,2,0}, {1,0,2,0}, {2,0,2,F_GOAL}, {3,0,2,0} }, 0, {} },

    { "corner", 3, 4,
      { {0,0,2,0}, {1,0,2,0}, {0,1,2,0}, {-1,1,2,F_GOAL} }, 0, {} },
};

const int LEVEL_COUNT = sizeof(LEVELS) / sizeof(LEVELS[0]);
