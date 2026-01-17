#pragma once
#include "sprite.h"
#include <stdint.h>

#define PROJECTILE_MAX_FRAMES 11 // A-K for animation
#define PROJECTILE_MAX_ACTIVE 50 // Max projectiles on screen

enum ProjectileType { PROJ_ENEMY_FIREBALL, PROJ_PLAYER_BULLET };

struct Projectile {
  float x, y;
  float vx, vy;
  float angle;
  bool active;
  ProjectileType type;

  // Animation
  int frameIndex;
  float animTimer;
  float animSpeed;
  float lifetime;
  float maxLifetime;

  // Rendering
  float distanceTraveled;
};

// API
bool loadProjectileSprites();
void cleanupProjectileSprites();
void initProjectiles();
void updateProjectiles(float deltaTime);
void renderProjectiles(uint32_t *pixels, int screenWidth, int screenHeight,
                       float *zBuffer);

// Spawning
void spawnEnemyProjectile(float x, float y, float targetX, float targetY);
void spawnPlayerProjectile(float x, float y, float angle);

// Collision detection helper
bool checkProjectilePlayerHit(float playerX, float playerY, float radius);
