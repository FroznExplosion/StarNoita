#ifndef BLOCK_DATA_H
#define BLOCK_DATA_H

#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/color.hpp>
#include <vector>

using namespace godot;

// Compact block storage for 2D terrain
// Each block is 4 bytes total
struct Block2D {
    uint16_t type_id;           // Block type ID (0-65535 types)
    uint8_t variant : 4;        // Visual variant (0-15)
    uint8_t metadata : 4;       // Additional data (rotation, state, etc.)
    uint8_t flags;              // Packed boolean flags

    // Flag bit positions
    enum Flags : uint8_t {
        HAS_GRAVITY     = 1 << 0,  // Falls when unsupported (sand, gravel)
        IS_LIQUID       = 1 << 1,  // Liquid block
        IS_PLATFORM     = 1 << 2,  // One-way platform
        SUPPORTS_BLEND  = 1 << 3,  // Use 47-tile auto-tiling
        IS_BACKGROUND   = 1 << 4,  // Background layer block
        IS_DAMAGED      = 1 << 5,  // Has damage (check separate health array)
        BLOCKS_LIGHT    = 1 << 6,  // Blocks light propagation
        EMITS_LIGHT     = 1 << 7   // Emits light
    };

    Block2D() : type_id(0), variant(0), metadata(0), flags(0) {}

    inline bool has_flag(Flags flag) const { return (flags & flag) != 0; }
    inline void set_flag(Flags flag, bool value) {
        if (value) flags |= flag;
        else flags &= ~flag;
    }
};

// Block health data (stored separately for memory efficiency)
// Only damaged blocks are stored in hashmap
struct BlockHealth {
    float current_health;    // Current HP (0-100)
    float max_health;        // Maximum HP (usually 100)

    BlockHealth() : current_health(100.0f), max_health(100.0f) {}
    BlockHealth(float max_hp) : current_health(max_hp), max_health(max_hp) {}

    inline float get_health_percentage() const {
        return current_health / max_health;
    }

    inline bool is_destroyed() const {
        return current_health <= 0.0f;
    }
};

// Full block definition for block registry
struct BlockDefinition {
    uint16_t id;                          // Unique block ID
    String name;                          // Block name
    Vector2i size;                        // Size in tiles (1x1, 2x3, etc.)

    // Health and damage properties
    float max_health;                     // Maximum health (default 100)
    float damage_reduction;               // Damage reduction amount
    int required_tool_tier;               // Minimum tool tier to mine
    float mining_time;                    // Base time to mine in seconds

    // Physics properties
    bool affected_by_gravity;             // Can fall (sand, gravel)
    bool breaks_on_fall;                  // Breaks if falls too far
    float density;                        // For buoyancy in liquids
    int stability_threshold;              // Neighbors needed to stay stable

    // Visual properties
    String texture_path;                  // Path to texture atlas
    bool use_autotile;                    // Use 47-tile blending
    std::vector<uint16_t> blends_with;    // Block IDs to blend with
    bool has_random_variants;             // Use random texture variants
    uint8_t variant_count;                // Number of variants

    // Lighting properties
    uint8_t light_opacity;                // How much light it blocks (0-255)
    uint8_t light_emission;               // Light level emitted (0-255)
    Color light_color;                    // Color of emitted light

    // Special properties
    bool is_door;                         // Door block
    bool is_chest;                        // Storage container
    bool is_platform;                     // One-way platform
    bool grows_plants;                    // Can have plants on top
    bool is_ore;                          // Ore block
    bool is_structure_block;              // Part of structure

    // Background generation properties
    bool can_be_background;               // Can appear in background layer
    uint16_t background_variant_id;       // Different block ID for background
    bool background_ore_priority;         // Ores keep background version in caves

    BlockDefinition()
        : id(0)
        , size(Vector2i(1, 1))
        , max_health(100.0f)
        , damage_reduction(0.0f)
        , required_tool_tier(0)
        , mining_time(1.0f)
        , affected_by_gravity(false)
        , breaks_on_fall(false)
        , density(1.0f)
        , stability_threshold(0)
        , use_autotile(false)
        , has_random_variants(false)
        , variant_count(1)
        , light_opacity(255)
        , light_emission(0)
        , light_color(Color(1, 1, 1))
        , is_door(false)
        , is_chest(false)
        , is_platform(false)
        , grows_plants(false)
        , is_ore(false)
        , is_structure_block(false)
        , can_be_background(true)
        , background_variant_id(0)
        , background_ore_priority(false)
    {}
};

// World layer definitions
enum WorldLayer : uint8_t {
    SPACE = 0,        // 10000 to 9000 (1000 blocks)
    SKY,              // 9000 to 7000 (2000 blocks)
    SURFACE,          // 8100 to 7900 (200 blocks around sea level)
    UNDERGROUND,      // 7900 to 3000 (4900 blocks)
    UNDERWORLD,       // 3000 to 2000 (1000 blocks)
    DEEP_WORLD,       // 2000 to 0 (2000 blocks)
    LAYER_COUNT
};

// Biome types
enum BiomeType : uint8_t {
    PLAINS = 0,
    FOREST,
    DESERT,
    SNOW,
    JUNGLE,
    SWAMP,
    OCEAN,
    BEACH,
    MOUNTAINS,
    VOLCANO,
    MUSHROOM,
    CORRUPTION,
    HALLOW,
    CAVE,
    CRYSTAL_CAVERN,
    SPACE_VOID,
    ASTEROID,
    BIOME_COUNT
};

// Liquid types
enum LiquidType : uint8_t {
    LIQUID_NONE = 0,
    WATER,
    LAVA,
    HONEY,
    ACID,
    LIQUID_TYPE_COUNT
};

#endif // BLOCK_DATA_H
