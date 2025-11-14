#include "world_generator.h"
#include <cmath>
#include <algorithm>

using namespace godot;

WorldGenerator::WorldGenerator(ChunkManager* chunks, BlockRegistry* registry, BiomeSystem* biomes)
    : chunk_manager(chunks)
    , block_registry(registry)
    , biome_system(biomes)
    , world_seed(12345)
{
    cave_generator = new CaveGenerator(chunks, registry, biomes);
    structure_generator = new StructureGenerator(chunks, registry, biomes);
}

WorldGenerator::~WorldGenerator() {
    delete cave_generator;
    delete structure_generator;
}

void WorldGenerator::set_seed(uint64_t seed) {
    world_seed = seed;
    biome_system->set_seed(seed);
    cave_generator->set_seed(seed + 1000);
    structure_generator->set_seed(seed + 2000);
}

void WorldGenerator::generate_world() {
    // NEW Generation pipeline order:
    // 1. Biomes first
    // 2. Buildings BEFORE terrain
    // 3. Terrain adapts to buildings
    // 4. Ores
    // 5. Caves (can't delete buildings)
    // 6. Background
    step1_generate_biomes();
    step2_place_buildings();  // NEW: Before terrain!
    step3_generate_terrain(); // Adapts to buildings
    step4_place_ores();
    step5_carve_caves();      // Protects building blocks
    step6_generate_background();
}

void WorldGenerator::step1_generate_biomes() {
    biome_system->generate_biome_map();
}

void WorldGenerator::step2_place_buildings() {
    // Place buildings BEFORE terrain so terrain can adapt
    structure_generator->place_structures(StructureGenerator::PRE_CAVE);
    // Note: Buildings mark their footprint, terrain generation will respect it
}

void WorldGenerator::step3_generate_terrain() {
    // Generate terrain for entire world
    // This now respects building positions and flattens terrain near them
    for (int x = 0; x < WORLD_WIDTH; x++) {
        BiomeType biome_type = biome_system->get_biome_at(x);
        const BiomeDefinition* biome = biome_system->get_biome_definition(biome_type);

        if (!biome) continue;

        float height = generate_terrain_height(x, biome);

        // Check if near terrain flattening markers (from buildings)
        const auto& markers = structure_generator->get_terrain_markers();
        for (const auto& marker : markers) {
            // Calculate horizontal distance (with world wrapping)
            int dx = abs(x - marker.position.x);
            if (dx > WORLD_WIDTH / 2) {
                dx = WORLD_WIDTH - dx;  // Account for world wrap
            }

            // If within flattening radius
            if (dx <= marker.flatten_radius) {
                // Calculate blend factor (1.0 at marker, 0.0 at edge)
                float distance_ratio = static_cast<float>(dx) / static_cast<float>(marker.flatten_radius);
                float blend = 1.0f - distance_ratio;  // Smooth falloff
                blend = blend * blend;  // Square for smoother curve

                // Blend toward marker's Y level
                float target_height = static_cast<float>(marker.position.y);
                height = height * (1.0f - blend) + target_height * blend;
            }
        }

        generate_column(x, height, biome);
    }
}

void WorldGenerator::step4_place_ores() {
    for (int x = 0; x < WORLD_WIDTH; x++) {
        BiomeType biome_type = biome_system->get_biome_at(x);
        const BiomeDefinition* biome = biome_system->get_biome_definition(biome_type);

        if (!biome) continue;

        place_ores_in_column(x, biome);
    }
}

void WorldGenerator::step5_carve_caves() {
    cave_generator->carve_caves();
}

void WorldGenerator::step6_generate_background() {
    // Generate background blocks across entire world
    // Background uses same block as foreground (stone creates stone background)
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            Vector2i pos(x, y);

            const Block2D* fg = chunk_manager->get_block_at_tile(pos);
            if (!fg) continue;

            // Has foreground block - create matching background
            if (fg->type_id != 0) {
                const BlockDefinition* def = block_registry->get_block_definition(fg->type_id);
                if (def && def->can_be_background) {
                    Block2D bg_block;
                    bg_block.type_id = fg->type_id; // Same as foreground
                    chunk_manager->set_block_at_tile(pos, bg_block, true);
                }
            }
        }
    }
}

