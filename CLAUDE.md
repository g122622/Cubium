# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Minecraft Reborn is a modern Minecraft clone with client-server architecture written in C++17 using Vulkan for rendering. The project aims to replicate the Java Edition 1.16.5 experience as closely as possible while maintaining compatibility with existing Minecraft ecosystem (resource packs, world saves, data packs).

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
- `SingleChunkLifecycleManager`: Manages chunk loading state and futures
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

## Assert Library

The project provides a comprehensive assertion library for runtime checks.

### Usage

```cpp
#include "common/util/assert/AssertAll.hpp"

// Basic assertions (Debug mode only)
MC_ASSERT(ptr != nullptr);
MC_ASSERT_MSG(size > 0, "Size must be positive");

// Release mode assertions (always enabled)
MC_ASSERT_RELEASE(index < capacity);

// Fatal assertions (always enabled, for critical errors)
MC_ASSERT_FATAL(state == State::Ready);

// Comparison assertions with value output
MC_ASSERT_EQ(expected, actual);  // Shows both values on failure
MC_ASSERT_NE(ptr, nullptr);
MC_ASSERT_LT(value, max);
MC_ASSERT_LE(value, max);
MC_ASSERT_GT(value, min);
MC_ASSERT_GE(value, min);

// Pointer assertions
MC_ASSERT_NOT_NULL(obj);
MC_ASSERT_NULL(optional);

// Range assertions
MC_ASSERT_RANGE(index, 0, size - 1);
MC_ASSERT_INDEX(row, height);
MC_ASSERT_INDEX_U(index, size);  // Unsigned index

// Special assertions
MC_ASSERT_UNREACHABLE();           // Marks unreachable code
MC_ASSERT_FAIL("Critical error");  // Always fails
MC_ASSERT_NOT_IMPLEMENTED();       // Marks unimplemented code

// Precondition/postcondition assertions
MC_PRECONDITION(size > 0);
MC_POSTCONDITION(result != nullptr);
MC_INVARIANT(m_count >= 0);

// Debug-only code
MC_DEBUG_ONLY(debugLog("Checking..."));

// Unused variable marker
MC_UNUSED(unusedParam);
```

### Assert Levels

| Level | Description |
|-------|-------------|
| Debug | Only active in Debug builds |
| Release | Always active |
| Fatal | Always active, for critical errors |

### Custom Handlers

```cpp
// Set custom handler
mc::assert::AssertConfig config;
config.handler = [](const mc::assert::AssertFailure& failure) {
    // Custom handling (log, throw, etc.)
    throw mc::assert::AssertException(failure);
};
config.captureStackTrace = true;  // Enable stack traces
config.breakOnFailure = true;     // Trigger debugger breakpoint
mc::assert::AssertManager::instance().setConfig(config);
```

### Built-in Handlers

- `defaultAssertHandler()` - Output to stderr and abort
- `logAssertHandler()` - Log using spdlog and abort
- `throwAssertHandler()` - Throw `AssertException`

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
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) rather than raw pointers
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

## Gotchas & Pitfalls

- `WorldGenRegion` is not an `IBlockReader`; do not pass it directly to `Block::isValidPosition`.
    - In worldgen features, use explicit local placement checks (`isWater`, support block checks) when running in `WorldGenRegion` context.
- `WorldGenRegion` now uses stage-specific `ChunkStatus::taskRange()` windows, and `getTopBlockY()` asserts if a requested chunk is missing.
    - Do not treat out-of-window height queries as "height 0"; fix the region radius or the call site instead.
- `IWorld` now exposes `BlockPos` overloads for block-position semantics.
    - Prefer them whenever the caller already has a `BlockPos`, and keep `ServerWorld` style xyz implementations from hiding them by re-exporting the overload set with `using IWorld::...`.
- `ISpawnWorldReader`, `ClientWorld`, and lighting/generation helpers are not part of that `IWorld` contract.
    - Keep those surfaces on their native xyz signatures; do not force `BlockPos` overloads onto non-`IWorld` readers just to mirror `IWorld`.
- Blue ice placement will always fail if there is no packed-ice neighbor around the sampled start position.
    - Tests must set up packed ice at the exact sampled neighborhood, not by replacing whole water layers in a way that shifts ocean-floor detection.
