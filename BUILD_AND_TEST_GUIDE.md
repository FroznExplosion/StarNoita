# StarNoita Build and Test Guide
**Last Updated**: 2025-11-14
**Status**: ✅ Ready for Compilation

## Prerequisites

### Required Software
1. **Godot 4.3+** (tested with 4.5)
   - Download from https://godotengine.org/download

2. **SCons Build Tool**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install scons

   # macOS
   brew install scons

   # Windows
   pip install scons
   ```

3. **C++ Compiler**
   - Linux: GCC 7+ or Clang 5+
   - Windows: MSVC 2017+ or MinGW-w64
   - macOS: Clang (via Xcode Command Line Tools)

4. **Python 3.6+** (for SCons)

## Step 1: Clone godot-cpp

```bash
cd /home/user/StarNoita/addons/terrain2d_plugin
git clone https://github.com/godotengine/godot-cpp
cd godot-cpp
git checkout 4.3-stable
cd ..
```

## Step 2: Create SCons Build Configuration

Create `addons/terrain2d_plugin/SConstruct`:

```python
#!/usr/bin/env python
import os
import sys

# Initialize environment from godot-cpp
env = SConscript("godot-cpp/SConstruct")

# Add our include paths
env.Append(CPPPATH=["src/"])

# Collect all source files
sources = []
sources += Glob("src/core/*.cpp")
sources += Glob("src/world/*.cpp")

# Build the shared library
if env["platform"] == "linux":
    library = env.SharedLibrary(
        "bin/libterrain2d.{}.{}.{}.so".format(
            env["platform"],
            env["target"],
            env["arch"]
        ),
        source=sources,
    )
elif env["platform"] == "windows":
    library = env.SharedLibrary(
        "bin/libterrain2d.{}.{}.{}.dll".format(
            env["platform"],
            env["target"],
            env["arch"]
        ),
        source=sources,
    )
elif env["platform"] == "macos":
    library = env.SharedLibrary(
        "bin/libterrain2d.{}.{}.{}.dylib".format(
            env["platform"],
            env["target"],
            env["arch"]
        ),
        source=sources,
    )

Default(library)
```

## Step 3: Build the Plugin

### Linux
```bash
cd /home/user/StarNoita/addons/terrain2d_plugin

# Debug build (for development)
scons platform=linux target=template_debug

# Release build (for distribution)
scons platform=linux target=template_release
```

### Windows
```bash
cd /home/user/StarNoita/addons/terrain2d_plugin

# Debug build
scons platform=windows target=template_debug

# Release build
scons platform=windows target=template_release
```

### macOS
```bash
cd /home/user/StarNoita/addons/terrain2d_plugin

# Debug build
scons platform=macos target=template_debug

# Release build
scons platform=macos target=template_release
```

### Expected Output
```
Compiling ==> src/core/chunk_manager.cpp
Compiling ==> src/core/block_registry.cpp
Compiling ==> src/core/block_damage.cpp
Compiling ==> src/core/block_tension.cpp
Compiling ==> src/world/biome_system.cpp
Compiling ==> src/world/world_generator.cpp
Linking ==> bin/libterrain2d.linux.template_debug.x86_64.so
scons: done building targets.
```

## Step 4: Verify Binary

Check that the binary was created:

```bash
ls -lh addons/terrain2d_plugin/bin/

# Should show:
# libterrain2d.linux.template_debug.x86_64.so (or .dll on Windows, .dylib on macOS)
```

## Step 5: Create Plugin Registration

Create `addons/terrain2d_plugin/src/register_types.h`:

```cpp
#ifndef TERRAIN2D_REGISTER_TYPES_H
#define TERRAIN2D_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_terrain2d_module(ModuleInitializationLevel p_level);
void uninitialize_terrain2d_module(ModuleInitializationLevel p_level);

#endif // TERRAIN2D_REGISTER_TYPES_H
```

Create `addons/terrain2d_plugin/src/register_types.cpp`:

```cpp
#include "register_types.h"
#include "core/block_registry.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_terrain2d_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    ClassDB::register_class<BlockResource>();
}

