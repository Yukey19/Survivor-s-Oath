#include "ui.h"
#include "raylib.h"
#include "raymath.h"   // DEG2RAD
#include "game.h"
#include "assets.h"
#include "player.h"
#include <stdio.h>     // (optional) if you ever use snprintf
#include "world.h"


// -----------------------------------------------------------------------------
// Small helper: simple UI bar
static void DrawBar(int x, int y, int w, int h, float pct, Color fill) {
    if (pct < 0.0f) pct = 0.0f; if (pct > 1.0f) pct = 1.0f;
    DrawRectangle(x, y, w, h, (Color) { 30, 30, 34, 220 });
    DrawRectangle(x + 1, y + 1, (int)((w - 2) * pct), h - 2, fill);
    DrawRectangleLines(x, y, w, h, (Color) { 0, 0, 0, 200 });
}

// -----------------------------------------------------------------------------
// Context prompt ("E to ...") shown when near a node
static void UI_DrawContextPrompt(const Game* g) {
    const float R_BERRY = 24.0f, R_STICK = 24.0f, R_CLUE = 36.0f, R_POND = 48.0f;

    // Find first nearby node (simple)
    const Node* best = NULL;
    for (int i = 0; i < g->nodeCount; ++i) {
        const Node* n = &g->nodes[i];
        if ((n->type == NODE_BERRY || n->type == NODE_STICK || n->type == NODE_CLUE) && n->taken) continue;

        float r = 0.0f;
        switch (n->type) {
        case NODE_BERRY: r = R_BERRY; break;
        case NODE_STICK: r = R_STICK; break;
        case NODE_CLUE:  r = R_CLUE;  break;
        case NODE_POND:  r = R_POND;  break;
        }
        if (Vector2Distance(g->player->pos, n->pos) <= r) { best = n; break; }
    }
    if (!best) return;

    const char* what = "";
    switch (best->type) {
    case NODE_BERRY: what = "E: Gather berries"; break;
    case NODE_STICK: what = "E: Pick up stick";  break;
    case NODE_POND:  what = "E: Drink water";    break;
    case NODE_CLUE:  what = "E: Inspect clue";   break;
    default: return;
    }

    int fontSize = 18;
    int w = MeasureText(what, fontSize);
    int x = (GetScreenWidth() - w) / 2;
    int y = GetScreenHeight() - 40;

    DrawRectangle(x - 8, y - 6, w + 16, 28, (Color) { 0, 0, 0, 150 });
    DrawRectangleLines(x - 8, y - 6, w + 16, 28, (Color) { 0, 0, 0, 220 });
    DrawText(what, x, y, fontSize, RAYWHITE);
}

// Draw floating pickup texts
static void UI_DrawPopFX(const Game* g) {
    for (int i = 0; i < g->popCount; ++i) {
        const PopFX* p = &g->pops[i];
        // convert world -> screen
        Vector2 sp = GetWorldToScreen2D(p->pos, g->cam);

        // motion + fade
        float a = 1.0f - (p->t / 0.9f);           // 1 → 0
        if (a < 0) a = 0;
        int   yoff = (int)(p->t * 40.0f);         // float upward
        int   fs = 16;
        int   w = MeasureText(p->msg, fs);

        Color shadow = (Color){ 0,0,0,(unsigned char)(180 * a) };
        Color tint = p->color; tint.a = (unsigned char)(255 * a);

        int sx = (int)(sp.x - w / 2);
        int sy = (int)(sp.y - 18 - yoff);

        DrawText(p->msg, sx + 1, sy + 1, fs, shadow);
        DrawText(p->msg, sx, sy, fs, tint);
    }
}

// --- Smooth nightfall overlay ----------------------------------------------
void UI_DrawNightFade(const Game* g)
{
    if (g->forceNight) {
        // use todBlend (0..1) to fade in a vignette
        float alpha = g->todBlend;               // 0 at start, 1 at full night
        if (alpha > 1.0f) alpha = 1.0f;

        int w = GetScreenWidth();
        int h = GetScreenHeight();

        // Draw a translucent black layer
        DrawRectangle(0, 0, w, h, Fade(BLACK, 0.5f * alpha));

        // Optional: vignette effect (circle gradient in center)
        Vector2 c = { w / 2.0f, h / 2.0f };
        float maxR = (float)(w > h ? w : h);

        DrawCircleGradient((int)c.x, (int)c.y, maxR,
            Fade(BLACK, 0.0f),
            Fade(BLACK, 0.8f * alpha));
    }
}

