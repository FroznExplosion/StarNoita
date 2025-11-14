extends Node2D

# This script wraps the Terrain2D C++ GDExtension node
# It provides a GDScript interface for the game to interact with the terrain system

# References to C++ systems (will be set when plugin loads)
var chunk_manager = null
var block_registry = null
var block_damage_system = null
var world_generator = null
var biome_system = null

# World settings
const WORLD_WIDTH: int = 1600
const WORLD_HEIGHT: int = 10000
const SEA_LEVEL: int = 8000
const CHUNK_SIZE: int = 32
const TILE_SIZE: int = 16

# Generation settings
@export var world_seed: int = 12345
@export var auto_generate: bool = true

func _ready():
	# Initialize C++ plugin systems
	# Note: This will fail until the C++ plugin is compiled
	# For now, this serves as documentation of the interface
	if Engine.has_singleton("Terrain2DPlugin"):
		initialize_cpp_systems()
		if auto_generate:
			generate_world()
	else:
		push_warning("Terrain2D C++ plugin not found! Compile the plugin first.")
		push_warning("See STARNOITA_IMPLEMENTATION_GUIDE.md for build instructions.")

func initialize_cpp_systems():
	# Get C++ singletons/systems
	var plugin = Engine.get_singleton("Terrain2DPlugin")
	if plugin:
		chunk_manager = plugin.get_chunk_manager()
		block_registry = plugin.get_block_registry()
		block_damage_system = plugin.get_block_damage_system()
		world_generator = plugin.get_world_generator()
		biome_system = plugin.get_biome_system()

		# Set world seed
		if world_generator:
			world_generator.set_seed(world_seed)

func generate_world():
	if not world_generator:
		push_error("WorldGenerator not initialized!")
		return

	print("Generating world with seed: %d" % world_seed)
	world_generator.generate_world()
	print("World generation complete!")

func _process(_delta: float):
	# Update active chunks based on camera position
	if chunk_manager:
		var camera = get_viewport().get_camera_2d()
		if camera:
			chunk_manager.update_active_chunks(camera.global_position)

# Public API for player/gameplay systems

func damage_blocks_3x3(center_tile: Vector2i, damage: float, tool_dr: float = 0.0):
	"""Damage a 3x3 area of blocks + 16 surrounding blocks"""
	if not block_damage_system:
		return

	# This calls C++ BlockDamageSystem::damage_3x3_area()
	block_damage_system.damage_3x3_area(center_tile, damage, tool_dr)

func damage_block(tile_pos: Vector2i, damage: float):
	"""Damage a single block"""
	if not chunk_manager:
		return

	chunk_manager.damage_block(tile_pos, damage)

func place_block(tile_pos: Vector2i, block_id: int, is_background: bool = false):
	"""Place a block at the given tile position"""
	if not chunk_manager:
		return

	chunk_manager.set_block_at_tile(tile_pos, block_id, is_background)

func get_block_at(tile_pos: Vector2i, is_background: bool = false) -> int:
	"""Get the block ID at the given tile position"""
	if not chunk_manager:
		return 0  # Air

	return chunk_manager.get_block_id_at_tile(tile_pos, is_background)

func get_biome_at_position(world_x: int) -> String:
	"""Get the biome name at the given X coordinate"""
	if not biome_system:
		return "Unknown"

	var biome_type: int = biome_system.get_biome_at(world_x)
	var biome_def = biome_system.get_biome_definition(biome_type)

	if biome_def:
		return biome_def.name
	return "Unknown"

func get_active_chunk_count() -> int:
	"""Get the number of currently loaded chunks"""
	if not chunk_manager:
		return 0

	return chunk_manager.get_active_chunk_count()

# Rendering
func _draw():
	# Render visible chunks
	# This will be handled by C++ rendering system
	# For now, this is a placeholder
	pass
