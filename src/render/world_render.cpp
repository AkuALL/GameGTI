#include "../../include/render.h"
#include "../../include/game_state.h"

#include <cmath>

void applyFlicker(float r, float g, float b) {
    glColor3f(r*flickerIntensity, g*flickerIntensity, b*flickerIntensity);
}

float cellHash(int ix, int iz) {
    int n = ix*1619 + iz*31337;
    n = (n<<13)^n;
    return 1.0f - ((n*(n*n*15731+789221)+1376312589)&0x7fffffff)/1073741824.0f;
}

void drawStoneSlab(float wx, float wz, float sw, float sh,
                   float bR, float bG, float bB,
                   float cR, float cG, float cB, int cracks) {
    glColor3f(bR,bG,bB);
    glBegin(GL_QUADS);
    glVertex3f(wx,     0,wz);    glVertex3f(wx+sw,0,wz);
    glVertex3f(wx+sw,  0,wz+sh); glVertex3f(wx,   0,wz+sh);
    glEnd();
    float g=0.04f;
    glColor3f(cR*0.55f,cG*0.55f,cB*0.55f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(wx+g,0.01f,wz+g);    glVertex3f(wx+sw-g,0.01f,wz+g);
    glVertex3f(wx+sw-g,0.01f,wz+sh-g); glVertex3f(wx+g,0.01f,wz+sh-g);
    glEnd();
    glColor3f(cR,cG,cB); glLineWidth(1);
    for (int c=0;c<cracks;c++) {
        float h1=cellHash((int)(wx*10+c),(int)(wz*10+c*7));
        float h2=cellHash((int)(wx*10+c*3),(int)(wz*10+c*11));
        float h3=cellHash((int)(wx*10+c*5),(int)(wz*10+c*13));
        float h4=cellHash((int)(wx*10+c*9),(int)(wz*10+c*17));
        glBegin(GL_LINES);
        glVertex3f(wx+g+fabsf(h1)*(sw-2*g),0.012f,wz+g+fabsf(h2)*(sh-2*g));
        glVertex3f(wx+g+fabsf(h3)*(sw-2*g),0.012f,wz+g+fabsf(h4)*(sh-2*g));
        glEnd();
    }
}

void drawFloor() {
    // 1. AKTIFKAN TEKSTUR LANTAI & ATUR MODULASI CAHAYA
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, floorTextureId);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 

    float slabW = 2.0f, slabH = 1.5f;
    int cols = (int)((MAP_MAX - MAP_MIN) / slabW) + 1;
    int rows = (int)((MAP_MAX - MAP_MIN) / slabH) + 1;

    for (int row = 0; row < rows; row++) {
        float fz = MAP_MIN + row * slabH;
        float ox = (row % 2 == 0) ? 0.0f : slabW * 0.5f;
        
        for (int col = 0; col < cols; col++) {
            float fx = MAP_MIN - ox + col * slabW;
            if (fx + slabW < MAP_MIN || fx > MAP_MAX || fz + slabH < MAP_MIN || fz > MAP_MAX) continue;
            
            float var = cellHash(col + row * 47, row + col * 31) * 0.5f + 0.5f;
            float r, g, b, cr, cg, cb; int nc;
            
            if (lightsOn) {
                float base = 0.50f + var * 0.18f, warm = 0.03f + var * 0.04f;
                r = base + warm; g = base + warm * 0.5f; b = base - warm * 0.5f;
                cr = 0.28f; cg = 0.25f; cb = 0.22f; nc = (int)(var * 2.5f);
            } else {
                float base = 0.08f + var * 0.07f;
                r = base * flickerIntensity * 0.9f; g = base * flickerIntensity * 0.95f; b = (base + 0.04f) * flickerIntensity;
                cr = cg = 0.03f * flickerIntensity; cb = 0.06f * flickerIntensity; nc = (int)(var * 2.0f);
            }

            glPushMatrix();
            glColor3f(r, g, b); // Set warna sesuai kondisi lampu/kedip game
            
            glBegin(GL_QUADS);
            glNormal3f(0.0f, 1.0f, 0.0f); 
            // Mapping koordinat gambar lantai ke tiap-tiap slab ubin batu
            glTexCoord2f(0.0f, 0.0f); glVertex3f(fx,         0.0f, fz);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(fx + slabW, 0.0f, fz);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(fx + slabW, 0.0f, fz + slabH);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(fx,         0.0f, fz + slabH);
            glEnd();
            
            glPopMatrix();

            // Sisa dekorasi tepi ubin (border) tetap digambar lewat fungsi aslimu
            drawStoneSlab(fx, fz, slabW, slabH, r, g, b, cr, cg, cb, nc);
        }
    }

    // MATIKAN TEKSTUR AGAR ATAP / CEILING GAK IKUTAN KENA GAMBAR LANTAI
    glDisable(GL_TEXTURE_2D);

    // --- BAGIAN ATAP & BALOK CEILING (Tetap ori sesuai kode aslimu) ---
    float ceilH = 3.5f, cofSz = 3.0f, cofBdr = 0.18f, cofDep = 0.12f;
    if (lightsOn) glColor3f(0.68f, 0.64f, 0.56f);
    else { float b = 0.07f * flickerIntensity; glColor3f(b * 0.85f, b * 0.8f, b); }
    
    glBegin(GL_QUADS);
    glVertex3f(MAP_MIN, ceilH, MAP_MIN); glVertex3f(MAP_MAX, ceilH, MAP_MIN);
    glVertex3f(MAP_MAX, ceilH, MAP_MAX); glVertex3f(MAP_MIN, ceilH, MAP_MAX);
    glEnd();
    
    for (float bx = MAP_MIN; bx <= MAP_MAX + cofSz; bx += cofSz) {
        glPushMatrix(); glTranslatef(bx, ceilH - cofDep * 0.5f, (MAP_MIN + MAP_MAX) * 0.5f);
        glScalef(cofBdr, cofDep, MAP_MAX - MAP_MIN + 1.0f);
        if (lightsOn) glColor3f(0.52f, 0.48f, 0.40f);
        else { float b = 0.04f * flickerIntensity; glColor3f(b * 0.8f, b * 0.75f, b); }
        glutSolidCube(1); glPopMatrix();
    }
    for (float bz = MAP_MIN; bz <= MAP_MAX + cofSz; bz += cofSz) {
        glPushMatrix(); glTranslatef((MAP_MIN + MAP_MAX) * 0.5f, ceilH - cofDep * 0.5f, bz);
        glScalef(MAP_MAX - MAP_MIN + 1.0f, cofDep, cofBdr);
        if (lightsOn) glColor3f(0.52f, 0.48f, 0.40f);
        else { float b = 0.04f * flickerIntensity; glColor3f(b * 0.8f, b * 0.75f, b); }
        glutSolidCube(1); glPopMatrix();
    }
}
// ===================================================
// FIXED RENDERING: TEKSTUR DI TENGAH DINDING SAMPING
// ===================================================
void drawWall(float x, float z, float sx, float sz) {
    float wallH = 3.5f; 
    float wainH = wallH * 0.35f; 
    float halfX = sx / 2.0f;
    float halfZ = sz / 2.0f;

    float tileX = sx / 3.5f;
    float tileZ = sz / 3.5f;

    // 1. GAMBAR BAGIAN BAWAH DINDING (POLOS COKELAT)
    if (lightsOn) {
        glColor3f(0.30f, 0.28f, 0.25f);
    } else {
        glColor3f(0.05f * flickerIntensity, 0.04f * flickerIntensity, 0.06f * flickerIntensity);
    }

    glBegin(GL_QUADS);
    // Depan bawah
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(x - halfX, 0.0f,  z + halfZ); glVertex3f(x + halfX, 0.0f,  z + halfZ);
    glVertex3f(x + halfX, wainH, z + halfZ); glVertex3f(x - halfX, wainH, z + halfZ);
    // Belakang bawah
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(x - halfX, 0.0f,  z - halfZ); glVertex3f(x - halfX, wainH, z - halfZ);
    glVertex3f(x + halfX, wainH, z - halfZ); glVertex3f(x + halfX, 0.0f,  z - halfZ);
    // Kiri bawah
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(x - halfX, 0.0f,  z - halfZ); glVertex3f(x - halfX, 0.0f,  z + halfZ);
    glVertex3f(x - halfX, wainH, z + halfZ); glVertex3f(x - halfX, wainH, z - halfZ);
    // Kanan bawah
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(x + halfX, 0.0f,  z - halfZ); glVertex3f(x + halfX, wainH, z - halfZ);
    glVertex3f(x + halfX, wainH, z + halfZ); glVertex3f(x + halfX, 0.0f,  z + halfZ);
    glEnd();

    // 2. GAMBAR LIS GARIS KUNING / EMAS DI TENGAH DINDING
    float trimH = 0.07f; 
    if (lightsOn) {
        glColor3f(0.55f, 0.50f, 0.42f);
    } else {
        glColor3f(0.15f * flickerIntensity, 0.13f * flickerIntensity, 0.10f * flickerIntensity);
    }

    glBegin(GL_QUADS);
    // Garis depan
    glVertex3f(x - halfX, wainH, z + halfZ);         glVertex3f(x + halfX, wainH, z + halfZ);
    glVertex3f(x + halfX, wainH + trimH, z + halfZ); glVertex3f(x - halfX, wainH + trimH, z + halfZ);
    // Garis belakang
    glVertex3f(x - halfX, wainH, z - halfZ);         glVertex3f(x - halfX, wainH + trimH, z - halfZ);
    glVertex3f(x + halfX, wainH + trimH, z - halfZ); glVertex3f(x + halfX, wainH, z - halfZ);
    // Garis kiri
    glVertex3f(x - halfX, wainH, z - halfZ);         glVertex3f(x - halfX, wainH, z + halfZ);
    glVertex3f(x - halfX, wainH + trimH, z + halfZ); glVertex3f(x - halfX, wainH + trimH, z - halfZ);
    // Garis kanan
    glVertex3f(x + halfX, wainH, z - halfZ);         glVertex3f(x + halfX, wainH + trimH, z - halfZ);
    glVertex3f(x + halfX, wainH + trimH, z + halfZ); glVertex3f(x + halfX, wainH, z + halfZ);
    glEnd();

    // 3. GAMBAR BAGIAN ATAS DINDING (BERMOTIF BERJEJER SURAT / GRAFITI)
    float topStart = wainH + trimH; 

    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // Penting agar gambar merespons senter

    // --- LOGIKA ACAK UNTUK PILIH GAMBAR GRAFITI HOROR ---
    GLuint textureDindingSkarang = wallTextureId; // Default pake dinding ori bawaan labirin
    int checker = (int)(abs(x) * 13 + abs(z) * 7); // Benih pengacak dari posisi koordinat koordinat x dan z

    // Menggunakan modulo angka prima agar kemunculannya langka dan acak di lorong labirin
    if (checker % 11 == 2) {
        textureDindingSkarang = scareTexId1; // Memunculkan wall1.bmp ("YOU CANT ESCAPE")
    } else if (checker % 13 == 4) {
        textureDindingSkarang = scareTexId2; // Memunculkan wall2.bmp ("TRYING TO HIDE?")
    } else if (checker % 17 == 5) {
        textureDindingSkarang = scareTexId3; // Memunculkan wall3.bmp ("RUN FROM THE LIGHT!")
    }

    glBindTexture(GL_TEXTURE_2D, textureDindingSkarang);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Pewarnaan permukaan agar warna grafiti merah menyala tajam dan responsif saat senter menyala/redup
    if (lightsOn) {
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        glColor3f(flickerIntensity * 0.9f, flickerIntensity * 0.9f, flickerIntensity * 0.95f);
    }

    glBegin(GL_QUADS);
    // Sisi Depan atas (+Z)
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f,  0.0f); glVertex3f(x - halfX, topStart, z + halfZ);
    glTexCoord2f(tileX, 0.0f); glVertex3f(x + halfX, topStart, z + halfZ);
    glTexCoord2f(tileX, 1.0f); glVertex3f(x + halfX, wallH,    z + halfZ);
    glTexCoord2f(0.0f,  1.0f); glVertex3f(x - halfX, wallH,    z + halfZ);

    // Sisi Belakang atas (-Z)
    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(tileX, 0.0f); glVertex3f(x - halfX, topStart, z - halfZ);
    glTexCoord2f(tileX, 1.0f); glVertex3f(x - halfX, wallH,    z - halfZ);
    glTexCoord2f(0.0f,  1.0f); glVertex3f(x + halfX, wallH,    z - halfZ);
    glTexCoord2f(0.0f,  0.0f); glVertex3f(x + halfX, topStart, z - halfZ);

    // Sisi Kiri atas (-X)
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f,  0.0f); glVertex3f(x - halfX, topStart, z - halfZ);
    glTexCoord2f(tileZ, 0.0f); glVertex3f(x - halfX, topStart, z + halfZ);
    glTexCoord2f(tileZ, 1.0f); glVertex3f(x - halfX, wallH,    z + halfZ);
    glTexCoord2f(0.0f,  1.0f); glVertex3f(x - halfX, wallH,    z - halfZ);

    // Sisi Kanan atas (+X)
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(tileZ, 0.0f); glVertex3f(x + halfX, topStart, z - halfZ);
    glTexCoord2f(tileZ, 1.0f); glVertex3f(x + halfX, wallH,    z - halfZ);
    glTexCoord2f(0.0f,  1.0f); glVertex3f(x + halfX, wallH,    z + halfZ);
    glTexCoord2f(0.0f,  0.0f); glVertex3f(x + halfX, topStart, z + halfZ);
    glEnd();

    // ── SISI ATAS PILAR (Kembalikan ke wallTextureId polos agar bagian atap pilar rapi) ──
    glBindTexture(GL_TEXTURE_2D, wallTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f,  tileZ); glVertex3f(x - halfX, wallH, z - halfZ);
    glTexCoord2f(0.0f,  0.0f);  glVertex3f(x - halfX, wallH, z + halfZ);
    glTexCoord2f(tileX, 0.0f);  glVertex3f(x + halfX, wallH, z + halfZ);
    glTexCoord2f(tileX, tileZ); glVertex3f(x + halfX, wallH, z - halfZ);
    glEnd();

    glDisable(GL_TEXTURE_2D); 
}

