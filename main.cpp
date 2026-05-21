#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <process.h> 
#pragma comment(lib, "winmm.lib")
#endif

#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <algorithm>

// Mengimpor library pembaca gambar sesuai modul praktikum
#include "src/assets/imageloader.h"
#include "src/assets/imageloader.cpp"
#include "src/assets/paths.h"
#include "src/game/state.h"

// ================= CONSTANTS =================
#define MAP_MIN -20.0f
#define MAP_MAX  20.0f
#ifndef M_PI
#define M_PI 3.14159265f
#endif

GameState gameState = STATE_MENU;

// ================= PLAYER =================
float playerX = 0.0f, playerZ = 0.0f;
float playerAngle = 0.0f;
float speed = 3.2f; // units per second
float runMultiplier = 1.8f;
float slowMultiplier = 0.45f;
// ================= TREASURE =================
// GANTI:
// float treasureX, treasureZ;
// DENGAN:
float treasureX[3], treasureZ[3];
bool treasureCollected[3] = {false, false, false};
int treasureCount = 2;
// Tambahkan fungsi ini setelah deklarasi global
int treasuresHeld() {
    int count = 0;
    for (int i = 0; i < treasureCount; i++)
        if (treasureCollected[i]) count++;
    return count;
}

bool allTreasuresCollected() {
    return treasuresHeld() == treasureCount;
}
bool playerIsHiding = false;
bool keyStates[256] = {false};
bool specialKeyStates[256] = {false};

// ================= CAMERA =================
float eyeHeight = 1.5f;
float lookDistance = 6.0f;

// ================= MOUSE =================
int lastMouseX = 400;
float mouseSensitivity = 0.2f;
int windowWidth = 800, windowHeight = 600;
bool ignoreNextMouseMove = false;

// ================= TEXTURE ID =================
GLuint wallTextureId; 

// ================= WALLS =================
struct Wall { float x, z, sx, sz; };
Wall walls[] = {
    // ── OUTER BOUNDARY ──
    { 0,  -20,   40,  0.6f},  // 0: south
    { 0,   20,   40,  0.6f},  // 1: north
    {-20,   0,  0.6f, 40},    // 2: west
    { 20,  4.75f, 0.6f, 29.6f},  // 3: east upper (z=-10.5 s/d z=20) — JANGAN UBAH
    { 20,-16.75f, 0.6f,  5.64f},  // 4: east lower (z=-20 s/d -13.5) — JANGAN UBAH

    // ── HORIZONTAL WALLS ──
    // z=16
    {-18,  16,    4,  0.6f},  // 5:  x=-20 s/d -16
    { -4,  16,   12,  0.6f},  // 6:  x=-10 s/d  2  | gap -16 s/d -10 = 6u ✓
    {  8,  16,   10,  0.6f},  // 7:  x=3 s/d 13    | gap 2 s/d 8 = 6u ✓

    // z=10
    {-17,  10,    6,  0.6f},  // 8:  x=-20 s/d -14  | DIPERBAIKI: nyambung west, gap ke -14
    { -8,  10,   12,  0.6f},  // 9:  x=-14 s/d -2   | DIPERBAIKI: sz=12, nyambung gap -14 s/d -2
    {  4,  10,    8,  0.6f},  // 10: x=0 s/d 8      | gap -2 s/d 4 = 6u ✓

    // z=4
    {-19,   4,   10,  0.6f},  // 11: x=-20 s/d -9   | DIPERBAIKI: mulai dari x=-20 (west)
    {  0,   4,   10,  0.6f},  // 12: x=-5 s/d 5     | gap -9 s/d -5 = 4u ✓
    { 14,   4,    8,  0.6f},  // 13: x=10 s/d 18    | DIPERBAIKI: sz=8 nyambung x=16

    // z=-2
    {-20,  -2,    6,  0.6f},  // 14: x=-20 s/d -17  ✓
    { -6,  -2,   10,  0.6f},  // 15: x=-11 s/d -1   ✓
    {  8,  -2,    8,  0.6f},  // 16: x=4 s/d 12     ✓

    // z=-8
    {-14,  -8,    8,  0.6f},  // 17: x=-18 s/d -10  ✓
    {  2,  -8,    8,  0.6f},  // 18: x=-2 s/d 6     ✓

    // z=-14
    {-20, -14,    6,  0.6f},  // 19: x=-20 s/d -17  ✓
    { -4, -14,   10,  0.6f},  // 20: x=-9 s/d 1     | DIPERBAIKI: sz=10, tutup gap x=0 s/d 4

    // ── VERTICAL WALLS ──
    // x=-16
    {-16,  18,  0.6f,  4},    // 21: z=16 s/d 20 ✓
    {-16,   7,  0.6f,  6},    // 22: z=4 s/d 10  ✓
    {-16,  -5,  0.6f,  6},    // 23: z=-8 s/d -2 ✓
    {-16, -17,  0.6f,  6},    // 24: z=-20 s/d -14 ✓

    // x=-4
    { -4,  18,  0.6f,  4},    // 25: ✓
    { -4,   7,  0.6f,  6},    // 26: ✓
    { -4,  -5,  0.6f,  6},    // 27: ✓
    { -4, -17,  0.6f,  6},    // 28: ✓

    // x=8
    {  8,  18,  0.6f,  4},    // 29: ✓
    {  8,   7,  0.6f,  6},    // 30: ✓
    {  8,  -5,  0.6f,  6},    // 31: ✓

    // x=16 — DIPERBAIKI: semua segmen nyambung tanpa gap vertikal
    { 16,  18,  0.6f,  4},    // 32: z=16 s/d 20 (nempel north) ✓
    { 16,   7,  0.6f,  6},    // 33: z=4 s/d 10  | DIPERBAIKI: ganti dari (1,6) → (7,6)
    { 16,  -5,  0.6f,  6},    // 34: z=-8 s/d -2 | DIPERBAIKI: ganti dari (-11,4) → (-5,6)
    { 16, -17,  0.6f,  6},    // 35: z=-20 s/d -14 | TAMBAH: nyambung south
};
int wallCount = 36;  // +1 karena tambah wall x=16 south
// ================= EXIT =================
float exitX = 20.5f, exitZ = -12.0f;  // tengah gap = ((-9.5)+(-14.5))/2 = -12

