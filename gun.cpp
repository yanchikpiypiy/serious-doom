#include "gun.h"
#include "sprite.h"

Sprite gunIdle;

bool isReloading = false;
float reloadTimer = 0.0f;
const float RELOAD_TIME = 0.8f;

bool loadGunSprites() {
  if (!loadSprite(&gunIdle, "sprites/SAKOA0.png")) {
    return false;
  }
  return true;
}

void cleanupGunSprites() { delete[] gunIdle.pixels; }

void updateGun(float deltaTime) {
  if (isReloading) {
    reloadTimer += deltaTime;
    if (reloadTimer >= RELOAD_TIME) {
      isReloading = false;
      reloadTimer = 0.0f;
    }
  }
}

void drawGun(uint32_t *pixels, int WIDTH, int HEIGHT) {
  float scale = 0.7f;
  int scaledWidth = (int)(gunIdle.width * scale);
  int scaledHeight = (int)(gunIdle.height * scale);

  int gunX = WIDTH / 2 - scaledWidth / 2;
  int gunY = HEIGHT - scaledHeight - 5;

  drawSpriteScaled(&gunIdle, gunX, gunY, scale, pixels, WIDTH, HEIGHT);
}

void startReload() {
  if (!isReloading) {
    isReloading = true;
    reloadTimer = 0.0f;
  }
}
