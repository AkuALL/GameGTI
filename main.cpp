#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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
enum GameState { STATE_MENU, STATE_PLAYING, STATE_GAMEOVER, STATE_WIN };
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
int treasureCount = 3;
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
void resetGame() {
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
            if (sqrtf(dx*dx + dz*dz) < 1.0f)
                treasureCollected[i] = true;
        }
    }
}

void checkExit() {
    if (playerX > 19.5f && playerZ > -13.5f && playerZ < -10.5f) {
        wonWithTreasure = allTreasuresCollected();
        float elapsed = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f - startTime;
        int held = treasuresHeld();
        finalScore = held > 0 ? (int)(10000.0f / (elapsed + 1.0f)) * 10 * held : 0;
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

        if (e.pathIndex >= (int)e.pathX.size()) {
            if (!chooseEnemyPatrolTarget(e)) continue;
        }

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

        float moveSpeed = e.speed * (lightsOn ? 2.0f : 1.0f) * dt * 60.0f;

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
            if (e.stuckFrames > 12) {
                if (!buildEnemyPath(e, e.targetX, e.targetZ)) {
                    chooseEnemyPatrolTarget(e);
                }
                e.stuckFrames = 0;
            }
        }

        float pdx = e.x - playerX, pdz = e.z - playerZ;
        if (sqrtf(pdx*pdx + pdz*pdz) < 0.85f && !playerIsHiding) {
            finalScore = 0; wonWithTreasure = false;
            gameState = STATE_GAMEOVER;
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
    if (!lightsOn) {
        gameTimer += dt;
        if (gameTimer >= lightCycleInterval) {
            lightsOn = true;
            lightsOnTimer = 0.0f;
            gameTimer = 0.0f;
        }
    } else {
        lightsOnTimer += dt;
        if (lightsOnTimer >= lightOnDuration) {
            lightsOn = false;
            lightsOnTimer = 0.0f;
            gameTimer = 0.0f;
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
        updatePlayerMovement(dt);
        updateEnemies(dt);
        if (gameState != STATE_PLAYING) {
            glutPostRedisplay();
            glutTimerFunc(16, update, 0);
            return;
        }
        updateDoor(dt); 
        checkTreasurePickup();
        checkExit();
    }
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
    if (gameState == STATE_MENU) {
        if (key == 13 || key == ' ') resetGame();
        return;
    }
    if (gameState == STATE_GAMEOVER || gameState == STATE_WIN) {
        if (key == 'r' || key == 'R') resetGame();
        if (key == 'm' || key == 'M') gameState = STATE_MENU;
        return;
    }
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

void drawGPSArrow(float cx,float cy,float tx,float tz,float r,float g,float b) {
    float dx=tx-playerX,dz=tz-playerZ;
    float arrowAngle=atan2f(dx,dz)*180.0f/(float)M_PI-playerAngle;
    glPushMatrix();glTranslatef(cx,cy,0);glRotatef(arrowAngle,0,0,1);glColor3f(r,g,b);
    glBegin(GL_TRIANGLES);glVertex2f(0,22);glVertex2f(-9,-8);glVertex2f(9,-8);glEnd();
    glBegin(GL_QUADS);glVertex2f(-3,-8);glVertex2f(3,-8);glVertex2f(3,-22);glVertex2f(-3,-22);glEnd();
    glPopMatrix();
}

void drawGPS() {
    beginOrtho();
    float bx1=20,by1=490,bs=80;
    // GANTI blok GPS treasure (bx1) dengan:
glColor3f(0.08f,0.08f,0.12f);
glBegin(GL_QUADS);glVertex2f(bx1,by1);glVertex2f(bx1+bs,by1);glVertex2f(bx1+bs,by1+bs);glVertex2f(bx1,by1+bs);glEnd();
glColor3f(0.5f,0.5f,0.5f);glLineWidth(2);
glBegin(GL_LINE_LOOP);glVertex2f(bx1,by1);glVertex2f(bx1+bs,by1);glVertex2f(bx1+bs,by1+bs);glVertex2f(bx1,by1+bs);glEnd();

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
    glBegin(GL_QUADS);glVertex2f(bx2,by2);glVertex2f(bx2+bs,by2);glVertex2f(bx2+bs,by2+bs);glVertex2f(bx2+bs,by2+bs);glEnd();
    glColor3f(0.5f,0.5f,0.5f);glLineWidth(2);
    glBegin(GL_LINE_LOOP);glVertex2f(bx2,by2);glVertex2f(bx2+bs,by2);glVertex2f(bx2+bs,by2+bs);glVertex2f(bx2,by2+bs);glEnd();
    drawGPSArrow(bx2+bs/2,by2+bs/2,exitX,exitZ,0,1,0.3f);
    drawText2D(bx2+18,by2-18,"EXIT",0,1,0.3f);
    char buf[64];
    float ttl=lightsOn?(lightOnDuration-lightsOnTimer):(lightCycleInterval-gameTimer);
    if(lightsOn){sprintf(buf,"LIGHTS OFF IN: %.1fs",ttl);drawText2D(300,570,buf,1,0.3f,0.3f);}
    else{sprintf(buf,"LIGHTS ON IN: %.0fs",ttl);drawText2D(300,570,buf,0.7f,0.7f,0.8f);}
    float elapsed=(float)glutGet(GLUT_ELAPSED_TIME)/1000.0f-startTime;
    sprintf(buf,"TIME: %.0fs",elapsed);drawText2D(650,570,buf,0.7f,0.7f,0.7f);
char tbuf[64];
sprintf(tbuf, "TREASURE: %d / %d", treasuresHeld(), treasureCount);
if (allTreasuresCollected())
    drawText2D(330, 540, tbuf, 1, 0.85f, 0);
else
    drawText2D(330, 540, tbuf, 0.5f, 0.5f, 0.55f);
    if(playerIsHiding)drawText2D(330,510,"HIDING",0.3f,1,0.3f);
    else if(isNearHidingSpot())drawText2D(330,510,"Press H to Hide",0.9f,0.9f,0.3f);
    if(lightsOn){
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1,0.85f,0,0.06f);glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(800,0);glVertex2f(800,600);glVertex2f(0,600);glEnd();
        glDisable(GL_BLEND);
        if(!playerIsHiding)drawTextLarge(185,470,"!! LIGHTS ON - ENEMIES FASTER !!",1,0.1f,0.1f);
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
    // Background utama: hitam keunguan
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.03f, 0.02f, 0.06f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    beginOrtho();

    // --- BACKGROUND GELAP ---
    glColor3f(0.03f, 0.015f, 0.055f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(800,0); glVertex2f(800,600); glVertex2f(0,600);
    glEnd();

    // --- KABUT / MIST DI BAWAH ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.10f, 0.05f, 0.18f, 0.60f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0); glVertex2f(800, 0); glVertex2f(800, 120); glVertex2f(0, 120);
    glEnd();
    glColor4f(0.12f, 0.06f, 0.20f, 0.40f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0); glVertex2f(800, 0); glVertex2f(800, 80); glVertex2f(0, 80);
    glEnd();
    glDisable(GL_BLEND);

    // --- MAZE MINI DI KANAN ---
    drawMenuMaze();

    // --- GLOW EXIT (hijau) di ujung kanan maze ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 800.0f;
    float pulse = 0.4f + 0.3f * sinf(t * 2.5f);
    glColor4f(0.0f, pulse, pulse * 0.4f, 0.55f);
    glBegin(GL_QUADS);
    glVertex2f(752, 295); glVertex2f(770, 295);
    glVertex2f(770, 340); glVertex2f(752, 340);
    glEnd();

    // --- GLOW TREASURE (kuning) di dalam maze ---
    float tpulse = 0.3f + 0.25f * sinf(t * 3.0f);
    glColor4f(tpulse * 1.5f, tpulse, 0.0f, 0.60f);
    glBegin(GL_QUADS);
    glVertex2f(530, 280); glVertex2f(555, 280);
    glVertex2f(555, 300); glVertex2f(530, 300);
    glEnd();

    // --- MATA MUSUH (merah menyala) ---
    float eye = 0.5f + 0.5f * sinf(t * 5.0f);
    glColor4f(eye, 0.0f, 0.0f, 0.85f);
    // Musuh 1
    for (float ex = 477.0f; ex <= 487.0f; ex += 8.0f) {
        glBegin(GL_QUADS);
        glVertex2f(ex, 237); glVertex2f(ex+4, 237);
        glVertex2f(ex+4, 241); glVertex2f(ex, 241);
        glEnd();
    }
    // Musuh 2
    glColor4f(eye * 0.8f, 0.0f, 0.0f, 0.7f);
    for (float ex = 620.0f; ex <= 630.0f; ex += 8.0f) {
        glBegin(GL_QUADS);
        glVertex2f(ex, 210); glVertex2f(ex+3, 210);
        glVertex2f(ex+3, 213); glVertex2f(ex, 213);
        glEnd();
    }
    glDisable(GL_BLEND);

    // --- PANEL KIRI (overlay gelap agar teks terbaca) ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.03f, 0.01f, 0.06f, 0.82f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(370,0); glVertex2f(370,600); glVertex2f(0,600);
    glEnd();
    glDisable(GL_BLEND);

    // --- POHON MATI KIRI (stick figure) ---
    glColor3f(0.08f, 0.04f, 0.12f);
    glLineWidth(4.0f);
    glBegin(GL_LINES);
    // Pohon 1
    glVertex2f(20, 150); glVertex2f(40, 420);
    glEnd();
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(40, 320); glVertex2f(15, 275);
    glVertex2f(40, 320); glVertex2f(65, 270);
    glVertex2f(40, 360); glVertex2f(10, 340);
    glEnd();
    glLineWidth(3.5f);
    glBegin(GL_LINES);
    // Pohon 2 (lebih kecil)
    glVertex2f(350, 200); glVertex2f(365, 420);
    glEnd();
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(358, 280); glVertex2f(340, 252);
    glVertex2f(358, 280); glVertex2f(375, 248);
    glEnd();

    // --- NISAN / GRAVESTONES ---
    glColor3f(0.09f, 0.05f, 0.15f);
    // Nisan 1
    glBegin(GL_QUADS);
    glVertex2f(55, 420); glVertex2f(73, 420); glVertex2f(73, 450); glVertex2f(55, 450);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(50, 446); glVertex2f(78, 446); glVertex2f(78, 452); glVertex2f(50, 452);
    glEnd();
    // Arc kepala nisan
    // (gunakan segmen polygon sebagai pengganti arc)
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(64, 420);
    for (int a = 0; a <= 180; a += 15) {
        float rad2 = a * (float)M_PI / 180.0f;
        glVertex2f(64 + cosf(rad2) * 9, 420 - sinf(rad2) * 9);
    }
    glEnd();
    // Nisan 2 (kecil)
    glBegin(GL_QUADS);
    glVertex2f(88, 430); glVertex2f(101, 430); glVertex2f(101, 452); glVertex2f(88, 452);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(84, 449); glVertex2f(105, 449); glVertex2f(105, 454); glVertex2f(84, 454);
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(94.5f, 430);
    for (int a = 0; a <= 180; a += 15) {
        float rad2 = a * (float)M_PI / 180.0f;
        glVertex2f(94.5f + cosf(rad2) * 7, 430 - sinf(rad2) * 7);
    }
    glEnd();

    // --- JUDUL ---
    // "MAZE" kecil di atas
    glColor3f(0.45f, 0.22f, 0.65f);
    glRasterPos2f(38, 545);
    const char* sub = "MAZE";
    for (const char* c = sub; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    // "ESCAPE" besar
    glColor3f(0.78f, 0.15f, 0.18f);
    glRasterPos2f(36, 515);
    const char* title = "ESCAPE";
    for (const char* c = title; *c; c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    // Ulangi untuk bold effect
    glRasterPos2f(37, 515);
    for (const char* c = title; *c; c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);

    // Subtitle
    glColor3f(0.35f, 0.18f, 0.45f);
    glRasterPos2f(36, 496);
    const char* sub2 = "FIND TREASURE & SURVIVE";
    for (const char* c = sub2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // --- GARIS PEMISAH ---
    glColor3f(0.20f, 0.08f, 0.28f);
    glLineWidth(0.5f);
    glBegin(GL_LINES);
    glVertex2f(36, 488); glVertex2f(340, 488);
    glEnd();

    // --- HOW TO PLAY ---
    glColor3f(0.45f, 0.28f, 0.55f);
    glRasterPos2f(36, 472);
    const char* howLabel = "--- HOW TO PLAY ---";
    for (const char* c = howLabel; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // Controls
    struct { const char* key; const char* desc; float kr; float kg; float kb; } controls[] = {
        {"WASD / Arrows",  ": Move",                    0.65f, 0.45f, 0.75f},
        {"Mouse",          ": Look / Turn",              0.65f, 0.45f, 0.75f},
        {"H",              ": Hide (near crates)",       0.65f, 0.45f, 0.75f},
        {"Gold GPS box",   ": Arrow to Treasure",        0.80f, 0.65f, 0.0f},
        {"Green GPS box",  ": Arrow to Exit",            0.0f,  0.70f, 0.30f},
    };
    float cy = 453.0f;
    for (int i = 0; i < 5; i++) {
        glColor3f(controls[i].kr, controls[i].kg, controls[i].kb);
        glRasterPos2f(36, cy);
        for (const char* c = controls[i].key; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        glColor3f(0.45f, 0.35f, 0.52f);
        glRasterPos2f(160, cy);
        for (const char* c = controls[i].desc; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        cy -= 17.0f;
    }

    // --- RULES ---
    glColor3f(0.20f, 0.08f, 0.28f);
    glLineWidth(0.5f);
    glBegin(GL_LINES);
    glVertex2f(36, cy - 2); glVertex2f(340, cy - 2);
    glEnd();
    cy -= 16.0f;

    glColor3f(0.45f, 0.28f, 0.55f);
    glRasterPos2f(36, cy);
    const char* rulesLabel = "--- RULES ---";
    for (const char* c = rulesLabel; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    cy -= 17.0f;

    struct { const char* line; float r, g, b; } rules[] = {
        {"Every 40s lights come ON for 10s!", 0.80f, 0.15f, 0.15f},
        {"Lights make enemies faster.",       0.55f, 0.40f, 0.62f},
        {"Hide in crates to stay safe!",      0.55f, 0.40f, 0.62f},
        {"Avoid the dark humanoid enemies!",  0.55f, 0.40f, 0.62f},
        {"Exit WITH treasure = best score!",  0.80f, 0.65f, 0.0f},
    };
    for (int i = 0; i < 5; i++) {
        glColor3f(rules[i].r, rules[i].g, rules[i].b);
        glRasterPos2f(36, cy);
        for (const char* c = rules[i].line; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        cy -= 17.0f;
    }

    // --- GARIS BAWAH ---
    glColor3f(0.20f, 0.08f, 0.28f);
    glBegin(GL_LINES);
    glVertex2f(36, cy - 4); glVertex2f(340, cy - 4);
    glEnd();
    cy -= 20.0f;

    // --- TOMBOL START (berkedip) ---
    float ba = 0.5f + 0.5f * sinf(t * 2.5f);
    glColor3f(0.1f * ba, ba * 0.85f, 0.2f * ba);
    glRasterPos2f(36, cy);
    const char* startMsg = "> Press ENTER or SPACE to Start";
    for (const char* c = startMsg; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    endOrtho();
}

void drawGameOver() {
    prepareStaticScreen(0.04f,0,0);
    beginOrtho();
    glColor3f(0.04f,0,0);glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(800,0);glVertex2f(800,600);glVertex2f(0,600);glEnd();
    float t=(float)glutGet(GLUT_ELAPSED_TIME)/1000.0f;
    float p=0.7f+0.3f*sinf(t*3.5f);
    drawTextLarge(255,400,"GAME OVER",p,0.05f,0.05f);
    char buf[64];
    if(wonWithTreasure){
        drawText2D(195,340,"You had the treasure but didn't make it...",1,0.75f,0);
        sprintf(buf,"Survival Score: %d",finalScore);drawText2D(290,300,buf,1,0.75f,0);
    } else {
        drawText2D(245,340,"You were caught in the dark...",0.8f,0.4f,0.4f);
        drawText2D(310,300,"Score: 0",0.6f,0.6f,0.6f);
    }
    drawTextLarge(220,200,"R - Retry   M - Menu",0.7f,0.7f,0.7f);
    endOrtho();
}

void drawWin() {
    prepareStaticScreen(0,0.04f,0.02f);
    beginOrtho();
    glColor3f(0,0.04f,0.02f);glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(800,0);glVertex2f(800,600);glVertex2f(0,600);glEnd();
    if(wonWithTreasure){
        drawTextLarge(130,420,"YOU ESCAPED WITH ALL TREASURES!",0.3f,1,0.45f);
        char buf[64];sprintf(buf,"FINAL SCORE: %d",finalScore);
        drawTextLarge(255,355,buf,0.3f,1,0.3f);
    } else {
        char buf[64];
        sprintf(buf,"You escaped with %d/%d treasures!", treasuresHeld(), treasureCount);
        drawText2D(155,420,buf,0.55f,0.8f,0.55f);
        sprintf(buf,"SCORE: %d", finalScore);
        drawTextLarge(275,355,buf,0.45f,0.65f,0.45f);
        drawText2D(165,310,"Collect ALL treasures for maximum score!",0.7f,0.7f,0.7f);
    }
    drawTextLarge(220,240,"R - Retry   M - Menu",0.7f,0.7f,0.7f);
    endOrtho();
}

void display() {
    glLoadIdentity();
    if(gameState==STATE_MENU)    {drawMenu();    glutSwapBuffers();return;}
    if(gameState==STATE_GAMEOVER){drawGameOver();glutSwapBuffers();return;}
    if(gameState==STATE_WIN)     {drawWin();     glutSwapBuffers();return;}

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
