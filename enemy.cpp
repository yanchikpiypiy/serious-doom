#include "enemy.h"
#include "map.h"
#include "player.h"
#include "projectile.h" // ADD THIS LINE
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
// Enemy AI states
enum EnemyState { IDLE, CHASING, SEARCHING, UNSTUCK };

struct EnemyAI {
  EnemyState state;
  float lastSeenX;
  float lastSeenY;
  float memoryTimer;
  float stuckTimer;
  float lastMovedX;
  float lastMovedY;
  float unstuckAngle;
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
    'A', 'B', 'C', 'D',                         // Walk frames 0-3
    'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M' // Shoot frames 4-12
};

static void buildEnemySpritePath(char *out, const char *base, char frame,
                                 const AngleFileInfo &info) {
  if (info.angle2 == -1)
    sprintf(out, "sprites/slhv/%s%c%d.png", base, frame, info.angle1);
  else
    sprintf(out, "sprites/slhv/%s%c%d%c%d.png", base, frame, info.angle1, frame,
            info.angle2);
}

bool loadEnemySprites() {
  for (int f = 0; f < ENEMY_TOTAL_FRAMES; f++) {
    for (int a = 0; a < 8; a++) {
      char path[256];
      buildEnemySpritePath(path, "SLHV", frameLetters[f], viewToFile[a]);
      if (!loadSprite(&allAngleSprites[f][a], path)) {
        printf("Failed to load: %s\n", path);
        return false;
      }
    }
  }

  printf("Enemy sprites loaded (%d frames, 8 angles)\n", ENEMY_TOTAL_FRAMES);
  return true;
}

void cleanupEnemySprites() {
  for (int f = 0; f < ENEMY_TOTAL_FRAMES; f++) {
    for (int a = 0; a < 8; a++) {
      if (allAngleSprites[f][a].pixels) {
        delete[] allAngleSprites[f][a].pixels;
        allAngleSprites[f][a].pixels = nullptr;
      }
    }
  }
}

