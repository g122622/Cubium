# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Minecraft Reborn is a modern Minecraft clone with client-server architecture written in C++17 using Vulkan for rendering. The project aims to replicate the Java Edition 1.16.5 experience as closely as possible while maintaining compatibility with existing Minecraft ecosystem (resource packs, world saves, data packs).

## Build Commands

```powershell
# Set vcpkg environment (required)
$env:VCPKG_ROOT = "D:\tools\vcpkg"

# Configure project (first time or after CMakeLists changes)
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
# 注：即使在开发过程中，也要尽量使用Release构建，因为Debug运行非常慢，除非必要否则不要用。
cmake --build build --config Release

# Run tests
./build/bin/Release/mc_tests.exe

# Run server
./build/bin/Release/minecraft-server.exe --help

# Run client
./build/bin/Release/minecraft-client.exe
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `MC_BUILD_CLIENT` | ON | Build client application |
| `MC_BUILD_SERVER` | ON | Build server application |
| `MC_BUILD_TESTS` | ON | Build unit tests |
| `MC_ENABLE_SANITIZERS` | OFF | Enable sanitizers for debug |
| `MC_ENABLE_VULKAN_VALIDATION` | ON | Enable Vulkan validation layers |
| `MC_ENABLE_TRACING` | OFF | Enable Perfetto performance tracing |

## Architecture

```
src/
├── common/                    # Shared code between client and server
│   ├── core/                  # Core types and utilities
│   │   ├── Types.hpp          # Primitive types (i8, i16, String, etc.)
│   │   ├── Result.hpp         # Error handling (Result<T>, Error, ErrorCode)
│   │   ├── Constants.hpp      # Game constants
│   │   ├── EnumSet.hpp        # Enum set utility
│   │   ├── BlockRaycastResult.hpp
│   │   └── settings/          # Settings system (SettingsBase, Options)
│   ├── command/               # Command system
│   │   ├── arguments/         # Command argument parsers
│   │   ├── exceptions/        # Command exceptions
│   │   └── suggestions/       # Tab completion suggestions
│   ├── entity/                # Entity system
│   │   ├── ai/                # AI system
│   │   │   ├── controller/    # Look/Movement/Jump controllers
│   │   │   ├── goal/          # Goal-based AI
│   │   │   │   └── goals/     # Specific goal implementations
│   │   │   └── pathfinding/   # Pathfinding system
│   │   ├── animal/            # Animal entities (Pig, Cow, Sheep, Chicken)
│   │   ├── attribute/         # Entity attributes
│   │   ├── combat/            # Combat system
│   │   ├── damage/            # Damage tracking
│   │   ├── inventory/         # Inventory system
│   │   ├── living/            # Living entity base
│   │   ├── loot/              # Loot tables
│   │   ├── mob/               # Mob entity base
│   │   └── movement/          # Movement system (AutoJump)
│   ├── item/                  # Item system
│   │   ├── crafting/          # Crafting recipes
│   │   ├── enchantment/       # Enchantment system
│   │   │   └── enchantments/  # Specific enchantments
│   │   ├── tier/              # Tool tier system
│   │   └── tool/              # Tool items
│   ├── network/               # Networking
│   │   ├── connection/        # LocalConnection for integrated server
│   │   ├── packet/            # Packet serialization
│   │   └── sync/              # Chunk synchronization
│   ├── perfetto/              # Performance tracing
│   ├── physics/               # Physics engine
│   │   └── collision/         # Collision detection
│   ├── resource/              # Resource pack system
│   │   ├── compat/            # MC version compatibility layer
│   │   │   ├── unified/       # Unified resource representations
│   │   │   ├── v1_12/         # MC 1.12 resource mapping
│   │   │   └── v1_13/         # MC 1.13+ resource mapping
│   │   └── loader/            # Resource loading pipeline
│   ├── screen/                # Screen types
│   ├── util/                  # Utilities
│   │   ├── cache/             # LRU cache implementations
│   │   ├── math/              # Math utilities
│   │   │   ├── random/        # Random number generators
│   │   │   └── ray/           # Raycast utilities
│   │   ├── nbt/               # NBT serialization
│   │   └── property/          # Property system
│   └── world/                 # World system
│       ├── biome/             # Biome system
│       │   └── layer/         # Biome layer generation
│       │       └── transformers/
│       ├── block/             # Block system
│       │   └── blocks/        # Specific block types
│       ├── blockentity/       # Block entities
│       ├── chunk/             # Chunk management
│       ├── dimension/         # Dimension system
│       ├── entity/            # World entity management
│       ├── fluid/             # Fluid system
│       │   └── fluids/        # Water, Lava, Empty
│       ├── gen/               # World generation
│       │   ├── carver/        # Cave/Canyon carvers
│       │   ├── chunk/         # Chunk generators
│       │   ├── feature/       # Features (ores, trees, vegetation)
│       │   │   ├── tree/      # Tree generation system
│       │   │   │   ├── trunk/ # TrunkPlacers (Straight, DarkOak, Fancy, Forky, Giant, MegaJungle)
│       │   │   │   └── foliage/ # FoliagePlacers (Blob, Pine, Spruce, Acacia, DarkOak, Jungle, MegaPine, Bush, Fancy)
│       │   │   ├── ore/       # OreFeature
│       │   │   ├── vegetation/ # FlowerFeature, GrassFeature, BigMushroomFeature, CactusFeature
│       │   │   ├── lake/      # LakeFeature
│       │   │   └── template/  # StructureTemplate, TemplateManager, TemplateLoader
│       │   ├── jigsaw/        # Jigsaw structure assembly system
│       │   ├── noise/         # Noise generators
│       │   ├── placement/     # Feature placement (Count, Chance, HeightRange, Biome, Noise, etc.)
│       │   ├── settings/      # Generation settings
│       │   ├── spawn/         # World spawn
│       │   ├── structure/     # Structure generation
│       │   │   └── structures/ # Village, Mineshaft, Stronghold, DesertPyramid, JungleTemple, OceanMonument, etc.
│       │   └── surface/       # Surface builders
│       ├── lighting/          # Lighting system
│       │   ├── engine/        # Light engines
│       │   ├── manager/       # Light manager
│       │   └── storage/       # Light storage
│       ├── spawn/             # Spawn info
│       ├── tick/              # Tick system
│       ├── time/              # Game time (day/night cycle)
│       └── weather/           # Weather system
├── server/                    # Server application
│   ├── application/           # ServerApplication, IntegratedServer
│   ├── core/                  # ServerCore module (modular design)
│   │   ├── ServerCore.hpp/cpp       # Facade class
│   │   ├── ServerCoreConfig.hpp     # Configuration struct
│   │   ├── ServerPlayerData.hpp     # Player data structure
│   │   ├── PlayerManager.hpp/cpp    # Player lifecycle
│   │   ├── ConnectionManager.hpp/cpp# Network communication
│   │   ├── TimeManager.hpp/cpp      # Game time, tick count
│   │   ├── TeleportManager.hpp/cpp  # Teleport handling
│   │   ├── KeepAliveManager.hpp/cpp # Heartbeat/ping
│   │   ├── PositionTracker.hpp/cpp  # Position tracking
│   │   ├── PacketHandler.hpp/cpp    # Unified packet handling
│   │   └── GameModeManager.hpp/cpp  # Game mode management
│   ├── network/               # TcpServer, TcpSession, TcpConnection
│   ├── command/               # Command system
│   │   ├── CommandRegistry.hpp
│   │   ├── ServerCommandSource.hpp
│   │   └── commands/          # Command implementations
│   ├── menu/                  # Container menu system
│   ├── player/                # ServerPlayer
│   ├── settings/              # ServerSettings
│   └── world/                 # ServerWorld
│       ├── ServerWorld.hpp
│       ├── ServerChunkManager.hpp
│       ├── ChunkWorkerPool.hpp
│       ├── drop/              # Block drop handling
│       ├── entity/            # EntityTracker, ItemPickupManager
│       ├── spawn/             # NaturalSpawner, SpawnConditions
│       └── weather/           # WeatherManager
├── client/                    # Client application
│   ├── application/           # ClientApplication
│   ├── chat/                  # ChatHistory
│   ├── input/                 # InputManager
│   ├── network/               # NetworkClient
│   ├── settings/              # ClientSettings
│   ├── window/                # Window (GLFW wrapper)
│   ├── resource/              # Resource loading
│   │   ├── ResourceManager.hpp
│   │   ├── BlockModelLoader.hpp
│   │   ├── BlockStateLoader.hpp
│   │   ├── TextureAtlasBuilder.hpp
│   │   ├── BlockModelCache.hpp
│   │   ├── ItemTextureAtlas.hpp
│   │   └── EntityTextureLoader.hpp
│   ├── renderer/              # Rendering system
│   │   ├── api/               # Platform-agnostic rendering interface
│   │   │   ├── IRenderEngine.hpp
│   │   │   ├── Types.hpp
│   │   │   ├── BlendMode.hpp, CompareOp.hpp, CullMode.hpp
│   │   │   ├── buffer/IBuffer.hpp
│   │   │   ├── camera/ICamera.hpp, CameraConfig.hpp
│   │   │   ├── mesh/MeshData.hpp
│   │   │   ├── pipeline/IPipeline.hpp, RenderState.hpp, RenderType.hpp
│   │   │   └── texture/ITexture.hpp, ITextureAtlas.hpp, TextureRegion.hpp
│   │   ├── trident/           # Trident Vulkan engine
│   │   │   ├── core/          # Core components
│   │   │   │   ├── TridentContext.hpp/cpp
│   │   │   │   ├── TridentEngine.hpp/cpp
│   │   │   │   ├── TridentSwapchain.hpp/cpp
│   │   │   │   ├── buffer/TridentBuffer.hpp/cpp
│   │   │   │   ├── pipeline/TridentPipeline.hpp/cpp
│   │   │   │   ├── render/RenderPassManager.hpp, FrameManager.hpp
│   │   │   │   ├── render/DescriptorManager.hpp, UniformManager.hpp
│   │   │   │   └── texture/TridentTexture.hpp/cpp
│   │   │   ├── chunk/         # ChunkRenderer, ChunkMesher, AO
│   │   │   ├── cloud/         # CloudRenderer
│   │   │   ├── entity/        # EntityRenderer, EntityPipeline, models
│   │   │   ├── fog/           # FogManager
│   │   │   ├── gui/           # GuiRenderer, texture atlases
│   │   │   ├── item/          # ItemRenderer
│   │   │   ├── particle/      # ParticleManager, RainParticle, SnowParticle
│   │   │   ├── postprocess/   # Post-processing
│   │   │   ├── sky/           # SkyRenderer, CelestialCalculations
│   │   │   ├── util/          # VulkanUtils
│   │   │   └── weather/       # WeatherRenderer
│   │   ├── mesh/              # MeshWorkerPool
│   │   ├── Camera.hpp/cpp
│   │   └── MeshTypes.hpp/cpp
│   ├── ui/                    # User interface
│   │   ├── Font.hpp/cpp, FontRenderer.hpp
│   │   ├── FontTextureAtlas.hpp
│   │   ├── TridentCanvas.hpp
│   │   ├── kagero/            # Kagero UI framework
│   │   │   ├── event/         # Event system
│   │   │   ├── layout/        # Layout system (Flex, Grid, Anchor)
│   │   │   ├── paint/         # Paint abstraction
│   │   │   ├── state/         # State management (Reactive)
│   │   │   ├── template/      # Template system
│   │   │   └── widget/        # Widget components
│   │   └── minecraft/         # Minecraft-specific UI
│   │       ├── resources/     # UI resources
│   │       ├── screens/       # Game screens
│   │       └── widgets/       # Game widgets
│   └── world/                 # ClientWorld
│       └── entity/            # ClientEntityManager
└── tests/                     # Test files
    ├── common/                # Common module tests
    ├── client/                # Client module tests
    ├── server/                # Server module tests
    ├── entity/                # Entity tests
    ├── lighting/              # Lighting tests
    ├── network/               # Network tests
    ├── physics/               # Physics tests
    └── ui/                    # UI tests
