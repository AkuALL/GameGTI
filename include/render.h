#ifndef SHADOW_ESCAPE_RENDER_H
#define SHADOW_ESCAPE_RENDER_H

#include <GL/glut.h>

int getBitmapTextWidth(void* font, const char* text);
float getBitmapTextWidthOrtho(void* font, const char* text);
void drawBitmapText(void* font, float x, float y, const char* t, float r, float g, float b);
void drawText2D(float x, float y, const char* t, float r, float g, float b);
void drawTextLarge(float x, float y, const char* t, float r, float g, float b);
void drawTextCentered(void* font, float centerX, float y, const char* t, float r, float g, float b);
void drawText2DCentered(float centerX, float y, const char* t, float r, float g, float b);
void drawTextLargeCentered(float centerX, float y, const char* t, float r, float g, float b);
void beginOrtho();
void endOrtho();
void prepareStaticScreen(float r, float g, float b);

void drawLevelClear();
void drawGPSArrow(float cx, float cy, float tx, float tz, float r, float g, float b);
void drawJumpscare();
void drawGPS();

void drawMiniMazeWall(float x, float y, float w, float h);
void drawMenuMaze();
void drawMenu();

void applyFlicker(float r, float g, float b);
float cellHash(int ix, int iz);
void drawStoneSlab(float wx, float wz, float sw, float sh,
                   float bR, float bG, float bB,
                   float cR, float cG, float cB, int cracks);
void drawFloor();
void drawWall(float x, float z, float sx, float sz);
void drawMaze();
void drawExit();
void drawHidingSpots();
void drawCoin(float x, float y, float z, float radius, float thickness);
void drawTreasure();
void drawEnemyBody(int i);
void drawEnemies();
void updateCamera();

void drawGameOver();
void drawWin();
void display();

#endif
