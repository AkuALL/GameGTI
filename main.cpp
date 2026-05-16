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
float eyeHeight = 1.5f;
float lookDistance = 6.0f;

// ================= MOUSE =================
int lastMouseX = 400;
float mouseSensitivity = 0.2f;

// ================= WALLS =================
struct Wall { float x, z, sx, sz; };

Wall walls[] = {
    // Outer boundary
    { 0,  -14,  28,  0.6f},   // 0: south wall
    { 0,   14,  28,  0.6f},   // 1: north wall
    {-14,   0,  0.6f, 28},    // 2: west wall
    { 14,   3.0f, 0.6f, 22},  // 3: east wall upper (gap at z=-8 to -12 = exit)
    { 14, -12.5f, 0.6f,  3},  // 4: east wall lower sliver

    // Horizontal walls
    {-5,   10,  18,  0.5f},   // 5
    { 4,    6,  12,  0.5f},   // 6
    {-8,    6,   8,  0.5f},   // 7
    {-4,    2,  12,  0.5f},   // 8
    { 6,   -2,  10,  0.5f},   // 9
    {-8,   -2,   8,  0.5f},   // 10
    { 2,   -6,  10,  0.5f},   // 11
    {-6,  -10,  10,  0.5f},   // 12
    { 5,  -10,   6,  0.5f},   // 13

    // Vertical walls
    {-10,   8,  0.5f, 12},    // 14
    { -2,   8,  0.5f,  8},    // 15
    {  6,   4,  0.5f, 12},    // 16
    { 10,   8,  0.5f,  8},    // 17
    {-10,  -4,  0.5f,  8},    // 18
    {  2,  -2,  0.5f,  8},    // 19
    { 10,  -4,  0.5f,  8},    // 20
    {-6,  -12,  0.5f,  4},    // 21
    { 6,  -12,  0.5f,  4},    // 22
};
int wallCount = 23;

// ================= EXIT =================
float exitX = 14.5f, exitZ = -10.0f;

// ================= TREASURE =================
float treasureX, treasureZ;

// ================= HIDING SPOTS =================
struct HidingSpot { float x, z; };
HidingSpot hidingSpots[] = {
    {-12.5f, -12.5f},
    {-12.5f,  12.5f},
    { 12.5f,  12.5f},
    { 12.5f,   8.0f},
    {-13.0f,   4.0f},
    {-13.0f,  -7.0f},
    {  0.5f,  12.5f},
    { -6.5f,  -1.5f},
    {  4.5f,   7.5f},
    { -4.0f, -12.5f},
    {  9.5f,  -1.5f},
    { -1.5f,   2.0f},
};
int hidingCount = 12;

// ================= ENEMIES =================
struct Enemy {
    float x, z;
    float angle;
    float speed;       // units per frame (seperti kode kedua)
    int   waypointIdx;
    float wpX[8], wpZ[8];
    int   wpCount;
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
float breathTimer = 0.0f;

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
    // speed = units per frame (sama seperti kode kedua: 0.05f normal)

    // ── Enemy 0: patrol top-left quadrant ──
    enemies[0].x = -12.0f; enemies[0].z = 12.0f;
    enemies[0].angle = 0; enemies[0].speed = 0.05f;
    enemies[0].waypointIdx = 0; enemies[0].wpCount = 4;
    enemies[0].wpX[0]=-12.0f; enemies[0].wpZ[0]= 12.0f;
    enemies[0].wpX[1]=-12.0f; enemies[0].wpZ[1]=  7.5f;
    enemies[0].wpX[2]= -6.0f; enemies[0].wpZ[2]=  7.5f;
    enemies[0].wpX[3]= -6.0f; enemies[0].wpZ[3]= 12.0f;