// -----------------------------------------------------------------------------
// Main overlays: night + flashlight cone, hit flash, HUD, reticle, prompt
void UI_DrawOverlays(const Game* g) {
    // --- Night overlay + Flashlight CONE ---
    float night = Game_IsNight(g);
    if (night > 0.15f) {
        float dark = 0.25f + 0.35f * night;
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, dark));

        if (g->lightRadius > 0.0f) {
            Vector2 sp = GetWorldToScreen2D(g->player->pos, g->cam);
            float  ang = g->player->facing;     // radians
            float  fov = 42.0f * DEG2RAD;       // cone half-angle
            float  R = g->lightRadius * g->player->scale;

            Vector2 a = (Vector2){ sp.x + cosf(ang - fov) * R, sp.y + sinf(ang - fov) * R };
            Vector2 b = (Vector2){ sp.x + cosf(ang + fov) * R, sp.y + sinf(ang + fov) * R };

            BeginBlendMode(BLEND_ADDITIVE);
            DrawTriangle(sp, a, b, (Color) { 255, 255, 255, 160 }); // cone
            DrawCircleGradient((int)sp.x, (int)sp.y, (float)(R * 0.35f),
                (Color) {
                255, 255, 255, 220
            }, (Color) { 0, 0, 0, 0 }); // soft origin
            EndBlendMode();
        }
    }

    // --- Hit flash overlay (kept under HUD so text stays readable) ---
    if (g->hitFlash > 0.0f) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RED, g->hitFlash * 0.6f));
    }

    // --- HUD bars & icons ---
    const int pad = 10;
    int x = pad, y = pad;

    if (g->assets->uiHeart.id) DrawTexture(g->assets->uiHeart, x, y, WHITE);
    DrawBar(x + 28, y + 4, 160, 14, g->player->hp / 3.0f, RED); y += 24;

    if (g->assets->uiFood.id) DrawTexture(g->assets->uiFood, x, y, WHITE);
    DrawBar(x + 28, y + 4, 160, 14, g->player->hunger / 100.0f, (Color) { 200, 120, 50, 255 }); y += 24;

    if (g->assets->uiWater.id) DrawTexture(g->assets->uiWater, x, y, WHITE);
    DrawBar(x + 28, y + 4, 160, 14, g->player->thirst / 100.0f, (Color) { 50, 140, 220, 255 }); y += 24;

    DrawText(TextFormat("Food[1]: %d  Water[2]: %d  Sticks: %d  Spear: %s (F to craft)",
        g->player->invFood, g->player->invWater, g->player->invStick,
        g->player->hasSpear ? "Yes" : "No"),
        pad, y + 2, 18, RAYWHITE);

    DrawText(TextFormat("Clues: %d / %d", g->cluesCollected, g->totalCluesRequired),
        GetScreenWidth() - 130, 10, 22,
        (g->cluesCollected >= g->totalCluesRequired - 1 ? GOLD : YELLOW));

    // small reticle
    Vector2 m = GetMousePosition();
    DrawLine((int)m.x - 6, (int)m.y, (int)m.x + 6, (int)m.y, Fade(RAYWHITE, 0.7f));
    DrawLine((int)m.x, (int)m.y - 6, (int)m.x, (int)m.y + 6, Fade(RAYWHITE, 0.7f));

    // context prompt (nearby interactable)
    UI_DrawContextPrompt(g);
    // floating pickup texts
    UI_DrawPopFX(g);

}

// -----------------------------------------------------------------------------
// Pause & center messages
void UI_DrawPause(void) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));
    const char* p = "PAUSED (ESC to resume)";
    DrawText(p, (GetScreenWidth() - MeasureText(p, 28)) / 2, GetScreenHeight() / 2 - 14, 28, RAYWHITE);
}

// ui.c 

void UI_DrawCenterMessage(const char* title, Color titleColor, const char* subtitle)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    int titleSize = 40;
    int subSize = 20;

    DrawText(title,
        sw / 2 - MeasureText(title, titleSize) / 2,
        sh / 2 - 60,
        titleSize, titleColor);

    if (subtitle && subtitle[0])
    {
        DrawText(subtitle,
            sw / 2 - MeasureText(subtitle, subSize) / 2,
            sh / 2 + 10,
            subSize, RAYWHITE);
    }
}

// ---- NEW: Win / Death overlays ----
void UI_DrawWin(const Game* g)
{
    // dim the screen slightly
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.35f));
    UI_DrawCenterMessage("YOU SURVIVED", YELLOW, "Press ENTER to return to title");
}

void UI_DrawDeath(const Game* g)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.45f));
    UI_DrawCenterMessage("YOU DIED", RED, "Press ENTER to return to title");
}

