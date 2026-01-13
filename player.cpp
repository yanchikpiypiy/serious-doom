#include "player.h"
#include "gun.h"
#include "map.h"
#include <cmath>

// Player state
float playerX = 1.5f;
float playerY = 6.5f;
float playerAngle = 0.0f;

// Input state
static bool moveForward = false;
static bool moveBackward = false;
static bool turnLeft = false;
static bool turnRight = false;

void handlePlayerInput(SDL_Keycode key, bool pressed) {
  if (key == SDLK_UP)
    moveForward = pressed;
  if (key == SDLK_DOWN)
    moveBackward = pressed;
  if (key == SDLK_LEFT)
    turnLeft = pressed;
  if (key == SDLK_RIGHT)
    turnRight = pressed;

  // Reload is instant action (only on press, not hold)
  if (key == SDLK_r && pressed) {
    startReload();
  }

  // Shoot on SPACE or Left CTRL
  if ((key == SDLK_SPACE || key == SDLK_LCTRL) && pressed) {
    startShoot();
  }
}

void updatePlayer(float deltaTime) {
  float oldX = playerX;
  float oldY = playerY;

  const float moveSpeed = 5.5f;
  const float rotSpeed = 2.0f;

  if (moveForward) {
    playerX += cos(playerAngle) * moveSpeed * deltaTime;
    playerY += sin(playerAngle) * moveSpeed * deltaTime;
  }
  if (moveBackward) {
    playerX -= cos(playerAngle) * moveSpeed * deltaTime;
    playerY -= sin(playerAngle) * moveSpeed * deltaTime;
  }
  if (turnLeft)
    playerAngle -= rotSpeed * deltaTime;
  if (turnRight)
    playerAngle += rotSpeed * deltaTime;

  // Collision
  if (getMapTile((int)playerY, (int)playerX) == 1) {
    playerX = oldX;
    playerY = oldY;
  }

  // Angle wrap
  if (playerAngle < 0)
    playerAngle += 2 * M_PI;
  if (playerAngle > 2 * M_PI)
    playerAngle -= 2 * M_PI;
}