- Avoid passing temporary `BlockState` copies to world write APIs.
    - Prefer canonical references returned by `state.with(...)` / `defaultState()`; `ServerWorld::setBlock` now canonicalizes by `stateId` as a safety net.
- `ChunkData::setSkyEmptinessMap(const bool* map)` / `setBlockEmptinessMap(const bool* map)` 需要拿到完整的区块段空隙图。
    - 不要再把 `std::vector<bool>` 的结果直接丢成 `nullptr`；如果上游拿到的是按段更新结果，必须先拷贝成连续的 `bool[]` 再写回区块。
- `Heightmap` 内部存储的是 `y + 1`，不是实际方块 Y。
    - 只有 `getTopBlockY()` 这一层才应该把它转换回块坐标，不要直接把原始高度图值当作方块位置。
- `WorldLightManager::tick(...)` now relies on ordered budget consumption.
    - Do not restore the previous half/half split unless the budget model is redesigned with matching tests.
- `TemptGoal` now filters real `Player` entities by their main/off-hand stacks, and `PanicGoal` / `WaterAvoidingRandomWalkingGoal` now consult `IWorld::isWaterAt(...)` / `isLavaAt(...)` directly.
    - Keep tests aligned with the world-query surface; do not fake these goals by stubbing movement alone.
- `KelpFeatureIds` and `SeagrassFeatureIds` now split by ocean temperature.
    - Keep `FeatureRegistry::initialize()` order, `BiomeGenerationSettings` mapping, and the ocean assertions in sync whenever new ocean variants are added.
- `CraftingMenu::stillValid()` now uses the player's distance to the crafting table.
    - Keep workbench accessibility tied to the block entity position so container validity matches the intended interaction range.
- `ChickenEntity::tick()` spawns an egg item entity when the timer expires.
    - Reset the timer immediately after spawning or chickens will emit eggs in bursts.
- `ChestContainer` and `FurnaceContainer` now require a real `PlayerInventory` and live under `AbstractContainerMenu`.
    - Do not route chest/furnace GUI creation through the legacy `Container` path; use the shared menu factory / open-container hook instead.
- `ContainerPacketHandler::handleContainerClick()` now depends on the active menu pointer stored on the integrated-server menu player.
    - Keep `getMenuPlayer().setOpenContainerMenu(...)` on open and `clearOpenContainerMenu()` on close, or client clicks will be dropped before they reach the menu.
- `GlassBottleItem` samples along the player's look ray before deciding whether a bottle can be filled.
    - Liquid blocks here do not provide usable collision shapes, so pure hit tests are not enough for water-source detection.
- `PaneBlock` connection shapes are cached per 4-bit mask and use normalized coordinates.
    - Do not fall back to a single center-shape placeholder or reintroduce pixel-space box coordinates; that breaks collision and rendering tests.
- Liquid face culling in `ChunkMesher` must treat empty-collision underwater plants like seagrass and kelp plant as face-hiding neighbors.
    - Do not key liquid visibility off transparency alone or you will reintroduce stray water quads around aquatic vegetation.
- `MatrixStack` call-order intuition can be misleading after PoseStack alignment.
    - In first-person rendering, apply transforms in vanilla order and rely on post-multiply semantics; avoid ad-hoc in-place row/column edits.
- `StackLayoutAlgorithm` in Kagero should stretch children on the container cross-axis, not just the line content size.
    - Stack/column tests that rely on full-width children need the adapter to honor the container width, otherwise inner bounds will stay at the intrinsic size.
- Do not share one first-person item mesh cache across both hands.
    - Main hand and off hand can hold different items in the same frame, so a single cache will thrash and allocate GPU memory every render.
- Retired first-person meshes must be reclaimed on a frame countdown, not only in `destroy()`.
    - Otherwise repeated item changes will keep old Vulkan buffers alive for the whole session.
- Do not put priority logic back into `MeshWorkerPool`.
    - Priority and cancellation policy belong to `MeshBuildScheduler`; `MeshWorkerPool` should stay execution-only.
- `ChunkMesher` 的 `generateSplitMesh()` 预留策略必须按 pass 区分。
    - 透明层的初始容量要明显小于实心层，否则双层网格会把峰值内存翻倍。
