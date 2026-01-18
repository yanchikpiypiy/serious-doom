#include "renderer.h"
#include "map.h"
#include "player.h"
#include "sprite.h"
#include <algorithm>
#include <cmath>

const float FOV = M_PI / 3.0f;
const float MAX_DIST = 20.0f;

static Sprite wallTexture;
static Sprite ceilingTexture;
static float zBuffer[1920]; // Max screen width

bool loadWallTexture(const char *filename) {
  return loadSprite(&wallTexture, filename);
}

bool loadCeilingTexture(const char *filename) {
  return loadSprite(&ceilingTexture, filename);
}

void cleanupWallTexture() {
  if (wallTexture.pixels) {
    delete[] wallTexture.pixels;
    wallTexture.pixels = nullptr;
  }
}

static void drawPixel(uint32_t *pixels, int WIDTH, int HEIGHT, int x, int y,
                      uint32_t color) {
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
    pixels[y * WIDTH + x] = color;
}

// DDA Raycasting - much faster and more accurate
static RayHit castRayDDA(float angle) {
  RayHit hit;

  float dirX = cos(angle);
  float dirY = sin(angle);

  // Starting position
  int mapX = (int)playerX;
  int mapY = (int)playerY;

  // Length of ray from one side to next in map
  float deltaDistX = fabs(1.0f / dirX);
  float deltaDistY = fabs(1.0f / dirY);

  // Direction to step in (+1 or -1)
  int stepX = dirX > 0 ? 1 : -1;
  int stepY = dirY > 0 ? 1 : -1;

  // Initial side distances
  float sideDistX = (dirX > 0) ? (mapX + 1.0f - playerX) * deltaDistX
                               : (playerX - mapX) * deltaDistX;

  float sideDistY = (dirY > 0) ? (mapY + 1.0f - playerY) * deltaDistY
                               : (playerY - mapY) * deltaDistY;

  // DDA algorithm
  bool hitWall = false;
  bool vertical = false;

  while (!hitWall && mapX >= 0 && mapX < MAP_SIZE && mapY >= 0 &&
         mapY < MAP_SIZE) {
    // Jump to next map square
    if (sideDistX < sideDistY) {
      sideDistX += deltaDistX;
      mapX += stepX;
      vertical = true;
    } else {
      sideDistY += deltaDistY;
      mapY += stepY;
      vertical = false;
    }

    // Check for wall
    if (mapX >= 0 && mapX < MAP_SIZE && mapY >= 0 && mapY < MAP_SIZE) {
      if (getMapTile(mapY, mapX) == 1) {
        hitWall = true;
      }
    } else {
      break; // Out of bounds
    }
  }

  // Calculate distance
  float distance;
  if (vertical) {
    distance = (mapX - playerX + (1 - stepX) / 2) / dirX;
  } else {
    distance = (mapY - playerY + (1 - stepY) / 2) / dirY;
  }

  hit.distance = distance;
  hit.hitX = playerX + dirX * distance;
  hit.hitY = playerY + dirY * distance;
  hit.vertical = vertical;

  return hit;
}

void render3DView(uint32_t *pixels, int WIDTH, int HEIGHT) {
  // Clear z-buffer
  for (int i = 0; i < WIDTH; i++) {
    zBuffer[i] = MAX_DIST;
  }

  for (int x = 0; x < WIDTH; x++) {
    float rayAngle = playerAngle - FOV / 2.0f + (x / (float)WIDTH) * FOV;
    RayHit hit = castRayDDA(rayAngle);

    // Fish-eye correction
    float dist = hit.distance * cos(rayAngle - playerAngle);
    if (dist < 0.001f)
      dist = 0.001f;

    // Store in z-buffer for sprite rendering
    zBuffer[x] = dist;

    int wallHeight = (int)((HEIGHT / dist) * 0.8f);
    int ceiling = (HEIGHT / 2) - wallHeight;
    int floor = (HEIGHT / 2) + wallHeight;

    // Calculate texture coordinate
    float textureX;
    if (hit.vertical) {
      textureX = hit.hitY - floorf(hit.hitY);
    } else {
      textureX = hit.hitX - floorf(hit.hitX);
    }

    int texX = (int)(textureX * wallTexture.width) % wallTexture.width;

    // Draw ceiling
    for (int y = 0; y < ceiling; y++)
      drawPixel(pixels, WIDTH, HEIGHT, x, y, 0xFF1a1a2e);

    // Draw textured wall
    for (int y = ceiling; y <= floor && y < HEIGHT; y++) {
      float wallProgress = (float)(y - ceiling) / (float)(floor - ceiling);
      int texY = (int)(wallProgress * wallTexture.height);
      if (texY >= wallTexture.height)
        texY = wallTexture.height - 1;

      uint32_t texColor = wallTexture.pixels[texY * wallTexture.width + texX];

      // Distance shading
      int shade = (int)(255 / (1.0f + dist * dist * 0.08f));
      uint8_t r = ((texColor >> 16) & 0xFF) * shade / 255;
      uint8_t g = ((texColor >> 8) & 0xFF) * shade / 255;
      uint8_t b = (texColor & 0xFF) * shade / 255;

      uint32_t shadedColor = (0xFF << 24) | (r << 16) | (g << 8) | b;
      drawPixel(pixels, WIDTH, HEIGHT, x, y, shadedColor);
    }

    // Draw floor
    for (int y = floor + 1; y < HEIGHT; y++)
      drawPixel(pixels, WIDTH, HEIGHT, x, y, 0xFF0f0f0f);
  }
}

float *getZBuffer() { return zBuffer; }

void renderMinimap(uint32_t *pixels, int WIDTH, int HEIGHT) {
  int tile = WIDTH / 80;
  if (tile < 3)
    tile = 3;
  if (tile > 12)
    tile = 12;

  int ox = 10;
  int oy = 10;

  for (int y = 0; y < MAP_SIZE; y++) {
    for (int x = 0; x < MAP_SIZE; x++) {
      if (!getMapTile(y, x))
        continue;

      for (int dy = 0; dy < tile; dy++)
        for (int dx = 0; dx < tile; dx++)
          drawPixel(pixels, WIDTH, HEIGHT, ox + x * tile + dx,
                    oy + y * tile + dy, 0xFF666666);
    }
  }

  int px = ox + (int)(playerX * tile);
  int py = oy + (int)(playerY * tile);

  for (int dy = -2; dy <= 2; dy++)
    for (int dx = -2; dx <= 2; dx++)
      if (dx * dx + dy * dy <= 4)
        drawPixel(pixels, WIDTH, HEIGHT, px + dx, py + dy, 0xFFFF0000);

  for (int i = 0; i < tile; i++) {
    drawPixel(pixels, WIDTH, HEIGHT, px + (int)(cos(playerAngle) * i),
              py + (int)(sin(playerAngle) * i), 0xFFFFFF00);
  }
}
