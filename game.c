#include "game.h"
#include "raylib.h"
#include "raymath.h"
#include "player.h"
#include "rival.h"
#include "assets.h"
#include "world.h"
#include "ui.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

static void Game_SpawnNodes(Game* g);

static void CamFollow(Game* g, float dt)
{
    // --- Smooth follow toward player position ---
    float k = 8.0f; // smoothing factor
    float s = 1.0f - expf(-k * GetFrameTime());

    // offset camera target to center on scaled player
    Vector2 target = (Vector2){
        g->player->pos.x,
        g->player->pos.y - (12.0f * g->player->scale) // adjust vertical center for player size
    };

    g->cam.target = Vector2Lerp(g->cam.target, target, s);
    g->cam.offset = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    g->cam.zoom = 1.2f;  // keep your preferred zoom level
}



void Game_AddPop(Game* g, Vector2 worldPos, Color color, const char* msg) {
    if (g->popCount >= MAX_POPS) {
        g->pops[0] = g->pops[g->popCount - 1];
        g->popCount--;
    }
    PopFX* p = &g->pops[g->popCount++];
    p->pos = worldPos;
    p->t = 0.0f;
    p->color = color;

#if defined(_MSC_VER)
    strncpy_s(p->msg, sizeof(p->msg), msg, _TRUNCATE);  // MSVC-safe
#else
    snprintf(p->msg, sizeof(p->msg), "%s", msg);
#endif
}

void Game_Init(Game* g, Assets* assets) {
    srand((unsigned)time(NULL));
    g->assets = assets;

    // general gameplay resets
    g->popCount = 0;
    g->lightRadius = 180.0f;
    g->hitFlash = 0.0f;
    g->shakeTime = 0.0f;

    // intro menu setup
    g->menuIndex = 0;
    g->showHelp = false;
    g->introAlpha = 0.0f;
    g->introTimer = 0.0f;
    g->storyTimer = 0.0f;
    g->storyActive = false;
    g->storyIndex = 0;
    g->state = STATE_INTRO;

    // progression
    g->cluesCollected = 0;
    g->totalCluesRequired = 4;

    // world + camera
    g->cam = (Camera2D){ 0 };
    g->cam.zoom = 2.0f;
    Game_SpawnNodes(g);

    g->player = Player_Create((Vector2) { WORLD_W / 2.0f, WORLD_H / 2.0f });
    g->rival = Rival_Create((Vector2) { 300, 300 });

    // time cycle
    g->timeOfDay = 0.20f;
    g->forceNight = false;
    g->todTarget = 0.70f;
    g->todBlend = 0.0f;


    // music setup
    float night = Game_IsNight(g);
    g->musicDayVol = (1.0f - night) * 0.8f;
    g->musicNightVol = night * 0.6f;

    if (g->assets->bgDay.ctxData) {
        PlayMusicStream(g->assets->bgDay);
        SetMusicVolume(g->assets->bgDay, g->musicDayVol);
    }
    if (g->assets->bgNight.ctxData) {
        PlayMusicStream(g->assets->bgNight);
        SetMusicVolume(g->assets->bgNight, g->musicNightVol);
    }
}

void Game_Shutdown(Game* g) {
    Player_Destroy(g->player);
    Rival_Destroy(g->rival);
}

float Game_IsNight(const Game* g) {
    return (g->timeOfDay > 0.45f && g->timeOfDay < 0.85f) ? 1.0f : 0.0f;
}

static void HandleGlobalShortcuts(Game* g) {
    if (IsKeyPressed(KEY_GRAVE)) g->quitRequested = true; // tilde = quick exit (dev)
}

void Game_SpawnNodes(Game* g) {
    g->nodeCount = 0;
    World_SpawnScatter(g->nodes, &g->nodeCount, MAX_NODES, NODE_BERRY, 22);
    World_SpawnScatter(g->nodes, &g->nodeCount, MAX_NODES, NODE_POND, 6);
    World_SpawnScatter(g->nodes, &g->nodeCount, MAX_NODES, NODE_STICK, 18);
    World_SpawnScatter(g->nodes, &g->nodeCount, MAX_NODES, NODE_CLUE, 4);
}