float WorldGenerator::generate_terrain_height(int world_x, const BiomeDefinition* biome) {
    if (!biome) return SEA_LEVEL;

    // Multi-octave noise for terrain
    float height = SEA_LEVEL;

    // Large features
    height += noise(world_x * biome->terrain_frequency, 100.0f) * biome->terrain_amplitude;

    // Medium features
    height += noise(world_x * biome->terrain_frequency * 2.5f, 200.0f) * (biome->terrain_amplitude * 0.5f);

    // Small details
    height += noise(world_x * biome->terrain_frequency * 5.0f, 300.0f) * (biome->terrain_amplitude * 0.25f);

    return height;
}

void WorldGenerator::generate_column(int world_x, float height, const BiomeDefinition* biome) {
    int terrain_top = static_cast<int>(height);

    for (int y = 0; y < WORLD_HEIGHT; y++) {
        Vector2i pos(world_x, y);
        Block2D block;

        if (y > terrain_top) {
            // Above ground - air or water
            if (y < SEA_LEVEL) {
                block.type_id = 0; // TODO: Water liquid instead
            } else {
                block.type_id = 0; // Air
            }
        } else if (y == terrain_top) {
            // Surface
            block.type_id = biome->surface_block;
        } else if (y > terrain_top - 5) {
            // Subsurface (dirt/sand layer)
            block.type_id = biome->subsurface_block;
        } else {
            // Deep stone
            block.type_id = biome->stone_block;
        }

        chunk_manager->set_block_at_tile(pos, block);
    }
}

void WorldGenerator::place_ores_in_column(int world_x, const BiomeDefinition* biome) {
    for (const auto& ore_config : biome->ores) {
        // Random chance for ore vein to spawn in this column
        float spawn_roll = noise(world_x * 0.1f, ore_config.ore_id * 1000.0f);
        if ((spawn_roll + 1.0f) * 0.5f > ore_config.rarity) {
            continue; // No ore here
        }

        // Determine vein center Y
        int min_y = SEA_LEVEL - ore_config.max_depth;
        int max_y = SEA_LEVEL - ore_config.min_depth;

        int vein_y = min_y + static_cast<int>(noise(world_x * 0.05f, ore_config.ore_id * 500.0f + 123.0f) * (max_y - min_y) * 0.5f + (max_y - min_y) * 0.5f);

        // Determine vein size
        int vein_size = ore_config.vein_size_min + static_cast<int>(noise(world_x * 0.03f, ore_config.ore_id * 777.0f) * (ore_config.vein_size_max - ore_config.vein_size_min) * 0.5f + (ore_config.vein_size_max - ore_config.vein_size_min) * 0.5f);

        // Place ore vein (simple blob)
        for (int i = 0; i < vein_size; i++) {
            int ox = static_cast<int>(noise(i * 123.0f, ore_config.ore_id * 456.0f) * 3.0f);
            int oy = static_cast<int>(noise(i * 456.0f, ore_config.ore_id * 789.0f) * 3.0f);

            Vector2i ore_pos(world_x + ox, vein_y + oy);

            // Only replace stone with ore
            const Block2D* existing = chunk_manager->get_block_at_tile(ore_pos);
            if (existing && existing->type_id == biome->stone_block) {
                Block2D ore_block;
                ore_block.type_id = ore_config.ore_id;
                chunk_manager->set_block_at_tile(ore_pos, ore_block);
            }
        }
    }
}

float WorldGenerator::noise(float x, float y, float seed_offset) const {
    // 2D noise
    float n = sin(x * 12.9898f + y * 78.233f + seed_offset + world_seed) * 43758.5453f;
    n = n - floor(n);
    return (n * 2.0f) - 1.0f;
}

