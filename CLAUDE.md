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

## Current Status

- Ocean biome generation has been aligned toward MC 1.16.5 behavior for shallow/deep warm, lukewarm, cold, and frozen variants.
- `BiomeGenerationSettings` now provides deep-ocean-specific factory methods:
    - `createDeepWarmOcean`
    - `createDeepLukewarmOcean`
    - `createDeepColdOcean`
    - `createDeepFrozenOcean`
- Blue ice generation now uses a vanilla-style spread model:
    - sea-level gating
    - packed-ice neighbor precondition
    - iterative propagation (`spreadAttempts`)
- End biome provider now follows vanilla island-height selection logic (`func_235317_a_` equivalent), including RNG skip behavior and noise-grid container filling.
- Ocean feature tests and biome-setting tests were updated to match the new parity-oriented expectations.
- `ServerWorld::setBlock` now executes a full block-update callback chain:
    - canonicalizes incoming `BlockState*` by `stateId`
    - invokes `onBlockRemoved/onBlockAdded`
    - applies neighbor `updatePostPlacement`
    - dispatches `neighborChanged` notifications
- Gravity-sensitive blocks are now wired through `FallingBlock`:
    - `minecraft:sand`
    - `minecraft:gravel`
    - `minecraft:red_sand`
- Agricultural survival/growth rules were tightened:
    - crops and stems now require farmland support
    - farmland moisture/water checks and dirt fallback are active
    - sugar cane now checks vanilla-like ground and water adjacency
- Fluid flow path now integrates `ILiquidContainer` checks in `FlowingFluid`:
    - `isBlocked` consults `ILiquidContainer::canContainFluid`
    - `flowInto` attempts `ILiquidContainer::receiveFluid` before block replacement
- Crafting-table interaction is now wired into right-click packet handling:
    - Integrated server opens the existing `CraftingMenu` when right-clicking crafting table with empty/non-block hand
    - Standalone server routes crafting-table open events through `ContainerManager` callbacks and sends open/close/content container packets
    - `ContainerManager` is now connected to `InventoryManager`, creates real `CraftingMenu` instances, and processes container click updates through menu state
- First-person hand rotation path has been rewritten against MC 1.16.5 `FirstPersonRenderer` formulas:
    - arm/item transforms now follow vanilla coefficients and order (`renderArmFirstPerson`, `transformSideFirstPerson`, `transformFirstPerson`)
    - first-person root orientation now uses camera-aligned basis derived from MC forward vector convention
    - custom `MatrixStack` transform composition is now strict post-multiply (`current = current * transform`) to match PoseStack semantics
- First-person hand item meshes are now cached per hand and retired with a frame-based countdown tied to `maxFramesInFlight`, which prevents main/offhand cache thrashing and repeated `vkAllocateMemory` churn.
- `EntityPipeline::destroy()` and `FirstPersonRenderer::destroy()` now clean up partially initialized Vulkan resources even when initialization fails early.
- `VulkanUtils::createBuffer()` and `VulkanUtils::createImage()` now unwind correctly when `vkBind*Memory()` fails.
- Client-side chunk mesh build path has been split into two layers:
    - `MeshBuildScheduler` owns priority, frustum weighting, reprioritization, and cancellation.
    - `MeshWorkerPool` is now a pure FIFO executor with cooperative cancel checks.
- `ClientWorld` now initializes mesh build via `initializeMeshSystem(threadCount, MeshSchedulerConfig)` and drives scheduling with `update(const MeshSchedulerViewState&)`.
- `ClientWorld::update(...)` now reconciles stale `activeMeshTaskId` values via `MeshBuildScheduler::isTaskTracked(...)` and resubmits in-frustum chunks that still have no mesh result.
- `ChunkMesher::generateMesh`, `generateSplitMesh`, and `generateSectionMesh` now accept a `cancelSignal` pointer and check cancellation in long-running loops.
- New client tests cover the refactored path:
    - `tests/client/test_mesh_worker_pool.cpp`
    - `tests/client/test_mesh_build_scheduler.cpp`

## Gotchas & Pitfalls

- `WorldGenRegion` is not an `IBlockReader`; do not pass it directly to `Block::isValidPosition`.
    - In worldgen features, use explicit local placement checks (`isWater`, support block checks) when running in `WorldGenRegion` context.
- Blue ice placement will always fail if there is no packed-ice neighbor around the sampled start position.
    - Tests must set up packed ice at the exact sampled neighborhood, not by replacing whole water layers in a way that shifts ocean-floor detection.
- Avoid passing temporary `BlockState` copies to world write APIs.
    - Prefer canonical references returned by `state.with(...)` / `defaultState()`; `ServerWorld::setBlock` now canonicalizes by `stateId` as a safety net.
- `MatrixStack` call-order intuition can be misleading after PoseStack alignment.
    - In first-person rendering, apply transforms in vanilla order and rely on post-multiply semantics; avoid ad-hoc in-place row/column edits.
- Do not share one first-person item mesh cache across both hands.
    - Main hand and off hand can hold different items in the same frame, so a single cache will thrash and allocate GPU memory every render.
- Retired first-person meshes must be reclaimed on a frame countdown, not only in `destroy()`.
    - Otherwise repeated item changes will keep old Vulkan buffers alive for the whole session.
- Do not put priority logic back into `MeshWorkerPool`.
    - Priority and cancellation policy belong to `MeshBuildScheduler`; `MeshWorkerPool` should stay execution-only.
- Always update `MeshSchedulerViewState` before calling `ClientWorld::update(...)` each frame.
    - If the view state is stale, frustum priority and behind-camera cancellation will lag behind camera movement.
- Pending mesh tasks cancelled before dispatch do not produce worker results.
    - Keep `activeMeshTaskId` in sync with scheduler tracking (or chunks can get stuck with a stale task id and never be resubmitted).
- When changing `ChunkMesher` mesh APIs, update both runtime and tests together.
    - `tests/client/renderer/test_renderer.cpp` now calls the 5-argument `generateSplitMesh(..., neighbors, cancelSignal)` signature.

## Self-Maintenance Rule

**After every major change** (new model, new page, new controller, route changes, migration changes, new test files, architectural shifts), update this CLAUDE.md file to reflect the current state. Specifically:

- Add new models/controllers/pages/routes to the relevant tables below
- Update test count if new tests are added
- Add any new gotchas or patterns to the "Gotchas & Pitfalls" section
- Update the "Current Status" section if the status changes
- Keep this file as the single source of truth for AI sessions working on this project

## 日志级别必须使用至少info，因为目前未开放debug级别的日志，debug级别日志看不到。

## 函数参数和配置结构体不允许使用、设置默认值，因为大量的默认值会导致数据流变得难以理解，难以追踪某个值是如何被设置的、是某层默认的还是外部传入的，增加理解和调试难度。若不得不增加默认值，必须征求我的同意。

## 【重要】README.md 使用指南

几乎每个目录下都有一个简体中文编写的 README.md，它很重要，在你探索项目实现的时候，提前看看它，能节省不少多余的文件查找与查看，节省不少上下文开销。

【重要】为了节省上下文，请你尽量阅读各级目录的 README.md 来获取你需要的信息，而不是直接查看代码文件。

例如：当你修改想修改或查看`src\common\world\chunk\ChunkLoadTicketManager.cpp`，你必须提前依次查看目录链上的各级目录上的readme文件：

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