```

## Key Types

All types are in namespace `mc` (client types in `mc::client`, server types in `mc::server`):

### Primitive Types
- `i8`, `i16`, `i32`, `i64` - Signed integers
- `u8`, `u16`, `u32`, `u64` - Unsigned integers
- `f32`, `f64` - Floating point (prefer f32 for performance)
- `String`, `StringView` - String types
- `Optional<T>` - Optional values

### Game Types
- `ChunkCoord`, `BlockCoord`, `WorldHeight` - Coordinate types
- `BlockId`, `ItemId`, `EntityId`, `BiomeId`, `DimensionId` - ID types
- `PlayerId` - Player identifier

### World Types
- `ChunkPos`, `BlockPos`, `SectionPos` - Position types
- `ChunkId` - 64-bit chunk identifier
- `BlockState` - Block state with properties
- `ChunkSection` - 16x16x16 block section
- `ChunkData` - Full chunk data (16 sections)

### Chunk Generation Types
- `ChunkStatus`: Generation stages (EMPTY → BIOMES → NOISE → SURFACE → CARVERS → FEATURES → LIGHT → HEIGHTMAPS → FULL)
- `ChunkPrimer`: Intermediate chunk state during generation
- `ChunkHolder`: Manages chunk loading state and futures
- `ChunkTask`: Generation task for worker pool
- `IChunk`: Chunk interface for generation

### Biome Types
- `BiomeId` - Biome identifier (170 biomes, MC 1.16.5 compatible)
- `Biome` - Biome definition with climate, features, carvers
- `BiomeContainer` - 4x4x4 sampled biome storage
- `BiomeProvider` - Base class for biome distribution
- `LayerBiomeProvider` - Layer-based biome generation (MC 1.16.5)

### Noise Types
- `INoiseGenerator` - Noise interface
- `ImprovedNoiseGenerator` - MC-style Perlin noise
- `OctavesNoiseGenerator` - Multi-octave noise (up to 16 octaves)
- `PerlinNoiseGenerator`, `SimplexNoiseGenerator` - Other noise types

### Renderer Types
- `Vertex`, `ModelVertex`, `GuiVertex` - Vertex types
- `Face` - Triangle face
- `MeshData` - Mesh vertex/index buffers
- `TextureRegion` - UV coordinates in atlas
- `BakedBlockModel`, `UnbakedBlockModel` - Model types

### Renderer API Types (Platform-agnostic)
- `IRenderEngine` - Main render engine interface
- `IVertexBuffer`, `IIndexBuffer`, `IUniformBuffer`, `IStagingBuffer` - Buffer interfaces
- `ITexture`, `ITextureAtlas` - Texture interfaces
- `ICamera` - Camera interface
- `RenderState` - Blend, depth, rasterizer state
- `RenderType` - Named render types (MC 1.16.5 style)

### Fog Types
- `FogMode`: Fog mode enum (None, Linear, Exp2)
- `FogUBO`: Fog uniform buffer data (fogStart, fogEnd, fogDensity, fogColor)
- `FogManager`: Fog effect manager

### Network Types
- `PacketType` - Packet type enumeration
- `PacketHeader` - 12-byte packet header
- `Packet` - Base packet class
- `PacketSerializer/Deserializer` - Binary serialization
- `IServerConnection` - Server connection interface
- `LocalEndpoint`, `LocalConnectionPair` - Local IPC for integrated server

### Error Handling
- `Result<T>` - Result type for fallible operations
- `Error` - Error container with code and message
- `ErrorCode` - Error code enumeration

### Settings Types
- `BooleanOption`, `RangeOption`, `FloatOption` - Setting option types
- `EnumOption<T>` - Enum setting type
- `StringOption`, `ResourcePackListOption` - Other settings
- `SettingsBase` - Base class for settings management

## Chunk Generation System

The chunk generation system follows MC 1.16.5 architecture:

### Generation Stages
```
EMPTY → BIOMES → NOISE → SURFACE → CARVERS → FEATURES → LIGHT → HEIGHTMAPS → FULL
```

Each stage has specific responsibilities:
- **EMPTY**: Initial state
- **BIOMES**: Generate biome data
- **NOISE**: Generate terrain density using multi-octave Perlin noise
- **SURFACE**: Apply surface blocks (grass, sand, etc.)
- **CARVERS**: Apply cave/canyon carving
- **FEATURES**: Place trees, ores, structures
- **LIGHT**: Calculate lighting
- **HEIGHTMAPS**: Calculate final heightmaps
- **FULL**: Complete chunk

### Key Classes

```cpp
// Create a chunk generator
DimensionSettings settings = DimensionSettings::overworld();
NoiseChunkGenerator generator(seed, std::move(settings));

