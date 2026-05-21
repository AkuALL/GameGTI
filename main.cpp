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
#include "imageloader.h"
#include "imageloader.cpp"

// ================= CONSTANTS =================
#define MAP_MIN -20.0f
#define MAP_MAX  20.0f
#ifndef M_PI
#define M_PI 3.14159265f
#endif

// ================= GAME STATE =================
enum GameState { STATE_MENU, STATE_PLAYING, STATE_GAMEOVER, STATE_WIN, STATE_LEVEL_CLEAR };
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

void stopSound() {
#ifdef _WIN32
    bgmPlaying = false;
    currentBGM = nullptr;
    PlaySoundA(NULL, NULL, 0);
#endif
}

void resumeBGM() {
#ifdef _WIN32
    if (currentBGM) {
        bgmPlaying = true;
        PlaySoundA(currentBGM, NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
    }
#endif
}

// Thread untuk SFX agar tidak freeze game
#ifdef _WIN32
struct SFXParam { char filename[256]; };

unsigned __stdcall sfxThread(void* arg) {
    SFXParam* p = (SFXParam*)arg;
    PlaySoundA(p->filename, NULL, SND_FILENAME | SND_SYNC);
    delete p;
    return 0;
}
#endif

void playBGM(const char* filename) {
#ifdef _WIN32
    currentBGM = filename;
    bgmPlaying = true;
    PlaySoundA(filename, NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
#endif
}

void playSFX(const char* filename) {
#ifdef _WIN32
    // Stop BGM dulu agar SFX terdengar jelas
    PlaySoundA(NULL, NULL, 0);

    SFXParam* p = new SFXParam();
    strncpy(p->filename, filename, 255);
    p->filename[255] = '\0';

    HANDLE h = (HANDLE)_beginthreadex(NULL, 0, sfxThread, p, 0, NULL);
    if (h) CloseHandle(h);
#endif
}

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
void resetGame();

// ===================================================
// SPAWN HELPERS
// ===================================================
bool positionValid(float x, float z, float margin = 0.8f) {
    if (x < MAP_MIN + 1.5f || x > MAP_MAX - 1.5f || z < MAP_MIN + 1.5f || z > MAP_MAX - 1.5f)
        return false;
    for (int i = 0; i < wallCount; i++) {
        float wx = walls[i].x, wz = walls[i].z;
        float hx = walls[i].sx / 2.0f + margin;
        float hz = walls[i].sz / 2.0f + margin;
        if (x > wx - hx && x < wx + hx && z > wz - hz && z < wz + hz)
            return false;
    }
    return true;
}

void randomPos(float &x, float &z) {
    int tries = 0;
    do {
        x = (float)(rand() % 340 - 170) / 10.0f;
        z = (float)(rand() % 340 - 170) / 10.0f;
        tries++;
    } while (!positionValid(x, z) && tries < 1000);
}

// ===================================================
// PLAYER COLLISION
// ===================================================
bool checkCollision(float newX, float newZ) {
    float playerSize = 0.45f;
    if (newX < MAP_MIN + playerSize || newX > MAP_MAX - playerSize ||
        newZ < MAP_MIN + playerSize || newZ > MAP_MAX - playerSize)
        return true;
    for (int i = 0; i < wallCount; i++) {
        float wx = walls[i].x, wz = walls[i].z;
        float halfX = walls[i].sx / 2.0f, halfZ = walls[i].sz / 2.0f;
        if (newX + playerSize > wx - halfX && newX - playerSize < wx + halfX &&
            newZ + playerSize > wz - halfZ && newZ - playerSize < wz + halfZ)
            return true;
    }
    return false;
}

// ===================================================
// ENEMY INIT
// ===================================================
void initEnemies() {
    // Enemy 0 - area kiri atas
    enemies[0].x = -18.0f; enemies[0].z = 13.0f;
    enemies[0].angle = 0; enemies[0].speed = 0.065f; enemies[0].stuckFrames = 0;
    enemies[0].pathX.clear(); enemies[0].pathZ.clear(); enemies[0].pathIndex = 0;
    enemies[0].waypointIdx = 0; enemies[0].wpCount = 4;
    enemies[0].wpX[0]=-18.0f; enemies[0].wpZ[0]= 13.0f;
    enemies[0].wpX[1]=-12.0f; enemies[0].wpZ[1]= 13.0f;
    enemies[0].wpX[2]=-12.0f; enemies[0].wpZ[2]=  7.0f;
    enemies[0].wpX[3]=-18.0f; enemies[0].wpZ[3]=  7.0f;

    // Enemy 1 - area kanan atas
    enemies[1].x = 11.0f; enemies[1].z = 18.0f;
    enemies[1].angle = 0; enemies[1].speed = 0.065f; enemies[1].stuckFrames = 0;
    enemies[1].pathX.clear(); enemies[1].pathZ.clear(); enemies[1].pathIndex = 0;
    enemies[1].waypointIdx = 0; enemies[1].wpCount = 4;
    enemies[1].wpX[0]= 11.0f; enemies[1].wpZ[0]= 18.0f;
    enemies[1].wpX[1]= 17.0f; enemies[1].wpZ[1]= 18.0f;
    enemies[1].wpX[2]= 17.0f; enemies[1].wpZ[2]= 12.0f;
    enemies[1].wpX[3]= 11.0f; enemies[1].wpZ[3]= 12.0f;

    // Enemy 2 - area kiri tengah
    enemies[2].x = -18.0f; enemies[2].z = -5.0f;
    enemies[2].angle = 0; enemies[2].speed = 0.07f; enemies[2].stuckFrames = 0;
    enemies[2].pathX.clear(); enemies[2].pathZ.clear(); enemies[2].pathIndex = 0;
    enemies[2].waypointIdx = 0; enemies[2].wpCount = 4;
    enemies[2].wpX[0]=-18.0f; enemies[2].wpZ[0]= -5.0f;
    enemies[2].wpX[1]=-18.0f; enemies[2].wpZ[1]= -8.0f;
    enemies[2].wpX[2]=-12.0f; enemies[2].wpZ[2]= -8.0f;
    enemies[2].wpX[3]=-12.0f; enemies[2].wpZ[3]= -5.0f;

    // Enemy 3 - area tengah
    enemies[3].x = 4.0f; enemies[3].z = -3.0f;
    enemies[3].angle = 0; enemies[3].speed = 0.08f; enemies[3].stuckFrames = 0;
    enemies[3].pathX.clear(); enemies[3].pathZ.clear(); enemies[3].pathIndex = 0;
    enemies[3].waypointIdx = 0; enemies[3].wpCount = 4;
    enemies[3].wpX[0]=  4.0f; enemies[3].wpZ[0]= -3.0f;
    enemies[3].wpX[1]= 10.0f; enemies[3].wpZ[1]= -3.0f;
    enemies[3].wpX[2]= 10.0f; enemies[3].wpZ[2]= -8.0f;
    enemies[3].wpX[3]=  4.0f; enemies[3].wpZ[3]= -8.0f;

    // Enemy 4 - area kanan bawah (dekat exit)
    enemies[4].x = 14.0f; enemies[4].z = -8.0f;
    enemies[4].angle = 0; enemies[4].speed = 0.072f; enemies[4].stuckFrames = 0;
    enemies[4].pathX.clear(); enemies[4].pathZ.clear(); enemies[4].pathIndex = 0;
    enemies[4].waypointIdx = 0; enemies[4].wpCount = 4;
    enemies[4].wpX[0]= 14.0f; enemies[4].wpZ[0]= -8.0f;
    enemies[4].wpX[1]= 14.0f; enemies[4].wpZ[1]=-16.0f;
    enemies[4].wpX[2]=  8.0f; enemies[4].wpZ[2]=-16.0f;
    enemies[4].wpX[3]=  8.0f; enemies[4].wpZ[3]= -8.0f;

    // Enemy 5 - area tengah atas
    enemies[5].x = -5.0f; enemies[5].z = 13.0f;
    enemies[5].angle = 0; enemies[5].speed = 0.068f; enemies[5].stuckFrames = 0;
    enemies[5].pathX.clear(); enemies[5].pathZ.clear(); enemies[5].pathIndex = 0;
    enemies[5].waypointIdx = 0; enemies[5].wpCount = 4;
    enemies[5].wpX[0]= -5.0f; enemies[5].wpZ[0]= 13.0f;
    enemies[5].wpX[1]=  5.0f; enemies[5].wpZ[1]= 13.0f;
    enemies[5].wpX[2]=  5.0f; enemies[5].wpZ[2]=  7.0f;
    enemies[5].wpX[3]= -5.0f; enemies[5].wpZ[3]=  7.0f;

    // Enemy 6 - area kiri bawah
    enemies[6].x = -18.0f; enemies[6].z = -16.0f;
    enemies[6].angle = 0; enemies[6].speed = 0.075f; enemies[6].stuckFrames = 0;
    enemies[6].pathX.clear(); enemies[6].pathZ.clear(); enemies[6].pathIndex = 0;
    enemies[6].waypointIdx = 0; enemies[6].wpCount = 4;
    enemies[6].wpX[0]=-18.0f; enemies[6].wpZ[0]=-16.0f;
    enemies[6].wpX[1]=-10.0f; enemies[6].wpZ[1]=-16.0f;
    enemies[6].wpX[2]=-10.0f; enemies[6].wpZ[2]=-11.0f;
    enemies[6].wpX[3]=-18.0f; enemies[6].wpZ[3]=-11.0f;

    // Enemy 7 - patrol tengah luas (paling cepat)
    enemies[7].x =  2.0f; enemies[7].z =  7.0f;
    enemies[7].angle = 0; enemies[7].speed = 0.09f; enemies[7].stuckFrames = 0;
    enemies[7].pathX.clear(); enemies[7].pathZ.clear(); enemies[7].pathIndex = 0;
    enemies[7].waypointIdx = 0; enemies[7].wpCount = 4;
    enemies[7].wpX[0]=  2.0f; enemies[7].wpZ[0]=  7.0f;
    enemies[7].wpX[1]=  2.0f; enemies[7].wpZ[1]= -5.0f;
    enemies[7].wpX[2]= -5.0f; enemies[7].wpZ[2]= -5.0f;
    enemies[7].wpX[3]= -5.0f; enemies[7].wpZ[3]=  7.0f;
}
GLuint loadTexture(Image* image) {
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->width, image->height, 0, GL_RGB, GL_UNSIGNED_BYTE, image->pixels);
    return textureId;
}

// ===================================================
// RESET / INIT
// ===================================================
void resetGame(int level = 1) {
    currentLevel = level;
    levelClearing = false;
    levelClearTimer = 0.0f;
    LevelConfig &cfg = levelConfigs[level - 1];
    treasureCount    = cfg.treasures;
    level2Countdown  = cfg.hasCountdown ? cfg.countdown : 9999.0f;
    level2CountdownTimer = 0.0f;
    chaseMode        = false;
    lightsOn         = false;
    lightsOnTimer    = 0.0f;
    gameTimer        = 0.0f;
    level2CountdownTimer = 0.0f;
    for (int i = 0; i < treasureCount; i++) treasureCollected[i] = false;
    srand((unsigned)time(NULL));
    memset(keyStates, 0, sizeof(keyStates));
    memset(specialKeyStates, 0, sizeof(specialKeyStates));
    playerIsHiding = false;
    lightsOn = false;
    lightsOnTimer = 0.0f;
    gameTimer = 0.0f;
    playerAngle = 0.0f;
    finalScore = 0;
    wonWithTreasure = false;
    flickerTimer = 0.0f;
    flickerIntensity = 1.0f;
    breathTimer = 0.0f;

    const float exitInsideX = 18.5f;
    const float exitInsideZ = -12.0f;

    int playerTries = 0;
    do {
        randomPos(playerX, playerZ);
        playerTries++;
    } while (!pathExists(playerX, playerZ, exitInsideX, exitInsideZ) && playerTries < 1000);
    if (!pathExists(playerX, playerZ, exitInsideX, exitInsideZ)) {
        playerX = 18.0f;
        playerZ = -12.0f;
    }

    // HAPUS blok treasure lama, GANTI DENGAN:
for (int ti = 0; ti < treasureCount; ti++) {
    float tx, tz;
    int treasureTries = 0;
    do {
        randomPos(tx, tz);
        bool tooClose = false;
        // Tidak terlalu dekat player
        if (fabs(tx - playerX) < 3.0f && fabs(tz - playerZ) < 3.0f) tooClose = true;
        // Tidak terlalu dekat treasure lain
        for (int tj = 0; tj < ti; tj++) {
            if (fabs(tx - treasureX[tj]) < 3.0f && fabs(tz - treasureZ[tj]) < 3.0f)
                tooClose = true;
        }
        if (!tooClose &&
            pathExists(playerX, playerZ, tx, tz) &&
            pathExists(tx, tz, exitInsideX, exitInsideZ)) {
            break;
        }
        treasureTries++;
    } while (treasureTries < 1000);

    if (treasureTries >= 1000) {
        // Fallback
        treasureX[ti] = -18.0f + ti * 6.0f;
        treasureZ[ti] = 18.0f;
    } else {
        treasureX[ti] = tx;
        treasureZ[ti] = tz;
    }
    treasureCollected[ti] = false;
}

    initEnemies();
    gameState = STATE_PLAYING;
    startTime = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    lastTime = startTime;
    playBGM("spottheme.wav"); 
}

// ===================================================
// CHECK HIDING
// ===================================================
bool isNearHidingSpot() {
    float radius = 1.1f;
    for (int i = 0; i < hidingCount; i++) {
        float dx = playerX - hidingSpots[i].x;
        float dz = playerZ - hidingSpots[i].z;
        if (sqrtf(dx*dx + dz*dz) < radius) return true;
    }
    return false;
}

void checkTreasurePickup() {
    for (int i = 0; i < treasureCount; i++) {
        if (!treasureCollected[i]) {
            float dx = playerX - treasureX[i], dz = playerZ - treasureZ[i];
            if (sqrtf(dx*dx + dz*dz) < 1.0f) {  // ← buka {
                treasureCollected[i] = true;
            }  // ← tutup }
        }
    }
}

void checkExit() {
    if (playerX > 19.5f && playerZ > -13.5f && playerZ < -10.5f) {
        if (currentLevel < 3 && allTreasuresCollected()) {
            gameState = STATE_LEVEL_CLEAR;  // tampilkan layar pilihan
            levelClearTimer = 0.0f;
            return;
        }
        // Level 3 Hard → WIN
        wonWithTreasure = allTreasuresCollected();
        float elapsed = (float)glutGet(GLUT_ELAPSED_TIME)/1000.0f - startTime;
        int mult = currentLevel;
        finalScore = allTreasuresCollected()
                     ? (int)(10000.0f/(elapsed+1.0f))*10*treasuresHeld()*mult
                     : 0;
        gameState = STATE_WIN;
    }
}

// ===================================================
// ENEMY COLLISION
// ===================================================
bool checkEnemyCollision(float newX, float newZ) {
    const float enemyRadius = 0.18f;

    if (newX < MAP_MIN + enemyRadius || newX > MAP_MAX - enemyRadius ||
        newZ < MAP_MIN + enemyRadius || newZ > MAP_MAX - enemyRadius)
        return true;

    for (int i = 0; i < wallCount; i++) {
        float wx = walls[i].x, wz = walls[i].z;
        float halfX = walls[i].sx / 2.0f, halfZ = walls[i].sz / 2.0f;
        float closestX = std::max(wx - halfX, std::min(newX, wx + halfX));
        float closestZ = std::max(wz - halfZ, std::min(newZ, wz + halfZ));
        float dx = newX - closestX;
        float dz = newZ - closestZ;

        if (dx * dx + dz * dz < enemyRadius * enemyRadius)
            return true;
    }
    return false;
}

bool pathExists(float startX, float startZ, float targetX, float targetZ) {
    const float gridStep = 0.5f;
    const int gridSize = (int)((MAP_MAX - MAP_MIN) / gridStep) + 1;

    auto toIndex = [&](float v) {
        int idx = (int)roundf((v - MAP_MIN) / gridStep);
        if (idx < 0) idx = 0;
        if (idx >= gridSize) idx = gridSize - 1;
        return idx;
    };

    int sx = toIndex(startX), sz = toIndex(startZ);
    int tx = toIndex(targetX), tz = toIndex(targetZ);

    std::vector<int> visited(gridSize * gridSize, 0);
    std::vector<int> queue;
    queue.reserve(gridSize * gridSize);

    auto nodeId = [&](int x, int z) { return z * gridSize + x; };
    auto worldX = [&](int x) { return MAP_MIN + x * gridStep; };
    auto worldZ = [&](int z) { return MAP_MIN + z * gridStep; };
    auto walkable = [&](int x, int z) {
        return x >= 0 && x < gridSize && z >= 0 && z < gridSize &&
               !checkCollision(worldX(x), worldZ(z));
    };

    if (!walkable(sx, sz) || !walkable(tx, tz)) return false;

    visited[nodeId(sx, sz)] = 1;
    queue.push_back(nodeId(sx, sz));

    const int dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    for (size_t head = 0; head < queue.size(); head++) {
        int id = queue[head];
        int x = id % gridSize;
        int z = id / gridSize;

        if (x == tx && z == tz) return true;

        for (int i = 0; i < 4; i++) {
            int nx = x + dirs[i][0];
            int nz = z + dirs[i][1];
            if (!walkable(nx, nz)) continue;

            int nid = nodeId(nx, nz);
            if (visited[nid]) continue;

            visited[nid] = 1;
            queue.push_back(nid);
        }
    }

    return false;
}

bool tryMoveEnemy(Enemy &e, float stepX, float stepZ) {
    float candX = e.x + stepX;
    float candZ = e.z + stepZ;

    if (!checkEnemyCollision(candX, candZ)) {
        e.x = candX;
        e.z = candZ;
        return true;
    }

    if (!checkEnemyCollision(e.x + stepX, e.z)) {
        e.x += stepX;
        return true;
    }

    if (!checkEnemyCollision(e.x, e.z + stepZ)) {
        e.z += stepZ;
        return true;
    }

    const float fractions[5] = {0.75f, 0.5f, 0.35f, 0.2f, 0.1f};
    for (int k = 0; k < 5; k++) {
        float smallX = e.x + stepX * fractions[k];
        float smallZ = e.z + stepZ * fractions[k];
        if (!checkEnemyCollision(smallX, smallZ)) {
            e.x = smallX;
            e.z = smallZ;
            return true;
        }
    }

    return false;
}

bool enemyPathClear(float startX, float startZ, float targetX, float targetZ) {
    float dx = targetX - startX;
    float dz = targetZ - startZ;
    float dist = sqrtf(dx * dx + dz * dz);
    if (dist < 0.001f) return true;

    int steps = (int)(dist / 0.12f) + 1;
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / (float)steps;
        float x = startX + dx * t;
        float z = startZ + dz * t;
        if (checkEnemyCollision(x, z)) return false;
    }
    return true;
}

