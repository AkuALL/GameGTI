#ifndef SHADOW_ESCAPE_INPUT_H
#define SHADOW_ESCAPE_INPUT_H

void mouseMotion(int x, int y);
void movePlayer(float forward, float strafe);
bool isShiftDown();
bool isCtrlDown();
int movementKeyIndex(unsigned char key);
void updatePlayerMovement(float dt);

void keyboard(unsigned char key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);
void specialKeys(int key, int x, int y);
void specialKeysUp(int key, int x, int y);

#endif