// Generate a chunk
ChunkPrimer primer(chunkX, chunkZ);
WorldGenRegion region(chunkX, chunkZ, neighbors);
generator.generateBiomes(region, primer);
generator.generateNoise(region, primer);
generator.buildSurface(region, primer);

// Convert to final chunk data
std::unique_ptr<ChunkData> data = primer.toChunkData();
```

### Async Generation

```cpp
// ServerChunkManager handles async generation
ServerChunkManager manager(world, std::move(generator));
manager.initialize();
manager.startWorkers(4);  // 4 worker threads

// Request chunk asynchronously
auto future = manager.getChunkAsync(x, z, &ChunkStatus::FULL);

// Tick the manager
manager.tick();
```

## Tree Generation System

The tree generation system follows MC 1.16.5 architecture with TrunkPlacers and FoliagePlacers:

### TrunkPlacers (6 types)
- **StraightTrunkPlacer**: Simple vertical trunk (oak, birch, spruce, jungle)
- **DarkOakTrunkPlacer**: 2x2 trunk for dark oak trees
- **FancyTrunkPlacer**: Curved trunk for fancy oak trees
- **ForkyTrunkPlacer**: Branching trunk for acacia trees
- **GiantTrunkPlacer**: 2x2 trunk for giant spruce trees
- **MegaJungleTrunkPlacer**: 2x2 trunk for mega jungle trees

### FoliagePlacers (9 types)
- **BlobFoliagePlacer**: Spherical foliage (oak, birch)
- **PineFoliagePlacer**: Conical foliage for pine trees
- **SpruceFoliagePlacer**: Sharp conical foliage for spruce
- **AcaciaFoliagePlacer**: Umbrella-shaped foliage
- **DarkOakFoliagePlacer**: Dense spherical foliage
- **JungleFoliagePlacer**: Sparse single-layer foliage
- **MegaPineFoliagePlacer**: Large conical for giant spruce
- **BushFoliagePlacer**: Single-layer spherical for bushes
- **FancyFoliagePlacer**: Dense spherical for fancy trees

### Pre-configured Tree Types
```cpp
// TreeFeatures provides factory methods for all vanilla trees
auto oakTree = TreeFeatures::createOakTree();
auto spruceTree = TreeFeatures::createSpruceTree();
auto darkOakTree = TreeFeatures::createDarkOakTree();
auto acaciaTree = TreeFeatures::createAcaciaTree();
// ... and more
```

## Structure Generation System

The structure generation system uses the Jigsaw assembly pattern from MC 1.16.5:

### Jigsaw System Components
- **JigsawManager**: BFS assembly algorithm for connecting structure pieces
- **JigsawPiece**: Individual structure template (SingleJigsawPiece, ListJigsawPiece)
- **JigsawPattern**: Pool of pieces with weights for random selection
- **JigsawPatternRegistry**: Global registry of all structure pools
- **JigsawMatcher**: Connection point matching with rotation/mirror transforms

### Supported Structures (8 types)
- **Village**: Plains, desert, savanna, taiga variants
- **Mineshaft**: Underground corridors with loot
- **Stronghold**: End portal rooms with complex corridors
- **DesertPyramid**: Desert temple with hidden loot
- **JungleTemple**: Jungle temple with traps
- **OceanMonument**: Underwater guardian temple
- **RuinedPortal**: Portal ruins in various biomes
- **BuriedTreasure**: Hidden treasure chests

### Template Loading
```cpp
// Load structure templates from NBT
TemplateLoader::loadFromResourcePack(pack, ResourceLocation("minecraft:village/plains/houses"));

