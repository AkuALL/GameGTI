#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <algorithm>

// ================= CONSTANTS =================
#define MAP_MIN -14.0f
#define MAP_MAX  14.0f
#define M_PI 3.14159265f

// ================= GAME STATE =================
enum GameState { STATE_MENU, STATE_PLAYING, STATE_GAMEOVER, STATE_WIN };
GameState gameState = STATE_MENU;

// ================= PLAYER =================
float playerX = 0.0f, playerZ = 0.0f;
float playerAngle = 0.0f;
float speed = 0.25f;
bool playerHasTreasure = false;
bool playerIsHiding = false;

// ================= CAMERA =================
float eyeHeight = 1.25f;
float lookDistance = 6.0f;

// ================= MOUSE =================
int lastMouseX = 400;
float mouseSensitivity = 0.2f;

// ================= WALLS =================
struct Wall { float x, z, sx, sz; };

// Expanded maze: MAP goes -14 to 14
Wall walls[] = {
    // Outer boundary (14 units)
    { 0,  -14,  28, 1},    // bottom wall
    { 0,   14,  28, 1},    // top wall
    {-14,   0,   1, 28},   // left wall (full)
    // Right wall split: gap at z ~ -11 to -9 (exit gap)
    { 14,   3.5f, 1, 19},  // right wall top portion
    { 14, -12.5f, 1,  3},  // right wall bottom small

    // ---- Interior walls (expanded maze) ----
    // Horizontal corridors
    {-7,   10,   1,  8},   // left vertical upper
    { 0,   10,   8,  1},   // horizontal top-mid
    { 8,    8,   1,  8},   // right vertical upper
    {-4,    6,   6,  1},   // horizontal mid-upper-left
    { 5,    4,   8,  1},   // horizontal mid-upper-right
    {-9,    2,   8,  1},   // horizontal center-left
    { 3,    0,   1, 10},   // vertical center
    {-5,   -2,   8,  1},   // horizontal center-mid
    { 8,   -4,   8,  1},   // horizontal mid-lower-right
    {-10,  -6,   1,  8},   // vertical lower-left
    { 0,   -8,   8,  1},   // horizontal lower-mid
    {-5,  -10,  10,  1},   // horizontal lower-left
    { 6,  -12,   6,  1},   // horizontal bottom-right
    {-2,   -4,   1,  6},   // small vertical center-lower
    { 11,   0,   1,  8},   // right-side vertical
    {-12,   6,   4,  1},   // small horizontal far-left upper
    { 5,  -10,   1,  6},   // small vertical lower-right
    {-8,  -12,   1,  4},   // small vertical bottom-left
    { 2,   12,   1,  4},   // small vertical top
    {-3,    2,   4,  1},   // small horizontal center
};
int wallCount = 24;

// ================= EXIT =================
float exitX = 14.5f, exitZ = -10.0f;

// ================= TREASURE =================
float treasureX, treasureZ;

// ================= HIDING SPOTS =================
struct HidingSpot { float x, z; };
HidingSpot hidingSpots[] = {
    {-12, -12},
    {-12,  12},
    { 12,  12},
    {-6,    4},
    { 7,   -2},
    { 0,   11},
    {-3,   -9},
    { 9,   -9},
    {-9,    0},
    { 5,    6},
    {-1,  -13},
    { 11,   5},
};
int hidingCount = 12;

// ================= ENEMIES =================
struct Enemy {
    float x, z;
    float angle;
    float speed;
    int waypointIdx;
    float wpX[8], wpZ[8];
    int wpCount;
    // For smooth rotation
    float targetAngle;
};

Enemy enemies[5];

// ================= LIGHT CYCLE =================
float gameTimer = 0.0f;
float lightCycleInterval = 60.0f;
float lightOnDuration = 10.0f;
bool lightsOn = false;
float lightsOnTimer = 0.0f;
float lastTime = 0.0f;

// ================= ATMOSPHERE =================
float flickerTimer = 0.0f;
float flickerIntensity = 1.0f;
float breathTimer = 0.0f; // ambient pulsing

// ================= SCORE =================
float startTime = 0.0f;
int finalScore = 0;
bool wonWithTreasure = false;

// ================= FORWARD DECLARATIONS =================
bool checkCollision(float newX, float newZ);
bool checkCollisionEnemy(float newX, float newZ, float margin);
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
        x = (float)(rand() % 240 - 120) / 10.0f;
        z = (float)(rand() % 240 - 120) / 10.0f;
        tries++;
    } while (!positionValid(x, z) && tries < 1000);
}

// ===================================================
// ENEMY WALL COLLISION
// ===================================================
bool checkCollisionEnemy(float newX, float newZ, float margin) {
    for (int i = 0; i < wallCount; i++) {
        float wx = walls[i].x, wz = walls[i].z;
        float halfX = walls[i].sx / 2.0f + margin;
        float halfZ = walls[i].sz / 2.0f + margin;
        if (newX > wx - halfX && newX < wx + halfX &&
            newZ > wz - halfZ && newZ < wz + halfZ)
            return true;
    }
    // Map boundary
    if (newX < MAP_MIN + margin || newX > MAP_MAX - margin ||
        newZ < MAP_MIN + margin || newZ > MAP_MAX - margin)
        return true;
    return false;
}

