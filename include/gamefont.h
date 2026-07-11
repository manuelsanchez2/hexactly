#pragma once

#include "raylib.h"

void loadFonts();
void unloadFonts();

Font& titleFont();
Font& markerFont();

Texture2D tickTexture();
Texture2D primaryButtonTexture();
Texture2D secondaryButtonTexture();
Texture2D squareButtonTexture();
Texture2D levelSquareTexture();
Texture2D hexTileTexture();
Texture2D flagGoalTexture();
Texture2D portalATexture();
Texture2D portalBTexture();
Texture2D bgTitleTexture();
Texture2D logoSheetTexture();
Texture2D btnInfoTexture();
Texture2D btnPauseTexture();
Texture2D btnUndoTexture();
Texture2D btnDailyTexture();
Texture2D btnDailyTickTexture();

Vector2 titleMeasure(const char *text, float size);
void    titleDraw(const char *text, float x, float y, float size, Color color);
void    titleDrawCentered(const char *text, float y, float size, Color color);
void    titleDrawCenteredAt(const char *text, float cx, float cy, float size, Color color);
void    titleDrawCenteredAtRot(const char *text, float cx, float cy, float size, float rotDeg, Color color);