// Place template in world
PlacementSettings settings;
settings.setRotation(90);
templ->place(world, pos, settings, rng, flags);
```

## Surface Builders (12 types)

| Builder | Description |
|---------|-------------|
| DefaultSurfaceBuilder | Standard grass/dirt surface |
| MountainSurfaceBuilder | Stone and snow at high elevations |
| DesertSurfaceBuilder | Sand and sandstone |
| SwampSurfaceBuilder | Clay patches in swamp areas |
| FrozenOceanSurfaceBuilder | Ice on frozen oceans |
| BadlandsSurfaceBuilder | Terracotta and red sand |
| BeachSurfaceBuilder | Sand near water level |
| GiantTreeTaigaSurfaceBuilder | Podzol and coarse dirt |
| ShatteredSavannaSurfaceBuilder | Stone patches |
| BambooJungleSurfaceBuilder | Podzol in bamboo areas |
| NetherForestsSurfaceBuilder | Netherrack base |
| SoulSandValleySurfaceBuilder | Soul sand and soul soil |

## Placement Modifiers (13 types)

| Placement | Description |
|-----------|-------------|
| CountPlacement | Multiple placements per chunk |
| ChancePlacement | Probability-based placement |
| HeightRangePlacement | Y-coordinate constraints |
| BiomePlacement | Biome filtering |
| NoisePlacement | Noise threshold-based |
| CountNoisePlacement | Noise-controlled count |
| DepthAveragePlacement | Around a baseline depth |
| TopSolidPlacement | On highest solid block |
| CarvingMaskPlacement | At carved positions |
| RandomOffsetPlacement | Random position offset |
| WaterDepthThresholdPlacement | Max water depth check |
| SeaLevelPlacement | At sea level |
| SpreadPlacement | Spread around position |

## Rendering System

The client uses the Trident Vulkan rendering engine with a platform-agnostic API layer.

### Architecture

```
client/renderer/
├── api/                    # Platform-agnostic interface (100%)
│   ├── IRenderEngine.hpp   # Main engine interface
│   ├── Types.hpp           # Vertex, ModelVertex, GuiVertex, IndexType
│   ├── BlendMode.hpp       # Blend modes
│   ├── CompareOp.hpp       # Depth comparison
│   ├── CullMode.hpp        # Face culling
│   ├── TridentApi.hpp      # Unified include
│   ├── buffer/IBuffer.hpp  # Buffer interfaces
│   ├── camera/             # Camera interface and config
│   ├── mesh/MeshData.hpp   # Mesh data structures
│   ├── pipeline/           # Pipeline, RenderState, RenderType
│   └── texture/            # Texture interfaces
├── trident/                # Vulkan implementation (100%)
│   ├── core/               # Core Vulkan components
│   │   ├── TridentContext.hpp/cpp
│   │   ├── TridentEngine.hpp/cpp   # Implements IRenderEngine
│   │   ├── TridentSwapchain.hpp/cpp
│   │   ├── buffer/TridentBuffer.hpp/cpp
│   │   ├── pipeline/TridentPipeline.hpp/cpp
│   │   ├── render/RenderPassManager.hpp, FrameManager.hpp
│   │   ├── render/DescriptorManager.hpp, UniformManager.hpp
│   │   └── texture/TridentTexture.hpp/cpp
│   ├── chunk/              # ChunkRenderer, ChunkMesher, AmbientOcclusionCalculator
│   ├── cloud/              # CloudRenderer (Fast/Fancy)
│   ├── entity/             # EntityRenderer, EntityPipeline, models
│   ├── fog/                # FogManager (Linear/Exp2 fog)
│   ├── gui/                # GuiRenderer, texture atlases, sprite system
│   ├── item/               # ItemRenderer
│   ├── particle/           # ParticleManager, RainParticle, SnowParticle
│   ├── postprocess/        # Post-processing effects
│   ├── sky/                # SkyRenderer, sun/moon/stars
│   ├── util/               # VulkanUtils
│   └── weather/            # WeatherRenderer
├── mesh/                   # MeshWorkerPool (async mesh building)
├── Camera.hpp/cpp          # Camera controller (implements ICamera)
└── MeshTypes.hpp/cpp       # Mesh types
```

### Sub-Renderers

| Renderer | Location | Description |
|----------|----------|-------------|
| ChunkRenderer | `trident/chunk/` | Chunk mesh GPU buffers, async upload |
| ChunkMesher | `trident/chunk/` | Mesh generation with face culling |
| AmbientOcclusionCalculator | `trident/chunk/` | AO for smooth lighting |
| SkyRenderer | `trident/sky/` | Sky dome, sun, moon, stars |
| CloudRenderer | `trident/cloud/` | Fast/Fancy cloud rendering |
| FogManager | `trident/fog/` | Linear/Exp2 fog, underwater/lava fog |
| WeatherRenderer | `trident/weather/` | Rain/snow layer rendering |
| ParticleManager | `trident/particle/` | Particle system |
| EntityRenderer | `trident/entity/` | Entity rendering with models |
| GuiRenderer | `trident/gui/` | GUI text, rectangles, sprites |
| ItemRenderer | `trident/item/` | Item icons in GUI |

## Resource Pack System

The resource pack system parses standard Minecraft resource pack format with a compatibility layer for MC 1.12 through MC 1.19+.

### Architecture

```
common/resource/
├── ResourceLocation.hpp     # Resource identifier (namespace:path)
├── IResourcePack.hpp        # Resource pack interface
├── FolderResourcePack.hpp   # Folder-based pack
├── ZipResourcePack.hpp      # ZIP-based pack
├── InMemoryResourcePack.hpp # Built-in vanilla resources
├── PackMetadata.hpp         # pack.mcmeta parsing
├── ResourcePackList.hpp     # Multi-pack management
├── VanillaResources.hpp     # Built-in vanilla models/blockstates
├── compat/                  # MC version compatibility layer
│   ├── PackFormat.hpp       # Version detection (1.6 to 1.19+)
│   ├── TextureMapper.hpp    # 250+ texture name mappings
│   ├── ResourceMapper.hpp   # Abstract mapper interface
│   ├── v1_12/               # MC 1.12 resource mapping
│   ├── v1_13/               # MC 1.13+ resource mapping
│   └── unified/             # Unified intermediate representation
└── loader/                  # Resource loading pipeline
    └── ResourceLoader.hpp   # Format detection, loading
