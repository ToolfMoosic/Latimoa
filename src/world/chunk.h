#pragma once

#include "world_constants.h"
#include <array>

struct Chunk {
	std::array<TileId, CHUNK_AREA> type;
	std::array<int16_t, CHUNK_AREA> height;
};