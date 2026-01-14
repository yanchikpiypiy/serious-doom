#pragma once
#include "sprite.h"

#define ENEMY_MAX_FRAMES 4
#define ENEMY_DOOM_ANGLES 5 // Doom stores only 5 angles

struct Enemy {
  float x, y;
  float vx, vy;
  bool alive;
  float prevX, prevY;
  int frameIndex;
  int frameCount;

  float animTimer;
  float animSpeed;

  Sprite sprites[ENEMY_MAX_FRAMES][ENEMY_DOOM_ANGLES];
};

// API
bool loadEnemySprites();
void cleanupEnemySprites();

void initEnemies();
void updateEnemies(float deltaTime);
void renderEnemies(uint32_t *pixels, int screenWidth, int screenHeight);

int getEnemyCount();
Enemy &getEnemy(int index);
