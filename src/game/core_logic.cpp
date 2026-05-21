#include "../assets/paths.h"

// Included from main.cpp. Compile main.cpp only so these definitions stay in one translation unit.

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
void resetGame(int level) {
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
    playBGM(ASSET_SOUND_BGM); 
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
