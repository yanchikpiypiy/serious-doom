#include "renderer.h"
#include "map.h"
#include "player.h"
#include "sprite.h"
#include <cmath>

const float FOV = 3.14159f / 3.0f;
const float RAY_STEP = 0.005f;
const float MAX_DIST = 20.0f;

// Wall texture - stored here in renderer.cpp
static Sprite wallTexture;

bool loadWallTexture(const char *filename) {
  return loadSprite(&wallTexture, filename);
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

// New detailed raycasting that returns hit information
static RayHit castRayDetailed(float angle) {
  RayHit hit;
  float rayX = playerX;
  float rayY = playerY;
  float dirX = cos(angle);
  float dirY = sin(angle);
  float distance = 0.0f;

  while (distance < MAX_DIST) {
    float prevX = rayX;
    float prevY = rayY;

    rayX += dirX * RAY_STEP;
    rayY += dirY * RAY_STEP;
    distance += RAY_STEP;

    int mx = (int)rayX;
    int my = (int)rayY;

    if (mx < 0 || my < 0 || mx >= MAP_SIZE || my >= MAP_SIZE) {
      hit.distance = distance;
      hit.hitX = rayX;
      hit.hitY = rayY;
      hit.vertical = false;
      return hit;
    }

    if (getMapTile(my, mx) == 1) {
      hit.distance = distance;
      hit.hitX = rayX;
      hit.hitY = rayY;

      // Determine if we hit a vertical or horizontal wall
      // by checking which grid coordinate changed
      int prevMx = (int)prevX;
      int prevMy = (int)prevY;
      hit.vertical = (mx != prevMx);

      return hit;
    }
  }

  hit.distance = MAX_DIST;
  hit.hitX = rayX;
  hit.hitY = rayY;
  hit.vertical = false;
  return hit;
}

void render3DView(uint32_t *pixels, int WIDTH, int HEIGHT) {
  int viewX = 0;
  int viewWidth = WIDTH;

  for (int x = 0; x < viewWidth; x++) {
    float rayAngle = playerAngle - FOV / 2.0f + (x / (float)viewWidth) * FOV;
    RayHit hit = castRayDetailed(rayAngle);

    // Fish-eye correction
    float dist = hit.distance * cos(rayAngle - playerAngle);
    if (dist < 0.001f)
      dist = 0.001f;

    int wallHeight = (int)((HEIGHT / dist) * 0.6f);
    int ceiling = (HEIGHT / 2) - wallHeight;
    int floor = (HEIGHT / 2) + wallHeight;

    // Calculate texture X coordinate
    float textureX;
    if (hit.vertical) {
      // Vertical wall - X crossed a grid line, use Y coordinate
      textureX = hit.hitY - floorf(hit.hitY);
    } else {
      // Horizontal wall - Y crossed a grid line, use X coordinate
      textureX = hit.hitX - floorf(hit.hitX);
    }

    // Convert to texture pixel coordinate
    int texX = (int)(textureX * wallTexture.width) % wallTexture.width;

    // Draw ceiling
    for (int y = 0; y < ceiling; y++)
      drawPixel(pixels, WIDTH, HEIGHT, viewX + x, y, 0xFF1a1a2e);

    // Draw textured wall column
    for (int y = ceiling; y <= floor && y < HEIGHT; y++) {
      // Calculate where we are on the wall (0.0 to 1.0)
      float wallProgress = (float)(y - ceiling) / (float)(floor - ceiling);

      // Map to texture Y coordinate
      int texY = (int)(wallProgress * wallTexture.height);
      if (texY >= wallTexture.height)
        texY = wallTexture.height - 1;

      // Get pixel from texture
      uint32_t texColor = wallTexture.pixels[texY * wallTexture.width + texX];

      // Apply distance-based shading
      int shade = (int)(255 / (1.0f + dist * dist * 0.08f));
      uint8_t r = ((texColor >> 16) & 0xFF) * shade / 255;
      uint8_t g = ((texColor >> 8) & 0xFF) * shade / 255;
      uint8_t b = (texColor & 0xFF) * shade / 255;

      uint32_t shadedColor = (0xFF << 24) | (r << 16) | (g << 8) | b;
      drawPixel(pixels, WIDTH, HEIGHT, viewX + x, y, shadedColor);
    }

    // Draw floor
    for (int y = floor + 1; y < HEIGHT; y++)
      drawPixel(pixels, WIDTH, HEIGHT, viewX + x, y, 0xFF0f0f0f);
  }
}

void renderMinimap(uint32_t *pixels, int WIDTH, int HEIGHT) {
  int tile = WIDTH / 80;
  if (tile < 3)
    tile = 3;
  if (tile > 12)
    tile = 12;

  int ox = 10;
  int oy = 10;

  // Draw only walls
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

  // Player dot
  int px = ox + (int)(playerX * tile);
  int py = oy + (int)(playerY * tile);

  for (int dy = -2; dy <= 2; dy++)
    for (int dx = -2; dx <= 2; dx++)
      if (dx * dx + dy * dy <= 4)
        drawPixel(pixels, WIDTH, HEIGHT, px + dx, py + dy, 0xFFFF0000);

  // Direction line
  for (int i = 0; i < tile; i++) {
    drawPixel(pixels, WIDTH, HEIGHT, px + (int)(cos(playerAngle) * i),
              py + (int)(sin(playerAngle) * i), 0xFFFFFF00);
  }
}
