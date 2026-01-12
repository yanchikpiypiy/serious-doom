#pragma once
#include <cstdint>

struct Sprite {
  uint32_t *pixels;
  int width;
  int height;
};

bool loadSprite(Sprite *sprite, const char *filename);
void drawSpriteScaled(Sprite *sprite, int x, int y, float scale,
                      uint32_t *pixels, int WIDTH, int HEIGHT);