void drawMaze() {
    for (int i=0;i<wallCount;i++) drawWall(walls[i].x,walls[i].z,walls[i].sx,walls[i].sz);
}

void drawExit() {
    const float frameX  = 20.0f;
    const float doorZ   = exitZ;       // -12.0f
    const float doorW   = 1.5f;        // lebih lebar sesuai gap besar
    const float doorH   = 3.4f;
    const float depth   = 0.22f;
    const float doorShift = 0.0f;
    const float fLeft   = doorZ - doorW + doorShift;
    const float fRight  = doorZ + doorW + doorShift;

    float t     = (float)glutGet(GLUT_ELAPSED_TIME) / 800.0f;
    float pulse = 0.55f + 0.45f * sinf(t * 2.5f);

    // ── helper warna ──
    auto stoneCol = [&](float b) {
        if (lightsOn) glColor3f(b*0.62f, b*0.60f, b*0.55f);
        else glColor3f(b*0.22f*flickerIntensity, b*0.20f*flickerIntensity, b*0.18f*flickerIntensity);
    };
    auto doorEdgeCol = [&](float b) {
        if (lightsOn) glColor3f(b*0.34f, b*0.22f, b*0.12f);
        else glColor3f(b*0.14f*flickerIntensity, b*0.08f*flickerIntensity, b*0.04f*flickerIntensity);
    };

    // ==============================================
    // 1. LANTAI GLOW HIJAU
    // ==============================================
    glPushMatrix();
    glTranslatef(19.5f, 0.02f, doorZ);
    glColor3f(0, pulse*0.7f, pulse*0.3f);
    glBegin(GL_QUADS);
    glVertex3f(-0.3f,0,-doorW); glVertex3f(0.3f,0,-doorW);
    glVertex3f(0.3f, 0, doorW); glVertex3f(-0.3f,0, doorW);
    glEnd();
    glPopMatrix();

    // ==============================================
    // 2. PILAR BATU KIRI & KANAN
    // ==============================================
    const float pilW  = 0.42f;  // lebar pilar
    const float pilD  = 0.38f;  // kedalaman pilar

    for (int side = 0; side < 2; side++) {
        float pz = (side == 0) ? fLeft - pilW*0.5f : fRight + pilW*0.5f;
        int blocks = 6;
        float bh = doorH / blocks;

        for (int b = 0; b < blocks; b++) {
            float bv  = (b % 2 == 0) ? 0.78f : 0.62f;
            float by0 = b * bh;
            float by1 = (b+1) * bh;

            stoneCol(bv);
            // Muka depan pilar
            glBegin(GL_QUADS);
            glVertex3f(frameX,        by0, pz - pilW*0.5f);
            glVertex3f(frameX,        by1, pz - pilW*0.5f);
            glVertex3f(frameX,        by1, pz + pilW*0.5f);
            glVertex3f(frameX,        by0, pz + pilW*0.5f);
            glEnd();
            // Sisi luar pilar
            stoneCol(bv * 0.75f);
            glBegin(GL_QUADS);
            glVertex3f(frameX,        by0, pz + (side==0?-1:1)*pilW*0.5f);
            glVertex3f(frameX-pilD,   by0, pz + (side==0?-1:1)*pilW*0.5f);
            glVertex3f(frameX-pilD,   by1, pz + (side==0?-1:1)*pilW*0.5f);
            glVertex3f(frameX,        by1, pz + (side==0?-1:1)*pilW*0.5f);
            glEnd();
            // Garis mortar
            glColor3f(0.08f,0.07f,0.06f);
            glLineWidth(1.2f);
            glBegin(GL_LINE_LOOP);
            glVertex3f(frameX, by0, pz-pilW*0.5f);
            glVertex3f(frameX, by1, pz-pilW*0.5f);
            glVertex3f(frameX, by1, pz+pilW*0.5f);
            glVertex3f(frameX, by0, pz+pilW*0.5f);
            glEnd();
        }

        // Kepala pilar (capital) — blok lebih lebar
        stoneCol(0.90f);
        float capY0 = doorH - 0.05f;
        float capY1 = doorH + 0.22f;
        float capExtra = 0.10f;
        glBegin(GL_QUADS);
        glVertex3f(frameX+0.04f,  capY0, pz - pilW*0.5f - capExtra);
        glVertex3f(frameX+0.04f,  capY1, pz - pilW*0.5f - capExtra);
        glVertex3f(frameX+0.04f,  capY1, pz + pilW*0.5f + capExtra);
        glVertex3f(frameX+0.04f,  capY0, pz + pilW*0.5f + capExtra);
        glEnd();
    }

    // ==============================================
    // 5. DAUN PINTU TUNGGAL (berengsel di kiri = fLeft)
    // ==============================================
    glPushMatrix();
    glTranslatef(frameX + 0.01f, 0.0f, fLeft);
    glRotatef(doorAngle, 0, 1, 0);   // buka ke dalam (negatif Z)

    const float leafW = doorW * 2.0f;  // lebar penuh
    const float leafD = 0.09f;
    // Texture pintu keluar dipasang tepat di daun pintu.
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, doorTextureId);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (lightsOn) glColor3f(1.0f, 1.0f, 1.0f);
    else glColor3f(flickerIntensity * 0.85f, flickerIntensity * 0.85f, flickerIntensity * 0.9f);

    glBegin(GL_QUADS);
    // S texture coord dibalik supaya gambar pintu mirror horizontal.
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(0.0f,   0.0f,  0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f,   0.0f,  leafW);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0f,   doorH, leafW);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(0.0f,   doorH, 0.0f);

    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-leafD, 0.0f,  0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-leafD, 0.0f,  leafW);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-leafD, doorH, leafW);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-leafD, doorH, 0.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    doorEdgeCol(0.75f);
    glBegin(GL_QUADS);
    glVertex3f(0.0f,   0.0f,  0.0f);  glVertex3f(-leafD, 0.0f,  0.0f);
    glVertex3f(-leafD, doorH, 0.0f);  glVertex3f(0.0f,   doorH, 0.0f);

    glVertex3f(0.0f,   0.0f,  leafW); glVertex3f(0.0f,   doorH, leafW);
    glVertex3f(-leafD, doorH, leafW); glVertex3f(-leafD, 0.0f,  leafW);

    glVertex3f(0.0f,   doorH, 0.0f);  glVertex3f(-leafD, doorH, 0.0f);
    glVertex3f(-leafD, doorH, leafW); glVertex3f(0.0f,   doorH, leafW);
    glEnd();

    glPopMatrix(); // end door leaf
}
void bindCabinetTexture(GLuint textureId) {
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void drawCabinetHidingSpot() {
    const float scale = 1.5f;
    const float w = 0.9f * scale;
    const float h = 1.65f * scale;
    const float d = 0.58f * scale;
    const float halfW = w * 0.5f;
    const float halfD = d * 0.5f;

    glEnable(GL_TEXTURE_2D);
    glColor3f(0.56f, 0.28f, 0.11f);

    bindCabinetTexture(cabinetFrontTexId);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-halfW, 0.0f, halfD);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( halfW, 0.0f, halfD);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( halfW, h,    halfD);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-halfW, h,    halfD);
    glEnd();

    bindCabinetTexture(cabinetBackTexId);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f( halfW, 0.0f, -halfD);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-halfW, 0.0f, -halfD);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-halfW, h,    -halfD);
    glTexCoord2f(0.0f, 1.0f); glVertex3f( halfW, h,    -halfD);
    glEnd();

    bindCabinetTexture(cabinetSideTexId);
    glBegin(GL_QUADS);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-halfW, 0.0f, -halfD);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-halfW, 0.0f,  halfD);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-halfW, h,     halfD);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-halfW, h,    -halfD);

    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(halfW, 0.0f, -halfD);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(halfW, 0.0f,  halfD);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(halfW, h,     halfD);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(halfW, h,    -halfD);

    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-halfW, h, -halfD);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-halfW, h,  halfD);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( halfW, h,  halfD);
    glTexCoord2f(0.0f, 1.0f); glVertex3f( halfW, h, -halfD);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void drawHidingSpots() {
    float t=(float)glutGet(GLUT_ELAPSED_TIME)/1000.0f;
    for (int i=0;i<hidingCount;i++) {
        glPushMatrix();glTranslatef(hidingSpots[i].x,0,hidingSpots[i].z);
        float glow=0.25f+0.1f*sinf(t*2.0f+i);
        if(lightsOn)glColor3f(0.8f,0.7f,0);else glColor3f(glow*0.9f,glow*0.6f,0);
        glBegin(GL_QUADS);
        glVertex3f(-0.55f,0.01f,-0.45f);glVertex3f(0.55f,0.01f,-0.45f);
        glVertex3f(0.55f,0.01f,0.45f);glVertex3f(-0.55f,0.01f,0.45f);
        glEnd();
        drawCabinetHidingSpot();
        glPopMatrix();
    }
}