```

### Pack Format Support

| Format | MC Version | Texture Paths |
|--------|------------|---------------|
| 1 | 1.6-1.8 | Legacy |
| 2 | 1.9-1.10 | Legacy |
| 3 | 1.11-1.12 | `textures/blocks/`, `textures/items/` |
| 4 | 1.13-1.14 | `textures/block/`, `textures/item/` (flattening) |
| 5 | 1.15-1.16.1 | Modern |
| 6 | 1.16.2-1.16.5 | Modern |
| 7 | 1.17 | Modern |
| 8 | 1.18 | Modern |
| 9 | 1.19 | Modern |

### Texture Name Mappings (250+ bidirectional)

- Logs: `log_jungle` ↔ `jungle_log`, `log_oak` ↔ `oak_log`
- Leaves: `leaves_jungle` ↔ `jungle_leaves`
- Wool: `wool_colored_white` ↔ `white_wool`
- Stone: `stone_granite` ↔ `granite`, `stone_andesite` ↔ `andesite`
- Flowers: `flower_rose` ↔ `poppy`, `flower_houstonia` ↔ `azure_bluet`
- Terracotta: `hardened_clay_stained_white` ↔ `white_terracotta`

## Network Layer

### Packet Types

| Direction | Types |
|-----------|-------|
| Client→Server | LoginRequest, PlayerMove, TeleportConfirm, ChatMessage, BlockInteraction, PlayerTryUseItemOnBlock |
| Server→Client | LoginResponse, PlayerSpawn, PlayerDespawn, ChunkData, UnloadChunk, BlockUpdate, Teleport, ChatBroadcast, TimeUpdate, GameStateChange, SpawnEntity, SpawnMob |
| Internal | Handshake, KeepAlive, Disconnect |

### Key Classes

- **Packet**: Base class with `serialize()` and `deserialize()`
- **PacketSerializer/Deserializer**: Binary serialization with VarInt/VarLong support
- **IServerConnection**: Connection interface (TCP or Local)
- **LocalEndpoint/LocalConnectionPair**: Process-internal IPC for integrated server
- **ChunkSyncManager**: Player chunk subscription management
- **PlayerChunkTracker**: Track loaded chunks per player

## Entity System

### Base Classes

- **Entity**: Base entity class with position, rotation, velocity
- **LivingEntity**: Entity with health, attributes, effects
- **Mob**: Living entity with AI goals
- **Player**: Player entity with inventory, abilities
- **ServerPlayer/ClientPlayer**: Server/client-specific player implementations

### AI System

```
entity/ai/
├── controller/         # Entity controllers
│   ├── LookController.hpp
│   ├── MoveController.hpp
│   └── JumpController.hpp
├── goal/               # Goal-based AI
│   ├── Goal.hpp        # Goal base class
│   ├── GoalSelector.hpp
│   └── goals/          # Specific goals
│       ├── RandomWalkingGoal.hpp
│       ├── LookAtPlayerGoal.hpp
│       ├── SwimGoal.hpp
│       └── ...
└── pathfinding/        # Pathfinding system
    ├── PathNavigator.hpp
    ├── NodeProcessor.hpp
    └── PathFinder.hpp
