#ifndef PLAYER_H
#define PLAYER_H
#include "raylib.h"
#include <stdbool.h>
#pragma once

struct Game;
struct Assets;

typedef struct Player {
    int dir4;   // 0=Down, 1=Left, 2=Right, 3=Up
    Vector2 vel;
    float accel;        // e.g., 900.0f
    float friction;     // e.g., 8.0f
    float maxSpeed;     // e.g., 180.0f
    Vector2 pos;
    float speed;
    int hp;
    bool hasSpear;
    int invFood, invWater, invStick;
    float hunger;              // 0..100
    float thirst;              // 0..100
    float attackCooldown;
    float   facing;
    float scale;
    float baseRadius;
} Player;

Player* Player_Create(Vector2 spawn);
void    Player_Destroy(Player* p);
void    Player_Update(Player* p, struct Game* g, float dt);
void    Player_Draw(const Player* p, const struct Assets* assets);

#endif