    // ── Enemy 1: patrol top-right quadrant ──
    enemies[1].x = 11.5f; enemies[1].z = 12.0f;
    enemies[1].angle = 0; enemies[1].speed = 0.05f;
    enemies[1].waypointIdx = 0; enemies[1].wpCount = 4;
    enemies[1].wpX[0]= 11.5f; enemies[1].wpZ[0]= 12.0f;
    enemies[1].wpX[1]= 11.5f; enemies[1].wpZ[1]=  7.0f;
    enemies[1].wpX[2]=  8.0f; enemies[1].wpZ[2]=  7.0f;
    enemies[1].wpX[3]=  8.0f; enemies[1].wpZ[3]= 12.0f;

    // ── Enemy 2: patrol center-left ──
    enemies[2].x = -12.0f; enemies[2].z = -5.5f;
    enemies[2].angle = 0; enemies[2].speed = 0.05f;
    enemies[2].waypointIdx = 0; enemies[2].wpCount = 4;
    enemies[2].wpX[0]=-12.0f; enemies[2].wpZ[0]= -5.5f;
    enemies[2].wpX[1]=-12.0f; enemies[2].wpZ[1]= -9.5f;
    enemies[2].wpX[2]= -5.0f; enemies[2].wpZ[2]= -9.5f;
    enemies[2].wpX[3]= -5.0f; enemies[2].wpZ[3]= -5.5f;

    // ── Enemy 3: patrol center ──
    enemies[3].x = 4.0f; enemies[3].z = -3.5f;
    enemies[3].angle = 0; enemies[3].speed = 0.06f;
    enemies[3].waypointIdx = 0; enemies[3].wpCount = 4;
    enemies[3].wpX[0]= 4.0f; enemies[3].wpZ[0]= -3.0f;
    enemies[3].wpX[1]= 8.5f; enemies[3].wpZ[1]= -3.0f;
    enemies[3].wpX[2]= 8.5f; enemies[3].wpZ[2]= -5.5f;
    enemies[3].wpX[3]= 4.0f; enemies[3].wpZ[3]= -5.5f;

    // ── Enemy 4: patrol bottom-right ──
    enemies[4].x = 11.5f; enemies[4].z = -6.0f;
    enemies[4].angle = 0; enemies[4].speed = 0.055f;
    enemies[4].waypointIdx = 0; enemies[4].wpCount = 4;
    enemies[4].wpX[0]= 11.5f; enemies[4].wpZ[0]= -6.0f;
    enemies[4].wpX[1]= 11.5f; enemies[4].wpZ[1]=-13.0f;
    enemies[4].wpX[2]=  8.5f; enemies[4].wpZ[2]=-13.0f;
    enemies[4].wpX[3]=  8.5f; enemies[4].wpZ[3]= -6.0f;
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
    float radius = 1.1f;
    for (int i = 0; i < hidingCount; i++) {
        float dx = playerX - hidingSpots[i].x;
        float dz = playerZ - hidingSpots[i].z;
        if (sqrtf(dx*dx + dz*dz) < radius) return true;
    }
    return false;
}

void checkTreasurePickup() {
    if (!playerHasTreasure) {
        float dx = playerX - treasureX, dz = playerZ - treasureZ;
        if (sqrtf(dx*dx + dz*dz) < 1.0f) playerHasTreasure = true;
    }
}

void checkExit() {
    if (playerX > 13.5f && playerZ > -12.0f && playerZ < -8.0f) {
        wonWithTreasure = playerHasTreasure;
        float elapsed = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f - startTime;
        finalScore = wonWithTreasure ? (int)(10000.0f / (elapsed + 1.0f)) * 10 : 0;
        gameState = STATE_WIN;
    }
}

