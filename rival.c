#include "raylib.h"     
#include "raymath.h"
#include "game.h"
#include "player.h"     
#include "rival.h"
#include "assets.h"
#include "world.h"

Rival* Rival_Create(Vector2 spawn) {
    Rival* r = MemAlloc(sizeof(Rival));
    *r = (Rival){ .pos = spawn, .alive = true, .t = 0.0f };
    r->scale = 1.8f;
    return r;
}
void Rival_Destroy(Rival* r) { MemFree(r); }

void Rival_Update(Rival* r, Game* g, float dt) {
    if (!r->alive || g->state != STATE_PLAYING) return;
    r->t += dt;

    float speed = 120.0f + 80.0f * Game_IsNight(g);
    Vector2 toP = Vector2Subtract(g->player->pos, r->pos);
    float d = Vector2Length(toP);
    if (d > 1) r->pos = Vector2Add(r->pos, Vector2Scale(Vector2Normalize(toP), speed * dt));

    // hurt player on contact
    if (d < 18.0f * r->scale) {
        static float hitTimer = 0; hitTimer += dt;
        if (hitTimer > 1.0f) {
            g->player->hp--; if (g->player->hp < 0) g->player->hp = 0;
            hitTimer = 0.0f;
     
            // NEW: screen effects
            g->hitFlash = 0.6f;      // red flash strength
            g->shakeTime = 0.25f;    // ~quarter second of shake
        }
    }

    // player attack
    if (g->player->hasSpear && g->player->attackCooldown <= 0 && IsKeyPressed(KEY_SPACE)) {
        g->player->attackCooldown = 0.5f;
        if (Vector2Distance(g->player->pos, r->pos) < 42.0f * r->scale)
            r->alive = false;
    }
}

void Rival_Draw(const Rival* r, const struct Assets* assets) {
    if (!r->alive) return;

    // shadow (scaled)
    DrawEllipse((int)r->pos.x, (int)(r->pos.y + 6 * r->scale),
        (int)(12 * r->scale), (int)(5 * r->scale),
        Fade(BLACK, 0.25f));

    // main sprite (scaled)
    Rectangle src = { 0, 0, (float)assets->texRival.width, (float)assets->texRival.height };
    Rectangle dst = { r->pos.x, r->pos.y, 24.0f * r->scale, 24.0f * r->scale };
    Vector2   origin = { 12.0f * r->scale, 12.0f * r->scale };

    DrawTexturePro(assets->texRival, src, dst, origin, 0.0f, WHITE);

    // label (optional)
    DrawText("Rival", (int)(r->pos.x - 18 * r->scale), (int)(r->pos.y - 28 * r->scale), (int)(12 * r->scale), RAYWHITE);
}


