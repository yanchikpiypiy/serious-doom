#include "enemy.h"
#include "gun.h"
#include "map.h"
#include "player.h"
#include "projectile.h" // ADD THIS
#include "renderer.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstring>

const int WIDTH = 620;
const int HEIGHT = 400;
uint32_t pixels[WIDTH * HEIGHT];
float fpsTimer = 0.0f;
int fpsFrames = 0;
int currentFPS = 0;

int main() {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *win =
      SDL_CreateWindow("Doom with Gun", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, WIDTH * 2, HEIGHT * 2, 0);
  SDL_Renderer *ren = SDL_CreateRenderer(win, -1, 0);
  SDL_Texture *tex =
      SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

  if (!loadGunSprites()) {
    printf("ERROR: Could not load gun sprites!\n");
    return 1;
  }

  if (!loadWallTexture("sprites/wall.png")) {
    printf("WARNING: Could not load wall texture! Using solid color.\n");
  }

  if (!loadCeilingTexture("sprites/Ceiling.png")) {
    printf("Error: Could not load a ceiuling sprite");
  }

  if (!loadEnemySprites()) {
    printf("Error: Could not load enemies\n");
    return 1;
  }

  if (!loadProjectileSprites()) { // ADD THIS
    printf("Error: Could not load projectile sprites\n");
    return 1;
  }

  initEnemies();
  initProjectiles(); // ADD THIS

  bool running = true;
  Uint32 lastTime = SDL_GetTicks();

  while (running) {
    Uint32 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    if (deltaTime > 0.05f)
      deltaTime = 0.05f;

    fpsTimer += deltaTime;
    fpsFrames++;
    if (fpsTimer >= 1.0f) {
      currentFPS = fpsFrames;
      printf("FPS: %d\n", currentFPS);
      fpsFrames = 0;
      fpsTimer = 0.0f;
    }

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        running = false;

      if (e.type == SDL_KEYDOWN) {
        handlePlayerInput(e.key.keysym.sym, true);
      }

      if (e.type == SDL_KEYUP) {
        handlePlayerInput(e.key.keysym.sym, false);
      }
    }

    updatePlayer(deltaTime);
    updateGun(deltaTime);
    updateEnemies(deltaTime);
    updateProjectiles(deltaTime); // ADD THIS

    // Check if player got hit  // ADD THIS
    if (checkProjectilePlayerHit(playerX, playerY, 0.3f)) {
      printf("Player hit by projectile!\n");
      // TODO: Reduce player health here
    }

    memset(pixels, 0, sizeof(pixels));
    render3DView(pixels, WIDTH, HEIGHT);
    renderEnemies(pixels, WIDTH, HEIGHT, getZBuffer());
    renderProjectiles(pixels, WIDTH, HEIGHT,
                      getZBuffer()); // ADD THIS (before enemies)

    drawGun(pixels, WIDTH, HEIGHT);
    renderMinimap(pixels, WIDTH, HEIGHT);

    SDL_UpdateTexture(tex, nullptr, pixels, WIDTH * sizeof(uint32_t));
    SDL_RenderCopy(ren, tex, nullptr, nullptr);
    SDL_RenderPresent(ren);

    SDL_Delay(1);
  }

  cleanupGunSprites();
  cleanupWallTexture();
  cleanupEnemySprites();
  cleanupProjectileSprites(); // ADD THIS

  SDL_Quit();
  return 0;
}
