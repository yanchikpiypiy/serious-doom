#pragma once
#include "sprite.h"

#define ENEMY_WALK_FRAMES 4
#define ENEMY_SHOOT_FRAMES 9
#define ENEMY_TOTAL_FRAMES 13 // 4 walk + 9 shoot
#define ENEMY_DOOM_ANGLES 5   // Doom stores only 5 angles

// Animation states
enum EnemyAnimState { ANIM_IDLE, ANIM_WALK, ANIM_SHOOT };

struct Enemy {
  float x, y;
  float vx, vy;
  bool alive;
  float prevX, prevY;
  int frameIndex;
  int frameCount;
  float facingAngle;
  float animTimer;
  float animSpeed;

  // Animation state
  EnemyAnimState animState;
  float shootTimer;
  float shootCooldown;

  Sprite sprites[ENEMY_TOTAL_FRAMES][ENEMY_DOOM_ANGLES];
};

// Gameplay constants
#define ENEMY_SHOOT_RANGE 9.0f
#define ENEMY_SHOOT_COOLDOWN 2.0f
#define ENEMY_SHOOT_DURATION 1.35f // 9 frames * 0.15s

// API
bool loadEnemySprites();
void cleanupEnemySprites();
void initEnemies();
void updateEnemies(float deltaTime);
void renderEnemies(uint32_t *pixels, int screenWidth, int screenHeight,
                   float *buffer);
int getEnemyCount();
Enemy &getEnemy(int index);
