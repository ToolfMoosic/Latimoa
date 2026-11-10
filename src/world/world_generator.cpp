
#include "world_generator.h"



void WorldGenerator::generate(Chunk& out, int cx, int cy) const {

	FastNoiseLite oceanMap;
	oceanMap.SetSeed(settings.seed);
	oceanMap.SetNoiseType(FastNoiseLite::NoiseType_Perlin)
}