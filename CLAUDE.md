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

### Lighting Types
- `LightType` - `SKY` / `BLOCK` light selector
- `IChunkLightProvider` - Internal light provider interface used by the lighting manager
- `WorldLightManager` - Public lighting facade for all server-side light access
- `SWMRNibbleArray` - Single-writer multi-reader nibble storage used by the light buffers

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
- Lighting propagation is flood-fill based and can leak around isolated obstacles in open space.
    - Do not assume a single roof block or wall block fully darkens the column below it.
    - If you need to test total occlusion, build a sealed enclosure or a thicker barrier.
- `BlockState::lightLevel()` is a cached static value.
    - Runtime emission and opacity checks for lighting must go through `Block::getLightLevel(...)` and `Block::getOpacity(...)` when the block can vary by state or context.
- 光照热路径不要逐方块回到 `ServerWorld::getBlockState(...)`。
    - `WorldLightManager` 在 tick 内必须优先缓存 `IChunk` 指针，再从区块内直接读取方块状态；否则 `getChunkShared` / `getBlockState` 会成为主要瓶颈。

## Self-Maintenance Rule

**After every major change** (new model, new page, new controller, route changes, migration changes, new test files, architectural shifts), update this CLAUDE.md file to reflect the current state. Specifically:

- Add new models/controllers/pages/routes to the relevant tables below
- Update test count if new tests are added
- Add any new gotchas or patterns to the "Gotchas & Pitfalls" section
- Keep this file as the single source of truth for AI sessions working on this project

## 日志级别必须使用至少info，因为目前未开放debug级别的日志，debug级别日志看不到。

## 函数参数和配置结构体不允许使用、设置默认值，因为大量的默认值会导致数据流变得难以理解，难以追踪某个值是如何被设置的、是某层默认的还是外部传入的，增加理解和调试难度。若不得不增加默认值，必须征求我的同意。
