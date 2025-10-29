#ifndef ASSETS_H
#define ASSETS_H
#include "raylib.h"
#pragma once

typedef struct Assets {
    Texture2D texPlayerRight;
    Texture2D texPlayerLeft;
    Texture2D texPlayerUp;
    Texture2D texPlayerDown;
    Texture2D texRival, texBerry, texStick, texPond, texClue;

    Texture2D uiHeart, uiFood, uiWater;
    // --- SFX ---
    Sound sPickupFood;
    Sound sPickupStick;
    Sound sDrink;
    Sound sClue;
    Sound sCraft;

    // --- Background music (streamed) ---
    Music bgDay;
    Music bgNight;
} Assets;

void Assets_Load(Assets* a);   // tries PNGs in ./assets; falls back to generated textures
void Assets_Unload(Assets* a);

#endif
#pragma once
