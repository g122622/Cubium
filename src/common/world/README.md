# World Module

The `world` module is the core of Minecraft Reborn's world simulation, providing terrain generation, block/fluid management, lighting systems, chunk handling, and world state management. This module implements Minecraft 1.16.5 compatible world mechanics.

## Directory Structure

```
world/
├── IWorld.hpp/cpp              # World access interface
├── IWorldWriter.hpp            # World writer interface for generation
├── WorldConstants.hpp          # World constants and utility functions
├── biome/                      # Biome system
│   ├── Biome.hpp/cpp           # Biome definition (170 biomes)
│   ├── BiomeGenerationSettings.hpp/cpp  # Biome generation config
│   ├── BiomeProvider.hpp/cpp   # Biome provider base class
│   ├── BiomeRegistry.hpp/cpp   # Biome registry
│   ├── Biomes.hpp              # Biome ID constants
│   └── layer/                  # Layer-based biome generation
│       ├── BiomeValues.hpp/cpp # Biome value constants
│       ├── Layer.hpp           # Layer interface
│       ├── LayerContext.hpp/cpp
│       ├── LayerUtil.hpp/cpp
│       ├── LayerCacheConfig.hpp
│       └── transformers/       # Layer transformers
│           ├── BiomeLayers.hpp/cpp
│           ├── ClimateLayers.hpp/cpp
│           ├── EdgeLayers.hpp/cpp
│           ├── MergeLayers.hpp/cpp
│           ├── SourceLayers.hpp/cpp
│           ├── TransformerTraits.hpp/cpp
│           └── ZoomLayers.hpp/cpp
├── block/                      # Block system
│   ├── Block.hpp/cpp           # Block base class
│   ├── BlockPos.hpp            # Block position type
│   ├── BlockRegistry.hpp/cpp   # Block registry
│   ├── Material.hpp/cpp        # Block materials
│   ├── HarvestTool.hpp         # Tool type definitions
│   ├── ILiquidContainer.hpp    # Liquid container interface
│   ├── VanillaBlocks.hpp/cpp   # Vanilla block definitions
│   └── blocks/                 # Specific block types
│       ├── AirBlock.hpp/cpp
│       ├── LiquidBlock.hpp/cpp
│       ├── RotatedPillarBlock.hpp/cpp
│       └── SimpleBlock.hpp/cpp
├── blockentity/                # Block entities
│   ├── BlockEntity.hpp/cpp     # Block entity base
│   ├── BlockEntityType.hpp/cpp # Block entity types
│   ├── ContainerBlockEntity.hpp
│   └── CraftingTableEntity.hpp/cpp
├── chunk/                      # Chunk management
│   ├── ChunkData.hpp/cpp       # Chunk data storage
│   ├── ChunkPos.hpp            # Chunk position type
│   ├── ChunkPrimer.hpp/cpp     # Intermediate chunk during generation
│   ├── ChunkStatus.hpp/cpp     # Chunk generation stages
│   ├── IChunk.hpp/cpp          # Chunk interface
│   ├── ChunkDistanceGraph.hpp/cpp    # BFS distance calculation
│   ├── ChunkLoadTicket.hpp     # Ticket types for chunk loading
│   ├── ChunkLoadTicketManager.hpp/cpp # Multi-source ticket aggregation
│   └── SingleChunkLifecycleManager.hpp/cpp # Per-chunk lifecycle and request generations
├── dimension/                  # Dimension system
│   └── DimensionRenderSettings.hpp
├── entity/                     # World entity management
│   └── EntityManager.hpp/cpp   # Entity lifecycle and queries
├── fluid/                      # Fluid system
│   ├── Fluid.hpp/cpp           # Fluid base class
│   ├── FlowingFluid.hpp/cpp    # Flowing fluid mechanics（含世界感知时序和孔洞判断）
│   ├── FluidRegistry.hpp/cpp   # Fluid registry
│   ├── FluidTags.hpp/cpp       # Fluid tags
│   ├── FLUID_TODO.md           # Fluid system TODO
│   └── fluids/                 # Specific fluids
│       ├── EmptyFluid.hpp/cpp
│       ├── LavaFluid.hpp/cpp
│       └── WaterFluid.hpp/cpp
├── gen/                        # World generation
│   ├── carver/                 # Cave/canyon carving
│   │   ├── WorldCarver.hpp/cpp # Base carver
│   │   ├── CaveCarver.hpp/cpp  # Cave generation
│   │   ├── CanyonCarver.hpp/cpp # Canyon generation
│   │   ├── UnderwaterCarver.hpp/cpp # Underwater caves
│   │   └── Carvers.hpp         # Carver constants
│   ├── chunk/                  # Chunk generators
│   │   ├── IChunkGenerator.hpp/cpp # Generator interface
│   │   └── NoiseChunkGenerator.hpp/cpp # Noise-based generator
│   ├── feature/                # World features
│   │   ├── Feature.hpp/cpp     # Feature base class
│   │   ├── ConfiguredFeature.hpp/cpp
│   │   ├── FeatureSpread.hpp/cpp
│   │   ├── DecorationStage.hpp # Feature decoration stages
│   │   ├── FeatureIds.hpp      # Feature IDs
│   │   ├── lake/               # Lake features
│   │   ├── ocean/              # Ocean features (kelp, seagrass, live/dead coral, blue ice, ocean props)
│   │   ├── ore/                # Ore features
│   │   ├── template/           # Structure templates
│   │   ├── tree/               # Tree generation
│   │   │   ├── TreeFeature.hpp/cpp
│   │   │   ├── trunk/          # Trunk placers
│   │   │   └── foliage/        # Foliage placers
│   │   └── vegetation/         # Vegetation features
│   ├── jigsaw/                 # Jigsaw structure assembly
│   │   ├── JigsawManager.hpp/cpp
│   │   ├── JigsawPattern.hpp/cpp
│   │   ├── JigsawPiece.hpp/cpp
│   │   └── JigsawJunction.hpp
│   ├── noise/                  # Noise generators
│   │   ├── INoiseGenerator.hpp
│   │   ├── Noise.hpp
│   │   ├── ImprovedNoiseGenerator.hpp/cpp
│   │   └── OctavesNoiseGenerator.hpp/cpp
│   ├── placement/              # Feature placement
│   │   ├── Placement.hpp/cpp
│   │   ├── PlacementRegistry.hpp/cpp
│   │   ├── Placements.hpp/cpp
│   │   └── PlacementUtils.hpp/cpp
│   ├── settings/               # Generation settings
│   │   ├── DimensionSettings.hpp/cpp
│   │   ├── NoiseSettings.hpp
│   │   ├── ScalingSettings.hpp
│   │   ├── SlideSettings.hpp
│   │   └── Settings.hpp
│   ├── spawn/                  # World spawn
│   │   └── WorldGenSpawner.hpp/cpp
│   ├── structure/              # Structure generation
│   │   ├── Structure.hpp/cpp   # Structure base
│   │   ├── StructureBoundingBox.hpp
│   │   ├── StructureManager.hpp/cpp
│   │   ├── JigsawStructure.hpp/cpp
│   │   ├── pieces/             # Structure pieces
│   │   └── structures/         # Specific structures
│   │       ├── VillageStructure.hpp/cpp
│   │       ├── MineshaftStructure.hpp/cpp
│   │       ├── StrongholdStructure.hpp/cpp
│   │       └── ... (10 structures total, 含 Shipwreck/OceanRuin)
│   └── surface/                # Surface builders
│       ├── Surface.hpp
│       ├── SurfaceBuilder.hpp
│       └── SurfaceBuilders.hpp/cpp (12 builders)
├── redstone/                   # Redstone system
│   ├── RedstoneSystem.hpp/cpp  # Redstone system manager
│   ├── RedstonePower.hpp/cpp   # Signal strength calculation
│   ├── RedstoneContext.hpp/cpp # Recursion protection
│   └── RedstoneHelper.hpp/cpp  # Helper functions
├── spawn/                      # Mob spawn info
│   └── MobSpawnInfo.hpp/cpp
├── tick/                       # Tick scheduling
│   ├── base/
│   │   ├── ScheduledTick.hpp   # Tick data structure
│   │   └── TickPriority.hpp    # Tick priority enum
│   ├── list/
│   │   ├── ITickList.hpp       # Tick list interface
│   │   ├── EmptyTickList.hpp   # Empty implementation
│   │   └── ServerTickList.hpp  # Server tick list
│   └── manager/
│       └── TickManager.hpp/cpp # Unified tick management
├── time/                       # Game time
│   └── GameTime.hpp/cpp        # Day/night cycle
└── weather/                    # Weather system
    ├── WeatherConstants.hpp    # Weather constants
    ├── WeatherState.hpp        # Weather state data
    └── WeatherUtils.hpp/cpp    # Weather utilities
```

