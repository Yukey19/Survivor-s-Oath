// ui.h
#ifndef UI_H
#define UI_H
#include "raylib.h"
#include "game.h"
#pragma once

void UI_DrawIntro(const Game* g);
void UI_DrawOverlays(const Game* g);
void UI_DrawPause(void);
void UI_DrawWin(const Game* g);
void UI_DrawDeath(const Game* g);
void UI_DrawNightFade(const Game* g);   // if you added the cinematic fade
void UI_DrawCenterMessage(const char* title, Color titleColor, const char* subtitle);
void UI_DrawStory(const Game* g);

#pragma once
#endif