#include "../../include/game_state.h"

GameState gameState = STATE_MENU;

float playerX = 0.0f, playerZ = 0.0f;
float playerAngle = 0.0f;
float speed = 3.2f;
float runMultiplier = 1.8f;
float slowMultiplier = 0.45f;
bool playerIsHiding = false;
bool keyStates[256] = {false};
bool specialKeyStates[256] = {false};

float treasureX[MAX_TREASURES], treasureZ[MAX_TREASURES];
bool treasureCollected[MAX_TREASURES] = {false, false, false};
int treasureCount = 2;

float eyeHeight = 1.5f;
float lookDistance = 6.0f;

int lastMouseX = 400;
float mouseSensitivity = 0.2f;
int windowWidth = 800, windowHeight = 600;
bool ignoreNextMouseMove = false;

GLuint wallTextureId = 0;

Wall walls[] = {
    { 0,  -20,   40,  0.6f},
    { 0,   20,   40,  0.6f},
    {-20,   0,  0.6f, 40},
    { 20,  4.75f, 0.6f, 29.6f},
    { 20,-16.75f, 0.6f,  5.64f},

    {-18,  16,    4,  0.6f},
    { -4,  16,   12,  0.6f},
    {  8,  16,   10,  0.6f},

    {-17,  10,    6,  0.6f},
    { -8,  10,   12,  0.6f},
    {  4,  10,    8,  0.6f},

    {-19,   4,   10,  0.6f},
    {  0,   4,   10,  0.6f},
    { 14,   4,    8,  0.6f},

    {-20,  -2,    6,  0.6f},
    { -6,  -2,   10,  0.6f},
    {  8,  -2,    8,  0.6f},

    {-14,  -8,    8,  0.6f},
    {  2,  -8,    8,  0.6f},

    {-20, -14,    6,  0.6f},
    { -4, -14,   10,  0.6f},

    {-16,  18,  0.6f,  4},
    {-16,   7,  0.6f,  6},
    {-16,  -5,  0.6f,  6},
    {-16, -17,  0.6f,  6},

    { -4,  18,  0.6f,  4},
    { -4,   7,  0.6f,  6},
    { -4,  -5,  0.6f,  6},
    { -4, -17,  0.6f,  6},

    {  8,  18,  0.6f,  4},
    {  8,   7,  0.6f,  6},
    {  8,  -5,  0.6f,  6},

    { 16,  18,  0.6f,  4},
    { 16,   7,  0.6f,  6},
    { 16,  -5,  0.6f,  6},
    { 16, -17,  0.6f,  6},
};
int wallCount = sizeof(walls) / sizeof(walls[0]);

float exitX = 20.5f, exitZ = -12.0f;

float doorAngle = 0.0f;
float doorOpenSpeed = 80.0f;
bool doorShouldOpen = false;

HidingSpot hidingSpots[] = {
    {-18.0f,  18.5f},
    { -7.0f,  18.5f},
    {  6.0f,  18.5f},
    {-18.0f,   7.0f},
    { -7.0f,   7.0f},
    { 11.0f,   7.0f},
    {-18.0f,  -5.0f},
    { -7.0f,  -5.0f},
    { 11.0f,  -5.0f},
    {-18.0f, -16.0f},
    { -7.0f, -16.0f},
    {  3.0f, -16.0f},
};
int hidingCount = sizeof(hidingSpots) / sizeof(hidingSpots[0]);

Enemy enemies[ENEMY_COUNT];

float gameTimer = 0.0f;
float lightCycleInterval = 40.0f;
float lightOnDuration = 10.0f;
bool lightsOn = false;
float lightsOnTimer = 0.0f;
float lastTime = 0.0f;

float flickerTimer = 0.0f;
float flickerIntensity = 1.0f;
float breathTimer = 0.0f;

float startTime = 0.0f;
int finalScore = 0;
bool wonWithTreasure = false;

float jumpscareTimer = 0.0f;
bool jumpscareActive = false;
float jumpscareAlpha = 0.0f;
float jumpscareDuration = 2.0f;
int jumpscareStartTime = 0;

bool bgmPlaying = false;
const char* currentBGM = nullptr;

int currentLevel = 1;
bool levelClearing = false;
float levelClearTimer = 0.0f;

float level2Countdown = 120.0f;
float level2CountdownTimer = 0.0f;
bool chaseMode = false;

int selectedLevel = 1;

LevelConfig levelConfigs[LEVEL_COUNT] = {
    {2, 0.0f,   false, false},
    {2, 120.0f, true,  false},
    {3, 120.0f, true,  true },
};
