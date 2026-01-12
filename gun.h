#pragma once
#include <cstdint>

bool loadGunSprites();
void cleanupGunSprites();
void updateGun(float deltaTime);
void drawGun(uint32_t *pixels, int WIDTH, int HEIGHT);
void startReload();
