#ifndef WORLD_CONSTANTS_H
#define WORLD_CONSTANTS_H

#include <godot_cpp/variant/vector2i.hpp>

using namespace godot;

// World dimensions (1600 x 10000 blocks)
constexpr int WORLD_WIDTH = 1600;          // Blocks wide (wraps horizontally)
constexpr int WORLD_HEIGHT = 10000;        // Blocks tall (0 = bedrock, 10000 = space)
constexpr int SEA_LEVEL = 8000;            // Sea level in absolute coordinates
constexpr int BEDROCK_LEVEL = 0;           // Bottom of world

// Layer absolute heights (0 = bedrock, 10000 = top of space)
constexpr int LAYER_SPACE_TOP = 10000;     // Top of space layer
constexpr int LAYER_SPACE_BOTTOM = 9000;   // Bottom of space (1000 blocks)
constexpr int LAYER_SKY_BOTTOM = 7000;     // Bottom of sky (2000 blocks)
constexpr int LAYER_SURFACE_TOP = 8100;    // Top of surface (+100 from sea)
constexpr int LAYER_SURFACE_BOTTOM = 7900; // Bottom of surface (-100 from sea)
constexpr int LAYER_UNDERGROUND_BOTTOM = 3000; // Bottom of underground (4900 blocks)
constexpr int LAYER_UNDERWORLD_BOTTOM = 2000;  // Bottom of underworld (1000 blocks)
constexpr int LAYER_DEEP_WORLD_BOTTOM = 0;     // Bottom of deep world (2000 blocks)

// Chunk dimensions
constexpr int CHUNK_WIDTH_BLOCKS = 32;
constexpr int CHUNK_HEIGHT_BLOCKS = 32;
constexpr int CHUNKS_HORIZONTAL = WORLD_WIDTH / CHUNK_WIDTH_BLOCKS;   // 50 chunks
constexpr int CHUNKS_VERTICAL = WORLD_HEIGHT / CHUNK_HEIGHT_BLOCKS;   // 313 chunks (rounded up)

// Tile rendering
constexpr int TILE_SIZE_PIXELS = 16;       // Each block is 16x16 pixels

// Coordinate conversion utilities
namespace WorldCoords {
    // Convert world position to tile position
    inline Vector2i world_to_tile(Vector2 world_pos) {
        return Vector2i(
            static_cast<int>(floor(world_pos.x / TILE_SIZE_PIXELS)),
            static_cast<int>(floor(world_pos.y / TILE_SIZE_PIXELS))
        );
    }

    // Convert tile position to world position (center of tile)
    inline Vector2 tile_to_world(Vector2i tile_pos) {
        return Vector2(
            tile_pos.x * TILE_SIZE_PIXELS + TILE_SIZE_PIXELS / 2.0f,
            tile_pos.y * TILE_SIZE_PIXELS + TILE_SIZE_PIXELS / 2.0f
        );
    }

    // Convert tile position to chunk position
    inline Vector2i tile_to_chunk(Vector2i tile_pos) {
        return Vector2i(
            static_cast<int>(floor(static_cast<float>(tile_pos.x) / CHUNK_WIDTH_BLOCKS)),
            static_cast<int>(floor(static_cast<float>(tile_pos.y) / CHUNK_HEIGHT_BLOCKS))
        );
    }

    // Convert tile position to local position within chunk
    inline Vector2i tile_to_local(Vector2i tile_pos) {
        int local_x = tile_pos.x % CHUNK_WIDTH_BLOCKS;
        int local_y = tile_pos.y % CHUNK_HEIGHT_BLOCKS;

        // Handle negative modulo
        if (local_x < 0) local_x += CHUNK_WIDTH_BLOCKS;
        if (local_y < 0) local_y += CHUNK_HEIGHT_BLOCKS;

        return Vector2i(local_x, local_y);
    }

    // Convert chunk position and local position to tile position
    inline Vector2i chunk_local_to_tile(Vector2i chunk_pos, Vector2i local_pos) {
        return Vector2i(
            chunk_pos.x * CHUNK_WIDTH_BLOCKS + local_pos.x,
            chunk_pos.y * CHUNK_HEIGHT_BLOCKS + local_pos.y
        );
    }

    // Wrap X coordinate for horizontal wrapping
    inline int wrap_x(int x) {
        while (x < 0) x += WORLD_WIDTH;
        return x % WORLD_WIDTH;
    }

    // Wrap tile X coordinate
    inline Vector2i wrap_tile_x(Vector2i tile_pos) {
        return Vector2i(wrap_x(tile_pos.x), tile_pos.y);
    }

    // Check if tile Y is within world bounds
    inline bool is_valid_y(int y) {
        return y >= 0 && y < WORLD_HEIGHT;
    }

    // Check if tile position is valid
    inline bool is_valid_tile(Vector2i tile_pos) {
        return is_valid_y(tile_pos.y); // X always wraps, only check Y
    }

    // Convert absolute Y to display Y (relative to sea level)
    // Sea level (8000) becomes 0, above is positive, below is negative
    inline int absolute_to_display_y(int absolute_y) {
        return absolute_y - SEA_LEVEL;
    }

    // Convert display Y to absolute Y
    inline int display_to_absolute_y(int display_y) {
        return display_y + SEA_LEVEL;
    }

    // Get world layer from absolute Y coordinate
    inline WorldLayer get_layer_at_y(int y) {
        if (y >= LAYER_SPACE_BOTTOM) return SPACE;
        if (y >= LAYER_SKY_BOTTOM) return SKY;
        if (y >= LAYER_SURFACE_BOTTOM && y <= LAYER_SURFACE_TOP) return SURFACE;
        if (y >= LAYER_UNDERGROUND_BOTTOM) return UNDERGROUND;
        if (y >= LAYER_UNDERWORLD_BOTTOM) return UNDERWORLD;
        return DEEP_WORLD;
    }
}

// Physics constants
constexpr float GRAVITY = 980.0f;           // Pixels per second squared
constexpr float MAX_FALL_SPEED = 1000.0f;   // Max falling velocity
constexpr float BREAK_VELOCITY = 500.0f;    // Velocity at which fragile blocks break

// Liquid constants
constexpr float MIN_LIQUID_LEVEL = 0.01f;   // Below this, liquid is removed
constexpr float MAX_LIQUID_LEVEL = 1.0f;    // Normal max level
constexpr float MAX_LIQUID_PRESSURE = 2.0f; // Can exceed normal max under pressure
constexpr float LIQUID_FLOW_RATE = 0.5f;    // Blocks per second

// Lighting constants
constexpr uint8_t MAX_LIGHT_LEVEL = 255;
constexpr uint8_t MIN_LIGHT_LEVEL = 0;
constexpr uint8_t AMBIENT_LIGHT_UNDERGROUND = 20;
constexpr uint8_t AMBIENT_LIGHT_SURFACE = 100;

#endif // WORLD_CONSTANTS_H
