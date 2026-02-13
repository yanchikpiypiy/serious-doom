#include "player.h"
#include "enemy.h"
#include "gun.h"
#include "map.h"
#include <cmath>

float playerX = 1.5f;
float playerY = 21.5f;
float playerAngle = 0.0f;

static bool moveForward = false;
static bool moveBackward = false;
static bool strafeLeft = false;
static bool strafeRight = false;
static bool turnLeft = false;
static bool turnRight = false;

void handlePlayerInput(SDL_Keycode key, bool pressed) {
  // Movement
  if (key == SDLK_w)
    moveForward = pressed;
  if (key == SDLK_s)
    moveBackward = pressed;
  if (key == SDLK_a)
    strafeLeft = pressed;
  if (key == SDLK_d)
    strafeRight = pressed;

  // Turning (arrow keys)
  if (key == SDLK_LEFT)
    turnLeft = pressed;
  if (key == SDLK_RIGHT)
    turnRight = pressed;

  // Actions
  if (key == SDLK_r && pressed)
    startReload();
  if (key == SDLK_SPACE && pressed) {
    startShoot();
    int hitEnemy = hitscanCheckEnemy(); // ADD THIS
    if (hitEnemy != -1) {
      damageEnemy(hitEnemy, 70); // Deal 25 damage
      printf("Hit enemy %d!\n", hitEnemy);
    }
  }
}

// Better collision check with radius
static bool checkCollision(float x, float y, float radius = 0.3f) {
  // Check 4 corners of player bounding box
  if (getMapTile((int)(y - radius), (int)(x - radius)) == 1)
    return true;
  if (getMapTile((int)(y - radius), (int)(x + radius)) == 1)
    return true;
  if (getMapTile((int)(y + radius), (int)(x - radius)) == 1)
    return true;
  if (getMapTile((int)(y + radius), (int)(x + radius)) == 1)
    return true;
  return false;
}

void updatePlayer(float deltaTime) {
  const float moveSpeed = 3.5f;
  const float rotSpeed = 2.5f;

  float newX = playerX;
  float newY = playerY;

  // Forward/backward movement
  if (moveForward) {
    newX += cos(playerAngle) * moveSpeed * deltaTime;
    newY += sin(playerAngle) * moveSpeed * deltaTime;
  }
  if (moveBackward) {
    newX -= cos(playerAngle) * moveSpeed * deltaTime;
    newY -= sin(playerAngle) * moveSpeed * deltaTime;
  }

  // Strafe movement
  if (strafeLeft) {
    newX += cos(playerAngle - M_PI / 2) * moveSpeed * deltaTime;
    newY += sin(playerAngle - M_PI / 2) * moveSpeed * deltaTime;
  }
  if (strafeRight) {
    newX += cos(playerAngle + M_PI / 2) * moveSpeed * deltaTime;
    newY += sin(playerAngle + M_PI / 2) * moveSpeed * deltaTime;
  }

  // Apply movement with collision check
  if (!checkCollision(newX, playerY))
    playerX = newX;
  if (!checkCollision(playerX, newY))
    playerY = newY;

  // Rotation
  if (turnLeft)
    playerAngle -= rotSpeed * deltaTime;
  if (turnRight)
    playerAngle += rotSpeed * deltaTime;

  // Normalize angle
  while (playerAngle < 0)
    playerAngle += 2 * M_PI;
  while (playerAngle >= 2 * M_PI)
    playerAngle -= 2 * M_PI;
}
