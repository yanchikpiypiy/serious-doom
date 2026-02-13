#pragma once
#include <cstdint>
struct Sprite {
  uint32_t *pixels;
  int width;
  int height;
};
bool loadSprite(Sprite *sprite, const char *filename);
void drawSpriteScaled(Sprite *sprite, int x, int y, float scale, bool mirror,
                      uint32_t *pixels, int WIDTH, int HEIGHT);
void drawSpriteScaledWithDepth(Sprite *sprite, int x, int y, float scale,
                               bool mirror, uint32_t *pixels, int WIDTH,
                               int HEIGHT, float *zBuffer, float depth);
void drawSpriteScaledXY(Sprite *sprite, int x, int y, float scaleX,
                        float scaleY, bool mirror, uint32_t *pixels, int WIDTH,
                        int HEIGHT);
void drawSpriteScaledWithDepthXY(Sprite *sprite, int x, int y, float scaleX,
                                 float scaleY, bool mirror, uint32_t *pixels,
                                 int WIDTH, int HEIGHT, float *zBuffer,
                                 float depth);