- Always update `MeshSchedulerViewState` before calling `ClientWorld::update(...)` each frame.
    - If the view state is stale, frustum priority and behind-camera cancellation will lag behind camera movement.
- `ClientApplication` 里给 `MeshBuildScheduler` 的并发预算要保持保守。
    - 不要再把 `maxDispatchedTaskCount` 按视距线性放大到很大，完成队列里每个 chunk mesh 都可能是数 MB 级别。
- Pending mesh tasks cancelled before dispatch do not produce worker results.
    - Keep `activeMeshTaskId` in sync with scheduler tracking (or chunks can get stuck with a stale task id and never be resubmitted).
- When changing `ChunkMesher` mesh APIs, update both runtime and tests together.
    - `tests/client/renderer/test_renderer.cpp` now calls the 5-argument `generateSplitMesh(..., neighbors, cancelSignal)` signature.
- Do not assume old BaseLightEngine queue bit-packing semantics are still valid.
    - Lighting queue entries now carry full world coordinates; if you reintroduce 6-bit X/Z truncation or stale decode paths, sky/block light will desync badly away from origin.
- In skylight code, be explicit about internal level meaning.
    - Internal level is inverted (`0` brightest, `15` darkest). Porting logic from vanilla/Starlight direct-light code without conversion can silently invert clear/repropagation behavior.
- For skylight roof-closure fixes, only block increase propagation from opaque source blocks.
    - Do not apply the same opaque-source gate to decrease paths, or stale light under roofs will never be removed.
- When handling `currentLevel < targetLevel` during decrease propagation, blocked-edge (`target=darkest`) cases cannot always be treated as “safe surviving source”.
    - Force clear + decrease cascade first, and enqueue adjacent increase rechecks; otherwise side skylight can be lost under FIFO wavefront execution.
- Do not switch `BaseLightEngine` queue processing back to LIFO (`--length` pop-back).
    - Starlight-style propagation depends on FIFO wavefront ordering; LIFO introduces unnecessary oscillation and delayed convergence under complex occlusion.
- `ClientWorld::entityManager()` returns `ClientEntityManager`, not the shared `common::EntityManager`.
    - 客户端本地 `Player` 不会在这条链路里跑 `Player::tick()`；客户端只会 tick 代理实体和本地物理。
- Do not assume `m_player->isInWater()` is authoritative on the client.
    - 这个值只会在 `Entity::baseTick()` / `updateEnvironmentState()` 或本地物理刷新路径里更新；它对本地玩家可用，但仍不是服务端权威结果。
- `Player::updateMoveDistance()` now uses a dedicated sampling position, and `Player::setPosition()` resets movement/bobbing state.
    - 不要再把 `prevPosition` 当成脚步声或视野晃动的采样基准；它是插值历史状态，多次物理更新会把同一段位移重复计数。
- `Player::updatePhysics()` can run in lightweight test worlds without a physics engine.
    - In that case the code falls back to direct movement, so tests should verify state refresh and not assume a full collision solver is present.
- `Entity::refreshDimensions()` is now the canonical way to rebuild an entity's cached `EntitySize` and `AxisAlignedBB`.
    - Any runtime size change must refresh the cache immediately, or movement and ground checks will keep using stale boxes.
- `DyeableArmorItem` stores color in `ItemStack`'s structured tag tree.
    - Clearing color must also clear empty `display` tags, otherwise metadata equality will diverge and armor stacks will stop merging as expected.
- Player stand-up transitions now check whether the target pose box fits before switching away from crouch/swim/sleep.
    - Do not bypass `setSneaking()` / `setSwimming()` / `setSleeping()` with a raw standing pose change when you want vanilla-like low-ceiling behavior.
- `EntityMetadataPacket` / `EntityMetadataSerializer` now feed both server tracking and client entity application.
    - `EntityTracker` 负责 spawn 内联 metadata 和 dirty metadata packet，`ClientEntity::setMetadata()` 负责把原始数据写进本地数据管理器；新增字段时三处必须一起改。
- `Entity::getTypeId()` now prefers an explicit runtime `typeId` injected during `EntityType::create(...)`.
    - 不要再依赖 `LegacyEntityType` 单独决定网络实体类型；很多工厂构造仍传 `LegacyEntityType::Unknown`，正确做法是保证实体通过注册表创建时注入注册名，繁殖等旁路也要显式继承父类型。
