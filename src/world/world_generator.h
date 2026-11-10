#pragma once

#include "world.h"
#include "Noise\FastNoiseLite.h"
#include <string>
#include "tile.h"

struct TileRule {              // first matching rule wins, checked top to bottom
    TileId tile;
    float  minHeight, maxHeight;   // 0..1 normalized height range
};

struct BiomeDef {
    std::string name;
    float idealTemp, idealHumidity, idealHeight;
    // terrain shape
    float baseHeight;              // 0..1
    float amplitude;               // how bumpy
    float frequency;
    int   octaves;
    // surface
    std::vector<TileRule>     tiles;
};

struct WorldGenerationSettings {
    uint32_t seed = 67;
    float    oceanPercent = 0.55f;  // fraction of the world that is water, 0..1
    float    scale = 0.01f;  // noise frequency: 0.01 = features ~100 tiles wide
    float    beachSize = 0.06f;  // share of the land height range that is sand
};

class WorldGenerator {
public:
    explicit WorldGenerator(const TileRegistry& tiles, const WorldGenerationSettings& s = {});
    void generate(Chunk& out, int cx, int cy) const;

private:
    float sampleHeight(float tx, float ty) const;   // 0..1
    float findSeaThreshold() const;

    WorldGenerationSettings settings_;
    FastNoiseLite heightNoise_;      // configured once, then only read (thread-safe)
    float seaThreshold_ = 0.5f;      // heights below this are water

    TileId water_, sand_, grass_, forest_, rock_;
};