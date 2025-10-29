#include "player.h"
#include "raylib.h"
#include "raymath.h"
#include "game.h"
#include "assets.h"
#include "world.h"    

static void ClampToWorld(Vector2* p) {
    if (p->x < 0) p->x = 0; if (p->y < 0) p->y = 0;
    if (p->x > WORLD_W) p->x = WORLD_W; if (p->y > WORLD_H) p->y = WORLD_H;
}

Player* Player_Create(Vector2 spawn) {
    Player* p = MemAlloc(sizeof(Player));
    *p = (Player){ .pos = spawn, .speed = 200, .hp = 3, .hasSpear = false,
                   .invFood = 1, .invWater = 1, .invStick = 0,
                   .hunger = 80, .thirst = 80, .attackCooldown = 0 };
    p->dir4 = 0;  // start facing Down
    p->facing = 0.0f;
    p->vel = (Vector2){ 0,0 };
    p->accel = 1400.0f;
    p->friction = 9.0f;
    p->maxSpeed = 260.0f;
    p->scale = 1.8f;
    p->baseRadius = 8.0f;    // collision radius
    return p;
}
void Player_Destroy(Player* p) { MemFree(p); }

// Gather items / drink / inspect clue when near and pressing E
// Gather items / drink / inspect clue when near and pressing E
static void Gather(Game* g, Player* p) {
    for (int i = 0; i < g->nodeCount; ++i) {
        Node* n = &g->nodes[i];

        if ((n->type == NODE_BERRY || n->type == NODE_STICK || n->type == NODE_CLUE) && n->taken)
            continue;

        // interaction radius scaled up to match larger world objects
        float r = ((n->type == NODE_POND) ? 48.0f :
            (n->type == NODE_CLUE) ? 36.0f : 24.0f) * 1.8f;

        if (Vector2Distance(p->pos, n->pos) > r) continue;

        switch (n->type) {
        case NODE_BERRY:
            p->invFood++; n->taken = true;
            Game_AddPop(g, n->pos, (Color) { 230, 80, 90, 255 }, "+Food");
            PlaySound(g->assets->sPickupFood);
            break;

        case NODE_STICK:
            p->invStick++; n->taken = true;
            Game_AddPop(g, n->pos, (Color) { 160, 120, 80, 255 }, "+Stick");
            PlaySound(g->assets->sPickupStick);
            break;

        case NODE_POND:
            p->invWater++;
            Game_AddPop(g, n->pos, (Color) { 60, 150, 230, 255 }, "+Water");
            PlaySound(g->assets->sDrink);
            break;

        case NODE_CLUE:
            n->taken = true; g->cluesCollected++;
            Game_AddPop(g, n->pos, (Color) { 255, 220, 80, 255 }, "Clue!");
            PlaySound(g->assets->sClue);
            break;

        default: break;
        }
        break; // handle one thing per press
    }
}

// Craft spear when pressing F (cost: 2 sticks)
static void Craft(Player* p, Game* g) {
    if (!p->hasSpear && p->invStick >= 2) {
        p->invStick -= 2;
        p->hasSpear = true;
        Game_AddPop(g, p->pos, (Color) { 220, 220, 150, 255 }, "Spear!");
        PlaySound(g->assets->sCraft);
    }
}


static void Eat(Player* p) { if (p->invFood > 0 && p->hunger < 100) { p->invFood--; p->hunger += 35; if (p->hunger > 100)p->hunger = 100; } }
static void Drink(Player* p) { if (p->invWater > 0 && p->thirst < 100) { p->invWater--; p->thirst += 45; if (p->thirst > 100)p->thirst = 100; } }

