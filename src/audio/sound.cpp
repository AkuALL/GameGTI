#include "../../include/audio.h"
#include "../../include/game_state.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include <process.h>
#pragma comment(lib, "winmm.lib")
#endif

#include <cstring>

void stopSound() {
#ifdef _WIN32
    bgmPlaying = false;
    currentBGM = nullptr;
    PlaySoundA(NULL, NULL, 0);
#endif
}

void resumeBGM() {
#ifdef _WIN32
    if (currentBGM) {
        bgmPlaying = true;
        PlaySoundA(currentBGM, NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
    }
#endif
}

// Thread untuk SFX agar tidak freeze game
#ifdef _WIN32
struct SFXParam { char filename[256]; };

unsigned __stdcall sfxThread(void* arg) {
    SFXParam* p = (SFXParam*)arg;
    PlaySoundA(p->filename, NULL, SND_FILENAME | SND_SYNC);
    delete p;
    return 0;
}
#endif

void playBGM(const char* filename) {
#ifdef _WIN32
    currentBGM = filename;
    bgmPlaying = true;
    PlaySoundA(filename, NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
#endif
}

void playSFX(const char* filename) {
#ifdef _WIN32
    // Stop BGM dulu agar SFX terdengar jelas
    PlaySoundA(NULL, NULL, 0);

    SFXParam* p = new SFXParam();
    strncpy(p->filename, filename, 255);
    p->filename[255] = '\0';

    HANDLE h = (HANDLE)_beginthreadex(NULL, 0, sfxThread, p, 0, NULL);
    if (h) CloseHandle(h);
#endif
}
