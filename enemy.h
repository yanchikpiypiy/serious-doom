#pragma once
#include "sprite.h"

struct Enemy {
  float x, y;           // Position in world
  bool alive;           // Alive/dead
  int angleIndex;       // Which direction sprite to show
  Sprite sprites[8][1]; // 8 directions, 1 frame for now
};

// Load/cleanup enemy sprites
bool loadEnemySprites();
void cleanupEnemySprites();

// Initialize / update / render
void initEnemies();
void updateEnemies(float deltaTime);
void renderEnemies(uint32_t *pixels, int screenWidth, int screenHeight);

// Access enemies
int getEnemyCount();
Enemy &getEnemy(int index);
