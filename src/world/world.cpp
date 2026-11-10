#include "world/world.h"

#include <cassert>

Chunk* World::findChunk(ChunkCoord c) {
    auto it = chunks_.find(c);
    return it == chunks_.end() ? nullptr : it->second.get();
}

const Chunk* World::findChunk(ChunkCoord c) const {
    auto it = chunks_.find(c);
    return it == chunks_.end() ? nullptr : it->second.get();
}

Chunk& World::insertChunk(ChunkCoord c, std::unique_ptr<Chunk> chunk) {
    assert(chunk && "insertChunk needs a chunk");
    auto& slot = chunks_[c];
    slot = std::move(chunk);
    slot->dirtyRender = true;
    return *slot;
}

std::unique_ptr<Chunk> World::removeChunk(ChunkCoord c) {
    auto it = chunks_.find(c);
    if (it == chunks_.end()) return nullptr;
    std::unique_ptr<Chunk> out = std::move(it->second);
    chunks_.erase(it);
    return out;
}

TileId World::getTile(int tx, int ty, TileId fallback) const {
    const Chunk* ch = findChunk(chunkOfTile(tx, ty));
    return ch ? ch->getType(tileToLocal(tx), tileToLocal(ty)) : fallback;
}

int16_t World::getHeight(int tx, int ty, int16_t fallback) const {
    const Chunk* ch = findChunk(chunkOfTile(tx, ty));
    return ch ? ch->getHeight(tileToLocal(tx), tileToLocal(ty)) : fallback;
}

bool World::setTile(int tx, int ty, TileId id) {
    Chunk* ch = findChunk(chunkOfTile(tx, ty));
    if (!ch) return false;
    ch->setType(tileToLocal(tx), tileToLocal(ty), id);
    return true;
}

bool World::setHeight(int tx, int ty, int16_t h) {
    Chunk* ch = findChunk(chunkOfTile(tx, ty));
    if (!ch) return false;
    ch->setHeight(tileToLocal(tx), tileToLocal(ty), h);
    return true;
}

void World::tick(double dt) {
    (void)dt;
    // Simulation systems go here: iterate loaded chunks with forEachChunk(...).
}