// ===================================================
// ENEMY INIT
// ===================================================
void initEnemies() {
    // Enemy 0: top-left patrol
    enemies[0] = {-11.0f, 11.0f, 0, 0.04f, 0, {}, {}, 4, 0};
    enemies[0].wpX[0]=-11; enemies[0].wpZ[0]=11;
    enemies[0].wpX[1]= -8; enemies[0].wpZ[1]=11;
    enemies[0].wpX[2]= -8; enemies[0].wpZ[2]= 7;
    enemies[0].wpX[3]=-11; enemies[0].wpZ[3]= 7;
    enemies[0].wpCount = 4;

    // Enemy 1: top-right patrol
    enemies[1] = {9.0f, 11.0f, 0, 0.04f, 0, {}, {}, 4, 0};
    enemies[1].wpX[0]= 9; enemies[1].wpZ[0]=11;
    enemies[1].wpX[1]=12; enemies[1].wpZ[1]=11;
    enemies[1].wpX[2]=12; enemies[1].wpZ[2]= 5;
    enemies[1].wpX[3]= 9; enemies[1].wpZ[3]= 5;
    enemies[1].wpCount = 4;

    // Enemy 2: bottom-left patrol
    enemies[2] = {-12.0f, -4.0f, 0, 0.04f, 0, {}, {}, 4, 0};
    enemies[2].wpX[0]=-12; enemies[2].wpZ[0]=-4;
    enemies[2].wpX[1]= -7; enemies[2].wpZ[1]=-4;
    enemies[2].wpX[2]= -7; enemies[2].wpZ[2]=-11;
    enemies[2].wpX[3]=-12; enemies[2].wpZ[3]=-11;
    enemies[2].wpCount = 4;

    // Enemy 3: center roamer
    enemies[3] = {4.0f, -2.0f, 0, 0.04f, 0, {}, {}, 6, 0};
    enemies[3].wpX[0]= 4; enemies[3].wpZ[0]=-2;
    enemies[3].wpX[1]= 1; enemies[3].wpZ[1]=-2;
    enemies[3].wpX[2]= 1; enemies[3].wpZ[2]=-7;
    enemies[3].wpX[3]= 4; enemies[3].wpZ[3]=-7;
    enemies[3].wpX[4]= 7; enemies[3].wpZ[4]=-7;
    enemies[3].wpX[5]= 7; enemies[3].wpZ[5]=-2;
    enemies[3].wpCount = 6;

    // Enemy 4: bottom-right patrol
    enemies[4] = {10.0f, -7.0f, 0, 0.045f, 0, {}, {}, 4, 0};
    enemies[4].wpX[0]=10; enemies[4].wpZ[0]=-7;
    enemies[4].wpX[1]=12; enemies[4].wpZ[1]=-7;
    enemies[4].wpX[2]=12; enemies[4].wpZ[2]=-13;
    enemies[4].wpX[3]=10; enemies[4].wpZ[3]=-13;
    enemies[4].wpCount = 4;
}

