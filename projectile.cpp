#include "projectile.h"
#include "map.h"
#include "player.h"
#include <cmath>
#include <cstdio>

static Projectile projectiles[PROJECTILE_MAX_ACTIVE];
static Sprite projectileSprites[PROJECTILE_MAX_FRAMES]
                               [8]; // 11 frames, 8 angles

static const float PROJECTILE_SPEED = 4.0f;
static const float PROJECTILE_MAX_LIFETIME = 3.0f;
static const float PROJECTILE_COLLISION_RADIUS = 0.3f;
const float PROJECTILE_HEIGHT_OFFSET = 0.10f; // units above ground
static const char frameLetters[PROJECTILE_MAX_FRAMES] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K'};

struct AngleFileInfo {
  int angle1;
  int angle2;
};

static const AngleFileInfo viewToFile[8] = {{1, -1}, {2, 8}, {3, 7}, {4, 6},
                                            {5, -1}, {4, 6}, {3, 7}, {2, 8}};

static const bool shouldMirror[8] = {false, false, false, false,
                                     false, true,  true,  true};

// Frames A and B have angles, C-K are single sprites (dissipating)
static const bool frameHasAngles[PROJECTILE_MAX_FRAMES] = {
    true,  true, // A, B have angles
    false, false, false, false, false,
    false, false, false, false // C-K are single sprites
};

static void buildProjectileSpritePath(char *out, const char *base, char frame,
                                      const AngleFileInfo &info,
                                      bool hasAngles) {
  if (!hasAngles) {
    // Dissipating frames: just SHBAC0, SHBAD0, etc.
    sprintf(out, "sprites/slhv/%s%c0.png", base, frame);
  } else if (info.angle2 == -1) {
    sprintf(out, "sprites/slhv/%s%c%d.png", base, frame, info.angle1);
  } else {
    sprintf(out, "sprites/slhv/%s%c%d%c%d.png", base, frame, info.angle1, frame,
            info.angle2);
  }
}

bool loadProjectileSprites() {
  // Load SHBA sprites (Soldier Harvester Base Attack)
  for (int f = 0; f < PROJECTILE_MAX_FRAMES; f++) {
    if (frameHasAngles[f]) {
      // Load all 8 angles for frames A and B
      for (int a = 0; a < 8; a++) {
        char path[256];
        buildProjectileSpritePath(path, "SHBA", frameLetters[f], viewToFile[a],
                                  true);

        if (!loadSprite(&projectileSprites[f][a], path)) {
          printf("Failed to load projectile sprite: %s\n", path);
          return false;
        }
      }
    } else {
      // Load single sprite for dissipating frames (C-K)
      char path[256];
      buildProjectileSpritePath(path, "SHBA", frameLetters[f], viewToFile[0],
                                false);

      if (!loadSprite(&projectileSprites[f][0], path)) {
        printf("Failed to load projectile sprite: %s\n", path);
        return false;
      }

      // Copy the same sprite to all angle slots since it's omnidirectional
      for (int a = 1; a < 8; a++) {
        projectileSprites[f][a] = projectileSprites[f][0];
      }
    }
  }

  printf("Projectile sprites loaded (%d frames, 8 angles)\n",
         PROJECTILE_MAX_FRAMES);
  return true;
}

void cleanupProjectileSprites() {
  for (int f = 0; f < PROJECTILE_MAX_FRAMES; f++) {
    if (frameHasAngles[f]) {
      // Clean up all angles for A and B
      for (int a = 0; a < 8; a++) {
        if (projectileSprites[f][a].pixels) {
          delete[] projectileSprites[f][a].pixels;
          projectileSprites[f][a].pixels = nullptr;
        }
      }
    } else {
      // Only clean up the first sprite for C-K (others are copies)
      if (projectileSprites[f][0].pixels) {
        delete[] projectileSprites[f][0].pixels;
        projectileSprites[f][0].pixels = nullptr;
      }
      // Clear the copied references
      for (int a = 1; a < 8; a++) {
        projectileSprites[f][a].pixels = nullptr;
      }
    }
  }
}

void initProjectiles() {
  for (int i = 0; i < PROJECTILE_MAX_ACTIVE; i++) {
    projectiles[i].active = false;
  }
  printf("Projectile system initialized\n");
}

void spawnEnemyProjectile(float x, float y, float targetX, float targetY) {
  // Find inactive projectile slot
  for (int i = 0; i < PROJECTILE_MAX_ACTIVE; i++) {
    if (!projectiles[i].active) {
      Projectile &p = projectiles[i];

      p.x = x;
      p.y = y;

      // Calculate direction to target
      float dx = targetX - x;
      float dy = targetY - y;
      float dist = sqrtf(dx * dx + dy * dy);

      if (dist > 0.1f) {
        dx /= dist;
        dy /= dist;
      }

      p.vx = dx * PROJECTILE_SPEED;
      p.vy = dy * PROJECTILE_SPEED;
      p.angle = atan2f(dy, dx);

      p.active = true;
      p.type = PROJ_ENEMY_FIREBALL;
      p.frameIndex = 0;
      p.animTimer = 0.0f;
      p.animSpeed = 0.12f; // Fast animation
      p.lifetime = 0.0f;
      p.maxLifetime = PROJECTILE_MAX_LIFETIME;
      p.distanceTraveled = 0.0f;

      break;
    }
  }
}

