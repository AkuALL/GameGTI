#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <algorithm>

// ================= CONSTANTS =================
#define MAP_MIN -9.0f
#define MAP_MAX  9.0f
#define M_PI 3.14159265f

// ================= GAME STATE =================
enum GameState { STATE_MENU, STATE_PLAYING, STATE_GAMEOVER, STATE_WIN };
GameState gameState = STATE_MENU;

// ================= PLAYER =================
float playerX = 0.0f, playerZ = 0.0f;
float playerAngle = 0.0f;
float speed = 0.3f;
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

// Outer boundary + interior walls forming a maze with exactly 1 exit
// Exit is at right wall (x=9) around z=-7 -> represented as gap
Wall walls[] = {
    // Outer boundary (4 segments, leaving exit gap at right side near z=-7)
    {0,  -9,  18, 1},    // bottom wall
    {0,   9,  18, 1},    // top wall
    {-9,  0,   1, 18},   // left wall (full)
    // Right wall split: top segment and bottom segment, gap at z ~ -7 to -5
    { 9,  3.5f, 1, 11},  // right wall top portion (z from -2 to 9)
    { 9, -8.0f, 1,  2},  // right wall bottom small (z from -9 to -7)
    // Interior walls
    {-4,  0,   1,  8},   // vertical divider left
    { 3,  3,   6,  1},   // horizontal mid-top
    { 2, -4,   1,  6},   // vertical mid
    {-2,  5,   4,  1},   // short horizontal top-left area
    { 5, -6,   4,  1},   // short horizontal bottom-right
    {-6, -5,   1,  4},   // small vertical left-bottom
    { 1,  7,   1,  4},   // small vertical top-middle
};
int wallCount = 12;

// ================= EXIT =================
// The exit is the gap in the right wall near z=-6 to z=-7
float exitX = 9.5f, exitZ = -6.0f;

// ================= TREASURE =================
float treasureX, treasureZ;

// ================= HIDING SPOTS =================
struct HidingSpot { float x, z; };
HidingSpot hidingSpots[] = {
    {-7, -7},
    {-7,  7},
    { 7,  7},
    {-5,  3},
    { 5, -2},
    { 0,  6},
    {-2, -6},
};
int hidingCount = 7;

// ================= ENEMIES =================
struct Enemy {
    float x, z;
    float angle;      // current heading (degrees)
    float speed;
    // patrol: waypoint index
    int waypointIdx;
    // simple path: list of waypoints
    float wpX[6], wpZ[6];
    int wpCount;
};

Enemy enemies[4];

// ================= LIGHT CYCLE =================
float gameTimer = 0.0f;       // total seconds elapsed
float lightCycleInterval = 60.0f; // every 60s lights come on
float lightOnDuration = 10.0f;
bool lightsOn = false;
float lightsOnTimer = 0.0f;
float lastTime = 0.0f;

// ================= SCORE =================
float startTime = 0.0f;
int finalScore = 0;
bool wonWithTreasure = false;

// ================= FORWARD DECLARATIONS =================
bool checkCollision(float newX, float newZ);
void resetGame();

// ===================================================
// SPAWN HELPERS
// ===================================================
bool positionValid(float x, float z, float margin = 0.6f) {
    // check inside map
    if (x < MAP_MIN + 1 || x > MAP_MAX - 1 || z < MAP_MIN + 1 || z > MAP_MAX - 1)
        return false;
    // check wall collision
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
        x = (float)(rand() % 160 - 80) / 10.0f; // -8 to 8
        z = (float)(rand() % 160 - 80) / 10.0f;
        tries++;
    } while (!positionValid(x, z) && tries < 500);
}

