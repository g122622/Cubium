# World Module

The `world` module is the core of Cubium's world simulation, providing terrain generation, block/fluid management, lighting systems, chunk handling, and world state management. This module implements Minecraft 1.16.5 compatible world mechanics.

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
├── border/                     # World border system
│   ├── WorldBorder.hpp/cpp     # World border management
│   └── README.md               # Border module documentation
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
├── storage/                    # World persistence
│   ├── LevelDatCodec.hpp/cpp   # level.dat NBT codec
│   ├── WorldListEntry.hpp/cpp  # World list entry model
│   ├── WorldListService.hpp/cpp # World list service
│   ├── WorldNameSanitizer.hpp/cpp # Directory name sanitization
│   ├── WorldRequests.hpp/cpp   # World operation requests
│   ├── WorldSessionLock.hpp/cpp # Session lock (RAII)
│   └── WorldStoragePaths.hpp/cpp # Save directory paths
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
    virtual bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) = 0;

    // Fluid access
    virtual const FluidState* getFluidState(i32 x, i32 y, i32 z) const = 0;

    // Chunk access
    virtual const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const = 0;

    // Light queries
    virtual u8 getBlockLight(i32 x, i32 y, i32 z) const = 0;
    virtual u8 getSkyLight(i32 x, i32 y, i32 z) const = 0;
    virtual u8 getLightSubtracted(const BlockPos& pos, u32 skyDarkening) const;
    virtual u8 getNeighborAwareLightSubtracted(const BlockPos& pos, u32 skyDarkening) const;
    virtual u8 getLight(const BlockPos& pos) const;
    virtual i32 getSkyDarkening() const;
    virtual f32 getBrightness(const BlockPos& pos) const;  // Returns 0.0-1.0
    virtual bool canSeeSky(const BlockPos& pos) const;  // Based on sky light >= 15

    // Collision detection
    virtual bool hasBlockCollision(const AxisAlignedBB& box) const = 0;
    virtual std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const = 0;

    // Entity queries
    virtual std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except) const = 0;

    // 最近玩家查询（MC 1.16.5 World.getClosestPlayer）
    virtual Player* getClosestPlayer(const Vector3& pos, f32 maxDistance = -1.0f);
    virtual const Player* getClosestPlayer(const Vector3& pos, f32 maxDistance = -1.0f) const;
    virtual Player* getClosestPlayer(const Vector3& pos, f32 maxDistance, const Entity* exclude);
    virtual const Player* getClosestPlayer(const Vector3& pos, f32 maxDistance, const Entity* exclude) const;
    virtual f64 getClosestPlayerDistanceSq(const Vector3& pos) const;

    // 维度上下文
    virtual bool isUltraWarm() const = 0;

    // 实体状态广播（默认空实现，ServerWorld 重写）
    virtual void broadcastEntityStatus(EntityId entityId, u8 status);

    // 方块实体管理（默认空实现，ServerWorld 重写）
    virtual BlockEntity* getBlockEntity(const BlockPos& pos);
    virtual const BlockEntity* getBlockEntity(const BlockPos& pos) const;
    virtual void setBlockEntity(const BlockPos& pos, BlockEntity* entity);
    virtual void removeBlockEntity(const BlockPos& pos);

    // 成就事件通知（默认空实现，ServerWorld 重写）
    virtual void onBlockPlaced(PlayerId playerId, const BlockPos& pos,
                               const BlockState* state, const ItemStack* item);
    virtual void onZombieVillagerCured(const std::string& starterUuid, Entity* zombie, Entity* villager);
    virtual void onPlayerDestroyItem(PlayerId playerId, const ItemStack& item, i32 slot, Hand hand);

    // ========== 命令执行 ==========
    // 执行命令，返回命令执行结果（成功返回正值，失败返回 0）
    // 用于命令方块矿车等实体执行命令，通过回调机制将命令执行委托给服务器
    // position: 命令执行位置（通常是实体的位置）
    // permissionLevel: 权限级别（命令方块矿车使用 2）
    [[nodiscard]] virtual i32 executeCommand(const std::string& command,
        const Vector3d& position,
        i32 permissionLevel)
    {
        (void)command;
        (void)position;
        (void)permissionLevel;
        return 0;
    }
};```

`IWorld` 现在还提供一组面向方块位置语义的 `BlockPos` 便捷重载，适合已经持有位置对象的调用点直接使用：

- `getBlockState(const BlockPos&)`
- `getFluidState(const BlockPos&)`
- `hasFluid(const BlockPos&)`
- `isWaterAt(const BlockPos&)`
- `isLavaAt(const BlockPos&)`
- `getBlockLight(const BlockPos&)`
- `getSkyLight(const BlockPos&)`
- `getLightSubtracted(const BlockPos&, u32 skyDarkening)`
- `getNeighborAwareLightSubtracted(const BlockPos&, u32 skyDarkening)`
- `getLight(const BlockPos&)`
- `getBrightness(const BlockPos&)`
- `isWithinWorldBounds(const BlockPos&)`

`ServerWorld` 会通过 `using IWorld::...` 重新暴露这些重载，避免自身的 xyz 成员把它们隐藏掉。像 `ISpawnWorldReader`、`ClientWorld`、`StarLightEngine` 这类非 `IWorld` surface 仍然保持原始 xyz 签名，不要强行把它们改成 `BlockPos` 风格。

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

### World Border System

世界边界系统限制玩家的活动范围，提供边界伤害和渐变动画。

**核心功能：**
- 边界大小设置（立即设置/线性插值过渡）
- 边界中心设置
- 伤害参数配置（每格伤害量、伤害缓冲距离）
- 警告参数配置（警告时间、警告距离）
- 边界检测方法（点检测、AABB 检测、区块检测）

```cpp
#include "common/world/border/WorldBorder.hpp"

