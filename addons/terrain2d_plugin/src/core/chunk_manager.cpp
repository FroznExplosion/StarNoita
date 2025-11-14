#include "chunk_manager.h"
#include <algorithm>

using namespace godot;

Vector2i ChunkManager::wrap_chunk_pos(Vector2i chunk_pos) const {
    int wrapped_x = chunk_pos.x;
    while (wrapped_x < 0) wrapped_x += CHUNKS_HORIZONTAL;
    wrapped_x = wrapped_x % CHUNKS_HORIZONTAL;
    return Vector2i(wrapped_x, chunk_pos.y);
}

bool ChunkManager::is_valid_chunk_y(int chunk_y) const {
    return chunk_y >= 0 && chunk_y < CHUNKS_VERTICAL;
}

void ChunkManager::update_active_chunks(Vector2 camera_world_pos) {
    // Convert camera position to chunk coordinates
    Vector2i camera_tile = WorldCoords::world_to_tile(camera_world_pos);
    Vector2i camera_chunk = WorldCoords::tile_to_chunk(camera_tile);

    // Check if camera moved to different chunk
    if (camera_chunk == last_camera_chunk) {
        return; // No update needed
    }
    last_camera_chunk = camera_chunk;

    // Calculate chunk range to keep loaded
    int min_chunk_x = camera_chunk.x - view_distance_horizontal;
    int max_chunk_x = camera_chunk.x + view_distance_horizontal;
    int min_chunk_y = std::max(0, camera_chunk.y - view_distance_vertical);
    int max_chunk_y = std::min(CHUNKS_VERTICAL - 1, camera_chunk.y + view_distance_vertical);

    // Load chunks in view range
    for (int cx = min_chunk_x; cx <= max_chunk_x; cx++) {
        for (int cy = min_chunk_y; cy <= max_chunk_y; cy++) {
            Vector2i chunk_pos(cx, cy);
            Vector2i wrapped_pos = wrap_chunk_pos(chunk_pos);

            if (!has_chunk(wrapped_pos)) {
                load_chunk(wrapped_pos);
            }
        }
    }

    // Unload distant chunks
    unload_distant_chunks(camera_chunk);
}

