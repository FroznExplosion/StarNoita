#include "block_registry.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

BlockRegistry* BlockRegistry::singleton = nullptr;

// BlockResource implementation
BlockResource::BlockResource() {
    definition = BlockDefinition();
}

void BlockResource::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_block_id", "id"), &BlockResource::set_block_id);
    ClassDB::bind_method(D_METHOD("get_block_id"), &BlockResource::get_block_id);

    ClassDB::bind_method(D_METHOD("set_block_name", "name"), &BlockResource::set_block_name);
    ClassDB::bind_method(D_METHOD("get_block_name"), &BlockResource::get_block_name);

    ClassDB::bind_method(D_METHOD("set_max_health", "health"), &BlockResource::set_max_health);
    ClassDB::bind_method(D_METHOD("get_max_health"), &BlockResource::get_max_health);

    ClassDB::bind_method(D_METHOD("set_damage_reduction", "reduction"), &BlockResource::set_damage_reduction);
    ClassDB::bind_method(D_METHOD("get_damage_reduction"), &BlockResource::get_damage_reduction);

    ClassDB::bind_method(D_METHOD("set_stability_threshold", "threshold"), &BlockResource::set_stability_threshold);
    ClassDB::bind_method(D_METHOD("get_stability_threshold"), &BlockResource::get_stability_threshold);

    ClassDB::bind_method(D_METHOD("set_affected_by_gravity", "value"), &BlockResource::set_affected_by_gravity);
    ClassDB::bind_method(D_METHOD("get_affected_by_gravity"), &BlockResource::get_affected_by_gravity);

    ClassDB::bind_method(D_METHOD("set_light_opacity", "opacity"), &BlockResource::set_light_opacity);
    ClassDB::bind_method(D_METHOD("get_light_opacity"), &BlockResource::get_light_opacity);

    ClassDB::bind_method(D_METHOD("set_light_emission", "emission"), &BlockResource::set_light_emission);
    ClassDB::bind_method(D_METHOD("get_light_emission"), &BlockResource::get_light_emission);

    ClassDB::bind_method(D_METHOD("set_is_ore", "value"), &BlockResource::set_is_ore);
    ClassDB::bind_method(D_METHOD("get_is_ore"), &BlockResource::get_is_ore);

    ClassDB::bind_method(D_METHOD("set_can_be_background", "value"), &BlockResource::set_can_be_background);
    ClassDB::bind_method(D_METHOD("get_can_be_background"), &BlockResource::get_can_be_background);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "block_id"), "set_block_id", "get_block_id");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "block_name"), "set_block_name", "get_block_name");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_health"), "set_max_health", "get_max_health");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "damage_reduction"), "set_damage_reduction", "get_damage_reduction");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "stability_threshold"), "set_stability_threshold", "get_stability_threshold");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "affected_by_gravity"), "set_affected_by_gravity", "get_affected_by_gravity");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "light_opacity"), "set_light_opacity", "get_light_opacity");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "light_emission"), "set_light_emission", "get_light_emission");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_ore"), "set_is_ore", "get_is_ore");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "can_be_background"), "set_can_be_background", "get_can_be_background");
}

// BlockRegistry implementation
BlockRegistry::BlockRegistry() {
    singleton = this;
}

BlockRegistry* BlockRegistry::get_singleton() {
    return singleton;
}

void BlockRegistry::register_block(const BlockDefinition& def) {
    blocks[def.id] = def;
    name_to_id[def.name] = def.id;
}

void BlockRegistry::register_block_resource(Ref<BlockResource> resource) {
    if (resource.is_valid()) {
        register_block(resource->get_definition());
    }
}

const BlockDefinition* BlockRegistry::get_block_definition(uint16_t id) const {
    auto it = blocks.find(id);
    if (it != blocks.end()) {
        return &it->second;
    }
    return nullptr;
}

uint16_t BlockRegistry::get_block_id(const String& name) const {
    auto it = name_to_id.find(name);
    if (it != name_to_id.end()) {
        return it->second;
    }
    return 0; // Return AIR if not found
}

bool BlockRegistry::has_block(uint16_t id) const {
    return blocks.find(id) != blocks.end();
}

void BlockRegistry::clear() {
    blocks.clear();
    name_to_id.clear();
    next_id = 1;
}