## Core Components

### IWorld Interface

`IWorld.hpp` defines the world access interface used by entities and game logic:

```cpp
class IWorld {
public:
    // Block access
    virtual const BlockState* getBlockState(i32 x, i32 y, i32 z) const = 0;
    virtual bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) = 0;

    // Fluid access
    virtual const FluidState* getFluidState(i32 x, i32 y, i32 z) const = 0;

    // Chunk access
    virtual const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const = 0;

    // Light queries
    virtual u8 getBlockLight(i32 x, i32 y, i32 z) const = 0;
    virtual u8 getSkyLight(i32 x, i32 y, i32 z) const = 0;

    // Collision detection
    virtual bool hasBlockCollision(const AxisAlignedBB& box) const = 0;
    virtual std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const = 0;

    // Entity queries
    virtual std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except) const = 0;

    // Tick scheduling
    virtual void scheduleBlockTick(const BlockPos& pos, Block& block, i32 delay, TickPriority priority) = 0;

    // 维度上下文
    virtual bool isUltraWarm() const = 0;
};
```

### Chunk System

The chunk system manages world data in 16x16x256 block sections:

**Key Classes:**
- **ChunkData**: Stores 16 ChunkSections, each 16x16x16 blocks
- **ChunkSection**: Stores block states, sky light, block light
- **ChunkPrimer**: Intermediate chunk during generation
- **ChunkStatus**: 13 generation stages (EMPTY to FULL)
- **SingleChunkLifecycleManager**: Manages chunk loading state, request generations, and cancellation
- **ChunkLoadTicketManager**: Aggregates ticket sources and computes load pressure