bool buildEnemyPath(Enemy &e, float targetX, float targetZ) {
    const float gridStep = 0.5f;
    const int gridSize = (int)((MAP_MAX - MAP_MIN) / gridStep) + 1;

    auto toIndex = [&](float v) {
        int idx = (int)roundf((v - MAP_MIN) / gridStep);
        if (idx < 0) idx = 0;
        if (idx >= gridSize) idx = gridSize - 1;
        return idx;
    };

    auto nodeId = [&](int x, int z) { return z * gridSize + x; };
    auto worldX = [&](int x) { return MAP_MIN + x * gridStep; };
    auto worldZ = [&](int z) { return MAP_MIN + z * gridStep; };
    auto walkable = [&](int x, int z) {
        return x >= 0 && x < gridSize && z >= 0 && z < gridSize &&
               !checkEnemyCollision(worldX(x), worldZ(z));
    };

    int sx = toIndex(e.x), sz = toIndex(e.z);
    int tx = toIndex(targetX), tz = toIndex(targetZ);

    if (!walkable(sx, sz) || !walkable(tx, tz)) return false;

    std::vector<int> queue;
    std::vector<int> prev(gridSize * gridSize, -1);
    queue.reserve(gridSize * gridSize);

    int start = nodeId(sx, sz);
    int goal = nodeId(tx, tz);
    prev[start] = start;
    queue.push_back(start);

    const int dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    for (size_t head = 0; head < queue.size(); head++) {
        int id = queue[head];
        if (id == goal) break;

        int x = id % gridSize;
        int z = id / gridSize;
        for (int i = 0; i < 4; i++) {
            int nx = x + dirs[i][0];
            int nz = z + dirs[i][1];
            if (!walkable(nx, nz)) continue;

            int nid = nodeId(nx, nz);
            if (prev[nid] != -1) continue;

            prev[nid] = id;
            queue.push_back(nid);
        }
    }

    if (prev[goal] == -1) return false;

    std::vector<int> reversed;
    for (int at = goal; at != start; at = prev[at]) {
        reversed.push_back(at);
    }
    std::reverse(reversed.begin(), reversed.end());

    e.pathX.clear();
    e.pathZ.clear();
    for (size_t i = 0; i < reversed.size(); i++) {
        int id = reversed[i];
        int x = id % gridSize;
        int z = id / gridSize;
        e.pathX.push_back(worldX(x));
        e.pathZ.push_back(worldZ(z));
    }

    e.targetX = targetX;
    e.targetZ = targetZ;
    e.pathIndex = 0;
    return !e.pathX.empty();
}

bool chooseEnemyPatrolTarget(Enemy &e) {
    for (int tries = 0; tries < 80; tries++) {
        float tx, tz;
        randomPos(tx, tz);

        float dx = tx - e.x;
        float dz = tz - e.z;
        if (sqrtf(dx * dx + dz * dz) < 4.0f) continue;

        if (!checkEnemyCollision(tx, tz) && buildEnemyPath(e, tx, tz)) {
            return true;
        }
    }
    return false;
}
void updateEnemies(float dt) {
    for (int i = 0; i < 8; i++) {
        Enemy &e = enemies[i];

        // ── CHASE MODE ──
        if (chaseMode || lightsOn) {
            if (playerIsHiding) {
                goto patrol_logic;
            }

            float pdx = e.x - playerX, pdz = e.z - playerZ;
            float distToPlayer = sqrtf(pdx*pdx + pdz*pdz);

            if (e.pathIndex >= (int)e.pathX.size() || distToPlayer > 2.0f) {
                buildEnemyPath(e, playerX, playerZ);
            }

            float chaseSpeed = e.speed * 3.0f * dt * 60.0f;

            if (!e.pathX.empty() && e.pathIndex < (int)e.pathX.size()) {
                float tx = e.pathX[e.pathIndex];
                float tz = e.pathZ[e.pathIndex];
                float dx = tx - e.x, dz = tz - e.z;
                float dist = sqrtf(dx*dx + dz*dz);
                if (dist < 0.18f) {
                    e.pathIndex++;
                } else {
                    float stepX = (dx / dist) * chaseSpeed;
                    float stepZ = (dz / dist) * chaseSpeed;
                    tryMoveEnemy(e, stepX, stepZ);
                    e.angle = atan2f(dx, dz) * 180.0f / (float)M_PI;
                }
            }

            if (distToPlayer < 0.85f && !playerIsHiding) {
                if (!jumpscareActive) {
                    jumpscareActive = true;
                    jumpscareTimer  = 0.0f;
                    jumpscareAlpha  = 0.0f;
                    jumpscareStartTime = glutGet(GLUT_ELAPSED_TIME);
                    playSFX("0521.wav"); 
}
            }
            continue;
        }

        // ── PATROL MODE ──
        patrol_logic:

        if (lightsOn && e.pathX.empty()) {
            chooseEnemyPatrolTarget(e);
            continue;
        }

        if (e.pathIndex >= (int)e.pathX.size()) {
            if (!chooseEnemyPatrolTarget(e)) continue;
            continue;
        }

        {
            float tx = e.pathX[e.pathIndex];
            float tz = e.pathZ[e.pathIndex];
            float dx = tx - e.x;
            float dz = tz - e.z;
            float dist = sqrtf(dx*dx + dz*dz);

            if (dist < 0.18f) {
                e.pathIndex++;
                e.stuckFrames = 0;
                if (e.pathIndex >= (int)e.pathX.size()) {
                    chooseEnemyPatrolTarget(e);
                }
                continue;
            }

            float moveSpeed = e.speed * (lightsOn ? 2.5f : 1.0f) * dt * 60.0f;
            float stepX = (dx / dist) * moveSpeed;
            float stepZ = (dz / dist) * moveSpeed;
            float oldX = e.x;
            float oldZ = e.z;

            if (tryMoveEnemy(e, stepX, stepZ)) {
                e.stuckFrames = 0;
                float movedX = e.x - oldX;
                float movedZ = e.z - oldZ;
                if (movedX * movedX + movedZ * movedZ > 0.0001f) {
                    e.angle = atan2f(movedX, movedZ) * 180.0f / (float)M_PI;
                }
            } else {
                e.stuckFrames++;
                int stuckThreshold = lightsOn ? 6 : 12;
                if (e.stuckFrames > stuckThreshold) {
                    if (!buildEnemyPath(e, e.targetX, e.targetZ)) {
                        chooseEnemyPatrolTarget(e);
                    }
                    e.stuckFrames = 0;
                }
            }

            if (!playerIsHiding) {
                float pdx = e.x - playerX, pdz = e.z - playerZ;
                if (sqrtf(pdx*pdx + pdz*pdz) < 0.85f) {
                    if (!jumpscareActive) {
                        jumpscareActive = true;
                        jumpscareTimer  = 0.0f;
                        jumpscareAlpha  = 0.0f;
                        jumpscareStartTime = glutGet(GLUT_ELAPSED_TIME);
                        playSFX("0521.wav");
                    }
                }
            }
        }
    }
}
void updateDoor(float dt) {
    float dx = playerX - exitX;
    float dz = playerZ - exitZ;
    doorShouldOpen = (sqrtf(dx*dx + dz*dz) < 4.0f);
    if (doorShouldOpen) { doorAngle += doorOpenSpeed * dt; if (doorAngle > 90.0f) doorAngle = 90.0f; }
    else                { doorAngle -= doorOpenSpeed * dt; if (doorAngle < 0.0f)  doorAngle = 0.0f;  }
}

