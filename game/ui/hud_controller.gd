extends Control

# References to UI elements
@onready var health_label: Label = $TopLeftPanel/HealthLabel
@onready var health_bar: ProgressBar = $TopLeftPanel/HealthBar
@onready var depth_label: Label = $TopLeftPanel/DepthLabel
@onready var biome_label: Label = $TopLeftPanel/BiomeLabel
@onready var fps_label: Label = $TopRightPanel/FPSLabel
@onready var chunks_label: Label = $TopRightPanel/ChunksLabel

# Player reference
var player: Node = null
var terrain_manager: Node = null

# Constants for depth calculation
const SEA_LEVEL: int = 8000
const TILE_SIZE: int = 16

func _ready():
	# Find player and terrain
	await get_tree().process_frame  # Wait one frame for scene to load
	player = get_node_or_null("/root/Main/Player")
	terrain_manager = get_node_or_null("/root/Main/Terrain2D")

func _process(_delta: float):
	# Update FPS
	fps_label.text = "FPS: %d" % Engine.get_frames_per_second()

	# Update depth based on player position
	if player:
		update_depth_display()
		update_biome_display()

	# Update chunks loaded count
	if terrain_manager and terrain_manager.has_method("get_active_chunk_count"):
		var chunk_count: int = terrain_manager.get_active_chunk_count()
		chunks_label.text = "Chunks Loaded: %d" % chunk_count
	else:
		chunks_label.text = "Chunks Loaded: N/A"

	# Update health (placeholder - will be connected to player health system)
	update_health_display(100, 100)

func update_health_display(current_health: float, max_health: float):
	health_label.text = "Health: %.0f / %.0f" % [current_health, max_health]
	health_bar.max_value = max_health
	health_bar.value = current_health

	# Color health bar based on percentage
	var health_percent := current_health / max_health
	if health_percent > 0.6:
		health_bar.modulate = Color(0, 1, 0)  # Green
	elif health_percent > 0.3:
		health_bar.modulate = Color(1, 1, 0)  # Yellow
	else:
		health_bar.modulate = Color(1, 0, 0)  # Red

func update_depth_display():
	if not player:
		return

	# Convert player Y position to tile coordinates
	var player_tile_y: int = floori(player.global_position.y / TILE_SIZE)

	# Calculate depth relative to sea level
	var depth_from_sea: int = SEA_LEVEL - player_tile_y
	var meters: float = abs(depth_from_sea)

	# Determine display text
	var depth_text: String
	if player_tile_y > SEA_LEVEL:
		# Below sea level (underground)
		depth_text = "Depth: %.0fm Underground" % meters
	elif player_tile_y < SEA_LEVEL:
		# Above sea level (sky)
		depth_text = "Height: %.0fm Above Sea" % meters
	else:
		# At sea level
		depth_text = "Depth: Sea Level"

	depth_label.text = depth_text

func update_biome_display():
	if not player or not terrain_manager:
		biome_label.text = "Biome: Unknown"
		return

	# Get biome at player position
	if terrain_manager.has_method("get_biome_at_position"):
		var player_tile_x: int = floori(player.global_position.x / TILE_SIZE)
		var biome_name: String = terrain_manager.get_biome_at_position(player_tile_x)
		biome_label.text = "Biome: %s" % biome_name
	else:
		biome_label.text = "Biome: N/A"

# Called by player when health changes
func on_player_health_changed(current: float, maximum: float):
	update_health_display(current, maximum)
