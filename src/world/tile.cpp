#include "tile.h"

#include <nlohmann/json.hpp>

#include <cassert>
#include <fstream>
#include <limits>
#include <stdexcept>

using nlohmann::json;

namespace {

    struct FlagName { const char* name; uint32_t bit; };
    constexpr FlagName FLAG_NAMES[] = {
        {"solid",      TF_SOLID},
        {"opaque",     TF_OPAQUE},
        {"buildable",  TF_BUILDABLE},
        {"flammable",  TF_FLAMMABLE},
        {"liquid_bed", TF_LIQUID_BED},
    };

    [[noreturn]] void fail(const std::string& file, const std::string& tile, const std::string& msg) {
        throw std::runtime_error(file + ": tile '" + tile + "': " + msg);
    }

    int readInt(const json& j, const char* key, int def, int lo, int hi,
        const std::string& file, const std::string& tile) {
        auto it = j.find(key);
        if (it == j.end()) return def;
        if (!it->is_number_integer()) fail(file, tile, std::string("'") + key + "' must be an integer");
        long long v = it->get<long long>();
        if (v < lo || v > hi)
            fail(file, tile, std::string("'") + key + "' must be between " +
                std::to_string(lo) + " and " + std::to_string(hi));
        return static_cast<int>(v);
    }

    float readFloat(const json& j, const char* key, float def, float lo,
        const std::string& file, const std::string& tile) {
        auto it = j.find(key);
        if (it == j.end()) return def;
        if (!it->is_number()) fail(file, tile, std::string("'") + key + "' must be a number");
        float v = it->get<float>();
        if (v < lo) fail(file, tile, std::string("'") + key + "' must be >= " + std::to_string(lo));
        return v;
    }

    // [r, g, b] or [r, g, b, a], each 0-255 -> packed 0xRRGGBBAA
    uint32_t readColor(const json& j, const char* key, uint32_t def,
        const std::string& file, const std::string& tile) {
        auto it = j.find(key);
        if (it == j.end()) return def;
        if (!it->is_array() || (it->size() != 3 && it->size() != 4))
            fail(file, tile, std::string("'") + key + "' must be [r, g, b] or [r, g, b, a]");
        uint32_t c[4] = { 0, 0, 0, 255 };
        for (size_t i = 0; i < it->size(); ++i) {
            const json& v = (*it)[i];
            if (!v.is_number_integer() || v.get<long long>() < 0 || v.get<long long>() > 255)
                fail(file, tile, std::string("'") + key + "' values must be integers 0-255");
            c[i] = static_cast<uint32_t>(v.get<long long>());
        }
        return (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | c[3];
    }

    uint32_t readFlags(const json& j, const std::string& file, const std::string& tile) {
        auto it = j.find("flags");
        if (it == j.end()) return 0;
        if (!it->is_array()) fail(file, tile, "'flags' must be an array of strings");
        uint32_t flags = 0;
        for (const json& f : *it) {
            if (!f.is_string()) fail(file, tile, "'flags' must be an array of strings");
            const std::string s = f.get<std::string>();
            bool found = false;
            for (const FlagName& fn : FLAG_NAMES) {
                if (s == fn.name) { flags |= fn.bit; found = true; break; }
            }
            if (!found) fail(file, tile, "unknown flag '" + s + "'");
        }
        return flags;
    }

    TileDef makeAir() {
        TileDef d;
        d.name = "air";
        d.minimapColor = 0x000000FF;
        return d;
    }

} // namespace

TileRegistry::TileRegistry() {
    defs_.push_back(makeAir());
    byName_["air"] = TILE_AIR;
}

const TileDef& TileRegistry::get(TileId id) const {
    assert(id < defs_.size() && "TileId out of range");
    return defs_[id];
}

bool TileRegistry::tryIdOf(std::string_view name, TileId& out) const {
    auto it = byName_.find(std::string(name));
    if (it == byName_.end()) return false;
    out = it->second;
    return true;
}

TileId TileRegistry::idOf(std::string_view name) const {
    TileId id;
    if (!tryIdOf(name, id))
        throw std::runtime_error("Unknown tile name: '" + std::string(name) + "'");
    return id;
}

void TileRegistry::loadFromFile(const std::string& path, uint32_t atlasCellCount) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open tile data file: " + path);