```

### Animal Entities

- **Pig**, **Cow**, **Sheep**, **Chicken** - Passive animals with breeding

### Entity Attributes

- `MAX_HEALTH`, `FOLLOW_RANGE`, `KNOCKBACK_RESISTANCE`
- `MOVEMENT_SPEED`, `ATTACK_DAMAGE`, `ATTACK_SPEED`
- `LUCK`, `ARMOR`, `ARMOR_TOUGHNESS`

## Item System

### Item Types

- **Item**: Base item class
- **BlockItem**: Places blocks
- **ToolItem**: Pickaxe, Axe, Shovel, Hoe, Sword
- **FoodItem**: Edible items
- **ArmorItem**: Wearable armor

### Crafting System

```
item/crafting/
├── IRecipe.hpp           # Recipe interface
├── ShapedRecipe.hpp      # Shaped crafting
├── ShapelessRecipe.hpp   # Shapeless crafting
├── SmeltingRecipe.hpp    # Furnace recipes
├── RecipeManager.hpp     # Recipe registry
└── Ingredient.hpp        # Recipe ingredient matching
```

### Enchantment System

- **Enchantment**: Base enchantment class
- **EnchantmentType**: ARMOR, WEAPON, DIGGER, etc.
- **Enchantments**: Protection, Sharpness, Efficiency, Unbreaking, Fortune, etc.

## UI System (Kagero Framework)

### Architecture

```
ui/kagero/
├── event/               # Event system
│   ├── Event.hpp
│   ├── EventBus.hpp
│   ├── InputEvents.hpp
│   └── WidgetEvents.hpp
├── layout/              # Layout system
│   ├── algorithms/      # FlexLayout, GridLayout, AnchorLayout
│   ├── constraints/     # Layout constraints
│   ├── core/            # LayoutEngine
│   └── integration/     # Widget adaptors
├── paint/               # Paint abstraction
│   ├── Color.hpp
│   ├── Geometry.hpp
│   ├── PaintContext.hpp
│   └── contracts/       # ICanvas, IImage, IPaint, IPath, ISurface
├── state/               # State management
│   ├── ReactiveState.hpp
│   ├── StateBinding.hpp
│   ├── StateObserver.hpp
│   └── StateStore.hpp
├── template/            # Template system
│   ├── binder/          # Binding context
│   ├── bindings/        # Built-in bindings
│   ├── compiler/        # Template compiler
│   ├── core/            # Template core
│   ├── parser/          # AST/Lexer/Parser
│   └── runtime/         # Runtime execution
└── widget/              # Widget components
    ├── Widget.hpp
    ├── ButtonWidget.hpp
    ├── CheckboxWidget.hpp
    ├── ContainerWidget.hpp
    ├── ListWidget.hpp
    ├── ScrollableWidget.hpp
    ├── SliderWidget.hpp
    ├── TextFieldWidget.hpp
    ├── TextWidget.hpp
    └── Viewport3DWidget.hpp
