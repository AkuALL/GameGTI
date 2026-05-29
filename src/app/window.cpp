#include "../../include/window.h"
#include "../../include/game_logic.h"
#include "../../include/game_state.h"
#include "../../include/imageloader.h"
#include "../../include/paths.h"

#include <GL/glut.h>

void reshape(int w,int h) {
    if(!h)h=1;
    windowWidth = w;
    windowHeight = h;
    lastMouseX = w / 2;
    int viewportW = w;
    int viewportH = (int)(w * 3.0f / 4.0f);
    if (viewportH > h) {
        viewportH = h;
        viewportW = (int)(h * 4.0f / 3.0f);
    }
    int viewportX = (w - viewportW) / 2;
    int viewportY = (h - viewportH) / 2;
    glViewport(viewportX, viewportY, viewportW, viewportH);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();gluPerspective(65.0,(double)viewportW/viewportH,0.3,200.0);
    glMatrixMode(GL_MODELVIEW);
}

void initRendering() {
    glEnable(GL_DEPTH_TEST); 
    glClearColor(0.03f,0.03f,0.06f,1);
    glShadeModel(GL_SMOOTH);

    Image* wallImg = loadBMP(ASSET_IMAGE_WALL); 
    wallTextureId = loadTexture(wallImg); 
    delete wallImg; 

    Image* floorImg = loadBMP(ASSET_IMAGE_FLOOR); 
    floorTextureId = loadTexture(floorImg); 
    delete floorImg;

    Image* s1 = loadBMP(ASSET_IMAGE_SCARE1);
    if (s1 != NULL) { scareTexId1 = loadTexture(s1); delete s1; }

    Image* s2 = loadBMP(ASSET_IMAGE_SCARE2);
    if (s2 != NULL) { scareTexId2 = loadTexture(s2); delete s2; }

    Image* s3 = loadBMP(ASSET_IMAGE_SCARE3);
    if (s3 != NULL) { scareTexId3 = loadTexture(s3); delete s3; }

}