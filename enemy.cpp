#include "enemy.h"
#include "map.h"
#include "player.h"
#include "projectile.h"
#include "sprite.h"
#include <cmath>
#include <cstdio>

#define MAX_ENEMIES 10
static Enemy enemies[MAX_ENEMIES];
static int enemyCount = 6;
static Sprite allAngleSprites[ENEMY_TOTAL_FRAMES][8];

// Enemy AI constants
static const float ENEMY_MOVE_SPEED = 1.8f;
static const float ENEMY_CHASE_RANGE = 12.0f;
static const float ENEMY_MIN_DISTANCE = 0.8f;
static const float ENEMY_FACING_THRESHOLD = 0.005f;
static const float ENEMY_ANGLE_CHANGE_THRESHOLD = 0.3f;
static const float ENEMY_MEMORY_TIME = 3.0f;
static const float ENEMY_STUCK_TIME = 0.5f;
static const float ENEMY_RADIUS = 0.25f;
static const float ENEMY_WALL_BUFFER = 0.25f; // Keep this distance from walls

// Enemy AI states
// Enemy AI states
enum EnemyState { IDLE, CHASING, SEARCHING, UNSTUCK, ALERT };

struct EnemyAI {
  EnemyState state;
  float lastSeenX;
  float lastSeenY;
  float memoryTimer;
  float stuckTimer;
  float lastMovedX;
  float lastMovedY;
  float unstuckAngle;
  // NEW FIELDS:
  float alertTimer;
  float searchTimer;
  float patrolAngle;
  int searchPoints;
};

static EnemyAI enemyAI[MAX_ENEMIES];

struct AngleFileInfo {
  int angle1;
  int angle2;
};

static const AngleFileInfo viewToFile[8] = {{1, -1}, {2, 8}, {3, 7}, {4, 6},
                                            {5, -1}, {4, 6}, {3, 7}, {2, 8}};

static const bool shouldMirror[8] = {false, false, false, false,
                                     false, true,  true,  true};

static const char frameLetters[ENEMY_TOTAL_FRAMES] = {
    'A', 'B', 'C', 'D',                          // Walk frames 0-3
    'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', // Shoot frames 4-12
    'N',                                         // Pain frame 13
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', // Death frames 14-22
    'X', 'Y', 'Z', '[', ']', '*'}; // XDEATH when higher the a trashhold

static void buildEnemySpritePath(char *out, const char *base, char frame,
                                 const AngleFileInfo &info, bool isBillboard) {
  if (isBillboard) {
    sprintf(out, "sprites/slhv/%s%c0.png", base, frame);
  } else if (info.angle2 == -1) {
    sprintf(out, "sprites/slhv/%s%c%d.png", base, frame, info.angle1);
  } else {
    sprintf(out, "sprites/slhv/%s%c%d%c%d.png", base, frame, info.angle1, frame,
            info.angle2);
  }
}

bool loadEnemySprites() {
  for (int f = 0; f < ENEMY_TOTAL_FRAMES; f++) {
    bool isBillboard = (f >= 14); // Death frames O-W are billboards

    if (isBillboard) {
      char path[256];
      buildEnemySpritePath(path, "SLHV", frameLetters[f], viewToFile[0], true);

      Sprite billboardSprite;
      if (!loadSprite(&billboardSprite, path)) {
        printf("Failed to load: %s\n", path);
        return false;
      }

      // Copy to all 8 angles
      for (int a = 0; a < 8; a++) {
        allAngleSprites[f][a] = billboardSprite;
      }
    } else {
      // Load 8-angle sprites (walk, shoot, pain)
      for (int a = 0; a < 8; a++) {
        char path[256];
        buildEnemySpritePath(path, "SLHV", frameLetters[f], viewToFile[a],
                             false);
        if (!loadSprite(&allAngleSprites[f][a], path)) {
          printf("Failed to load: %s\n", path);
          return false;
        }
      }
    }
  }

  printf("Enemy sprites loaded (%d frames, 8 angles)\n", ENEMY_TOTAL_FRAMES);
  return true;
}

void cleanupEnemySprites() {
  for (int f = 0; f < ENEMY_TOTAL_FRAMES; f++) {
    bool isBillboard = (f >= 14);

    if (isBillboard) {
      if (allAngleSprites[f][0].pixels) {
        delete[] allAngleSprites[f][0].pixels;
        allAngleSprites[f][0].pixels = nullptr;
      }
    } else {
      for (int a = 0; a < 8; a++) {
        if (allAngleSprites[f][a].pixels) {
          delete[] allAngleSprites[f][a].pixels;
          allAngleSprites[f][a].pixels = nullptr;
        }
      }
    }
  }
}