void initEnemies() {
  // Better spawn positions - spread across the map
  float spawnPositions[][2] = {
      {12.0f, 10.0f}, // Main hall
      {3.0f, 3.0f},   // North wing
      {20.0f, 3.0f},  // Northeast
      {3.0f, 16.0f},  // West courtyard
      {20.0f, 16.0f}, // East storage
      {12.0f, 19.0f}  // Lower corridor
  };

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

    // Animation state
    e.animState = ANIM_IDLE;
    e.shootTimer = 0.0f;
    e.shootCooldown = 0.0f;

    // Initialize AI state
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

// Line-of-sight check
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

// Calculate shortest angular distance
static float angleDifference(float a1, float a2) {
  float diff = a2 - a1;
  while (diff > M_PI)
    diff -= 2 * M_PI;
  while (diff < -M_PI)
    diff += 2 * M_PI;
  return fabs(diff);
}

void updateEnemies(float dt) {
  for (int i = 0; i < enemyCount; i++) {
    Enemy &e = enemies[i];
    EnemyAI &ai = enemyAI[i];
    if (!e.alive)
      continue;

    float oldX = e.x;
    float oldY = e.y;

    float dx = playerX - e.x;
    float dy = playerY - e.y;
    float distToPlayer = sqrtf(dx * dx + dy * dy);

    bool canSeePlayer = hasLineOfSight(e.x, e.y, playerX, playerY);

    // Update shoot cooldown
    if (e.shootCooldown > 0.0f) {
      e.shootCooldown -= dt;
    }

    // Check if enemy is stuck
    float moveDist = sqrtf((e.x - ai.lastMovedX) * (e.x - ai.lastMovedX) +
                           (e.y - ai.lastMovedY) * (e.y - ai.lastMovedY));

    if (moveDist < 0.05f && (ai.state == CHASING || ai.state == SEARCHING)) {
      ai.stuckTimer += dt;
      if (ai.stuckTimer > ENEMY_STUCK_TIME) {
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

    // AI State Machine
    if (canSeePlayer && distToPlayer < ENEMY_CHASE_RANGE &&
        ai.state != UNSTUCK) {
      ai.state = CHASING;
      ai.lastSeenX = playerX;
      ai.lastSeenY = playerY;
      ai.memoryTimer = ENEMY_MEMORY_TIME;

    } else if (ai.state == CHASING && !canSeePlayer) {
      ai.state = SEARCHING;

    } else if (ai.state == SEARCHING) {
      ai.memoryTimer -= dt;
      if (ai.memoryTimer <= 0.0f) {
        ai.state = IDLE;
      }
    } else if (ai.state == UNSTUCK) {
      ai.memoryTimer += dt;
      if (ai.memoryTimer > 1.0f) {
        ai.memoryTimer = 0.0f;
        ai.state = canSeePlayer ? CHASING : IDLE;
      }
    }

    // Shooting logic - check if enemy should shoot
    bool shouldShoot = false;
    if (ai.state == CHASING && canSeePlayer &&
        distToPlayer < ENEMY_SHOOT_RANGE && e.shootCooldown <= 0.0f &&
        e.animState != ANIM_SHOOT) {
      shouldShoot = true;
    }

    // Start shooting animation
    if (shouldShoot) {
      e.animState = ANIM_SHOOT;
      e.frameIndex = ENEMY_WALK_FRAMES; // Start at frame E (index 4)
      e.shootTimer = 0.0f;
      e.animTimer = 0.0f;
      e.shootCooldown = ENEMY_SHOOT_COOLDOWN;
    }

    // Movement based on state (don't move while shooting)
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
        shouldMove = distToTarget > 0.5f;

        if (distToTarget < 0.5f) {
          ai.state = IDLE;
        }

      } else if (ai.state == UNSTUCK) {
        float unstuckDist = 2.0f;
        targetX = e.x + cosf(ai.unstuckAngle) * unstuckDist;
        targetY = e.y + sinf(ai.unstuckAngle) * unstuckDist;
        dx = targetX - e.x;
        dy = targetY - e.y;
        distToTarget = sqrtf(dx * dx + dy * dy);
        shouldMove = true;

      } else {
        // IDLE
        e.vx = 0;
        e.vy = 0;
      }

      // Execute movement
      if (shouldMove && distToTarget > 0.1f) {
        dx = targetX - e.x;
        dy = targetY - e.y;
        float normDist = sqrtf(dx * dx + dy * dy);
        dx /= normDist;
        dy /= normDist;

        float newX = e.x + dx * ENEMY_MOVE_SPEED * dt;
        float newY = e.y + dy * ENEMY_MOVE_SPEED * dt;

        if (getMapTile((int)newY, (int)newX) == 0) {
          e.x = newX;
          e.y = newY;
          e.vx = dx * ENEMY_MOVE_SPEED;
          e.vy = dy * ENEMY_MOVE_SPEED;
        } else {
          // Try sliding along walls
          if (getMapTile((int)oldY, (int)newX) == 0) {
            e.x = newX;
            e.vx = dx * ENEMY_MOVE_SPEED;
            e.vy = 0;
          } else if (getMapTile((int)newY, (int)oldX) == 0) {
            e.y = newY;
            e.vx = 0;
            e.vy = dy * ENEMY_MOVE_SPEED;
          } else {
            e.vx = 0;
            e.vy = 0;
          }
        }
      }
    } else {
      // Stop moving while shooting
      e.vx = 0;
      e.vy = 0;
    }

    // Update facing and animation
    float moveDX = e.x - oldX;
    float moveDY = e.y - oldY;
    float moveMagnitude = sqrtf(moveDX * moveDX + moveDY * moveDY);
    bool isMoving = moveMagnitude > ENEMY_FACING_THRESHOLD;

    // Handle shooting animation
    if (e.animState == ANIM_SHOOT) {
      e.shootTimer += dt;
      e.animTimer += dt;

      // Keep facing the player while Shooting
      dx = e.x - playerX;
      dy = e.y - playerY;
      e.facingAngle = atan2f(dy, dx) + M_PI;
      if (e.animTimer >= e.animSpeed) {
        e.animTimer = 0.0f;
        e.frameIndex++;

        // SPAWN PROJECTILE at frame F (index 5) - THIS IS THE KEY ADDITION
        if (e.frameIndex == 11) {
          spawnEnemyProjectile(e.x, e.y, playerX, playerY);
        }

        // Check if shooting animation is complete
        if (e.frameIndex >= ENEMY_TOTAL_FRAMES) {
          e.animState = ANIM_IDLE;
          e.frameIndex = 0;
          e.shootTimer = 0.0f;
        }
      }
    }
    // Handle walking animation
    else if (isMoving) {
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
    }
    // Idle state
    else {
      e.animState = ANIM_IDLE;
      e.animTimer = 0.0f;
      e.frameIndex = 0;
    }
  }
}

// Render enemies
void renderEnemies(uint32_t *pixels, int screenWidth, int screenHeight,
                   float *buffer) {
  int w = screenWidth;
  int h = screenHeight;
  float *zBuffer = buffer;
  const float FOV = M_PI / 3.0f;

  for (int i = 0; i < enemyCount; i++) {
    Enemy &e = enemies[i];
    if (!e.alive)
      continue;

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
    int spriteH = int((float(h) / corrected) * 1.1f);
    int spriteW =
        int((float(sprite.width) / float(sprite.height)) * float(spriteH));

    int drawX = int((0.5f + relAngle / FOV) * w - spriteW / 2);
    int drawY = floorLine - spriteH;

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

int getEnemyCount() { return enemyCount; }
Enemy &getEnemy(int i) { return enemies[i]; }
