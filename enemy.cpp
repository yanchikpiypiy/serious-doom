#include "enemy.h"
#include "player.h"
#include "renderer.h"
#include "sprite.h"
#include <cmath>
#include <cstdio>

// =========================
// Simple enemy system

static Enemy enemies[1];
static int enemyCount = 1;

bool loadEnemySprites() {
  // Map to your actual sprite files
  // Angle mapping: 0=front, 1-3=right side, 4=back, 5-7=left side
  const char *angleFiles[8] = {
      "sprites/slhv/SLHVA1.png",   // angle 0 - front
      "sprites/slhv/SLHVA2A8.png", // angle 1 - front-right
      "sprites/slhv/SLHVA3A7.png", // angle 2 - right
      "sprites/slhv/SLHVA4A6.png", // angle 3 - back-right
      "sprites/slhv/SLHVA5.png",   // angle 4 - back
      "sprites/slhv/SLHVA4A6.png", // angle 5 - back-left (mirror of 3)
      "sprites/slhv/SLHVA3A7.png", // angle 6 - left (mirror of 2)
      "sprites/slhv/SLHVA2A8.png"  // angle 7 - front-left (mirror of 1)
  };

  for (int a = 0; a < 8; a++) {
    for (int f = 0; f < 1; f++) { // 1 frame for now
      if (!loadSprite(&enemies[0].sprites[a][f], angleFiles[a])) {
        printf("Failed to load enemy sprite: %s\n", angleFiles[a]);
        return false;
      }
    }
  }
  printf("Enemy sprites loaded\n");
  return true;
}

void cleanupEnemySprites() {
  for (int a = 0; a < 8; a++) {
    for (int f = 0; f < 1; f++) {
      if (enemies[0].sprites[a][f].pixels) {
        delete[] enemies[0].sprites[a][f].pixels;
        enemies[0].sprites[a][f].pixels = nullptr;
      }
    }
  }
}

void initEnemies() {
  enemies[0].x = 5.5f;
  enemies[0].y = 6.5f;
  enemies[0].alive = true;
}

void updateEnemies(float /*deltaTime*/) {
  // No AI yet
}

void renderEnemies(uint32_t *pixels, int screenWidth, int screenHeight) {
  const float FOV = 3.14159f / 3.0f;
  const int NUM_ANGLES = 8;

  for (int i = 0; i < enemyCount; i++) {
    Enemy &e = enemies[i];
    if (!e.alive)
      continue;

    // Vector from player to enemy
    float dx = e.x - playerX;
    float dy = e.y - playerY;
    float distance = sqrtf(dx * dx + dy * dy);
    if (distance < 0.01f)
      continue;

    // Angle from player to enemy
    float angleToEnemy = atan2f(dy, dx);

    // Relative to player's viewing direction
    float relativeAngle = angleToEnemy - playerAngle;

    // Normalize to 0 to 2π
    while (relativeAngle < 0)
      relativeAngle += 2 * M_PI;
    while (relativeAngle >= 2 * M_PI)
      relativeAngle -= 2 * M_PI;

    // Check if in FOV
    float viewAngle = relativeAngle;
    if (viewAngle > M_PI)
      viewAngle -= 2 * M_PI;

    if (fabs(viewAngle) > FOV / 2)
      continue; // outside FOV

    // Determine which sprite angle to use (0-7)
    // relativeAngle represents where enemy is relative to player's view
    // This directly maps to which side of the enemy we see
    int angleIndex =
        (int)((relativeAngle / (2 * M_PI)) * NUM_ANGLES + 0.5f) % NUM_ANGLES;

    Sprite &sprite = e.sprites[angleIndex][0]; // single frame for now

    // Project to screen
    float correctedDist = distance * cosf(viewAngle);
    if (correctedDist < 0.5f)
      correctedDist = 0.5f;

    int spriteH = int((screenHeight / correctedDist) * 0.6f);
    int spriteW = int((sprite.width / float(sprite.height)) * spriteH);

    // Screen X position
    int drawX = int((0.5f + viewAngle / FOV) * screenWidth - spriteW / 2);

    // Screen Y position - align sprite bottom to floor line (middle of screen)
    int drawY = (screenHeight / 2) - spriteH / 2;

    drawSpriteScaled(&sprite, drawX, drawY, float(spriteW) / sprite.width,
                     pixels, screenWidth, screenHeight);
  }
}

int getEnemyCount() { return enemyCount; }

Enemy &getEnemy(int index) { return enemies[index]; }
