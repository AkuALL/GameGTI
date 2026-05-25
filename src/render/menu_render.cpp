#include "../../include/render.h"
#include "../../include/game_state.h"

#include <cmath>

void drawMiniMazeWall(float x, float y, float w, float h) {
    // Dinding batu gelap dengan lis terang
    glColor3f(0.12f, 0.07f, 0.20f);
    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x+w, y);
    glVertex2f(x+w, y+h); glVertex2f(x, y+h);
    glEnd();
    glColor3f(0.22f, 0.14f, 0.32f);
    glLineWidth(0.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x+w, y);
    glVertex2f(x+w, y+h); glVertex2f(x, y+h);
    glEnd();
}
void drawMenuMaze() {
    // ── OUTER BOUNDARY ──
    drawMiniMazeWall(390, 160, 370, 5);   // north  (z=20)
    drawMiniMazeWall(390, 443, 370, 5);   // south  (z=-20)
    drawMiniMazeWall(390, 160, 5, 288);   // west   (x=-20)
    drawMiniMazeWall(755, 160, 5, 130);   // east upper (z=20 s/d -10.5)
    drawMiniMazeWall(755, 353, 5,  90);   // east lower (z=-13.5 s/d -20)

    // ── HORIZONTAL WALLS ── (z=16: screenY = 448 - 36*7.2 = 189)
    // z=16 → screenY=188
    drawMiniMazeWall(390, 188, 37, 5);    // wall 5:  x=-20 s/d -16
    drawMiniMazeWall(482, 188, 111, 5);   // wall 6:  x=-10 s/d 2   | gap -16 s/d -10
    drawMiniMazeWall(574, 188, 92,  5);   // wall 7:  x=3 s/d 13    | gap 2 s/d 8

    // z=10 → screenY=230
    drawMiniMazeWall(390, 230, 55, 5);    // wall 8:  x=-20 s/d -14 (diperbaiki)
    drawMiniMazeWall(445, 230, 111, 5);   // wall 9:  x=-14 s/d -2  (diperbaiki)
    drawMiniMazeWall(575, 230, 74, 5);    // wall 10: x=4 s/d 12

    // z=4 → screenY=274
    drawMiniMazeWall(390, 274, 92, 5);    // wall 11: x=-20 s/d -10 (diperbaiki)
    drawMiniMazeWall(528, 274, 92, 5);    // wall 12: x=-5 s/d 5
    drawMiniMazeWall(666, 274, 55, 5);    // wall 13: x=10 s/d 16   (diperbaiki)

    // z=-2 → screenY=317
    drawMiniMazeWall(390, 317, 55, 5);    // wall 14: x=-20 s/d -14
    drawMiniMazeWall(482, 317, 92, 5);    // wall 15: x=-11 s/d -1
    drawMiniMazeWall(610, 317, 74, 5);    // wall 16: x=4 s/d 12

    // z=-8 → screenY=360
    drawMiniMazeWall(408, 360, 74, 5);    // wall 17: x=-18 s/d -10
    drawMiniMazeWall(556, 360, 74, 5);    // wall 18: x=-2 s/d 6

    // z=-14 → screenY=404
    drawMiniMazeWall(390, 404, 55, 5);    // wall 19: x=-20 s/d -14
    drawMiniMazeWall(482, 404, 92, 5);    // wall 20: x=-9 s/d 1    (diperbaiki)

    // ── VERTICAL WALLS ──
    // x=-16 → screenX=427
    drawMiniMazeWall(427, 160, 5, 29);    // wall 21: z=16 s/d 20
    drawMiniMazeWall(427, 231, 5, 43);    // wall 22: z=4 s/d 10
    drawMiniMazeWall(427, 318, 5, 43);    // wall 23: z=-8 s/d -2
    drawMiniMazeWall(427, 361, 5, 43);    // wall 24: z=-20 s/d -14

    // x=-4 → screenX=538
    drawMiniMazeWall(538, 160, 5, 29);    // wall 25: z=16 s/d 20
    drawMiniMazeWall(538, 231, 5, 43);    // wall 26: z=4 s/d 10
    drawMiniMazeWall(538, 318, 5, 43);    // wall 27: z=-8 s/d -2
    drawMiniMazeWall(538, 361, 5, 43);    // wall 28: z=-20 s/d -14

    // x=8 → screenX=649
    drawMiniMazeWall(649, 160, 5, 29);    // wall 29: z=16 s/d 20
    drawMiniMazeWall(649, 231, 5, 43);    // wall 30: z=4 s/d 10
    drawMiniMazeWall(649, 318, 5, 43);    // wall 31: z=-8 s/d -2

    // x=16 → screenX=723
    drawMiniMazeWall(723, 160, 5, 29);    // wall 32: z=16 s/d 20
    drawMiniMazeWall(723, 231, 5, 43);    // wall 33: z=4 s/d 10    (diperbaiki)
    drawMiniMazeWall(723, 318, 5, 43);    // wall 34: z=-8 s/d -2   (diperbaiki)
    drawMiniMazeWall(723, 361, 5, 43);    // wall 35: z=-20 s/d -14 (baru)
}
void drawMenu() {
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.03f, 0.02f, 0.06f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    beginOrtho();

    glColor3f(0.03f, 0.015f, 0.055f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(800,0); glVertex2f(800,600); glVertex2f(0,600);
    glEnd();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.10f, 0.05f, 0.18f, 0.60f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0); glVertex2f(800, 0); glVertex2f(800, 120); glVertex2f(0, 120);
    glEnd();
    glDisable(GL_BLEND);

    drawMenuMaze();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 800.0f;
    float pulse = 0.4f + 0.3f * sinf(t * 2.5f);
    glColor4f(0.0f, pulse, pulse * 0.4f, 0.55f);
    glBegin(GL_QUADS);
    glVertex2f(752, 295); glVertex2f(770, 295);
    glVertex2f(770, 340); glVertex2f(752, 340);
    glEnd();
    float tpulse = 0.3f + 0.25f * sinf(t * 3.0f);
    glColor4f(tpulse * 1.5f, tpulse, 0.0f, 0.60f);
    glBegin(GL_QUADS);
    glVertex2f(530, 280); glVertex2f(555, 280);
    glVertex2f(555, 300); glVertex2f(530, 300);
    glEnd();
    float eye = 0.5f + 0.5f * sinf(t * 5.0f);
    glColor4f(eye, 0.0f, 0.0f, 0.85f);
    for (float ex = 477.0f; ex <= 487.0f; ex += 8.0f) {
        glBegin(GL_QUADS);
        glVertex2f(ex, 237); glVertex2f(ex+4, 237);
        glVertex2f(ex+4, 241); glVertex2f(ex, 241);
        glEnd();
    }
    glColor4f(eye * 0.8f, 0.0f, 0.0f, 0.7f);
    for (float ex = 620.0f; ex <= 630.0f; ex += 8.0f) {
        glBegin(GL_QUADS);
        glVertex2f(ex, 210); glVertex2f(ex+3, 210);
        glVertex2f(ex+3, 213); glVertex2f(ex, 213);
        glEnd();
    }
    glDisable(GL_BLEND);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.03f, 0.01f, 0.06f, 0.82f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(370,0); glVertex2f(370,600); glVertex2f(0,600);
    glEnd();
    glDisable(GL_BLEND);

    // Pohon & nisan
    glColor3f(0.08f, 0.04f, 0.12f);
    glLineWidth(4.0f);
    glBegin(GL_LINES);
    glVertex2f(20, 150); glVertex2f(40, 420);
    glEnd();
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(40, 320); glVertex2f(15, 275);
    glVertex2f(40, 320); glVertex2f(65, 270);
    glVertex2f(40, 360); glVertex2f(10, 340);
    glEnd();

    // ── JUDUL ──
    glColor3f(0.45f, 0.22f, 0.65f);
    glRasterPos2f(38, 570);
    const char* sub = "MAZE";
    for (const char* c = sub; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    glColor3f(0.78f, 0.15f, 0.18f);
    glRasterPos2f(36, 545);
    const char* title = "ESCAPE";
    for (const char* c = title; *c; c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    glRasterPos2f(37, 545);
    for (const char* c = title; *c; c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);

    // ── PILIH LEVEL ──
    glColor3f(0.55f, 0.35f, 0.70f);
    glRasterPos2f(36, 518);
    const char* lsLabel = "--- PILIH LEVEL ---";
    for (const char* c = lsLabel; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    // ── KOTAK LEVEL 1 ──
    float boxW = 130, boxH = 40;
    float box1x = 36, box1y = 468;
    if (selectedLevel == 1) glColor3f(0.25f, 0.55f, 0.25f);
    else                    glColor3f(0.10f, 0.20f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(box1x, box1y); glVertex2f(box1x+boxW, box1y);
    glVertex2f(box1x+boxW, box1y+boxH); glVertex2f(box1x, box1y+boxH);
    glEnd();
    glColor3f(0.4f, 0.8f, 0.4f); glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(box1x, box1y); glVertex2f(box1x+boxW, box1y);
    glVertex2f(box1x+boxW, box1y+boxH); glVertex2f(box1x, box1y+boxH);
    glEnd();
    if (selectedLevel == 1) glColor3f(0.9f, 1.0f, 0.9f);
    else                    glColor3f(0.5f, 0.7f, 0.5f);
    glRasterPos2f(box1x + 30, box1y + 20);
    const char* lv1 = "LEVEL 1";
    for (const char* c = lv1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    glColor3f(0.4f, 0.6f, 0.4f);
    glRasterPos2f(box1x + 20, box1y + 8);
    const char* lv1desc = "Easy - 2 harta";
    for (const char* c = lv1desc; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // ── KOTAK LEVEL 2 ──
    float box2x = 180, box2y = 468;
    if (selectedLevel == 2) glColor3f(0.55f, 0.18f, 0.12f);
    else                    glColor3f(0.20f, 0.08f, 0.06f);
    glBegin(GL_QUADS);
    glVertex2f(box2x, box2y); glVertex2f(box2x+boxW, box2y);
    glVertex2f(box2x+boxW, box2y+boxH); glVertex2f(box2x, box2y+boxH);
    glEnd();
    glColor3f(0.9f, 0.3f, 0.3f); glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(box2x, box2y); glVertex2f(box2x+boxW, box2y);
    glVertex2f(box2x+boxW, box2y+boxH); glVertex2f(box2x, box2y+boxH);
    glEnd();
    if (selectedLevel == 2) glColor3f(1.0f, 0.85f, 0.85f);
    else                    glColor3f(0.7f, 0.4f, 0.4f);
    glRasterPos2f(box2x + 30, box2y + 20);
    const char* lv2 = "LEVEL 2";
    for (const char* c = lv2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    glColor3f(0.6f, 0.35f, 0.35f);
    glRasterPos2f(box2x + 4, box2y + 8);
    const char* lv2desc = "Medium - timer 2 mnt";
    for (const char* c = lv2desc; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // ── KOTAK LEVEL 3 (SULIT) ──
    float box3x = 36, box3y = 418;
    if (selectedLevel == 3) glColor3f(0.50f, 0.10f, 0.10f);
    else                    glColor3f(0.18f, 0.05f, 0.05f);
    glBegin(GL_QUADS);
    glVertex2f(box3x, box3y); glVertex2f(box3x+boxW*2+14, box3y);
    glVertex2f(box3x+boxW*2+14, box3y+boxH); glVertex2f(box3x, box3y+boxH);
    glEnd();
    glColor3f(1.0f, 0.2f, 0.2f); glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(box3x, box3y); glVertex2f(box3x+boxW*2+14, box3y);
    glVertex2f(box3x+boxW*2+14, box3y+boxH); glVertex2f(box3x, box3y+boxH);
    glEnd();
    if (selectedLevel == 3) glColor3f(1.0f, 0.7f, 0.7f);
    else                    glColor3f(0.6f, 0.3f, 0.3f);
    glRasterPos2f(box3x + 100, box3y + 20);  // teks judul di tengah atas
    const char* lv3 = "LEVEL 3";
    for (const char* c = lv3; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    glColor3f(0.55f, 0.25f, 0.25f);
    glRasterPos2f(box3x + 95, box3y + 8);  // teks deskripsi di bawahnya
    const char* lv3desc = "Hard - 3 harta";
    for (const char* c = lv3desc; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // ── GARIS + DESKRIPSI LEVEL YANG DIPILIH ──
    glColor3f(0.20f, 0.08f, 0.28f);
    glLineWidth(0.5f);
    glBegin(GL_LINES);
    glVertex2f(36, 412); glVertex2f(340, 412);
    glEnd();

    if (selectedLevel == 1) {
        glColor3f(0.45f, 0.75f, 0.45f);
        glRasterPos2f(36, 398);
        const char* d1 = "Kumpulkan 2 harta lalu cari pintu keluar.";
        for (const char* c = d1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        glColor3f(0.35f, 0.55f, 0.35f);
        glRasterPos2f(36, 384);
        const char* d2 = "Lampu menyala tiap 40 detik (selama 10 detik).";
        for (const char* c = d2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    } else if (selectedLevel == 2) {
        glColor3f(0.85f, 0.45f, 0.35f);
        glRasterPos2f(36, 398);
        const char* d1 = "Timer 2 menit, habis = semua musuh mengejar!";
        for (const char* c = d1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        glColor3f(0.65f, 0.35f, 0.25f);
        glRasterPos2f(36, 384);
        const char* d2 = "Kumpulkan 2 harta sebelum waktu habis.";
        for (const char* c = d2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    } else {
        glColor3f(1.0f, 0.35f, 0.35f);
        glRasterPos2f(36, 398);
        const char* d1 = "Timer 2 menit, habis = semua musuh mengejar!";
        for (const char* c = d1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        glColor3f(0.75f, 0.25f, 0.25f);
        glRasterPos2f(36, 384);
        const char* d2 = "Kumpulkan 3 harta. Mode paling sulit!";
        for (const char* c = d2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // ── SUBTITLE ──
    glColor3f(0.55f, 0.35f, 0.70f);
    glRasterPos2f(36, 368);
    const char* findTreasure = "CARI HARTA & KELUAR!";
    for (const char* c = findTreasure; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // ── GARIS PEMISAH ──
    glColor3f(0.20f, 0.08f, 0.28f);
    glLineWidth(0.5f);
    glBegin(GL_LINES);
    glVertex2f(36, 360); glVertex2f(340, 360);
    glEnd();

    // ── CARA BERMAIN ──
    glColor3f(0.55f, 0.35f, 0.70f);
    glRasterPos2f(36, 346);
    const char* howLabel = "--- CARA BERMAIN ---";
    for (const char* c = howLabel; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(0.85f, 0.85f, 0.85f);
    glRasterPos2f(36, 331);
    const char* h1a = "WASD / Panah";
    for (const char* c = h1a; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    glColor3f(0.55f, 0.55f, 0.60f);
    glRasterPos2f(175, 331);
    const char* h1b = ": Bergerak";
    for (const char* c = h1b; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(0.85f, 0.85f, 0.85f);
    glRasterPos2f(36, 316);
    const char* h2a = "Mouse";
    for (const char* c = h2a; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    glColor3f(0.55f, 0.55f, 0.60f);
    glRasterPos2f(175, 316);
    const char* h2b = ": Arahkan pandangan";
    for (const char* c = h2b; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(0.85f, 0.85f, 0.85f);
    glRasterPos2f(36, 301);
    const char* h3a = "H";
    for (const char* c = h3a; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    glColor3f(0.55f, 0.55f, 0.60f);
    glRasterPos2f(175, 301);
    const char* h3b = ": Bersembunyi (dalam kotak kuning)";
    for (const char* c = h3b; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(1.0f, 0.85f, 0.0f);
    glRasterPos2f(36, 286);
    const char* h4a = "Kotak GPS kuning";
    for (const char* c = h4a; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    glColor3f(0.55f, 0.55f, 0.60f);
    glRasterPos2f(175, 286);
    const char* h4b = ": Menunjuk ke harta";
    for (const char* c = h4b; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(0.2f, 1.0f, 0.4f);
    glRasterPos2f(36, 271);
    const char* h5a = "Kotak GPS hijau";
    for (const char* c = h5a; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    glColor3f(0.55f, 0.55f, 0.60f);
    glRasterPos2f(175, 271);
    const char* h5b = ": Menunjuk ke pintu keluar";
    for (const char* c = h5b; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // ── GARIS PEMISAH ──
    glColor3f(0.20f, 0.08f, 0.28f);
    glBegin(GL_LINES);
    glVertex2f(36, 263); glVertex2f(340, 263);
    glEnd();

    // ── PERATURAN ──
    glColor3f(0.55f, 0.35f, 0.70f);
    glRasterPos2f(36, 249);
    const char* rulesLabel = "--- PERATURAN ---";
    for (const char* c = rulesLabel; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(1.0f, 0.35f, 0.15f);
    glRasterPos2f(36, 234);
    const char* r1 = "Tiap 40 detik lampu menyala selama 10 detik!";
    for (const char* c = r1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(0.75f, 0.75f, 0.80f);
    glRasterPos2f(36, 219);
    const char* r2 = "Lampu menyala = musuh mengejar kamu!";
    for (const char* c = r2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glRasterPos2f(36, 204);
    const char* r3 = "Bersembunyi di peti agar tetap aman.";
    for (const char* c = r3; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glRasterPos2f(36, 189);
    const char* r4 = "Hindari musuh";
    for (const char* c = r4; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glColor3f(1.0f, 0.85f, 0.0f);
    glRasterPos2f(36, 174);
    const char* r5 = "Keluar membawa seluruh treasure = skor terbaik!";
    for (const char* c = r5; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // ── GARIS PEMISAH ──
    glColor3f(0.20f, 0.08f, 0.28f);
    glLineWidth(0.5f);
    glBegin(GL_LINES);
    glVertex2f(36, 162); glVertex2f(340, 162);
    glEnd();

    // ── TOMBOL MULAI ──
    float ba = 0.5f + 0.5f * sinf(t * 2.5f);
    glColor3f(0.1f * ba, ba * 0.85f, 0.2f * ba);
    glRasterPos2f(36, 142);
    const char* startMsg = "> Tekan ENTER atau SPASI untuk Mulai";
    for (const char* c = startMsg; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    endOrtho();
}
