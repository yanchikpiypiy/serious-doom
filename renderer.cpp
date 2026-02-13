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
  for (int i = 0; i < WIDTH; i++)
    zBuffer[i] = MAX_DIST;

  // Camera direction
  float dirX = cos(playerAngle);
  float dirY = sin(playerAngle);

  // Camera plane
  float planeX = -dirY * tan(FOV / 2.0f);
  float planeY = dirX * tan(FOV / 2.0f);

  for (int x = 0; x < WIDTH; x++) {
    // camera space X
    float cameraX = 2.0f * x / (float)WIDTH - 1.0f;

    // ray direction
    float rayDirX = dirX + planeX * cameraX;
    float rayDirY = dirY + planeY * cameraX;

    float rayAngle = atan2(rayDirY, rayDirX);

    // cast ray
    RayHit hit = castRayDDA(rayAngle);

    float dist = hit.distance;
    if (dist < 0.0001f)
      dist = 0.0001f;

    zBuffer[x] = dist;

    // wall height (correct perspective)
    int wallHeight = (int)(HEIGHT / dist);

    // wall draw bounds
    int drawStart = -wallHeight / 2 + HEIGHT / 2;
    int drawEnd = wallHeight / 2 + HEIGHT / 2;

    if (drawStart < 0)
      drawStart = 0;
    if (drawEnd >= HEIGHT)
      drawEnd = HEIGHT - 1;

    //
    // CEILING CASTING (STATIC)
    //
    for (int y = 0; y < drawStart; y++) {
      float p = (HEIGHT / 2.0f) - y;
      if (p == 0)
        p = 0.0001f;

      float rowDist = (HEIGHT / 2.0f) / p;

      float worldX = playerX + rowDist * rayDirX;
      float worldY = playerY + rowDist * rayDirY;

      float fracX = (worldX * 0.6f) - floorf(worldX * 0.6f);
      float fracY = (worldY * 0.6f) - floorf(worldY * 0.6f);

      int texX = (int)(fracX * ceilingTexture.width);
      int texY = (int)(fracY * ceilingTexture.height);

      if (texX < 0)
        texX = 0;
      if (texY < 0)
        texY = 0;
      if (texX >= ceilingTexture.width)
        texX = ceilingTexture.width - 1;
      if (texY >= ceilingTexture.height)
        texY = ceilingTexture.height - 1;

      uint32_t texColor =
          ceilingTexture.pixels[texY * ceilingTexture.width + texX];

      float fog = 1.0f / (1.0f + rowDist * rowDist * 0.1f);

      uint8_t r = ((texColor >> 16) & 0xFF) * fog;
      uint8_t g = ((texColor >> 8) & 0xFF) * fog;
      uint8_t b = (texColor & 0xFF) * fog;

      uint32_t shaded = (0xFF << 24) | (r << 16) | (g << 8) | b;

      drawPixel(pixels, WIDTH, HEIGHT, x, y, shaded);
    }

    //
    // WALL TEXTURE X
    //
    float wallX;

    if (hit.vertical)
      wallX = playerY + dist * rayDirY;
    else
      wallX = playerX + dist * rayDirX;

    wallX -= floorf(wallX);

    int wallTexX = (int)(wallX * wallTexture.width);

    if (wallTexX < 0)
      wallTexX = 0;
    if (wallTexX >= wallTexture.width)
      wallTexX = wallTexture.width - 1;

    //
    // DRAW WALL (CORRECT TEXTURE PROJECTION)
    //
    for (int y = drawStart; y <= drawEnd; y++) {
      int d = y * 256 - HEIGHT * 128 + wallHeight * 128;

      int texY = ((d * wallTexture.height) / wallHeight) / 256;

      if (texY < 0)
        texY = 0;
      if (texY >= wallTexture.height)
        texY = wallTexture.height - 1;

      uint32_t texColor =
          wallTexture.pixels[texY * wallTexture.width + wallTexX];

      // distance shading
      float shadeFactor = 1.0f / (1.0f + dist * dist * 0.08f);

      uint8_t r = ((texColor >> 16) & 0xFF) * shadeFactor;
      uint8_t g = ((texColor >> 8) & 0xFF) * shadeFactor;
      uint8_t b = (texColor & 0xFF) * shadeFactor;

      uint32_t shaded = (0xFF << 24) | (r << 16) | (g << 8) | b;

      drawPixel(pixels, WIDTH, HEIGHT, x, y, shaded);
    }

    //
    // FLOOR
    //
    for (int y = drawEnd + 1; y < HEIGHT; y++) {
      drawPixel(pixels, WIDTH, HEIGHT, x, y, 0xFF0f0f0f);
    }
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
