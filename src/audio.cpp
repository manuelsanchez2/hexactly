#include "audio.h"
#include "storage.h"
#include "raylib.h"
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>

static const int SR = 44100;

static Music music;
static Sound  sMerge, sInvalid, sWin;
static bool   loaded = false;

static float musicVol = 0.6f;
static float sfxVol   = 0.8f;

static float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

static Sound makeSweep(float f0, float f1, float dur, float amp) {
    int frames = (int)(SR * dur);
    short* d = (short*)malloc(sizeof(short) * frames);
    float phase = 0.0f;
    for (int i = 0; i < frames; i++) {
        float prog = (float)i / frames;
        float env  = (prog < 0.1f) ? prog / 0.1f : (1.0f - prog);
        float f    = f0 + (f1 - f0) * prog;
        phase += 2.0f * PI * f / SR;
        d[i] = (short)(sinf(phase) * amp * env * 32767);
    }
    Wave w; w.frameCount = (unsigned)frames; w.sampleRate = SR;
    w.sampleSize = 16; w.channels = 1; w.data = d;
    Sound s = LoadSoundFromWave(w);
    free(d);
    return s;
}

static Sound makeJingle(const float* freqs, int n, float noteDur, float amp) {
    int per = (int)(SR * noteDur);
    int frames = per * n;
    short* d = (short*)malloc(sizeof(short) * frames);
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < per; i++) {
            float prog = (float)i / per;
            float env  = (prog < 0.1f) ? prog / 0.1f : (1.0f - prog);
            float t    = (float)i / SR;
            d[k * per + i] = (short)(sinf(2.0f * PI * freqs[k] * t) * amp * env * 32767);
        }
    }
    Wave w; w.frameCount = (unsigned)frames; w.sampleRate = SR;
    w.sampleSize = 16; w.channels = 1; w.data = d;
    Sound s = LoadSoundFromWave(w);
    free(d);
    return s;
}

void audioInit() {
    music = LoadMusicStream("resources/music.mp3");
    music.looping = true;

    sMerge   = makeSweep(520.0f, 1040.0f, 0.12f, 0.40f);
    sInvalid = makeSweep(200.0f,  120.0f, 0.12f, 0.40f);
    static const float notes[] = { 523.0f, 659.0f, 784.0f, 1047.0f };
    sWin = makeJingle(notes, 4, 0.12f, 0.40f);
    loaded = true;

    audioLoadSettings();
    audioSetMusicVolume(musicVol);
    audioSetSfxVolume(sfxVol);
    PlayMusicStream(music);
}

void audioLoadSettings() {
    std::string data = storageRead("settings");
    if (data.empty()) return;
    std::istringstream in(data);
    std::string line;
    while (std::getline(in, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        float v = (float)atof(line.substr(eq + 1).c_str());
        if      (key == "music") musicVol = clamp01(v);
        else if (key == "sfx")   sfxVol   = clamp01(v);
    }
}

void audioSaveSettings() {
    std::ostringstream out;
    out << "music=" << musicVol << "\n";
    out << "sfx="   << sfxVol   << "\n";
    storageWrite("settings", out.str());
}

void audioShutdown() {
    if (!loaded) return;
    UnloadMusicStream(music);
    UnloadSound(sMerge);
    UnloadSound(sInvalid);
    UnloadSound(sWin);
    loaded = false;
}

void audioUpdate() {
    if (loaded && IsMusicStreamPlaying(music)) UpdateMusicStream(music);
}

void audioSetMusicVolume(float v) {
    musicVol = clamp01(v);
    if (loaded) SetMusicVolume(music, musicVol);
}
float audioGetMusicVolume() { return musicVol; }

void audioSetSfxVolume(float v) {
    sfxVol = clamp01(v);
    if (loaded) {
        SetSoundVolume(sMerge,   sfxVol);
        SetSoundVolume(sInvalid, sfxVol);
        SetSoundVolume(sWin,     sfxVol);
    }
}
float audioGetSfxVolume() { return sfxVol; }

void playMerge()   { if (loaded) PlaySound(sMerge); }
void playInvalid() { if (loaded) PlaySound(sInvalid); }
void playWin()     { if (loaded) PlaySound(sWin); }
