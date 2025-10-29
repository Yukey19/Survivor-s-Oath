#include "assets.h"
#include "raylib.h"
#include "raymath.h"
#include <math.h>   // sinf, PI
#include <stdio.h>  // snprintf

// Load a sprite with guaranteed RGBA, premultiplied alpha, and crisp filtering
static Texture2D LoadSprite(const char* path) {
    Image img = LoadImage(path);
    if (!img.data) { TraceLog(LOG_ERROR, "FAILED to load %s", path); return (Texture2D) { 0 }; }

    // Ensure RGBA
    if (img.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    // Premultiply to avoid white/black fringes on transparency
    ImageAlphaPremultiply(&img);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);

    // Pixel art friendly (no blurry halos) + no tiling seams
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    SetTextureWrap(tex, TEXTURE_WRAP_CLAMP);

    TraceLog(LOG_INFO, "Loaded %s (%dx%d)", path, tex.width, tex.height);
    return tex;
}


static void Pix(Texture2D* t) {
    if (t && t->id) SetTextureFilter(*t, TEXTURE_FILTER_POINT); 
}

static Texture2D LoadIfExists(const char* path) {
    if (FileExists(path)) return LoadTexture(path);
    return (Texture2D) { 0 }; // width==0 means missing
}

static Image GenMoodyGrassTile(int s) {
    Image img = GenImageColor(s, s, (Color) { 20, 27, 32, 255 });          // dark base
    // scatter a few darker pixels for texture
    for (int i = 0; i < s * s / 6; ++i) {
        int x = GetRandomValue(0, s - 1), y = GetRandomValue(0, s - 1);
        ImageDrawPixel(&img, x, y, (Color) { 15, 22, 28, 255 });
    }
    // a couple of lighter blades
    for (int i = 0; i < s / 2; ++i) {
        int x = GetRandomValue(0, s - 1), y = GetRandomValue(0, s - 1);
        ImageDrawPixel(&img, x, y, (Color) { 35, 45, 55, 255 });
    }
    return img;
}

static Image GenCircleImage(int size, Color fill, Color bg) {
    Image img = GenImageColor(size, size, bg);
    ImageDrawCircle(&img, size / 2, size / 2, size / 2 - 2, fill);
    ImageDrawCircleLines(&img, size / 2, size / 2, size / 2 - 2, BLACK);
    return img;
}

static Sound LoadSndIfExists(const char* path) {
    if (FileExists(path)) return LoadSound(path);
    Sound s = (Sound){ 0 };
    return s;
}

// Generate short mono 16-bit sine beep and return as Sound
static Sound GenBeep(float freq, float seconds, float volume) {
    const int sampleRate = 44100;
    const int channels = 1;
    const int frames = (int)(seconds * sampleRate);

    short* pcm = (short*)MemAlloc(frames * channels * sizeof(short));
    for (int i = 0; i < frames; ++i) {
        float t = (float)i / sampleRate;
        float v = sinf(2.0f * PI * freq * t) * volume;
        if (v > 1.0f) v = 1.0f; if (v < -1.0f) v = -1.0f;
        pcm[i] = (short)(v * 32767.0f);
    }
    Wave w = { (unsigned)frames, 44100, 16, 1, pcm };
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w); // frees pcm
    return s;
}

// Try assets/<name>.ogg/.mp3/.wav in that order
static Music LoadMusicIfExists(const char* baseName) {
    const char* exts[] = { ".ogg", ".mp3", ".wav" };
    char path[128];
    for (int i = 0; i < 3; ++i) {
        snprintf(path, sizeof(path), "assets/%s%s", baseName, exts[i]);
        if (FileExists(path)) return LoadMusicStream(path);
    }
    Music m = (Music){ 0 };
    return m;
}


static Image GenPond(int size) {
    Image img = GenImageColor(size, size, (Color) { 40, 58, 44, 255 });
    ImageDrawCircle(&img, size / 2, size / 2, size / 2 - 2, (Color) { 40, 120, 180, 255 });
    return img;
}