### Chunk Loading Flow

1. Ticket sources update `ChunkLoadTicketManager`.
2. `ChunkDistanceGraph` converts ticket pressure into chunk levels.
3. `SingleChunkLifecycleManager` decides whether a chunk can queue, start, finish, or cancel generation.
4. `ServerChunkManager` and the server-side worker pool execute the request.
5. Late results are dropped when the request generation no longer matches.

This design keeps player movement, forced tickets, portal tickets, teleport tickets, and other request sources on the same scheduling path.

**Generation Stages:**
```
EMPTY -> STRUCTURE_STARTS -> STRUCTURE_REFERENCES -> BIOMES -> NOISE ->
SURFACE -> CARVERS -> LIQUID_CARVERS -> FEATURES -> LIGHT -> SPAWN ->
HEIGHTMAPS -> FULL
```

### Block System

**Block and BlockState:**
- Blocks are registered via `BlockRegistry`
- Each block can have multiple states (properties like facing, powered, etc.)
- States use `StateHolder<Block, BlockState>` for O(1) state transitions

```cpp
// Block properties
BlockProperties props(Material::ROCK)
    .hardness(1.5f)
    .resistance(6.0f)
    .requiresTool();

// Get block state
const BlockState* state = Block::getBlockState(stateId);
bool isSolid = state->isSolid();
u8 lightLevel = state->lightLevel();
```

### Biome System

**170 Biomes (MC 1.16.5 compatible):**
- Layer-based biome generation matching MC 1.16.5
- BiomeProvider provides biome queries
- BiomeGenerationSettings defines features, carvers, surface builders

```cpp
// Create biome provider
auto provider = createOverworldBiomeProvider(seed);
BiomeId biome = provider->getBiome(x, y, z);

// Biome properties
const Biome& def = BiomeRegistry::get(biomeId);
f32 temp = def.temperature();
const BlockState* surface = def.surfaceBlock();
```

### Lighting System

The lighting system handles sky light and block light propagation:

**Components:**
- **WorldLightManager**: Coordinating sky/block light engines
- **BlockLightEngine**: Propagates light from emissive blocks
- **SkyLightEngine**: Propagates sky light from above
- **SWMRNibbleArray**: Single-Writer-Multiple-Reader light storage

```cpp
// Light management
WorldLightManager lightManager(provider, hasBlockLight, hasSkyLight);
lightManager.checkBlock(pos);  // Recalculate light at pos
lightManager.tick(maxUpdates, updateSky, updateBlock);
```

### World Generation

**NoiseChunkGenerator** implements MC 1.16.5 terrain generation:

```cpp
DimensionSettings settings = DimensionSettings::overworld();
NoiseChunkGenerator generator(seed, std::move(settings));

// Generate chunk
ChunkPrimer primer(chunkX, chunkZ);
generator.generateBiomes(region, primer);
generator.generateNoise(region, primer);
generator.buildSurface(region, primer);
```

**Feature Placement System:**
- 13 placement modifiers (Count, Chance, HeightRange, Biome, Noise, etc.)
- Surface Builders (12 types: Default, Mountain, Desert, Swamp, etc.)
- Tree Generation (6 TrunkPlacers, 9 FoliagePlacers)
- Structure Generation (10 structures: Village, Mineshaft, Stronghold, OceanMonument, RuinedPortal, BuriedTreasure, Shipwreck, OceanRuin, etc.)