```

## Performance Tracing (Perfetto)

### Configuration

```cpp
// Compile-time switches
MC_ENABLE_TRACING      // Master switch
MC_TRACE_RENDERING     // Rendering subsystem
MC_TRACE_GAME_TICK     // Game tick
MC_TRACE_CHUNK_GENERATION
MC_TRACE_CHUNK_LOAD
MC_TRACE_NETWORK
MC_TRACE_IO
MC_TRACE_MEMORY
```

### Trace Categories

- `rendering.*` - Frame, Vulkan, chunk mesh, entity, GUI, sky, etc.
- `game.*` - Tick, entity, physics, AI
- `world.*` - Chunk, biome, generation stages
- `network.*` - Packet, sync, connection
- `server.*` - Server tick, player, world, entity

### Usage

```cpp
#include "perfetto/TraceEvents.hpp"

// Initialize
mc::perfetto::TraceConfig config;
config.outputPath = "trace.perfetto-trace";
mc::perfetto::PerfettoManager::instance().initialize(config);
mc::perfetto::PerfettoManager::instance().startTracing();

// Scoped event
MC_TRACE_EVENT("rendering.frame", "RenderFrame");

// Counter
MC_TRACE_COUNTER("rendering.frame", "FPS", fps);

// Cleanup
mc::perfetto::PerfettoManager::instance().stopTracing();
mc::perfetto::PerfettoManager::instance().shutdown();
```

## Error Handling Pattern

Use `Result<T>` for fallible operations:

```cpp
// Returning a value or error
Result<int> divide(int a, int b) {
    if (b == 0) {
        return Error(ErrorCode::InvalidArgument, "Division by zero");
    }
    return a / b;  // Implicit conversion
}

// Checking result
auto result = divide(10, 2);
if (result.success()) {
    int value = result.value();
} else {
    // Handle error
}
```

### Error Codes

| Category | Codes |
|----------|-------|
| General | Unknown, InvalidArgument, NullPointer, OutOfRange, Overflow, OutOfBounds, InvalidState, InvalidData, NotInitialized |
| Resource | NotFound, AlreadyExists, ResourceExhausted, OutOfMemory |
| File | FileNotFound, FileOpenFailed, FileReadFailed, FileWriteFailed, FileCorrupted, DecompressionFailed |
| Network | ConnectionFailed, ConnectionClosed, ConnectionTimeout, InvalidPacket, ProtocolError |
| Game | InvalidBlock, InvalidItem, InvalidEntity, InvalidPlayer, InvalidWorld |
| Render | InitializationFailed, OperationFailed, CapacityExceeded, Unsupported |
| Permission | PermissionDenied, Unauthorized |
| ResourcePack | ResourcePackNotFound, ResourcePackInvalid, ResourceNotFound, ResourceParseError, TextureLoadFailed, TextureAtlasFull, ModelNotFound, BlockStateNotFound |

## Naming Conventions

- **Namespaces**: lowercase (`mc`, `mc::client`, `mc::server`)
- **Classes/Structs**: PascalCase (`ChunkManager`, `Vector3`)
- **Functions**: camelCase (`loadChunk`, `getPlayerName`)
- **Member variables**: `m_` prefix + camelCase (`m_health`, `m_position`)
- **Constants**: UPPER_SNAKE_CASE (`MAX_PLAYERS`, `CHUNK_WIDTH`)
- **Enum values**: PascalCase (`BlockType::Stone`, `ErrorCode::NotFound`)
- **Files**: PascalCase (`ChunkManager.hpp`, `ChunkManager.cpp`)

## Include Order

1. Corresponding header file (for .cpp files)
2. Project internal headers
3. Third-party library headers
4. Standard library headers

## Dependencies

Managed via vcpkg:
- **glm** - Math library
- **spdlog** - Logging
- **nlohmann-json** - JSON parsing
- **glfw3** - Window/input
- **Vulkan** - Graphics API
- **VulkanMemoryAllocator** - GPU memory management
- **asio** - Networking (async I/O)
- **GTest** - Testing framework
- **stb** - Image loading
- **perfetto** - Performance tracing

## Code Style

- C++17 standard
- `#pragma once` for header guards
- `[[nodiscard]]` for functions returning values that must be checked
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Use `const&` for large object parameters
- Use `string_view` for read-only string parameters