void initEnemies() {
  float spawnPositions[][2] = {{12.0f, 10.0f}, {3.0f, 3.0f},   {20.0f, 3.0f},
                               {3.0f, 15.0f},  {20.0f, 16.0f}, {12.0f, 19.5f}};

  for (int i = 0; i < enemyCount; i++) {
    Enemy &e = enemies[i];
    e.x = spawnPositions[i][0];
    e.y = spawnPositions[i][1];
    e.vx = 0.0f;
    e.vy = 0.0f;
    e.prevX = e.x;
    e.prevY = e.y;
    e.facingAngle = 0.0f;
    e.frameIndex = 0;
    e.frameCount = ENEMY_WALK_FRAMES;
    e.animTimer = 0.0f;
    e.animSpeed = 0.15f;
    e.alive = true;
    e.animState = ANIM_IDLE;
    e.shootTimer = 0.0f;
    e.shootCooldown = 0.0f;
    e.health = ENEMY_MAX_HEALTH;

    enemyAI[i].state = IDLE;
    enemyAI[i].lastSeenX = e.x;
    enemyAI[i].lastSeenY = e.y;
    enemyAI[i].memoryTimer = 0.0f;
    enemyAI[i].stuckTimer = 0.0f;
    enemyAI[i].lastMovedX = e.x;
    enemyAI[i].lastMovedY = e.y;
    enemyAI[i].unstuckAngle = 0.0f;
  }

  printf("Initialized %d enemies\n", enemyCount);
}

static bool hasLineOfSight(float x1, float y1, float x2, float y2) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  float dist = sqrtf(dx * dx + dy * dy);

  if (dist < 0.1f)
    return true;

  dx /= dist;
  dy /= dist;

  float step = 0.1f;
  for (float t = 0; t < dist; t += step) {
    float x = x1 + dx * t;
    float y = y1 + dy * t;
    if (getMapTile((int)y, (int)x) == 1) {
      return false;
    }
  }
  return true;
}

// SIMPLIFIED: Check if a position would collide with walls
static bool isPositionValid(float x, float y) {
  // Check center
  if (getMapTile((int)y, (int)x) == 1) {
    return false;
  }

  // Check 4 cardinal directions for the buffer zone
  if (getMapTile((int)y, (int)(x + ENEMY_WALL_BUFFER)) == 1)
    return false;
  if (getMapTile((int)y, (int)(x - ENEMY_WALL_BUFFER)) == 1)
    return false;
  if (getMapTile((int)(y + ENEMY_WALL_BUFFER), (int)x) == 1)
    return false;
  if (getMapTile((int)(y - ENEMY_WALL_BUFFER), (int)x) == 1)
    return false;

  return true;
}

void damageEnemy(int enemyIndex, int damage) {
  if (enemyIndex < 0 || enemyIndex >= enemyCount)
    return;

  Enemy &e = enemies[enemyIndex];
  if (!e.alive || e.animState == ANIM_DEATH || e.animState == ANIM_XDEATH) {
    return;
  }

  printf("Enemy %d took %d damage! (health %d -> %d)\n", enemyIndex, damage,
         e.health, e.health - damage);

  e.health -= damage;

  if (e.health <= 0) {
    if (e.health <= -ENEMY_XDEATH_TRASHHOLD) {
      e.animState = ANIM_XDEATH;
      e.frameIndex = 23;
      e.animTimer = 0.0f;
      e.vx = 0;
      e.vy = 0;
    } else {
      e.animState = ANIM_DEATH;
      e.frameIndex = 14;
      e.animTimer = 0.0f;
      e.vx = 0;
      e.vy = 0;
    }
  } else {
    if ((rand() % 256) < ENEMY_PAIN_CHANCE) {
      e.animState = ANIM_PAIN;
      e.frameIndex = 13;
      e.animTimer = 0.0f;
      e.shootTimer = 0.0f; // Interrupt shooting
    }
  }
}

