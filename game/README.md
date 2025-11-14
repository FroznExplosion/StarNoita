# StarNoita Game Scenes

This directory contains all the Godot scene files and GDScript code for the StarNoita game.

## Directory Structure

```
game/
├── main.tscn                  # Root game scene
├── player/                    # Player-related files
│   ├── player.tscn           # Player character scene
│   └── player_controller.gd  # Player movement and mining logic
├── ui/                        # User interface
│   ├── hud.tscn              # HUD overlay scene
│   └── hud_controller.gd     # HUD update logic
└── world/                     # World/terrain related
    └── terrain_controller.gd # GDScript wrapper for C++ terrain plugin
```

## Scene Overview

### Main Scene (main.tscn)
The root scene of the game. Contains:
- Camera2D: Follows the player with smooth movement
- Terrain2D: The C++ terrain system node
- Player: Player character instance
- HUD: UI overlay (in CanvasLayer)

### Player Scene (player.tscn)
Player character with:
- CharacterBody2D: Physics-based character controller
- Sprite2D: Visual representation (placeholder - needs texture)
- CollisionShape2D: Physics collision
- MiningCursor: 3×3 cursor that follows mouse

**Controls:**
- WASD / Arrow Keys: Move left/right
- Space: Jump
- Left Mouse: Mine blocks (3×3 area + 16 surrounding)
- Right Mouse: Place blocks

### HUD Scene (hud.tscn)
User interface overlay showing:
- Player health bar
- Depth/height display (relative to sea level at Y=8000)
- Current biome
- FPS counter
- Chunks loaded count

## Integration with C++ Plugin

The terrain system is implemented in C++ for performance. The GDScript files interface with the C++ plugin through:

1. **terrain_controller.gd**: Wraps the C++ Terrain2D node
   - Provides GDScript API for gameplay systems
   - Handles chunk loading based on camera position
   - Exposes mining/placement functions

2. **Player Controller**: Calls terrain controller methods
   - `damage_blocks_3x3()` for mining
   - `place_block()` for building
   - `get_block_at()` for collision detection

## World Coordinates

- **World Size**: 1600 tiles wide × 10,000 tiles tall
- **Sea Level**: Y = 8000
- **Tile Size**: 16 pixels
- **Chunk Size**: 32×32 tiles

**Height Reference:**
- Y=10000: Space (top)
- Y=9000: Sky layer
- Y=8000: Sea level
- Y=7000: Underground starts
- Y=0: Bedrock (bottom)

## Damage System

The player's mining cursor damages blocks in a 3×3 pattern:

```
[ 50% ][ 50% ][ 50% ][ 50% ][ 50% ]
[ 50% ][100% ][100% ][100% ][ 50% ]
[ 50% ][100% ][ C  ][100% ][ 50% ]
[ 50% ][100% ][100% ][100% ][ 50% ]
[ 50% ][ 50% ][ 50% ][ 50% ][ 50% ]
```

- **C**: Cursor center position
- **100%**: 3×3 center receives full damage
- **50%**: 16 surrounding blocks receive 50% damage

Damage formula: `ActualDamage = ToolDamage - BlockDamageReduction`

## Next Steps

Before the game is playable:

1. **Compile C++ Plugin**: Follow build instructions in `STARNOITA_IMPLEMENTATION_GUIDE.md`
2. **Add Textures**: Replace placeholder sprites with actual textures
3. **Implement Lighting**: Add lighting system to terrain rendering
4. **Add Sound**: Mining sounds, ambient sounds, music
5. **Inventory System**: Block selection and storage
6. **Save/Load**: World persistence

## Testing Without C++ Plugin

The scenes can be opened in Godot editor, but the terrain system won't function until the C++ plugin is compiled. You'll see warnings in the console:

```
Terrain2D C++ plugin not found! Compile the plugin first.
See STARNOITA_IMPLEMENTATION_GUIDE.md for build instructions.
```
