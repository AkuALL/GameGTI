#include "../../include/render.h"
#include "../../include/game_logic.h"
#include "../../include/game_state.h"

#include <cstdio>
#include <cmath>

void drawGameOver() {
    prepareStaticScreen(0.04f,0,0);
    beginOrtho();
    glColor3f(0.04f,0,0);
    glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(800,0);glVertex2f(800,600);glVertex2f(0,600);glEnd();
    float t=(float)glutGet(GLUT_ELAPSED_TIME)/1000.0f;
    float p=0.7f+0.3f*sinf(t*3.5f);
    drawTextLargeCentered(400,420,"GAME OVER",p,0.05f,0.05f);

    // Tunjukkan di level berapa kalahnya
    char lvlbuf[32];
    sprintf(lvlbuf, "Died on Level %d", currentLevel);
    drawText2DCentered(400, 385, lvlbuf, 0.6f, 0.3f, 0.3f);

    char buf[64];
    if(wonWithTreasure){
        drawText2DCentered(400,340,"Kamu berhasil mendapat treasure tapi gagal keluar...",1,0.75f,0);
        sprintf(buf,"Score: %d",finalScore);
        drawText2DCentered(400,300,buf,1,0.75f,0);
    } else {
        drawText2DCentered(400,340,"Kamu tertangkap...",0.8f,0.4f,0.4f);
        drawText2DCentered(400,300,"Score: 0",0.6f,0.6f,0.6f);
    }

    // Tombol retry & menu
    drawTextLargeCentered(400,220,"R - Retry Level   M - Main Menu",0.7f,0.7f,0.7f);

    // Info retry
    sprintf(buf, "(Retry akan mengulang level saat ini. (level %d))", currentLevel);
    drawTextCentered(GLUT_BITMAP_HELVETICA_12, 400, 185, buf, 0.5f, 0.5f, 0.5f);

    endOrtho();
}

void drawWin() {
    prepareStaticScreen(0,0.04f,0.02f);
    beginOrtho();
    glColor3f(0,0.04f,0.02f);glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(800,0);glVertex2f(800,600);glVertex2f(0,600);glEnd();
    if(wonWithTreasure){
        drawTextLargeCentered(400,420,"KAMU BERHASIL MEMBAWA SEMUA TREASURE!!",0.3f,1,0.45f);
        char buf[64];sprintf(buf,"FINAL SCORE: %d",finalScore);
        drawTextLargeCentered(400,355,buf,0.3f,1,0.3f);
    } else {
        char buf[64];
        sprintf(buf,"Kamu keluar membawa %d/%d treasure!", treasuresHeld(), treasureCount);
        drawText2DCentered(400,420,buf,0.55f,0.8f,0.55f);
        sprintf(buf,"SCORE: %d", finalScore);
        drawTextLargeCentered(400,355,buf,0.45f,0.65f,0.45f);
        drawText2DCentered(400,310,"Dapatkan semua treasure untuk skor terbaik!!",0.7f,0.7f,0.7f);
    }
    drawTextLargeCentered(400,240,"R - Retry   M - Menu",0.7f,0.7f,0.7f);
    endOrtho();
}

void display() {
    glLoadIdentity();
    if (gameState == STATE_MENU)        { drawMenu();       glutSwapBuffers(); return; }
    if (gameState == STATE_GAMEOVER)    { drawGameOver();   glutSwapBuffers(); return; }
    if (gameState == STATE_WIN)         { drawWin();        glutSwapBuffers(); return; }
    if (gameState == STATE_LEVEL_CLEAR) { drawLevelClear(); glutSwapBuffers(); return; }

    // ── LEVEL CLEAR OVERLAY ──
    if (levelClearing) { drawLevelClear(); glutSwapBuffers(); return; }

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
