#include <GL/glut.h>
#include <cstdlib>
#include <ctime>

#include "include/game_state.h"
#include "include/input.h"
#include "include/render.h"
#include "include/update_loop.h"
#include "include/window.h"

int main(int argc, char** argv) {
    srand((unsigned)time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Kelompok 5 GTI - Lab D2");

    initRendering();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialUpFunc(specialKeysUp);
    glutIgnoreKeyRepeat(1);
    glutPassiveMotionFunc(mouseMotion);
    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(windowWidth / 2, windowHeight / 2);
    glutTimerFunc(16, update, 0);
    glutMainLoop();

    return 0;
}
