#include "world.h"
#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#define NODE_SCALE 1.8f   // Match your player/rival scale

static float frand(float a, float b) { return a + ((float)rand() / (float)RAND_MAX) * (b - a); }

void World_SpawnScatter(Node* out, int* count, int cap, int type, int num) {
    for (int i = 0; i < num && *count < cap; i++) {
        out[(*count)++] = (Node){ .pos = (Vector2){ frand(100.0f, WORLD_W - 100.0f), frand(100.0f, WORLD_H - 100.0f) },
                                  .type = type, .taken = false };
    }
}

#include "world.h"
#include "assets.h"

void World_DrawGround(struct Assets* assets) {
    (void)assets; // not used
    // Flat ground across the whole world — simple, safe, unbreakable
    DrawRectangle(0, 0, WORLD_W, WORLD_H, (Color) { 34, 46, 40, 255 });
}

// define the static pointer
Assets* g_worldAssets = NULL;

void World_DrawNodes(Node* nodes, int nodeCount, struct Assets* assets) {
    const float S = 1.8f;  // global visual scale for world items (match player/rival)

    for (int i = 0; i < nodeCount; i++) {
        Node* n = &nodes[i];

        switch (n->type) {
        case NODE_BERRY:
            if (!n->taken) {
                Rectangle src = (Rectangle){ 0.0f, 0.0f,
                    (float)assets->texBerry.width, (float)assets->texBerry.height };
                Rectangle dst = (Rectangle){ n->pos.x, n->pos.y, 16.0f * S, 16.0f * S };
                Vector2   origin = (Vector2){ 8.0f * S, 8.0f * S };
                DrawTexturePro(assets->texBerry, src, dst, origin, 0.0f, WHITE);
            }
            break;

        case NODE_STICK:
            if (!n->taken) {
                Rectangle src = (Rectangle){ 0.0f, 0.0f,
                    (float)assets->texStick.width, (float)assets->texStick.height };
                Rectangle dst = (Rectangle){ n->pos.x, n->pos.y, 12.0f * S, 18.0f * S };
                Vector2   origin = (Vector2){ 6.0f * S, 9.0f * S };
                DrawTexturePro(assets->texStick, src, dst, origin, 0.0f, WHITE);
            }
            break;

        case NODE_POND: {
            Rectangle src = (Rectangle){ 0.0f, 0.0f,
                (float)assets->texPond.width, (float)assets->texPond.height };
            Rectangle dst = (Rectangle){ n->pos.x, n->pos.y, 64.0f * S, 64.0f * S };
            Vector2   origin = (Vector2){ 32.0f * S, 32.0f * S };
            DrawTexturePro(assets->texPond, src, dst, origin, 0.0f, WHITE);
        } break;

        case NODE_CLUE:
            if (!n->taken) {
                float pulse = (sinf((float)GetTime() * 4.0f) * 0.5f + 0.5f);
                Color tint = (Color){ 255, 255, 255, (unsigned char)(190 + 55 * pulse) };

                Rectangle src = (Rectangle){ 0.0f, 0.0f,
                    (float)assets->texClue.width, (float)assets->texClue.height };
                Rectangle dst = (Rectangle){ n->pos.x, n->pos.y, 18.0f * S, 18.0f * S };
                Vector2   origin = (Vector2){ 9.0f * S, 9.0f * S };
                DrawTexturePro(assets->texClue, src, dst, origin, 0.0f, tint);
            }
            break;

        default:
            break;
        }
    }
}