int hitscanCheckEnemy() {
  const float MAX_RANGE = 20.0f;

  float rayDX = cosf(playerAngle);
  float rayDY = sinf(playerAngle);

  for (int i = 0; i < enemyCount; i++) {
    Enemy &e = enemies[i];
    if (!e.alive || e.animState == ANIM_DEATH)
      continue;

    float toEnemyX = e.x - playerX;
    float toEnemyY = e.y - playerY;
    float dist = sqrtf(toEnemyX * toEnemyX + toEnemyY * toEnemyY);

    if (dist > MAX_RANGE)
      continue;

    float dotProduct = toEnemyX * rayDX + toEnemyY * rayDY;
    if (dotProduct < 0)
      continue;

    float closestX = playerX + rayDX * dotProduct;
    float closestY = playerY + rayDY * dotProduct;

    float dx = e.x - closestX;
    float dy = e.y - closestY;
    float distToRay = sqrtf(dx * dx + dy * dy);

    if (distToRay < ENEMY_RADIUS) {
      if (hasLineOfSight(playerX, playerY, e.x, e.y)) {
        return i;
      }
    }
  }
  return -1;
}

void updateEnemies(float dt) {
  for (int i = 0; i < enemyCount; i++) {
    Enemy &e = enemies[i];
    EnemyAI &ai = enemyAI[i];

    if (!e.alive)
      continue;

    // Handle death animation
    if (e.animState == ANIM_DEATH || e.animState == ANIM_XDEATH) {
      e.animTimer += dt;
      if (e.animTimer >= 0.20f) {
        e.animTimer = 0.0f;
        e.frameIndex++;
        if (e.frameIndex >= 29) {
          e.frameIndex = 28;
          e.alive = false;
        }
      }
      continue;
    }

    // Handle pain animation
    if (e.animState == ANIM_PAIN) {
      e.animTimer += dt;
      if (e.animTimer >= 0.25f) {
        e.animState = ANIM_IDLE;
        e.frameIndex = 0;
        e.animTimer = 0.0f;
      }
      continue;
    }

    float oldX = e.x;
    float oldY = e.y;

    float dx = playerX - e.x;
    float dy = playerY - e.y;
    float distToPlayer = sqrtf(dx * dx + dy * dy);

    bool canSeePlayer = hasLineOfSight(e.x, e.y, playerX, playerY);

    if (e.shootCooldown > 0.0f) {
      e.shootCooldown -= dt;
    }

    // Stuck detection - less sensitive
    float moveDist = sqrtf((e.x - ai.lastMovedX) * (e.x - ai.lastMovedX) +
                           (e.y - ai.lastMovedY) * (e.y - ai.lastMovedY));

    if (moveDist < 0.15f && (ai.state == CHASING || ai.state == SEARCHING)) {
      ai.stuckTimer += dt;
      if (ai.stuckTimer > 2.5f) {
        ai.state = UNSTUCK;
        float angleToPlayer = atan2f(dy, dx);
        ai.unstuckAngle =
            angleToPlayer + (rand() % 2 ? 1.0f : -1.0f) *
                                (M_PI / 4.0f + (rand() % 100) / 200.0f);
        ai.stuckTimer = 0.0f;
      }
    } else {
      ai.stuckTimer = 0.0f;
      ai.lastMovedX = e.x;
      ai.lastMovedY = e.y;
    }

    // REDUCED detection range
    float detectionRange = 8.0f; // Reduced from 12.0f

    // AI State Machine - IMPROVED
    if (canSeePlayer && distToPlayer < detectionRange && ai.state != UNSTUCK) {
      ai.state = CHASING;
      ai.lastSeenX = playerX;
      ai.lastSeenY = playerY;
      ai.memoryTimer = ENEMY_MEMORY_TIME;
      ai.alertTimer = 10.0f;
    } else if (ai.state == CHASING && !canSeePlayer) {
      ai.state = SEARCHING;
      ai.searchTimer = 6.0f;
      ai.searchPoints = 0;
      ai.patrolAngle = atan2f(dy, dx);
    } else if (ai.state == SEARCHING) {
      ai.memoryTimer -= dt;
      ai.searchTimer -= dt;

      // Check if reached current search point
      float dxSearch = ai.lastSeenX - e.x;
      float dySearch = ai.lastSeenY - e.y;
      float distToSearchPoint =
          sqrtf(dxSearch * dxSearch + dySearch * dySearch);

      // Create search pattern around last seen position
      if (distToSearchPoint < 1.2f && ai.searchPoints < 4) {
        ai.searchPoints++;
        float searchRadius = 2.5f;
        ai.patrolAngle += (M_PI / 2.0f) + ((rand() % 100) / 100.0f - 0.5f);
        ai.lastSeenX += cosf(ai.patrolAngle) * searchRadius;
        ai.lastSeenY += sinf(ai.patrolAngle) * searchRadius;
      }

      // Give up searching after timer expires
      if (ai.searchTimer <= 0.0f) {
        ai.state = ALERT;
        ai.alertTimer = 8.0f;
      }
    } else if (ai.state == ALERT) {
      ai.alertTimer -= dt;
      if (ai.alertTimer <= 0.0f) {
        ai.state = IDLE;
      }
    } else if (ai.state == UNSTUCK) {
      ai.memoryTimer += dt;
      if (ai.memoryTimer > 1.0f) {
        ai.memoryTimer = 0.0f;
        ai.state =
            (canSeePlayer && distToPlayer < detectionRange) ? CHASING : IDLE;
      }
    }

    // Shooting logic - reduced range
    float shootRange = 6.0f; // Reduced shoot range
    bool shouldShoot = false;
    if (ai.state == CHASING && canSeePlayer && distToPlayer < shootRange &&
        e.shootCooldown <= 0.0f && e.animState != ANIM_SHOOT) {
      shouldShoot = true;
    }

    if (shouldShoot) {
      e.animState = ANIM_SHOOT;
      e.frameIndex = ENEMY_WALK_FRAMES;
      e.shootTimer = 0.0f;
      e.animTimer = 0.0f;
      e.shootCooldown = ENEMY_SHOOT_COOLDOWN;
    }

    // Movement
    float targetX, targetY, distToTarget;
    bool shouldMove = false;

    if (e.animState != ANIM_SHOOT) {
      if (ai.state == CHASING) {
        targetX = playerX;
        targetY = playerY;
        distToTarget = distToPlayer;
        shouldMove = distToTarget > ENEMY_MIN_DISTANCE;
      } else if (ai.state == SEARCHING) {
        dx = ai.lastSeenX - e.x;
        dy = ai.lastSeenY - e.y;
        distToTarget = sqrtf(dx * dx + dy * dy);
        targetX = ai.lastSeenX;
        targetY = ai.lastSeenY;
        shouldMove = distToTarget > 0.8f;
      } else if (ai.state == ALERT) {
        // Standing still in alert state
        e.vx = 0;
        e.vy = 0;
      } else if (ai.state == UNSTUCK) {
        float unstuckDist = 2.0f;
        targetX = e.x + cosf(ai.unstuckAngle) * unstuckDist;
        targetY = e.y + sinf(ai.unstuckAngle) * unstuckDist;
        dx = targetX - e.x;
        dy = targetY - e.y;
        distToTarget = sqrtf(dx * dx + dy * dy);
        shouldMove = true;
      } else {
        e.vx = 0;
        e.vy = 0;
      }

      if (shouldMove && distToTarget > 0.1f) {
        dx = targetX - e.x;
        dy = targetY - e.y;
        float normDist = sqrtf(dx * dx + dy * dy);
        dx /= normDist;
        dy /= normDist;

        // Movement speed adjustment
        float moveSpeed = ENEMY_MOVE_SPEED;
        if (ai.state == SEARCHING) {
          moveSpeed *= 0.7f; // Slower when searching
        }

        float newX = e.x + dx * moveSpeed * dt;
        float newY = e.y + dy * moveSpeed * dt;

        // Try full movement first
        if (isPositionValid(newX, newY)) {
          e.x = newX;
          e.y = newY;
          e.vx = dx * moveSpeed;
          e.vy = dy * moveSpeed;
        }
        // Try just X movement (slide along Y wall)
        else if (isPositionValid(newX, oldY)) {
          e.x = newX;
          e.y = oldY;
          e.vx = dx * moveSpeed;
          e.vy = 0;
        }
        // Try just Y movement (slide along X wall)
        else if (isPositionValid(oldX, newY)) {
          e.x = oldX;
          e.y = newY;
          e.vx = 0;
          e.vy = dy * moveSpeed;
        }
        // Can't move at all
        else {
          e.vx = 0;
          e.vy = 0;
        }
      }
    } else {
      e.vx = 0;
      e.vy = 0;
    }

    // Animation
    float moveDX = e.x - oldX;
    float moveDY = e.y - oldY;
    float moveMagnitude = sqrtf(moveDX * moveDX + moveDY * moveDY);
    bool isMoving = moveMagnitude > ENEMY_FACING_THRESHOLD;

    if (e.animState == ANIM_SHOOT) {
      e.shootTimer += dt;
      e.animTimer += dt;

      dx = e.x - playerX;
      dy = e.y - playerY;
      e.facingAngle = atan2f(dy, dx) + M_PI;

      if (e.animTimer >= e.animSpeed) {
        e.animTimer = 0.0f;
        e.frameIndex++;

        if (e.frameIndex == 11) {
          spawnEnemyProjectile(e.x, e.y, playerX, playerY);
        }

        if (e.frameIndex >= 13) {
          e.animState = ANIM_IDLE;
          e.frameIndex = 0;
          e.shootTimer = 0.0f;
        }
      }
    } else if (isMoving) {
      e.animState = ANIM_WALK;

      float newFacingAngle = atan2f(moveDY, moveDX);
      float angleDiff = newFacingAngle - e.facingAngle;
      while (angleDiff > M_PI)
        angleDiff -= 2 * M_PI;
      while (angleDiff < -M_PI)
        angleDiff += 2 * M_PI;

      if (fabs(angleDiff) > ENEMY_ANGLE_CHANGE_THRESHOLD) {
        e.facingAngle = newFacingAngle;
      } else if (fabs(angleDiff) > 0.1f) {
        e.facingAngle += angleDiff * 0.3f;
      }

      e.prevX = oldX;
      e.prevY = oldY;

      e.animTimer += dt;
      if (e.animTimer >= e.animSpeed) {
        e.animTimer = 0.0f;
        e.frameIndex = (e.frameIndex + 1) % ENEMY_WALK_FRAMES;
      }
    } else {
      e.animState = ANIM_IDLE;
      e.animTimer = 0.0f;
      e.frameIndex = 0;
    }
  }
}
void renderEnemies(uint32_t *pixels, int screenWidth, int screenHeight,
                   float *buffer) {
  int w = screenWidth;
  int h = screenHeight;
  float *zBuffer = buffer;
  const float FOV = M_PI / 3.0f;

  for (int i = 0; i < enemyCount; i++) {
    Enemy &e = enemies[i];
    float dx = e.x - playerX;
    float dy = e.y - playerY;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 0.1f)
      continue;

    float angleToEnemy = atan2f(dy, dx);
    float relAngle = angleToEnemy - playerAngle;
    while (relAngle < -M_PI)
      relAngle += 2 * M_PI;
    while (relAngle > M_PI)
      relAngle -= 2 * M_PI;
    if (fabs(relAngle) > FOV * 0.5f)
      continue;

    float spriteAngle = e.facingAngle - angleToEnemy + M_PI;
    while (spriteAngle < 0)
      spriteAngle += 2 * M_PI;
    while (spriteAngle >= 2 * M_PI)
      spriteAngle -= 2 * M_PI;

    int angleIndex = int(((spriteAngle / (2 * M_PI)) * 8) + 0.5) & 7;
    bool mirror = shouldMirror[angleIndex];
    Sprite &sprite = allAngleSprites[e.frameIndex][angleIndex];

    float corrected = dist * cosf(relAngle);
    if (corrected < 0.1f)
      corrected = 0.1f;

    int wallHeight = int((float(h) / corrected) * 0.8f);
    int floorLine = (h / 2) + wallHeight;
    int targetHeight = int((float(h) / corrected) * 1.1f);

    // Death frames: use FIRST death frame's height as reference
    Sprite &refSprite = allAngleSprites[0][angleIndex];
    float scale = float(targetHeight) / refSprite.height;

    // Sprite dimensions at this scale
    int spriteW = int(sprite.width * scale);
    int spriteH = int(sprite.height * scale);
    int drawX = int((0.5f + relAngle / FOV) * w - spriteW / 2);
    int drawY = floorLine - spriteH;

    if (zBuffer) {
      float renderDepth = corrected;
      // Make death sprites slightly closer so they render over floor
      if (e.frameIndex >= 14) {
        renderDepth -= 0.2f; // Bring corpses 0.1 units closer
      }

      drawSpriteScaledWithDepth(&sprite, drawX, drawY, scale, mirror, pixels, w,
                                h, zBuffer, renderDepth);
    } else {
      drawSpriteScaled(&sprite, drawX, drawY, scale, mirror, pixels, w, h);
    }
  }
}
int getEnemyCount() { return enemyCount; }
Enemy &getEnemy(int i) { return enemies[i]; }