// ===================================================
// COLLISION
// ===================================================
bool checkCollision(float newX, float newZ) {
    float playerSize = 0.45f;
    // Map boundary
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
// RESET / INIT
// ===================================================
void resetGame() {
    srand((unsigned)time(NULL));
    playerHasTreasure = false;
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

    randomPos(playerX, playerZ);

    float tx, tz;
    do { randomPos(tx, tz); }
    while (fabs(tx - playerX) < 3.0f && fabs(tz - playerZ) < 3.0f);
    treasureX = tx; treasureZ = tz;

    initEnemies();
    gameState = STATE_PLAYING;
    startTime = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    lastTime = startTime;
}

// ===================================================
// CHECK HIDING
// ===================================================
bool isNearHidingSpot() {
    float radius = 1.4f;
    for (int i = 0; i < hidingCount; i++) {
        float dx = playerX - hidingSpots[i].x;
        float dz = playerZ - hidingSpots[i].z;
        if (sqrtf(dx*dx + dz*dz) < radius) return true;
    }
    return false;
}

// ===================================================
// CHECK TREASURE PICKUP
// ===================================================
void checkTreasurePickup() {
    if (!playerHasTreasure) {
        float dx = playerX - treasureX, dz = playerZ - treasureZ;
        if (sqrtf(dx*dx + dz*dz) < 1.0f) {
            playerHasTreasure = true;
        }
    }
}

// ===================================================
// CHECK EXIT
// ===================================================
void checkExit() {
    if (playerX > 13.5f && playerZ > -12.0f && playerZ < -8.0f) {
        wonWithTreasure = playerHasTreasure;
        float elapsed = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f - startTime;
        if (wonWithTreasure) {
            finalScore = (int)(10000.0f / (elapsed + 1.0f)) * 10;
        } else {
            finalScore = 0;
        }
        gameState = STATE_WIN;
    }
}

// ===================================================
// ENEMY UPDATE (with wall avoidance)
// ===================================================
void updateEnemies(float dt) {
    for (int i = 0; i < 5; i++) {
        Enemy &e = enemies[i];
        int next = (e.waypointIdx + 1) % e.wpCount;
        float tx = e.wpX[next], tz = e.wpZ[next];
        float dx = tx - e.x, dz = tz - e.z;
        float dist = sqrtf(dx*dx + dz*dz);
        if (dist < 0.2f) {
            e.waypointIdx = next;
            continue;
        }

        float moveSpeed = e.speed * (lightsOn ? 2.2f : 1.0f);
        float nx = e.x + (dx / dist) * moveSpeed;
        float nz = e.z + (dz / dist) * moveSpeed;

        // Wall collision for enemy
        float emargin = 0.45f;
        if (!checkCollisionEnemy(nx, nz, emargin)) {
            e.x = nx;
            e.z = nz;
        } else {
            // Try sliding on X only
            float nxOnly = e.x + (dx / dist) * moveSpeed;
            if (!checkCollisionEnemy(nxOnly, e.z, emargin))
                e.x = nxOnly;
            // Try sliding on Z only
            else if (!checkCollisionEnemy(e.x, nz, emargin))
                e.z = nz;
            else
                e.waypointIdx = next; // stuck, skip to next waypoint
        }

        // Smooth angle
        float targetAngle = atan2f(dx, dz) * 180.0f / (float)M_PI;
        e.angle = targetAngle;

        // Check enemy-player collision
        float pdx = e.x - playerX, pdz = e.z - playerZ;
        if (sqrtf(pdx*pdx + pdz*pdz) < 0.9f) {
            if (!playerIsHiding) {
                finalScore = 0;
                wonWithTreasure = false;
                gameState = STATE_GAMEOVER;
            }
        }
    }
}

// ===================================================
// LIGHT CYCLE UPDATE
// ===================================================
void updateLights(float dt) {
    if (!lightsOn) {
        gameTimer += dt;
        if (gameTimer >= lightCycleInterval) {
            lightsOn = true;
            lightsOnTimer = 0.0f;
            gameTimer = 0.0f;
            if (!playerIsHiding) {
                finalScore = 0;
                if (playerHasTreasure) {
                    float elapsed = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f - startTime;
                    finalScore = (int)(elapsed * 50.0f);
                }
                wonWithTreasure = playerHasTreasure;
                gameState = STATE_GAMEOVER;
                return;
            }
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

// ===================================================
// ATMOSPHERE UPDATE
// ===================================================
void updateAtmosphere(float dt) {
    breathTimer += dt * 0.8f;

    // Subtle flicker when lights are off
    if (!lightsOn) {
        flickerTimer += dt;
        if (flickerTimer > 0.05f + (float)(rand() % 10) / 100.0f) {
            flickerTimer = 0.0f;
            // Random subtle flicker
            flickerIntensity = 0.88f + (float)(rand() % 25) / 100.0f;
        }
    } else {
        flickerIntensity = 1.0f;
    }
}

// ===================================================
// TIMER / UPDATE
// ===================================================
void update(int value) {
    if (gameState == STATE_PLAYING) {
        float now = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        float dt = now - lastTime;
        lastTime = now;

        updateAtmosphere(dt);
        updateLights(dt);
        if (gameState != STATE_PLAYING) { glutPostRedisplay(); return; }
        updateEnemies(dt);
        if (gameState != STATE_PLAYING) { glutPostRedisplay(); return; }
        checkTreasurePickup();
        checkExit();
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// ===================================================
// MOUSE
// ===================================================
void mouseMotion(int x, int y) {
    if (gameState != STATE_PLAYING) return;
    int deltaX = x - lastMouseX;
    lastMouseX = x;
    playerAngle += deltaX * mouseSensitivity;
    if (playerAngle >= 360.0f) playerAngle -= 360.0f;
    if (playerAngle < 0.0f) playerAngle += 360.0f;
    glutPostRedisplay();
}

// ===================================================
// MOVE PLAYER
// ===================================================
void movePlayer(float forward, float strafe) {
    float rad = playerAngle * (float)M_PI / 180.0f;
    float newX = playerX + sinf(rad)*forward + cosf(rad)*strafe;
    float newZ = playerZ + cosf(rad)*forward - sinf(rad)*strafe;
    if (!checkCollision(newX, newZ)) {
        playerX = newX;
        playerZ = newZ;
    }
}

// ===================================================
// KEYBOARD
// ===================================================
void keyboard(unsigned char key, int x, int y) {
    if (gameState == STATE_MENU) {
        if (key == 13 || key == ' ') resetGame();
        return;
    }
    if (gameState == STATE_GAMEOVER || gameState == STATE_WIN) {
        if (key == 'r' || key == 'R') { resetGame(); }
        if (key == 'm' || key == 'M') { gameState = STATE_MENU; }
        return;
    }
    switch (key) {
        case 'w': case 'W': movePlayer(speed, 0); break;
        case 's': case 'S': movePlayer(-speed, 0); break;
        case 'a': case 'A': movePlayer(0, -speed); break;
        case 'd': case 'D': movePlayer(0, speed); break;
        case 'h': case 'H':
            if (isNearHidingSpot()) playerIsHiding = !playerIsHiding;
            else playerIsHiding = false;
            break;
        case 27: exit(0); break;
    }
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    if (gameState != STATE_PLAYING) return;
    switch (key) {
        case GLUT_KEY_UP:    movePlayer(speed, 0); break;
        case GLUT_KEY_DOWN:  movePlayer(-speed, 0); break;
        case GLUT_KEY_LEFT:  movePlayer(0, -speed); break;
        case GLUT_KEY_RIGHT: movePlayer(0, speed); break;
    }
    glutPostRedisplay();
}

// ===================================================
// DRAW HELPERS
// ===================================================

// Apply flicker to current color
void applyFlicker(float r, float g, float b) {
    glColor3f(r * flickerIntensity, g * flickerIntensity, b * flickerIntensity);
}

// ===================================================
// PROCEDURAL STONE TEXTURE FLOOR
// ===================================================

// Simple hash for deterministic pseudo-random per cell
float cellHash(int ix, int iz) {
    int n = ix * 1619 + iz * 31337;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

// Draw a single stone slab at world position (wx, wz) with size (sw x sh)
// slab variation: base color offset, crack style
void drawStoneSlab(float wx, float wz, float sw, float sh,
                   float baseR, float baseG, float baseB,
                   float crackR, float crackG, float crackB,
                   int cracks) {
    // Slab surface
    glColor3f(baseR, baseG, baseB);
    glBegin(GL_QUADS);
    glVertex3f(wx,      0.0f, wz);
    glVertex3f(wx + sw, 0.0f, wz);
    glVertex3f(wx + sw, 0.0f, wz + sh);
    glVertex3f(wx,      0.0f, wz + sh);
    glEnd();

    // Grout line (thin dark border inside each slab)
    float g = 0.04f; // grout thickness
    glColor3f(crackR * 0.55f, crackG * 0.55f, crackB * 0.55f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(wx + g,      0.01f, wz + g);
    glVertex3f(wx + sw - g, 0.01f, wz + g);
    glVertex3f(wx + sw - g, 0.01f, wz + sh - g);
    glVertex3f(wx + g,      0.01f, wz + sh - g);
    glEnd();

    // Surface cracks (thin lines inside the slab)
    glColor3f(crackR, crackG, crackB);
    glLineWidth(1.0f);
    float cx = wx + sw * 0.5f;
    float cz = wz + sh * 0.5f;
    for (int c = 0; c < cracks; c++) {
        // deterministic offset from cell position
        float h1 = cellHash((int)(wx * 10 + c), (int)(wz * 10 + c * 7));
        float h2 = cellHash((int)(wx * 10 + c * 3), (int)(wz * 10 + c * 11));
        float h3 = cellHash((int)(wx * 10 + c * 5), (int)(wz * 10 + c * 13));
        float h4 = cellHash((int)(wx * 10 + c * 9), (int)(wz * 10 + c * 17));
        float x1 = wx + g + fabsf(h1) * (sw - 2*g);
        float z1 = wz + g + fabsf(h2) * (sh - 2*g);
        float x2 = wx + g + fabsf(h3) * (sw - 2*g);
        float z2 = wz + g + fabsf(h4) * (sh - 2*g);
        glBegin(GL_LINES);
        glVertex3f(x1, 0.012f, z1);
        glVertex3f(x2, 0.012f, z2);
        glEnd();
    }
}

void drawFloor() {
    // --- FLOOR: irregular stone slabs in a grid pattern ---
    // We use two slab sizes alternated: 1.5 x 2.0 and 2.0 x 1.5
    // giving a brick-like offset feel without actual checkerboard

    float slabW = 2.0f;
    float slabH = 1.5f;
    int cols = (int)((MAP_MAX - MAP_MIN) / slabW) + 1;
    int rows = (int)((MAP_MAX - MAP_MIN) / slabH) + 1;

    for (int row = 0; row < rows; row++) {
        float fz = MAP_MIN + row * slabH;
        // Offset every other row (brick pattern)
        float offsetX = (row % 2 == 0) ? 0.0f : slabW * 0.5f;

        for (int col = 0; col < cols; col++) {
            float fx = MAP_MIN - offsetX + col * slabW;

            // Clip to map
            if (fx + slabW < MAP_MIN || fx > MAP_MAX) continue;
            if (fz + slabH < MAP_MIN || fz > MAP_MAX) continue;

            // Per-slab color variation using hash
            float var = cellHash(col + row * 47, row + col * 31) * 0.5f + 0.5f; // 0..1

            float r, g, b;
            float cr, cg, cb; // crack color
            int numCracks;

            if (lightsOn) {
                // Lit: warm grey stone with slight variation
                float base = 0.50f + var * 0.18f;
                float warm = 0.03f + var * 0.04f;
                r = base + warm;
                g = base + warm * 0.5f;
                b = base - warm * 0.5f;
                cr = 0.28f; cg = 0.25f; cb = 0.22f;
                numCracks = (int)(var * 2.5f); // 0-2 cracks per slab
            } else {
                // Dark: deep slate with blue-grey tinge
                float base = 0.08f + var * 0.07f;
                r = base * flickerIntensity * 0.9f;
                g = base * flickerIntensity * 0.95f;
                b = (base + 0.04f) * flickerIntensity;
                cr = 0.03f * flickerIntensity;
                cg = 0.03f * flickerIntensity;
                cb = 0.06f * flickerIntensity;
                numCracks = (int)(var * 2.0f);
            }

            drawStoneSlab(fx, fz, slabW, slabH, r, g, b, cr, cg, cb, numCracks);
        }
    }

    // --- CEILING ---
    float ceilH = 3.5f;
    if (!lightsOn) {
        // Dark ceiling with subtle beam-like pattern
        float base = 0.04f * flickerIntensity;
        glColor3f(base * 0.8f, base * 0.8f, base);
        glBegin(GL_QUADS);
        glVertex3f(MAP_MIN, ceilH, MAP_MIN);
        glVertex3f(MAP_MAX, ceilH, MAP_MIN);
        glVertex3f(MAP_MAX, ceilH, MAP_MAX);
        glVertex3f(MAP_MIN, ceilH, MAP_MAX);
        glEnd();

        // Ceiling beams (dark stripes across ceiling)
        glColor3f(0.02f * flickerIntensity, 0.02f * flickerIntensity, 0.03f * flickerIntensity);
        float beamSpacing = 4.0f;
        for (float bx = MAP_MIN; bx < MAP_MAX; bx += beamSpacing) {
            glBegin(GL_QUADS);
            glVertex3f(bx,        ceilH - 0.01f, MAP_MIN);
            glVertex3f(bx + 0.3f, ceilH - 0.01f, MAP_MIN);
            glVertex3f(bx + 0.3f, ceilH - 0.01f, MAP_MAX);
            glVertex3f(bx,        ceilH - 0.01f, MAP_MAX);
            glEnd();
        }
    } else {
        // Lit ceiling: concrete panels with joints
        float base = 0.62f;
        glColor3f(base, base, base + 0.04f);
        glBegin(GL_QUADS);
        glVertex3f(MAP_MIN, ceilH, MAP_MIN);
        glVertex3f(MAP_MAX, ceilH, MAP_MIN);
        glVertex3f(MAP_MAX, ceilH, MAP_MAX);
        glVertex3f(MAP_MIN, ceilH, MAP_MAX);
        glEnd();

        // Panel joints
        glColor3f(0.45f, 0.45f, 0.47f);
        glLineWidth(1.5f);
        float panelSize = 3.0f;
        for (float px = MAP_MIN; px <= MAP_MAX; px += panelSize) {
            glBegin(GL_LINES);
            glVertex3f(px, ceilH - 0.01f, MAP_MIN);
            glVertex3f(px, ceilH - 0.01f, MAP_MAX);
            glEnd();
        }
        for (float pz = MAP_MIN; pz <= MAP_MAX; pz += panelSize) {
            glBegin(GL_LINES);
            glVertex3f(MAP_MIN, ceilH - 0.01f, pz);
            glVertex3f(MAP_MAX, ceilH - 0.01f, pz);
            glEnd();
        }
    }
}

void drawWall(float x, float z, float sx, float sz) {
    float wallH = 3.5f;

    // Main wall block
    glPushMatrix();
    glTranslatef(x, wallH * 0.5f, z);
    glScalef(sx, wallH, sz);
    if (lightsOn) {
        glColor3f(0.52f, 0.56f, 0.65f);
    } else {
        applyFlicker(0.08f, 0.10f, 0.22f);
    }
    glutSolidCube(1.0f);
    if (!lightsOn) {
        glColor3f(0.01f, 0.01f, 0.04f);
        glutWireCube(1.0f);
    }
    glPopMatrix();

    // Mortar bands at 1/3 and 2/3 height
    for (int band = 1; band <= 2; band++) {
        float by = (wallH / 3.0f) * band;
        glPushMatrix();
        glTranslatef(x, by, z);
        glScalef(sx + 0.01f, 0.06f, sz + 0.01f);
        if (lightsOn)
            glColor3f(0.33f, 0.36f, 0.42f);
        else
            glColor3f(0.02f, 0.02f, 0.06f);
        glutSolidCube(1.0f);
        glPopMatrix();
    }
}

void drawMaze() {
    for (int i = 0; i < wallCount; i++)
        drawWall(walls[i].x, walls[i].z, walls[i].sx, walls[i].sz);
}

// Draw exit marker — glowing green strip
void drawExit() {
    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 800.0f;
    float pulse = 0.6f + 0.4f * sinf(t * 3.0f);

    glPushMatrix();
    glTranslatef(13.5f, 0.05f, exitZ);
    glColor3f(0.0f, pulse, 0.2f * pulse);
    glBegin(GL_QUADS);
    glVertex3f(-0.5f, 0, -2.0f);
    glVertex3f( 0.5f, 0, -2.0f);
    glVertex3f( 0.5f, 0,  2.0f);
    glVertex3f(-0.5f, 0,  2.0f);
    glEnd();

    // Vertical beacon strips on wall
    glColor3f(0.0f, pulse * 0.8f, 0.0f);
    glPushMatrix();
    glTranslatef(0.6f, 1.0f, 0);
    glScalef(0.2f, 2.0f, 4.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix();
}

// ===================================================
// HIDING SPOT — drawn as a wooden crate/box
// ===================================================
void drawHidingSpots() {
    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    for (int i = 0; i < hidingCount; i++) {
        float hx = hidingSpots[i].x;
        float hz = hidingSpots[i].z;

        glPushMatrix();
        glTranslatef(hx, 0.0f, hz);

        // Crate base (bottom)
        if (lightsOn) {
            glColor3f(0.55f, 0.38f, 0.18f);
        } else {
            float pulse = 0.3f + 0.08f * sinf(t * 1.5f + i);
            applyFlicker(pulse * 0.9f, pulse * 0.65f, pulse * 0.2f);
        }
        glPushMatrix();
        glTranslatef(0, 0.6f, 0);
        glScalef(1.1f, 1.2f, 1.1f);
        glutSolidCube(1.0f);
        glPopMatrix();

        // Crate lid (slightly lighter)
        if (lightsOn) {
            glColor3f(0.65f, 0.48f, 0.22f);
        } else {
            float pulse = 0.4f + 0.10f * sinf(t * 1.5f + i);
            applyFlicker(pulse, pulse * 0.7f, pulse * 0.25f);
        }
        glPushMatrix();
        glTranslatef(0, 1.25f, 0);
        glScalef(1.15f, 0.15f, 1.15f);
        glutSolidCube(1.0f);
        glPopMatrix();

        // Crate cross-planks (dark lines drawn as thin boxes)
        glColor3f(0.18f, 0.11f, 0.04f);
        // Front cross plank horizontal
        glPushMatrix();
        glTranslatef(0, 0.6f, 0.56f);
        glScalef(1.1f, 0.08f, 0.05f);
        glutSolidCube(1.0f);
        glPopMatrix();
        // Front cross plank vertical
        glPushMatrix();
        glTranslatef(0, 0.6f, 0.56f);
        glScalef(0.08f, 1.2f, 0.05f);
        glutSolidCube(1.0f);
        glPopMatrix();

        // Glow on floor beneath hiding spot
        float glow = 0.25f + 0.1f * sinf(t * 2.0f + i);
        if (lightsOn) {
            glColor3f(0.8f, 0.7f, 0.0f);
        } else {
            glColor3f(glow * 0.9f, glow * 0.6f, 0.0f);
        }
        glBegin(GL_QUADS);
        glVertex3f(-0.7f, 0.01f, -0.7f);
        glVertex3f( 0.7f, 0.01f, -0.7f);
        glVertex3f( 0.7f, 0.01f,  0.7f);
        glVertex3f(-0.7f, 0.01f,  0.7f);
        glEnd();

        glPopMatrix();
    }
}

// ===================================================
// TREASURE — pile of gold coins
// ===================================================
void drawCoin(float x, float y, float z, float radius, float thickness) {
    glPushMatrix();
    glTranslatef(x, y, z);
    // Draw as a flat disk using a cylinder
    int segs = 12;
    // Top face
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0, thickness / 2.0f, 0);
    for (int s = 0; s <= segs; s++) {
        float a = s * 2.0f * (float)M_PI / segs;
        glVertex3f(cosf(a)*radius, thickness/2.0f, sinf(a)*radius);
    }
    glEnd();
    // Bottom face
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0, -thickness / 2.0f, 0);
    for (int s = segs; s >= 0; s--) {
        float a = s * 2.0f * (float)M_PI / segs;
        glVertex3f(cosf(a)*radius, -thickness/2.0f, sinf(a)*radius);
    }
    glEnd();
    // Side
    glBegin(GL_QUAD_STRIP);
    for (int s = 0; s <= segs; s++) {
        float a = s * 2.0f * (float)M_PI / segs;
        float cx = cosf(a)*radius, cz = sinf(a)*radius;
        glVertex3f(cx, -thickness/2.0f, cz);
        glVertex3f(cx,  thickness/2.0f, cz);
    }
    glEnd();
    glPopMatrix();
}

void drawTreasure() {
    if (playerHasTreasure) return;
    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 600.0f;
    float hover = sinf(t * 1.5f) * 0.08f;

    // Pulsing gold glow on floor
    float glowPulse = 0.3f + 0.2f * sinf(t * 3.0f);
    glColor3f(glowPulse, glowPulse * 0.7f, 0.0f);
    glPushMatrix();
    glTranslatef(treasureX, 0.02f, treasureZ);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0, 0, 0);
    for (int s = 0; s <= 20; s++) {
        float a = s * 2.0f * (float)M_PI / 20;
        float r = 0.8f + 0.15f * sinf(t * 4.0f + s);
        glVertex3f(cosf(a)*r, 0, sinf(a)*r);
    }
    glEnd();
    glPopMatrix();

    // Stack of coins
    glPushMatrix();
    glTranslatef(treasureX, hover, treasureZ);

    // Coin color — rich gold
    float goldR = lightsOn ? 1.0f : (0.85f * flickerIntensity);
    float goldG = lightsOn ? 0.78f : (0.65f * flickerIntensity);
    float goldB = lightsOn ? 0.0f : 0.0f;

    // Bottom scattered coins (flat on floor)
    glColor3f(goldR, goldG, goldB);
    drawCoin( 0.15f, 0.04f,  0.10f, 0.18f, 0.06f);
    drawCoin(-0.20f, 0.04f,  0.05f, 0.16f, 0.06f);
    drawCoin( 0.00f, 0.04f, -0.20f, 0.17f, 0.06f);
    drawCoin( 0.25f, 0.04f, -0.05f, 0.15f, 0.06f);
    drawCoin(-0.10f, 0.04f, -0.15f, 0.18f, 0.06f);

    // Middle stacked coins (slightly raised, some tilted)
    glColor3f(goldR * 1.05f, goldG * 1.05f, goldB);
    drawCoin( 0.05f, 0.12f,  0.0f,  0.20f, 0.07f);
    drawCoin(-0.08f, 0.18f,  0.08f, 0.19f, 0.07f);
    drawCoin( 0.10f, 0.24f, -0.05f, 0.18f, 0.07f);

    // Top coin stack (center pile, tallest)
    glColor3f(fminf(goldR * 1.1f, 1.0f), goldG, goldB);
    drawCoin( 0.0f, 0.10f,  0.0f, 0.22f, 0.08f);
    drawCoin( 0.0f, 0.18f,  0.0f, 0.21f, 0.08f);
    drawCoin( 0.0f, 0.26f,  0.0f, 0.20f, 0.08f);
    drawCoin( 0.0f, 0.34f,  0.0f, 0.19f, 0.08f);
    drawCoin( 0.0f, 0.42f,  0.0f, 0.17f, 0.08f);

    // Glint (small bright sphere on top)
    float glint = 0.5f + 0.5f * sinf(t * 5.0f);
    glColor3f(1.0f, 1.0f, glint * 0.5f);
    glPushMatrix();
    glTranslatef(0, 0.55f, 0);
    glutSolidSphere(0.06f, 8, 8);
    glPopMatrix();

    glPopMatrix();
}

// ===================================================
// ENEMY — humanoid shape (no player body in first-person)
// ===================================================
void drawEnemyBody(int i) {
    Enemy &e = enemies[i];
    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 500.0f;

    // Bobbing motion
    float bob = sinf(t * 3.0f + i * 1.5f) * 0.05f;

    glPushMatrix();
    glTranslatef(e.x, bob, e.z);
    glRotatef(-e.angle + 180.0f, 0, 1, 0);

    // --- Torso ---
    if (lightsOn) {
        glColor3f(0.7f, 0.1f, 0.35f);
    } else {
        glColor3f(0.5f, 0.05f, 0.25f);
    }
    glPushMatrix();
    glTranslatef(0, 1.1f, 0);
    glScalef(0.55f, 0.7f, 0.35f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // --- Head ---
    if (lightsOn) {
        glColor3f(0.85f, 0.2f, 0.4f);
    } else {
        glColor3f(0.6f, 0.1f, 0.28f);
    }
    glPushMatrix();
    glTranslatef(0, 1.65f, 0);
    glutSolidSphere(0.22f, 10, 10);
    glPopMatrix();

    // Eyes (glowing white-yellow, always visible in dark)
    float eyeGlow = lightsOn ? 1.0f : (0.8f + 0.2f * sinf(t * 4.0f + i));
    glColor3f(eyeGlow, eyeGlow * 0.9f, 0.0f);
    glPushMatrix();
    glTranslatef( 0.09f, 1.68f, -0.19f);
    glutSolidSphere(0.055f, 6, 6);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-0.09f, 1.68f, -0.19f);
    glutSolidSphere(0.055f, 6, 6);
    glPopMatrix();

    // --- Pelvis / hips ---
    if (lightsOn) glColor3f(0.5f, 0.08f, 0.25f);
    else glColor3f(0.35f, 0.04f, 0.18f);
    glPushMatrix();
    glTranslatef(0, 0.65f, 0);
    glScalef(0.6f, 0.25f, 0.38f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // --- Left arm ---
    float armSwing = sinf(t * 3.0f + i * 1.5f) * 15.0f;
    if (lightsOn) glColor3f(0.65f, 0.08f, 0.30f);
    else glColor3f(0.45f, 0.04f, 0.22f);
    glPushMatrix();
    glTranslatef(-0.35f, 1.05f, 0);
    glRotatef(armSwing, 1, 0, 0);
    glTranslatef(0, -0.25f, 0);
    glScalef(0.16f, 0.5f, 0.16f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // --- Right arm ---
    glPushMatrix();
    glTranslatef( 0.35f, 1.05f, 0);
    glRotatef(-armSwing, 1, 0, 0);
    glTranslatef(0, -0.25f, 0);
    glScalef(0.16f, 0.5f, 0.16f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // --- Left leg ---
    float legSwing = sinf(t * 3.0f + i * 1.5f + (float)M_PI) * 18.0f;
    if (lightsOn) glColor3f(0.4f, 0.07f, 0.2f);
    else glColor3f(0.28f, 0.04f, 0.14f);
    glPushMatrix();
    glTranslatef(-0.18f, 0.5f, 0);
    glRotatef(legSwing, 1, 0, 0);
    glTranslatef(0, -0.3f, 0);
    glScalef(0.2f, 0.6f, 0.22f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // --- Right leg ---
    glPushMatrix();
    glTranslatef( 0.18f, 0.5f, 0);
    glRotatef(-legSwing, 1, 0, 0);
    glTranslatef(0, -0.3f, 0);
    glScalef(0.2f, 0.6f, 0.22f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Dark shadow / foot blob
    glColor3f(0.05f, 0.02f, 0.05f);
    glPushMatrix();
    glTranslatef(0, 0.02f, 0);
    glScalef(0.7f, 0.05f, 0.5f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix();
}

void drawEnemies() {
    for (int i = 0; i < 5; i++) drawEnemyBody(i);
}

// ===================================================
// CAMERA
// ===================================================
void updateCamera() {
    float rad = playerAngle * (float)M_PI / 180.0f;
    // Subtle head sway for creepiness
    float sway = sinf(breathTimer * 2.5f) * 0.018f;
    float lookX = playerX + sinf(rad) * lookDistance;
    float lookZ = playerZ + cosf(rad) * lookDistance;
    gluLookAt(playerX, eyeHeight + sway, playerZ,
              lookX, eyeHeight + sway, lookZ,
              0, 1, 0);
}

// ===================================================
// TEXT RENDERING
// ===================================================
void drawText2D(float x, float y, const char *text, float r, float g, float b) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (const char *c = text; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
}

void drawTextLarge(float x, float y, const char *text, float r, float g, float b) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (const char *c = text; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
}

// ===================================================
// HUD / GPS
// ===================================================
void beginOrtho() {
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void endOrtho() {
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

void drawGPSArrow(float cx, float cy, float targetX, float targetZ, float r, float g, float b) {
    float dx = targetX - playerX, dz = targetZ - playerZ;
    float angle = atan2f(dx, dz) * 180.0f / (float)M_PI;
    float arrowAngle = angle - playerAngle;

    glPushMatrix();
    glTranslatef(cx, cy, 0);
    glRotatef(arrowAngle, 0, 0, 1);
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLES);
    glVertex2f(0, 22);
    glVertex2f(-9, -8);
    glVertex2f(9, -8);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(-3, -8);
    glVertex2f(3, -8);
    glVertex2f(3, -22);
    glVertex2f(-3, -22);
    glEnd();
    glPopMatrix();
}

void drawGPS() {
    beginOrtho();

    float box1X = 20, box1Y = 490, boxSize = 80;
    glColor3f(0.08f, 0.08f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(box1X, box1Y); glVertex2f(box1X+boxSize, box1Y);
    glVertex2f(box1X+boxSize, box1Y+boxSize); glVertex2f(box1X, box1Y+boxSize);
    glEnd();
    glColor3f(0.5f, 0.5f, 0.5f); glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2f(box1X, box1Y); glVertex2f(box1X+boxSize, box1Y);
    glVertex2f(box1X+boxSize, box1Y+boxSize); glVertex2f(box1X, box1Y+boxSize);
    glEnd();

    if (!playerHasTreasure) {
        drawGPSArrow(box1X+boxSize/2, box1Y+boxSize/2, treasureX, treasureZ, 1.0f, 0.85f, 0.0f);
    } else {
        glColor3f(0.0f, 1.0f, 0.3f); glLineWidth(3);
        glBegin(GL_LINE_STRIP);
        glVertex2f(box1X+20, box1Y+35); glVertex2f(box1X+35, box1Y+20); glVertex2f(box1X+60, box1Y+55);
        glEnd();
    }
    drawText2D(box1X+5, box1Y-18, playerHasTreasure ? "GOT IT!" : "TREASURE", 1.0f, 0.85f, 0.0f);

    float box2X = 110, box2Y = 490;
    glColor3f(0.08f, 0.08f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(box2X, box2Y); glVertex2f(box2X+boxSize, box2Y);
    glVertex2f(box2X+boxSize, box2Y+boxSize); glVertex2f(box2X, box2Y+boxSize);
    glEnd();
    glColor3f(0.5f, 0.5f, 0.5f); glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2f(box2X, box2Y); glVertex2f(box2X+boxSize, box2Y);
    glVertex2f(box2X+boxSize, box2Y+boxSize); glVertex2f(box2X, box2Y+boxSize);
    glEnd();
    drawGPSArrow(box2X+boxSize/2, box2Y+boxSize/2, exitX, exitZ, 0.0f, 1.0f, 0.3f);
    drawText2D(box2X+18, box2Y-18, "EXIT", 0.0f, 1.0f, 0.3f);

    char buf[64];
    float timeToLight = lightsOn ? (lightOnDuration - lightsOnTimer) : (lightCycleInterval - gameTimer);
    if (lightsOn) {
        sprintf(buf, "LIGHTS OFF IN: %.1fs", timeToLight);
        drawText2D(300, 570, buf, 1.0f, 0.3f, 0.3f);
    } else {
        sprintf(buf, "LIGHTS ON IN: %.0fs", timeToLight);
        drawText2D(300, 570, buf, 0.7f, 0.7f, 0.8f);
    }

    float elapsed = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f - startTime;
    sprintf(buf, "TIME: %.0fs", elapsed);
    drawText2D(650, 570, buf, 0.7f, 0.7f, 0.7f);

    if (playerHasTreasure)
        drawText2D(330, 540, "TREASURE: COLLECTED!", 1.0f, 0.85f, 0.0f);
    else
        drawText2D(330, 540, "TREASURE: NOT YET", 0.5f, 0.5f, 0.55f);

    if (playerIsHiding)
        drawText2D(330, 510, "HIDING", 0.3f, 1.0f, 0.3f);
    else if (isNearHidingSpot())
        drawText2D(330, 510, "Press H to Hide", 0.9f, 0.9f, 0.3f);

    if (lightsOn) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.0f, 0.85f, 0.0f, 0.06f);
        glBegin(GL_QUADS);
        glVertex2f(0,0); glVertex2f(800,0); glVertex2f(800,600); glVertex2f(0,600);
        glEnd();
        glDisable(GL_BLEND);

        if (!playerIsHiding)
            drawTextLarge(200, 470, "!! LIGHTS ON - HIDE NOW !!", 1.0f, 0.1f, 0.1f);
        else
            drawTextLarge(270, 470, "STAY HIDDEN!", 0.2f, 1.0f, 0.2f);
    } else {
        // Dark red vignette around edges when lights off (ominous)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float vign = 0.18f + 0.04f * sinf(breathTimer * 0.5f);
        glColor4f(0.15f, 0.0f, 0.0f, vign);
        // top
        glBegin(GL_QUADS); glVertex2f(0,550); glVertex2f(800,550); glVertex2f(800,600); glVertex2f(0,600); glEnd();
        // bottom
        glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(800,0); glVertex2f(800,50); glVertex2f(0,50); glEnd();
        // left
        glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(60,0); glVertex2f(60,600); glVertex2f(0,600); glEnd();
        // right
        glBegin(GL_QUADS); glVertex2f(740,0); glVertex2f(800,0); glVertex2f(800,600); glVertex2f(740,600); glEnd();
        glDisable(GL_BLEND);
    }

    drawText2D(20, 20, "WASD/Arrows:Move  H:Hide  Mouse:Turn", 0.4f, 0.4f, 0.45f);
    endOrtho();
}

// ===================================================
// MENU SCREEN
// ===================================================
void drawMenu() {
    beginOrtho();
    glColor3f(0.03f, 0.03f, 0.06f);
    glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(800,0); glVertex2f(800,600); glVertex2f(0,600); glEnd();

    // Eerie horizontal scanlines
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.12f);
    for (int y = 0; y < 600; y += 4) {
        glBegin(GL_QUADS);
        glVertex2f(0, y); glVertex2f(800, y); glVertex2f(800, y+2); glVertex2f(0, y+2);
        glEnd();
    }
    glDisable(GL_BLEND);

    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float titlePulse = 0.85f + 0.15f * sinf(t * 2.0f);
    drawTextLarge(240, 510, "MAZE ESCAPE", titlePulse, titlePulse * 0.15f, titlePulse * 0.15f);
    drawTextLarge(185, 470, "Find the treasure, then escape!", 0.75f, 0.75f, 0.85f);

    drawText2D(200, 415, "--- HOW TO PLAY ---", 0.6f, 0.6f, 1.0f);
    drawText2D(180, 390, "WASD / Arrow Keys : Move", 0.82f, 0.82f, 0.82f);
    drawText2D(180, 368, "Mouse             : Look / Turn", 0.82f, 0.82f, 0.82f);
    drawText2D(180, 346, "H                 : Hide (near wooden crates)", 0.82f, 0.82f, 0.82f);
    drawText2D(180, 324, "Gold GPS box      : Arrow to Treasure", 1.0f, 0.85f, 0.0f);
    drawText2D(180, 302, "Green GPS box     : Arrow to Exit", 0.0f, 1.0f, 0.3f);

    drawText2D(200, 265, "--- RULES ---", 0.6f, 0.6f, 1.0f);
    drawText2D(180, 242, "Every 60s lights come ON for 10s!", 0.9f, 0.25f, 0.25f);
    drawText2D(180, 220, "Be inside a crate or GAME OVER!", 0.9f, 0.25f, 0.25f);
    drawText2D(180, 198, "Avoid the humanoid enemies!", 0.9f, 0.4f, 0.65f);
    drawText2D(180, 176, "Exit with treasure = best score!", 1.0f, 0.85f, 0.0f);

    float blinkAlpha = 0.5f + 0.5f * sinf(t * 2.5f);
    drawTextLarge(215, 80, "Press ENTER or SPACE to Start", 0.2f * blinkAlpha + 0.1f, blinkAlpha * 0.85f, 0.2f * blinkAlpha + 0.1f);
    endOrtho();
}

// ===================================================
// GAME OVER SCREEN
// ===================================================
void drawGameOver() {
    beginOrtho();
    glColor3f(0.04f, 0.0f, 0.0f);
    glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(800,0); glVertex2f(800,600); glVertex2f(0,600); glEnd();

    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float pulse = 0.7f + 0.3f * sinf(t * 3.5f);
    drawTextLarge(255, 400, "GAME OVER", pulse, 0.05f, 0.05f);

    char buf[64];
    if (wonWithTreasure) {
        drawText2D(195, 340, "You had the treasure but didn't make it...", 1.0f, 0.75f, 0.0f);
        sprintf(buf, "Survival Score: %d", finalScore);
        drawText2D(290, 300, buf, 1.0f, 0.75f, 0.0f);
    } else {
        drawText2D(245, 340, "You were caught in the dark...", 0.8f, 0.4f, 0.4f);
        drawText2D(310, 300, "Score: 0", 0.6f, 0.6f, 0.6f);
    }

    drawTextLarge(220, 200, "R - Retry   M - Menu", 0.7f, 0.7f, 0.7f);
    endOrtho();
}

// ===================================================
// WIN SCREEN
// ===================================================
void drawWin() {
    beginOrtho();
    glColor3f(0.0f, 0.04f, 0.02f);
    glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(800,0); glVertex2f(800,600); glVertex2f(0,600); glEnd();

    if (wonWithTreasure) {
        drawTextLarge(165, 420, "YOU ESCAPED WITH TREASURE!", 1.0f, 0.85f, 0.0f);
        char buf[64];
        sprintf(buf, "FINAL SCORE: %d", finalScore);
        drawTextLarge(255, 355, buf, 0.3f, 1.0f, 0.3f);
    } else {
        drawTextLarge(155, 420, "You escaped... but left the treasure!", 0.55f, 0.8f, 0.55f);
        drawTextLarge(275, 355, "SCORE: 0", 0.45f, 0.65f, 0.45f);
        drawText2D(210, 310, "Find the treasure first for a real score!", 0.7f, 0.7f, 0.7f);
    }
    drawTextLarge(220, 240, "R - Retry   M - Menu", 0.7f, 0.7f, 0.7f);
    endOrtho();
}

// ===================================================
// DISPLAY
// ===================================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    if (gameState == STATE_MENU) {
        drawMenu(); glutSwapBuffers(); return;
    }
    if (gameState == STATE_GAMEOVER) {
        drawGameOver(); glutSwapBuffers(); return;
    }
    if (gameState == STATE_WIN) {
        drawWin(); glutSwapBuffers(); return;
    }

    // -- STATE_PLAYING --
    if (lightsOn) {
        glClearColor(0.55f, 0.52f, 0.45f, 1);
    } else {
        // Very dark, slightly blue-tinged — oppressive
        float fog = 0.03f * flickerIntensity;
        glClearColor(fog, fog, fog * 1.4f, 1);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    updateCamera();

    // Fog for atmosphere
    if (!lightsOn) {
        GLfloat fogColor[4] = {0.02f, 0.02f, 0.05f, 1.0f};
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogf(GL_FOG_START, 4.0f);
        glFogf(GL_FOG_END, 16.0f);
    } else {
        GLfloat fogColor[4] = {0.55f, 0.52f, 0.45f, 1.0f};
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogf(GL_FOG_START, 12.0f);
        glFogf(GL_FOG_END, 30.0f);
    }

    drawFloor();
    drawMaze();
    drawHidingSpots();
    drawExit();
    drawTreasure();
    drawEnemies();

    glDisable(GL_FOG);

    drawGPS();

    glutSwapBuffers();
}

// ===================================================
// RESHAPE
// ===================================================
void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(65.0, (double)w / h, 0.3, 200.0);
    glMatrixMode(GL_MODELVIEW);
}

// ===================================================
// INIT
// ===================================================
void init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.03f, 0.03f, 0.06f, 1);
    glShadeModel(GL_SMOOTH);
}

// ===================================================
// MAIN
// ===================================================
int main(int argc, char **argv) {
    srand((unsigned)time(NULL));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Maze Escape - Find Treasure & Survive");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutPassiveMotionFunc(mouseMotion);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}