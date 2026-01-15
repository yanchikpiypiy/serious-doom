#pragma once
#include <cstdint>

struct RayHit {
  float distance;
  float hitX;
  float hitY;
  bool vertical; // hit vertical wall or horizontal wall
};

extern const float FOV;

float *getZBuffer(); // Add this declaration
void render3DView(uint32_t *pixels, int WIDTH, int HEIGHT);
void renderMinimap(uint32_t *pixels, int WIDTH, int HEIGHT);

bool loadWallTexture(const char *filename);
void cleanupWallTexture();
