// gun.cpp - Updated for Doom shotgun sprites
#include "gun.h"
#include <cstdio>
#include <sprite.h>

// Gun sprites
static Sprite gunIdle;      // SAKOA0 - Idle/ready position
static Sprite gunFire[3];   // SAKOB0, SAKOC0, SAKOD0 - Shooting animation
static Sprite gunReload[9]; // SAKOE0-SAKOM0 - Reload animation

// Gun state
bool isReloading = false;
bool isShooting = false;

static float reloadTimer = 0.0f;
static float shootTimer = 0.0f;
static int currentReloadFrame = 0;
static int currentShootFrame = 0;

// Timing constants
const float RELOAD_TIME = 1.0f;       // Total reload duration (9 frames)
const float RELOAD_FRAME_TIME = 0.1f; // Time per reload frame
const float SHOOT_TIME = 0.3f;        // Total shoot animation time
const float SHOOT_FRAME_TIME = 0.1f;  // Time per shoot frame

bool loadGunSprites() {
  // Load idle sprite - SAKOA0
  if (!loadSprite(&gunIdle, "sprites/SAKOA0.png")) {
    printf("Failed to load SAKOA0.png (idle)\n");
    return false;
  }
  printf("Loaded idle: SAKOA0\n");

  // Load shooting animation - SAKOB0, SAKOC0, SAKOD0
  const char *shootFiles[] = {
      "sprites/SAKOB0.png", // Frame 1 - gun fires
      "sprites/SAKOC0.png", // Frame 2 - recoil
      "sprites/SAKOD0.png"  // Frame 3 - returning
  };

  for (int i = 0; i < 3; i++) {
    if (!loadSprite(&gunFire[i], shootFiles[i])) {
      printf("Failed to load %s\n", shootFiles[i]);
      return false;
    }
    printf("Loaded shoot frame %d: %s\n", i + 1, shootFiles[i]);
  }

  // Load reload animation - SAKOE0 through SAKOM0
  const char *reloadFiles[] = {
      "sprites/SAKOE0.png", // Frame 1 - start opening
      "sprites/SAKOF0.png", // Frame 2
      "sprites/SAKOG0.png", // Frame 3
      "sprites/SAKOH0.png", // Frame 4
      "sprites/SAKOI0.png", // Frame 5 - shells ejecting
      "sprites/SAKOJ0.png", // Frame 6
      "sprites/SAKOK0.png", // Frame 7
      "sprites/SAKOL0.png", // Frame 8
      "sprites/SAKOM0.png"  // Frame 9 - closing
  };

  for (int i = 0; i < 9; i++) {
    if (!loadSprite(&gunReload[i], reloadFiles[i])) {
      printf("Failed to load %s\n", reloadFiles[i]);
      return false;
    }
    printf("Loaded reload frame %d: %s\n", i + 1, reloadFiles[i]);
  }

  printf("All shotgun sprites loaded successfully!\n");
  return true;
}

void cleanupGunSprites() {
  if (gunIdle.pixels)
    delete[] gunIdle.pixels;

  for (int i = 0; i < 3; i++) {
    if (gunFire[i].pixels)
      delete[] gunFire[i].pixels;
  }

  for (int i = 0; i < 9; i++) {
    if (gunReload[i].pixels)
      delete[] gunReload[i].pixels;
  }
}

void updateGun(float deltaTime) {
  // Update shooting animation (3 frames)
  if (isShooting) {
    shootTimer += deltaTime;

    // Calculate which shoot frame to show
    currentShootFrame = (int)(shootTimer / SHOOT_FRAME_TIME);
    if (currentShootFrame >= 3) {
      currentShootFrame = 2; // Stay on last frame briefly
    }

    // Check if shooting animation complete
    if (shootTimer >= SHOOT_TIME) {
      isShooting = false;
      shootTimer = 0.0f;
      currentShootFrame = 0;
      isReloading = true;
    }
  }

  // Update reload animation (9 frames)
  if (isReloading) {
    reloadTimer += deltaTime;

    // Calculate which reload frame to show (0-8)
    currentReloadFrame = (int)(reloadTimer / RELOAD_FRAME_TIME);
    if (currentReloadFrame >= 9) {
      currentReloadFrame = 8; // Stay on last frame
    }

    // Check if reload complete
    if (reloadTimer >= RELOAD_TIME) {
      isReloading = false;
      reloadTimer = 0.0f;
      currentReloadFrame = 0;
      printf("Reload complete!\n");
    }
  }
}

void drawGun(uint32_t *pixels, int WIDTH, int HEIGHT) {
  float scale = 0.7f;

  // Choose which sprite to draw
  Sprite *currentSprite = &gunIdle;

  if (isShooting) {
    // Show shooting animation
    currentSprite = &gunFire[currentShootFrame];
  } else if (isReloading) {
    // Show reload animation
    currentSprite = &gunReload[currentReloadFrame];
  }

  int scaledWidth = (int)(currentSprite->width * scale);
  int scaledHeight = (int)(currentSprite->height * scale);

  // Center horizontally, place at bottom of screen
  int gunX = WIDTH / 2 - scaledWidth / 2;
  int gunY = HEIGHT - scaledHeight - 5;

  drawSpriteScaled(currentSprite, gunX, gunY, scale, pixels, WIDTH, HEIGHT);
}

void startReload() {
  if (!isReloading && !isShooting) {
    isReloading = true;
    reloadTimer = 0.0f;
    currentReloadFrame = 0;
    printf("Reloading shotgun...\n");
  }
}

void startShoot() {
  if (!isReloading && !isShooting) {
    isShooting = true;
    shootTimer = 0.0f;
    currentShootFrame = 0;
    printf("BOOM! Shotgun blast!\n");
  }
}
