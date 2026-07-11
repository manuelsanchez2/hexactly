#pragma once

#include "raylib.h"

const int SCREEN_WIDTH  = 720;
const int SCREEN_HEIGHT = 720;

// Feature flag: daily challenges (the operator-tile puzzles). Disabled until the
// mode is finished - when 0 the daily button and all its UI stay hidden and the
// mode is unreachable. Flip to 1 to bring it back.
#ifndef FEATURE_DAILY
#define FEATURE_DAILY 0
#endif

#define PAPER  (Color){ 255, 253, 248, 255 }
#define INK    (Color){  30,  30,  30, 255 }
#define HEXRED (Color){ 220,  50,  47, 255 }
