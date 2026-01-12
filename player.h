#pragma once
#include <SDL2/SDL.h>

extern float playerX;
extern float playerY;
extern float playerAngle;

void handlePlayerInput(SDL_Keycode key, bool pressed);
void updatePlayer(float deltaTime);