void spawnPlayerProjectile(float x, float y, float angle) {
  // For future player weapons
  for (int i = 0; i < PROJECTILE_MAX_ACTIVE; i++) {
    if (!projectiles[i].active) {
      Projectile &p = projectiles[i];

      p.x = x;
      p.y = y;
      p.vx = cosf(angle) * PROJECTILE_SPEED * 1.5f;
      p.vy = sinf(angle) * PROJECTILE_SPEED * 1.5f;
      p.angle = angle;

      p.active = true;
      p.type = PROJ_PLAYER_BULLET;
      p.frameIndex = 0;
      p.animTimer = 0.0f;
      p.animSpeed = 0.1f;
      p.lifetime = 0.0f;
      p.maxLifetime = PROJECTILE_MAX_LIFETIME;
      p.distanceTraveled = 0.0f;

      break;
    }
  }
}

void updateProjectiles(float dt) {
  for (int i = 0; i < PROJECTILE_MAX_ACTIVE; i++) {
    Projectile &p = projectiles[i];
    if (!p.active)
      continue;

    // Update lifetime
    p.lifetime += dt;
    if (p.lifetime >= p.maxLifetime) {
      p.active = false;
      continue;
    }

    // Move projectile
    float oldX = p.x;
    float oldY = p.y;

    p.x += p.vx * dt;
    p.y += p.vy * dt;

    // Track distance traveled
    float dx = p.x - oldX;
    float dy = p.y - oldY;
    p.distanceTraveled += sqrtf(dx * dx + dy * dy);

    // Wall collision
    if (getMapTile((int)p.y, (int)p.x) == 1) {
      p.active = false;
      continue;
    }

    // Update animation based on distance traveled
    // Frames A, B for travel; C-K for dissipation
    if (p.distanceTraveled < 4.0f) {
      // Travel frames (A, B)
      int targetFrame = (int)(p.distanceTraveled);
      p.frameIndex = (targetFrame < 2) ? targetFrame : 1;
    } else {
      // Dissipate frames (C, D, E, F, G, H, I, J, K)
      p.animTimer += dt;
      if (p.animTimer >= p.animSpeed) {
        p.animTimer = 0.0f;
        p.frameIndex++;
        if (p.frameIndex >= PROJECTILE_MAX_FRAMES) {
          p.active = false;
          continue;
        }
      }
    }
  }
}

bool checkProjectilePlayerHit(float playerX, float playerY, float radius) {
  for (int i = 0; i < PROJECTILE_MAX_ACTIVE; i++) {
    Projectile &p = projectiles[i];
    if (!p.active || p.type != PROJ_ENEMY_FIREBALL)
      continue;

    float dx = p.x - playerX;
    float dy = p.y - playerY;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < radius + PROJECTILE_COLLISION_RADIUS) {
      p.active = false; // Destroy projectile
      return true;
    }
  }
  return false;
}

void renderProjectiles(uint32_t *pixels, int screenWidth, int screenHeight,
                       float *zBuffer) {
  int w = screenWidth;
  int h = screenHeight;
  const float FOV = M_PI / 3.0f;

  for (int i = 0; i < PROJECTILE_MAX_ACTIVE; i++) {
    Projectile &p = projectiles[i];
    if (!p.active)
      continue;

    float dx = p.x - playerX;
    float dy = p.y - playerY;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < 0.1f)
      continue;

    float angleToProj = atan2f(dy, dx);
    float relAngle = angleToProj - playerAngle;

    while (relAngle < -M_PI)
      relAngle += 2 * M_PI;
    while (relAngle > M_PI)
      relAngle -= 2 * M_PI;

    if (fabs(relAngle) > FOV * 0.5f)
      continue;

    // Calculate sprite angle (only matters for frames A and B)
    int angleIndex = 0;
    bool mirror = false;

    if (frameHasAngles[p.frameIndex]) {
      float spriteAngle = p.angle - angleToProj + M_PI;
      while (spriteAngle < 0)
        spriteAngle += 2 * M_PI;
      while (spriteAngle >= 2 * M_PI)
        spriteAngle -= 2 * M_PI;

      angleIndex = int((spriteAngle / (2 * M_PI)) * 8) & 7;
      mirror = shouldMirror[angleIndex];
    }

    Sprite &sprite = projectileSprites[p.frameIndex][angleIndex];

    float corrected = dist * cosf(relAngle);
    if (corrected < 0.1f)
      corrected = 0.1f;

    int spriteH = int((float(h) / corrected) * 0.6f); // Smaller than enemies
    int spriteW =
        int((float(sprite.width) / float(sprite.height)) * float(spriteH));
    // Instead of floorLine calculation
    float projHeight =
        (0.5f + PROJECTILE_HEIGHT_OFFSET); // fractional vertical offset
    int drawY = int((h / 2) - (spriteH * projHeight));
    int drawX = int((0.5f + relAngle / FOV) * w - spriteW / 2);

    if (zBuffer) {
      drawSpriteScaledWithDepth(&sprite, drawX, drawY,
                                float(spriteW) / sprite.width, mirror, pixels,
                                w, h, zBuffer, corrected);
    } else {
      drawSpriteScaled(&sprite, drawX, drawY, float(spriteW) / sprite.width,
                       mirror, pixels, w, h);
    }
  }
}