void UI_DrawIntro(const Game* g) {
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    // background + fade
    DrawRectangle(0, 0, sw, sh, (Color) { 12, 14, 18, 255 });
    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 1.0f - g->introAlpha));

    // title
    const char* title = "Survivor's Oath: Blood & Bonds";
    int ts = 42;
    DrawText(title, sw / 2 - MeasureText(title, ts) / 2, sh / 5, ts, RAYWHITE);

    // menu items
    const char* items[3] = { "New Game", "How To Play (H)", "Quit" };
    int fs = 26;
    int startY = sh / 2 - 10;

    for (int i = 0; i < 3; ++i) {
        int w = MeasureText(items[i], fs);
        int x = sw / 2 - w / 2;
        int y = startY + i * 40;

        // highlight the selected row
        if (i == g->menuIndex) {
            int pad = 10;
            DrawRectangle(x - pad, y - 6, w + pad * 2, fs + 10, (Color) { 40, 60, 90, 140 });
            DrawRectangleLines(x - pad, y - 6, w + pad * 2, fs + 10, (Color) { 80, 120, 180, 220 });
        }
        DrawText(items[i], x, y, fs, RAYWHITE);
    }

    // blinking "Press ENTER" hint
    if (((int)(g->introTimer * 2)) % 2 == 0) {
        const char* hint = "Press ENTER";
        int hw = MeasureText(hint, 20);
        DrawText(hint, sw / 2 - hw / 2, startY + 3 * 40 + 18, 20, LIGHTGRAY);
    }

    // help card overlay
    if (g->showHelp) {
        int w = 520, h = 260;
        int x = sw / 2 - w / 2, y = sh / 2 - h / 2;
        DrawRectangle(x, y, w, h, (Color) { 20, 22, 28, 235 });
        DrawRectangleLines(x, y, w, h, (Color) { 100, 120, 160, 255 });

        int lh = 20, yy = y + 18, pad = 18;
        DrawText("How To Play", x + pad, yy, 26, YELLOW); yy += 34;
        DrawText("- WASD: Move", x + pad, yy, lh, RAYWHITE); yy += lh + 6;
        DrawText("- Mouse: Aim", x + pad, yy, lh, RAYWHITE); yy += lh + 6;
        DrawText("- E: Interact (gather, drink, clue)", x + pad, yy, lh, RAYWHITE); yy += lh + 6;
        DrawText("- 1/2: Eat / Drink from inventory", x + pad, yy, lh, RAYWHITE); yy += lh + 6;
        DrawText("- F: Craft spear (2 sticks)", x + pad, yy, lh, RAYWHITE); yy += lh + 6;
        DrawText("- SPACE: Attack (with spear)", x + pad, yy, lh, RAYWHITE); yy += lh + 6;
        DrawText("- Find 4 clues. After 3, night falls…", x + pad, yy, lh, RAYWHITE); yy += lh + 10;
        DrawText("Press H to close", x + w - pad - MeasureText("Press H to close", lh), y + h - lh - 12, lh, LIGHTGRAY);
    }
}

void UI_DrawStory(const Game* g)
{
    const char* LINES[] = {
        "A storm took the ship. I woke up alone.",
        "No sign of my son… only broken crates and footprints in the sand.",
        "This island breathes—water, berries, dangers—and someone else is here.",
        "I made an oath: survive, track the clues, and bring him home."
    };
    const int STORY_LINE_COUNT = (int)(sizeof(LINES) / sizeof(LINES[0]));

    // background
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color) { 14, 16, 22, 255 });

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    int titleSize = 30;
    int bodySize = 22;

    // title
    const char* title = "Survivor's Oath: Blood & Bonds";
    DrawText(title, sw / 2 - MeasureText(title, titleSize) / 2, sh / 5, titleSize, RAYWHITE);

    // current line
    int idx = g->storyIndex;
    if (idx < 0) idx = 0;
    const char* line = LINES[(idx < STORY_LINE_COUNT) ? idx : (STORY_LINE_COUNT - 1)];

    DrawText(line,
        sw / 2 - MeasureText(line, bodySize) / 2,
        sh / 2 - 20,
        bodySize, LIGHTGRAY);

    // prompt (blink)
    if (((int)(GetTime() * 2)) % 2 == 0) {
        const char* hint = (g->storyIndex < STORY_LINE_COUNT - 1) ?
            "Press ENTER to continue" :
            "Press ENTER to begin";
        DrawText(hint,
            sw / 2 - MeasureText(hint, 18) / 2,
            sh - 80,
            18, (Color) { 200, 200, 200, 220 });
    }
}



