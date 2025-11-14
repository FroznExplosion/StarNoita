#ifndef BIOME_SYSTEM_H
#define BIOME_SYSTEM_H

#include "block_data.h"
#include "world_constants.h"
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/color.hpp>
#include <vector>
#include <unordered_set>

using namespace godot;

// Biome definition with all properties
struct BiomeDefinition {
    BiomeType type;
    String name;

    // Climate requirements
    float temperature;         // 0.0 (cold) to 1.0 (hot)
    float humidity;           // 0.0 (dry) to 1.0 (wet)

    // Height constraints
    int min_height;           // Minimum Y (absolute coords)
    int max_height;           // Maximum Y (absolute coords)

    // Block palette
    uint16_t surface_block;      // Top layer (grass, sand, etc.)
    uint16_t subsurface_block;   // Below surface (dirt, sandstone)
    uint16_t stone_block;        // Deep underground
    uint16_t background_block;   // Background variant

    // Terrain generation
    float terrain_frequency;     // Noise frequency
    float terrain_amplitude;     // Height variation
    float cave_frequency;        // Cave density

    // Environmental effects
    float evaporation_rate;      // Liquid evaporation multiplier
    float rain_frequency;        // How often it rains
    Color ambient_light;         // Tint color

    // Ore distribution (block ID, rarity 0-1, min depth, max depth)
    struct OreConfig {
        uint16_t ore_id;
        float rarity;           // 0.0-1.0 (lower = rarer)
        int min_depth;          // Below sea level
        int max_depth;
        int vein_size_min;      // Min blocks per vein
        int vein_size_max;      // Max blocks per vein
    };
    std::vector<OreConfig> ores;

    // Biome compatibility
    std::vector<BiomeType> cannot_border;  // Can't be adjacent
    std::vector<BiomeType> prefers_near;   // Prefers to spawn near

    BiomeDefinition()
        : type(PLAINS)
        , temperature(0.5f)
        , humidity(0.5f)
        , min_height(0)
        , max_height(WORLD_HEIGHT)
        , surface_block(5)  // grass
        , subsurface_block(2) // dirt
        , stone_block(1)    // stone
        , background_block(10) // bg stone
        , terrain_frequency(0.01f)
        , terrain_amplitude(50.0f)
        , cave_frequency(0.05f)
        , evaporation_rate(1.0f)
        , rain_frequency(0.1f)
        , ambient_light(Color(1, 1, 1))
    {}
};

class BiomeSystem {
private:
    // All registered biomes
    std::vector<BiomeDefinition> biomes;

    // Biome map (stored per world X coordinate)
    std::unordered_map<int, BiomeType> biome_map;

    // Noise seed for biome generation
    uint64_t biome_seed;

public:
    BiomeSystem();
    ~BiomeSystem() = default;

    // Initialize default biomes
    void initialize_default_biomes();

    // Set seed for biome generation
    void set_seed(uint64_t seed) { biome_seed = seed; }

    // Generate biome map for world
    void generate_biome_map();

    // Get biome at world X coordinate
    BiomeType get_biome_at(int world_x) const;

    // Get biome definition
    const BiomeDefinition* get_biome_definition(BiomeType type) const;

    // Check if two biomes can be adjacent
    bool can_biomes_border(BiomeType a, BiomeType b) const;

    // Get temperature/humidity at position (for biome selection)
    float get_temperature(int world_x) const;
    float get_humidity(int world_x) const;

    // Clear biome map
    void clear() { biome_map.clear(); }

private:
    // Select biome based on climate
    BiomeType select_biome_from_climate(float temperature, float humidity, int height) const;

    // Simple noise function (will use FastNoise later)
    float noise(float x, float seed_offset) const;
};

#endif // BIOME_SYSTEM_H
