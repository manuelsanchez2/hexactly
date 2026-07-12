#pragma once

struct Progress {
    unsigned long long done = 0;
    bool seenRules   = false;
    bool seenWalls   = false;
    bool seenPortals = false;
    bool seenBombs   = false;
    bool seenDailyOps = false;   // shown the operator tip on the first daily

    long dailyLastDay = -1;   // epoch-day of the last daily win (-1 = never)
    int  dailyStreak  = 0;    // consecutive-day win streak
    int  dailyBest    = 0;    // best streak ever
};

Progress loadProgress();
void     saveProgress(const Progress& p);

inline bool levelDone(const Progress& p, int i)    { return (p.done >> i) & 1ull; }
inline void markLevelDone(Progress& p, int i)      { p.done |= (1ull << i); }

int firstIncomplete(const Progress& p, int count);
