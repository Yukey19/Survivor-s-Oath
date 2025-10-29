#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_NODES  256
#define MAX_POPS   64
#pragma once


typedef enum GameState {
    STATE_INTRO = 0,
    STATE_STORY,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAMEOVER,
    STATE_WIN
} GameState;

typedef enum NodeType {
    NODE_BERRY = 0,
    NODE_STICK,
    NODE_POND,
    NODE_CLUE
} NodeType;

typedef struct Node {
    Vector2 pos;
    NodeType type;
    bool taken;
} Node;

typedef struct PopFX {
    Vector2 pos;
    float   t;
    char    msg[16];   // <-- was text[16]; UI uses .msg
    Color   color;     // <-- was col;     code uses .color
} PopFX;

struct Player;
struct Rival;
struct Assets;

typedef struct Game {

    // --- intro/menu ---
    int  menuIndex;        // 0..2 (Start / How to Play / Quit)
    bool showHelp;         // toggles the help overlay on the intro screen

    // --- story screen ---
    bool  storyActive;     // optional flag; fine to keep
    int   storyIndex;      // which line we’re on
    float storyTimer;      // timer for story advance debounce

    // --- camera & time ---
    Camera2D cam;
    float    timeOfDay;    // 0..1
    float    introAlpha;   // 0..1 fade for intro
    float    introTimer;

    // --- light/shake/hit ---
    float lightRadius;
    float hitFlash;
    float shakeTime;

    // --- night force/fade ---
    bool  forceNight;
    float todTarget;
    float todBlend;

    // --- meta ---
    GameState state;
    bool quitRequested;

    // --- goal ---
    int  totalCluesRequired;
    int  cluesCollected;

    // --- objects ---
    Node  nodes[MAX_NODES];
    int   nodeCount;

    // --- fx ---
    PopFX pops[MAX_POPS];
    int   popCount;

    // --- characters/resources ---
    struct Player* player;
    struct Rival* rival;
    struct Assets* assets;

    // --- audio mix ---
    float musicDayVol;
    float musicNightVol;
} Game;

// ---- game API used by other modules
void Game_Init(Game* g, struct Assets* assets);
void Game_Update(Game* g, float dt);
void Game_Draw(Game* g);
void Game_Shutdown(Game* g);

// helpers used by player/ui/rival
void Game_AddPop(Game* g, Vector2 worldPos, Color color, const char* msg);
float Game_IsNight(const Game* g);   // returns 0 or 1 right now

#endif // GAME_H