float WorldGenerator::noise(float x, float seed_offset) const {
    float n = sin(x * 12.9898f + seed_offset + world_seed) * 43758.5453f;
    n = n - floor(n);
    return (n * 2.0f) - 1.0f;
}

// CaveGenerator implementation
void CaveGenerator::carve_caves() {
    for (int x = 0; x < WORLD_WIDTH; x++) {
        // Get biome at this X coordinate for biome-specific cave stone
        BiomeType biome_type = biome_system->get_biome_at(x);
        const BiomeDefinition* biome = biome_system->get_biome_definition(biome_type);

        for (int y = 0; y < SEA_LEVEL; y++) {  // Only underground
            if (is_cave(x, y)) {
                Vector2i pos(x, y);

                const Block2D* existing = chunk_manager->get_block_at_tile(pos);
                if (!existing) continue;

                const BlockDefinition* def = block_registry->get_block_definition(existing->type_id);

                // PROTECT building blocks - never delete
                if (def && def->is_structure_block) {
                    continue; // Buildings are sacred!
                }

                // Ores stay in foreground
                if (def && def->is_ore) {
                    continue; // Keep ore
                }

                // Check if this is on cave edge or interior
                bool on_edge = is_cave_edge(x, y);

                if (on_edge) {
                    // Cave edge - keep stone (regular biome stone)
                    // Don't delete, stone stays as "cave wall"
                    continue;
                } else {
                    // Cave interior - replace stone with biome-specific cave_stone variant
                    if (biome && existing->type_id == biome->stone_block) {
                        Block2D cave_stone;
                        cave_stone.type_id = biome->cave_stone_block; // Use biome-specific variant!
                        chunk_manager->set_block_at_tile(pos, cave_stone);
                    } else {
                        // Other blocks (dirt, etc) - remove to make cave
                        Block2D air;
                        air.type_id = 0;
                        chunk_manager->set_block_at_tile(pos, air);
                    }
                }
            }
        }
    }
}

bool CaveGenerator::is_cave(int x, int y) const {
    // 3D perlin noise for caves
    float cave_value = cave_noise(x * 0.05f, y * 0.05f);

    // Caves exist where noise > threshold
    return cave_value > 0.3f;
}

void CaveGenerator::generate_cave_background(int x, int y) {
    Vector2i pos(x, y);

    // Check if on cave edge
    if (is_cave_edge(x, y)) {
        // Place cave wall (outline block)
        Block2D wall;
        wall.type_id = 11; // cave_wall
        chunk_manager->set_block_at_tile(pos, wall, true);
    } else {
        // Interior - place background stone
        Block2D bg;
        bg.type_id = 10; // background_stone
        chunk_manager->set_block_at_tile(pos, bg, true);
    }

    // If foreground has ore, place ore in background too
    const Block2D* fg = chunk_manager->get_block_at_tile(pos);
    if (fg && fg->type_id != 0) {
        const BlockDefinition* def = block_registry->get_block_definition(fg->type_id);
        if (def && def->is_ore && def->background_ore_priority) {
            // Keep ore in background
            Block2D bg_ore;
            bg_ore.type_id = fg->type_id;
            chunk_manager->set_block_at_tile(pos, bg_ore, true);
        }
    }
}

bool CaveGenerator::is_cave_edge(int x, int y) const {
    // Check 4 cardinal neighbors
    if (!is_cave(x + 1, y)) return true;
    if (!is_cave(x - 1, y)) return true;
    if (!is_cave(x, y + 1)) return true;
    if (!is_cave(x, y - 1)) return true;

    return false;
}

float CaveGenerator::cave_noise(float x, float y) const {
    // Simple 2D noise for caves (will use proper perlin later)
    float n = sin(x * 12.9898f + y * 78.233f + cave_seed) * 43758.5453f;
    n = n - floor(n);
    return (n * 2.0f) - 1.0f;
}

