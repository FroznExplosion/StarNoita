#include "biome_system.h"
#include <cmath>
#include <algorithm>

using namespace godot;

BiomeSystem::BiomeSystem() : biome_seed(12345) {
    initialize_default_biomes();
}

void BiomeSystem::initialize_default_biomes() {
    biomes.clear();

    // PLAINS - default temperate biome
    BiomeDefinition plains;
    plains.type = PLAINS;
    plains.name = "Plains";
    plains.temperature = 0.5f;
    plains.humidity = 0.5f;
    plains.min_height = LAYER_SURFACE_BOTTOM;
    plains.max_height = LAYER_SURFACE_TOP;
    plains.surface_block = 5;  // grass
    plains.subsurface_block = 2; // dirt
    plains.stone_block = 1;      // stone
    plains.cave_stone_block = 10; // cave_stone
    plains.terrain_amplitude = 30.0f;
    plains.ores = {
        {6, 0.7f, 0, 2000, 3, 8},    // Copper
        {7, 0.4f, 1000, 3000, 3, 6},  // Iron
    };
    biomes.push_back(plains);

    // FOREST - more humid
    BiomeDefinition forest;
    forest.type = FOREST;
    forest.name = "Forest";
    forest.temperature = 0.5f;
    forest.humidity = 0.7f;
    forest.min_height = LAYER_SURFACE_BOTTOM;
    forest.max_height = LAYER_SURFACE_TOP;
    forest.surface_block = 5;  // grass
    forest.subsurface_block = 2; // dirt
    forest.stone_block = 1;      // stone
    forest.cave_stone_block = 10; // cave_stone
    forest.terrain_amplitude = 40.0f;
    forest.rain_frequency = 0.3f;
    forest.ores = {
        {6, 0.6f, 0, 2000, 3, 8},
        {7, 0.5f, 1000, 3000, 4, 7},
    };
    biomes.push_back(forest);

    // DESERT - hot and dry
    BiomeDefinition desert;
    desert.type = DESERT;
    desert.name = "Desert";
    desert.temperature = 0.9f;
    desert.humidity = 0.1f;
    desert.min_height = LAYER_SURFACE_BOTTOM;
    desert.max_height = LAYER_SURFACE_TOP;
    desert.surface_block = 3;  // sand
    desert.subsurface_block = 3; // sand
    desert.stone_block = 1;      // stone
    desert.cave_stone_block = 10; // cave_stone
    desert.terrain_amplitude = 20.0f;
    desert.evaporation_rate = 3.0f;
    desert.rain_frequency = 0.01f;
    desert.cannot_border = {SNOW};
    desert.ores = {
        {6, 0.5f, 0, 2000, 2, 5},
        {8, 0.2f, 1500, 3500, 2, 4},  // Gold
    };
    biomes.push_back(desert);

    // SNOW - cold
    BiomeDefinition snow;
    snow.type = SNOW;
    snow.name = "Snow";
    snow.temperature = 0.1f;
    snow.humidity = 0.3f;
    snow.min_height = LAYER_SURFACE_BOTTOM;
    snow.max_height = LAYER_SKY_BOTTOM;
    snow.surface_block = 3;  // TODO: snow block
    snow.subsurface_block = 2; // dirt
    snow.stone_block = 1;      // stone
    snow.cave_stone_block = 10; // cave_stone
    snow.terrain_amplitude = 60.0f;
    snow.cannot_border = {DESERT, JUNGLE};
    snow.ores = {
        {7, 0.6f, 500, 2500, 4, 8},
    };
    biomes.push_back(snow);

    // MOUNTAINS - high altitude
    BiomeDefinition mountains;
    mountains.type = MOUNTAINS;
    mountains.name = "Mountains";
    mountains.temperature = 0.3f;
    mountains.humidity = 0.4f;
    mountains.min_height = LAYER_SURFACE_TOP - 200;
    mountains.max_height = LAYER_SKY_BOTTOM;
    mountains.surface_block = 1;  // stone
    mountains.subsurface_block = 1;
    mountains.stone_block = 1;      // stone
    mountains.cave_stone_block = 10; // cave_stone
    mountains.terrain_amplitude = 150.0f;
    mountains.ores = {
        {6, 0.4f, 0, 1500, 3, 6},
        {7, 0.7f, 500, 2500, 5, 10},
        {8, 0.3f, 1000, 3000, 2, 5},
    };
    biomes.push_back(mountains);

    // CAVE - underground
    BiomeDefinition cave;
    cave.type = CAVE;
    cave.name = "Cave";
    cave.temperature = 0.5f;
    cave.humidity = 0.5f;
    cave.min_height = 0;
    cave.max_height = LAYER_UNDERGROUND_BOTTOM;
    cave.stone_block = 1;
    cave.cave_stone_block = 10; // cave_stone
    cave.background_block = 10;
    cave.cave_frequency = 0.08f;
    cave.ambient_light = Color(0.3, 0.3, 0.4);
    biomes.push_back(cave);
}

