#ifndef WORLD_GENERATOR_H
#define WORLD_GENERATOR_H

#include "chunk_2d.h"
#include "block_data.h"
#include "biome_system.h"
#include "../core/chunk_manager.h"
#include "../core/block_registry.h"
#include <godot_cpp/variant/vector2i.hpp>

using namespace godot;

class CaveGenerator;
class StructureGenerator;

class WorldGenerator {
private:
    ChunkManager* chunk_manager;
    BlockRegistry* block_registry;
    BiomeSystem* biome_system;
    CaveGenerator* cave_generator;
    StructureGenerator* structure_generator;

    uint64_t world_seed;

public:
    WorldGenerator(ChunkManager* chunks, BlockRegistry* registry, BiomeSystem* biomes);
    ~WorldGenerator();

    // Set world seed
    void set_seed(uint64_t seed);

    // Generate entire world
    void generate_world();

    // Generate a specific chunk
    void generate_chunk(Chunk2D* chunk);

    // Generation pipeline steps (NEW ORDER!)
    void step1_generate_biomes();
    void step2_place_buildings();        // Before terrain!
    void step3_generate_terrain();       // Adapts to buildings
    void step4_place_ores();
    void step5_carve_caves();            // Caves can't delete buildings
    void step6_generate_background();

private:
    // Generate height at X coordinate
    float generate_terrain_height(int world_x, const BiomeDefinition* biome);

    // Place block layers at column
    void generate_column(int world_x, float height, const BiomeDefinition* biome);

    // Place ores in column
    void place_ores_in_column(int world_x, const BiomeDefinition* biome);

    // Simple noise function
    float noise(float x, float y, float seed_offset) const;
    float noise(float x, float seed_offset) const;
};

// Cave generation system
class CaveGenerator {
private:
    ChunkManager* chunk_manager;
    BlockRegistry* block_registry;
    BiomeSystem* biome_system;
    uint64_t cave_seed;

public:
    CaveGenerator(ChunkManager* chunks, BlockRegistry* registry, BiomeSystem* biomes)
        : chunk_manager(chunks), block_registry(registry), biome_system(biomes), cave_seed(0) {}

    void set_seed(uint64_t seed) { cave_seed = seed; }

    // Carve caves through the world
    void carve_caves();

    // Check if position should be cave
    bool is_cave(int x, int y) const;

    // Generate cave background (edges + interior)
    void generate_cave_background(int x, int y);

private:
    // Detect if position is on cave edge
    bool is_cave_edge(int x, int y) const;

    // 3D noise for cave generation
    float cave_noise(float x, float y) const;
};

// Structure generation system
class StructureGenerator {
public:
    enum StructurePhase {
        PRE_CAVE,   // Placed before caves (dungeons, some buildings)
        POST_CAVE   // Placed after caves (houses, NPCs that need cave entrances)
    };

    struct StructureTemplate {
        String name;
        Vector2i size;
        StructurePhase phase;
        WorldLayer layer;
        std::vector<BiomeType> allowed_biomes;
        int min_spacing;        // Minimum distance between structures
        float spawn_chance;     // 0.0-1.0
        bool needs_flat_ground;
        bool has_doorway;       // Creates doorway to cave entrance

        // Structure data (could be loaded from file)
        std::vector<std::vector<uint16_t>> blocks;
    };

private:
    ChunkManager* chunk_manager;
    BlockRegistry* block_registry;
    BiomeSystem* biome_system;
    uint64_t structure_seed;

    std::vector<StructureTemplate> structures;
    std::vector<Vector2i> placed_structures;  // Track placed structure positions

public:
    StructureGenerator(ChunkManager* chunks, BlockRegistry* registry, BiomeSystem* biomes)
        : chunk_manager(chunks), block_registry(registry), biome_system(biomes), structure_seed(0) {}

    void set_seed(uint64_t seed) { structure_seed = seed; }

    // Place structures (call for each phase)
    void place_structures(StructurePhase phase);

    // Add structure template
    void register_structure(const StructureTemplate& structure);

    // Clear placed structure tracking
    void clear_placed() { placed_structures.clear(); }

private:
    // Find valid position for structure
    Vector2i find_structure_position(const StructureTemplate& structure);

    // Check if position is valid for structure
    bool can_place_structure(const StructureTemplate& structure, Vector2i pos);

    // Place structure at position
    void place_structure(const StructureTemplate& structure, Vector2i pos);

    // Create doorway to nearest cave
    void create_cave_doorway(Vector2i structure_pos, Vector2i structure_size);

    float rand_float();
};

#endif // WORLD_GENERATOR_H
