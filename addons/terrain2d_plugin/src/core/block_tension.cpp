#include "block_tension.h"
#include <cmath>
#include <algorithm>

using namespace godot;

void BlockTensionSystem::update(float delta_time) {
    // Update falling blocks
    for (auto it = falling_blocks.begin(); it != falling_blocks.end();) {
        FallingBlock& fb = *it;

        // Apply gravity
        fb.velocity.y += GRAVITY * delta_time;

        // Clamp to terminal velocity
        if (fb.velocity.y > MAX_FALL_SPEED) {
            fb.velocity.y = MAX_FALL_SPEED;
        }

        // Move block
        fb.position += fb.velocity * delta_time;

        // Check if block hit ground or went out of bounds
        Vector2i tile_pos = WorldCoords::world_to_tile(fb.position);

        // Check if out of world bounds
        if (!WorldCoords::is_valid_y(tile_pos.y)) {
            // Block fell out of world - remove it
            it = falling_blocks.erase(it);
            continue;
        }

        // Check if hit solid block below
        Vector2i below_pos = Vector2i(tile_pos.x, tile_pos.y + 1);
        const Block2D* below = chunk_manager->get_block_at_tile(below_pos);

        if (below && below->type_id != 0) { // 0 = air
            // Hit something - try to place block
            if (try_place_falling_block(fb)) {
                // Successfully placed
                it = falling_blocks.erase(it);
            } else {
                // Couldn't place - try to move sideways or break
                // For now, just spawn item drop
                spawn_item_drop(fb);
                it = falling_blocks.erase(it);
            }
        } else {
            ++it;
        }
    }
}

bool BlockTensionSystem::is_block_stable(Vector2i tile_pos) {
    const Block2D* block = chunk_manager->get_block_at_tile(tile_pos);
    if (!block || block->type_id == 0) {
        return true; // Air is always stable
    }

    const BlockDefinition* def = block_registry->get_block_definition(block->type_id);
    if (!def) {
        return true; // Unknown block, assume stable
    }

    // Blocks without gravity are always stable
    if (!def->affected_by_gravity) {
        return true;
    }

    // Check background support
    bool has_background_support = false;
    int solid_neighbors = count_solid_neighbors(tile_pos, has_background_support);

    // If block has background support and enough neighbors, it's stable
    if (has_background_support && solid_neighbors >= def->stability_threshold) {
        return true;
    }

    // If no background support, block can fall regardless of neighbors
    if (!has_background_support) {
        return false;
    }

    // Has background but not enough neighbors
    return false;
}

void BlockTensionSystem::queue_stability_check(Vector2i tile_pos) {
    stability_check_queue.push_back(tile_pos);
}

void BlockTensionSystem::process_stability_queue() {
    std::vector<Vector2i> current_queue = std::move(stability_check_queue);
    stability_check_queue.clear();

    for (const Vector2i& pos : current_queue) {
        if (!is_block_stable(pos)) {
            make_block_fall(pos);
        }
    }
}

void BlockTensionSystem::check_neighbors_after_mining(Vector2i mined_pos) {
    // Cardinal directions (top, right, bottom, left)
    const Vector2i cardinals[4] = {
        Vector2i(0, -1),  // Top
        Vector2i(1, 0),   // Right
        Vector2i(0, 1),   // Bottom
        Vector2i(-1, 0)   // Left
    };

    for (const Vector2i& offset : cardinals) {
        Vector2i neighbor_pos = mined_pos + offset;
        const Block2D* neighbor = chunk_manager->get_block_at_tile(neighbor_pos);

        if (!neighbor || neighbor->type_id == 0) {
            continue; // No block here
        }

        const BlockDefinition* def = block_registry->get_block_definition(neighbor->type_id);
        if (!def || !def->affected_by_gravity) {
            continue; // Block doesn't fall
        }

        // Cardinal neighbors have a chance to fall even with background support
        // Check if neighbor has background support
        const Block2D* background = chunk_manager->get_block_at_tile(neighbor_pos, true);
        bool has_background = (background && background->type_id != 0);

        if (has_background) {
            // 30% chance to fall anyway if directly next to mined block
            // This creates the "cascading" effect
            float fall_chance = 0.3f;
            float random_val = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

            if (random_val < fall_chance) {
                make_block_fall(neighbor_pos);
            } else {
                // Still queue for normal stability check
                queue_stability_check(neighbor_pos);
            }
        } else {
            // No background support - definitely check stability
            queue_stability_check(neighbor_pos);
        }
    }

    // Also check diagonal neighbors (they use normal stability rules)
    const Vector2i diagonals[4] = {
        Vector2i(-1, -1), // Top-left
        Vector2i(1, -1),  // Top-right
        Vector2i(-1, 1),  // Bottom-left
        Vector2i(1, 1)    // Bottom-right
    };

    for (const Vector2i& offset : diagonals) {
        Vector2i neighbor_pos = mined_pos + offset;
        queue_stability_check(neighbor_pos);
    }
}