// ===================================================
// ENEMY UPDATE — pola sederhana dari kode kedua
// (langsung += per frame, bukan dt-based)
// ===================================================
void updateEnemies(float dt) {
    for (int i = 0; i < 5; i++) {
        Enemy &e = enemies[i];

        int next = (e.waypointIdx + 1) % e.wpCount;
        float tx = e.wpX[next];
        float tz = e.wpZ[next];

        float dx = tx - e.x;
        float dz = tz - e.z;
        float dist = sqrtf(dx*dx + dz*dz);

        // Sudah sampai waypoint → pindah ke berikutnya
        if (dist < 0.15f) {
            e.waypointIdx = next;
            continue;
        }

        // Kecepatan: saat lights on 2x lebih cepat (sama seperti kode kedua)
        float moveSpeed = e.speed * (lightsOn ? 2.0f : 1.0f);

        // Gerak langsung per frame (pola kode kedua)
        e.x += (dx / dist) * moveSpeed;
        e.z += (dz / dist) * moveSpeed;
        e.angle = atan2f(dx, dz) * 180.0f / (float)M_PI;

        // Cek tabrakan dengan player
        float pdx = e.x - playerX, pdz = e.z - playerZ;
        if (sqrtf(pdx*pdx + pdz*pdz) < 0.85f && !playerIsHiding) {
            finalScore = 0; wonWithTreasure = false;
            gameState = STATE_GAMEOVER;
        }
    }
}

// ===================================================
// LIGHT CYCLE
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
// INPUT
// ===================================================
void mouseMotion(int x, int y) {
    if (gameState != STATE_PLAYING) return;
    int deltaX = x - lastMouseX;
    lastMouseX = x;
    playerAngle += deltaX * mouseSensitivity;
    if (playerAngle >= 360.0f) playerAngle -= 360.0f;
    if (playerAngle <    0.0f) playerAngle += 360.0f;
    glutPostRedisplay();
}

void movePlayer(float forward, float strafe) {
    float rad = playerAngle * (float)M_PI / 180.0f;
    float newX = playerX + sinf(rad)*forward + cosf(rad)*strafe;
    float newZ = playerZ + cosf(rad)*forward - sinf(rad)*strafe;
    if (!checkCollision(newX, newZ)) { playerX = newX; playerZ = newZ; }
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
        case 'w': case 'W': movePlayer( speed,  0); break;
        case 's': case 'S': movePlayer(-speed,  0); break;
        case 'd': case 'D': movePlayer(0, -speed); break;
        case 'a': case 'A': movePlayer(0,  speed); break;
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
        case GLUT_KEY_UP:    movePlayer( speed, 0); break;
        case GLUT_KEY_DOWN:  movePlayer(-speed, 0); break;
        case GLUT_KEY_LEFT:  movePlayer(0,  speed); break;
        case GLUT_KEY_RIGHT: movePlayer(0, -speed); break;
    }
    glutPostRedisplay();
}

// ===================================================
// RENDERING HELPERS (semua tidak diubah dari kode pertama)
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

