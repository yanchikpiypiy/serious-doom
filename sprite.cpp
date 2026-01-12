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

void drawSpriteScaled(Sprite *sprite, int x, int y, float scale,
                      uint32_t *pixels, int WIDTH, int HEIGHT) {
  int scaledWidth = (int)(sprite->width * scale);
  int scaledHeight = (int)(sprite->height * scale);

  for (int sy = 0; sy < scaledHeight; sy++) {
    for (int sx = 0; sx < scaledWidth; sx++) {
      int origX = (int)(sx / scale);
      int origY = (int)(sy / scale);

      if (origX >= sprite->width || origY >= sprite->height)
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