void Game_Update(Game* g, float dt) {
    HandleGlobalShortcuts(g);

    switch (g->state) {
    case STATE_INTRO: {
        g->introTimer += dt;

        if (g->introAlpha < 1.0f) {
            g->introAlpha += dt * 2.0f;
            if (g->introAlpha > 1.0f) g->introAlpha = 1.0f;
        }

        if (!g->showHelp) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
                g->menuIndex = (g->menuIndex + 2) % 3;
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
                g->menuIndex = (g->menuIndex + 1) % 3;

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (g->menuIndex == 0) {
                    g->cluesCollected = 0;
                    g->timeOfDay = 0.20f;
                    g->forceNight = false;
                    g->todBlend = 0.0f;
                    g->todTarget = 0.70f;
                    g->state = STATE_STORY;
                    g->storyIndex = 0;
                    g->storyTimer = 0.0f;
                }
                else if (g->menuIndex == 1) {
                    g->showHelp = true;
                }
                else if (g->menuIndex == 2) {
                    g->quitRequested = true;
                }
            }
        }
        else {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
                g->showHelp = false;
        }

        if (IsKeyPressed(KEY_ESCAPE) && !g->showHelp)
            g->quitRequested = true;
    } break;

    case STATE_STORY: {
        g->storyTimer += dt;

        if (IsKeyPressed(KEY_ENTER) && g->storyTimer > 0.20f) {
            g->storyTimer = 0.0f;
            g->storyIndex++;

            const int STORY_LINE_COUNT = 4;
            if (g->storyIndex >= STORY_LINE_COUNT) {
                g->state = STATE_PLAYING;
            }
        }

        if (IsKeyPressed(KEY_ESCAPE))
            g->quitRequested = true;
    } break;

    case STATE_PLAYING: {
        // music update
        if (g->assets->bgDay.ctxData)   UpdateMusicStream(g->assets->bgDay);
        if (g->assets->bgNight.ctxData) UpdateMusicStream(g->assets->bgNight);

        float night = Game_IsNight(g);
        float targetDay = (1.0f - night) * 0.8f;
        float targetNight = night * 0.6f;

        g->musicDayVol = Lerp(g->musicDayVol, targetDay, dt * 2.0f);
        g->musicNightVol = Lerp(g->musicNightVol, targetNight, dt * 2.0f);

        if (g->assets->bgDay.ctxData)   SetMusicVolume(g->assets->bgDay, g->musicDayVol);
        if (g->assets->bgNight.ctxData) SetMusicVolume(g->assets->bgNight, g->musicNightVol);

        if (g->hitFlash > 0.0f) {
            g->hitFlash -= dt * 2.0f;
            if (g->hitFlash < 0.0f) g->hitFlash = 0.0f;
        }
        if (g->shakeTime > 0.0f) {
            g->shakeTime -= dt;
            if (g->shakeTime < 0.0f) g->shakeTime = 0.0f;
        }

        // --- Day/Night cycle ---
        if (!g->forceNight) {
            g->timeOfDay += dt * 0.002f;
            if (g->timeOfDay > 1.0f) g->timeOfDay -= 1.0f;

            if (g->cluesCollected >= 3 && g->timeOfDay < 0.65f) {
                g->forceNight = true;
                g->todTarget = 0.70f;
                g->todBlend = 0.0f;
            }
        }
        else {
            g->todBlend += dt * 0.9f;
            if (g->todBlend > 1.0f) g->todBlend = 1.0f;

            float t = dt * 3.0f;
            if (t > 1.0f) t = 1.0f;
            g->timeOfDay = Lerp(g->timeOfDay, g->todTarget, t);

            if (fabsf(g->timeOfDay - g->todTarget) < 0.01f) {
                g->timeOfDay = g->todTarget;
                g->forceNight = false;
            }
        }

        Player_Update(g->player, g, dt);
        Rival_Update(g->rival, g, dt);
        if (IsKeyPressed(KEY_ESCAPE)) g->state = STATE_PAUSED;
        CamFollow(g, dt);

        // Dynamic zoom based on player size
        float targetZoom = 1.0f / g->player->scale;
        targetZoom = Clamp(targetZoom, 0.35f, 2.0f);
        float zSmooth = 1.0f - expf(-8.0f * dt);
        g->cam.zoom += (targetZoom - g->cam.zoom) * zSmooth;

        // --- Camera shake ---
        if (g->shakeTime > 0.0f) {
            float k = 8.0f;
            float s = 1.0f - expf(-k * dt);
            Vector2 desired = g->player->pos;
            g->cam.target = Vector2Lerp(g->cam.target, desired, s);

            float cx = GetScreenWidth() / 2.0f;
            float cy = GetScreenHeight() / 2.0f;
            g->cam.offset = (Vector2){ cx, cy };

            g->cam.target.x = floorf(g->cam.target.x);
            g->cam.target.y = floorf(g->cam.target.y);

            float shake = g->shakeTime;
            float amp = 4.0f * shake;
            float timeNow = (float)GetTime();
            Vector2 jitter = { sinf(timeNow * 50.0f) * amp, cosf(timeNow * 45.0f) * amp };
            g->cam.offset.x += jitter.x;
            g->cam.offset.y += jitter.y;

            g->shakeTime = fmaxf(0.0f, g->shakeTime - 3.0f * dt);
        }

        // update popups
        for (int i = 0; i < g->popCount;) {
            g->pops[i].t += dt;
            if (g->pops[i].t > 0.9f) {
                g->pops[i] = g->pops[g->popCount - 1];
                g->popCount--;
                continue;
            }
            i++;
        }

        if (g->player->hp <= 0) g->state = STATE_GAMEOVER;
        if (g->cluesCollected >= 4) g->state = STATE_WIN;
    } break;

    case STATE_PAUSED:
        if (IsKeyPressed(KEY_ESCAPE)) g->state = STATE_PLAYING;
        break;

    case STATE_GAMEOVER:
    case STATE_WIN:
        if (IsKeyPressed(KEY_ENTER)) {
            Game_Shutdown(g);
            Game_Init(g, g->assets);
            g->state = STATE_INTRO;
        }
        break;
    }
}

