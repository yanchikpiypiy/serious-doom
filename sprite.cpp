#include "sprite.h"
#include <cstdio>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool loadSprite(Sprite *sprite, const char *filename) {
  int channels;
  unsigned char *data =
      stbi_load(filename, &sprite->width, &sprite->height, &channels, 4);
  if (!data) {
    printf("Failed to load: %s\n", filename);
    return false;
  }

  sprite->pixels = new uint32_t[sprite->width * sprite->height];

  // Convert RGBA to ARGB
  for (int i = 0; i < sprite->width * sprite->height; i++) {
    unsigned char r = data[i * 4 + 0];
    unsigned char g = data[i * 4 + 1];
    unsigned char b = data[i * 4 + 2];
    unsigned char a = data[i * 4 + 3];

    sprite->pixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
  }

  stbi_image_free(data);

  printf("Loaded %s: %dx%d\n", filename, sprite->width, sprite->height);
  return true;
}

// Improved sprite scaling - replaces your drawSpriteScaled function
void drawSpriteScaled(Sprite *sprite, int x, int y, float scale, bool mirror,
                      uint32_t *pixels, int WIDTH, int HEIGHT) {
  if (!sprite || !sprite->pixels)
    return;

  int scaledWidth = (int)(sprite->width * scale + 0.5f);
  int scaledHeight = (int)(sprite->height * scale + 0.5f);

  // MODE 1: Sharp pixel art (best for 320x200 Doom-style)
  // Uses nearest-neighbor but with better rounding
  for (int sy = 0; sy < scaledHeight; sy++) {
    for (int sx = 0; sx < scaledWidth; sx++) {
      // Better sampling: add 0.5 for proper rounding to nearest pixel
      int origX = (int)((sx + 0.5f) / scale);
      int origY = (int)((sy + 0.5f) / scale);

      // MIRROR HORIZONTALLY if flag is set
      if (mirror) {
        origX = sprite->width - 1 - origX;
      }

      // Clamp to valid range
      if (origX >= sprite->width)
        origX = sprite->width - 1;
      if (origY >= sprite->height)
        origY = sprite->height - 1;
      if (origX < 0 || origY < 0)
        continue;

      uint32_t pixel = sprite->pixels[origY * sprite->width + origX];
      uint8_t alpha = (pixel >> 24) & 0xFF;
      if (alpha < 128)
        continue;

      int px = x + sx;
      int py = y + sy;
      if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT)
        pixels[py * WIDTH + px] = pixel;
    }
  }
}

void drawSpriteScaledWithDepth(Sprite *sprite, int x, int y, float scale,
                               bool mirror, uint32_t *pixels, int WIDTH,
                               int HEIGHT, float *zBuffer, float depth) {
  if (!sprite || !sprite->pixels)
    return;

  int scaledWidth = (int)(sprite->width * scale + 0.5f);
  int scaledHeight = (int)(sprite->height * scale + 0.5f);

  for (int sy = 0; sy < scaledHeight; sy++) {
    for (int sx = 0; sx < scaledWidth; sx++) {
      int origX = (int)((sx + 0.5f) / scale);
      int origY = (int)((sy + 0.5f) / scale);

      if (mirror) {
        origX = sprite->width - 1 - origX;
      }

      if (origX >= sprite->width)
        origX = sprite->width - 1;
      if (origY >= sprite->height)
        origY = sprite->height - 1;
      if (origX < 0 || origY < 0)
        continue;

      uint32_t pixel = sprite->pixels[origY * sprite->width + origX];
      uint8_t alpha = (pixel >> 24) & 0xFF;
      if (alpha < 128)
        continue;

      int px = x + sx;
      int py = y + sy;

      // Z-buffer check: only draw if sprite is closer than wall
      if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
        if (depth < zBuffer[px]) {
          pixels[py * WIDTH + px] = pixel;
        }
      }
    }
  }
}
