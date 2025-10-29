#ifndef WORLD_MODULE_H
#define WORLD_MODULE_H
#include "raylib.h"
#include "game.h"
#include "assets.h"
#pragma once

struct Assets;

#define WORLD_W 4000
#define WORLD_H 3000

void World_SpawnScatter(Node* out, int* count, int cap, int type, int num);
void World_DrawGround(struct Assets* assets);
void World_DrawNodes(Node* nodes, int nodeCount, struct Assets* assets);

#endif // WORLD_MODULE_H