mc::world::border::WorldBorder border;

// 设置边界大小
border.setSize(1000.0);  // 立即设置为 1000 格

// 渐变设置边界大小
border.setSizeLerp(1000.0, 500.0, 60000);  // 60秒内从 1000 缩小到 500

// 设置边界中心
border.setCenter(100.0, 200.0);

// 设置伤害参数
border.setDamagePerBlock(0.2);  // 每格 0.2 伤害
border.setDamageBuffer(5.0);    // 5 格缓冲区

// 设置警告参数
border.setWarningTime(15);      // 15 秒警告时间
border.setWarningDistance(5);   // 5 格警告距离

// 检测点是否在边界内
bool inside = border.contains(x, z);

// 获取点到边界的距离（正数=在内，负数=在外）
double distance = border.getClosestDistance(x, z);
```

**状态模式：**

边界大小使用状态模式实现：
- `StationaryBorderState`：静止边界，固定大小
- `MovingBorderState`：移动边界，线性插值过渡

状态转换：
- `setSize()` 创建静止状态
- `setSizeLerp()` 创建移动状态
- `tick()` 更新移动状态，过渡完成后转为静止状态

**监听器模式：**

`IBorderListener` 接口用于网络同步事件：

```cpp
class MyListener : public IBorderListener {
    void onSizeChanged(double newSize) override {
        // 发送 WorldBorderPacket(SetSize)
    }
    void onTransitionStarted(double oldSize, double newSize, u64 timeMs) override {
        // 发送 WorldBorderPacket(LerpSize)
    }
    void onCenterChanged(double x, double z) override {
        // 发送 WorldBorderPacket(SetCenter)
    }
    // ...
};
```

**玩家边界伤害：**

在 `Player::tick()` 中检测玩家是否越界：

```cpp
// 参考 MC 1.16.5 LivingEntity.baseTick() 第306-318行
if (m_world != nullptr && !isSpectator() && !m_abilities.invulnerable) {
    const auto& border = m_world->worldBorder();
    if (!border.intersects(boundingBox())) {
        double distance = border.getClosestDistance(boundingBox()) + border.getDamageBuffer();
        if (distance < 0.0 && border.getDamagePerBlock() > 0.0) {
            i32 damage = std::max(1, static_cast<i32>(std::floor(-distance * border.getDamagePerBlock())));
            hurt(DamageSources::inWall(), static_cast<f32>(damage));
        }
    }
}
```

**伤害计算：**
- 距离 = `getClosestDistance(entity) + damageBuffer`
- 如果 距离 < 0，造成伤害：`max(1, floor(-距离 * damagePerBlock))`
- 示例：越界 10 格，damageBuffer = 5，damagePerBlock = 0.2
  - 距离 = -10 + 5 = -5
  - 伤害 = max(1, floor(5 * 0.2)) = 1

### WorldEvents 系统

`WorldEvents` 命名空间定义了世界事件ID常量，用于 `playEvent()` 方法触发音效和粒子效果。

**事件分类：**

| 范围 | 类型 | 示例 |
|------|------|------|
| 1000-1039 | 音效事件 | 门开关、唱片、铁砧、传送门等 |
| 1500-1503 | 特殊效果事件 | 堆肥桶、岩浆熄灭、红石火把熄灭等 |
| 2000-2008 | 粒子/效果事件 | 发射器烟雾、方块破坏、药水效果等 |
| 3000-3001 | 末地传送门事件 | 传送门生成效果、末影人咆哮 |

**常用事件ID：**

```cpp
#include "common/world/WorldEvents.hpp"

