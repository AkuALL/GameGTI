#ifndef SHADOW_ESCAPE_GAME_LOGIC_H
#define SHADOW_ESCAPE_GAME_LOGIC_H

#include <GL/glut.h>
#include "game_state.h"

class Image;

int treasuresHeld();
bool allTreasuresCollected();

bool positionValid(float x, float z, float margin = 0.8f);
void randomPos(float &x, float &z);
bool checkCollision(float newX, float newZ);

void initEnemies();
GLuint loadTexture(Image* image);
void resetGame(int level = 1);

bool isNearHidingSpot();
void checkTreasurePickup();
void checkExit();

#endif