### Fluid System

Fluids (water, lava) flow through the world:

```cpp
class Fluid {
    virtual bool isSource(const FluidState& state) const = 0;
    virtual i32 getLevel(const FluidState& state) const = 0;
    virtual i32 getTickDelay() const = 0;  // Water: 5, Lava: 30/10
    virtual bool canSourcesMultiply() const = 0;  // Water: true, Lava: false
};
```

### Tick System

Scheduled ticks for blocks and fluids:

```cpp
TickManager tickManager(world);

// Schedule block tick
tickManager.scheduleBlockTick(pos, block, 10, TickPriority::Normal);

// Schedule fluid tick (water flow)
tickManager.scheduleFluidTick(pos, water, 5);

// Process ticks each game tick
tickManager.tick(currentTick);
```

### Weather System

```cpp
WeatherState weather;
weather.rainTime = 12000;
weather.thunderTime = 3600;

bool isRaining = weather.isRaining();      // strength > 0.2
bool isThunder = weather.isThundering();   // strength > 0.9
u8 skyLight = weather.skyLightLimit();     // 15/12/10 based on weather
```

## Module Relationships

```
                    ┌─────────────────┐
                    │     IWorld      │
                    │   (Interface)   │
                    └────────┬────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
         ▼                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│   ChunkData     │ │   Block/Fluid   │ │   Lighting      │
│   ChunkManager  │ │   Registry      │ │   System        │
└────────┬────────┘ └────────┬────────┘ └────────┬────────┘
         │                   │                   │
         │                   │                   │
         └───────────────────┼───────────────────┘
                             │
                    ┌────────┴────────┐
                    │   Generation    │
                    │  (Noise, Biome, │
                    │   Feature, etc) │
                    └─────────────────┘
```

## Inputs and Outputs

### Inputs
- **World Seed**: Integer seed for deterministic generation
- **Dimension Settings**: Configuration for terrain generation
- **Block/Fluid State Changes**: Player/modification events
- **Tick Schedules**: Scheduled block/fluid updates
- **Entity Actions**: Entity movement and interactions

### Outputs
- **Chunk Data**: Serialized chunk for network/storage
- **Block/Fluid States**: World state queries
- **Light Values**: Sky and block light at positions
- **Entity Positions**: Entity locations and collisions
- **Generated Structures**: Placed features and structures

## Dependencies

### Internal Dependencies
- `common/core/Types.hpp` - Primitive types (i8, i16, i32, String, etc.)
- `common/core/Result.hpp` - Error handling
- `common/core/Constants.hpp` - Game constants
- `common/util/math/` - Math utilities, random, vectors
- `common/util/nbt/` - NBT serialization
- `common/util/property/` - Property/state system
- `common/physics/collision/` - Collision shapes
- `common/entity/` - Entity base classes
- `common/resource/` - Resource locations

### External Dependencies
- `glm` - Math library
- `spdlog` - Logging

## Usage Examples

### Creating a World

```cpp
// Create biome provider
auto biomeProvider = createOverworldBiomeProvider(seed);

// Create chunk generator
DimensionSettings settings = DimensionSettings::overworld();
NoiseChunkGenerator generator(seed, std::move(settings), std::move(biomeProvider));

// Generate a chunk
ChunkPrimer primer(chunkX, chunkZ);
WorldGenRegion region(chunkX, chunkZ, neighbors);
generator.generateBiomes(region, primer);
generator.generateNoise(region, primer);
generator.buildSurface(region, primer);
generator.applyCarvers(region, primer, false);
generator.placeFeatures(region, primer);

// Convert to chunk data
std::unique_ptr<ChunkData> data = primer.toChunkData();
```

### Querying Block States

```cpp
// Get block at position
const BlockState* state = world.getBlockState(x, y, z);
if (state && !state->isAir()) {
    f32 hardness = state->hardness();
    bool isSolid = state->isSolid();
    u8 light = state->lightLevel();
}

// Check fluid
const FluidState* fluid = world.getFluidState(x, y, z);
if (fluid && !fluid->isEmpty()) {
    bool isSource = fluid->isSource();
    i32 level = fluid->getLevel();
}
```

### Light Queries

```cpp
// Get combined light at position
u8 blockLight = world.getBlockLight(x, y, z);
u8 skyLight = world.getSkyLight(x, z);
u8 combined = std::max(blockLight, skyLight - skyDarkening);

// Height query
i32 height = world.getHeight(x, z);  // Top solid block
```

