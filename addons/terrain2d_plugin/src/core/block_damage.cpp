#include "block_damage.h"
#include <algorithm>

using namespace godot;

DamageResult BlockDamageSystem::damage_block(Vector2i tile_pos, float raw_damage, const Tool& tool, bool is_background) {
    DamageResult result;
    result.destroyed_pos = tile_pos;

    const Block2D* block = chunk_manager->get_block_at_tile(tile_pos, is_background);
    if (!block || block->type_id == 0) {
        return result; // No block to damage
    }

    const BlockDefinition* def = block_registry->get_block_definition(block->type_id);
    if (!def) {
        return result; // Unknown block
    }

    // Check if tool can mine this block
    if (!can_mine_block(tool, def)) {
        return result; // Tool not strong enough
    }

    // Calculate actual damage after reduction
    float actual_damage = calculate_actual_damage(raw_damage, def->damage_reduction);

    // Apply damage
    apply_damage_to_block(tile_pos, actual_damage, is_background, result);

    return result;
}

std::vector<DamageResult> BlockDamageSystem::damage_3x3_area(Vector2i center_pos, float raw_damage, const Tool& tool) {
    std::vector<DamageResult> results;

    // Define 3x3 area (center block + 8 surrounding)
    const Vector2i main_area[9] = {
        Vector2i(-1, -1), Vector2i(0, -1), Vector2i(1, -1),
        Vector2i(-1,  0), Vector2i(0,  0), Vector2i(1,  0),
        Vector2i(-1,  1), Vector2i(0,  1), Vector2i(1,  1)
    };

    // Define 16 surrounding blocks (outer ring)
    const Vector2i surrounding[16] = {
        // Top row
        Vector2i(-2, -2), Vector2i(-1, -2), Vector2i(0, -2), Vector2i(1, -2), Vector2i(2, -2),
        // Middle rows (left and right)
        Vector2i(-2, -1), Vector2i(2, -1),
        Vector2i(-2,  0), Vector2i(2,  0),
        Vector2i(-2,  1), Vector2i(2,  1),
        // Bottom row
        Vector2i(-2,  2), Vector2i(-1,  2), Vector2i(0,  2), Vector2i(1,  2), Vector2i(2,  2)
    };

    // Damage the 3x3 main area with full damage
    for (const Vector2i& offset : main_area) {
        Vector2i target_pos = center_pos + offset;
        DamageResult result = damage_block(target_pos, raw_damage, tool, false);
        if (result.block_destroyed) {
            results.push_back(result);
        }
    }

    // Damage the 16 surrounding blocks with 50% of the APPLIED damage (after reduction)
    for (const Vector2i& offset : surrounding) {
        Vector2i target_pos = center_pos + offset;

        const Block2D* block = chunk_manager->get_block_at_tile(target_pos);
        if (!block || block->type_id == 0) {
            continue; // No block here
        }

        const BlockDefinition* def = block_registry->get_block_definition(block->type_id);
        if (!def) {
            continue;
        }

        // Calculate actual damage for main hit
        float actual_main_damage = calculate_actual_damage(raw_damage, def->damage_reduction);

        // Surrounding blocks take 50% of the APPLIED damage
        float surrounding_damage = actual_main_damage * 0.5f;

        // Apply the surrounding damage (no reduction applied again since it's already been reduced)
        DamageResult result;
        result.destroyed_pos = target_pos;

        // Directly apply the surrounding damage
        if (apply_damage_to_block(target_pos, surrounding_damage, false, result)) {
            results.push_back(result);

            // Damaged blocks near mined blocks can fall even with background support
            const Block2D* background = chunk_manager->get_block_at_tile(target_pos, true);
            if (background && background->type_id != 0) {
                // Has background but still chance to fall if damaged
                if (def->affected_by_gravity) {
                    tension_system->queue_stability_check(target_pos);
                }
            }
        }
    }

    return results;
}

float BlockDamageSystem::calculate_actual_damage(float raw_damage, float damage_reduction) const {
    float actual_damage = raw_damage - damage_reduction;
    return std::max(0.0f, actual_damage);  // Never negative
}

bool BlockDamageSystem::can_mine_block(const Tool& tool, const BlockDefinition* block_def) const {
    if (!block_def) {
        return false;
    }

    // Check if tool tier is high enough
    return tool.tier >= block_def->required_tool_tier;
}