// ===================================================
// LIGHT CYCLE & ATMOSPHERE
// ===================================================
void updateLights(float dt) {
    if (chaseMode && lightsOnTimer >= 9999.0f) return;

    gameTimer += dt;  // selalu increment

    if (!lightsOn) {
        // Tunggu interval sebelum lampu menyala
        if (gameTimer >= lightCycleInterval) {
            lightsOn = true;
            lightsOnTimer = 0.0f;
            gameTimer = 0.0f;
            if (levelConfigs[currentLevel - 1].lightsChaseMode) {
                chaseMode = true;
            }
        }
    } else {
        // Hitung durasi lampu menyala
        lightsOnTimer += dt;
        if (lightsOnTimer >= lightOnDuration) {
            lightsOn = false;
            lightsOnTimer = 0.0f;
            gameTimer = 0.0f;
            if (levelConfigs[currentLevel - 1].lightsChaseMode) {
                chaseMode = false;
                for (int i = 0; i < 8; i++) {
                    enemies[i].pathX.clear();
                    enemies[i].pathZ.clear();
                    enemies[i].pathIndex = 0;
                }
            }
        }
    }
}
void updateAtmosphere(float dt) {
    breathTimer += dt * 0.8f;
    if (!lightsOn) {
        flickerTimer += dt;
        if (flickerTimer > 0.05f + (float)(rand() % 10) / 100.0f) {
            flickerTimer = 0.0f;
            flickerIntensity = 0.88f + (float)(rand() % 25) / 100.0f;
        }
    } else {
        flickerIntensity = 1.0f;
    }
}
void update(int value) {
    if (gameState == STATE_PLAYING) {
        float now = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        float dt  = now - lastTime;
        if (dt > 0.1f) dt = 0.1f;
        lastTime = now;

        updateAtmosphere(dt);
        updateLights(dt);
        if (gameState != STATE_PLAYING) {
            glutPostRedisplay();
            glutTimerFunc(16, update, 0);
            return;
        }

        LevelConfig &cfg = levelConfigs[currentLevel - 1];
        if (cfg.hasCountdown && !chaseMode) {
            level2CountdownTimer += dt;
            if (level2CountdownTimer >= cfg.countdown) {
                chaseMode     = true;
                lightsOn      = true;
                lightsOnTimer = 9999.0f;
                gameTimer     = 0.0f;
            }
        }

        updatePlayerMovement(dt);
        updateEnemies(dt);

        // Update jumpscare
        if (jumpscareActive) {
            float elapsed = (glutGet(GLUT_ELAPSED_TIME) - jumpscareStartTime) / 1000.0f;
            if (elapsed < 1.0f) {
                jumpscareAlpha = 1.0f;
            } else {
                jumpscareActive = false;
                jumpscareAlpha  = 0.0f;
                finalScore      = 0;
                wonWithTreasure = false;
                gameState       = STATE_GAMEOVER;
            }
        }

        if (gameState != STATE_PLAYING) {
            glutPostRedisplay();
            glutTimerFunc(16, update, 0);
            return;
        }

        updateDoor(dt);
        checkTreasurePickup();
        checkExit();
    }   // ← tutup if (gameState == STATE_PLAYING)

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}
// ===================================================
// INPUT SYSTEMS
// ===================================================
void mouseMotion(int x, int y) {
    if (gameState != STATE_PLAYING) return;
    (void)y;

    if (ignoreNextMouseMove) {
        ignoreNextMouseMove = false;
        return;
    }

    int centerX = windowWidth / 2;
    int centerY = windowHeight / 2;
    int deltaX = x - centerX;

    if (deltaX == 0) return;

    playerAngle -= deltaX * mouseSensitivity;
    if (playerAngle >= 360.0f) playerAngle -= 360.0f;
    if (playerAngle <    0.0f) playerAngle += 360.0f;

    ignoreNextMouseMove = true;
    glutWarpPointer(centerX, centerY);
    glutPostRedisplay();
}

void movePlayer(float forward, float strafe) {
    if (playerIsHiding) return;

    float rad = playerAngle * (float)M_PI / 180.0f;
    float newX = playerX + sinf(rad)*forward + cosf(rad)*strafe;
    float newZ = playerZ + cosf(rad)*forward - sinf(rad)*strafe;
    if (!checkCollision(newX, newZ)) { playerX = newX; playerZ = newZ; }
}

bool isShiftDown() {
#ifdef _WIN32
    return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
#else
    return false;
#endif
}

bool isCtrlDown() {
#ifdef _WIN32
    return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
#else
    return false;
#endif
}

int movementKeyIndex(unsigned char key) {
    switch (key) {
        case 'w': case 'W': case 23: return 'w';
        case 'a': case 'A': case 1:  return 'a';
        case 's': case 'S': case 19: return 's';
        case 'd': case 'D': case 4:  return 'd';
    }
    return 0;
}

void updatePlayerMovement(float dt) {
    if (playerIsHiding) return;

    float forward = 0.0f;
    float strafe = 0.0f;
    float currentSpeed = speed;

    if (isShiftDown()) currentSpeed *= runMultiplier;
    if (isCtrlDown()) currentSpeed *= slowMultiplier;

    if (keyStates['w'] || specialKeyStates[GLUT_KEY_UP]) forward += currentSpeed * dt;
    if (keyStates['s'] || specialKeyStates[GLUT_KEY_DOWN]) forward -= currentSpeed * dt;
    if (keyStates['a'] || specialKeyStates[GLUT_KEY_LEFT]) strafe += currentSpeed * dt;
    if (keyStates['d'] || specialKeyStates[GLUT_KEY_RIGHT]) strafe -= currentSpeed * dt;

    if (forward != 0.0f && strafe != 0.0f) {
        const float diagonalFix = 0.70710678f;
        forward *= diagonalFix;
        strafe *= diagonalFix;
    }

    if (forward != 0.0f || strafe != 0.0f) {
        movePlayer(forward, strafe);
    }
}
void keyboard(unsigned char key, int x, int y) {
    // ── LEVEL CLEAR: pilih lanjut atau menu ──
    if (gameState == STATE_LEVEL_CLEAR) {
        if (key == 'n' || key == 'N') {
            resetGame(currentLevel + 1);  // lanjut ke level berikutnya
        }
        if (key == 'm' || key == 'M') {
            gameState = STATE_MENU;
            glutPostRedisplay();
        }
        return;
    }

    if (gameState == STATE_MENU) {
        if (key == '1') { selectedLevel = 1; glutPostRedisplay(); return; }
        if (key == '2') { selectedLevel = 2; glutPostRedisplay(); return; }
        if (key == '3') { selectedLevel = 3; glutPostRedisplay(); return; }
        if (key == 13 || key == ' ') {
            resetGame(selectedLevel);
        }
        return;
    }

    if (gameState == STATE_GAMEOVER || gameState == STATE_WIN) {
        if (key == 'r' || key == 'R') {
            resetGame(currentLevel);  // retry di level yang sama
        }
        if (key == 'm' || key == 'M') gameState = STATE_MENU;
        return;
    }

    // STATE_PLAYING
    switch (key) {
        case 'h': case 'H':
            if (isNearHidingSpot()) playerIsHiding = !playerIsHiding;
            else playerIsHiding = false;
            break;
        case 27: exit(0); break;
        default: {
            int moveKey = movementKeyIndex(key);
            if (moveKey) keyStates[moveKey] = true;
            break;
        }
    }
    glutPostRedisplay();
}
void keyboardUp(unsigned char key, int x, int y) {
    (void)x; (void)y;
    int moveKey = movementKeyIndex(key);
    if (moveKey) keyStates[moveKey] = false;
}

void specialKeys(int key, int x, int y) {
    if (gameState != STATE_PLAYING) return;
    switch (key) {
        case GLUT_KEY_UP:
        case GLUT_KEY_DOWN:
        case GLUT_KEY_LEFT:
        case GLUT_KEY_RIGHT:
            specialKeyStates[key] = true;
            break;
    }
    glutPostRedisplay();
}

void specialKeysUp(int key, int x, int y) {
    (void)x; (void)y;
    if (key >= 0 && key < 256) specialKeyStates[key] = false;
}

// ===================================================
// RENDERING HELPERS & MAP COMPONENTS
// ===================================================
void applyFlicker(float r, float g, float b) {
    glColor3f(r*flickerIntensity, g*flickerIntensity, b*flickerIntensity);
}

float cellHash(int ix, int iz) {
    int n = ix*1619 + iz*31337;
    n = (n<<13)^n;
    return 1.0f - ((n*(n*n*15731+789221)+1376312589)&0x7fffffff)/1073741824.0f;
}