// ===================================================
// ENEMY INIT
// ===================================================
void initEnemies() {
    // Predefined patrol routes (looping waypoints)
    // Enemy 0: top-left quadrant
    enemies[0] = {-6.0f, 6.0f, 0, 0.05f, 0, {}, {}, 4};
    enemies[0].wpX[0]=-6; enemies[0].wpZ[0]=6;
    enemies[0].wpX[1]=-2; enemies[0].wpZ[1]=6;
    enemies[0].wpX[2]=-2; enemies[0].wpZ[2]=2;
    enemies[0].wpX[3]=-6; enemies[0].wpZ[3]=2;

    // Enemy 1: top-right quadrant
    enemies[1] = {4.0f, 6.0f, 0, 0.05f, 0, {}, {}, 4};
    enemies[1].wpX[0]=4; enemies[1].wpZ[0]=6;
    enemies[1].wpX[1]=7; enemies[1].wpZ[1]=6;
    enemies[1].wpX[2]=7; enemies[1].wpZ[2]=4;
    enemies[1].wpX[3]=4; enemies[1].wpZ[3]=4;

    // Enemy 2: bottom-left quadrant
    enemies[2] = {-7.0f, -3.0f, 0, 0.05f, 0, {}, {}, 4};
    enemies[2].wpX[0]=-7; enemies[2].wpZ[0]=-3;
    enemies[2].wpX[1]=-5; enemies[2].wpZ[1]=-3;
    enemies[2].wpX[2]=-5; enemies[2].wpZ[2]=-7;
    enemies[2].wpX[3]=-7; enemies[2].wpZ[3]=-7;

    // Enemy 3: center / right area
    enemies[3] = {5.0f, -1.0f, 0, 0.05f, 0, {}, {}, 4};
    enemies[3].wpX[0]=5; enemies[3].wpZ[0]=-1;
    enemies[3].wpX[1]=7; enemies[3].wpZ[1]=-1;
    enemies[3].wpX[2]=7; enemies[3].wpZ[2]=-5;
    enemies[3].wpX[3]=5; enemies[3].wpZ[3]=-5;
}

// ===================================================
// COLLISION
// ===================================================
bool checkCollision(float newX, float newZ) {
    float playerSize = 0.4f;
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

    // Random spawn for player
    randomPos(playerX, playerZ);

    // Random spawn for treasure (not too close to player)
    float tx, tz;
    do { randomPos(tx, tz); }
    while (fabs(tx - playerX) < 2.0f && fabs(tz - playerZ) < 2.0f);
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
    float radius = 1.2f;
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
        if (sqrtf(dx*dx + dz*dz) < 0.8f) {
            playerHasTreasure = true;
        }
    }
}

// ===================================================
// CHECK EXIT
// ===================================================
void checkExit() {
    // Exit is at x > 8.5 and z in [-8, -5]
    if (playerX > 8.5f && playerZ > -8.0f && playerZ < -5.0f) {
        wonWithTreasure = playerHasTreasure;
        float elapsed = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f - startTime;
        if (wonWithTreasure) {
            // Higher score for faster completion
            finalScore = (int)(10000.0f / (elapsed + 1.0f)) * 10;
        } else {
            finalScore = 0;
        }
        gameState = STATE_WIN;
    }
}

