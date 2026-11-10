#include "world_generator.h"

#include <algorithm>
#include <cmath>

WorldGenerator::WorldGenerator(const TileRegistry& tiles, const WorldGenerationSettings& s)
    : settings_(s) {
    water_ = tiles.idOf("water_shallow");
    sand_ = tiles.idOf("sand");
    grass_ = tiles.idOf("grass");
    forest_ = tiles.idOf("forest");
    rock_ = tiles.idOf("rock");


    heightNoise_.SetSeed((int)settings_.seed);
    heightNoise_.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    heightNoise_.SetFrequency(settings_.scale);
    heightNoise_.SetFractalType(FastNoiseLite::FractalType_FBm);
    heightNoise_.SetFractalOctaves(4);

    seaThreshold_ = findSeaThreshold();
}

float WorldGenerator::sampleHeight(float tx, float ty) const {
    float n = heightNoise_.GetNoise(tx, ty);                 // roughly -1..1, clusters near 0
    return std::clamp(n * 0.5f + 0.5f, 0.0f, 1.0f);
}

// Noise values bunch up in the middle, so "80% ocean" can't be a fixed height value.
// Sample the noise once and pick the height that has oceanPercent of samples below it.
float WorldGenerator::findSeaThreshold() const {
    constexpr int N = 256, STEP = 16;                        // 65k samples over ~4000x4000 tiles
    std::vector<float> v;
    v.reserve(N * N);
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x)
            v.push_back(sampleHeight(float((x - N / 2) * STEP), float((y - N / 2) * STEP)));

    float p = std::clamp(settings_.oceanPercent, 0.0f, 1.0f);
    size_t k = (size_t)(p * float(v.size() - 1));
    std::nth_element(v.begin(), v.begin() + k, v.end());
    return v[k];
}

void WorldGenerator::generate(Chunk& out, int cx, int cy) const {
    const float sea = seaThreshold_;
    const float landRange = std::max(1.0f - sea, 0.0001f);

    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            int tx = cx * CHUNK_SIZE + x;                    // world tile coordinates
            int ty = cy * CHUNK_SIZE + y;
            int i = y * CHUNK_SIZE + x;

            float h = sampleHeight((float)tx, (float)ty);
            out.height[i] = (int16_t)std::lround(h * 100.0f);

            if (h < sea) {
                out.type[i] = water_;
                continue;
            }

            // 0 at the shoreline, 1 at the highest ground
            float land = (h - sea) / landRange;

            if (land < settings_.beachSize) out.type[i] = sand_;
            else if (land < 0.45f)               out.type[i] = grass_;
            else if (land < 0.75f)               out.type[i] = forest_;
            else                                 out.type[i] = rock_;
        }
    }
}