- Do not bootstrap `StarLightEngine::light(...)` from chunk-owned nibble arrays when running unlit initialization.
    - Mirror Moonrise: start with temporary NULL-state nibbles, run `handleEmptySectionChanges(..., isUnlit=true)`, then `lightChunk(...)`, and finally write the generated nibbles back to the chunk.
- In unlit `light(...)` bootstrap, do not seed emptiness cache from stale default maps.
    - Seed the cache as null first; otherwise `SkyStarLightEngine::initNibble(...)` can compute a wrong `lowestY` and skip expected skylight source setup.
- Do not reintroduce compatibility aliases for the lighting subsystem.
    - `StarLightEngine`, `BlockStarLightEngine`, `SkyStarLightEngine`, and `StarLightLightingProvider` are the canonical names now.
- Keep world->section conversion centralized via `LightEngineUtils::worldToSectionPos(...)`.
    - Re-introducing ad-hoc bit-decode logic in storage maps can silently route writes/reads to wrong sections.
- Do not apply source-face blocking blindly to all block-light propagation.
    - Full-cube emissive sources still need outward propagation; source-face occlusion checks should target conditional shapes.
- Do not route `BlockModelCache::getBlockAppearance(const BlockState*)` through `toModelKey()` in render hot paths.
    - The `stateId` cache is the intended fast path; rebuilding model keys there reintroduces avoidable string parsing.
- Do not reintroduce default parameters into lighting APIs.
    - Where a call pattern needs a simplified form, add an overload instead of a default argument.
- `WorldLightManager::tick(...)` now relies on ordered budget consumption.
    - Do not restore the previous half/half split unless the budget model is redesigned with matching tests.
- Do not reintroduce `BlockPos` wrapper overloads on `WorldLightManager` block-update or light-query entrypoints.
    - Raw coordinates are the canonical surface for lighting dispatch now.
- When mirroring Starlight branch flow, keep raw light-level handling local to the entrypoint and convert before queueing if the propagation core still expects internal levels.
    - Feeding raw source levels straight into the propagation queues will desynchronize the current inverse-level storage model.
- Do not treat client light packets as immediate mesh rebuild triggers.
    - `ClientWorld` now uses `meshRebuildPending` to coalesce repeated `onLightUpdate()` calls for the same chunk while a task is still active.
- Do not send `BlockUpdatePacket` directly from server application code.
    - `ServerWorld::setOnBlockChanged()` now feeds `BlockUpdateSyncManager`; same-coordinate dedupe and tick-end flush must stay centralized.
- `KeepAlivePacket::deserialize()` expects the完整包（12 字节头 + 8 字节时间戳）。
    - 服务端处理心跳响应时不要先剥掉头部，否则单人模式下会把正常的 KeepAlive 回复误报成 `Packet too small for keep alive`。
- When testing `ServerWorld::setBlock()`, initialize the world first.
    - Uninitialized worlds hit the light-update assert path (`MC_ASSERT_RELEASE(false)`) because `m_lightManager` is null.
- Command aliases should use `CommandNode::setRedirect(...)` instead of duplicating child subtrees.
    - `/teleport` and `/xp` now rely on redirects so the command tree, help output, and suggestions all stay in sync.
- `CommandDispatcher::getSuggestions()` is the canonical tab-completion entry point.
    - Attach dynamic completion data through `CommandNode::setCustomSuggestions(...)` instead of hardcoding tab lists in commands.
- `CommandRegistry::getCommandNames()` is derived from the dispatcher tree.
    - Help output automatically tracks aliases and future command registrations, so do not reintroduce a separate manual name list.
- `CommandTreePacket` is the authoritative command snapshot for the client.
    - Client-side completion must be rebuilt from `onCommandTree()` after login and cleared again on disconnect, otherwise chat suggestions will drift stale.
- `CommandTreePacket` only serializes the packet body.
    - Server code must wrap it with `ConnectionManager::encapsulatePacket()` exactly once, and client code must strip the outer network header before calling `handleCommandTree()`; double encapsulation makes the inner header look like an empty JSON string.
- `SaplingBlock` and `TreeFeature` must agree on root support blocks.
    - If you expand one side, expand the other in the same change or you will get the classic “can place but cannot grow” mismatch.