    json root;
    try {
        root = json::parse(in);
    }
    catch (const json::exception& e) {
        throw std::runtime_error(path + ": JSON error: " + e.what());
    }
    if (!root.is_object() || !root.contains("tiles") || !root["tiles"].is_array())
        throw std::runtime_error(path + ": expected an object with a \"tiles\" array");

    // Build into locals so a failed load leaves the registry untouched.
    std::vector<TileDef> defs;
    std::unordered_map<std::string, TileId> byName;
    std::vector<std::string> minedIntoNames;   // parallel to defs, resolved in pass 2

    defs.push_back(makeAir());
    byName["air"] = TILE_AIR;
    minedIntoNames.emplace_back();

    // Pass 1: read every tile and assign IDs in file order.
    for (const json& jt : root["tiles"]) {
        if (!jt.is_object() || !jt.contains("name") || !jt["name"].is_string())
            throw std::runtime_error(path + ": every tile needs a string \"name\"");

        TileDef d;
        d.name = jt["name"].get<std::string>();
        const std::string& n = d.name;

        if (byName.count(n)) fail(path, n, "duplicate name (note: \"air\" is built in)");
        if (defs.size() >= std::numeric_limits<TileId>::max()) fail(path, n, "too many tiles");

        d.atlasBase = static_cast<uint16_t>(readInt(jt, "atlas", 0, 0, 65535, path, n));
        d.variantCount = static_cast<uint8_t>(readInt(jt, "variants", 1, 1, 255, path, n));
        d.frameCount = static_cast<uint8_t>(readInt(jt, "frames", 1, 1, 255, path, n));
        d.frameTimeTicks = static_cast<uint8_t>(readInt(jt, "frameTime", 10, 1, 255, path, n));
        d.blendPriority = static_cast<uint8_t>(readInt(jt, "blendPriority", 0, 0, 255, path, n));
        d.minimapColor = readColor(jt, "color", d.minimapColor, path, n);
        d.flags = readFlags(jt, path, n);
        d.moveCost = readFloat(jt, "moveCost", 1.0f, 0.0f, path, n);
        d.hardness = static_cast<uint16_t>(readInt(jt, "hardness", 0, 0, 65535, path, n));
        d.fertility = static_cast<uint8_t>(readInt(jt, "fertility", 0, 0, 255, path, n));
        d.porosity = static_cast<uint8_t>(readInt(jt, "porosity", 0, 0, 255, path, n));

        if (atlasCellCount > 0) {
            uint32_t cells = uint32_t(d.variantCount) * d.frameCount;
            if (uint32_t(d.atlasBase) + cells > atlasCellCount)
                fail(path, n, "atlas cells " + std::to_string(d.atlasBase) + ".." +
                    std::to_string(d.atlasBase + cells - 1) +
                    " run past the end of the atlas (" +
                    std::to_string(atlasCellCount) + " cells)");
        }

        std::string minedInto;
        if (auto it = jt.find("minedInto"); it != jt.end()) {
            if (!it->is_string()) fail(path, n, "'minedInto' must be a tile name");
            minedInto = it->get<std::string>();
        }

        byName[n] = static_cast<TileId>(defs.size());
        defs.push_back(std::move(d));
        minedIntoNames.push_back(std::move(minedInto));
    }

    // Pass 2: names that point at other tiles (they may be defined later in the file).
    for (size_t i = 1; i < defs.size(); ++i) {
        if (minedIntoNames[i].empty()) continue;
        auto it = byName.find(minedIntoNames[i]);
        if (it == byName.end())
            fail(path, defs[i].name, "minedInto refers to unknown tile '" + minedIntoNames[i] + "'");
        defs[i].minedInto = it->second;
    }

    defs_ = std::move(defs);
    byName_ = std::move(byName);
}