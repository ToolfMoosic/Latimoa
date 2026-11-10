#pragma once
#include "world/chunk.h"

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>

// Owns loaded chunks and gives tile-level access to them.
// It does NOT generate chunks: something else (a streamer, or main for now)
// creates chunks and hands them over with insertChunk(). Not thread-safe:
// call it from one thread, and generate chunks on workers into a local
// unique_ptr<Chunk> before inserting.
class World {
public:
    // ---- chunk access ------------------------------------------------------
    Chunk* findChunk(ChunkCoord c);               // nullptr if not loaded
    const Chunk* findChunk(ChunkCoord c) const;
    bool         hasChunk(ChunkCoord c) const { return chunks_.contains(c); }
    size_t       chunkCount() const { return chunks_.size(); }

    // Takes ownership. Replaces any chunk already at c. Returns the stored chunk.
    Chunk& insertChunk(ChunkCoord c, std::unique_ptr<Chunk> chunk);

    // Removes a chunk and gives it back (e.g. so it can be saved). Null if absent.
    std::unique_ptr<Chunk> removeChunk(ChunkCoord c);

    void reserveChunks(size_t n) { chunks_.reserve(n); }

    // Visit every loaded chunk: fn(ChunkCoord, Chunk&)
    template <class Fn> void forEachChunk(Fn&& fn) {
        for (auto& [coord, chunk] : chunks_) fn(coord, *chunk);
    }
    template <class Fn> void forEachChunk(Fn&& fn) const {
        for (const auto& [coord, chunk] : chunks_) fn(coord, *chunk);
    }

    // ---- tile access (world tile coordinates) -------------------------------
    // Reads from an unloaded chunk return the fallback instead of generating.
    TileId  getTile(int tx, int ty, TileId fallback = TILE_AIR) const;
    int16_t getHeight(int tx, int ty, int16_t fallback = 0) const;

    // Writes return false (and change nothing) if the chunk isn't loaded.
    bool setTile(int tx, int ty, TileId id);
    bool setHeight(int tx, int ty, int16_t h);

    // ---- simulation ---------------------------------------------------------
    void tick(double dt);

private:
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>> chunks_;
};