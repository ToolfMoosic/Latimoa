#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

// Enum name for every tile type
enum TileId : uint16_t {
	TILE_AIR,
};

enum TileFlags : uint32_t {
    TF_SOLID = 1 << 0,   // blocks movement
};

struct TileDef {
    // identity
    std::string name;            // "grass" (cold data, only for tools/UI/loading)

    // rendering
    uint16_t atlasBase;          // first atlas cell
    uint8_t  variantCount;       // random variants (consecutive cells)
    uint8_t  frameCount;         // animation frames (consecutive cells)
    uint8_t  frameTimeTicks;
    uint8_t  blendPriority;      // which terrain draws its edge over the neighbor
    uint32_t minimapColor;

    // gameplay
    uint32_t flags;              // TileFlags
    float    moveCost;           // 1.0 = normal, 0 = impassable (or use TF_SOLID)
    uint16_t hardness;           // mining time
    TileId   minedInto;          // what it becomes when dug
    uint16_t dropItem;           // item id, 0 = none

    // simulation
    uint8_t  fertility;
    uint8_t  porosity;           // how fast liquid soaks in (if you want that)
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