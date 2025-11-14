#ifndef CHUNK_2D_H
#define CHUNK_2D_H

#include "block_data.h"
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <unordered_map>
#include <array>

using namespace godot;

// Hash function for Vector2i (for unordered_map)
struct Vector2iHash {
    std::size_t operator()(const Vector2i& v) const {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
    }
};

// Chunk constants
constexpr int CHUNK_WIDTH = 32;
constexpr int CHUNK_HEIGHT = 32;
constexpr int CHUNK_SIZE = CHUNK_WIDTH * CHUNK_HEIGHT;

class Chunk2D {
public:
    // Chunk data arrays (separate for cache efficiency)
    std::array<std::array<Block2D, CHUNK_HEIGHT>, CHUNK_WIDTH> foreground;
    std::array<std::array<Block2D, CHUNK_HEIGHT>, CHUNK_WIDTH> background;
    std::array<std::array<uint8_t, CHUNK_HEIGHT>, CHUNK_WIDTH> lighting;

    // Liquid data (stored separately, sparse)
    struct LiquidCell {
        LiquidType type;
        float level;        // 0.0 to 1.0+ (can exceed for pressure)

        LiquidCell() : type(LIQUID_NONE), level(0.0f) {}
        LiquidCell(LiquidType t, float l) : type(t), level(l) {}
    };
    std::unordered_map<Vector2i, LiquidCell, Vector2iHash> liquids;

    // Block health (sparse - only damaged blocks stored)
    std::unordered_map<Vector2i, BlockHealth, Vector2iHash> block_health;

    // Chunk metadata
    Vector2i chunk_position;    // Position in chunk coordinates
    bool is_generated;          // Has been generated
    bool dirty_mesh;            // Needs mesh rebuild
    bool dirty_lighting;        // Needs lighting recalc
    bool dirty_background;      // Background needs update

    Chunk2D(Vector2i pos)
        : chunk_position(pos)
        , is_generated(false)
        , dirty_mesh(true)
        , dirty_lighting(true)
        , dirty_background(true)
    {
        // Initialize all blocks to air (type 0)
        for (int x = 0; x < CHUNK_WIDTH; x++) {
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                foreground[x][y] = Block2D();
                background[x][y] = Block2D();
                lighting[x][y] = 0;
            }
        }
    }

    // Block access with bounds checking
    inline Block2D* get_block(Vector2i local_pos, bool is_background = false) {
        if (local_pos.x < 0 || local_pos.x >= CHUNK_WIDTH ||
            local_pos.y < 0 || local_pos.y >= CHUNK_HEIGHT) {
            return nullptr;
        }
        return is_background ? &background[local_pos.x][local_pos.y]
                             : &foreground[local_pos.x][local_pos.y];
    }

    inline const Block2D* get_block(Vector2i local_pos, bool is_background = false) const {
        if (local_pos.x < 0 || local_pos.x >= CHUNK_WIDTH ||
            local_pos.y < 0 || local_pos.y >= CHUNK_HEIGHT) {
            return nullptr;
        }
        return is_background ? &background[local_pos.x][local_pos.y]
                             : &foreground[local_pos.x][local_pos.y];
    }

    // Set block and mark dirty
    inline void set_block(Vector2i local_pos, const Block2D& block, bool is_background = false) {
        if (local_pos.x < 0 || local_pos.x >= CHUNK_WIDTH ||
            local_pos.y < 0 || local_pos.y >= CHUNK_HEIGHT) {
            return;
        }

        if (is_background) {
            background[local_pos.x][local_pos.y] = block;
            dirty_background = true;
        } else {
            foreground[local_pos.x][local_pos.y] = block;
            dirty_mesh = true;
        }
        dirty_lighting = true;
    }

    // Block health management
    inline BlockHealth* get_health(Vector2i local_pos) {
        auto it = block_health.find(local_pos);
        if (it != block_health.end()) {
            return &it->second;
        }
        return nullptr;
    }

    inline void set_health(Vector2i local_pos, float health, float max_health = 100.0f) {
        if (health >= max_health) {
            // Full health - remove from sparse map
            block_health.erase(local_pos);
        } else {
            block_health[local_pos] = BlockHealth(max_health);
            block_health[local_pos].current_health = health;
        }
    }

    inline void damage_block(Vector2i local_pos, float damage, float max_health = 100.0f) {
        auto it = block_health.find(local_pos);
        if (it != block_health.end()) {
            it->second.current_health -= damage;
        } else {
            // First damage - create health entry
            block_health[local_pos] = BlockHealth(max_health);
            block_health[local_pos].current_health = max_health - damage;
        }
    }

    // Liquid management
    inline LiquidCell* get_liquid(Vector2i local_pos) {
        auto it = liquids.find(local_pos);
        if (it != liquids.end()) {
            return &it->second;
        }
        return nullptr;
    }

    inline void set_liquid(Vector2i local_pos, LiquidType type, float level) {
        if (level <= 0.0f || type == LIQUID_NONE) {
            liquids.erase(local_pos);
        } else {
            liquids[local_pos] = LiquidCell(type, level);
        }
        dirty_mesh = true;
    }

    // Lighting access
    inline uint8_t get_light(Vector2i local_pos) const {
        if (local_pos.x < 0 || local_pos.x >= CHUNK_WIDTH ||
            local_pos.y < 0 || local_pos.y >= CHUNK_HEIGHT) {
            return 0;
        }
        return lighting[local_pos.x][local_pos.y];
    }

    inline void set_light(Vector2i local_pos, uint8_t light_level) {
        if (local_pos.x < 0 || local_pos.x >= CHUNK_WIDTH ||
            local_pos.y < 0 || local_pos.y >= CHUNK_HEIGHT) {
            return;
        }
        lighting[local_pos.x][local_pos.y] = light_level;
    }

    // Clear chunk data
    void clear() {
        for (int x = 0; x < CHUNK_WIDTH; x++) {
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                foreground[x][y] = Block2D();
                background[x][y] = Block2D();
                lighting[x][y] = 0;
            }
        }
        liquids.clear();
        block_health.clear();
        is_generated = false;
        dirty_mesh = true;
        dirty_lighting = true;
        dirty_background = true;
    }

    // Memory usage estimation
    size_t get_memory_usage() const {
        size_t base = sizeof(Chunk2D);
        size_t liquid_mem = liquids.size() * (sizeof(Vector2i) + sizeof(LiquidCell));
        size_t health_mem = block_health.size() * (sizeof(Vector2i) + sizeof(BlockHealth));
        return base + liquid_mem + health_mem;
    }
};

#endif // CHUNK_2D_H