static Color SkyColor(float t) {
    if (t < 0.45f) return (Color) { 120, 170, 210, 255 };
    if (t < 0.55f) return (Color) { 60, 80, 120, 255 };
    if (t < 0.85f) return (Color) { 18, 20, 28, 255 };
    return (Color) { 80, 110, 160, 255 };
}

void Game_Draw(Game* g) {
    if (g->state == STATE_INTRO) {
        ClearBackground(BLACK);
        const char* t = "SURVIVOR'S OATH: BLOOD & BONDS";
        DrawText(t, (GetScreenWidth() - MeasureText(t, 36)) / 2, 180, 36, RAYWHITE);
        const char* s = "WASD move • E interact • 1 eat • 2 drink • F craft spear • SPACE attack";
        DrawText(s, (GetScreenWidth() - MeasureText(s, 18)) / 2, 240, 18, LIGHTGRAY);
        DrawText("Press ENTER to begin", (GetScreenWidth() - MeasureText("Press ENTER to begin", 22)) / 2, 300, 22, YELLOW);
        return;
    }

    ClearBackground(SkyColor(g->timeOfDay));
    BeginMode2D(g->cam);

    World_DrawGround(g->assets);
    World_DrawNodes(g->nodes, g->nodeCount, g->assets);
    Rival_Draw(g->rival, g->assets);
    Player_Draw(g->player, g->assets);

    EndMode2D();

    if (g->state == STATE_PAUSED) UI_DrawPause();
    if (g->state == STATE_GAMEOVER) UI_DrawCenterMessage("YOU DIED", RED, "Press ENTER to restart");
    if (g->state == STATE_WIN) UI_DrawCenterMessage("TRACKS FOUND — REUNION CLOSE", YELLOW, "Press ENTER to start a new run");
}
