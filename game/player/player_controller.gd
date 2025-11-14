extends CharacterBody2D

# Player properties
@export var speed: float = 200.0
@export var jump_velocity: float = -400.0
@export var mining_reach: float = 5.0  # tiles
@export var mining_damage: float = 25.0  # damage per click
@export var tool_damage_reduction: float = 0.0  # tool ignores this much block DR

# Gravity
var gravity: float = 980.0

# Mining cursor
@onready var cursor: Node2D = $MiningCursor
@onready var cursor_sprite: Sprite2D = $MiningCursor/CursorSprite

# Terrain reference (set by Main scene)
var terrain_manager: Node = null

# Constants
const TILE_SIZE: int = 16
const CURSOR_SIZE: int = 3  # 3x3 cursor

func _ready():
	# Find Terrain2D node
	terrain_manager = get_node_or_null("/root/Main/Terrain2D")
	if not terrain_manager:
		push_warning("Terrain2D node not found!")

func _physics_process(delta: float):
	# Apply gravity
	if not is_on_floor():
		velocity.y += gravity * delta

	# Handle jump
	if Input.is_action_just_pressed("ui_accept") and is_on_floor():
		velocity.y = jump_velocity

	# Get input direction
	var direction := Input.get_axis("ui_left", "ui_right")
	if direction:
		velocity.x = direction * speed
	else:
		velocity.x = move_toward(velocity.x, 0, speed)

	# Move character
	move_and_slide()

	# Update camera position
	emit_signal("position_changed", global_position)

func _process(_delta: float):
	# Update cursor position based on mouse
	update_cursor_position()

	# Handle mining
	if Input.is_action_just_pressed("mine"):  # Left click
		mine_at_cursor()

	# Handle placing blocks
	if Input.is_action_just_pressed("place"):  # Right click
		place_at_cursor()

func update_cursor_position():
	# Get mouse position in world coordinates
	var mouse_world_pos := get_global_mouse_position()

	# Convert to tile coordinates
	var tile_x := floori(mouse_world_pos.x / TILE_SIZE)
	var tile_y := floori(mouse_world_pos.y / TILE_SIZE)

	# Check if in reach
	var distance := global_position.distance_to(Vector2(tile_x * TILE_SIZE, tile_y * TILE_SIZE))
	var in_reach := distance <= (mining_reach * TILE_SIZE)

	# Snap cursor to tile grid (center of 3x3 area)
	var cursor_world_pos := Vector2(
		tile_x * TILE_SIZE + TILE_SIZE / 2.0,
		tile_y * TILE_SIZE + TILE_SIZE / 2.0
	)
	cursor.global_position = cursor_world_pos

	# Change cursor color based on reach
	if in_reach:
		cursor_sprite.modulate = Color(1, 1, 0, 0.5)  # Yellow semi-transparent
	else:
		cursor_sprite.modulate = Color(1, 0, 0, 0.3)  # Red semi-transparent

func mine_at_cursor():
	if not terrain_manager:
		return

	# Get cursor tile position
	var tile_x := floori(cursor.global_position.x / TILE_SIZE)
	var tile_y := floori(cursor.global_position.y / TILE_SIZE)

	# Check reach
	var distance := global_position.distance_to(cursor.global_position)
	if distance > (mining_reach * TILE_SIZE):
		return  # Out of reach

	# Mine 3x3 area (center tile is the cursor position)
	# This will call the C++ BlockDamageSystem.damage_3x3_area()
	if terrain_manager.has_method("damage_blocks_3x3"):
		terrain_manager.damage_blocks_3x3(
			Vector2i(tile_x, tile_y),
			mining_damage,
			tool_damage_reduction
		)
	else:
		# Fallback: damage tiles directly (for testing without C++ plugin)
		for x in range(-1, 2):  # -1, 0, 1
			for y in range(-1, 2):  # -1, 0, 1
				var target_tile := Vector2i(tile_x + x, tile_y + y)
				if terrain_manager.has_method("damage_block"):
					terrain_manager.damage_block(target_tile, mining_damage)

func place_at_cursor():
	if not terrain_manager:
		return

	# Get cursor tile position
	var tile_x := floori(cursor.global_position.x / TILE_SIZE)
	var tile_y := floori(cursor.global_position.y / TILE_SIZE)

	# Check reach
	var distance := global_position.distance_to(cursor.global_position)
	if distance > (mining_reach * TILE_SIZE):
		return  # Out of reach

	# Place block (for now, just place stone = ID 1)
	if terrain_manager.has_method("place_block"):
		terrain_manager.place_block(Vector2i(tile_x, tile_y), 1)  # stone

# Signal for camera to follow
signal position_changed(new_position)
