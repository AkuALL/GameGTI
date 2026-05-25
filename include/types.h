#ifndef SHADOW_ESCAPE_TYPES_H
#define SHADOW_ESCAPE_TYPES_H

#include <vector>

enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAMEOVER,
    STATE_WIN,
    STATE_LEVEL_CLEAR
};

struct Wall {
    float x, z;
    float sx, sz;
};

struct HidingSpot {
    float x, z;
};

struct Enemy {
    float x, z;
    float angle;
    float speed;
    int waypointIdx;
    int stuckFrames;
    float targetX, targetZ;
    std::vector<float> pathX, pathZ;
    int pathIndex;
    float wpX[8], wpZ[8];
    int wpCount;
};

struct LevelConfig {
    int treasures;
    float countdown;
    bool hasCountdown;
    bool lightsChaseMode;
};

#endif