void drawCoin(float x,float y,float z,float radius,float thickness) {
    glPushMatrix();glTranslatef(x,y,z);
    int segs=12;
    glBegin(GL_TRIANGLE_FAN);glVertex3f(0,thickness/2,0);
    for(int s=0;s<=segs;s++){float a=s*2*(float)M_PI/segs;glVertex3f(cosf(a)*radius,thickness/2,sinf(a)*radius);}glEnd();
    glBegin(GL_TRIANGLE_FAN);glVertex3f(0,-thickness/2,0);
    for(int s=segs;s>=0;s--){float a=s*2*(float)M_PI/segs;glVertex3f(cosf(a)*radius,-thickness/2,sinf(a)*radius);}glEnd();
    glBegin(GL_QUAD_STRIP);
    for(int s=0;s<=segs;s++){float a=s*2*(float)M_PI/segs,cx=cosf(a)*radius,cz=sinf(a)*radius;
        glVertex3f(cx,-thickness/2,cz);glVertex3f(cx,thickness/2,cz);}glEnd();
    glPopMatrix();
}
void drawTreasure() {
    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 600.0f;
    for (int i = 0; i < treasureCount; i++) {
        if (treasureCollected[i]) continue;
        float hover = sinf(t * 1.5f + i * 2.0f) * 0.08f;
        float gp = 0.3f + 0.2f * sinf(t * 3 + i);

        glColor3f(gp, gp * 0.7f, 0);
        glPushMatrix(); glTranslatef(treasureX[i], 0.02f, treasureZ[i]);
        glBegin(GL_TRIANGLE_FAN); glVertex3f(0, 0, 0);
        for (int s = 0; s <= 20; s++) {
            float a = s * 2 * (float)M_PI / 20;
            float r = 0.8f + 0.15f * sinf(t * 4 + s);
            glVertex3f(cosf(a)*r, 0, sinf(a)*r);
        }
        glEnd(); glPopMatrix();

        glPushMatrix(); glTranslatef(treasureX[i], hover, treasureZ[i]);
        float gR = lightsOn ? 1.0f : 0.85f * flickerIntensity;
        float gG = lightsOn ? 0.78f : 0.65f * flickerIntensity;
        glColor3f(gR, gG, 0);
        drawCoin(0.15f, 0.04f, 0.10f, 0.18f, 0.06f);
        drawCoin(-0.20f, 0.04f, 0.05f, 0.16f, 0.06f);
        drawCoin(0.00f, 0.04f, -0.20f, 0.17f, 0.06f);
        drawCoin(0.25f, 0.04f, -0.05f, 0.15f, 0.06f);
        drawCoin(-0.10f, 0.04f, -0.15f, 0.18f, 0.06f);
        for (int k = 0; k < 5; k++)
            drawCoin(0, 0.10f + k * 0.08f, 0, 0.22f - k * 0.01f, 0.08f);
        float gl = 0.5f + 0.5f * sinf(t * 5 + i);
        glColor3f(1, 1, gl * 0.5f);
        glPushMatrix(); glTranslatef(0, 0.55f, 0); glutSolidSphere(0.06f, 8, 8); glPopMatrix();
        glPopMatrix();
    }
}
void drawEnemyBody(int i) {
    Enemy &e = enemies[i];
    float t = (float)glutGet(GLUT_ELAPSED_TIME) / 500.0f;
    float bob = sinf(t * 3 + i * 1.5f) * 0.06f;

    glPushMatrix();
    glTranslatef(e.x, bob, e.z);
    glRotatef(e.angle + 180.0f, 0, 1, 0);

    // ── BAYANGAN DI LANTAI ──
    glColor3f(0.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(0, 0.01f, 0);
    glScalef(1.0f, 0.02f, 0.8f);
    glutSolidSphere(0.5f, 8, 4);
    glPopMatrix();

    // ── JUBAH / BODY UTAMA (lebih tinggi, ramping) ──
    if (lightsOn) glColor3f(0.08f, 0.04f, 0.12f);
    else glColor3f(0.04f, 0.02f, 0.08f);
    glPushMatrix();
    glTranslatef(0, 1.0f, 0);
    glScalef(0.45f, 1.1f, 0.3f);
    glutSolidCube(1);
    glPopMatrix();

    // ── BAHU LEBAR ──
    if (lightsOn) glColor3f(0.10f, 0.05f, 0.15f);
    else glColor3f(0.05f, 0.02f, 0.10f);
    glPushMatrix();
    glTranslatef(0, 1.55f, 0);
    glScalef(0.75f, 0.18f, 0.32f);
    glutSolidCube(1);
    glPopMatrix();

    // ── KEPALA (elongated, lebih panjang ke atas) ──
    if (lightsOn) glColor3f(0.12f, 0.06f, 0.16f);
    else glColor3f(0.06f, 0.03f, 0.10f);
    glPushMatrix();
    glTranslatef(0, 1.95f, 0);
    glScalef(0.85f, 1.1f, 0.75f);
    glutSolidSphere(0.22f, 10, 10);
    glPopMatrix();

    // ── MATA MERAH MENYALA ──
    float eg = lightsOn ? 1.0f : (0.7f + 0.3f * sinf(t * 5 + i));
    float eyeFlare = 0.5f + 0.5f * sinf(t * 7 + i * 2.1f); // kedip acak
    glColor3f(eg, eg * eyeFlare * 0.1f, 0.0f);
    glPushMatrix(); glTranslatef( 0.09f, 2.0f, -0.19f); glutSolidSphere(0.06f, 8, 8); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.09f, 2.0f, -0.19f); glutSolidSphere(0.06f, 8, 8); glPopMatrix();

    // ── GLOW MATA (halo kecil) ──
    glColor3f(eg * 0.4f, 0.0f, 0.0f);
    glPushMatrix(); glTranslatef( 0.09f, 2.0f, -0.17f); glutSolidSphere(0.10f, 6, 6); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.09f, 2.0f, -0.17f); glutSolidSphere(0.10f, 6, 6); glPopMatrix();

    // ── MULUT / CELAH GELAP ──
    glColor3f(0.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(0, 1.82f, -0.19f);
    glScalef(0.18f, 0.04f, 0.04f);
    glutSolidCube(1);
    glPopMatrix();

    // ── TANDUK KECIL ──
    if (lightsOn) glColor3f(0.25f, 0.05f, 0.05f);
    else glColor3f(0.12f, 0.02f, 0.02f);
    glPushMatrix();
    glTranslatef( 0.08f, 2.22f, 0);
    glRotatef(-20, 0, 0, 1);
    glScalef(0.05f, 0.18f, 0.05f);
    glutSolidCone(1, 1, 6, 1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-0.08f, 2.22f, 0);
    glRotatef( 20, 0, 0, 1);
    glScalef(0.05f, 0.18f, 0.05f);
    glutSolidCone(1, 1, 6, 1);
    glPopMatrix();

    // ── TANGAN PANJANG (cakar) ──
    float armSw = sinf(t * 3 + i * 1.5f) * 20.0f;
    if (lightsOn) glColor3f(0.10f, 0.05f, 0.14f);
    else glColor3f(0.05f, 0.02f, 0.08f);
    glPushMatrix();
    glTranslatef(-0.38f, 1.4f, 0);
    glRotatef(armSw + 15, 1, 0, 0);
    glTranslatef(0, -0.3f, 0);
    glScalef(0.12f, 0.65f, 0.12f);
    glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef( 0.38f, 1.4f, 0);
    glRotatef(-armSw + 15, 1, 0, 0);
    glTranslatef(0, -0.3f, 0);
    glScalef(0.12f, 0.65f, 0.12f);
    glutSolidCube(1);
    glPopMatrix();

    // ── CAKAR (ujung tangan) ──
    glColor3f(0.35f, 0.05f, 0.05f);
    float clawOffL[3] = {-0.05f, 0.0f, 0.05f};
    for (int c = 0; c < 3; c++) {
        glPushMatrix();
        glTranslatef(-0.38f + clawOffL[c], 0.75f + sinf(t*3+i*1.5f+armSw*0.01f)*0.3f, -0.1f);
        glScalef(0.03f, 0.12f, 0.03f);
        glutSolidCone(1, 1, 5, 1);
        glPopMatrix();
        glPushMatrix();
        glTranslatef( 0.38f + clawOffL[c], 0.75f - sinf(t*3+i*1.5f+armSw*0.01f)*0.3f, -0.1f);
        glScalef(0.03f, 0.12f, 0.03f);
        glutSolidCone(1, 1, 5, 1);
        glPopMatrix();
    }

    // ── KAKI ──
    float legSw = sinf(t * 3 + i * 1.5f + (float)M_PI) * 22.0f;
    if (lightsOn) glColor3f(0.06f, 0.03f, 0.10f);
    else glColor3f(0.03f, 0.01f, 0.06f);
    glPushMatrix();
    glTranslatef(-0.16f, 0.45f, 0);
    glRotatef(legSw, 1, 0, 0);
    glTranslatef(0, -0.32f, 0);
    glScalef(0.18f, 0.65f, 0.18f);
    glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef( 0.16f, 0.45f, 0);
    glRotatef(-legSw, 1, 0, 0);
    glTranslatef(0, -0.32f, 0);
    glScalef(0.18f, 0.65f, 0.18f);
    glutSolidCube(1);
    glPopMatrix();

    glPopMatrix();
}
void drawEnemies() { for(int i=0;i<8;i++) drawEnemyBody(i); }

void updateCamera() {
    float rad=playerAngle*(float)M_PI/180.0f;
    float sway=sinf(breathTimer*2.5f)*0.018f;
    gluLookAt(playerX,eyeHeight+sway,playerZ,
              playerX+sinf(rad)*lookDistance,eyeHeight+sway,playerZ+cosf(rad)*lookDistance,
              0,1,0);
}