// ================= DOOR STATE (tambah di global vars) =================
float doorAngle = 0.0f;
float doorOpenSpeed = 80.0f;
bool doorShouldOpen = false;

// ================= HIDING SPOTS =================
struct HidingSpot { float x, z; };

HidingSpot hidingSpots[] = {
    {-18.0f,  18.5f},  // pojok kiri atas, di luar semua dinding
    { -7.0f,  18.5f},  // lorong atas tengah
    {  6.0f,  18.5f},  // lorong atas kanan
    {-18.0f,   7.0f},  // lorong kiri, antara z=4 dan z=10
    { -7.0f,   7.0f},  // lorong tengah-kiri antara z=4 dan z=10
    { 11.0f,   7.0f},  // lorong kanan antara z=4 dan z=10
    {-18.0f,  -5.0f},  // lorong kiri antara z=-8 dan z=-2
    { -7.0f,  -5.0f},  // lorong tengah antara z=-8 dan z=-2
    { 11.0f,  -5.0f},  // lorong kanan antara z=-8 dan z=-2
    {-18.0f, -16.0f},  // lorong kiri bawah
    { -7.0f, -16.0f},  // lorong tengah bawah
    {  3.0f, -16.0f},  // lorong kanan bawah
};
int hidingCount = 12;

// ================= ENEMIES =================
struct Enemy {
    float x, z;
    float angle;
    float speed;       
    int   waypointIdx;
    int   stuckFrames;
    float targetX, targetZ;
    std::vector<float> pathX, pathZ;
    int pathIndex;
    float wpX[8], wpZ[8];
    int   wpCount;
};

Enemy enemies[8];

// ================= LIGHT CYCLE =================
float gameTimer = 0.0f;
float lightCycleInterval = 40.0f;
float lightOnDuration = 10.0f;
bool lightsOn = false;
float lightsOnTimer = 0.0f;
float lastTime = 0.0f;

// ================= ATMOSPHERE =================
float flickerTimer = 0.0f;
float flickerIntensity = 1.0f;
float breathTimer = 0.0f;

// ================= SCORE =================
float startTime = 0.0f;
int finalScore = 0;
bool wonWithTreasure = false;

// ================= JUMPSCARE =================
float jumpscareTimer = 0.0f;
bool jumpscareActive = false;
float jumpscareAlpha = 0.0f;
float jumpscareDuration = 2.0f; // detik
int jumpscareStartTime = 0;

// ================= SOUND =================
bool bgmPlaying = false;
const char* currentBGM = nullptr;


// ================= LEVEL SYSTEM =================
int currentLevel = 1;
bool levelClearing = false;      // layar "Level Clear"
float levelClearTimer = 0.0f;    // berapa lama layar Level Clear ditampilkan

// Level 2 countdown
float level2Countdown = 120.0f;   // detik countdown sebelum chase mode
float level2CountdownTimer = 0.0f;
bool chaseMode = false;           // true = timer habis, enemy kejar player

// ================= MENU LEVEL SELECT =================
int selectedLevel = 1;  // level yang dipilih di menu

struct LevelConfig {
    int    treasures;
    float  countdown;      // 0 = tanpa batas
    bool   hasCountdown;
    bool   lightsChaseMode; // lampu ON = langsung chase
};
LevelConfig levelConfigs[3] = {
    {2, 0.0f,   false, false}, // Easy
    {2, 120.0f, true,  false}, // Medium
    {3, 120.0f, true,  true }, // Hard
};

// ================= FORWARD DECLARATIONS =================
bool checkCollision(float newX, float newZ);
bool pathExists(float startX, float startZ, float targetX, float targetZ);
void updatePlayerMovement(float dt);

// ================= MODULES =================
// Module files are included here so the existing single-file build still works.
#include "src/audio/sound.cpp"
#include "src/game/core_logic.cpp"
#include "src/game/enemy_ai.cpp"
#include "src/game/update_loop.cpp"
#include "src/input/controls.cpp"
#include "src/render/world_render.cpp"
#include "src/render/hud_render.cpp"
#include "src/render/menu_render.cpp"
#include "src/render/screen_render.cpp"
#include "src/app/window.cpp"

int main(int argc,char**argv) {
    srand((unsigned)time(NULL));
    glutInit(&argc,argv); 
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH); 
    glutInitWindowSize(800,600);glutInitWindowPosition(100,100); 
    glutCreateWindow("Maze Escape - Find Treasure & Survive"); 
    
    initRendering(); 
    
    glutDisplayFunc(display);glutReshapeFunc(reshape); 
    glutKeyboardFunc(keyboard);glutSpecialFunc(specialKeys); 
    glutKeyboardUpFunc(keyboardUp);glutSpecialUpFunc(specialKeysUp);
    glutIgnoreKeyRepeat(1);
    glutPassiveMotionFunc(mouseMotion);
    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(windowWidth / 2, windowHeight / 2);
    glutTimerFunc(16,update,0); 
    glutMainLoop(); 
    return 0; 
}
