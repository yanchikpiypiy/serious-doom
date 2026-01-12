#include <SDL2/SDL.h>
#include <cmath>
#include <cstdint>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const int WIDTH = 1920;
const int HEIGHT = 1080;
const int MAP_SIZE = 8;
const float FOV = 3.14159f / 3.0f;
const float RAY_STEP = 0.005f;
const float MAX_DIST = 20.0f;

uint32_t pixels[WIDTH * HEIGHT];

int map[MAP_SIZE][MAP_SIZE] = {
    {1, 1, 1, 1, 1, 1, 1, 1}, {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 1, 1, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1}, {1, 1, 1, 1, 1, 1, 1, 1}};

float playerX = 2.0f;
float playerY = 2.0f;
float playerAngle = 0.0f;

struct Sprite {
  uint32_t *pixels;
  int width;
  int height;
};

Sprite gunIdle;

bool isReloading = false;
float reloadTimer = 0.0f;
const float RELOAD_TIME = 0.8f;

void drawPixel(int x, int y, uint32_t color) {
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
    pixels[y * WIDTH + x] = color;
}

void drawVerticalLine(int x, int y1, int y2, uint32_t color) {
  if (y1 < 0)
    y1 = 0;
  if (y2 >= HEIGHT)
    y2 = HEIGHT - 1;
  for (int y = y1; y <= y2; y++)
    drawPixel(x, y, color);
}

float castRay(float angle) {
  float rayX = playerX;
  float rayY = playerY;
  float dirX = cos(angle);
  float dirY = sin(angle);
  float distance = 0.0f;

  while (distance < MAX_DIST) {
    rayX += dirX * RAY_STEP;
    rayY += dirY * RAY_STEP;
    distance += RAY_STEP;

    int mx = (int)rayX;
    int my = (int)rayY;

    if (mx < 0 || my < 0 || mx >= MAP_SIZE || my >= MAP_SIZE)
      return distance;
    if (map[my][mx] == 1)
      return distance;
  }
  return MAX_DIST;
}

void render3DView() {
  // FIXED: Draw full width, start at x=0
  int viewX = 0;
  int viewWidth = WIDTH;

  for (int x = 0; x < viewWidth; x++) {
    float rayAngle = playerAngle - FOV / 2.0f + (x / (float)viewWidth) * FOV;
    float rawDist = castRay(rayAngle);
    float dist = rawDist * cos(rayAngle - playerAngle);
    if (dist < 0.001f)
      dist = 0.001f;

    int wallHeight = (int)((HEIGHT / dist) * 0.6f);
    int ceiling = (HEIGHT / 2) - wallHeight;
    int floor = (HEIGHT / 2) + wallHeight;

    int shade = (int)(180 / (1.0f + dist * dist * 0.1f));
    uint32_t wallColor = (0xFF << 24) | (shade << 16) | (shade << 8) | shade;

    for (int y = 0; y < ceiling; y++)
      drawPixel(viewX + x, y, 0xFF1a1a2e);

    drawVerticalLine(viewX + x, ceiling, floor, wallColor);

    for (int y = floor; y < HEIGHT; y++)
      drawPixel(viewX + x, y, 0xFF0f0f0f);
  }
}

void renderMinimap() {
  int tile = 10;
  int ox = 10, oy = 10;

  for (int y = 0; y < MAP_SIZE; y++) {
    for (int x = 0; x < MAP_SIZE; x++) {
      uint32_t color = map[y][x] ? 0xFF666666 : 0xFF111111;
      for (int dy = 0; dy < tile; dy++)
        for (int dx = 0; dx < tile; dx++)
          drawPixel(ox + x * tile + dx, oy + y * tile + dy, color);
    }
  }

  int px = ox + (int)(playerX * tile);
  int py = oy + (int)(playerY * tile);

  for (int dy = -2; dy <= 2; dy++)
    for (int dx = -2; dx <= 2; dx++)
      if (dx * dx + dy * dy <= 4)
        drawPixel(px + dx, py + dy, 0xFFFF0000);

  for (int i = 0; i < 8; i++)
    drawPixel(px + cos(playerAngle) * i, py + sin(playerAngle) * i, 0xFFFFFF00);
}

void updateGun(float deltaTime) {
  if (isReloading) {
    reloadTimer += deltaTime;
    if (reloadTimer >= RELOAD_TIME) {
      isReloading = false;
      reloadTimer = 0.0f;
    }
  }
}