## Random Module

```cpp
#include "util/math/random/Random.hpp"

mc::math::Random rng(seed);
i32 value = rng.nextInt(100);    // [0, 100)
i32 range = rng.nextInt(10, 20); // [10, 20]
f32 f = rng.nextFloat();          // [0.0, 1.0)
f32 g = rng.nextGaussian(0.0, 1.0); // Normal distribution
```

Available algorithms (selected via macro):
- `Mt19937Random` - Mersenne Twister (highest compatibility)
- `Xoroshiro128ppRandom` - xoroshiro128++ (default, high performance)
- `Xoshiro256ppRandom` - xoshiro256++ (high quality)
- `LcgRandom` - Linear Congruential (minimal state)

## Important Notes

### MC Java Source Reference
You can access MC Java 1.16.5 source code at `D:\Minecraft\MC研究\Minecraft1.16.5源码\net\minecraft` for reference. This project aims to replicate Java Edition gameplay as closely as possible.

### Code Quality
- Assertions and unit tests are required
- Doc comments on every method
- Test coverage must be 95%+
- "Test as contract" principle

### Directory Structure
Maintain clean, elegant directory structure with proper subdirectories. Never dump many files in one directory.

### Build Warnings
All compilation warnings must be resolved.

### Namespace Usage
Use nested namespaces to isolate subsystems:
```cpp
namespace mc {
namespace entity {
namespace attribute {
enum class Operation : u8 { ... };
}}}
```

## Current Status

| Module | Status | Description |
|--------|--------|-------------|
| **Core** | Complete | Types, math, error handling, settings |
| **Network** | Complete | TCP server, packet serialization, LocalConnection |
| **Server Core** | Complete | Modular design with managers |
| **World Generation** | Complete | MC 1.16.5 terrain generation |
| **Chunk System** | Complete | ChunkData, ChunkHolder, async generation |
| **Biome System** | Complete | 170 biomes, layer-based distribution |
| **Block System** | Complete | BlockRegistry, BlockState, properties |
| **Fluid System** | Complete | Water, lava flow mechanics |
| **Lighting System** | Complete | Sky/block light propagation |
| **Entity System** | Complete | Base entities, AI goals, pathfinding |
| **Item System** | Complete | Items, tools, armor, food |
| **Crafting System** | Complete | Shaped, shapeless, smelting recipes |
| **Enchantment System** | Complete | All vanilla enchantments |
| **Trident Renderer** | Complete | Vulkan rendering engine |
| **API Layer** | Complete | Platform-agnostic interfaces |
| **Sub-Renderers** | Complete | Chunk, sky, cloud, fog, weather, entity, GUI, particle |
| **Resource Pack** | Complete | MC 1.12-1.19+ compatibility |
| **Performance Tracing** | Complete | Perfetto integration |
| **Kagero UI** | Complete | Full UI framework with layout, state, templates |
| **Physics** | Complete | Collision detection, AABB |
| **Tree Generation** | Complete | 6 TrunkPlacers, 9 FoliagePlacers, 9 tree types |
| **Structure Generation** | Complete | Jigsaw system, 8 structure types |
| **Surface Builders** | Complete | 12 surface builder types |
| **Placement System** | Complete | 13 placement modifiers |
| **Carvers** | Complete | Cave, Canyon, Underwater carvers |
| **Tests** | **2906 passing** | 399 test suites, all passing |

## World Generation Modules Summary

### Completed Systems

| System | Files | Description |
|--------|-------|-------------|
| Tree Generation | 8 files | 6 TrunkPlacers, 9 FoliagePlacers, 9 tree types |
| Structure Generation | 12 files | Jigsaw system, 8 structure types |
| Surface Builders | 1 file | 12 surface builder implementations |
| Placement Modifiers | 4 files | 13 placement types |
| Carvers | 4 files | Cave, Canyon, Underwater carvers |
| Biome Layers | 7 files | 170 biomes, layer-based distribution |
| Noise Generators | 2 files | ImprovedNoise, OctavesNoise |
| Chunk Generation | 2 files | NoiseChunkGenerator with MC 1.16.5 algorithm |

## Self-Maintenance Rule

**After every major change** (new model, new page, new controller, route changes, migration changes, new test files, architectural shifts), update this CLAUDE.md file to reflect the current state. Specifically:

- Add new models/controllers/pages/routes to the relevant tables below
- Update test count if new tests are added
- Add any new gotchas or patterns to the "Gotchas & Pitfalls" section
- Update the "Current Status" section if the status changes
- Keep this file as the single source of truth for AI sessions working on this project
