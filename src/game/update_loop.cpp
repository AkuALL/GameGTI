// Included from main.cpp. Compile main.cpp only so these definitions stay in one translation unit.

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
