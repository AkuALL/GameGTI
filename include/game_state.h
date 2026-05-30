#ifndef SHADOW_ESCAPE_GAME_STATE_H
#define SHADOW_ESCAPE_GAME_STATE_H

#include <GL/glut.h>
#include "config.h"
#include "types.h"

extern GameState gameState;

extern float playerX, playerZ;
extern float playerAngle;
extern float speed;
extern float runMultiplier;
extern float slowMultiplier;
extern bool playerIsHiding;
extern bool keyStates[256];
extern bool specialKeyStates[256];

extern float treasureX[MAX_TREASURES], treasureZ[MAX_TREASURES];
extern bool treasureCollected[MAX_TREASURES];
extern int treasureCount;

extern float eyeHeight;
extern float lookDistance;

extern int lastMouseX;
extern float mouseSensitivity;
extern int windowWidth, windowHeight;
extern bool ignoreNextMouseMove;

extern GLuint wallTextureId;
extern GLuint floorTextureId;
extern GLuint doorTextureId;

extern GLuint scareTexId1;
extern GLuint scareTexId2;
extern GLuint scareTexId3;


extern Wall walls[];
extern int wallCount;

extern float exitX, exitZ;

extern float doorAngle;
extern float doorOpenSpeed;
extern bool doorShouldOpen;

extern HidingSpot hidingSpots[];
extern int hidingCount;

extern Enemy enemies[ENEMY_COUNT];

extern float gameTimer;
extern float lightCycleInterval;
extern float lightOnDuration;
extern bool lightsOn;
extern float lightsOnTimer;
extern float lastTime;

extern float flickerTimer;
extern float flickerIntensity;
extern float breathTimer;

extern float startTime;
extern int finalScore;
extern bool wonWithTreasure;

extern float jumpscareTimer;
extern bool jumpscareActive;
extern float jumpscareAlpha;
extern float jumpscareDuration;
extern int jumpscareStartTime;

extern bool bgmPlaying;
extern const char* currentBGM;

extern int currentLevel;
extern bool levelClearing;
extern float levelClearTimer;

extern float level2Countdown;
extern float level2CountdownTimer;
extern bool chaseMode;

extern int selectedLevel;
extern LevelConfig levelConfigs[LEVEL_COUNT];

#endif
