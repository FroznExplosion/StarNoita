#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include "../world/chunk_2d.h"
#include "../world/world_constants.h"
#include "../world/block_data.h"
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace godot;

class ChunkManager {
private:
    // Active chunks stored by chunk position
    std::unordered_map<Vector2i, std::unique_ptr<Chunk2D>, Vector2iHash> chunks;

    // Chunks queued for generation
    std::vector<Vector2i> generation_queue;

    // View distance in chunks
    int view_distance_horizontal = 8;  // Chunks left/right of camera
    int view_distance_vertical = 6;    // Chunks up/down of camera

    // Last camera chunk position (for detecting movement)
    Vector2i last_camera_chunk;

public:
    ChunkManager() : last_camera_chunk(Vector2i(-9999, -9999)) {}
    ~ChunkManager() = default;

    // Update active chunks based on camera position
    void update_active_chunks(Vector2 camera_world_pos);

    // Get chunk at chunk coordinates (wraps X, returns nullptr if Y out of bounds)
    Chunk2D* get_chunk(Vector2i chunk_pos);
    const Chunk2D* get_chunk(Vector2i chunk_pos) const;

    // Load or generate chunk
    Chunk2D* load_chunk(Vector2i chunk_pos);

    // Unload distant chunks
    void unload_distant_chunks(Vector2i center_chunk);

    // Block access by tile coordinates (handles wrapping and chunk lookup)
    Block2D* get_block_at_tile(Vector2i tile_pos, bool is_background = false);
    const Block2D* get_block_at_tile(Vector2i tile_pos, bool is_background = false) const;

    // Set block at tile coordinates
    void set_block_at_tile(Vector2i tile_pos, const Block2D& block, bool is_background = false);

    // Get/set block health
    BlockHealth* get_block_health(Vector2i tile_pos);
    void set_block_health(Vector2i tile_pos, float health, float max_health = 100.0f);
    void damage_block(Vector2i tile_pos, float damage, float max_health = 100.0f);

    // Liquid access
    Chunk2D::LiquidCell* get_liquid_at_tile(Vector2i tile_pos);
    void set_liquid_at_tile(Vector2i tile_pos, LiquidType type, float level);

    // Get all active chunks
    const std::unordered_map<Vector2i, std::unique_ptr<Chunk2D>, Vector2iHash>& get_all_chunks() const {
        return chunks;
    }

    // Check if chunk exists
    bool has_chunk(Vector2i chunk_pos) const;

    // Get number of loaded chunks
    size_t get_loaded_chunk_count() const { return chunks.size(); }

    // Get memory usage
    size_t get_total_memory_usage() const;

    // Clear all chunks
    void clear_all();

private:
    // Wrap chunk X coordinate for horizontal wrapping
    Vector2i wrap_chunk_pos(Vector2i chunk_pos) const;

    // Check if chunk Y coordinate is valid
    bool is_valid_chunk_y(int chunk_y) const;
};

#endif // CHUNK_MANAGER_H