- `MushroomBlock` natural tick is for low-light spreading, not giant mushroom construction.
    - Keep giant mushroom generation in the feature layer so the block stays testable and local.
- `CactusBlock` collision damage should target living entities only.
    - Use `LivingEntity::hurt()` with `DamageSources::cactus()`; non-living collisions should stay no-op.
- Do not link `spdlog::spdlog` or `GTest::gtest` directly into executables that already consume `mc_common` or `GTest::gtest_main`.
    - Apple ld will emit duplicate-library warnings when the same static library appears twice on the final link line.
- On AppleClang, no-argument `MC_TRACE_EVENT(...)` or `MC_TRACE_*` calls can still trigger `-Wvariadic-macro-arguments-omitted`.
    - Pass an explicit dummy key/value payload when a trace site has no real arguments.
- 流体流动判定必须区分“目标流体状态”和“用于阻挡判断的流体类型”。
    - 修改 `FlowingFluid::canFlow()` / `canFlowInto()` 时，不能把所有路径都硬塞成 `*this`，否则容器方块和特殊替换规则会偏离原版语义。
- 液体方块必须继续把随机 tick 透传给流体。
    - `LiquidBlock::ticksRandomly()` 和 `LiquidBlock::randomTick()` 是岩浆火焰扩散的入口，漏掉后会出现“方块看起来对了，但行为不触发”的假正确。
- 岩浆时序是世界相关的。
    - `ServerWorld::setBlock()` 和流体 tick 调度要继续使用 `fluid.getTickDelay(*this)`，不要把主世界/下界差异重新硬编码回固定常量。
- 天气降水判定不能只看温度。
    - `WeatherUtils::canRainAt()` / `canSnowAt()` 需要结合生物群系的 `BiomeClimate::Precipitation::None` 以及温度阈值一起判断；沙漠、蘑菇岛、恶地等无降水生物群系必须在注册数据里显式标记为 `None`。
- 玩家命令反馈不要默认依赖 `ServerPlayer*`。
    - `ServerCommandSource::sendMessage()` 现在必须在只有 `playerId` 的情况下也能把消息发回在线连接，不能只写日志。
- 玩家背包同步必须使用 `PlayerInventoryPacket`。
    - `ContainerContentPacket` 只保留给真正打开的容器菜单；玩家物品栏刷新、拾取同步和 `/clear` 这类操作都应走玩家背包包。
- 创造模式物品库写回必须使用 `CreativeInventoryActionPacket`。
    - `CreativeScreen` 负责本地搜索、滚动和槽位编辑，真正落盘到服务端时必须走这个专用包，不要复用普通容器点击包。
- `CreativeInventory` 相关测试和启动代码必须按 `VanillaBlocks::initialize()` -> `Items::initialize()` -> `BlockItemRegistry::instance().initializeVanillaBlockItems()` 的顺序初始化。
    - 少了 `VanillaBlocks` 这一步时，创造物品库会出现空列表或缺失方块物品。
- `InventoryManager::setOnInventoryUpdate()` 在 `MinecraftServer::initializeInteractionManagers()` 里已经接好。
    - 服务器侧背包变更如果走 `inventoryManager()`，就要依赖这条回调刷新客户端，不要再手写一套新的同步分支。
