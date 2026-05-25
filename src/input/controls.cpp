#include "../../include/input.h"
#include "../../include/game_logic.h"
#include "../../include/game_state.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <GL/glut.h>
#include <cmath>
#include <cstdlib>

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