// FIXED: Added scale parameter
void drawSpriteScaled(Sprite *sprite, int x, int y, float scale) {
  int scaledWidth = (int)(sprite->width * scale);
  int scaledHeight = (int)(sprite->height * scale);

  for (int sy = 0; sy < scaledHeight; sy++) {
    for (int sx = 0; sx < scaledWidth; sx++) {
      // Sample from original sprite
      int origX = (int)(sx / scale);
      int origY = (int)(sy / scale);

      if (origX >= sprite->width || origY >= sprite->height)
        continue;

      uint32_t pixel = sprite->pixels[origY * sprite->width + origX];
      uint8_t alpha = (pixel >> 24) & 0xFF;
      if (alpha < 128)
        continue;

      drawPixel(x + sx, y + sy, pixel);
    }
  }
}

void drawGun() {
  // FIXED: Scale gun down to 35% (makes it ~63×52 pixels)
  float scale = 3.5f;
  int scaledWidth = (int)(gunIdle.width * scale);
  int scaledHeight = (int)(gunIdle.height * scale);

  // FIXED: Position at actual bottom
  int gunX = WIDTH / 2 - scaledWidth / 2;
  int gunY = HEIGHT - scaledHeight - 5; // 5 pixel margin from bottom

  drawSpriteScaled(&gunIdle, gunX, gunY, scale);
}

bool loadSprite(Sprite *sprite, const char *filename) {
  int channels;
  unsigned char *data =
      stbi_load(filename, &sprite->width, &sprite->height, &channels, 4);
  if (!data) {
    printf("Failed to load: %s\n", filename);
    return false;
  }

  sprite->pixels = new uint32_t[sprite->width * sprite->height];

  // FIXED: Convert RGBA to ARGB
  for (int i = 0; i < sprite->width * sprite->height; i++) {
    unsigned char r = data[i * 4 + 0]; // Red
    unsigned char g = data[i * 4 + 1]; // Green
    unsigned char b = data[i * 4 + 2]; // Blue
    unsigned char a = data[i * 4 + 3]; // Alpha

    // Pack as ARGB (0xAARRGGBB)
    sprite->pixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
  }

  stbi_image_free(data);

  printf("Loaded %s: %dx%d\n", filename, sprite->width, sprite->height);
  return true;
}
int main() {
  SDL_Init(SDL_INIT_VIDEO);

  SDL_Window *win = SDL_CreateWindow("Doom with Gun", SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);

  SDL_Renderer *ren = SDL_CreateRenderer(win, -1, 0);
  SDL_Texture *tex =
      SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

  if (!loadSprite(&gunIdle, "sprites/SAKOA0.png")) {
    printf("ERROR: Could not load gun sprite!\n");
    return 1;
  }

  bool running = true;
  Uint32 lastTime = SDL_GetTicks();

  while (running) {
    Uint32 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;
    if (deltaTime > 0.05f)
      deltaTime = 0.05f;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        running = false;

      if (e.type == SDL_KEYDOWN) {
        float oldX = playerX;
        float oldY = playerY;
        float moveSpeed = 10.5f;
        float rotSpeed = 6.0f;

        if (e.key.keysym.sym == SDLK_UP) {
          playerX += cos(playerAngle) * moveSpeed * deltaTime;
          playerY += sin(playerAngle) * moveSpeed * deltaTime;
        }
        if (e.key.keysym.sym == SDLK_DOWN) {
          playerX -= cos(playerAngle) * moveSpeed * deltaTime;
          playerY -= sin(playerAngle) * moveSpeed * deltaTime;
        }
        if (e.key.keysym.sym == SDLK_LEFT)
          playerAngle -= rotSpeed * deltaTime;
        if (e.key.keysym.sym == SDLK_RIGHT)
          playerAngle += rotSpeed * deltaTime;

        if (e.key.keysym.sym == SDLK_r && !isReloading) {
          isReloading = true;
          reloadTimer = 0.0f;
        }

        if (map[(int)playerY][(int)playerX] == 1) {
          playerX = oldX;
          playerY = oldY;
        }
      }
    }

    if (playerAngle < 0)
      playerAngle += 2 * M_PI;
    if (playerAngle > 2 * M_PI)
      playerAngle -= 2 * M_PI;

    memset(pixels, 0, sizeof(pixels));

    // FIXED: Draw order - 3D first, then gun, then minimap (so minimap is on
    // top)
    render3DView();
    drawGun();
    renderMinimap(); // Draw last so it's on top
    updateGun(deltaTime);

    SDL_UpdateTexture(tex, nullptr, pixels, WIDTH * sizeof(uint32_t));
    SDL_RenderCopy(ren, tex, nullptr, nullptr);
    SDL_RenderPresent(ren);

    SDL_Delay(1);
  }

  delete[] gunIdle.pixels;
  SDL_Quit();
  return 0;
}
