#pragma once
#include "world/tile.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

// CHUNK_SIZE must be a power of two; CHUNK_SHIFT must be log2(CHUNK_SIZE).
constexpr int CHUNK_SIZE = 32;
constexpr int CHUNK_SHIFT = 5;
constexpr int CHUNK_MASK = CHUNK_SIZE - 1;
constexpr int CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
static_assert((1 << CHUNK_SHIFT) == CHUNK_SIZE, "CHUNK_SHIFT must be log2(CHUNK_SIZE)");

// ---- coordinate helpers (correct for negative coordinates) -----------------
inline int tileToChunk(int t) { return t >> CHUNK_SHIFT; }   // floor division
inline int tileToLocal(int t) { return t & CHUNK_MASK; }     // 0 .. CHUNK_SIZE-1
inline int localIndex(int lx, int ly) { return ly * CHUNK_SIZE + lx; }

struct ChunkCoord {
    int x = 0, y = 0;
    bool operator==(const ChunkCoord&) const = default;
};

template <> struct std::hash<ChunkCoord> {
    size_t operator()(const ChunkCoord& c) const noexcept {
        uint64_t k = (uint64_t(uint32_t(c.x)) << 32) | uint32_t(c.y);
        k ^= k >> 30; k *= 0xbf58476d1ce4e5b9ULL;   // splitmix64 finalizer
        k ^= k >> 27; k *= 0x94d049bb133111ebULL;
        k ^= k >> 31;
        return size_t(k);
    }
};

inline ChunkCoord chunkOfTile(int tx, int ty) { return { tileToChunk(tx), tileToChunk(ty) }; }

// Plain data with a couple of helpers that keep the dirty flag honest.
// Hot loops may read the arrays directly; writes should go through set*().
struct Chunk {
    std::array<TileId, CHUNK_AREA> type{};     // terrain material
    std::array<int16_t, CHUNK_AREA> height{};   // integer height levels

    bool dirtyRender = true;                    // needs its cached texture redrawn

    TileId  getType(int lx, int ly) const { return type[localIndex(lx, ly)]; }
    int16_t getHeight(int lx, int ly) const { return height[localIndex(lx, ly)]; }

    void setType(int lx, int ly, TileId id) {
        type[localIndex(lx, ly)] = id;
        dirtyRender = true;
    }
    void setHeight(int lx, int ly, int16_t h) {
        height[localIndex(lx, ly)] = h;
        dirtyRender = true;
    }
};