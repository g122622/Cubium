# Debug Mode Implementation

This document describes the debug mode (调试模式) implementation, which is used to visualize all block states for resource pack development and debugging.

## Overview

Debug mode generates a flat world where every block state is displayed once in a grid pattern:
- Y=60: Barrier block layer (基座层)
- Y=70: Block state grid (方块状态网格)

The world is in spectator mode, with time frozen at noon and weather disabled.

## Directory Structure

```
common/world/
├── WorldConfig.hpp                    # WorldType enum and configuration
└── gen/chunk/
    ├── DebugChunkGenerator.hpp        # Debug chunk generator header
    └── DebugChunkGenerator.cpp        # Debug chunk generator implementation

server/
├── application/IntegratedServer.hpp   # IntegratedServerConfig with WorldType
├── application/IntegratedServer.cpp   # Debug mode initialization
├── world/ServerWorld.hpp              # isDebugWorld flag
└── interaction/BlockInteractionManager.cpp  # Block modification blocking
```

## Key Components

### 1. WorldType Enum

```cpp
enum class WorldType : u8 {
    Default,        // Normal terrain generation
    Flat,           // Superflat
    LargeBiomes,    // Large biomes
    Amplified,      // Amplified terrain
    Debug           // Debug mode - show all block states
};
```

### 2. DebugChunkGenerator

The debug chunk generator produces a grid of all block states:

```cpp
class DebugChunkGenerator : public BaseChunkGenerator {
public:
    // Get all valid block states (static)
    static const std::vector<const BlockState*>& getAllValidStates();

    // Grid dimensions
    static i32 getGridWidth();
    static i32 getGridHeight();

    // Get block state at world coordinates
    static const BlockState* getBlockStateFor(i32 x, i32 z);

    // Initialize the block state list (must call after BlockRegistry is populated)
    static void initializeValidStates();
};
```

### 3. Block State Grid Algorithm

Blocks are placed only at odd coordinates:
- `(x > 0 && z > 0 && x % 2 != 0 && z % 2 != 0)`

Grid index calculation:
```cpp
i32 gridX = x / 2;
i32 gridZ = z / 2;
i32 index = abs(gridX * GRID_WIDTH + gridZ);
```

### 4. Debug World Behaviors

When `ServerWorld::isDebugWorld()` is true:

1. **Block modification disabled**:
   - `ServerWorld::setBlockState()` returns false
   - `BlockInteractionManager::handleBlockPlacement()` returns error
   - `BlockInteractionManager::handleBlockBreak()` returns error

2. **Tick processing disabled**:
   - `ServerWorld::tick()` skips scheduled ticks
   - Weather updates disabled
   - Redstone cleanup disabled

3. **Special initialization** (in `IntegratedServer`):
   - Game mode set to Spectator
   - Daytime set to 6000 (noon)
   - Daylight cycle disabled
   - Weather set to clear

### 5. Network Synchronization

The `LoginResponsePacket` includes an `isDebugWorld` flag to notify clients:

```cpp
class LoginResponsePacket {
    bool m_isDebugWorld = false;
public:
    bool isDebugWorld() const;
    void setIsDebugWorld(bool isDebugWorld);
};
```

## Usage

### Creating a Debug World

```cpp
IntegratedServerConfig config;
config.worldType = WorldType::Debug;  // Default in current implementation
config.defaultGameMode = GameMode::Spectator;
config.seed = 0;  // Seed doesn't affect debug mode

auto server = std::make_unique<IntegratedServer>();
server->initialize(config);
```

### Querying Debug Mode

```cpp
// In ServerWorld
if (world.isDebugWorld()) {
    // Disable block modifications, ticks, etc.
}

// In WorldConfig
if (config.isDebugWorld()) {
    // Apply debug-specific settings
}
```

## Test Cases

Located in `tests/common/world/gen/DebugChunkGeneratorTest.cpp`:

| Test | Description |
|------|-------------|
| CreateGenerator | Verify generator construction |
| InitializeValidStates | Verify block state collection |
| GetBlockStateFor | Verify block placement algorithm |
| GridIndexCalculation | Verify grid index mapping |
| BiomeAlwaysPlains | Verify biome is always Plains |
| GetHeight | Verify height returns 60 or 70 |
| GridSizeConsistency | Verify grid can hold all states |
| WorldTypeEnum | Verify WorldType names |
| ParseWorldType | Verify string parsing |
| WorldConfigDebugCheck | Verify isDebugWorld() method |

## Reference

This implementation follows Minecraft Java Edition 1.16.5 debug mode:
- `net.minecraft.world.gen.DebugChunkGenerator`
- `net.minecraft.world.World#isDebugWorld()`
- World type: `debug_all_block_states`
