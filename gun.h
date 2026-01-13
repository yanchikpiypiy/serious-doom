#pragma once
#include <cstdint>

// Loading / cleanup
bool loadGunSprites();
void cleanupGunSprites();

// Update & draw
void updateGun(float deltaTime);
void drawGun(uint32_t *pixels, int WIDTH, int HEIGHT);

// Actions
void startReload();
void startShoot();