void BiomeSystem::generate_biome_map() {
    biome_map.clear();

    // Generate biomes across world width
    for (int x = 0; x < WORLD_WIDTH; x++) {
        float temp = get_temperature(x);
        float humid = get_humidity(x);

        // For surface, select based on climate
        BiomeType biome = select_biome_from_climate(temp, humid, SEA_LEVEL);
        biome_map[x] = biome;
    }
}

BiomeType BiomeSystem::get_biome_at(int world_x) const {
    // Wrap X
    while (world_x < 0) world_x += WORLD_WIDTH;
    world_x = world_x % WORLD_WIDTH;

    auto it = biome_map.find(world_x);
    if (it != biome_map.end()) {
        return it->second;
    }
    return PLAINS; // Default
}

const BiomeDefinition* BiomeSystem::get_biome_definition(BiomeType type) const {
    for (const auto& biome : biomes) {
        if (biome.type == type) {
            return &biome;
        }
    }
    return nullptr;
}

bool BiomeSystem::can_biomes_border(BiomeType a, BiomeType b) const {
    const BiomeDefinition* def_a = get_biome_definition(a);
    if (!def_a) return true;

    for (BiomeType forbidden : def_a->cannot_border) {
        if (forbidden == b) return false;
    }
    return true;
}

float BiomeSystem::get_temperature(int world_x) const {
    // Use noise for temperature
    float temp = noise(world_x * 0.001f, 1000.0f);
    temp = (temp + 1.0f) * 0.5f; // Remap from [-1,1] to [0,1]
    return temp;
}

float BiomeSystem::get_humidity(int world_x) const {
    // Use noise for humidity
    float humid = noise(world_x * 0.001f, 2000.0f);
    humid = (humid + 1.0f) * 0.5f; // Remap from [-1,1] to [0,1]
    return humid;
}

BiomeType BiomeSystem::select_biome_from_climate(float temperature, float humidity, int height) const {
    // Find best matching biome based on climate
    BiomeType best_match = PLAINS;
    float best_distance = 999999.0f;

    for (const auto& biome : biomes) {
        // Check height constraints
        if (height < biome.min_height || height > biome.max_height) {
            continue;
        }

        // Calculate climate distance
        float temp_diff = temperature - biome.temperature;
        float humid_diff = humidity - biome.humidity;
        float distance = sqrt(temp_diff * temp_diff + humid_diff * humid_diff);

        if (distance < best_distance) {
            best_distance = distance;
            best_match = biome.type;
        }
    }

    return best_match;
}

float BiomeSystem::noise(float x, float seed_offset) const {
    // Simple perlin-like noise (placeholder - will use FastNoise library)
    float n = sin(x * 12.9898f + seed_offset) * 43758.5453f;
    n = n - floor(n);
    return (n * 2.0f) - 1.0f; // Remap to [-1, 1]
}
