#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include "world/world.h"
#include "world/world_generator.h"
#include "world/tile.h"

// ---- Atlas layout: must match how the atlas image was made -----------------
// test_atlas.bmp         -> ATLAS_PAD = 0  (64x64 cells, stride 32)
// test_atlas_padded.bmp  -> ATLAS_PAD = 1  (60x60 cells, stride 34)
constexpr int         TILE_PX = 32;
constexpr int         ATLAS_PX = 2048;
constexpr int         ATLAS_PAD = 0;
constexpr const char* ATLAS_FILE = "assets/textures/test_atlas.bmp";
constexpr int         ATLAS_STRIDE = TILE_PX + 2 * ATLAS_PAD;
constexpr int         ATLAS_COLS = ATLAS_PX / ATLAS_STRIDE;
constexpr int         ATLAS_CELLS = ATLAS_COLS * ATLAS_COLS;

// Source rectangle of one atlas cell.
static SDL_FRect atlasRect(uint16_t cell) {
    return SDL_FRect{
        float((cell % ATLAS_COLS) * ATLAS_STRIDE + ATLAS_PAD),
        float((cell / ATLAS_COLS) * ATLAS_STRIDE + ATLAS_PAD),
        float(TILE_PX), float(TILE_PX)
    };
}

// Stable per-tile pseudo-random number: picks a visual variant with no storage.
static uint32_t tileHash(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static SDL_Texture* loadAtlas(SDL_Renderer* renderer, const std::string& path) {
    SDL_Surface* surface = SDL_LoadBMP(path.c_str());
    if (!surface) {
        SDL_Log("Could not load atlas '%s': %s", path.c_str(), SDL_GetError());
        return nullptr;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!tex) {
        SDL_Log("Could not create atlas texture: %s", SDL_GetError());
        return nullptr;
    }
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);   // crisp pixels when zooming

    float tw = 0, th = 0;
    SDL_GetTextureSize(tex, &tw, &th);
    if ((int)tw != ATLAS_PX || (int)th != ATLAS_PX)
        SDL_Log("Warning: atlas is %dx%d but ATLAS_PX is %d", (int)tw, (int)th, ATLAS_PX);
    return tex;
}

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

    // ---- Assets ---------------------------------------------------------
    const char* basePath = SDL_GetBasePath();
    const std::string base = basePath ? basePath : "";

    TileRegistry tiles;
    try {
        tiles.loadFromFile(base + "assets/data/tiles.json", ATLAS_CELLS);
    }
    catch (const std::exception& e) {
        SDL_Log("Failed to load tiles: %s", e.what());
        return 1;
    }

    SDL_Texture* atlas = loadAtlas(renderer, base + ATLAS_FILE);
    if (!atlas) return 1;

    // ---- World ----------------------------------------------------------
    std::unique_ptr<WorldGenerator> generator;
    try {
        generator = std::make_unique<WorldGenerator>(tiles);   // after tiles.loadFromFile(...)
    }
    catch (const std::exception& e) {
        SDL_Log("World generator setup failed: %s", e.what());
        return 1;
    }

    World world;

    float camX = 0, camY = 0;          // camera in world pixels
    float zoom = 2.0f;

    const double TICK_DT = 1.0 / 20.0; // simulation: 20 ticks/sec
    double accumulator = 0.0;
    uint64_t tickCount = 0;            // drives tile animation
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
            ++tickCount;
            accumulator -= TICK_DT;
        }

        // Which tiles are on screen? (must come before chunk generation)
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        float tileSize = TILE_PX * zoom;
        int minTx = (int)std::floor(camX / TILE_PX);
        int minTy = (int)std::floor(camY / TILE_PX);
        int maxTx = minTx + (int)(w / tileSize) + 2;
        int maxTy = minTy + (int)(h / tileSize) + 2;

        // Make sure every visible chunk exists
        for (int cy = tileToChunk(minTy); cy <= tileToChunk(maxTy); ++cy)
            for (int cx = tileToChunk(minTx); cx <= tileToChunk(maxTx); ++cx) {
                ChunkCoord c{ cx, cy };
                if (!world.hasChunk(c)) {
                    auto chunk = std::make_unique<Chunk>();
                    generator->generate(*chunk, cx, cy);
                    world.insertChunk(c, std::move(chunk));
                }
            }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (int ty = minTy; ty <= maxTy; ty++) {
            for (int tx = minTx; tx <= maxTx; tx++) {
                // Snap both edges to whole pixels so neighboring tiles never leave gaps.
                float x0 = std::floor((tx * TILE_PX - camX) * zoom);
                float y0 = std::floor((ty * TILE_PX - camY) * zoom);
                float x1 = std::floor(((tx + 1) * TILE_PX - camX) * zoom);
                float y1 = std::floor(((ty + 1) * TILE_PX - camY) * zoom);
                SDL_FRect dst{ x0, y0, x1 - x0, y1 - y0 };

                TileId id = world.getTile(tx, ty);
                if (!tiles.isValid(id)) {            // unknown ID: magenta = "fix me"
                    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
                    SDL_RenderFillRect(renderer, &dst);
                    continue;
                }

                const TileDef& def = tiles.get(id);
                uint32_t variant = tileHash(tx, ty);
                uint32_t frame = uint32_t(tickCount / def.frameTimeTicks);
                SDL_FRect src = atlasRect(atlasCell(def, variant, frame));
                SDL_RenderTexture(renderer, atlas, &src, &dst);
            }
        }
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(atlas);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}