using namespace mc::world::WorldEvents;

// 门音效
IRON_DOOR_OPEN_SOUND      // 1005
IRON_DOOR_CLOSE_SOUND     // 1011
WOODEN_DOOR_OPEN_SOUND    // 1006
WOODEN_DOOR_CLOSE_SOUND   // 1012

// 方块效果
BREAK_BLOCK_EFFECTS       // 2001 - data 为方块状态ID
DISPENSER_SMOKE           // 2000 - data 为方向
BONEMEAL_PARTICLES        // 2005 - data 为粒子数量

// 特殊事件
COMPOSTER_FILLED_UP       // 1500 - data>0 有空间, data<=0 已满
END_PORTAL_FRAME_FILL     // 1503 - 放置末影之眼
PORTAL_TRAVEL_SOUND       // 1032 - 传送门传送
```

**参考：**
- MC 1.16.5 `net.minecraft.client.renderer.WorldRenderer.playEvent`
- MC 1.16.5 `net.minecraftforge.common.util.Constants.WorldEvents`

### Storage System

World persistence layer for save/load functionality:

```cpp
#include "world/storage/WorldListService.hpp"

// List worlds
WorldStoragePaths paths(baseDir);
WorldListService service(paths);
auto worlds = service.listWorlds();

for (const auto& entry : worlds.value()) {
    std::cout << entry.displayName << " (" << entry.levelId << ")\n";
    std::cout << "  Last played: " << entry.lastPlayedMs << "\n";
    std::cout << "  Seed: " << entry.seed << "\n";
}

// Create new world
CreateWorldRequest request;
request.displayName = "New World";
request.seed = 12345;
auto levelId = service.createWorld(request);

// 服务器运行时保存
// flushAllDirty()：增量刷新脏 Section
// saveAll()：全量保存当前缓存的 Section
```

**Key Components:**
- **GlobalStorageManager**: 跨存档发现、列表、路径解析与打开入口
- **SingleLevelStorageManager**: 单存档持久化门面，暴露 `loadChunk()`、`saveChunk()`、`flushAllDirty()`、`saveAll()`、玩家数据与自动保存接口
- **LevelDatCodec**: Reads/writes gzip-compressed NBT `level.dat` files
- **WorldListService**: Enumerates, creates, deletes, backs up worlds
- **WorldSessionLock**: RAII lock to prevent concurrent world access
- **WorldNameSanitizer**: Handles directory naming conflicts (World, World (1), etc.)
- **WorldStoragePaths**: Manages saves/ and backups/ directory paths
- **SectionManager**: Per-dimension Section cache/load/save manager
- **AutoSave**: 周期性脏数据刷新，已内聚进 `SingleLevelStorageManager` 并接入 `ServerWorld`、`/save-on`、`/save-off`

**生命周期规则补充：**
- `SingleLevelStorageManager` 是共享存储门面，关闭流程中的全量保存应由上层统一显式调用，而不是依赖内部子管理器或析构函数偷偷补保存。
- `PlayerDataManager` 这类子管理器析构时不再隐式落盘，避免与共享存储关闭流程重复。

See `storage/README.md` for detailed documentation.

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
- `common/core/Types.hpp` - Primitive types (i8, i16, i32, std::string, etc.)
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
const i32 radius = 8;
std::vector<IChunk*> neighbors = {...};
WorldGenRegion region(chunkX, chunkZ, radius, std::move(neighbors));
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

// Check if position can see sky (based on sky light level)
bool outdoor = world.canSeeSky(pos);  // Returns true if skyLight >= 15

// Height query
i32 height = world.getHeight(x, z);  // Top solid block

// Light level calculation for mob spawning (MC 1.16.5)
u8 light = world.getLight(pos);  // Uses current sky darkening
u8 neighborLight = world.getNeighborAwareLightSubtracted(pos, 10);  // Thundering uses fixed darkening 10
i32 darkening = world.getSkyDarkening();  // Calculate from dayTime, rain, thunder
```