// StructureGenerator implementation
void StructureGenerator::place_structures(StructurePhase phase) {
    for (const auto& structure : structures) {
        if (structure.phase != phase) continue;

        // Calculate how many to place based on world size
        int target_count = static_cast<int>(WORLD_WIDTH / structure.min_spacing * structure.spawn_chance);

        for (int i = 0; i < target_count; i++) {
            Vector2i pos = find_structure_position(structure);
            if (pos.x >= 0) {  // Valid position found
                place_structure(structure, pos);
            }
        }
    }
}

void StructureGenerator::register_structure(const StructureTemplate& structure) {
    structures.push_back(structure);
}

Vector2i StructureGenerator::find_structure_position(const StructureTemplate& structure) {
    // Try random positions until we find a valid one
    for (int attempt = 0; attempt < 100; attempt++) {
        int x = static_cast<int>(rand_float() * WORLD_WIDTH);
        int y = SEA_LEVEL - 100 + static_cast<int>(rand_float() * 200);  // Near surface

        Vector2i pos(x, y);

        if (can_place_structure(structure, pos)) {
            return pos;
        }
    }

    return Vector2i(-1, -1);  // Failed to find position
}

bool StructureGenerator::can_place_structure(const StructureTemplate& structure, Vector2i pos) {
    // Check minimum spacing from other structures
    for (const Vector2i& other_pos : placed_structures) {
        int dx = abs(pos.x - other_pos.x);
        if (dx > WORLD_WIDTH / 2) dx = WORLD_WIDTH - dx;  // Account for wrapping

        int dy = abs(pos.y - other_pos.y);
        int distance = sqrt(dx * dx + dy * dy);

        if (distance < structure.min_spacing) {
            return false;
        }
    }

    // Check biome compatibility
    BiomeType biome = biome_system->get_biome_at(pos.x);
    bool biome_allowed = false;
    for (BiomeType allowed : structure.allowed_biomes) {
        if (allowed == biome) {
            biome_allowed = true;
            break;
        }
    }

    return biome_allowed;
}

void StructureGenerator::place_structure(const StructureTemplate& structure, Vector2i pos) {
    // Add terrain flattening markers if building needs flat ground
    // Max 2 markers per building
    if (structure.needs_flat_ground && structure.phase == PRE_CAVE) {
        // Example: For a building at pos with size (width, height)
        // Place marker at the base center with radius based on width
        int base_y = pos.y + structure.size.y;  // Bottom of structure
        int center_x = pos.x + structure.size.x / 2;
        int flatten_radius = std::max(structure.size.x / 2 + 5, 8);  // At least 8 blocks radius

        add_terrain_marker(Vector2i(center_x, base_y), flatten_radius);

        // Optional: Add second marker for larger buildings
        if (structure.size.x > 20) {
            // Left side marker
            int left_x = pos.x + structure.size.x / 4;
            add_terrain_marker(Vector2i(left_x, base_y), flatten_radius / 2);
        }
    }

    // Place structure blocks
    for (int x = 0; x < structure.size.x; x++) {
        for (int y = 0; y < structure.size.y; y++) {
            Vector2i block_pos = pos + Vector2i(x, y);

            // If post-cave phase, delete existing blocks
            if (structure.phase == POST_CAVE) {
                Block2D air;
                air.type_id = 0;
                chunk_manager->set_block_at_tile(block_pos, air);
            }

            // Place structure block (if template data exists)
            // TODO: Load from template data
            // When implemented, mark blocks with is_structure_block = true in BlockDefinition
        }
    }

    // Create cave doorway if needed
    if (structure.has_doorway && structure.phase == POST_CAVE) {
        create_cave_doorway(pos, structure.size);
    }

    placed_structures.push_back(pos);
}

void StructureGenerator::create_cave_doorway(Vector2i structure_pos, Vector2i structure_size) {
    // Find nearest cave within reasonable distance
    // TODO: Implement cave entrance finding and doorway creation
}

float StructureGenerator::rand_float() {
    structure_seed = (structure_seed * 1103515245 + 12345) & 0x7fffffff;
    return static_cast<float>(structure_seed) / static_cast<float>(0x7fffffff);
}