void drawStoneSlab(float wx, float wz, float sw, float sh,
                   float bR, float bG, float bB,
                   float cR, float cG, float cB, int cracks) {
    glColor3f(bR,bG,bB);
    glBegin(GL_QUADS);
    glVertex3f(wx,     0,wz);    glVertex3f(wx+sw,0,wz);
    glVertex3f(wx+sw,  0,wz+sh); glVertex3f(wx,   0,wz+sh);
    glEnd();
    float g=0.04f;
    glColor3f(cR*0.55f,cG*0.55f,cB*0.55f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(wx+g,0.01f,wz+g);    glVertex3f(wx+sw-g,0.01f,wz+g);
    glVertex3f(wx+sw-g,0.01f,wz+sh-g); glVertex3f(wx+g,0.01f,wz+sh-g);
    glEnd();
    glColor3f(cR,cG,cB); glLineWidth(1);
    for (int c=0;c<cracks;c++) {
        float h1=cellHash((int)(wx*10+c),(int)(wz*10+c*7));
        float h2=cellHash((int)(wx*10+c*3),(int)(wz*10+c*11));
        float h3=cellHash((int)(wx*10+c*5),(int)(wz*10+c*13));
        float h4=cellHash((int)(wx*10+c*9),(int)(wz*10+c*17));
        glBegin(GL_LINES);
        glVertex3f(wx+g+fabsf(h1)*(sw-2*g),0.012f,wz+g+fabsf(h2)*(sh-2*g));
        glVertex3f(wx+g+fabsf(h3)*(sw-2*g),0.012f,wz+g+fabsf(h4)*(sh-2*g));
        glEnd();
    }
}

void drawFloor() {
    float slabW=2.0f,slabH=1.5f;
    int cols=(int)((MAP_MAX-MAP_MIN)/slabW)+1;
    int rows=(int)((MAP_MAX-MAP_MIN)/slabH)+1;
    for (int row=0;row<rows;row++) {
        float fz=MAP_MIN+row*slabH;
        float ox=(row%2==0)?0.0f:slabW*0.5f;
        for (int col=0;col<cols;col++) {
            float fx=MAP_MIN-ox+col*slabW;
            if (fx+slabW<MAP_MIN||fx>MAP_MAX||fz+slabH<MAP_MIN||fz>MAP_MAX) continue;
            float var=cellHash(col+row*47,row+col*31)*0.5f+0.5f;
            float r,g,b,cr,cg,cb; int nc;
            if (lightsOn) {
                float base=0.50f+var*0.18f,warm=0.03f+var*0.04f;
                r=base+warm;g=base+warm*0.5f;b=base-warm*0.5f;
                cr=0.28f;cg=0.25f;cb=0.22f;nc=(int)(var*2.5f);
            } else {
                float base=0.08f+var*0.07f;
                r=base*flickerIntensity*0.9f;g=base*flickerIntensity*0.95f;b=(base+0.04f)*flickerIntensity;
                cr=cg=0.03f*flickerIntensity;cb=0.06f*flickerIntensity;nc=(int)(var*2.0f);
            }
            drawStoneSlab(fx,fz,slabW,slabH,r,g,b,cr,cg,cb,nc);
        }
    }
    float ceilH=3.5f,cofSz=3.0f,cofBdr=0.18f,cofDep=0.12f;
    if (lightsOn) glColor3f(0.68f,0.64f,0.56f);
    else { float b=0.07f*flickerIntensity; glColor3f(b*0.85f,b*0.8f,b); }
    glBegin(GL_QUADS);
    glVertex3f(MAP_MIN,ceilH,MAP_MIN);glVertex3f(MAP_MAX,ceilH,MAP_MIN);
    glVertex3f(MAP_MAX,ceilH,MAP_MAX);glVertex3f(MAP_MIN,ceilH,MAP_MAX);
    glEnd();
    for (float bx=MAP_MIN;bx<=MAP_MAX+cofSz;bx+=cofSz) {
        glPushMatrix();glTranslatef(bx,ceilH-cofDep*0.5f,(MAP_MIN+MAP_MAX)*0.5f);
        glScalef(cofBdr,cofDep,MAP_MAX-MAP_MIN+1.0f);
        if(lightsOn)glColor3f(0.52f,0.48f,0.40f);
        else{float b=0.04f*flickerIntensity;glColor3f(b*0.8f,b*0.75f,b);}
        glutSolidCube(1);glPopMatrix();
    }
    for (float bz=MAP_MIN;bz<=MAP_MAX+cofSz;bz+=cofSz) {
        glPushMatrix();glTranslatef((MAP_MIN+MAP_MAX)*0.5f,ceilH-cofDep*0.5f,bz);
        glScalef(MAP_MAX-MAP_MIN+1.0f,cofDep,cofBdr);
        if(lightsOn)glColor3f(0.52f,0.48f,0.40f);
        else{float b=0.04f*flickerIntensity;glColor3f(b*0.8f,b*0.75f,b);}
        glutSolidCube(1);glPopMatrix();
    }
}

// ===================================================
// FIXED RENDERING: TEKSTUR DI TENGAH DINDING SAMPING
// ===================================================
void drawWall(float x, float z, float sx, float sz) {
    float wallH = 3.5f; 
    float wainH = wallH * 0.35f; 
    float halfX = sx / 2.0f;
    float halfZ = sz / 2.0f;

    float tileX = sx / 3.5f;
    float tileZ = sz / 3.5f;

    // 1. GAMBAR BAGIAN BAWAH DINDING (POLOS COKELAT)
    if (lightsOn) {
        glColor3f(0.30f, 0.28f, 0.25f);
    } else {
        glColor3f(0.05f * flickerIntensity, 0.04f * flickerIntensity, 0.06f * flickerIntensity);
    }

    glBegin(GL_QUADS);
    // Depan bawah
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(x - halfX, 0.0f,  z + halfZ); glVertex3f(x + halfX, 0.0f,  z + halfZ);
    glVertex3f(x + halfX, wainH, z + halfZ); glVertex3f(x - halfX, wainH, z + halfZ);
    // Belakang bawah
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(x - halfX, 0.0f,  z - halfZ); glVertex3f(x - halfX, wainH, z - halfZ);
    glVertex3f(x + halfX, wainH, z - halfZ); glVertex3f(x + halfX, 0.0f,  z - halfZ);
    // Kiri bawah
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(x - halfX, 0.0f,  z - halfZ); glVertex3f(x - halfX, 0.0f,  z + halfZ);
    glVertex3f(x - halfX, wainH, z + halfZ); glVertex3f(x - halfX, wainH, z - halfZ);
    // Kanan bawah
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(x + halfX, 0.0f,  z - halfZ); glVertex3f(x + halfX, wainH, z - halfZ);
    glVertex3f(x + halfX, wainH, z + halfZ); glVertex3f(x + halfX, 0.0f,  z + halfZ);
    glEnd();

    // 2. GAMBAR LIS GARIS KUNING / EMAS DI TENGAH DINDING
    float trimH = 0.07f; 
    if (lightsOn) {
        glColor3f(0.55f, 0.50f, 0.42f);
    } else {
        glColor3f(0.15f * flickerIntensity, 0.13f * flickerIntensity, 0.10f * flickerIntensity);
    }

    glBegin(GL_QUADS);
    // Garis depan
    glVertex3f(x - halfX, wainH, z + halfZ);         glVertex3f(x + halfX, wainH, z + halfZ);
    glVertex3f(x + halfX, wainH + trimH, z + halfZ); glVertex3f(x - halfX, wainH + trimH, z + halfZ);
    // Garis belakang
    glVertex3f(x - halfX, wainH, z - halfZ);         glVertex3f(x - halfX, wainH + trimH, z - halfZ);
    glVertex3f(x + halfX, wainH + trimH, z - halfZ); glVertex3f(x + halfX, wainH, z - halfZ);
    // Garis kiri
    glVertex3f(x - halfX, wainH, z - halfZ);         glVertex3f(x - halfX, wainH, z + halfZ);
    glVertex3f(x - halfX, wainH + trimH, z + halfZ); glVertex3f(x - halfX, wainH + trimH, z - halfZ);
    // Garis kanan
    glVertex3f(x + halfX, wainH, z - halfZ);         glVertex3f(x + halfX, wainH + trimH, z - halfZ);
    glVertex3f(x + halfX, wainH + trimH, z + halfZ); glVertex3f(x + halfX, wainH, z + halfZ);
    glEnd();

    // 3. GAMBAR BAGIAN ATAS DINDING (BERMOTIF BERJEJER SURAT)
    float topStart = wainH + trimH; 

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, wallTextureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (lightsOn) {
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        glColor3f(flickerIntensity * 0.4f, flickerIntensity * 0.4f, flickerIntensity * 0.5f);
    }

    glBegin(GL_QUADS);
    // Sisi Depan atas (+Z)
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f,  0.0f); glVertex3f(x - halfX, topStart, z + halfZ);
    glTexCoord2f(tileX, 0.0f); glVertex3f(x + halfX, topStart, z + halfZ);
    glTexCoord2f(tileX, 1.0f); glVertex3f(x + halfX, wallH,    z + halfZ);
    glTexCoord2f(0.0f,  1.0f); glVertex3f(x - halfX, wallH,    z + halfZ);

    // Sisi Belakang atas (-Z)
    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(tileX, 0.0f); glVertex3f(x - halfX, topStart, z - halfZ);
    glTexCoord2f(tileX, 1.0f); glVertex3f(x - halfX, wallH,    z - halfZ);
    glTexCoord2f(0.0f,  1.0f); glVertex3f(x + halfX, wallH,    z - halfZ);
    glTexCoord2f(0.0f,  0.0f); glVertex3f(x + halfX, topStart, z - halfZ);

    // Sisi Kiri atas (-X)
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f,  0.0f); glVertex3f(x - halfX, topStart, z - halfZ);
    glTexCoord2f(tileZ, 0.0f); glVertex3f(x - halfX, topStart, z + halfZ);
    glTexCoord2f(tileZ, 1.0f); glVertex3f(x - halfX, wallH,    z + halfZ);
    glTexCoord2f(0.0f,  1.0f); glVertex3f(x - halfX, wallH,    z - halfZ);

    // Sisi Kanan atas (+X)
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(tileZ, 0.0f); glVertex3f(x + halfX, topStart, z - halfZ);
    glTexCoord2f(tileZ, 1.0f); glVertex3f(x + halfX, wallH,    z - halfZ);
    glTexCoord2f(0.0f,  1.0f); glVertex3f(x + halfX, wallH,    z + halfZ);
    glTexCoord2f(0.0f,  0.0f); glVertex3f(x + halfX, topStart, z + halfZ);
    glEnd();

    // ── SISI ATAS PILAR (Atap) ──
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f,  tileZ); glVertex3f(x - halfX, wallH, z - halfZ);
    glTexCoord2f(0.0f,  0.0f);  glVertex3f(x - halfX, wallH, z + halfZ);
    glTexCoord2f(tileX, 0.0f);  glVertex3f(x + halfX, wallH, z + halfZ);
    glTexCoord2f(tileX, tileZ); glVertex3f(x + halfX, wallH, z - halfZ);
    glEnd();

    glDisable(GL_TEXTURE_2D); 
}

void drawMaze() {
    for (int i=0;i<wallCount;i++) drawWall(walls[i].x,walls[i].z,walls[i].sx,walls[i].sz);
}

void drawExit() {
    const float frameX  = 20.0f;
    const float doorZ   = exitZ;       // -12.0f
    const float doorW   = 1.5f;        // lebih lebar sesuai gap besar
    const float doorH   = 3.4f;
    const float depth   = 0.22f;
    const float doorShift = 0.0f;
    const float fLeft   = doorZ - doorW + doorShift;
    const float fRight  = doorZ + doorW + doorShift;

    float t     = (float)glutGet(GLUT_ELAPSED_TIME) / 800.0f;
    float pulse = 0.55f + 0.45f * sinf(t * 2.5f);

    // ── helper warna ──
    auto stoneCol = [&](float b) {
        if (lightsOn) glColor3f(b*0.62f, b*0.60f, b*0.55f);
        else glColor3f(b*0.22f*flickerIntensity, b*0.20f*flickerIntensity, b*0.18f*flickerIntensity);
    };
    auto brassCol = [&](float b) {
        // Ornamen coklat tua / gelap
        if (lightsOn) glColor3f(b*0.38f, b*0.22f, b*0.10f);
        else glColor3f(b*0.16f*flickerIntensity, b*0.09f*flickerIntensity, b*0.04f*flickerIntensity);
    };

    auto woodCol = [&](float b) {
        // Kayu coklat hangat
        if (lightsOn) glColor3f(b*0.45f, b*0.28f, b*0.12f);
        else glColor3f(b*0.20f*flickerIntensity, b*0.12f*flickerIntensity, b*0.05f*flickerIntensity);
    };

    // ==============================================
    // 1. LANTAI GLOW HIJAU
    // ==============================================
    glPushMatrix();
    glTranslatef(19.5f, 0.02f, doorZ);
    glColor3f(0, pulse*0.7f, pulse*0.3f);
    glBegin(GL_QUADS);
    glVertex3f(-0.3f,0,-doorW); glVertex3f(0.3f,0,-doorW);
    glVertex3f(0.3f, 0, doorW); glVertex3f(-0.3f,0, doorW);
    glEnd();
    glPopMatrix();

    // ==============================================
    // 2. PILAR BATU KIRI & KANAN
    // ==============================================
    const float pilW  = 0.42f;  // lebar pilar
    const float pilD  = 0.38f;  // kedalaman pilar

    for (int side = 0; side < 2; side++) {
        float pz = (side == 0) ? fLeft - pilW*0.5f : fRight + pilW*0.5f;
        int blocks = 6;
        float bh = doorH / blocks;

        for (int b = 0; b < blocks; b++) {
            float bv  = (b % 2 == 0) ? 0.78f : 0.62f;
            float by0 = b * bh;
            float by1 = (b+1) * bh;

            stoneCol(bv);
            // Muka depan pilar
            glBegin(GL_QUADS);
            glVertex3f(frameX,        by0, pz - pilW*0.5f);
            glVertex3f(frameX,        by1, pz - pilW*0.5f);
            glVertex3f(frameX,        by1, pz + pilW*0.5f);
            glVertex3f(frameX,        by0, pz + pilW*0.5f);
            glEnd();
            // Sisi luar pilar
            stoneCol(bv * 0.75f);
            glBegin(GL_QUADS);
            glVertex3f(frameX,        by0, pz + (side==0?-1:1)*pilW*0.5f);
            glVertex3f(frameX-pilD,   by0, pz + (side==0?-1:1)*pilW*0.5f);
            glVertex3f(frameX-pilD,   by1, pz + (side==0?-1:1)*pilW*0.5f);
            glVertex3f(frameX,        by1, pz + (side==0?-1:1)*pilW*0.5f);
            glEnd();
            // Garis mortar
            glColor3f(0.08f,0.07f,0.06f);
            glLineWidth(1.2f);
            glBegin(GL_LINE_LOOP);
            glVertex3f(frameX, by0, pz-pilW*0.5f);
            glVertex3f(frameX, by1, pz-pilW*0.5f);
            glVertex3f(frameX, by1, pz+pilW*0.5f);
            glVertex3f(frameX, by0, pz+pilW*0.5f);
            glEnd();
        }

        // Kepala pilar (capital) — blok lebih lebar
        stoneCol(0.90f);
        float capY0 = doorH - 0.05f;
        float capY1 = doorH + 0.22f;
        float capExtra = 0.10f;
        glBegin(GL_QUADS);
        glVertex3f(frameX+0.04f,  capY0, pz - pilW*0.5f - capExtra);
        glVertex3f(frameX+0.04f,  capY1, pz - pilW*0.5f - capExtra);
        glVertex3f(frameX+0.04f,  capY1, pz + pilW*0.5f + capExtra);
        glVertex3f(frameX+0.04f,  capY0, pz + pilW*0.5f + capExtra);
        glEnd();
    }

    // ==============================================
    // 5. DAUN PINTU TUNGGAL (berengsel di kiri = fLeft)
    // ==============================================
    glPushMatrix();
    glTranslatef(frameX + 0.01f, 0.0f, fLeft);
    glRotatef(doorAngle, 0, 1, 0);   // buka ke dalam (negatif Z)

    const float leafW = doorW * 2.0f;  // lebar penuh
    const float leafD = 0.09f;

    // ── Body kayu (papan vertikal) ──
    const int boards = 5;
    float boardW = leafW / boards;

    for (int b = 0; b < boards; b++) {
        float bz0 = b * boardW;
        float bz1 = (b+1) * boardW - 0.025f; // celah antar papan
        float bv  = (b % 2 == 0) ? 1.0f : 0.82f;

        woodCol(bv);
        glBegin(GL_QUADS);
        // Muka depan
        glVertex3f(0,      0,    bz0); glVertex3f(0,     doorH, bz0);
        glVertex3f(0,      doorH,bz1); glVertex3f(0,     0,     bz1);
        // Belakang
        glVertex3f(-leafD, 0,    bz0); glVertex3f(-leafD,0,     bz1);
        glVertex3f(-leafD, doorH,bz1); glVertex3f(-leafD,doorH, bz0);
        // Sisi kecil papan
        woodCol(bv * 0.70f);
        glVertex3f(0, 0, bz1); glVertex3f(-leafD,0,bz1);
        glVertex3f(-leafD,doorH,bz1); glVertex3f(0,doorH,bz1);
        glEnd();
    }

    // ── 3 STRIP BESI HORIZONTAL ──
    float stripY[3] = { doorH*0.18f, doorH*0.50f, doorH*0.80f };
    for (int i = 0; i < 3; i++) {
        // Strip utama
        brassCol(1.0f);
        glPushMatrix();
        glTranslatef(-leafD*0.5f, stripY[i], leafW*0.5f);
        glScalef(leafD + 0.04f, 0.11f, leafW * 0.96f);
        glutSolidCube(1);
        glPopMatrix();

        // Paku/rivet di kiri-tengah-kanan
        float rivZ[3] = { leafW*0.15f, leafW*0.50f, leafW*0.85f };
        for (int r = 0; r < 3; r++) {
            glColor3f(0.75f, 0.60f, 0.20f);
            glPushMatrix();
            glTranslatef(0.04f, stripY[i], rivZ[r]);
            glutSolidSphere(0.038f, 7, 7);
            glPopMatrix();
        }
    }

    // ── ORNAMEN TENGAH (lingkaran brass) ──
    brassCol(1.0f);
    glPushMatrix();
    glTranslatef(0.05f, doorH * 0.50f, leafW * 0.50f);
    glutSolidSphere(0.13f, 12, 12);
    glPopMatrix();

    // ── HANDLE / RING ──
    glColor3f(0.80f, 0.65f, 0.22f);
    glPushMatrix();
    glTranslatef(-leafD - 0.06f, doorH * 0.46f, leafW * 0.85f);
    glutSolidTorus(0.025f, 0.10f, 8, 14);
    glPopMatrix();

    glPopMatrix(); // end door leaf

    glPopMatrix();
}
void drawHidingSpots() {
    float t=(float)glutGet(GLUT_ELAPSED_TIME)/1000.0f;
    for (int i=0;i<hidingCount;i++) {
        glPushMatrix();glTranslatef(hidingSpots[i].x,0,hidingSpots[i].z);
        if(lightsOn)glColor3f(0.55f,0.38f,0.18f);
        else{float p=0.3f+0.08f*sinf(t*1.5f+i);applyFlicker(p*0.9f,p*0.65f,p*0.2f);}
        glPushMatrix();glTranslatef(0,0.42f,0);glScalef(0.75f,0.84f,0.75f);glutSolidCube(1);glPopMatrix();
        if(lightsOn)glColor3f(0.65f,0.48f,0.22f);
        else{float p=0.4f+0.10f*sinf(t*1.5f+i);applyFlicker(p,p*0.7f,p*0.25f);}
        glPushMatrix();glTranslatef(0,0.88f,0);glScalef(0.80f,0.10f,0.80f);glutSolidCube(1);glPopMatrix();
        glColor3f(0.18f,0.11f,0.04f);
        glPushMatrix();glTranslatef(0,0.42f,0.39f);glScalef(0.75f,0.06f,0.04f);glutSolidCube(1);glPopMatrix();
        glPushMatrix();glTranslatef(0,0.42f,0.39f);glScalef(0.06f,0.84f,0.04f);glutSolidCube(1);glPopMatrix();
        float glow=0.25f+0.1f*sinf(t*2.0f+i);
        if(lightsOn)glColor3f(0.8f,0.7f,0);else glColor3f(glow*0.9f,glow*0.6f,0);
        glBegin(GL_QUADS);
        glVertex3f(-0.5f,0.01f,-0.5f);glVertex3f(0.5f,0.01f,-0.5f);
        glVertex3f(0.5f,0.01f,0.5f);glVertex3f(-0.5f,0.01f,0.5f);
        glEnd();
        glPopMatrix();
    }
}

