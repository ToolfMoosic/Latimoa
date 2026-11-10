#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

using TileId = uint16_t;
constexpr TileId TILE_AIR = 0;

enum TileFlags : uint32_t {
    TF_SOLID = 1 << 0,   // blocks movement
	TF_OPAQUE = 1 << 1,   // blocks vision
};

struct TileDef {
    std::string name;

    uint16_t atlasBase = 0;
    uint8_t  variantCount = 1;
    uint8_t  frameCount = 1;
    uint8_t  frameTimeTicks = 10;
    uint8_t  blendPriority = 0;
    uint32_t minimapColor = 0xFF00FFFF;

    uint32_t flags = 0;
    float    moveCost = 1.0f;
    uint16_t hardness = 0;
    TileId   minedInto = TILE_AIR;
    uint16_t dropItem = 0;

    uint8_t fertility = 0;
    uint8_t porosity = 0;
};

// Read-only after loading, so it is safe to share across threads.
class TileRegistry {
public:
    TileRegistry();   // starts with just "air" at id 0

    // Replaces the contents with the tiles in the JSON file.
    // atlasCellCount > 0 enables a check that every tile's cells fit in the atlas.
    // Throws std::runtime_error with a readable message on any problem; if it
    // throws, the registry is left unchanged.
    void loadFromFile(const std::string& path, uint32_t atlasCellCount = 0);

    const TileDef& get(TileId id) const;
    bool           isValid(TileId id) const { return id < defs_.size(); }
    size_t         count() const { return defs_.size(); }

    bool hasFlag(TileId id, uint32_t flag) const { return (get(id).flags & flag) != 0; }

    // Name lookups hash a string: use at startup, not per tile or per tick.
    bool   tryIdOf(std::string_view name, TileId& out) const;
    TileId idOf(std::string_view name) const;   // throws if the name is unknown

private:
    std::vector<TileDef> defs_;
    std::unordered_map<std::string, TileId> byName_;
};

inline uint16_t atlasCell(const TileDef& d, uint32_t variant, uint32_t frame) {
    return static_cast<uint16_t>(d.atlasBase +
        (variant % d.variantCount) * d.frameCount +
        (frame % d.frameCount));
}