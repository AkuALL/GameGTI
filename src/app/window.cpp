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

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();gluPerspective(65.0,(double)w/h,0.3,200.0);
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

    Image* doorImg = loadBMP(ASSET_IMAGE_DOOR);
    doorTextureId = loadTexture(doorImg);
    delete doorImg;

    Image* cabinetFrontImg = loadBMP(ASSET_IMAGE_CABINET_FRONT);
    cabinetFrontTexId = loadTexture(cabinetFrontImg);
    delete cabinetFrontImg;

    Image* cabinetBackImg = loadBMP(ASSET_IMAGE_CABINET_BACK);
    cabinetBackTexId = loadTexture(cabinetBackImg);
    delete cabinetBackImg;

    Image* cabinetSideImg = loadBMP(ASSET_IMAGE_CABINET_SIDE);
    cabinetSideTexId = loadTexture(cabinetSideImg);
    delete cabinetSideImg;

    Image* s1 = loadBMP(ASSET_IMAGE_SCARE1);
    if (s1 != NULL) { scareTexId1 = loadTexture(s1); delete s1; }

    Image* s2 = loadBMP(ASSET_IMAGE_SCARE2);
    if (s2 != NULL) { scareTexId2 = loadTexture(s2); delete s2; }

    Image* s3 = loadBMP(ASSET_IMAGE_SCARE3);
    if (s3 != NULL) { scareTexId3 = loadTexture(s3); delete s3; }

}
