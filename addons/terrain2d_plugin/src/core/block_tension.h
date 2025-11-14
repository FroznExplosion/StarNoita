#ifndef BLOCK_TENSION_H
#define BLOCK_TENSION_H

#include "../world/block_data.h"
#include "../world/world_constants.h"
#include "chunk_manager.h"
#include "block_registry.h"
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <vector>
#include <cstdint>

using namespace godot;

// Falling block entity (for blocks that have lost support)
struct FallingBlock {
    Vector2 position;        // World position (pixels)
    Vector2 velocity;        // Velocity (pixels/second)
    Block2D block_data;      // Block type and properties
    uint16_t block_id;       // Original block type ID

    FallingBlock(Vector2 pos, Block2D data, uint16_t id)
        : position(pos)
        , velocity(Vector2(0, 0))
        , block_data(data)
        , block_id(id)
    {}
};

class BlockTensionSystem {
private:
    ChunkManager* chunk_manager;
    BlockRegistry* block_registry;

    // Active falling blocks
    std::vector<FallingBlock> falling_blocks;

    // Blocks queued for stability check
    std::vector<Vector2i> stability_check_queue;

public:
    BlockTensionSystem(ChunkManager* chunks, BlockRegistry* registry)
        : chunk_manager(chunks)
        , block_registry(registry)
    {}

    // Update physics for falling blocks
    void update(float delta_time);

    // Check if block at position is stable
    bool is_block_stable(Vector2i tile_pos);

    // Queue block for stability check
    void queue_stability_check(Vector2i tile_pos);

    // Process all queued stability checks
    void process_stability_queue();

    // Check support after block is mined/destroyed
    // This checks cardinal neighbors for potential falling
    void check_neighbors_after_mining(Vector2i mined_pos);

    // Convert block to falling entity
    void make_block_fall(Vector2i tile_pos);

    // Get number of active falling blocks
    size_t get_falling_block_count() const { return falling_blocks.size(); }

    // Clear all falling blocks
    void clear_falling_blocks() { falling_blocks.clear(); }

private:
    // Count solid neighbors (including background)
    int count_solid_neighbors(Vector2i tile_pos, bool& has_background_support);

    // Check if block at position is solid
    bool is_solid_block(Vector2i tile_pos);

    // Check if block can provide support
    bool can_support(const Block2D* block, const BlockDefinition* def);

    // Try to place falling block at position
    bool try_place_falling_block(const FallingBlock& fb);

    // Spawn item drop from falling block
    void spawn_item_drop(const FallingBlock& fb);
};

#endif // BLOCK_TENSION_H
