#include "../../include/render.h"
#include "../../include/game_logic.h"
#include "../../include/game_state.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

int getBitmapTextWidth(void* font, const char* text) {
    int width = 0;
    for (const char* c = text; *c; c++) width += glutBitmapWidth(font, *c);
    return width;
}

float getBitmapTextWidthOrtho(void* font, const char* text) {
    float scaleX = (windowWidth > 0) ? (800.0f / windowWidth) : 1.0f;
    return getBitmapTextWidth(font, text) * scaleX;
}

void drawBitmapText(void* font, float x, float y, const char* t, float r, float g, float b) {
    glColor3f(r,g,b);glRasterPos2f(x,y);
    for(const char*c=t;*c;c++) glutBitmapCharacter(font,*c);
}
void drawText2D(float x,float y,const char*t,float r,float g,float b) {
    drawBitmapText(GLUT_BITMAP_HELVETICA_18, x, y, t, r, g, b);
}
void drawTextLarge(float x,float y,const char*t,float r,float g,float b) {
    drawBitmapText(GLUT_BITMAP_TIMES_ROMAN_24, x, y, t, r, g, b);
}
void drawTextCentered(void* font, float centerX, float y, const char* t, float r, float g, float b) {
    drawBitmapText(font, centerX - getBitmapTextWidthOrtho(font, t) / 2.0f, y, t, r, g, b);
}
void drawText2DCentered(float centerX, float y, const char* t, float r, float g, float b) {
    drawTextCentered(GLUT_BITMAP_HELVETICA_18, centerX, y, t, r, g, b);
}
void drawTextLargeCentered(float centerX, float y, const char* t, float r, float g, float b) {
    drawTextCentered(GLUT_BITMAP_TIMES_ROMAN_24, centerX, y, t, r, g, b);
}

void beginOrtho() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();gluOrtho2D(0,800,0,600);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
}
void endOrtho() {
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();
    glMatrixMode(GL_MODELVIEW);glEnable(GL_DEPTH_TEST);
}