void drawCoin(float x,float y,float z,float radius,float thickness) {
    glPushMatrix();glTranslatef(x,y,z);
    int segs=12;
    glBegin(GL_TRIANGLE_FAN);glVertex3f(0,thickness/2,0);
    for(int s=0;s<=segs;s++){float a=s*2*(float)M_PI/segs;glVertex3f(cosf(a)*radius,thickness/2,sinf(a)*radius);}glEnd();
    glBegin(GL_TRIANGLE_FAN);glVertex3f(0,-thickness/2,0);
    for(int s=segs;s>=0;s--){float a=s*2*(float)M_PI/segs;glVertex3f(cosf(a)*radius,-thickness/2,sinf(a)*radius);}glEnd();
    glBegin(GL_QUAD_STRIP);
    for(int s=0;s<=segs;s++){float a=s*2*(float)M_PI/segs,cx=cosf(a)*radius,cz=sinf(a)*radius;
        glVertex3f(cx,-thickness/2,cz);glVertex3f(cx,thickness/2,cz);}glEnd();
    glPopMatrix();
}
void drawTreasure() {
    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 600.0f;
    for (int i = 0; i < treasureCount; i++) {
        if (treasureCollected[i]) continue;
        float hover = sinf(t * 1.5f + i * 2.0f) * 0.08f;
        float gp = 0.3f + 0.2f * sinf(t * 3 + i);

        glColor3f(gp, gp * 0.7f, 0);
        glPushMatrix(); glTranslatef(treasureX[i], 0.02f, treasureZ[i]);
        glBegin(GL_TRIANGLE_FAN); glVertex3f(0, 0, 0);
        for (int s = 0; s <= 20; s++) {
            float a = s * 2 * (float)M_PI / 20;
            float r = 0.8f + 0.15f * sinf(t * 4 + s);
            glVertex3f(cosf(a)*r, 0, sinf(a)*r);
        }
        glEnd(); glPopMatrix();

        glPushMatrix(); glTranslatef(treasureX[i], hover, treasureZ[i]);
        float gR = lightsOn ? 1.0f : 0.85f * flickerIntensity;
        float gG = lightsOn ? 0.78f : 0.65f * flickerIntensity;
        glColor3f(gR, gG, 0);
        drawCoin(0.15f, 0.04f, 0.10f, 0.18f, 0.06f);
        drawCoin(-0.20f, 0.04f, 0.05f, 0.16f, 0.06f);
        drawCoin(0.00f, 0.04f, -0.20f, 0.17f, 0.06f);
        drawCoin(0.25f, 0.04f, -0.05f, 0.15f, 0.06f);
        drawCoin(-0.10f, 0.04f, -0.15f, 0.18f, 0.06f);
        for (int k = 0; k < 5; k++)
            drawCoin(0, 0.10f + k * 0.08f, 0, 0.22f - k * 0.01f, 0.08f);
        float gl = 0.5f + 0.5f * sinf(t * 5 + i);
        glColor3f(1, 1, gl * 0.5f);
        glPushMatrix(); glTranslatef(0, 0.55f, 0); glutSolidSphere(0.06f, 8, 8); glPopMatrix();
        glPopMatrix();
    }
}
void drawEnemyBody(int i) {
    Enemy &e = enemies[i];
    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 500.0f;
    float bob = sinf(t * 3 + i * 1.5f) * 0.06f;

    glPushMatrix();
    glTranslatef(e.x, bob, e.z);
    glRotatef(e.angle + 180.0f, 0, 1, 0);

    // ── BAYANGAN DI LANTAI ──
    glColor3f(0.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(0, 0.01f, 0);
    glScalef(1.0f, 0.02f, 0.8f);
    glutSolidSphere(0.5f, 8, 4);
    glPopMatrix();

    // ── JUBAH / BODY UTAMA (lebih tinggi, ramping) ──
    if (lightsOn) glColor3f(0.08f, 0.04f, 0.12f);
    else glColor3f(0.04f, 0.02f, 0.08f);
    glPushMatrix();
    glTranslatef(0, 1.0f, 0);
    glScalef(0.45f, 1.1f, 0.3f);
    glutSolidCube(1);
    glPopMatrix();

    // ── BAHU LEBAR ──
    if (lightsOn) glColor3f(0.10f, 0.05f, 0.15f);
    else glColor3f(0.05f, 0.02f, 0.10f);
    glPushMatrix();
    glTranslatef(0, 1.55f, 0);
    glScalef(0.75f, 0.18f, 0.32f);
    glutSolidCube(1);
    glPopMatrix();

    // ── KEPALA (elongated, lebih panjang ke atas) ──
    if (lightsOn) glColor3f(0.12f, 0.06f, 0.16f);
    else glColor3f(0.06f, 0.03f, 0.10f);
    glPushMatrix();
    glTranslatef(0, 1.95f, 0);
    glScalef(0.85f, 1.1f, 0.75f);
    glutSolidSphere(0.22f, 10, 10);
    glPopMatrix();

    // ── MATA MERAH MENYALA ──
    float eg = lightsOn ? 1.0f : (0.7f + 0.3f * sinf(t * 5 + i));
    float eyeFlare = 0.5f + 0.5f * sinf(t * 7 + i * 2.1f); // kedip acak
    glColor3f(eg, eg * eyeFlare * 0.1f, 0.0f);
    glPushMatrix(); glTranslatef( 0.09f, 2.0f, -0.19f); glutSolidSphere(0.06f, 8, 8); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.09f, 2.0f, -0.19f); glutSolidSphere(0.06f, 8, 8); glPopMatrix();

    // ── GLOW MATA (halo kecil) ──
    glColor3f(eg * 0.4f, 0.0f, 0.0f);
    glPushMatrix(); glTranslatef( 0.09f, 2.0f, -0.17f); glutSolidSphere(0.10f, 6, 6); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.09f, 2.0f, -0.17f); glutSolidSphere(0.10f, 6, 6); glPopMatrix();

    // ── MULUT / CELAH GELAP ──
    glColor3f(0.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(0, 1.82f, -0.19f);
    glScalef(0.18f, 0.04f, 0.04f);
    glutSolidCube(1);
    glPopMatrix();

    // ── TANDUK KECIL ──
    if (lightsOn) glColor3f(0.25f, 0.05f, 0.05f);
    else glColor3f(0.12f, 0.02f, 0.02f);
    glPushMatrix();
    glTranslatef( 0.08f, 2.22f, 0);
    glRotatef(-20, 0, 0, 1);
    glScalef(0.05f, 0.18f, 0.05f);
    glutSolidCone(1, 1, 6, 1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-0.08f, 2.22f, 0);
    glRotatef( 20, 0, 0, 1);
    glScalef(0.05f, 0.18f, 0.05f);
    glutSolidCone(1, 1, 6, 1);
    glPopMatrix();

    // ── TANGAN PANJANG (cakar) ──
    float armSw = sinf(t * 3 + i * 1.5f) * 20.0f;
    if (lightsOn) glColor3f(0.10f, 0.05f, 0.14f);
    else glColor3f(0.05f, 0.02f, 0.08f);
    glPushMatrix();
    glTranslatef(-0.38f, 1.4f, 0);
    glRotatef(armSw + 15, 1, 0, 0);
    glTranslatef(0, -0.3f, 0);
    glScalef(0.12f, 0.65f, 0.12f);
    glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef( 0.38f, 1.4f, 0);
    glRotatef(-armSw + 15, 1, 0, 0);
    glTranslatef(0, -0.3f, 0);
    glScalef(0.12f, 0.65f, 0.12f);
    glutSolidCube(1);
    glPopMatrix();

    // ── CAKAR (ujung tangan) ──
    glColor3f(0.35f, 0.05f, 0.05f);
    float clawOffL[3] = {-0.05f, 0.0f, 0.05f};
    for (int c = 0; c < 3; c++) {
        glPushMatrix();
        glTranslatef(-0.38f + clawOffL[c], 0.75f + sinf(t*3+i*1.5f+armSw*0.01f)*0.3f, -0.1f);
        glScalef(0.03f, 0.12f, 0.03f);
        glutSolidCone(1, 1, 5, 1);
        glPopMatrix();
        glPushMatrix();
        glTranslatef( 0.38f + clawOffL[c], 0.75f - sinf(t*3+i*1.5f+armSw*0.01f)*0.3f, -0.1f);
        glScalef(0.03f, 0.12f, 0.03f);
        glutSolidCone(1, 1, 5, 1);
        glPopMatrix();
    }

    // ── KAKI ──
    float legSw = sinf(t * 3 + i * 1.5f + (float)M_PI) * 22.0f;
    if (lightsOn) glColor3f(0.06f, 0.03f, 0.10f);
    else glColor3f(0.03f, 0.01f, 0.06f);
    glPushMatrix();
    glTranslatef(-0.16f, 0.45f, 0);
    glRotatef(legSw, 1, 0, 0);
    glTranslatef(0, -0.32f, 0);
    glScalef(0.18f, 0.65f, 0.18f);
    glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef( 0.16f, 0.45f, 0);
    glRotatef(-legSw, 1, 0, 0);
    glTranslatef(0, -0.32f, 0);
    glScalef(0.18f, 0.65f, 0.18f);
    glutSolidCube(1);
    glPopMatrix();

    glPopMatrix();
}
void drawEnemies() { for(int i=0;i<8;i++) drawEnemyBody(i); }

void updateCamera() {
    float rad=playerAngle*(float)M_PI/180.0f;
    float sway=sinf(breathTimer*2.5f)*0.018f;
    gluLookAt(playerX,eyeHeight+sway,playerZ,
              playerX+sinf(rad)*lookDistance,eyeHeight+sway,playerZ+cosf(rad)*lookDistance,
              0,1,0);
}

