#ifndef BLOCK_DAMAGE_H
#define BLOCK_DAMAGE_H

#include "../world/block_data.h"
#include "chunk_manager.h"
#include "block_registry.h"
#include "block_tension.h"
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/rect2i.hpp>
#include <vector>

using namespace godot;

// Tool definition for damage calculation
struct Tool {
    float damage;              // Raw damage output
    int tier;                  // Tool tier (0-10)
    float mining_speed;        // Mining speed multiplier

    Tool() : damage(10.0f), tier(0), mining_speed(1.0f) {}
    Tool(float dmg, int t, float speed) : damage(dmg), tier(t), mining_speed(speed) {}
};

// Result of a block damage operation
struct DamageResult {
    bool block_destroyed;          // Was the block destroyed?
    uint16_t destroyed_block_id;   // ID of destroyed block
    Vector2i destroyed_pos;        // Position of destroyed block
    float overkill_damage;         // Damage beyond what was needed

    DamageResult() : block_destroyed(false), destroyed_block_id(0), overkill_damage(0.0f) {}
};

// Block regeneration tracker
struct BlockRegeneration {
    float last_damage_time;     // Time when block was last damaged
    float next_regen_time;      // Time for next regeneration tick

    BlockRegeneration() : last_damage_time(0.0f), next_regen_time(0.0f) {}
};

class BlockDamageSystem {
private:
    ChunkManager* chunk_manager;
    BlockRegistry* block_registry;
    BlockTensionSystem* tension_system;

    // Track damaged blocks for regeneration
    std::unordered_map<Vector2i, BlockRegeneration, Vector2iHash> regeneration_tracker;
    float current_time;  // Track game time

public:
    BlockDamageSystem(ChunkManager* chunks, BlockRegistry* registry, BlockTensionSystem* tension)
        : chunk_manager(chunks)
        , block_registry(registry)
        , tension_system(tension)
        , current_time(0.0f)
    {}

    // Damage a single block
    // Returns true if block was destroyed
    DamageResult damage_block(Vector2i tile_pos, float raw_damage, const Tool& tool, bool is_background = false);

    // Damage blocks in a 3x3 area (for player cursor)
    // Also damages the 16 surrounding blocks with 50% of applied damage
    std::vector<DamageResult> damage_3x3_area(Vector2i center_pos, float raw_damage, const Tool& tool);

    // Calculate actual damage after reduction
    float calculate_actual_damage(float raw_damage, float damage_reduction) const;

    // Check if tool can mine block
    bool can_mine_block(const Tool& tool, const BlockDefinition* block_def) const;

    // Destroy a block instantly
    void destroy_block(Vector2i tile_pos, bool is_background = false);

    // Restore block to full health
    void restore_block_health(Vector2i tile_pos);

    // Update regeneration system (call every frame)
    // Regenerates blocks after 2 seconds: 35 health per 0.5 seconds
    void update_regeneration(float delta_time);

private:
    // Apply damage and check if block should be destroyed
    bool apply_damage_to_block(Vector2i tile_pos, float damage, bool is_background, DamageResult& result);

    // Handle block destruction
    void handle_block_destruction(Vector2i tile_pos, bool is_background, DamageResult& result);

    // Spawn item drop when block is destroyed
    void spawn_item_drop(Vector2i tile_pos, uint16_t block_id);
};

#endif // BLOCK_DAMAGE_H
