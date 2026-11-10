#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cmath>
#include "world/world.h"

struct Color { Uint8 r, g, b; };
static const Color TILE_COLORS[] = {
    {40, 90, 200},   // WATER
    {220, 205, 140}, // SAND
    {90, 170, 70},   // GRASS
    {30, 100, 40},   // FOREST
    {120, 120, 125}, // ROCK
};

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("My Game", 1280, 720, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!window || !renderer) {
        SDL_Log("Window/renderer failed: %s", SDL_GetError());
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);

    World world;
    float camX = 0, camY = 0;          // camera in world pixels
    const int TILE_PX = 16;
    float zoom = 2.0f;

    const double TICK_DT = 1.0 / 20.0; // simulation: 20 ticks/sec
    double accumulator = 0.0;
    Uint64 last = SDL_GetTicksNS();

    bool running = true;
    while (running) {
        Uint64 now = SDL_GetTicksNS();
        double frameDt = (now - last) / 1e9;
        last = now;
        if (frameDt > 0.25) frameDt = 0.25; // avoid spiral of death

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) running = false;
            if (e.type == SDL_EVENT_MOUSE_WHEEL)
                zoom = SDL_clamp(zoom * (e.wheel.y > 0 ? 1.1f : 0.9f), 0.5f, 6.0f);
        }

        // Camera movement (per-frame, not part of the sim)
        const bool* keys = SDL_GetKeyboardState(nullptr);
        float speed = 600.0f * (float)frameDt / zoom;
        if (keys[SDL_SCANCODE_W]) camY -= speed;
        if (keys[SDL_SCANCODE_S]) camY += speed;
        if (keys[SDL_SCANCODE_A]) camX -= speed;
        if (keys[SDL_SCANCODE_D]) camX += speed;

        // Fixed-timestep simulation
        accumulator += frameDt;
        while (accumulator >= TICK_DT) {
            world.tick(TICK_DT);
            accumulator -= TICK_DT;
        }

        // Render
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        float tileSize = TILE_PX * zoom;
        int minTx = (int)std::floor(camX / TILE_PX);
        int minTy = (int)std::floor(camY / TILE_PX);
        int maxTx = minTx + (int)(w / tileSize) + 2;
        int maxTy = minTy + (int)(h / tileSize) + 2;

        for (int ty = minTy; ty <= maxTy; ty++) {
            for (int tx = minTx; tx <= maxTx; tx++) {
                Color c = TILE_COLORS[world.getTile(tx, ty)];
                SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
                SDL_FRect r{ (tx * TILE_PX - camX) * zoom,
                             (ty * TILE_PX - camY) * zoom,
                             tileSize, tileSize };
                SDL_RenderFillRect(renderer, &r);
            }
        }
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}