void prepareStaticScreen(float r, float g, float b) {
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


void drawLevelClear() {
    prepareStaticScreen(0, 0.05f, 0.03f);
    beginOrtho();

    // Background
    glColor3f(0, 0.05f, 0.03f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(800,0); glVertex2f(800,600); glVertex2f(0,600);
    glEnd();

    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float p = 0.7f + 0.3f * sinf(t * 4.0f);

    // Judul
    if (currentLevel == 1) {
        drawTextLargeCentered(400, 430, "LEVEL 1- EASY SELESAI!", 0.2f, p, 0.4f);
    } else if (currentLevel == 2) {
        drawTextLargeCentered(400, 430, "LEVEL 2-MEDIUM SELESAI!", 0.2f, p, 0.4f);
    }

    // Garis pemisah
    glColor3f(0.1f, 0.4f, 0.2f);
    glBegin(GL_LINES);
    glVertex2f(200, 395); glVertex2f(600, 395);
    glEnd();

    // Teks pilihan
    drawText2DCentered(400, 365, "Lanjutkan?", 0.7f, 0.9f, 0.7f);

    // ── TOMBOL: NEXT LEVEL ──
    float btnW = 220, btnH = 50;
    float btn1x = 140, btn1y = 280;

    glColor3f(0.10f, 0.40f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(btn1x, btn1y); glVertex2f(btn1x+btnW, btn1y);
    glVertex2f(btn1x+btnW, btn1y+btnH); glVertex2f(btn1x, btn1y+btnH);
    glEnd();
    glColor3f(0.3f, p, 0.4f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(btn1x, btn1y); glVertex2f(btn1x+btnW, btn1y);
    glVertex2f(btn1x+btnW, btn1y+btnH); glVertex2f(btn1x, btn1y+btnH);
    glEnd();
    glColor3f(0.8f, 1.0f, 0.8f);
    glRasterPos2f(btn1x + 18, btn1y + 20);
    const char* nextMsg = (currentLevel == 1) ? "N - Next: Level 2" : "N - Next: Level 3";
    for (const char* c = nextMsg; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    // ── TOMBOL: MAIN MENU ──
    float btn2x = 440, btn2y = 280;

    glColor3f(0.12f, 0.08f, 0.30f);
    glBegin(GL_QUADS);
    glVertex2f(btn2x, btn2y); glVertex2f(btn2x+btnW, btn2y);
    glVertex2f(btn2x+btnW, btn2y+btnH); glVertex2f(btn2x, btn2y+btnH);
    glEnd();
    glColor3f(0.4f, 0.3f, p);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(btn2x, btn2y); glVertex2f(btn2x+btnW, btn2y);
    glVertex2f(btn2x+btnW, btn2y+btnH); glVertex2f(btn2x, btn2y+btnH);
    glEnd();
    glColor3f(0.75f, 0.70f, 1.0f);
    glRasterPos2f(btn2x + 35, btn2y + 20);
    const char* menuMsg = "M - Main Menu";
    for (const char* c = menuMsg; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    // Info level berikutnya
    glColor3f(0.5f, 0.7f, 0.5f);
    const char* info = (currentLevel == 1)
        ? "Level 2: Timer 2 menit! Habis = semua musuh kejar!"
        : "Level 3: 3 Harta. Timer 2 menit! Habis = semua musuh kejar!";
    drawTextCentered(GLUT_BITMAP_HELVETICA_12, 400, 245, info, 0.5f, 0.7f, 0.5f);

    // Hint keyboard
    float ba = 0.5f + 0.5f * sinf(t * 3.0f);
    const char* hint = "Press N or M";
    drawTextCentered(GLUT_BITMAP_HELVETICA_18, 400, 200, hint, ba * 0.4f, ba * 0.8f, ba * 0.4f);

    endOrtho();
}

void drawGPSArrow(float cx,float cy,float tx,float tz,float r,float g,float b) {
    float dx=tx-playerX,dz=tz-playerZ;
    float arrowAngle=atan2f(dx,dz)*180.0f/(float)M_PI-playerAngle;
    glPushMatrix();glTranslatef(cx,cy,0);glRotatef(arrowAngle,0,0,1);glColor3f(r,g,b);
    glBegin(GL_TRIANGLES);glVertex2f(0,22);glVertex2f(-9,-8);glVertex2f(9,-8);glEnd();
    glBegin(GL_QUADS);glVertex2f(-3,-8);glVertex2f(3,-8);glVertex2f(3,-22);glVertex2f(-3,-22);glEnd();
    glPopMatrix();
}
void drawJumpscare() {
    if (!jumpscareActive) return;

    float elapsed = (glutGet(GLUT_ELAPSED_TIME) - jumpscareStartTime) / 1000.0f;
    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 500.0f;

    float cx = 400, cy = 300;

    // ── ZOOM: loncat super cepat dalam 0.15 detik ──
    float zoomT = elapsed / 0.15f;
    if (zoomT > 1.0f) zoomT = 1.0f;
    // Overshoot: melebihi target lalu balik sedikit (efek "lompat")
    float bounce = zoomT < 0.7f
        ? zoomT / 0.7f                          // naik cepat
        : 1.0f + (1.0f - zoomT/1.0f) * 0.15f;  // sedikit overshoot
    float size = 20.0f + bounce * 310.0f;        // dari kecil banget ke SANGAT besar

    // ── SHAKE: kencang & konstan sepanjang durasi ──
    float shakeAmp = (elapsed < 0.15f) ? 0.0f : 18.0f; // shake mulai setelah zoom
    float shakeX = (rand() % (int)(shakeAmp * 2 + 1) - (int)shakeAmp);
    float shakeY = (rand() % (int)(shakeAmp * 2 + 1) - (int)shakeAmp);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ── FLASH PUTIH seketika saat muncul ──
    if (elapsed < 0.08f) {
        float flashAlpha = 1.0f - (elapsed / 0.08f);
        glColor4f(1.0f, 1.0f, 1.0f, flashAlpha);
        glBegin(GL_QUADS);
        glVertex2f(0,0); glVertex2f(800,0);
        glVertex2f(800,600); glVertex2f(0,600);
        glEnd();
    }

    // ── OVERLAY MERAH ──
    glColor4f(0.9f, 0.0f, 0.0f, jumpscareAlpha * 0.90f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(800,0);
    glVertex2f(800,600); glVertex2f(0,600);
    glEnd();

    // ── KEPALA — zoom in dari tengah ──
    glColor4f(0.05f, 0.02f, 0.08f, jumpscareAlpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx + shakeX, cy + shakeY);
    for (int s = 0; s <= 20; s++) {
        float a = s * 2 * (float)M_PI / 20;
        glVertex2f(cx + shakeX + cosf(a) * size,
                   cy + shakeY + sinf(a) * size * 1.2f);
    }
    glEnd();

    // ── MATA — lebih besar & lebih glow ──
    float eyeGlow = 0.85f + 0.15f * sinf(t * 15);
    glColor4f(1.0f, 0.0f, 0.0f, jumpscareAlpha * eyeGlow);
    for (int eye = -1; eye <= 1; eye += 2) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx + shakeX + eye * size * 0.32f,
                   cy + shakeY - size * 0.12f);
        for (int s = 0; s <= 16; s++) {
            float a = s * 2 * (float)M_PI / 16;
            float r = size * 0.22f; // mata lebih besar
            glVertex2f(cx + shakeX + eye * size * 0.32f + cosf(a) * r,
                       cy + shakeY - size * 0.12f + sinf(a) * r * 0.85f);
        }
        glEnd();
    }

    // ── GLOW MERAH DI MATA (halo luar) ──
    glColor4f(1.0f, 0.0f, 0.0f, jumpscareAlpha * 0.35f);
    for (int eye = -1; eye <= 1; eye += 2) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx + shakeX + eye * size * 0.32f,
                   cy + shakeY - size * 0.12f);
        for (int s = 0; s <= 16; s++) {
            float a = s * 2 * (float)M_PI / 16;
            float r = size * 0.38f;
            glVertex2f(cx + shakeX + eye * size * 0.32f + cosf(a) * r,
                       cy + shakeY - size * 0.12f + sinf(a) * r);
        }
        glEnd();
    }

    // ── MULUT LEBAR ──
    glColor4f(0.0f, 0.0f, 0.0f, jumpscareAlpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx + shakeX, cy + shakeY + size * 0.28f);
    for (int s = 0; s <= 14; s++) {
        float a = s * (float)M_PI / 14;
        glVertex2f(cx + shakeX + cosf(a) * size * 0.45f,
                   cy + shakeY + size * 0.28f + sinf(a) * size * 0.30f);
    }
    glEnd();

    // ── TEKS "FOUND YOU" — muncul langsung sejak 0.15s ──
    if (elapsed > 0.15f) {
        const char* msg = "FOUND YOU";
        void* font = GLUT_BITMAP_TIMES_ROMAN_24;
        float textX = 400.0f - getBitmapTextWidthOrtho(font, msg) / 2.0f;
        float textY = 160.0f;

        // Shadow teks
        glColor4f(0.0f, 0.0f, 0.0f, jumpscareAlpha);
        glRasterPos2f(textX + 3 + shakeX, textY - 3 + shakeY);
        for (const char* c = msg; *c; c++)
            glutBitmapCharacter(font, *c);
        // Teks utama putih
        glColor4f(1.0f, 1.0f, 1.0f, jumpscareAlpha);
        glRasterPos2f(textX + shakeX, textY + shakeY);
        for (const char* c = msg; *c; c++)
            glutBitmapCharacter(font, *c);
    }

    glDisable(GL_BLEND);
}
void drawGPS() {
    beginOrtho();
    float bx1=20,by1=490,bs=80;
    // GANTI blok GPS treasure (bx1) dengan:
glColor3f(0.08f,0.08f,0.12f);
glBegin(GL_QUADS);glVertex2f(bx1,by1);glVertex2f(bx1+bs,by1);glVertex2f(bx1+bs,by1+bs);glVertex2f(bx1,by1+bs);glEnd();
glColor3f(0.5f,0.5f,0.5f);glLineWidth(2);
glBegin(GL_LINE_LOOP);glVertex2f(bx1,by1);glVertex2f(bx1+bs,by1);glVertex2f(bx1+bs,by1+bs);glVertex2f(bx1,by1+bs);glEnd();

// ── HUD LEVEL 2: COUNTDOWN TIMER ──
LevelConfig &hcfg = levelConfigs[currentLevel - 1];
if (hcfg.hasCountdown) {
    char cbuf[64];
    if (!chaseMode) {
        // Countdown belum habis — tampilkan timer normal
        float remaining = level2Countdown - level2CountdownTimer;
        sprintf(cbuf, "COUNTDOWN: %.0fs", remaining);
        float danger = 1.0f - (remaining / level2Countdown);
        drawTextLarge(270, 540, cbuf, 0.5f + danger*0.5f, (1.0f-danger)*0.6f, 0);
        if (remaining <= 10.0f) {
            float blink = (sinf((float)glutGet(GLUT_ELAPSED_TIME)/150.0f) > 0) ? 1.0f : 0.0f;
            if (blink > 0.5f) drawTextLarge(185, 480, "!! MUSUH AKAN MENYERANG SEGERA !!", 1, 0.1f, 0);
        }
    } else {
        // ✅ Chase mode PERMANEN (countdown habis) — baru tampilkan ini
        drawTextLarge(230, 540, "!! CHASE MODE - LARIII !!", 1.0f, 0.05f, 0.05f);
    }
}
// ── INDIKATOR LEVEL ──
char lvlbuf[16];
sprintf(lvlbuf, "LEVEL %d", currentLevel);
drawText2D(710, 570, lvlbuf, 0.6f, 0.6f, 0.9f);
if (!allTreasuresCollected()) {
    // Tunjuk treasure terdekat yang belum diambil
    float nearDist = 1e9f; int nearIdx = -1;
    for (int i = 0; i < treasureCount; i++) {
        if (treasureCollected[i]) continue;
        float dx = playerX - treasureX[i], dz = playerZ - treasureZ[i];
        float d = sqrtf(dx*dx + dz*dz);
        if (d < nearDist) { nearDist = d; nearIdx = i; }
    }
    if (nearIdx >= 0)
        drawGPSArrow(bx1+bs/2, by1+bs/2, treasureX[nearIdx], treasureZ[nearIdx], 1, 0.85f, 0);
} else {
    glColor3f(0,1,0.3f);glLineWidth(3);
    glBegin(GL_LINE_STRIP);
    glVertex2f(bx1+20,by1+35);glVertex2f(bx1+35,by1+20);glVertex2f(bx1+60,by1+55);
    glEnd();
}

char tlabel[32];
sprintf(tlabel, "TRSR %d/%d", treasuresHeld(), treasureCount);
drawText2D(bx1+2, by1-18, allTreasuresCollected() ? "ALL GOT!" : tlabel, 1, 0.85f, 0);
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
    sprintf(buf,"TIME: %.0fs",elapsed);drawText2D(500,570,buf,0.7f,0.7f,0.7f);
    if(playerIsHiding)drawText2D(330,510,"HIDING",0.3f,1,0.3f);
    else if(isNearHidingSpot())drawText2D(330,510,"Press H to Hide",0.9f,0.9f,0.3f);
    if(lightsOn){
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1,0.85f,0,0.06f);glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(800,0);glVertex2f(800,600);glVertex2f(0,600);glEnd();
        glDisable(GL_BLEND);
        if(!playerIsHiding)drawTextLarge(185,470,"!! LIGHTS ON - HIDE !!",1,0.1f,0.1f);
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
    drawJumpscare();
    endOrtho();
}
// ===================================================
// HELPER: Gambar dinding maze mini untuk menu