- 冰块融化与破坏路径必须分开处理。
    - `IceBlock::randomTick()` 只负责融化，`onBlockRemoved()` 只负责破坏后的替换；不要再让随机刻回调 `onBlockRemoved()`，也不要把同一坐标的写回放在旧方块回调之前。
    - `CropBlock` 的骨粉增长必须从世界种子和方块位置派生随机数，不能再回到全局 `rand()`。
        - `FarmlandBlock` 的降雨补湿要同时看 `isRaining()` 和 `canRainAt(pos.up())`，否则测试世界里会出现伪阳性。
        - `ClientApplication` 已按功能域拆到 `src/client/application/features/`，主文件只保留编排与生命周期。
            - `setupInputBindings()` / `setupCamera()` 等逻辑已迁移，不要再往主文件补回重复实现。
        - `ClientApplicationBootstrap.cpp` 负责客户端初始化骨架，`initialize()` 只做调度。
            - 核心注册表、窗口/输入、渲染、游戏系统和 UI 初始化都应继续下沉到 bootstrap/helper 方法里。
        - `ClientApplicationHelpers` 的公共辅助函数位于 `mc::client::application::features` 命名空间。
            - 调用处必须显式限定或导入作用域，否则会出现“找不到标识符”的连锁编译错误。
        - `TargetInfoWidget` 位于 `mc::client::ui::minecraft::targetinfo`，`DebugScreenWidget` 仍位于 `mc::client::ui::minecraft`。
            - 不要把这两个命名空间混用到同一条类型解析路径里，`ClientApplication` 的目标信息刷新逻辑已经拆到 `features/`。
        - `ClientApplication::handleEvents()` 现在只做输入轮询和分流。
            - 覆盖层输入放在 `handleUiOverlayInput()`，游戏快捷键放在 `handleGameplayShortcutInput()`，玩家视角/移动放在 `handleMouseAndMovementInput()`，不要把新逻辑再塞回 `handleEvents()`。
        - `ClientApplication::handleBlockMiningInput()` 和 `handleBlockPlacementInput()` 已分开。
            - 挖掘的取消、开始、完成逻辑继续留在独立 helper 里，不要重新合并成一个大输入状态机。

## Self-Maintenance Rule

**After every major change** (new model, new page, new controller, route changes, migration changes, new test files, architectural shifts), update this CLAUDE.md file to reflect the current state. Specifically:

- Add new models/controllers/pages/routes to the relevant tables below
- Update test count if new tests are added
- Add any new gotchas or patterns to the "Gotchas & Pitfalls" section
- Keep this file as the single source of truth for AI sessions working on this project

## 日志级别必须使用至少info，因为目前未开放debug级别的日志，debug级别日志看不到。

## 函数参数和配置结构体不允许使用、设置默认值，因为大量的默认值会导致数据流变得难以理解，难以追踪某个值是如何被设置的、是某层默认的还是外部传入的，增加理解和调试难度。若不得不增加默认值，必须征求我的同意。

## 【重要】README.md 使用指南

几乎每个目录下都有一个简体中文编写的 README.md，它很重要，在你探索项目实现的时候，提前看看它，能节省不少多余的文件查找与查看，节省不少上下文开销。

【重要】为了节省上下文，请你尽量阅读各级目录的 README.md 来获取你需要的信息，而不是直接查看代码文件。

例如：当你修改想修改或查看`src\common\world\chunk\ChunkLoadTicketManager.cpp`，你必须提前依次查看目录链上的各级目录上的readme文件：

- 项目根目录的 README.md
- src\common\README.md
- src\common\world\README.md
- src\common\world\chunk\README.md

同理，当你对代码职责、接口、架构等做了修改，必须视情况决定是否递归修改/新增目录链上的各级README.md！

如果你发现当前的README.md内容过时了，或者不够清晰了，或者缺乏重要信息了，你必须更正/更新它！如果你发现自己途径的目录没有README.md了，你必须新增一个！

注意，tests目录不考虑，不要写 README.md。

### 文档内容

每个 README.md 都必须使用简体中文，必须至少包含：
1. **目录结构树** - 清晰展示文件组织
2. **文件介绍** - 每个文件的职责和主要内容
3. **模块关系** - 分析各组件之间的依赖关系
4. **整体职责** - 模块作为整体的职责说明
5. **输入/输出** - 数据流向分析
6. **依赖项** - 外部和内部依赖
7. **使用方法** - 代码示例和集成指南
8. **容易踩的坑** - 常见问题和解决方案
9. **测试用例** - 相关测试文件说明
10. **Mermaid图表** - 架构图、流程图、类图等（使用简体中文和彩色样式）

## 其他

### 对于重构类任务的准则

1.不做任何api兼容，直接一步到位新代码（因此主调者需要修改，这符合预期）
2.我有强烈代码洁癖，不允许留任何旧代码旧文件。不允许通过注释等任何手段保留旧代码，不允许以兼容为理由保留旧代码，不允许未经允许的情况下乱加adapter或兼容层来保留旧代码。重构类任务的目标就是把旧代码完全替换掉，留下干净的新实现。

### 对于所有任务的准则

当你完成一个任务后，不要停下来，请继续做后面的任务，直到任务清空你才能停！你时间充足、上下文也充足
