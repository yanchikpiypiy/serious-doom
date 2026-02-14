#pragma once
#include "sprite.h"

#define ENEMY_WALK_FRAMES 4   // A, B, C, D
#define ENEMY_SHOOT_FRAMES 9  // E-M
#define ENEMY_PAIN_FRAMES 1   // N
#define ENEMY_DEATH_FRAMES 9  // O-W
#define ENEMY_TOTAL_FRAMES 29 // 4+9+1+9
#define ENEMY_DOOM_ANGLES 5

// Animation states
enum EnemyAnimState {
  ANIM_IDLE,
  ANIM_WALK,
  ANIM_SHOOT,
  ANIM_PAIN,
  ANIM_DEATH,
  ANIM_XDEATH
};

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
  int health;

  EnemyAnimState animState;
  float shootTimer;
  float shootCooldown;
  Sprite sprites[ENEMY_TOTAL_FRAMES][ENEMY_DOOM_ANGLES];
};

// Gameplay constants
#define ENEMY_MAX_HEALTH 100
#define ENEMY_PAIN_CHANCE 160
#define ENEMY_SHOOT_RANGE 9.0f
#define ENEMY_SHOOT_COOLDOWN 2.0f
#define ENEMY_XDEATH_TRASHHOLD 40

// API
bool loadEnemySprites();
void cleanupEnemySprites();
void initEnemies();
void updateEnemies(float deltaTime);
void renderEnemies(uint32_t *pixels, int screenWidth, int screenHeight,
                   float *buffer);
int getEnemyCount();
Enemy &getEnemy(int index);
void damageEnemy(int enemyIndex, int damage);
int hitscanCheckEnemy();
