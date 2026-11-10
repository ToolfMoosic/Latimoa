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
    // where it occurs (climate)
    float idealTemp, idealMoisture;
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
	int seaLevel = 10;
	float oceanPercent = 0.8;
	float scale = 1;

};

class WorldGenerator {
public:

	explicit WorldGenerator(const WorldGenerationSettings& s = {});
	void generate(Chunk& out, int cx, int cy) const;

private:
	WorldGenerationSettings settings;

};