void BlockTensionSystem::make_block_fall(Vector2i tile_pos) {
    const Block2D* block = chunk_manager->get_block_at_tile(tile_pos);
    if (!block || block->type_id == 0) {
        return; // No block to fall
    }

    const BlockDefinition* def = block_registry->get_block_definition(block->type_id);
    if (!def || !def->affected_by_gravity) {
        return; // Block doesn't fall
    }

    // Create falling block entity
    Vector2 world_pos = WorldCoords::tile_to_world(tile_pos);
    FallingBlock fb(world_pos, *block, block->type_id);
    falling_blocks.push_back(fb);

    // Remove block from world
    Block2D air;
    air.type_id = 0;
    chunk_manager->set_block_at_tile(tile_pos, air);

    // Queue neighbors for stability check
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            queue_stability_check(tile_pos + Vector2i(dx, dy));
        }
    }
}

int BlockTensionSystem::count_solid_neighbors(Vector2i tile_pos, bool& has_background_support) {
    // Check background first
    const Block2D* background = chunk_manager->get_block_at_tile(tile_pos, true);
    has_background_support = (background && background->type_id != 0);

    // Count solid neighbors (8 directions)
    int solid_count = 0;
    const Vector2i offsets[8] = {
        Vector2i(-1, -1), Vector2i(0, -1), Vector2i(1, -1),
        Vector2i(-1, 0),                   Vector2i(1, 0),
        Vector2i(-1, 1),  Vector2i(0, 1),  Vector2i(1, 1)
    };

    for (const Vector2i& offset : offsets) {
        Vector2i neighbor_pos = tile_pos + offset;
        if (is_solid_block(neighbor_pos)) {
            solid_count++;
        }
    }

    return solid_count;
}

bool BlockTensionSystem::is_solid_block(Vector2i tile_pos) {
    const Block2D* block = chunk_manager->get_block_at_tile(tile_pos);
    if (!block || block->type_id == 0) {
        return false; // Air is not solid
    }

    // Liquids are not solid for support purposes
    if (block->has_flag(Block2D::IS_LIQUID)) {
        return false;
    }

    // Platforms are not solid for support from above
    if (block->has_flag(Block2D::IS_PLATFORM)) {
        return false;
    }

    return true;
}

bool BlockTensionSystem::can_support(const Block2D* block, const BlockDefinition* def) {
    if (!block || !def) {
        return false;
    }

    // Air can't support
    if (block->type_id == 0) {
        return false;
    }

    // Liquids can't support
    if (block->has_flag(Block2D::IS_LIQUID)) {
        return false;
    }

    // Falling blocks can't support while falling
    if (def->affected_by_gravity) {
        return false;
    }

    return true;
}

bool BlockTensionSystem::try_place_falling_block(const FallingBlock& fb) {
    Vector2i tile_pos = WorldCoords::world_to_tile(fb.position);

    // Check if position is empty
    const Block2D* existing = chunk_manager->get_block_at_tile(tile_pos);
    if (existing && existing->type_id != 0) {
        return false; // Position occupied
    }

    // Get block definition
    const BlockDefinition* def = block_registry->get_block_definition(fb.block_id);
    if (!def) {
        return false;
    }

    // Check if block should break on impact
    if (def->breaks_on_fall && std::abs(fb.velocity.y) > BREAK_VELOCITY) {
        return false; // Will spawn item drop instead
    }

    // Place block
    chunk_manager->set_block_at_tile(tile_pos, fb.block_data);

    return true;
}

void BlockTensionSystem::spawn_item_drop(const FallingBlock& fb) {
    // TODO: Integrate with item drop system
    // For now, just print debug message
    Vector2i tile_pos = WorldCoords::world_to_tile(fb.position);
    // UtilityFunctions::print("Block ", fb.block_id, " broke at ", tile_pos);
}