void Assets_Load(Assets* a) {

    // Try to load from disk
    a->texPlayerRight = LoadTexture("assets/player_right.png");
    a->texPlayerLeft = LoadTexture("assets/player_left.png");
    a->texPlayerUp = LoadTexture("assets/player_up.png");
    a->texPlayerDown = LoadTexture("assets/player_down.png");
    a->texRival = LoadIfExists("assets/rival.png");
    a->texBerry = LoadIfExists("assets/berry.png");
    a->texStick = LoadIfExists("assets/stick.png");
    a->texPond = LoadIfExists("assets/pond.png");
    a->texClue = LoadIfExists("assets/clue.png");
    a->uiHeart = LoadIfExists("assets/ui_heart.png");
    a->uiFood = LoadIfExists("assets/ui_food.png");
    a->uiWater = LoadIfExists("assets/ui_water.png");


    // Generate placeholders where missing
    // --- Generate placeholders for missing player direction sprites ---
    if (a->texPlayerRight.width == 0) {
        Image i = GenCircleImage(24, (Color) { 255, 255, 255, 255 }, (Color) { 0, 0, 0, 0 });
        ImageDrawRectangle(&i, 12, 8, 10, 8, (Color) { 120, 180, 255, 255 });
        a->texPlayerRight = LoadTextureFromImage(i);
        UnloadImage(i);
    }

    if (a->texPlayerLeft.width == 0) {
        Image i = GenCircleImage(24, (Color) { 255, 255, 255, 255 }, (Color) { 0, 0, 0, 0 });
        ImageDrawRectangle(&i, 2, 8, 10, 8, (Color) { 120, 180, 255, 255 });
        a->texPlayerLeft = LoadTextureFromImage(i);
        UnloadImage(i);
    }

    if (a->texPlayerUp.width == 0) {
        Image i = GenCircleImage(24, (Color) { 255, 255, 255, 255 }, (Color) { 0, 0, 0, 0 });
        ImageDrawRectangle(&i, 7, 2, 10, 8, (Color) { 120, 180, 255, 255 });
        a->texPlayerUp = LoadTextureFromImage(i);
        UnloadImage(i);
    }

    if (a->texPlayerDown.width == 0) {
        Image i = GenCircleImage(24, (Color) { 255, 255, 255, 255 }, (Color) { 0, 0, 0, 0 });
        ImageDrawRectangle(&i, 7, 12, 10, 8, (Color) { 120, 180, 255, 255 });
        a->texPlayerDown = LoadTextureFromImage(i);
        UnloadImage(i);
    }
    if (a->texRival.width == 0) { Image i = GenCircleImage(26, (Color) { 200, 60, 60, 255 }, (Color) { 0, 0, 0, 0 }); a->texRival = LoadTextureFromImage(i); UnloadImage(i); }
    if (a->texBerry.width == 0) { Image i = GenCircleImage(20, (Color) { 180, 40, 60, 255 }, (Color) { 0, 0, 0, 0 }); a->texBerry = LoadTextureFromImage(i); UnloadImage(i); }
    if (a->texStick.width == 0) { Image i = GenImageColor(18, 18, (Color) { 0, 0, 0, 0 }); ImageDrawRectangle(&i, 7, 2, 4, 14, (Color) { 120, 80, 60, 255 }); a->texStick = LoadTextureFromImage(i); UnloadImage(i); }
    if (a->texPond.width == 0) { Image i = GenPond(56); a->texPond = LoadTextureFromImage(i); UnloadImage(i); }
    if (a->texClue.width == 0) { Image i = GenCircleImage(22, (Color) { 230, 230, 40, 230 }, (Color) { 0, 0, 0, 0 }); a->texClue = LoadTextureFromImage(i); UnloadImage(i); }
    if (a->uiHeart.width == 0) { Image i = GenImageColor(20, 20, (Color) { 0, 0, 0, 0 }); ImageDrawRectangle(&i, 4, 6, 12, 10, RED); a->uiHeart = LoadTextureFromImage(i); UnloadImage(i); }
    if (a->uiFood.width == 0) { Image i = GenImageColor(20, 20, (Color) { 0, 0, 0, 0 }); ImageDrawRectangle(&i, 6, 6, 8, 8, (Color) { 200, 120, 50, 255 }); a->uiFood = LoadTextureFromImage(i); UnloadImage(i); }
    if (a->uiWater.width == 0) { Image i = GenImageColor(20, 20, (Color) { 0, 0, 0, 0 }); ImageDrawCircle(&i, 10, 10, 7, (Color) { 50, 140, 220, 255 }); a->uiWater = LoadTextureFromImage(i); UnloadImage(i); }
   
    Pix(&a->texPlayerRight);
    Pix(&a->texPlayerLeft);
    Pix(&a->texPlayerUp);
    Pix(&a->texPlayerDown);
    Pix(&a->texRival);
    Pix(&a->texBerry);
    Pix(&a->texStick);
    Pix(&a->texPond);
    Pix(&a->texClue);   
    Pix(&a->uiHeart);
    Pix(&a->uiFood);
    Pix(&a->uiWater);


    // ---------- SFX ----------
    a->sPickupFood = LoadSndIfExists("assets/pickup_food.wav");
    a->sPickupStick = LoadSndIfExists("assets/pickup_stick.wav");
    a->sDrink = LoadSndIfExists("assets/drink.wav");
    a->sClue = LoadSndIfExists("assets/clue.wav");
    a->sCraft = LoadSndIfExists("assets/craft.wav");

    // Fallback beeps if files are missing
    if (a->sPickupFood.frameCount == 0) a->sPickupFood = GenBeep(880.0f, 0.07f, 0.45f);
    if (a->sPickupStick.frameCount == 0) a->sPickupStick = GenBeep(660.0f, 0.07f, 0.45f);
    if (a->sDrink.frameCount == 0) a->sDrink = GenBeep(520.0f, 0.10f, 0.40f);
    if (a->sClue.frameCount == 0) a->sClue = GenBeep(980.0f, 0.09f, 0.50f);
    if (a->sCraft.frameCount == 0) a->sCraft = GenBeep(180.0f, 0.10f, 0.55f); // low thunk

    SetSoundVolume(a->sPickupFood, 0.55f);
    SetSoundVolume(a->sPickupStick, 0.55f);
    SetSoundVolume(a->sDrink, 0.60f);
    SetSoundVolume(a->sClue, 0.65f);
    SetSoundVolume(a->sCraft, 0.70f);

    // ---------- Background music ----------
    a->bgDay = LoadMusicIfExists("bg_day");   // assets/bg_day.(ogg|mp3|wav)
    a->bgNight = LoadMusicIfExists("bg_night"); // assets/bg_night.(ogg|mp3|wav)
    if (a->bgDay.ctxData) { a->bgDay.looping = true; SetMusicVolume(a->bgDay, 0.0f); }
    if (a->bgNight.ctxData) { a->bgNight.looping = true; SetMusicVolume(a->bgNight, 0.0f); }

}

void Assets_Unload(Assets* a) {
    UnloadTexture(a->texPlayerRight);
    UnloadTexture(a->texPlayerLeft);
    UnloadTexture(a->texPlayerUp);
    UnloadTexture(a->texPlayerDown);
    UnloadTexture(a->texRival); UnloadTexture(a->texBerry);
    UnloadTexture(a->texStick);  UnloadTexture(a->texPond);  UnloadTexture(a->texClue);
    UnloadTexture(a->uiHeart);   UnloadTexture(a->uiFood);   UnloadTexture(a->uiWater);

    

    // SFX
    if (a->sPickupFood.frameCount)  UnloadSound(a->sPickupFood);
    if (a->sPickupStick.frameCount) UnloadSound(a->sPickupStick);
    if (a->sDrink.frameCount)       UnloadSound(a->sDrink);
    if (a->sClue.frameCount)        UnloadSound(a->sClue);
    if (a->sCraft.frameCount)       UnloadSound(a->sCraft);

    // Music
    if (a->bgDay.ctxData)   UnloadMusicStream(a->bgDay);
    if (a->bgNight.ctxData) UnloadMusicStream(a->bgNight);
}
