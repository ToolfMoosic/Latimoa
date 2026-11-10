#pragma once
#include <cstdint>
#include <array>
#include <unordered_map>
#include <cmath>

constexpr int CHUNK_SIZE = 32;

enum Tile : uint8_t { WATER, SAND, GRASS, FOREST, ROCK };

struct Chunk {
    std::array<uint8_t, CHUNK_SIZE* CHUNK_SIZE> tiles{};
    bool dirty = true;   // for re-render / sim scheduling later
};

struct ChunkCoord {
    int x, y;
    bool operator==(const ChunkCoord& o) const { return x == o.x && y == o.y; }
};
struct ChunkHash {
    size_t operator()(const ChunkCoord& c) const {
        return (size_t)(uint32_t)c.x * 73856093u ^ (size_t)(uint32_t)c.y * 19349663u;
    }
};

// Placeholder noise. Swap for FastNoiseLite later.
inline float hash2(int x, int y) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (h ^ (h >> 16)) / 4294967295.0f;
}
inline float smoothNoise(float x, float y) {
    int xi = (int)std::floor(x), yi = (int)std::floor(y);
    float fx = x - xi, fy = y - yi;
    fx = fx * fx * (3 - 2 * fx);
    fy = fy * fy * (3 - 2 * fy);
    float a = hash2(xi, yi), b = hash2(xi + 1, yi);
    float c = hash2(xi, yi + 1), d = hash2(xi + 1, yi + 1);
    return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy;
}

class World {
public:
    Chunk& getChunk(int cx, int cy) {
        ChunkCoord key{ cx, cy };
        auto it = chunks_.find(key);
        if (it != chunks_.end()) return it->second;
        Chunk& c = chunks_[key];
        generate(c, cx, cy);
        return c;
    }

    uint8_t getTile(int tx, int ty) {
        int cx = floorDiv(tx), cy = floorDiv(ty);
        int lx = tx - cx * CHUNK_SIZE, ly = ty - cy * CHUNK_SIZE;
        return getChunk(cx, cy).tiles[ly * CHUNK_SIZE + lx];
    }

    static int floorDiv(int v) {
        return v >= 0 ? v / CHUNK_SIZE : -((-v + CHUNK_SIZE - 1) / CHUNK_SIZE);
    }

    void tick(double dt) { (void)dt; /* simulation goes here */ }

private:
    void generate(Chunk& c, int cx, int cy) {
        for (int y = 0; y < CHUNK_SIZE; y++)
            for (int x = 0; x < CHUNK_SIZE; x++) {
                float wx = (cx * CHUNK_SIZE + x) * 0.05f;
                float wy = (cy * CHUNK_SIZE + y) * 0.05f;
                float n = smoothNoise(wx, wy) * 0.65f + smoothNoise(wx * 3, wy * 3) * 0.35f;
                uint8_t t = n < 0.35f ? WATER : n < 0.42f ? SAND
                    : n < 0.62f ? GRASS : n < 0.78f ? FOREST : ROCK;
                c.tiles[y * CHUNK_SIZE + x] = t;
            }
    }
    std::unordered_map<ChunkCoord, Chunk, ChunkHash> chunks_;
};