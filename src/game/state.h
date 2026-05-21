#ifndef SHADOW_ESCAPE_STATE_H
#define SHADOW_ESCAPE_STATE_H

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <GL/glut.h>
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265f
#endif

enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAMEOVER,
    STATE_WIN,
    STATE_LEVEL_CLEAR
};

extern GameState gameState;

extern float playerX, playerZ;
extern float playerAngle;
extern float speed;
extern float runMultiplier;
extern float slowMultiplier;
extern bool playerIsHiding;
extern bool keyStates[256];
extern bool specialKeyStates[256];

extern int windowWidth, windowHeight;
extern bool ignoreNextMouseMove;
extern float mouseSensitivity;

extern int currentLevel;
extern int selectedLevel;

bool checkCollision(float newX, float newZ);
bool isNearHidingSpot();
void resetGame(int level = 1);

#endif
