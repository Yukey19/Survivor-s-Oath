#include "raylib.h"
#include "raymath.h"
#include "game.h"
#include "ui.h"
#include "assets.h"

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1100, 650, "Survivor's Oath: Blood & Bonds");
    InitAudioDevice();
    SetTargetFPS(60);

    Assets assets = { 0 };
    Assets_Load(&assets);          // tries to load PNGs; makes placeholders if missing

    Game G = { 0 };
    Game_Init(&G, &assets);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Game_Update(&G, dt);

        BeginDrawing();
        Game_Draw(&G);
        UI_DrawOverlays(&G);       // HUD, bars, prompts
        EndDrawing();

        if (G.quitRequested) break;
    }

    Game_Shutdown(&G);
    Assets_Unload(&assets);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
