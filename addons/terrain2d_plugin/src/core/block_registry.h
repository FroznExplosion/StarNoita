#ifndef BLOCK_REGISTRY_H
#define BLOCK_REGISTRY_H

#include "../world/block_data.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <unordered_map>
#include <vector>

using namespace godot;

// GDScript-accessible block resource
class BlockResource : public Resource {
    GDCLASS(BlockResource, Resource)

private:
    BlockDefinition definition;

protected:
    static void _bind_methods();

public:
    BlockResource();
    ~BlockResource() = default;

    // Getters and setters for GDScript
    void set_block_id(int id) { definition.id = static_cast<uint16_t>(id); }
    int get_block_id() const { return definition.id; }

    void set_block_name(const String& name) { definition.name = name; }
    String get_block_name() const { return definition.name; }

    void set_max_health(float health) { definition.max_health = health; }
    float get_max_health() const { return definition.max_health; }

    void set_damage_reduction(float reduction) { definition.damage_reduction = reduction; }
    float get_damage_reduction() const { return definition.damage_reduction; }

    void set_stability_threshold(int threshold) { definition.stability_threshold = threshold; }
    int get_stability_threshold() const { return definition.stability_threshold; }

    void set_affected_by_gravity(bool value) { definition.affected_by_gravity = value; }
    bool get_affected_by_gravity() const { return definition.affected_by_gravity; }

    void set_light_opacity(int opacity) { definition.light_opacity = static_cast<uint8_t>(opacity); }
    int get_light_opacity() const { return definition.light_opacity; }

    void set_light_emission(int emission) { definition.light_emission = static_cast<uint8_t>(emission); }
    int get_light_emission() const { return definition.light_emission; }

    void set_is_ore(bool value) { definition.is_ore = value; }
    bool get_is_ore() const { return definition.is_ore; }

    void set_can_be_background(bool value) { definition.can_be_background = value; }
    bool get_can_be_background() const { return definition.can_be_background; }

    // Get the full definition
    const BlockDefinition& get_definition() const { return definition; }
    BlockDefinition& get_definition_mut() { return definition; }
};

// Block registry singleton
class BlockRegistry {
private:
    static BlockRegistry* singleton;

    // Block definitions by ID
    std::unordered_map<uint16_t, BlockDefinition> blocks;

    // Block ID by name lookup
    std::unordered_map<String, uint16_t> name_to_id;

    // Next auto-assigned ID
    uint16_t next_id = 1; // 0 is reserved for AIR

public:
    BlockRegistry();
    ~BlockRegistry() = default;

    static BlockRegistry* get_singleton();

    // Register a block definition
    void register_block(const BlockDefinition& def);

    // Register block from GDScript resource
    void register_block_resource(Ref<BlockResource> resource);

    // Get block definition by ID
    const BlockDefinition* get_block_definition(uint16_t id) const;

    // Get block ID by name
    uint16_t get_block_id(const String& name) const;

    // Check if block ID exists
    bool has_block(uint16_t id) const;

    // Get all registered blocks
    const std::unordered_map<uint16_t, BlockDefinition>& get_all_blocks() const {
        return blocks;
    }

    // Clear all blocks
    void clear();

    // Initialize default blocks (air, stone, dirt, etc.)
    void initialize_default_blocks();
};

#endif // BLOCK_REGISTRY_H