// ===================================================
// ENEMY UPDATE
// ===================================================
void updateEnemies(float dt) {
    for (int i = 0; i < 4; i++) {
        Enemy &e = enemies[i];
        int next = (e.waypointIdx + 1) % e.wpCount;
        float tx = e.wpX[next], tz = e.wpZ[next];
        float dx = tx - e.x, dz = tz - e.z;
        float dist = sqrtf(dx*dx + dz*dz);
        if (dist < 0.15f) {
            e.waypointIdx = next;
            continue;
        }
        float moveSpeed = e.speed * (lightsOn ? 2.0f : 1.0f); // faster when lights on
        e.x += (dx / dist) * moveSpeed;
        e.z += (dz / dist) * moveSpeed;
        e.angle = atan2f(dx, dz) * 180.0f / (float)M_PI;

        // Check enemy-player collision
        float pdx = e.x - playerX, pdz = e.z - playerZ;
        if (sqrtf(pdx*pdx + pdz*pdz) < 0.8f) {
            if (!playerIsHiding) {
                // Caught!
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
        // Every lightCycleInterval seconds, lights come on
        if (gameTimer >= lightCycleInterval) {
            lightsOn = true;
            lightsOnTimer = 0.0f;
            gameTimer = 0.0f;
            // If player not hiding -> game over
            if (!playerIsHiding) {
                finalScore = 0;
                if (playerHasTreasure) {
                    float elapsed = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f - startTime;
                    // Survived with treasure but not exited: score based on survival time
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
// TIMER / UPDATE
// ===================================================
void update(int value) {
    if (gameState == STATE_PLAYING) {
        float now = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        float dt = now - lastTime;
        lastTime = now;

        updateLights(dt);
        if (gameState != STATE_PLAYING) { glutPostRedisplay(); return; }
        updateEnemies(dt);
        if (gameState != STATE_PLAYING) { glutPostRedisplay(); return; }
        checkTreasurePickup();
        checkExit();
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, 0); // ~60fps
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
        if (key == 13 || key == ' ') resetGame(); // Enter or Space
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
            // Toggle hiding if near a hiding spot
            if (isNearHidingSpot()) playerIsHiding = !playerIsHiding;
            else playerIsHiding = false;
            break;
        case 27: exit(0); break; // ESC
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
void drawFloor() {
    if (lightsOn)
        glColor3f(0.85f, 0.85f, 0.8f);
    else
        glColor3f(0.25f, 0.25f, 0.28f);
    glBegin(GL_QUADS);
    glVertex3f(-10, 0, -10);
    glVertex3f(-10, 0,  10);
    glVertex3f( 10, 0,  10);
    glVertex3f( 10, 0, -10);
    glEnd();
}

void drawWall(float x, float z, float sx, float sz) {
    glPushMatrix();
    glTranslatef(x, 1.0f, z);
    glScalef(sx, 2.0f, sz);
    if (lightsOn)
        glColor3f(0.5f, 0.6f, 1.0f);
    else
        glColor3f(0.1f, 0.15f, 0.45f);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawMaze() {
    for (int i = 0; i < wallCount; i++)
        drawWall(walls[i].x, walls[i].z, walls[i].sx, walls[i].sz);
}

// Draw exit marker
void drawExit() {
    glPushMatrix();
    glTranslatef(9.0f, 0.05f, -6.0f);
    glColor3f(0.0f, 1.0f, 0.2f);
    glBegin(GL_QUADS);
    glVertex3f(-0.4f, 0, -1.5f);
    glVertex3f( 0.4f, 0, -1.5f);
    glVertex3f( 0.4f, 0,  1.5f);
    glVertex3f(-0.4f, 0,  1.5f);
    glEnd();
    // Arrow pointing right
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex3f(0.6f, 0.1f, -6.0f + exitZ - exitZ); // just a flag marker
    glEnd();
    glPopMatrix();
}

// Draw hiding spots
void drawHidingSpots() {
    for (int i = 0; i < hidingCount; i++) {
        glPushMatrix();
        glTranslatef(hidingSpots[i].x, 0.05f, hidingSpots[i].z);
        if (lightsOn)
            glColor3f(0.8f, 0.8f, 0.0f);
        else
            glColor3f(0.4f, 0.4f, 0.0f);
        glBegin(GL_QUADS);
        glVertex3f(-0.6f, 0, -0.6f);
        glVertex3f( 0.6f, 0, -0.6f);
        glVertex3f( 0.6f, 0,  0.6f);
        glVertex3f(-0.6f, 0,  0.6f);
        glEnd();
        glPopMatrix();
    }
}

// Draw treasure
void drawTreasure() {
    if (playerHasTreasure) return;
    glPushMatrix();
    glTranslatef(treasureX, 0.5f, treasureZ);
    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 500.0f;
    glTranslatef(0, sinf(t) * 0.15f, 0);
    glColor3f(1.0f, 0.85f, 0.0f);
    glutSolidCube(0.4f);
    glPopMatrix();
}

// Draw player
void drawPlayer() {
    glPushMatrix();
    glTranslatef(playerX, 0.5f, playerZ);
    glRotatef(playerAngle, 0, 1, 0);
    if (playerIsHiding)
        glColor3f(0.3f, 0.3f, 0.3f);
    else
        glColor3f(1.0f, 0.2f, 0.2f);
    glutSolidCube(0.7f);
    // Nose (direction indicator)
    glTranslatef(0, 0, -0.45f);
    glColor3f(1.0f, 1.0f, 0.0f);
    glutSolidSphere(0.15f, 8, 8);
    glPopMatrix();
}

// Draw enemies
void drawEnemies() {
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
        glTranslatef(enemies[i].x, 0.6f, enemies[i].z);
        glRotatef(enemies[i].angle, 0, 1, 0);
        glColor3f(1.0f, 0.0f, 0.5f);
        glutSolidCube(0.7f);
        // Eyes
        glPushMatrix();
        glTranslatef(0.2f, 0.1f, -0.4f);
        glColor3f(1, 1, 0);
        glutSolidSphere(0.1f, 6, 6);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(-0.2f, 0.1f, -0.4f);
        glColor3f(1, 1, 0);
        glutSolidSphere(0.1f, 6, 6);
        glPopMatrix();
        glPopMatrix();
    }
}

// ===================================================
// CAMERA
// ===================================================
void updateCamera() {
    float rad = playerAngle * (float)M_PI / 180.0f;
    float lookX = playerX + sinf(rad) * lookDistance;
    float lookZ = playerZ + cosf(rad) * lookDistance;

    gluLookAt(playerX, eyeHeight, playerZ,
              lookX, eyeHeight, lookZ,
              0, 1, 0);
}

// ===================================================
// TEXT RENDERING (bitmap)
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

// Draw a single arrow in GPS box pointing to target
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

    // ----- GPS Box 1: Treasure Arrow -----
    float box1X = 20, box1Y = 490, boxSize = 80;
    // Box background
    glColor3f(0.1f, 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(box1X, box1Y);
    glVertex2f(box1X + boxSize, box1Y);
    glVertex2f(box1X + boxSize, box1Y + boxSize);
    glVertex2f(box1X, box1Y + boxSize);
    glEnd();
    // Border
    glColor3f(0.6f, 0.6f, 0.6f);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2f(box1X, box1Y);
    glVertex2f(box1X + boxSize, box1Y);
    glVertex2f(box1X + boxSize, box1Y + boxSize);
    glVertex2f(box1X, box1Y + boxSize);
    glEnd();

    if (!playerHasTreasure) {
        drawGPSArrow(box1X + boxSize/2, box1Y + boxSize/2, treasureX, treasureZ, 1.0f, 0.85f, 0.0f);
    } else {
        // Show checkmark
        glColor3f(0.0f, 1.0f, 0.3f);
        glLineWidth(3);
        glBegin(GL_LINE_STRIP);
        glVertex2f(box1X + 20, box1Y + 35);
        glVertex2f(box1X + 35, box1Y + 20);
        glVertex2f(box1X + 60, box1Y + 55);
        glEnd();
    }
    drawText2D(box1X + 5, box1Y - 18, playerHasTreasure ? "GOT IT!" : "TREASURE", 1.0f, 0.85f, 0.0f);

    // ----- GPS Box 2: Exit Arrow -----
    float box2X = 110, box2Y = 490;
    glColor3f(0.1f, 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(box2X, box2Y);
    glVertex2f(box2X + boxSize, box2Y);
    glVertex2f(box2X + boxSize, box2Y + boxSize);
    glVertex2f(box2X, box2Y + boxSize);
    glEnd();
    glColor3f(0.6f, 0.6f, 0.6f);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2f(box2X, box2Y);
    glVertex2f(box2X + boxSize, box2Y);
    glVertex2f(box2X + boxSize, box2Y + boxSize);
    glVertex2f(box2X, box2Y + boxSize);
    glEnd();
    drawGPSArrow(box2X + boxSize/2, box2Y + boxSize/2, exitX, exitZ, 0.0f, 1.0f, 0.3f);
    drawText2D(box2X + 18, box2Y - 18, "EXIT", 0.0f, 1.0f, 0.3f);

    // ----- HUD: Timer -----
    char buf[64];

    // Light cycle countdown
    float timeToLight = lightsOn ? (lightOnDuration - lightsOnTimer) : (lightCycleInterval - gameTimer);
    if (lightsOn) {
        sprintf(buf, "LIGHTS OFF IN: %.1fs", timeToLight);
        drawText2D(300, 570, buf, 1.0f, 0.3f, 0.3f);
    } else {
        sprintf(buf, "LIGHTS ON IN: %.0fs", timeToLight);
        drawText2D(300, 570, buf, 0.9f, 0.9f, 0.9f);
    }

    // Total elapsed
    float elapsed = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f - startTime;
    sprintf(buf, "TIME: %.0fs", elapsed);
    drawText2D(650, 570, buf, 0.8f, 0.8f, 0.8f);

    // Treasure status
    if (playerHasTreasure)
        drawText2D(330, 540, "TREASURE: COLLECTED!", 1.0f, 0.85f, 0.0f);
    else
        drawText2D(330, 540, "TREASURE: NOT YET", 0.6f, 0.6f, 0.6f);

    // Hiding status
    if (playerIsHiding)
        drawText2D(330, 510, "HIDING", 0.3f, 1.0f, 0.3f);
    else if (isNearHidingSpot())
        drawText2D(330, 510, "Press H to Hide", 0.9f, 0.9f, 0.3f);

    // Light ON overlay warning
    if (lightsOn) {
        glColor4f(1.0f, 0.9f, 0.0f, 0.08f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(800, 0);
        glVertex2f(800, 600);
        glVertex2f(0, 600);
        glEnd();
        glDisable(GL_BLEND);

        if (!playerIsHiding) {
            drawTextLarge(230, 470, "!! LIGHTS ON - HIDE NOW !!", 1.0f, 0.1f, 0.1f);
        } else {
            drawTextLarge(280, 470, "STAY HIDDEN!", 0.2f, 1.0f, 0.2f);
        }
    }

    // Controls reminder (small)
    drawText2D(20, 20, "WASD/Arrows:Move  H:Hide  Mouse:Turn", 0.5f, 0.5f, 0.5f);

    endOrtho();
}

// ===================================================
// MENU SCREEN
// ===================================================
void drawMenu() {
    beginOrtho();

    // Background
    glColor3f(0.05f, 0.05f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(800,0); glVertex2f(800,600); glVertex2f(0,600);
    glEnd();

    drawTextLarge(230, 480, "MAZE ESCAPE", 1.0f, 0.85f, 0.0f);
    drawTextLarge(200, 440, "Find the treasure, then escape!", 0.9f, 0.9f, 0.9f);

    drawText2D(200, 380, "--- HOW TO PLAY ---", 0.8f, 0.8f, 1.0f);
    drawText2D(180, 350, "WASD / Arrow Keys : Move", 0.9f, 0.9f, 0.9f);
    drawText2D(180, 325, "Mouse             : Look / Turn", 0.9f, 0.9f, 0.9f);
    drawText2D(180, 300, "H                 : Hide (near yellow spots)", 0.9f, 0.9f, 0.9f);
    drawText2D(180, 275, "Gold GPS box      : Arrow to Treasure", 1.0f, 0.85f, 0.0f);
    drawText2D(180, 250, "Green GPS box     : Arrow to Exit", 0.0f, 1.0f, 0.3f);

    drawText2D(200, 200, "--- RULES ---", 0.8f, 0.8f, 1.0f);
    drawText2D(180, 175, "Every 60s lights come ON for 10s!", 1.0f, 0.3f, 0.3f);
    drawText2D(180, 150, "Be in a yellow hiding spot or GAME OVER!", 1.0f, 0.3f, 0.3f);
    drawText2D(180, 125, "Avoid the pink enemies!", 1.0f, 0.5f, 0.7f);
    drawText2D(180, 100, "Exit with treasure = best score!", 1.0f, 0.85f, 0.0f);

    drawTextLarge(260, 50, "Press ENTER or SPACE to Start", 0.3f, 1.0f, 0.3f);

    endOrtho();
}

// ===================================================
// GAME OVER SCREEN
// ===================================================
void drawGameOver() {
    beginOrtho();
    glColor3f(0.05f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(800,0); glVertex2f(800,600); glVertex2f(0,600);
    glEnd();

    drawTextLarge(270, 400, "GAME OVER", 1.0f, 0.1f, 0.1f);

    char buf[64];
    if (wonWithTreasure) {
        drawText2D(220, 340, "You had the treasure but didn't make it...", 1.0f, 0.85f, 0.0f);
        sprintf(buf, "Survival Score: %d", finalScore);
        drawText2D(300, 300, buf, 1.0f, 0.85f, 0.0f);
    } else {
        drawText2D(260, 340, "Caught without the treasure!", 0.9f, 0.5f, 0.5f);
        drawText2D(300, 300, "Score: 0", 0.7f, 0.7f, 0.7f);
    }

    drawTextLarge(250, 220, "Press R to Return to Menu", 0.8f, 0.8f, 0.8f);
    endOrtho();
}

// ===================================================
// WIN SCREEN
// ===================================================
void drawWin() {
    beginOrtho();
    glColor3f(0.0f, 0.05f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(800,0); glVertex2f(800,600); glVertex2f(0,600);
    glEnd();

    if (wonWithTreasure) {
        drawTextLarge(200, 420, "YOU ESCAPED WITH TREASURE!", 1.0f, 0.85f, 0.0f);
        char buf[64];
        sprintf(buf, "FINAL SCORE: %d", finalScore);
        drawTextLarge(270, 360, buf, 0.3f, 1.0f, 0.3f);
    } else {
        drawTextLarge(180, 420, "You escaped... but left the treasure!", 0.6f, 0.8f, 0.6f);
        drawTextLarge(280, 360, "SCORE: 0", 0.5f, 0.7f, 0.5f);
        drawText2D(230, 320, "Find the treasure first for a real score!", 0.8f, 0.8f, 0.8f);
    }

    drawTextLarge(250, 260, "Press R to Return to Menu", 0.8f, 0.8f, 0.8f);
    endOrtho();
}

// ===================================================
// DISPLAY
// ===================================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    if (gameState == STATE_MENU) {
        drawMenu();
        glutSwapBuffers();
        return;
    }
    if (gameState == STATE_GAMEOVER) {
        drawGameOver();
        glutSwapBuffers();
        return;
    }
    if (gameState == STATE_WIN) {
        drawWin();
        glutSwapBuffers();
        return;
    }

    // -- STATE_PLAYING --
    updateCamera();

    // Fog / ambient light: dim vs bright
    if (lightsOn) {
        glClearColor(0.6f, 0.6f, 0.5f, 1);
    } else {
        glClearColor(0.05f, 0.05f, 0.08f, 1);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    updateCamera();

    drawFloor();
    drawMaze();
    drawHidingSpots();
    drawExit();
    drawTreasure();
    // First-person camera sits inside the player, so do not render the player body.
    drawEnemies();

    // HUD
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
    gluPerspective(60.0, (double)w / h, 0.5, 200.0);
    glMatrixMode(GL_MODELVIEW);
}

// ===================================================
// INIT
// ===================================================
void init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.08f, 1);
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
