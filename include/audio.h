#pragma once

void audioInit();
void audioShutdown();
void audioUpdate();

void audioLoadSettings();
void audioSaveSettings();

void playMerge();
void playInvalid();
void playWin();

void  audioSetMusicVolume(float v);
float audioGetMusicVolume();
void  audioSetSfxVolume(float v);
float audioGetSfxVolume();