Chunk2D* ChunkManager::get_chunk(Vector2i chunk_pos) {
    Vector2i wrapped_pos = wrap_chunk_pos(chunk_pos);

    if (!is_valid_chunk_y(wrapped_pos.y)) {
        return nullptr;
    }

    auto it = chunks.find(wrapped_pos);
    if (it != chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

const Chunk2D* ChunkManager::get_chunk(Vector2i chunk_pos) const {
    Vector2i wrapped_pos = wrap_chunk_pos(chunk_pos);

    if (!is_valid_chunk_y(wrapped_pos.y)) {
        return nullptr;
    }

    auto it = chunks.find(wrapped_pos);
    if (it != chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

Chunk2D* ChunkManager::load_chunk(Vector2i chunk_pos) {
    Vector2i wrapped_pos = wrap_chunk_pos(chunk_pos);

    if (!is_valid_chunk_y(wrapped_pos.y)) {
        return nullptr;
    }

    // Check if already loaded
    auto it = chunks.find(wrapped_pos);
    if (it != chunks.end()) {
        return it->second.get();
    }

    // Create new chunk
    auto chunk = std::make_unique<Chunk2D>(wrapped_pos);

    // TODO: Try to load from disk first
    // TODO: If not on disk, generate chunk

    // For now, just mark as needing generation
    if (!chunk->is_generated) {
        generation_queue.push_back(wrapped_pos);
    }

    Chunk2D* chunk_ptr = chunk.get();
    chunks[wrapped_pos] = std::move(chunk);

    return chunk_ptr;
}

void ChunkManager::unload_distant_chunks(Vector2i center_chunk) {
    // Calculate unload distance (slightly larger than view distance)
    int unload_dist_h = view_distance_horizontal + 2;
    int unload_dist_v = view_distance_vertical + 2;

    // Find chunks to unload
    std::vector<Vector2i> to_unload;

    for (const auto& [chunk_pos, chunk] : chunks) {
        // Calculate distance from center (accounting for wrapping)
        int dx = std::abs(chunk_pos.x - center_chunk.x);
        // Account for wrapping
        if (dx > CHUNKS_HORIZONTAL / 2) {
            dx = CHUNKS_HORIZONTAL - dx;
        }

        int dy = std::abs(chunk_pos.y - center_chunk.y);

        // Check if too far
        if (dx > unload_dist_h || dy > unload_dist_v) {
            to_unload.push_back(chunk_pos);
        }
    }

    // Unload chunks
    for (const auto& pos : to_unload) {
        // TODO: Save to disk before unloading if modified
        chunks.erase(pos);
    }
}

Block2D* ChunkManager::get_block_at_tile(Vector2i tile_pos, bool is_background) {
    // Wrap X coordinate
    tile_pos = WorldCoords::wrap_tile_x(tile_pos);

    // Check Y bounds
    if (!WorldCoords::is_valid_y(tile_pos.y)) {
        return nullptr;
    }

    // Get chunk
    Vector2i chunk_pos = WorldCoords::tile_to_chunk(tile_pos);
    Chunk2D* chunk = get_chunk(chunk_pos);
    if (!chunk) {
        return nullptr;
    }

    // Get local position in chunk
    Vector2i local_pos = WorldCoords::tile_to_local(tile_pos);
    return chunk->get_block(local_pos, is_background);
}

const Block2D* ChunkManager::get_block_at_tile(Vector2i tile_pos, bool is_background) const {
    // Wrap X coordinate
    tile_pos = WorldCoords::wrap_tile_x(tile_pos);

    // Check Y bounds
    if (!WorldCoords::is_valid_y(tile_pos.y)) {
        return nullptr;
    }

    // Get chunk
    Vector2i chunk_pos = WorldCoords::tile_to_chunk(tile_pos);
    const Chunk2D* chunk = get_chunk(chunk_pos);
    if (!chunk) {
        return nullptr;
    }

    // Get local position in chunk
    Vector2i local_pos = WorldCoords::tile_to_local(tile_pos);
    return chunk->get_block(local_pos, is_background);
}

void ChunkManager::set_block_at_tile(Vector2i tile_pos, const Block2D& block, bool is_background) {
    // Wrap X coordinate
    tile_pos = WorldCoords::wrap_tile_x(tile_pos);

    // Check Y bounds
    if (!WorldCoords::is_valid_y(tile_pos.y)) {
        return;
    }

    // Get or load chunk
    Vector2i chunk_pos = WorldCoords::tile_to_chunk(tile_pos);
    Chunk2D* chunk = get_chunk(chunk_pos);
    if (!chunk) {
        chunk = load_chunk(chunk_pos);
    }
    if (!chunk) {
        return;
    }

    // Set block
    Vector2i local_pos = WorldCoords::tile_to_local(tile_pos);
    chunk->set_block(local_pos, block, is_background);
}

BlockHealth* ChunkManager::get_block_health(Vector2i tile_pos) {
    tile_pos = WorldCoords::wrap_tile_x(tile_pos);
    if (!WorldCoords::is_valid_y(tile_pos.y)) {
        return nullptr;
    }

    Vector2i chunk_pos = WorldCoords::tile_to_chunk(tile_pos);
    Chunk2D* chunk = get_chunk(chunk_pos);
    if (!chunk) {
        return nullptr;
    }

    Vector2i local_pos = WorldCoords::tile_to_local(tile_pos);
    return chunk->get_health(local_pos);
}

void ChunkManager::set_block_health(Vector2i tile_pos, float health, float max_health) {
    tile_pos = WorldCoords::wrap_tile_x(tile_pos);
    if (!WorldCoords::is_valid_y(tile_pos.y)) {
        return;
    }

    Vector2i chunk_pos = WorldCoords::tile_to_chunk(tile_pos);
    Chunk2D* chunk = get_chunk(chunk_pos);
    if (!chunk) {
        chunk = load_chunk(chunk_pos);
    }
    if (!chunk) {
        return;
    }

    Vector2i local_pos = WorldCoords::tile_to_local(tile_pos);
    chunk->set_health(local_pos, health, max_health);
}

void ChunkManager::damage_block(Vector2i tile_pos, float damage, float max_health) {
    tile_pos = WorldCoords::wrap_tile_x(tile_pos);
    if (!WorldCoords::is_valid_y(tile_pos.y)) {
        return;
    }

    Vector2i chunk_pos = WorldCoords::tile_to_chunk(tile_pos);
    Chunk2D* chunk = get_chunk(chunk_pos);
    if (!chunk) {
        chunk = load_chunk(chunk_pos);
    }
    if (!chunk) {
        return;
    }

    Vector2i local_pos = WorldCoords::tile_to_local(tile_pos);
    chunk->damage_block(local_pos, damage, max_health);
}

Chunk2D::LiquidCell* ChunkManager::get_liquid_at_tile(Vector2i tile_pos) {
    tile_pos = WorldCoords::wrap_tile_x(tile_pos);
    if (!WorldCoords::is_valid_y(tile_pos.y)) {
        return nullptr;
    }

    Vector2i chunk_pos = WorldCoords::tile_to_chunk(tile_pos);
    Chunk2D* chunk = get_chunk(chunk_pos);
    if (!chunk) {
        return nullptr;
    }

    Vector2i local_pos = WorldCoords::tile_to_local(tile_pos);
    return chunk->get_liquid(local_pos);
}

void ChunkManager::set_liquid_at_tile(Vector2i tile_pos, LiquidType type, float level) {
    tile_pos = WorldCoords::wrap_tile_x(tile_pos);
    if (!WorldCoords::is_valid_y(tile_pos.y)) {
        return;
    }

    Vector2i chunk_pos = WorldCoords::tile_to_chunk(tile_pos);
    Chunk2D* chunk = get_chunk(chunk_pos);
    if (!chunk) {
        chunk = load_chunk(chunk_pos);
    }
    if (!chunk) {
        return;
    }

    Vector2i local_pos = WorldCoords::tile_to_local(tile_pos);
    chunk->set_liquid(local_pos, type, level);
}

bool ChunkManager::has_chunk(Vector2i chunk_pos) const {
    Vector2i wrapped_pos = wrap_chunk_pos(chunk_pos);
    if (!is_valid_chunk_y(wrapped_pos.y)) {
        return false;
    }
    return chunks.find(wrapped_pos) != chunks.end();
}

size_t ChunkManager::get_total_memory_usage() const {
    size_t total = 0;
    for (const auto& [pos, chunk] : chunks) {
        total += chunk->get_memory_usage();
    }
    return total;
}

void ChunkManager::clear_all() {
    chunks.clear();
    generation_queue.clear();
    last_camera_chunk = Vector2i(-9999, -9999);
}