void drawText2D(float x,float y,const char*t,float r,float g,float b) {
    glColor3f(r,g,b);glRasterPos2f(x,y);
    for(const char*c=t;*c;c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,*c);
}
void drawTextLarge(float x,float y,const char*t,float r,float g,float b) {
    glColor3f(r,g,b);glRasterPos2f(x,y);
    for(const char*c=t;*c;c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,*c);
}

void beginOrtho() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();gluOrtho2D(0,800,0,600);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
}
void endOrtho() {
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();
    glMatrixMode(GL_MODELVIEW);glEnable(GL_DEPTH_TEST);
}

void prepareStaticScreen(float r, float g, float b) {
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


void drawLevelClear() {
    prepareStaticScreen(0, 0.05f, 0.03f);
    beginOrtho();

    // Background
    glColor3f(0, 0.05f, 0.03f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(800,0); glVertex2f(800,600); glVertex2f(0,600);
    glEnd();

    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float p = 0.7f + 0.3f * sinf(t * 4.0f);

    // Judul
    if (currentLevel == 1) {
        drawTextLarge(250, 430, "LEVEL 1- EASY SELESAI!", 0.2f, p, 0.4f);
    } else if (currentLevel == 2) {
        drawTextLarge(220, 430, "LEVEL 2-MEDIUM SELESAI!", 0.2f, p, 0.4f);
    }

    // Garis pemisah
    glColor3f(0.1f, 0.4f, 0.2f);
    glBegin(GL_LINES);
    glVertex2f(200, 395); glVertex2f(600, 395);
    glEnd();

    // Teks pilihan
    drawText2D(330, 365, "Lanjutkan?", 0.7f, 0.9f, 0.7f);

    // ── TOMBOL: NEXT LEVEL ──
    float btnW = 220, btnH = 50;
    float btn1x = 140, btn1y = 280;

    glColor3f(0.10f, 0.40f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(btn1x, btn1y); glVertex2f(btn1x+btnW, btn1y);
    glVertex2f(btn1x+btnW, btn1y+btnH); glVertex2f(btn1x, btn1y+btnH);
    glEnd();
    glColor3f(0.3f, p, 0.4f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(btn1x, btn1y); glVertex2f(btn1x+btnW, btn1y);
    glVertex2f(btn1x+btnW, btn1y+btnH); glVertex2f(btn1x, btn1y+btnH);
    glEnd();
    glColor3f(0.8f, 1.0f, 0.8f);
    glRasterPos2f(btn1x + 18, btn1y + 20);
    const char* nextMsg = (currentLevel == 1) ? "N - Next: Level 2" : "N - Next: Level 3";
    for (const char* c = nextMsg; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    // ── TOMBOL: MAIN MENU ──
    float btn2x = 440, btn2y = 280;

    glColor3f(0.12f, 0.08f, 0.30f);
    glBegin(GL_QUADS);
    glVertex2f(btn2x, btn2y); glVertex2f(btn2x+btnW, btn2y);
    glVertex2f(btn2x+btnW, btn2y+btnH); glVertex2f(btn2x, btn2y+btnH);
    glEnd();
    glColor3f(0.4f, 0.3f, p);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(btn2x, btn2y); glVertex2f(btn2x+btnW, btn2y);
    glVertex2f(btn2x+btnW, btn2y+btnH); glVertex2f(btn2x, btn2y+btnH);
    glEnd();
    glColor3f(0.75f, 0.70f, 1.0f);
    glRasterPos2f(btn2x + 35, btn2y + 20);
    const char* menuMsg = "M - Main Menu";
    for (const char* c = menuMsg; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    // Info level berikutnya
    glColor3f(0.5f, 0.7f, 0.5f);
    glRasterPos2f(230, 245);
    const char* info = (currentLevel == 1)
        ? "Level 2: Timer 2 menit! Habis = semua musuh kejar!"
        : "Level 3: 3 Harta. Timer 2 menit! Habis = semua musuh kejar!";
    for (const char* c = info; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // Hint keyboard
    float ba = 0.5f + 0.5f * sinf(t * 3.0f);
    glColor3f(ba * 0.4f, ba * 0.8f, ba * 0.4f);
    glRasterPos2f(320, 200);
    const char* hint = "Press N or M";
    for (const char* c = hint; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    endOrtho();
}

void drawGPSArrow(float cx,float cy,float tx,float tz,float r,float g,float b) {
    float dx=tx-playerX,dz=tz-playerZ;
    float arrowAngle=atan2f(dx,dz)*180.0f/(float)M_PI-playerAngle;
    glPushMatrix();glTranslatef(cx,cy,0);glRotatef(arrowAngle,0,0,1);glColor3f(r,g,b);
    glBegin(GL_TRIANGLES);glVertex2f(0,22);glVertex2f(-9,-8);glVertex2f(9,-8);glEnd();
    glBegin(GL_QUADS);glVertex2f(-3,-8);glVertex2f(3,-8);glVertex2f(3,-22);glVertex2f(-3,-22);glEnd();
    glPopMatrix();
}
void drawJumpscare() {
    if (!jumpscareActive) return;

    float elapsed = (glutGet(GLUT_ELAPSED_TIME) - jumpscareStartTime) / 1000.0f;
    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 500.0f;

    float cx = 400, cy = 300;

    // ── ZOOM: loncat super cepat dalam 0.15 detik ──
    float zoomT = elapsed / 0.15f;
    if (zoomT > 1.0f) zoomT = 1.0f;
    // Overshoot: melebihi target lalu balik sedikit (efek "lompat")
    float bounce = zoomT < 0.7f
        ? zoomT / 0.7f                          // naik cepat
        : 1.0f + (1.0f - zoomT/1.0f) * 0.15f;  // sedikit overshoot
    float size = 20.0f + bounce * 310.0f;        // dari kecil banget ke SANGAT besar

    // ── SHAKE: kencang & konstan sepanjang durasi ──
    float shakeAmp = (elapsed < 0.15f) ? 0.0f : 18.0f; // shake mulai setelah zoom
    float shakeX = (rand() % (int)(shakeAmp * 2 + 1) - (int)shakeAmp);
    float shakeY = (rand() % (int)(shakeAmp * 2 + 1) - (int)shakeAmp);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ── FLASH PUTIH seketika saat muncul ──
    if (elapsed < 0.08f) {
        float flashAlpha = 1.0f - (elapsed / 0.08f);
        glColor4f(1.0f, 1.0f, 1.0f, flashAlpha);
        glBegin(GL_QUADS);
        glVertex2f(0,0); glVertex2f(800,0);
        glVertex2f(800,600); glVertex2f(0,600);
        glEnd();
    }

    // ── OVERLAY MERAH ──
    glColor4f(0.9f, 0.0f, 0.0f, jumpscareAlpha * 0.90f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(800,0);
    glVertex2f(800,600); glVertex2f(0,600);
    glEnd();

    // ── KEPALA — zoom in dari tengah ──
    glColor4f(0.05f, 0.02f, 0.08f, jumpscareAlpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx + shakeX, cy + shakeY);
    for (int s = 0; s <= 20; s++) {
        float a = s * 2 * (float)M_PI / 20;
        glVertex2f(cx + shakeX + cosf(a) * size,
                   cy + shakeY + sinf(a) * size * 1.2f);
    }
    glEnd();

    // ── MATA — lebih besar & lebih glow ──
    float eyeGlow = 0.85f + 0.15f * sinf(t * 15);
    glColor4f(1.0f, 0.0f, 0.0f, jumpscareAlpha * eyeGlow);
    for (int eye = -1; eye <= 1; eye += 2) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx + shakeX + eye * size * 0.32f,
                   cy + shakeY - size * 0.12f);
        for (int s = 0; s <= 16; s++) {
            float a = s * 2 * (float)M_PI / 16;
            float r = size * 0.22f; // mata lebih besar
            glVertex2f(cx + shakeX + eye * size * 0.32f + cosf(a) * r,
                       cy + shakeY - size * 0.12f + sinf(a) * r * 0.85f);
        }
        glEnd();
    }

    // ── GLOW MERAH DI MATA (halo luar) ──
    glColor4f(1.0f, 0.0f, 0.0f, jumpscareAlpha * 0.35f);
    for (int eye = -1; eye <= 1; eye += 2) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx + shakeX + eye * size * 0.32f,
                   cy + shakeY - size * 0.12f);
        for (int s = 0; s <= 16; s++) {
            float a = s * 2 * (float)M_PI / 16;
            float r = size * 0.38f;
            glVertex2f(cx + shakeX + eye * size * 0.32f + cosf(a) * r,
                       cy + shakeY - size * 0.12f + sinf(a) * r);
        }
        glEnd();
    }

    // ── MULUT LEBAR ──
    glColor4f(0.0f, 0.0f, 0.0f, jumpscareAlpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx + shakeX, cy + shakeY + size * 0.28f);
    for (int s = 0; s <= 14; s++) {
        float a = s * (float)M_PI / 14;
        glVertex2f(cx + shakeX + cosf(a) * size * 0.45f,
                   cy + shakeY + size * 0.28f + sinf(a) * size * 0.30f);
    }
    glEnd();

    // ── TEKS "FOUND YOU" — muncul langsung sejak 0.15s ──
    if (elapsed > 0.15f) {
        // Shadow teks
        glColor4f(0.0f, 0.0f, 0.0f, jumpscareAlpha);
        glRasterPos2f(288 + shakeX, 157 + shakeY);
        const char* msg = "FOUND YOU";
        for (const char* c = msg; *c; c++)
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
        // Teks utama putih
        glColor4f(1.0f, 1.0f, 1.0f, jumpscareAlpha);
        glRasterPos2f(285 + shakeX, 160 + shakeY);
        for (const char* c = msg; *c; c++)
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    }

    glDisable(GL_BLEND);
}
void drawGPS() {
    beginOrtho();
    float bx1=20,by1=490,bs=80;
    // GANTI blok GPS treasure (bx1) dengan:
glColor3f(0.08f,0.08f,0.12f);
glBegin(GL_QUADS);glVertex2f(bx1,by1);glVertex2f(bx1+bs,by1);glVertex2f(bx1+bs,by1+bs);glVertex2f(bx1,by1+bs);glEnd();
glColor3f(0.5f,0.5f,0.5f);glLineWidth(2);
glBegin(GL_LINE_LOOP);glVertex2f(bx1,by1);glVertex2f(bx1+bs,by1);glVertex2f(bx1+bs,by1+bs);glVertex2f(bx1,by1+bs);glEnd();

// ── HUD LEVEL 2: COUNTDOWN TIMER ──
LevelConfig &hcfg = levelConfigs[currentLevel - 1];
if (hcfg.hasCountdown) {
    char cbuf[64];
    if (!chaseMode) {
        // Countdown belum habis — tampilkan timer normal
        float remaining = level2Countdown - level2CountdownTimer;
        sprintf(cbuf, "COUNTDOWN: %.0fs", remaining);
        float danger = 1.0f - (remaining / level2Countdown);
        drawTextLarge(270, 540, cbuf, 0.5f + danger*0.5f, (1.0f-danger)*0.6f, 0);
        if (remaining <= 10.0f) {
            float blink = (sinf((float)glutGet(GLUT_ELAPSED_TIME)/150.0f) > 0) ? 1.0f : 0.0f;
            if (blink > 0.5f) drawTextLarge(185, 480, "!! MUSUH AKAN MENYERANG SEGERA !!", 1, 0.1f, 0);
        }
    } else {
        // ✅ Chase mode PERMANEN (countdown habis) — baru tampilkan ini
        drawTextLarge(230, 540, "!! CHASE MODE - LARIII !!", 1.0f, 0.05f, 0.05f);
    }
}
// ── INDIKATOR LEVEL ──
char lvlbuf[16];
sprintf(lvlbuf, "LEVEL %d", currentLevel);
drawText2D(710, 570, lvlbuf, 0.6f, 0.6f, 0.9f);
if (!allTreasuresCollected()) {
    // Tunjuk treasure terdekat yang belum diambil
    float nearDist = 1e9f; int nearIdx = -1;
    for (int i = 0; i < treasureCount; i++) {
        if (treasureCollected[i]) continue;
        float dx = playerX - treasureX[i], dz = playerZ - treasureZ[i];
        float d = sqrtf(dx*dx + dz*dz);
        if (d < nearDist) { nearDist = d; nearIdx = i; }
    }
    if (nearIdx >= 0)
        drawGPSArrow(bx1+bs/2, by1+bs/2, treasureX[nearIdx], treasureZ[nearIdx], 1, 0.85f, 0);
} else {
    glColor3f(0,1,0.3f);glLineWidth(3);
    glBegin(GL_LINE_STRIP);
    glVertex2f(bx1+20,by1+35);glVertex2f(bx1+35,by1+20);glVertex2f(bx1+60,by1+55);
    glEnd();
}

char tlabel[32];
sprintf(tlabel, "TRSR %d/%d", treasuresHeld(), treasureCount);
drawText2D(bx1+2, by1-18, allTreasuresCollected() ? "ALL GOT!" : tlabel, 1, 0.85f, 0);
    float bx2=110,by2=490;
    glColor3f(0.08f,0.08f,0.12f);
    glBegin(GL_QUADS);glVertex2f(bx2,by2);glVertex2f(bx2+bs,by2);glVertex2f(bx2+bs,by2+bs);glVertex2f(bx2,by2+bs);glEnd();
    glColor3f(0.5f,0.5f,0.5f);glLineWidth(2);
    glBegin(GL_LINE_LOOP);glVertex2f(bx2,by2);glVertex2f(bx2+bs,by2);glVertex2f(bx2+bs,by2+bs);glVertex2f(bx2,by2+bs);glEnd();
    drawGPSArrow(bx2+bs/2,by2+bs/2,exitX,exitZ,0,1,0.3f);
    drawText2D(bx2+18,by2-18,"EXIT",0,1,0.3f);
    char buf[64];
    float ttl=lightsOn?(lightOnDuration-lightsOnTimer):(lightCycleInterval-gameTimer);
    if(lightsOn){sprintf(buf,"LIGHTS OFF IN: %.1fs",ttl);drawText2D(300,570,buf,1,0.3f,0.3f);}
    else{sprintf(buf,"LIGHTS ON IN: %.0fs",ttl);drawText2D(300,570,buf,0.7f,0.7f,0.8f);}
    float elapsed=(float)glutGet(GLUT_ELAPSED_TIME)/1000.0f-startTime;
    sprintf(buf,"TIME: %.0fs",elapsed);drawText2D(500,570,buf,0.7f,0.7f,0.7f);
    if(playerIsHiding)drawText2D(330,510,"HIDING",0.3f,1,0.3f);
    else if(isNearHidingSpot())drawText2D(330,510,"Press H to Hide",0.9f,0.9f,0.3f);
    if(lightsOn){
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1,0.85f,0,0.06f);glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(800,0);glVertex2f(800,600);glVertex2f(0,600);glEnd();
        glDisable(GL_BLEND);
        if(!playerIsHiding)drawTextLarge(185,470,"!! LIGHTS ON - HIDE !!",1,0.1f,0.1f);
        else drawTextLarge(270,470,"STAY HIDDEN!",0.2f,1,0.2f);
    } else {
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        float vign=0.18f+0.04f*sinf(breathTimer*0.5f);glColor4f(0.15f,0,0,vign);
        glBegin(GL_QUADS);glVertex2f(0,550);glVertex2f(800,550);glVertex2f(800,600);glVertex2f(0,600);glEnd();
        glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(800,0);glVertex2f(800,50);glVertex2f(0,50);glEnd();
        glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(60,0);glVertex2f(60,600);glVertex2f(0,600);glEnd();
        glBegin(GL_QUADS);glVertex2f(740,0);glVertex2f(800,0);glVertex2f(800,600);glVertex2f(740,600);glEnd();
        glDisable(GL_BLEND);
    }
    drawText2D(20,20,"WASD/Arrows:Move  H:Hide  Mouse:Turn",0.4f,0.4f,0.45f);
    drawJumpscare();
    endOrtho();
}
// ===================================================
// HELPER: Gambar dinding maze mini untuk menu
// ===================================================
void drawMiniMazeWall(float x, float y, float w, float h) {
    // Dinding batu gelap dengan lis terang
    glColor3f(0.12f, 0.07f, 0.20f);
    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x+w, y);
    glVertex2f(x+w, y+h); glVertex2f(x, y+h);
    glEnd();
    glColor3f(0.22f, 0.14f, 0.32f);
    glLineWidth(0.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x+w, y);
    glVertex2f(x+w, y+h); glVertex2f(x, y+h);
    glEnd();
}
void drawMenuMaze() {
    // ── OUTER BOUNDARY ──
    drawMiniMazeWall(390, 160, 370, 5);   // north  (z=20)
    drawMiniMazeWall(390, 443, 370, 5);   // south  (z=-20)
    drawMiniMazeWall(390, 160, 5, 288);   // west   (x=-20)
    drawMiniMazeWall(755, 160, 5, 130);   // east upper (z=20 s/d -10.5)
    drawMiniMazeWall(755, 353, 5,  90);   // east lower (z=-13.5 s/d -20)

    // ── HORIZONTAL WALLS ── (z=16: screenY = 448 - 36*7.2 = 189)
    // z=16 → screenY=188
    drawMiniMazeWall(390, 188, 37, 5);    // wall 5:  x=-20 s/d -16
    drawMiniMazeWall(482, 188, 111, 5);   // wall 6:  x=-10 s/d 2   | gap -16 s/d -10
    drawMiniMazeWall(574, 188, 92,  5);   // wall 7:  x=3 s/d 13    | gap 2 s/d 8

    // z=10 → screenY=230
    drawMiniMazeWall(390, 230, 55, 5);    // wall 8:  x=-20 s/d -14 (diperbaiki)
    drawMiniMazeWall(445, 230, 111, 5);   // wall 9:  x=-14 s/d -2  (diperbaiki)
    drawMiniMazeWall(575, 230, 74, 5);    // wall 10: x=4 s/d 12

    // z=4 → screenY=274
    drawMiniMazeWall(390, 274, 92, 5);    // wall 11: x=-20 s/d -10 (diperbaiki)
    drawMiniMazeWall(528, 274, 92, 5);    // wall 12: x=-5 s/d 5
    drawMiniMazeWall(666, 274, 55, 5);    // wall 13: x=10 s/d 16   (diperbaiki)

    // z=-2 → screenY=317
    drawMiniMazeWall(390, 317, 55, 5);    // wall 14: x=-20 s/d -14
    drawMiniMazeWall(482, 317, 92, 5);    // wall 15: x=-11 s/d -1
    drawMiniMazeWall(610, 317, 74, 5);    // wall 16: x=4 s/d 12

    // z=-8 → screenY=360
    drawMiniMazeWall(408, 360, 74, 5);    // wall 17: x=-18 s/d -10
    drawMiniMazeWall(556, 360, 74, 5);    // wall 18: x=-2 s/d 6

    // z=-14 → screenY=404
    drawMiniMazeWall(390, 404, 55, 5);    // wall 19: x=-20 s/d -14
    drawMiniMazeWall(482, 404, 92, 5);    // wall 20: x=-9 s/d 1    (diperbaiki)

    // ── VERTICAL WALLS ──
    // x=-16 → screenX=427
    drawMiniMazeWall(427, 160, 5, 29);    // wall 21: z=16 s/d 20
    drawMiniMazeWall(427, 231, 5, 43);    // wall 22: z=4 s/d 10
    drawMiniMazeWall(427, 318, 5, 43);    // wall 23: z=-8 s/d -2
    drawMiniMazeWall(427, 361, 5, 43);    // wall 24: z=-20 s/d -14

    // x=-4 → screenX=538
    drawMiniMazeWall(538, 160, 5, 29);    // wall 25: z=16 s/d 20
    drawMiniMazeWall(538, 231, 5, 43);    // wall 26: z=4 s/d 10
    drawMiniMazeWall(538, 318, 5, 43);    // wall 27: z=-8 s/d -2
    drawMiniMazeWall(538, 361, 5, 43);    // wall 28: z=-20 s/d -14

    // x=8 → screenX=649
    drawMiniMazeWall(649, 160, 5, 29);    // wall 29: z=16 s/d 20
    drawMiniMazeWall(649, 231, 5, 43);    // wall 30: z=4 s/d 10
    drawMiniMazeWall(649, 318, 5, 43);    // wall 31: z=-8 s/d -2

    // x=16 → screenX=723
    drawMiniMazeWall(723, 160, 5, 29);    // wall 32: z=16 s/d 20
    drawMiniMazeWall(723, 231, 5, 43);    // wall 33: z=4 s/d 10    (diperbaiki)
    drawMiniMazeWall(723, 318, 5, 43);    // wall 34: z=-8 s/d -2   (diperbaiki)
    drawMiniMazeWall(723, 361, 5, 43);    // wall 35: z=-20 s/d -14 (baru)
}
void drawMenu() {
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.03f, 0.02f, 0.06f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    beginOrtho();

    glColor3f(0.03f, 0.015f, 0.055f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(800,0); glVertex2f(800,600); glVertex2f(0,600);
    glEnd();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.10f, 0.05f, 0.18f, 0.60f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0); glVertex2f(800, 0); glVertex2f(800, 120); glVertex2f(0, 120);
    glEnd();
    glDisable(GL_BLEND);

    drawMenuMaze();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 800.0f;
    float pulse = 0.4f + 0.3f * sinf(t * 2.5f);
    glColor4f(0.0f, pulse, pulse * 0.4f, 0.55f);
    glBegin(GL_QUADS);
    glVertex2f(752, 295); glVertex2f(770, 295);
    glVertex2f(770, 340); glVertex2f(752, 340);
    glEnd();
    float tpulse = 0.3f + 0.25f * sinf(t * 3.0f);
    glColor4f(tpulse * 1.5f, tpulse, 0.0f, 0.60f);
    glBegin(GL_QUADS);
    glVertex2f(530, 280); glVertex2f(555, 280);
    glVertex2f(555, 300); glVertex2f(530, 300);
    glEnd();
    float eye = 0.5f + 0.5f * sinf(t * 5.0f);
    glColor4f(eye, 0.0f, 0.0f, 0.85f);
    for (float ex = 477.0f; ex <= 487.0f; ex += 8.0f) {
        glBegin(GL_QUADS);
        glVertex2f(ex, 237); glVertex2f(ex+4, 237);
        glVertex2f(ex+4, 241); glVertex2f(ex, 241);
        glEnd();
    }
    glColor4f(eye * 0.8f, 0.0f, 0.0f, 0.7f);
    for (float ex = 620.0f; ex <= 630.0f; ex += 8.0f) {
        glBegin(GL_QUADS);
        glVertex2f(ex, 210); glVertex2f(ex+3, 210);
        glVertex2f(ex+3, 213); glVertex2f(ex, 213);
        glEnd();
    }
    glDisable(GL_BLEND);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.03f, 0.01f, 0.06f, 0.82f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(370,0); glVertex2f(370,600); glVertex2f(0,600);
    glEnd();
    glDisable(GL_BLEND);

    // Pohon & nisan
    glColor3f(0.08f, 0.04f, 0.12f);
    glLineWidth(4.0f);
    glBegin(GL_LINES);
    glVertex2f(20, 150); glVertex2f(40, 420);
    glEnd();
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(40, 320); glVertex2f(15, 275);
    glVertex2f(40, 320); glVertex2f(65, 270);
    glVertex2f(40, 360); glVertex2f(10, 340);
    glEnd();

    // ── JUDUL ──
    glColor3f(0.45f, 0.22f, 0.65f);
    glRasterPos2f(38, 570);
    const char* sub = "MAZE";
    for (const char* c = sub; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    glColor3f(0.78f, 0.15f, 0.18f);
    glRasterPos2f(36, 545);
    const char* title = "ESCAPE";
    for (const char* c = title; *c; c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    glRasterPos2f(37, 545);
    for (const char* c = title; *c; c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);

    // ── PILIH LEVEL ──
    glColor3f(0.55f, 0.35f, 0.70f);
    glRasterPos2f(36, 518);
    const char* lsLabel = "--- PILIH LEVEL ---";
    for (const char* c = lsLabel; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    // ── KOTAK LEVEL 1 ──
    float boxW = 130, boxH = 40;
    float box1x = 36, box1y = 468;
    if (selectedLevel == 1) glColor3f(0.25f, 0.55f, 0.25f);
    else                    glColor3f(0.10f, 0.20f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(box1x, box1y); glVertex2f(box1x+boxW, box1y);
    glVertex2f(box1x+boxW, box1y+boxH); glVertex2f(box1x, box1y+boxH);
    glEnd();
    glColor3f(0.4f, 0.8f, 0.4f); glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(box1x, box1y); glVertex2f(box1x+boxW, box1y);
    glVertex2f(box1x+boxW, box1y+boxH); glVertex2f(box1x, box1y+boxH);
    glEnd();
    if (selectedLevel == 1) glColor3f(0.9f, 1.0f, 0.9f);
    else                    glColor3f(0.5f, 0.7f, 0.5f);
    glRasterPos2f(box1x + 30, box1y + 20);
    const char* lv1 = "LEVEL 1";
    for (const char* c = lv1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    glColor3f(0.4f, 0.6f, 0.4f);
    glRasterPos2f(box1x + 20, box1y + 8);
    const char* lv1desc = "Easy - 2 harta";
    for (const char* c = lv1desc; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // ── KOTAK LEVEL 2 ──
    float box2x = 180, box2y = 468;
    if (selectedLevel == 2) glColor3f(0.55f, 0.18f, 0.12f);
    else                    glColor3f(0.20f, 0.08f, 0.06f);
    glBegin(GL_QUADS);
    glVertex2f(box2x, box2y); glVertex2f(box2x+boxW, box2y);
    glVertex2f(box2x+boxW, box2y+boxH); glVertex2f(box2x, box2y+boxH);
    glEnd();
    glColor3f(0.9f, 0.3f, 0.3f); glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(box2x, box2y); glVertex2f(box2x+boxW, box2y);
    glVertex2f(box2x+boxW, box2y+boxH); glVertex2f(box2x, box2y+boxH);
    glEnd();
    if (selectedLevel == 2) glColor3f(1.0f, 0.85f, 0.85f);
    else                    glColor3f(0.7f, 0.4f, 0.4f);
    glRasterPos2f(box2x + 30, box2y + 20);
    const char* lv2 = "LEVEL 2";
    for (const char* c = lv2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    glColor3f(0.6f, 0.35f, 0.35f);
    glRasterPos2f(box2x + 4, box2y + 8);
    const char* lv2desc = "Medium - timer 2 mnt";
    for (const char* c = lv2desc; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // ── KOTAK LEVEL 3 (SULIT) ──
    float box3x = 36, box3y = 418;
    if (selectedLevel == 3) glColor3f(0.50f, 0.10f, 0.10f);
    else                    glColor3f(0.18f, 0.05f, 0.05f);
    glBegin(GL_QUADS);
    glVertex2f(box3x, box3y); glVertex2f(box3x+boxW*2+14, box3y);
    glVertex2f(box3x+boxW*2+14, box3y+boxH); glVertex2f(box3x, box3y+boxH);
    glEnd();
    glColor3f(1.0f, 0.2f, 0.2f); glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(box3x, box3y); glVertex2f(box3x+boxW*2+14, box3y);
    glVertex2f(box3x+boxW*2+14, box3y+boxH); glVertex2f(box3x, box3y+boxH);
    glEnd();
    if (selectedLevel == 3) glColor3f(1.0f, 0.7f, 0.7f);
    else                    glColor3f(0.6f, 0.3f, 0.3f);
    glRasterPos2f(box3x + 100, box3y + 20);  // teks judul di tengah atas
    const char* lv3 = "LEVEL 3";
    for (const char* c = lv3; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    glColor3f(0.55f, 0.25f, 0.25f);
    glRasterPos2f(box3x + 95, box3y + 8);  // teks deskripsi di bawahnya
    const char* lv3desc = "Hard - 3 harta";
    for (const char* c = lv3desc; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // ── GARIS + DESKRIPSI LEVEL YANG DIPILIH ──
    glColor3f(0.20f, 0.08f, 0.28f);
    glLineWidth(0.5f);
    glBegin(GL_LINES);
    glVertex2f(36, 412); glVertex2f(340, 412);
    glEnd();

    if (selectedLevel == 1) {
        glColor3f(0.45f, 0.75f, 0.45f);
        glRasterPos2f(36, 398);
        const char* d1 = "Kumpulkan 2 harta lalu cari pintu keluar.";
        for (const char* c = d1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        glColor3f(0.35f, 0.55f, 0.35f);
        glRasterPos2f(36, 384);
        const char* d2 = "Lampu menyala tiap 40 detik (selama 10 detik).";
        for (const char* c = d2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    } else if (selectedLevel == 2) {
        glColor3f(0.85f, 0.45f, 0.35f);
        glRasterPos2f(36, 398);
        const char* d1 = "Timer 2 menit, habis = semua musuh mengejar!";
        for (const char* c = d1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        glColor3f(0.65f, 0.35f, 0.25f);
        glRasterPos2f(36, 384);
        const char* d2 = "Kumpulkan 2 harta sebelum waktu habis.";
        for (const char* c = d2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    } else {
        glColor3f(1.0f, 0.35f, 0.35f);
        glRasterPos2f(36, 398);
        const char* d1 = "Timer 2 menit, habis = semua musuh mengejar!";
        for (const char* c = d1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        glColor3f(0.75f, 0.25f, 0.25f);
        glRasterPos2f(36, 384);
        const char* d2 = "Kumpulkan 3 harta. Mode paling sulit!";
        for (const char* c = d2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // ── SUBTITLE ──
    glColor3f(0.55f, 0.35f, 0.70f);
    glRasterPos2f(36, 368);
    const char* findTreasure = "CARI HARTA & KELUAR!";
    for (const char* c = findTreasure; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // ── GARIS PEMISAH ──
    glColor3f(0.20f, 0.08f, 0.28f);
    glLineWidth(0.5f);
    glBegin(GL_LINES);
    glVertex2f(36, 360); glVertex2f(340, 360);
    glEnd();

    // ── CARA BERMAIN ──
    glColor3f(0.55f, 0.35f, 0.70f);
    glRasterPos2f(36, 346);
    const char* howLabel = "--- CARA BERMAIN ---";
    for (const char* c = howLabel; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(0.85f, 0.85f, 0.85f);
    glRasterPos2f(36, 331);
    const char* h1a = "WASD / Panah";
    for (const char* c = h1a; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    glColor3f(0.55f, 0.55f, 0.60f);
    glRasterPos2f(175, 331);
    const char* h1b = ": Bergerak";
    for (const char* c = h1b; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(0.85f, 0.85f, 0.85f);
    glRasterPos2f(36, 316);
    const char* h2a = "Mouse";
    for (const char* c = h2a; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    glColor3f(0.55f, 0.55f, 0.60f);
    glRasterPos2f(175, 316);
    const char* h2b = ": Arahkan pandangan";
    for (const char* c = h2b; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(0.85f, 0.85f, 0.85f);
    glRasterPos2f(36, 301);
    const char* h3a = "H";
    for (const char* c = h3a; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    glColor3f(0.55f, 0.55f, 0.60f);
    glRasterPos2f(175, 301);
    const char* h3b = ": Bersembunyi (dalam kotak kuning)";
    for (const char* c = h3b; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(1.0f, 0.85f, 0.0f);
    glRasterPos2f(36, 286);
    const char* h4a = "Kotak GPS kuning";
    for (const char* c = h4a; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    glColor3f(0.55f, 0.55f, 0.60f);
    glRasterPos2f(175, 286);
    const char* h4b = ": Menunjuk ke harta";
    for (const char* c = h4b; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(0.2f, 1.0f, 0.4f);
    glRasterPos2f(36, 271);
    const char* h5a = "Kotak GPS hijau";
    for (const char* c = h5a; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    glColor3f(0.55f, 0.55f, 0.60f);
    glRasterPos2f(175, 271);
    const char* h5b = ": Menunjuk ke pintu keluar";
    for (const char* c = h5b; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // ── GARIS PEMISAH ──
    glColor3f(0.20f, 0.08f, 0.28f);
    glBegin(GL_LINES);
    glVertex2f(36, 263); glVertex2f(340, 263);
    glEnd();

    // ── PERATURAN ──
    glColor3f(0.55f, 0.35f, 0.70f);
    glRasterPos2f(36, 249);
    const char* rulesLabel = "--- PERATURAN ---";
    for (const char* c = rulesLabel; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(1.0f, 0.35f, 0.15f);
    glRasterPos2f(36, 234);
    const char* r1 = "Tiap 40 detik lampu menyala selama 10 detik!";
    for (const char* c = r1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(0.75f, 0.75f, 0.80f);
    glRasterPos2f(36, 219);
    const char* r2 = "Lampu menyala = musuh mengejar kamu!";
    for (const char* c = r2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glRasterPos2f(36, 204);
    const char* r3 = "Bersembunyi di peti agar tetap aman.";
    for (const char* c = r3; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glRasterPos2f(36, 189);
    const char* r4 = "Hindari musuh";
    for (const char* c = r4; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(1.0f, 0.85f, 0.0f);
    glRasterPos2f(36, 174);
    const char* r5 = "Keluar membawa seluruh treasure = skor terbaik!";
    for (const char* c = r5; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // ── GARIS PEMISAH ──
    glColor3f(0.20f, 0.08f, 0.28f);
    glLineWidth(0.5f);
    glBegin(GL_LINES);
    glVertex2f(36, 162); glVertex2f(340, 162);
    glEnd();

    // ── TOMBOL MULAI ──
    float ba = 0.5f + 0.5f * sinf(t * 2.5f);
    glColor3f(0.1f * ba, ba * 0.85f, 0.2f * ba);
    glRasterPos2f(36, 142);
    const char* startMsg = "> Tekan ENTER atau SPASI untuk Mulai";
    for (const char* c = startMsg; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    endOrtho();
}
void drawGameOver() {
    prepareStaticScreen(0.04f,0,0);
    beginOrtho();
    glColor3f(0.04f,0,0);
    glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(800,0);glVertex2f(800,600);glVertex2f(0,600);glEnd();
    float t=(float)glutGet(GLUT_ELAPSED_TIME)/1000.0f;
    float p=0.7f+0.3f*sinf(t*3.5f);
    drawTextLarge(320,420,"GAME OVER",p,0.05f,0.05f);

    // Tunjukkan di level berapa kalahnya
    char lvlbuf[32];
    sprintf(lvlbuf, "Died on Level %d", currentLevel);
    drawText2D(325, 385, lvlbuf, 0.6f, 0.3f, 0.3f);

    char buf[64];
    if(wonWithTreasure){
        drawText2D(310,340,"Kamu berhasil mendapat treasure tapi gagal keluar...",1,0.75f,0);
        sprintf(buf,"Score: %d",finalScore);
        drawText2D(330,300,buf,1,0.75f,0);
    } else {
        drawText2D(316,340,"Kamu tertangkap...",0.8f,0.4f,0.4f);
        drawText2D(350,300,"Score: 0",0.6f,0.6f,0.6f);
    }

    // Tombol retry & menu
    drawTextLarge(218,220,"R - Retry Level   M - Main Menu",0.7f,0.7f,0.7f);

    // Info retry
    glColor3f(0.5f, 0.5f, 0.5f);
    sprintf(buf, "(Retry akan mengulang level saat ini. (level %d))", currentLevel);
    glRasterPos2f(260, 185);
    for (const char* c = buf; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    endOrtho();
}

void drawWin() {
    prepareStaticScreen(0,0.04f,0.02f);
    beginOrtho();
    glColor3f(0,0.04f,0.02f);glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(800,0);glVertex2f(800,600);glVertex2f(0,600);glEnd();
    if(wonWithTreasure){
        drawTextLarge(50,420,"KAMU BERHASIL MEMBAWA SEMUA TREASURE!!",0.3f,1,0.45f);
        char buf[64];sprintf(buf,"FINAL SCORE: %d",finalScore);
        drawTextLarge(255,355,buf,0.3f,1,0.3f);
    } else {
        char buf[64];
        sprintf(buf,"Kamu keluar membawa %d/%d treasure!", treasuresHeld(), treasureCount);
        drawText2D(155,420,buf,0.55f,0.8f,0.55f);
        sprintf(buf,"SCORE: %d", finalScore);
        drawTextLarge(275,355,buf,0.45f,0.65f,0.45f);
        drawText2D(165,310,"Dapatkan semua treasure untuk skor terbaik!!",0.7f,0.7f,0.7f);
    }
    drawTextLarge(220,240,"R - Retry   M - Menu",0.7f,0.7f,0.7f);
    endOrtho();
}

void display() {
    glLoadIdentity();
    if (gameState == STATE_MENU)        { drawMenu();       glutSwapBuffers(); return; }
    if (gameState == STATE_GAMEOVER)    { drawGameOver();   glutSwapBuffers(); return; }
    if (gameState == STATE_WIN)         { drawWin();        glutSwapBuffers(); return; }
    if (gameState == STATE_LEVEL_CLEAR) { drawLevelClear(); glutSwapBuffers(); return; }

    // ── LEVEL CLEAR OVERLAY ──
    if (levelClearing) { drawLevelClear(); glutSwapBuffers(); return; }

    if(lightsOn)glClearColor(0.55f,0.52f,0.45f,1);
    else{float f=0.03f*flickerIntensity;glClearColor(f,f,f*1.4f,1);}
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();updateCamera();

    GLfloat fc[4];
    if(!lightsOn){fc[0]=0.02f;fc[1]=0.02f;fc[2]=0.05f;fc[3]=1;}
    else         {fc[0]=0.55f;fc[1]=0.52f;fc[2]=0.45f;fc[3]=1;}
    glEnable(GL_FOG);glFogi(GL_FOG_MODE,GL_LINEAR);glFogfv(GL_FOG_COLOR,fc);
    glFogf(GL_FOG_START,lightsOn?14.0f:3.5f);glFogf(GL_FOG_END,lightsOn?32.0f:14.0f);

    drawFloor();drawMaze();drawHidingSpots();drawExit();drawTreasure();drawEnemies();
    glDisable(GL_FOG);drawGPS();
    glutSwapBuffers();
}

void reshape(int w,int h) {
    if(!h)h=1;
    windowWidth = w;
    windowHeight = h;
    lastMouseX = w / 2;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();gluPerspective(65.0,(double)w/h,0.3,200.0);
    glMatrixMode(GL_MODELVIEW);
}

void initRendering() {
    glEnable(GL_DEPTH_TEST); 
    glClearColor(0.03f,0.03f,0.06f,1);
    glShadeModel(GL_SMOOTH);

    Image* wallImg = loadBMP("wall.bmp"); 
    wallTextureId = loadTexture(wallImg); 
    delete wallImg; 
}

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