**Light Calculation Methods (MC 1.16.5):**

| Method | Purpose | Formula |
|--------|---------|---------|
| `getLightSubtracted(pos, skyDarkening)` | Basic light calculation | `max(blockLight, skyLight - skyDarkening)` |
| `getNeighborAwareLightSubtracted(pos, skyDarkening)` | Light with world border check | Returns 15 outside bounds, otherwise `getLightSubtracted` |
| `getLight(pos)` | Current light level | `getNeighborAwareLightSubtracted(pos, getSkyDarkening())` |
| `getSkyDarkening()` | Sky darkening factor | Based on `dayTime`, `isRaining()`, `isThundering()` |

**Sky Darkening Calculation:**

The sky darkening factor determines how much the sky light is reduced:
- Noon (dayTime=6000): darkening ≈ 0
- Midnight (dayTime=18000): darkening ≈ 11
- Rain: additional darkening
- Thunder: darkening = 10 (fixed, allows monster spawning during day)

**Monster Spawning Light check:**

MC 1.16.5 uses a two-stage light check for monster spawning:

```cpp
// Stage 1: Quick sky light check
if (skyLight > random.nextInt(32)) return false;

// Stage 2: Comprehensive light check
u8 light;
if (world.isThundering()) {
    // Thundering: fixed sky darkening of 10
    light = world.getNeighborAwareLightSubtracted(pos, 10);
} else {
    // Normal: use current time-based sky darkening
    light = world.getLight(pos);
}
return light <= random.nextInt(8);
```

**canSeeSky Implementation:**

The `canSeeSky()` method checks if a position has direct line of sight to the sky. This is implemented based on MC 1.16.5:

```cpp
bool canSeeSky(const BlockPos& pos) const {
    if (!hasSkyLight()) {
        return false;  // Nether/End have no sky light
    }
    return getSkyLight(pos) >= 15;
}
```

This correctly handles:
- Transparent blocks (glass, water) - sky light passes through
- Partial blocks (slabs, stairs) - uses shape-based occlusion
- Indoor positions - sky light is blocked by solid blocks

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
chunk->getBlockState(localX, y, localZ);  // May crash if chunk not loaded

// GOOD
if (chunk && chunk->getStatus() == ChunkLoadStatus::Full) {
    chunk->getBlockState(localX, y, localZ);
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

### 9. IWorld BlockPos Overloads

**问题**：非 `IWorld` 接口强制添加 `BlockPos` 重载会导致接口膨胀。

**解决方案**：`IWorld` 现在暴露了 `BlockPos` 重载用于方块位置语义，只要调用者已有 `BlockPos` 就应优先使用，并通过 `using IWorld::...` 重导出重载集。但是 `ISpawnWorldReader`、`ClientWorld` 和光照/生成辅助类不属于该 `IWorld` 契约，保持这些接口使用原生 xyz 签名，不要仅仅为了镜像 `IWorld` 而在非 `IWorld` 读取器上强制添加 `BlockPos` 重载。

### 10. ServerWorld setBlockState Tests

**问题**：测试 `ServerWorld::setBlockState()` 时，未初始化的世界会触发光照更新断言路径。

**解决方案**：测试前先初始化世界，否则 `m_lightManager` 为 null 会触发 `MC_ASSERT_RELEASE(false)` 断言。

### 11. BlockUpdatePacket Sending

**问题**：直接从服务器应用程序代码发送 `BlockUpdatePacket` 会绕过去重和批处理逻辑。

**解决方案**：不要直接从服务器应用程序代码发送 `BlockUpdatePacket`。`ServerWorld::setOnBlockChanged()` 现在供给 `BlockUpdateSyncManager`；同坐标去重和 tick 结束刷新必须保持集中化。

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
| `border/WorldBorderTest.cpp` | World border size, center, damage, lerp |
| `LightLevelTest.cpp` | Light calculation methods (getLightSubtracted, getNeighborAwareLightSubtracted, getLight, getSkyDarkening) |
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