### Chunk Loading with Tickets

```cpp
ChunkLoadTicketManager ticketManager;
ticketManager.setViewDistance(10);

// Set callbacks
ticketManager.setLevelChangeCallback([](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
    if (newLevel <= 33 && oldLevel > 33) {
        loadChunk(x, z);
    } else if (newLevel > 33 && oldLevel <= 33) {
        unloadChunk(x, z);
    }
});

// Update player position (triggers chunk loading)
ticketManager.updatePlayerPosition(playerId, chunkX, chunkZ);
ticketManager.processUpdates();
```

## Common Pitfalls

### 1. Chunk Generation Order
**Problem**: Accessing blocks in a chunk before it reaches FULL status.
**Solution**: Always check chunk status before accessing block data. Use futures to wait for specific stages.

```cpp
// BAD
ChunkData* chunk = getChunk(x, z);
chunk->getBlock(localX, y, localZ);  // May crash if chunk not loaded

// GOOD
if (chunk && chunk->getStatus() == ChunkLoadStatus::Full) {
    chunk->getBlock(localX, y, localZ);
}
```

### 2. Light Storage Threading
**Problem**: Reading light data while light engine is updating.
**Solution**: Use SWMRNibbleArray's thread-safe accessors or synchronize access.

### 3. Biome Sampling Resolution
**Problem**: Biomes are sampled at 4x4 block resolution.
**Solution**: Use BiomeContainer for proper 4x4x4 sampling, not per-block queries.

```cpp
// Biomes are stored at 4x4x4 resolution
BiomeContainer biomes = chunk.getBiomes();
BiomeId biome = biomes.getBiomeAt(x / 4, y / 4, z / 4);
```

### 4. Fluid Level Direction
**Problem**: Fluid level increases toward source, not away.
**Solution**: Level 8 = source, level 1 = furthest from source.

```cpp
// Level 8 = source block
// Level 7-1 = flowing, decreasing toward edge
bool isSource = fluidState.getLevel() == 8;
```

### 5. Tick Priority Overflow
**Problem**: Too many scheduled ticks causing performance issues.
**Solution**: Limit tick counts per tick, use priorities for critical updates.

```cpp
// TickManager has limits
tickManager.tick(currentTick);  // Processes up to 65536 ticks each type
```

### 6. Coordinate Conversions
**Problem**: Incorrect world-to-chunk coordinate conversion for negative values.
**Solution**: Use provided utility functions from WorldConstants.hpp.

```cpp
// BAD
i32 chunkX = x / 16;  // Wrong for negative x

// GOOD
i32 chunkX = world::toChunkCoord(x);  // Handles negatives correctly
i32 localX = world::toLocalCoord(x);
```

### 7. BlockState Pointer Validity
**Problem**: Storing BlockState pointers beyond registry lifetime.
**Solution**: BlockState pointers are valid for the entire program lifetime (registry never unloads).

### 8. Generation Thread Safety
**Problem**: Modifying chunk data from multiple threads.
**Solution**: Use SingleChunkLifecycleManager's mutex-protected accessors, or generate into ChunkPrimer first.

## Test Coverage

Tests are located in `tests/common/world/` and `tests/server/world/`:

| Test File | Coverage |
|-----------|----------|
| `biome/layer/BiomeLayerTest.cpp` | Biome layer generation |
| `biome/layer/MergeLayersTest.cpp` | Biome layer merging |
| `fluid/FluidTest.cpp` | Fluid mechanics |
| `tick/ServerTickListTest.cpp` | Tick scheduling |
| `gen/ChunkSpawnIntegrationTest.cpp` | Chunk spawn generation |
| `gen/WorldGenSpawnerTest.cpp` | Mob spawning during gen |
| `gen/WorldGenDeterminismTest.cpp` | Generation determinism |
| `gen/test_vegetation_features.cpp` | Tree/vegetation features |
| `EntityManagerSpawnTest.cpp` | Entity spawning |
| `EntityTrackerTest.cpp` | Entity tracking |
| `ItemPickupManagerTest.cpp` | Item pickup |
| `NaturalSpawnerTest.cpp` | Natural mob spawning |
| `ServerWorldTest.cpp` | Server world integration |
| `ServerWorldCollisionTests.cpp` | Collision detection |

## Related Documentation

- `CLAUDE.md` - Project-wide architecture and conventions
- `src/common/world/fluid/FLUID_TODO.md` - Fluid system TODO
- `src/server/world/` - Server-specific world implementations
- `src/client/world/` - Client-specific world implementations
