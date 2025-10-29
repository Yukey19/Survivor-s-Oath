#ifndef RIVAL_H
#define RIVAL_H
#include "raylib.h"
#include <stdbool.h>
#pragma once

struct Game;
struct Assets;

typedef struct Rival {
    Vector2 pos;
    bool alive;
    float t;
    float scale;         // NEW
} Rival;

Rival* Rival_Create(Vector2 spawn);
void   Rival_Destroy(Rival* r);
void   Rival_Update(Rival* r, struct Game* g, float dt);
void   Rival_Draw(const Rival* r, const struct Assets* assets);

#endif
