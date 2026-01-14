#include "enemy.h"
#include "player.h"
#include "sprite.h"
#include <cctype>
#include <cmath>
#include <cstdio>

// =========================
// Simple enemy system

// Global enemy array and count
#define MAX_ENEMIES 10
static Enemy enemies[MAX_ENEMIES];
static int enemyCount = 1;

// Store all 8 viewing angles here (frames x angles)
static Sprite allAngleSprites[ENEMY_MAX_FRAMES][8];

// Map view angle (0-7) to file format
// Angles 0 and 4 (A1, A5): no mirror, just "A1" or "A5"
// Angles 1,2,3 (A2A8, A3A7, A4A6): has mirror pair in filename
// Angles 5,6,7: use the mirrored versions from same files
struct AngleFileInfo {
  int angle1;
  int angle2; // -1 if no mirror pair
};

static const AngleFileInfo viewToFile[8] = {
    {1, -1}, // View 0 -> A1 (no mirror)
    {2, 8},  // View 1 -> A2A8
    {3, 7},  // View 2 -> A3A7
    {4, 6},  // View 3 -> A4A6
    {5, -1}, // View 4 -> A5 (no mirror)
    {4, 6},  // View 5 -> A4A6 (mirrored)
    {3, 7},  // View 6 -> A3A7 (mirrored)
    {2, 8}   // View 7 -> A2A8 (mirrored)
};

// Animation frame letters
static const char frameLetters[ENEMY_MAX_FRAMES] = {'A', 'B', 'C', 'D'};

static void buildEnemySpritePath(char *out, const char *base, char frame,
                                 const AngleFileInfo &info) {
  if (info.angle2 == -1) {
    // No mirror: sprites/slhv/SLHVA1.png
    sprintf(out, "sprites/slhv/%s%c%d.png", base, frame, info.angle1);
  } else {
    // Has mirror: sprites/slhv/SLHVA2A8.png or SLHVB2B8.png (frame letter
    // repeated!)
    sprintf(out, "sprites/slhv/%s%c%d%c%d.png", base, frame, info.angle1, frame,
            info.angle2);
  }
}

bool loadEnemySprites() {
  Enemy &e = enemies[0];
  e.frameCount = ENEMY_MAX_FRAMES;

  // Load all 8 viewing angles from their corresponding files
  for (int f = 0; f < e.frameCount; f++) {
    for (int viewAngle = 0; viewAngle < 8; viewAngle++) {
      char path[256];
      buildEnemySpritePath(path, "SLHV", frameLetters[f],
                           viewToFile[viewAngle]);

      if (!loadSprite(&allAngleSprites[f][viewAngle], path)) {
        printf("Failed to load enemy sprite: %s\n", path);
        return false;
      }
    }
  }

  // Also load into the Enemy struct's array (first 5 angles for compatibility)
  for (int f = 0; f < e.frameCount; f++) {
    for (int a = 0; a < ENEMY_DOOM_ANGLES; a++) {
      e.sprites[f][a] = allAngleSprites[f][a];
    }
  }

  printf("Enemy sprites loaded (%d frames, 8 view angles)\n", e.frameCount);
  return true;
}

void cleanupEnemySprites() {
  // Clean up all angle sprites
  for (int f = 0; f < ENEMY_MAX_FRAMES; f++) {
    for (int a = 0; a < 8; a++) {
      if (allAngleSprites[f][a].pixels) {
        delete[] allAngleSprites[f][a].pixels;
        allAngleSprites[f][a].pixels = nullptr;
      }
    }
  }

  // Clear Enemy struct references (they point to same data)
  Enemy &e = enemies[0];
  for (int f = 0; f < e.frameCount; f++) {
    for (int a = 0; a < ENEMY_DOOM_ANGLES; a++) {
      e.sprites[f][a].pixels = nullptr;
    }
  }
}

void initEnemies() {
  Enemy &e = enemies[0];

  e.x = 5.5f;
  e.y = 6.5f;
  e.vx = 1.0f;
  e.vy = 0.0f;

  e.prevX = e.x;
  e.prevY = e.y;

  e.alive = true;
  e.frameIndex = 0;

  e.animTimer = 0.0f;
  e.animSpeed = 0.15f;
}

void updateEnemies(float deltaTime) {
  for (int i = 0; i < enemyCount; i++) {
    Enemy &e = enemies[i];
    if (!e.alive)
      continue;

    // Store previous position
    float oldX = e.x;
    float oldY = e.y;

    // Simple movement (walk left/right)
    e.x += e.vx * deltaTime;

    if (e.x < 4.0f || e.x > 8.0f)
      e.vx = -e.vx;

    // Update prevX/prevY only if we actually moved
    float dx = e.x - oldX;
    float dy = e.y - oldY;
    if (fabs(dx) > 0.001f || fabs(dy) > 0.001f) {
      e.prevX = oldX;
      e.prevY = oldY;
    }
    // Otherwise keep the last valid prevX/prevY for direction

    // Animate only if moving
    if (fabs(e.vx) > 0.01f || fabs(e.vy) > 0.01f) {
      e.animTimer += deltaTime;
      if (e.animTimer >= e.animSpeed) {
        e.animTimer = 0.0f;
        e.frameIndex = (e.frameIndex + 1) % e.frameCount;
      }
    } else {
      e.frameIndex = 0;
    }
  }
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

    // Calculate enemy's facing direction from actual movement (dx, dy)
    float dx_enemy = e.x - e.prevX;
    float dy_enemy = e.y - e.prevY;
    float enemyFacingAngle = atan2f(dy_enemy, dx_enemy);

    // Calculate what sprite to show: where is player relative to enemy's
    // facing? Add PI to flip it so A1 (front) shows when player is in front of
    // enemy
    float spriteAngle = angleToEnemy - enemyFacingAngle + M_PI;

    // Normalize to 0 to 2π
    while (spriteAngle < 0)
      spriteAngle += 2 * M_PI;
    while (spriteAngle >= 2 * M_PI)
      spriteAngle -= 2 * M_PI;

    // Determine which sprite angle to use (0-7)
    int angleIndex =
        (int)((spriteAngle / (2 * M_PI)) * NUM_ANGLES + 0.5f) % NUM_ANGLES;
    // Get sprite from the global array
    Sprite &sprite = allAngleSprites[e.frameIndex][angleIndex];

    // Project to screen with fish-eye correction
    float correctedDist = distance * cosf(viewAngle);
    if (correctedDist < 0.1f)
      correctedDist = 0.1f;

    // Calculate wall height at this distance (same formula as renderer)
    int wallHeight = int((float(screenHeight) / correctedDist) * 0.8f);

    // Floor line is at: screenHeight/2 + wallHeight
    int floorLine = (screenHeight / 2) + wallHeight;

    // Enemy sprite height - scale based on distance
    int spriteH = int((float(screenHeight) / correctedDist) * 1.1f);
    int spriteW = int((float(sprite.width) / float(sprite.height)) * spriteH);

    // Screen X position
    int drawX = int((0.5f + viewAngle / FOV) * screenWidth - spriteW / 2);

    // Align sprite BOTTOM to the floor line
    int drawY = floorLine - spriteH;

    drawSpriteScaled(&sprite, drawX, drawY, float(spriteW) / sprite.width,
                     pixels, screenWidth, screenHeight);
  }
}

int getEnemyCount() { return enemyCount; }

Enemy &getEnemy(int index) { return enemies[index]; }