void drawWall(float x,float z,float sx,float sz) {
    float wallH=3.5f,wainH=wallH*0.35f;
    auto col=[&](float lr,float lg,float lb,float dr,float dg,float db){
        if(lightsOn)glColor3f(lr,lg,lb); else glColor3f(dr*flickerIntensity,dg*flickerIntensity,db*flickerIntensity);
    };
    glPushMatrix();glTranslatef(x,wallH*0.5f,z);glScalef(sx,wallH,sz);
    col(0.30f,0.28f,0.25f, 0.05f,0.04f,0.06f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(x,wainH*0.5f,z);glScalef(sx+0.015f,wainH,sz+0.015f);
    col(0.22f,0.20f,0.18f, 0.07f,0.06f,0.08f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(x,wainH,z);glScalef(sx+0.02f,0.07f,sz+0.02f);
    col(0.55f,0.50f,0.42f, 0.15f,0.13f,0.10f);glutSolidCube(1);glPopMatrix();
    float panelBot=wainH+0.07f,panelH=wallH-0.12f-panelBot;
    glPushMatrix();glTranslatef(x,panelBot+panelH*0.5f,z);glScalef(sx+0.013f,panelH,sz+0.013f);
    col(0.72f,0.68f,0.60f, 0.10f,0.09f,0.12f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(x,wallH-0.06f,z);glScalef(sx+0.025f,0.12f,sz+0.025f);
    col(0.60f,0.56f,0.48f, 0.12f,0.10f,0.08f);glutSolidCube(1);glPopMatrix();
}

void drawMaze() {
    for (int i=0;i<wallCount;i++) drawWall(walls[i].x,walls[i].z,walls[i].sx,walls[i].sz);
}

void drawExit() {
    float t=(float)glutGet(GLUT_ELAPSED_TIME)/800.0f;
    float pulse=0.6f+0.4f*sinf(t*3.0f);
    glPushMatrix();glTranslatef(13.5f,0.05f,exitZ);
    glColor3f(0,pulse,0.2f*pulse);
    glBegin(GL_QUADS);
    glVertex3f(-0.5f,0,-2);glVertex3f(0.5f,0,-2);glVertex3f(0.5f,0,2);glVertex3f(-0.5f,0,2);
    glEnd();
    glColor3f(0,pulse*0.8f,0);
    glPushMatrix();glTranslatef(0.6f,1.75f,0);glScalef(0.2f,3.5f,4);glutSolidCube(1);glPopMatrix();
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
    if(playerHasTreasure)return;
    float t=(float)glutGet(GLUT_ELAPSED_TIME)/600.0f;
    float hover=sinf(t*1.5f)*0.08f;
    float gp=0.3f+0.2f*sinf(t*3);
    glColor3f(gp,gp*0.7f,0);
    glPushMatrix();glTranslatef(treasureX,0.02f,treasureZ);
    glBegin(GL_TRIANGLE_FAN);glVertex3f(0,0,0);
    for(int s=0;s<=20;s++){float a=s*2*(float)M_PI/20,r=0.8f+0.15f*sinf(t*4+s);glVertex3f(cosf(a)*r,0,sinf(a)*r);}
    glEnd();glPopMatrix();
    glPushMatrix();glTranslatef(treasureX,hover,treasureZ);
    float gR=lightsOn?1.0f:0.85f*flickerIntensity,gG=lightsOn?0.78f:0.65f*flickerIntensity;
    glColor3f(gR,gG,0);
    drawCoin(0.15f,0.04f,0.10f,0.18f,0.06f);drawCoin(-0.20f,0.04f,0.05f,0.16f,0.06f);
    drawCoin(0.00f,0.04f,-0.20f,0.17f,0.06f);drawCoin(0.25f,0.04f,-0.05f,0.15f,0.06f);
    drawCoin(-0.10f,0.04f,-0.15f,0.18f,0.06f);
    for(int k=0;k<5;k++) drawCoin(0,0.10f+k*0.08f,0,0.22f-k*0.01f,0.08f);
    float gl=0.5f+0.5f*sinf(t*5);glColor3f(1,1,gl*0.5f);
    glPushMatrix();glTranslatef(0,0.55f,0);glutSolidSphere(0.06f,8,8);glPopMatrix();
    glPopMatrix();
}

void drawEnemyBody(int i) {
    Enemy &e=enemies[i];
    float t=(float)glutGet(GLUT_ELAPSED_TIME)/500.0f;
    float bob=sinf(t*3+i*1.5f)*0.05f;
    glPushMatrix();
    glTranslatef(e.x,bob,e.z);
    glRotatef(-e.angle+180.0f,0,1,0);

    if(lightsOn)glColor3f(0.7f,0.1f,0.35f);else glColor3f(0.5f,0.05f,0.25f);
    glPushMatrix();glTranslatef(0,1.1f,0);glScalef(0.55f,0.7f,0.35f);glutSolidCube(1);glPopMatrix();
    if(lightsOn)glColor3f(0.85f,0.2f,0.4f);else glColor3f(0.6f,0.1f,0.28f);
    glPushMatrix();glTranslatef(0,1.65f,0);glutSolidSphere(0.22f,10,10);glPopMatrix();
    float eg=lightsOn?1.0f:(0.8f+0.2f*sinf(t*4+i));
    glColor3f(eg,eg*0.9f,0);
    glPushMatrix();glTranslatef(0.09f,1.68f,-0.19f);glutSolidSphere(0.055f,6,6);glPopMatrix();
    glPushMatrix();glTranslatef(-0.09f,1.68f,-0.19f);glutSolidSphere(0.055f,6,6);glPopMatrix();
    if(lightsOn)glColor3f(0.5f,0.08f,0.25f);else glColor3f(0.35f,0.04f,0.18f);
    glPushMatrix();glTranslatef(0,0.65f,0);glScalef(0.6f,0.25f,0.38f);glutSolidCube(1);glPopMatrix();
    float armSw=sinf(t*3+i*1.5f)*15.0f;
    if(lightsOn)glColor3f(0.65f,0.08f,0.30f);else glColor3f(0.45f,0.04f,0.22f);
    glPushMatrix();glTranslatef(-0.35f,1.05f,0);glRotatef(armSw,1,0,0);glTranslatef(0,-0.25f,0);glScalef(0.16f,0.5f,0.16f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0.35f,1.05f,0);glRotatef(-armSw,1,0,0);glTranslatef(0,-0.25f,0);glScalef(0.16f,0.5f,0.16f);glutSolidCube(1);glPopMatrix();
    float legSw=sinf(t*3+i*1.5f+(float)M_PI)*18.0f;
    if(lightsOn)glColor3f(0.4f,0.07f,0.2f);else glColor3f(0.28f,0.04f,0.14f);
    glPushMatrix();glTranslatef(-0.18f,0.5f,0);glRotatef(legSw,1,0,0);glTranslatef(0,-0.3f,0);glScalef(0.2f,0.6f,0.22f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0.18f,0.5f,0);glRotatef(-legSw,1,0,0);glTranslatef(0,-0.3f,0);glScalef(0.2f,0.6f,0.22f);glutSolidCube(1);glPopMatrix();
    glColor3f(0.05f,0.02f,0.05f);
    glPushMatrix();glTranslatef(0,0.02f,0);glScalef(0.7f,0.05f,0.5f);glutSolidCube(1);glPopMatrix();

    glPopMatrix();
}

void drawEnemies() { for(int i=0;i<5;i++) drawEnemyBody(i); }

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
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();gluOrtho2D(0,800,0,600);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
}
void endOrtho() {
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();
    glMatrixMode(GL_MODELVIEW);glEnable(GL_DEPTH_TEST);
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
    glColor3f(0.08f,0.08f,0.12f);
    glBegin(GL_QUADS);glVertex2f(bx1,by1);glVertex2f(bx1+bs,by1);glVertex2f(bx1+bs,by1+bs);glVertex2f(bx1,by1+bs);glEnd();
    glColor3f(0.5f,0.5f,0.5f);glLineWidth(2);
    glBegin(GL_LINE_LOOP);glVertex2f(bx1,by1);glVertex2f(bx1+bs,by1);glVertex2f(bx1+bs,by1+bs);glVertex2f(bx1,by1+bs);glEnd();
    if(!playerHasTreasure) drawGPSArrow(bx1+bs/2,by1+bs/2,treasureX,treasureZ,1,0.85f,0);
    else{glColor3f(0,1,0.3f);glLineWidth(3);glBegin(GL_LINE_STRIP);glVertex2f(bx1+20,by1+35);glVertex2f(bx1+35,by1+20);glVertex2f(bx1+60,by1+55);glEnd();}
    drawText2D(bx1+5,by1-18,playerHasTreasure?"GOT IT!":"TREASURE",1,0.85f,0);
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
    sprintf(buf,"TIME: %.0fs",elapsed);drawText2D(650,570,buf,0.7f,0.7f,0.7f);
    if(playerHasTreasure)drawText2D(330,540,"TREASURE: COLLECTED!",1,0.85f,0);
    else drawText2D(330,540,"TREASURE: NOT YET",0.5f,0.5f,0.55f);
    if(playerIsHiding)drawText2D(330,510,"HIDING",0.3f,1,0.3f);
    else if(isNearHidingSpot())drawText2D(330,510,"Press H to Hide",0.9f,0.9f,0.3f);
    if(lightsOn){
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1,0.85f,0,0.06f);glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(800,0);glVertex2f(800,600);glVertex2f(0,600);glEnd();
        glDisable(GL_BLEND);
        if(!playerIsHiding)drawTextLarge(200,470,"!! LIGHTS ON - HIDE NOW !!",1,0.1f,0.1f);
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

void drawMenu() {
    beginOrtho();
    glColor3f(0.03f,0.03f,0.06f);glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(800,0);glVertex2f(800,600);glVertex2f(0,600);glEnd();
    float t=(float)glutGet(GLUT_ELAPSED_TIME)/1000.0f;
    float tp=0.85f+0.15f*sinf(t*2);
    drawTextLarge(240,510,"MAZE ESCAPE",tp,tp*0.15f,tp*0.15f);
    drawTextLarge(185,470,"Find the treasure, then escape!",0.75f,0.75f,0.85f);
    drawText2D(200,415,"--- HOW TO PLAY ---",0.6f,0.6f,1);
    drawText2D(180,390,"WASD / Arrow Keys : Move",0.82f,0.82f,0.82f);
    drawText2D(180,368,"Mouse             : Look / Turn",0.82f,0.82f,0.82f);
    drawText2D(180,346,"H                 : Hide (near wooden crates)",0.82f,0.82f,0.82f);
    drawText2D(180,324,"Gold GPS box      : Arrow to Treasure",1,0.85f,0);
    drawText2D(180,302,"Green GPS box     : Arrow to Exit",0,1,0.3f);
    drawText2D(200,265,"--- RULES ---",0.6f,0.6f,1);
    drawText2D(180,242,"Every 60s lights come ON for 10s!",0.9f,0.25f,0.25f);
    drawText2D(180,220,"Be inside a crate or GAME OVER!",0.9f,0.25f,0.25f);
    drawText2D(180,198,"Avoid the humanoid enemies!",0.9f,0.4f,0.65f);
    drawText2D(180,176,"Exit with treasure = best score!",1,0.85f,0);
    float ba=0.5f+0.5f*sinf(t*2.5f);
    drawTextLarge(215,80,"Press ENTER or SPACE to Start",0.2f*ba+0.1f,ba*0.85f,0.2f*ba+0.1f);
    endOrtho();
}

void drawGameOver() {
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
    beginOrtho();
    glColor3f(0,0.04f,0.02f);glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(800,0);glVertex2f(800,600);glVertex2f(0,600);glEnd();
    if(wonWithTreasure){
        drawTextLarge(165,420,"YOU ESCAPED WITH TREASURE!",1,0.85f,0);
        char buf[64];sprintf(buf,"FINAL SCORE: %d",finalScore);
        drawTextLarge(255,355,buf,0.3f,1,0.3f);
    } else {
        drawTextLarge(155,420,"You escaped... but left the treasure!",0.55f,0.8f,0.55f);
        drawTextLarge(275,355,"SCORE: 0",0.45f,0.65f,0.45f);
        drawText2D(210,310,"Find the treasure first for a real score!",0.7f,0.7f,0.7f);
    }
    drawTextLarge(220,240,"R - Retry   M - Menu",0.7f,0.7f,0.7f);
    endOrtho();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
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
    if(!h)h=1;glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();gluPerspective(65.0,(double)w/h,0.3,200.0);
    glMatrixMode(GL_MODELVIEW);
}

void init() {
    glEnable(GL_DEPTH_TEST);glClearColor(0.03f,0.03f,0.06f,1);glShadeModel(GL_SMOOTH);
}

int main(int argc,char**argv) {
    srand((unsigned)time(NULL));
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(800,600);glutInitWindowPosition(100,100);
    glutCreateWindow("Maze Escape - Find Treasure & Survive");
    init();
    glutDisplayFunc(display);glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);glutSpecialFunc(specialKeys);
    glutPassiveMotionFunc(mouseMotion);
    glutTimerFunc(16,update,0);
    glutMainLoop();
    return 0;
}