void BlockRegistry::initialize_default_blocks() {
    // AIR (ID 0) - always empty
    BlockDefinition air;
    air.id = 0;
    air.name = "air";
    air.light_opacity = 0;
    air.max_health = 0;
    register_block(air);

    // STONE (ID 1) - basic solid block
    BlockDefinition stone;
    stone.id = 1;
    stone.name = "stone";
    stone.max_health = 100;
    stone.damage_reduction = 80;  // Reduces 80 damage
    stone.light_opacity = 255;    // Fully opaque
    stone.affected_by_gravity = false;
    stone.stability_threshold = 0;
    stone.use_autotile = true;
    stone.can_be_background = true;
    stone.background_variant_id = 10; // Different texture for background
    register_block(stone);

    // DIRT (ID 2)
    BlockDefinition dirt;
    dirt.id = 2;
    dirt.name = "dirt";
    dirt.max_health = 100;
    dirt.damage_reduction = 20;
    dirt.light_opacity = 255;
    dirt.affected_by_gravity = false;
    dirt.use_autotile = true;
    dirt.grows_plants = true;
    dirt.can_be_background = true;
    register_block(dirt);

    // SAND (ID 3) - affected by gravity
    BlockDefinition sand;
    sand.id = 3;
    sand.name = "sand";
    sand.max_health = 100;
    sand.damage_reduction = 10;
    sand.light_opacity = 255;
    sand.affected_by_gravity = true;
    sand.stability_threshold = 2;  // Needs 2+ neighbors to be stable
    sand.can_be_background = true;
    register_block(sand);

    // GRAVEL (ID 4) - affected by gravity
    BlockDefinition gravel;
    gravel.id = 4;
    gravel.name = "gravel";
    gravel.max_health = 100;
    gravel.damage_reduction = 15;
    gravel.light_opacity = 255;
    gravel.affected_by_gravity = true;
    gravel.stability_threshold = 1;  // Needs 1+ neighbor to be stable
    gravel.can_be_background = true;
    register_block(gravel);

    // GRASS (ID 5)
    BlockDefinition grass;
    grass.id = 5;
    grass.name = "grass";
    grass.max_health = 100;
    grass.damage_reduction = 20;
    grass.light_opacity = 255;
    grass.use_autotile = true;
    grass.can_be_background = true;
    register_block(grass);

    // COPPER ORE (ID 6)
    BlockDefinition copper;
    copper.id = 6;
    copper.name = "copper_ore";
    copper.max_health = 100;
    copper.damage_reduction = 50;
    copper.light_opacity = 255;
    copper.is_ore = true;
    copper.can_be_background = true;
    copper.background_ore_priority = true;  // Ores stay in background in caves
    register_block(copper);

    // IRON ORE (ID 7)
    BlockDefinition iron;
    iron.id = 7;
    iron.name = "iron_ore";
    iron.max_health = 100;
    iron.damage_reduction = 60;
    iron.light_opacity = 255;
    iron.is_ore = true;
    iron.can_be_background = true;
    iron.background_ore_priority = true;
    register_block(iron);

    // GOLD ORE (ID 8)
    BlockDefinition gold;
    gold.id = 8;
    gold.name = "gold_ore";
    gold.max_health = 100;
    gold.damage_reduction = 70;
    gold.light_opacity = 255;
    gold.is_ore = true;
    gold.can_be_background = true;
    gold.background_ore_priority = true;
    register_block(gold);

    // TORCH (ID 9) - emits light
    BlockDefinition torch;
    torch.id = 9;
    torch.name = "torch";
    torch.max_health = 100;
    torch.damage_reduction = 0;
    torch.light_opacity = 0;
    torch.light_emission = 255;
    torch.light_color = Color(1.0, 0.9, 0.7);  // Warm light
    torch.size = Vector2i(1, 1);
    torch.breaks_on_fall = true;
    register_block(torch);

    // CAVE STONE (ID 10) - inside caves, drops regular stone
    BlockDefinition cave_stone;
    cave_stone.id = 10;
    cave_stone.name = "cave_stone";
    cave_stone.max_health = 100;
    cave_stone.damage_reduction = 80;  // Same as stone
    cave_stone.light_opacity = 255;
    cave_stone.affected_by_gravity = false;
    cave_stone.use_autotile = true;
    cave_stone.can_be_background = false;  // Only in foreground inside caves
    register_block(cave_stone);

    // More cave variants for different biomes will be added:
    // MOSSY_STONE (ID 11) - for swamp biome
    BlockDefinition mossy_stone;
    mossy_stone.id = 11;
    mossy_stone.name = "mossy_stone";
    mossy_stone.max_health = 100;
    mossy_stone.damage_reduction = 80;
    mossy_stone.light_opacity = 255;
    mossy_stone.use_autotile = true;
    register_block(mossy_stone);

    // MOSSY_CAVE_STONE (ID 12) - cave variant of mossy stone
    BlockDefinition mossy_cave_stone;
    mossy_cave_stone.id = 12;
    mossy_cave_stone.name = "mossy_cave_stone";
    mossy_cave_stone.max_health = 100;
    mossy_cave_stone.damage_reduction = 80;
    mossy_cave_stone.light_opacity = 255;
    mossy_cave_stone.use_autotile = true;
    register_block(mossy_cave_stone);
}