void BlockDamageSystem::destroy_block(Vector2i tile_pos, bool is_background) {
    const Block2D* block = chunk_manager->get_block_at_tile(tile_pos, is_background);
    if (!block || block->type_id == 0) {
        return; // No block to destroy
    }

    uint16_t block_id = block->type_id;

    // Set to air
    Block2D air;
    air.type_id = 0;
    chunk_manager->set_block_at_tile(tile_pos, air, is_background);

    // Remove health data
    chunk_manager->set_block_health(tile_pos, 100.0f, 100.0f);

    // Spawn item drop
    if (!is_background) {
        spawn_item_drop(tile_pos, block_id);

        // Check stability of neighbors
        tension_system->check_neighbors_after_mining(tile_pos);
    }
}

void BlockDamageSystem::restore_block_health(Vector2i tile_pos) {
    const Block2D* block = chunk_manager->get_block_at_tile(tile_pos);
    if (!block || block->type_id == 0) {
        return;
    }

    const BlockDefinition* def = block_registry->get_block_definition(block->type_id);
    if (!def) {
        return;
    }

    // Restore to full health (removes from sparse health map)
    chunk_manager->set_block_health(tile_pos, def->max_health, def->max_health);
}

bool BlockDamageSystem::apply_damage_to_block(Vector2i tile_pos, float damage, bool is_background, DamageResult& result) {
    if (damage <= 0.0f) {
        return false; // No damage applied
    }

    const Block2D* block = chunk_manager->get_block_at_tile(tile_pos, is_background);
    if (!block || block->type_id == 0) {
        return false;
    }

    const BlockDefinition* def = block_registry->get_block_definition(block->type_id);
    if (!def) {
        return false;
    }

    // Get current health (default to max if not damaged yet)
    BlockHealth* health = chunk_manager->get_block_health(tile_pos);
    float current_health = health ? health->current_health : def->max_health;

    // Apply damage
    current_health -= damage;

    // Check if block is destroyed
    if (current_health <= 0.0f) {
        result.overkill_damage = -current_health; // How much extra damage was dealt
        handle_block_destruction(tile_pos, is_background, result);
        // Remove from regeneration tracker
        regeneration_tracker.erase(tile_pos);
        return true;
    } else {
        // Update health
        chunk_manager->set_block_health(tile_pos, current_health, def->max_health);

        // Track for regeneration
        BlockRegeneration& regen = regeneration_tracker[tile_pos];
        regen.last_damage_time = current_time;
        regen.next_regen_time = current_time + 2.0f; // 2 second delay before regen starts

        return false;
    }
}

void BlockDamageSystem::update_regeneration(float delta_time) {
    current_time += delta_time;

    std::vector<Vector2i> to_remove;

    for (auto& [pos, regen] : regeneration_tracker) {
        // Check if 2 second delay has passed
        if (current_time < regen.next_regen_time) {
            continue; // Still waiting
        }

        // Regenerate 35 health per 0.5 seconds
        const Block2D* block = chunk_manager->get_block_at_tile(pos);
        if (!block || block->type_id == 0) {
            to_remove.push_back(pos);
            continue;
        }

        const BlockDefinition* def = block_registry->get_block_definition(block->type_id);
        if (!def) {
            to_remove.push_back(pos);
            continue;
        }

        // Get current health
        BlockHealth* health = chunk_manager->get_block_health(pos);
        if (!health) {
            // Fully healed already
            to_remove.push_back(pos);
            continue;
        }

        // Regenerate health
        float new_health = health->current_health + 35.0f;

        if (new_health >= def->max_health) {
            // Fully healed
            chunk_manager->set_block_health(pos, def->max_health, def->max_health);
            to_remove.push_back(pos);
        } else {
            // Partially healed
            chunk_manager->set_block_health(pos, new_health, def->max_health);
            // Schedule next regen tick in 0.5 seconds
            regen.next_regen_time = current_time + 0.5f;
        }
    }

    // Remove fully healed blocks from tracker
    for (const auto& pos : to_remove) {
        regeneration_tracker.erase(pos);
    }
}

void BlockDamageSystem::handle_block_destruction(Vector2i tile_pos, bool is_background, DamageResult& result) {
    const Block2D* block = chunk_manager->get_block_at_tile(tile_pos, is_background);
    if (!block || block->type_id == 0) {
        return;
    }

    result.block_destroyed = true;
    result.destroyed_block_id = block->type_id;
    result.destroyed_pos = tile_pos;

    // Destroy the block
    destroy_block(tile_pos, is_background);
}

void BlockDamageSystem::spawn_item_drop(Vector2i tile_pos, uint16_t block_id) {
    // TODO: Integrate with item system
    // For now, just placeholder
    // Vector2 world_pos = WorldCoords::tile_to_world(tile_pos);
    // ItemDrop item;
    // item.item_id = block_id;
    // item.position = world_pos;
    // item_system->spawn_item(item);
}