void Player_Update(Player* p, Game* g, float dt) {
    // --- needs drain ---
    float heat = (g->timeOfDay < 0.45f) ? 1.1f : 0.9f;
    float radius = p->baseRadius * p->scale;
    p->hunger -= 2.0f * dt;
    p->thirst -= 3.0f * dt * heat;
    if (p->hunger <= 0 || p->thirst <= 0) {
        p->hp -= (p->hunger <= 0 && p->thirst <= 0) ? 2 : 1;
        if (p->hp < 0) p->hp = 0;
        if (p->hunger < 0) p->hunger = 0;
        if (p->thirst < 0) p->thirst = 0;
    }

    // --- input: build raw WASD vector ---
    Vector2 a = (Vector2){ 0.0f, 0.0f };  // <— THIS must exist
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    a.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  a.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  a.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) a.x += 1.0f;

    // --- 4-way facing from raw WASD (before normalize) ---
    if (a.x != 0.0f || a.y != 0.0f) {
        if (fabsf(a.x) >= fabsf(a.y)) {
            p->dir4 = (a.x > 0.0f) ? 2 : 1;   // Right : Left
        }
        else {
            p->dir4 = (a.y > 0.0f) ? 0 : 3;   // Down  : Up
        }
    }

    if (IsKeyPressed(KEY_F)) Craft(p, g);

    // --- acceleration from input (normalized) ---
    if (a.x != 0.0f || a.y != 0.0f) {
        a = Vector2Normalize(a);
        a = Vector2Scale(a, p->accel);
    }

    // integrate velocity
    p->vel.x += a.x * dt;
    p->vel.y += a.y * dt;

    // damping
    float damp = expf(-p->friction * dt);
    p->vel.x *= damp;
    p->vel.y *= damp;

    // sprint + top speed
    float run = IsKeyDown(KEY_LEFT_SHIFT) ? 1.5f : 1.0f;
    p->maxSpeed = 300.0f * run;

    float sp = Vector2Length(p->vel);
    if (sp > p->maxSpeed) {
        p->vel = Vector2Scale(Vector2Normalize(p->vel), p->maxSpeed);
    }

    // apply to position
    p->pos.x += p->vel.x * dt;
    p->pos.y += p->vel.y * dt;

    // keep mouse aim if you still use p->facing elsewhere
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), g->cam);
    Vector2 aim = Vector2Subtract(mouseWorld, p->pos);
    if (Vector2LengthSqr(aim) > 0.001f) p->facing = atan2f(aim.y, aim.x);

    ClampToWorld(&p->pos);

    // interactions
    if (IsKeyPressed(KEY_E))   Gather(g, p);
    if (IsKeyPressed(KEY_ONE)) Eat(p);
    if (IsKeyPressed(KEY_TWO)) Drink(p);

    if (p->attackCooldown > 0.0f) p->attackCooldown -= dt;
}


void Player_Draw(const Player* p, const struct Assets* assets) {
    // shadow
    DrawEllipse((int)p->pos.x, (int)(p->pos.y + 6 * p->scale),
        (int)(10 * p->scale), (int)(4 * p->scale),
        Fade(BLACK, 0.25f));
    

    Texture2D tex = assets->texPlayerDown;
    if (p->dir4 == 1) tex = assets->texPlayerLeft;
    else if (p->dir4 == 2) tex = assets->texPlayerRight;
    else if (p->dir4 == 3) tex = assets->texPlayerUp;



    Rectangle src = (Rectangle){ 0.0f, 0.0f, (float)tex.width, (float)tex.height };
    Rectangle dst = (Rectangle){ p->pos.x, p->pos.y, 24.0f * p->scale, 24.0f * p->scale };
    Vector2   origin = (Vector2){ 12.0f * p->scale, 12.0f * p->scale };

    DrawTexturePro(tex, src, dst, origin, 0.0f, WHITE);

    // optional spear overlay
    if (p->hasSpear) {
        float dx = (p->dir4 == 2) ? 18.0f : (p->dir4 == 1) ? -18.0f : 0.0f;
        float dy = (p->dir4 == 3) ? -18.0f : (p->dir4 == 0) ? 18.0f : 0.0f;
        Vector2 base = (Vector2){ p->pos.x, p->pos.y };
        Vector2 tip = (Vector2){ p->pos.x + dx * p->scale, p->pos.y + dy * p->scale };
        DrawLineEx(base, tip, 3.0f * p->scale, (Color) { 210, 210, 160, 255 });

    }
}