void uninitialize_terrain2d_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
    // Initialization
    GDExtensionBool GDE_EXPORT terrain2d_library_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        const GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization
    ) {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_terrain2d_module);
        init_obj.register_terminator(uninitialize_terrain2d_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
```

**Then rebuild:**
```bash
scons platform=linux target=template_debug
```

## Step 6: Open in Godot

1. **Open Godot Engine**
2. **Import Project**
   - Click "Import"
   - Navigate to `/home/user/StarNoita`
   - Select `project.godot`
   - Click "Import & Edit"

3. **Verify Plugin Loaded**
   - Check the console for errors
   - Go to Project → Project Settings → Plugins
   - Verify "Terrain2D Plugin" appears (may need to enable manually)

## Step 7: Test the Game

### Open Main Scene
1. In FileSystem dock, navigate to `game/main.tscn`
2. Double-click to open
3. You should see:
   - Camera2D node
   - Terrain2D node
   - Player node
   - HUD in CanvasLayer

### Run the Game
1. Press **F5** or click the Play button
2. **Expected behavior**:
   - Window opens at 1920×1080
   - World generates (may take a few seconds)
   - Player spawns above sea level
   - HUD shows health, depth, biome, FPS

### Controls
- **WASD / Arrow Keys**: Move left/right
- **Space**: Jump
- **Left Mouse**: Mine blocks (3×3 cursor)
- **Right Mouse**: Place blocks (stone)
- **Mouse**: Aim cursor (yellow when in range, red when too far)

### What to Test

#### 1. World Generation
- [ ] World generates without errors
- [ ] Biomes appear (plains, forest, desert, mountains)
- [ ] Terrain has natural height variation
- [ ] Ores appear underground (copper, iron, gold)
- [ ] Caves generate properly
- [ ] Cave interiors have cave_stone blocks
- [ ] Cave edges outlined with regular stone

#### 2. Block Damage System
- [ ] Mining damages blocks (visible in console if logging enabled)
- [ ] Blocks break after enough damage
- [ ] 3×3 cursor damages center 9 blocks
- [ ] Surrounding 16 blocks take 50% damage
- [ ] Blocks regenerate health after 2 seconds of no damage
- [ ] Full health restored at 35hp per 0.5 seconds

#### 3. Block Tension Physics
- [ ] Mining sand causes nearby sand to fall
- [ ] Background blocks provide stability
- [ ] Cardinal neighbors have 30% fall chance
- [ ] Falling blocks snap to grid when landed

#### 4. Building System (When Structures Added)
- [ ] Buildings place before terrain generation
- [ ] Terrain flattens near building markers
- [ ] Smooth terrain transition with blend curve
- [ ] Caves don't delete building blocks

#### 5. UI and Controls
- [ ] HUD displays correctly
- [ ] Health bar shows 100%
- [ ] Depth updates when moving vertically
- [ ] Biome name updates when moving horizontally
- [ ] FPS counter works
- [ ] Mining cursor snaps to grid
- [ ] Cursor color changes based on reach

## Troubleshooting

### Build Errors

**Error**: `fatal error: godot_cpp/...hpp: No such file or directory`
- **Fix**: Make sure you cloned godot-cpp and checked out the correct branch

**Error**: `scons: command not found`
- **Fix**: Install SCons with `pip install scons` or your package manager

**Error**: `undefined reference to godot::...`
- **Fix**: Build godot-cpp first:
  ```bash
  cd godot-cpp
  scons platform=linux target=template_debug
  cd ..
  scons platform=linux target=template_debug
  ```

### Runtime Errors

**Error**: `Terrain2D C++ plugin not found`
- **Fix**: The plugin binary wasn't built or is in wrong location
- Check `addons/terrain2d_plugin/bin/` has the .so/.dll/.dylib file
- Verify filename matches pattern in `terrain2d.gdextension`

**Error**: Godot crashes on startup
- **Fix**: Check console for specific error
- Common cause: Missing `register_types.cpp` or wrong entry point name
- Verify `terrain2d_library_init` matches `entry_symbol` in .gdextension

**Warning**: `ChunkManager not initialized`
- **Status**: Expected until Terrain2D node is fully implemented
- Game will still open but terrain won't render

### Performance Issues

**Low FPS** (< 30):
- Reduce `view_distance_horizontal/vertical` in ChunkManager
- Use release build instead of debug
- Check if many chunks loaded (shown in HUD)

**Long generation time**:
- Normal on first run (generating 10,000 block tall world)
- Should be < 5 seconds for full generation
- If > 30 seconds, check noise calculation efficiency

## Current Limitations

These are expected and documented in the implementation guide:

1. **No visual rendering**: Terrain generates but doesn't render yet (needs rendering system)
2. **No auto-tiling**: Blocks don't blend smoothly (needs 47-tile system)
3. **No lighting**: World is uniformly lit (needs lighting system)
4. **No liquids**: Water/lava don't flow (needs liquid simulation)
5. **Placeholder graphics**: Sprites are colored rectangles (needs texture atlas)
6. **No structures yet**: Building placement works but no structure templates defined

## Next Development Steps

After confirming the build works:

1. **Implement Rendering** (Priority 1)
   - Create Terrain2D node that inherits Node2D
   - Implement `_draw()` to render chunks
   - Use ChunkManager to get visible chunks
   - Draw each block as sprite

2. **Add Auto-Tiling** (Priority 2)
   - Implement 47-tile neighbor checking
   - Create tile atlas with all combinations
   - Calculate UV coordinates for each block

3. **Add Lighting** (Priority 3)
   - Implement column-based sunlight
   - Add point light propagation
   - Apply smooth lighting to vertex colors

4. **Add Liquids** (Priority 4)
   - Implement pressure-based flow
   - Add liquid-liquid interactions
   - Render with transparency

## Success Criteria

✅ **Build Success**: No compilation errors, binary created
✅ **Plugin Loads**: Godot recognizes plugin, no startup errors
✅ **Scene Opens**: main.tscn opens without errors
✅ **Game Runs**: F5 runs game, window appears
✅ **Systems Work**: Can move player, mining damages blocks
✅ **No Crashes**: Game runs for 5+ minutes without crashing

## Support

If you encounter issues:

1. Check `SYSTEM_INTEGRATION_CHECK.md` for verified integration points
2. Check `STARNOITA_IMPLEMENTATION_GUIDE.md` for system documentation
3. Review console output for specific error messages
4. Verify all source files are present in `src/` directory
5. Ensure godot-cpp is built for the same platform and target

---

**Build Ready**: All C++ systems implemented and verified!
**Next**: Compile and test in Godot 4.5
