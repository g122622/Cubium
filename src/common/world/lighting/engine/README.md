# 光照引擎模块 (Lighting Engine)

本目录包含 Minecraft 光照系统的核心引擎实现，负责计算和传播方块光照和天空光照。
当前实现基于 Starlight 优化算法，核心类为 `StarLightEngine`、`BlockStarLightEngine`、`SkyStarLightEngine`。

## 目录结构

```
engine/
├── BaseLightEngine.hpp/cpp     # StarLightEngine 基类（Starlight 优化版）
├── BlockLightEngine.hpp/cpp    # BlockStarLightEngine 方块光照引擎
├── SkyLightEngine.hpp/cpp      # SkyStarLightEngine 天空光照引擎
├── LightEngineCache.hpp/cpp    # 光照引擎缓存系统（备用）
├── LightEngineUtils.hpp/cpp    # 光照引擎工具类
└── README.md                   # 本文档
```

## 核心设计

### 方向枚举 (LightAxisDirection)

```cpp
enum class LightAxisDirection : u8 {
    POSITIVE_X = 0,  // 东
    NEGATIVE_X = 1,  // 西
    POSITIVE_Z = 2,  // 南
    NEGATIVE_Z = 3,  // 北
    POSITIVE_Y = 4,  // 上
    NEGATIVE_Y = 5   // 下
};
```

**设计要点**:
- 偶数为正方向，奇数为负方向
- 通过 XOR 1 可快速获取相反方向
- 与 `Direction.hpp` 中的 `AxisDirection` 不同，后者只有 Positive/Negative 两个值

### 队列条目编码 (64位紧凑格式)

```
位 0-5:   X 坐标（相对于编码偏移）
位 6-11:  Z 坐标（相对于编码偏移）
位 12-27: Y 坐标（相对于编码偏移）
位 28-31: 传播级别 (0-15)
位 32-37: 方向位集
位 38-40: 状态标志
位 41-62: 未使用
位 63:    FLAG_WRITE_LEVEL
```

**标志位定义**:
- `FLAG_WRITE_LEVEL`: 需要写入光照等级
- `FLAG_RECHECK_LEVEL`: 需要重新检查光照等级
- `FLAG_HAS_SIDED_TRANSPARENT_BLOCKS`: 当前方块有条件透明面

### SWMRNibbleArray 状态管理

`SWMRNibbleArray` 采用 Starlight 的四状态设计：

| 状态 | 说明 | 存储状态 |
|------|------|----------|
| Null | 不存在 | 无存储，读取返回 0 |
| Uninit | 未初始化（全零） | 无存储，读取返回 0 |
| Init | 已初始化 | 有实际数据 |
| Hidden | 隐藏状态 | 有数据，但转换为 Vanilla 时视为 Null |

**关键方法**:
- `getUpdating()`: 更新侧读取
- `getVisible()`: 可见侧读取（线程安全）
- `set()`: 更新侧写入
- `updateVisible()`: 同步更新侧到可见侧
- `setNull()/setUninitialized()/setNonNull()/setHidden()`: 状态转换

**写时复制 (Copy-on-Write)**:
- `storageUpdating` 和 `storageVisible` 初始时指向同一数组
- 写入时（`set()` 调用）会触发写时复制，创建独立副本
- `updateVisible()` 同步时，两指针重新指向同一数组

### BlockStarLightEngine（BlockLightEngine.hpp/cpp）

**职责**: 实现方块光照的传播算法。方块光源（如火把、萤石）发出的光向相邻方块传播，每传播一个方块衰减 1 级。

**主要内容**:

```mermaid
classDiagram
    StarLightEngine <|-- BlockStarLightEngine
    class BlockStarLightEngine {
        -m_storage: BlockLightStorage
        -m_emptinessMaps: unordered_map~i64, EmptinessMap~

        +checkBlock(lightAccess, worldX, worldY, worldZ)
        +onBlockEmissionIncrease(lightAccess, worldX, worldY, worldZ, lightLevel)
        +getLightFor(worldX, worldY, worldZ) u8
        +updateSectionStatus(pos: SectionPos, isEmpty)
        +setData(pos, array, retain)
        +hasWork() bool
        +tick(maxUpdates, updateSkyLight, updateBlockLight) i32

        #isRoot(pos) bool [override]
        #computeLevel(pos, excludedSource, level) i32 [override]
        #notifyNeighbors(pos, level, isDecreasing) [override]
        #getLevel(pos) i32 [override]
        #setLevel(pos, level) [override]
        #getEdgeLevel(fromPos, toPos, startLevel) i32 [override]
        #isSectionEmpty(sectionPos) bool [override]
    }
```

**光照传播规则**:
- 光源发出 15 级光
- 每穿过一个方块衰减 1 级（最小 1 级）
- 部分透明方块（如水）有特定透明度
- 完全不透明方块（透明度 15）阻挡光线
- `checkBlock()` 入口对齐 Starlight：先写当前位置源级别，再同时入增亮/减亮队列
- `getEdgeLevel()` 对来源面遮挡采用“条件形状优先”策略，不会错误阻断完整方块光源（如萤石）向外传播

### SkyStarLightEngine（SkyLightEngine.hpp/cpp）

**职责**: 实现天空光照的传播算法。天空光照从天空向下传播，在透明方块中传播时不衰减，只有在不透明方块阻挡时才会衰减。

**主要内容**:

```mermaid
classDiagram
    StarLightEngine <|-- SkyStarLightEngine
    class SkyStarLightEngine {
        -m_storage: SkyLightStorage

        +checkBlock(lightAccess, worldX, worldY, worldZ)
        +getLightFor(worldX, worldY, worldZ) u8
        +updateSectionStatus(pos: SectionPos, isEmpty)
        +setData(pos, array, retain)
        +setColumnEnabled(columnPos, enabled)
        +hasWork() bool
        +tick(maxUpdates, updateSkyLight, updateBlockLight) i32

        #isRoot(pos) bool [override]
        #computeLevel(pos, excludedSource, level) i32 [override]
        #notifyNeighbors(pos, level, isDecreasing) [override]
        #getLevel(pos) i32 [override]
        #setLevel(pos, level) [override]
        #getEdgeLevel(fromPos, toPos, startLevel) i32 [override]
    }
```

**天空光照特点**:
- 天空光照没有根节点（`isRoot()` 始终返回 `false`）
- 从天空向下传播时，透明方块不衰减
- 遇到不透明方块时开始衰减
- 支持跨区块段向下传播（空区块段优化）
- `checkBlock()` 会基于 `ROOT_POS` 贡献判断当前点是否可作为天空源重入增亮队列

### LightEngineCache.hpp/cpp

**职责**: 提供光照引擎的缓存系统，避免重复的区块查找操作。

**主要内容**:

```mermaid
classDiagram
    class LightEngineCache {
        +CACHE_RADIUS: i32 = 2
        +CHUNK_CACHE_SIZE: i32 = 25

        -m_provider: StarLightLightingProvider*
        -m_minSection: i32
        -m_maxSection: i32
        -m_chunkCache: array~const IChunk*, 25~
        -m_sectionCache: unique_ptr~const void*[]~
        -m_nibbleCache: unique_ptr~SWMRNibbleArray*[]~
        -m_emptinessMapCache: array~const bool*, 25~

        +setupCaches(centerX, centerY, centerZ, relaxed, loadTwoRadius)
        +destroyCaches()
        +getChunk(chunkX, chunkZ) const IChunk*
        +getSection(sectionX, sectionY, sectionZ) const void*
        +getNibble(sectionX, sectionY, sectionZ) SWMRNibbleArray*
        +isSectionEmpty(sectionX, sectionY, sectionZ) bool
        +getBlockState(worldX, worldY, worldZ) const BlockState*
        +getLightLevel(worldX, worldY, worldZ) u8
        +getCacheHitRate() f32
    }
```

**缓存范围**:
- 以中心区块为原点的 5x5 区块区域
- 高度范围：从 `minSection - 1` 到 `maxSection + 1`（包含缓冲区）

### LightEngineUtils.hpp/cpp

**职责**: 提供光照引擎的共享工具方法。

**主要内容**:

```mermaid
classDiagram
    class LightEngineUtils {
        <<utility>>
        +ROOT_POS: i64 = LONG_MAX
        +ALL_DIRECTIONS: Direction[6]
        +HORIZONTAL_DIRECTIONS: Direction[4]

        +packPos(x, y, z) i64
        +unpackPos(packed, x, y, z)
        +offsetPos(pos, dir) i64
        +worldToSectionPos(worldPos) i64
        +extractNibbleIndices(packed, x, localY, z)
        +getBlockAndOpacity(chunk, worldPos, opacityOut) BlockState*
        +getVoxelShape(state) CollisionShape&
        +facesHaveOcclusion(world, stateA, posA, stateB, posB, dir, opacityA) bool
        +blocksLightInDirection(state, dir) bool
    }

    class DirectionBit {
        <<enumeration>>
        DIR_NONE
        DIR_DOWN
        DIR_UP
        DIR_NORTH
        DIR_SOUTH
        DIR_WEST
        DIR_EAST
        DIR_ALL
        DIR_HORIZONTAL
        DIR_VERTICAL
    }

    class DirectionBits {
        <<namespace>>
        +fromDirection(dir) DirectionBit
        +opposite(bits) DirectionBit
        +allExcept(bits) DirectionBit
        +hasDirection(bits, dir) bool
        +count(bits) u32
        +toDirections(bits) DirectionArray
    }
```

**位置编码格式** (64位):

| 位范围 | 内容 | 说明 |
|--------|------|------|
| 0-11 | Y 坐标 | 12位有符号 (-2048 ~ 2047) |
| 12-37 | Z 坐标 | 26位有符号 |
| 38-63 | X 坐标 | 26位有符号 |

## 模块架构

```mermaid
flowchart TB
    subgraph Storage["存储层 (storage/)"]
        BlockLightStorage
        SkyLightStorage
        SWMRNibbleArray
        EmptinessMap
    end

    subgraph Engine["引擎层 (engine/)"]
        StarLightEngine
        BlockStarLightEngine
        SkyStarLightEngine
        LightEngineCache
        LightEngineUtils
    end

    subgraph World["世界层"]
        StarLightLightingProvider
        IChunk
        BlockState
    end

    StarLightEngine --> LightEngineCache
    StarLightEngine --> LightEngineUtils

    BlockStarLightEngine --> StarLightEngine
    BlockStarLightEngine --> BlockLightStorage
    BlockStarLightEngine --> EmptinessMap

    SkyStarLightEngine --> StarLightEngine
    SkyStarLightEngine --> SkyLightStorage

    LightEngineCache --> StarLightLightingProvider
    LightEngineCache --> IChunk

    LightEngineUtils --> BlockState
    LightEngineUtils --> IChunk
```

## 模块职责

### 整体职责

光照引擎模块负责计算和更新 Minecraft 世界中的光照数据，包括：

1. **方块光照 (Block Light)**: 由发光方块（火把、萤石等）产生的光照
2. **天空光照 (Sky Light)**: 来自天空的光照，受高度和遮挡物影响

### 输入

| 输入项 | 类型 | 说明 |
|--------|------|------|
| 区块数据 | `IChunk*` | 包含方块状态和高度图 |
| 光源位置 | `BlockPos` | 发光方块的位置和亮度 |
| 区块段状态 | `SectionPos, bool` | 区块段是否全为空气 |
| 光照数据 | `SWMRNibbleArray` | 从存档加载的光照数据 |

### 输出

| 输出项 | 类型 | 说明 |
|--------|------|------|
| 更新的光照数据 | `SWMRNibbleArray*` | 修改后的光照数组 |
| 光照变更通知 | 回调 | 通知客户端光照变化 |

### 依赖项

```
common/world/lighting/storage/  - 光照存储层
common/world/chunk/             - 区块数据结构
common/world/block/             - 方块状态和属性
common/physics/collision/       - 碰撞形状用于遮挡检测
common/util/Direction.hpp       - 方向枚举
```

## 使用方法

### 基本用法

```cpp
#include "common/world/lighting/engine/BlockLightEngine.hpp"
#include "common/world/lighting/engine/SkyLightEngine.hpp"

// 创建光照引擎
BlockStarLightEngine blockLightEngine(chunkProvider);
SkyStarLightEngine skyLightEngine(chunkProvider);

// 设置区块段状态
blockLightEngine.updateSectionStatus(SectionPos(0, 0, 0), false);  // 非空
skyLightEngine.updateSectionStatus(SectionPos(0, 0, 0), false);

// 当方块发光等级增加时
blockLightEngine.onBlockEmissionIncrease(chunkProvider, 100, 64, 200, 15);  // 火把

// 处理光照更新（每 tick 调用）
int remaining = blockLightEngine.tick(512, false, true);
remaining = skyLightEngine.tick(512, true, false);

// 获取光照等级
u8 blockLight = blockLightEngine.getLightFor(100, 64, 200);
u8 skyLight = skyLightEngine.getLightFor(100, 64, 200);
```

### 批量光照计算

```cpp
// 启用缓存以提高批量计算性能
blockLightEngine.enableCache(centerX, centerY, centerZ);

// 执行光照计算...

// 完成后禁用缓存
blockLightEngine.disableCache();
```

### 检查方块变化

```cpp
void onBlockChanged(World& world, const BlockPos& pos) {
    // 检查光照更新
    blockLightEngine.checkBlock(chunkProvider, pos.x, pos.y, pos.z);
    skyLightEngine.checkBlock(chunkProvider, pos.x, pos.y, pos.z);
}
```

## 容易踩的坑

### 1. 光照传播顺序

**问题**: 减亮队列必须在增亮队列之前处理。

**原因**: 减亮操作需要先清除旧光照，增亮操作才能正确计算新光照。

另外，队列内部应保持 FIFO 语义（按入队顺序），不要改回尾部弹出式 LIFO。
LIFO 会导致传播波前顺序紊乱，在复杂遮挡场景下出现重复震荡与额外回补。

对于 `currentLevel < targetLevel` 且 `target = 最暗` 的减亮分支，
不能只做“强制清暗 + 继续减亮”，还要补充相邻节点的增亮重检入队。
否则在 FIFO 波前下，像“悬空单石头下方”这类场景会丢失合法侧向天光。

```cpp
// 正确的 tick 处理顺序
i32 StarLightEngine::processUpdates(i32 maxUpdates) {
    // 先处理减亮队列
    if (m_decreaseQueueInitialLength > 0) {
        maxUpdates = processDecreaseQueue(maxUpdates);
    }
    // 再处理增亮队列
    if (m_increaseQueueInitialLength > 0) {
        maxUpdates = processIncreaseQueue(maxUpdates);
    }
}
```

### 2. 空区块段优化

**问题**: 忘记更新空区块段状态会导致光照计算错误。

**解决方案**: 当区块段内容变化时，必须调用 `updateSectionStatus()`。

```cpp
void onChunkSectionChanged(ChunkSection& section, SectionPos pos) {
    bool isEmpty = section.isEmpty();
    blockLightEngine.updateSectionStatus(pos, isEmpty);
    skyLightEngine.updateSectionStatus(pos, isEmpty);
}
```

### 3. 天空光照的特殊处理

**问题**: 天空光照引擎内部等级语义与可见光值相反。

**原因**:
- 内部传播级别使用 `0=最亮, 15=最暗`，而可见光值是 `15=最亮, 0=最暗`。
- 天空光照没有单一的固定根节点，根贡献通过 `getEdgeLevel(ROOT_POS, ...)` 计算。

**注意**: 在 `checkBlock()` 与 `getEdgeLevel()` 中必须先做语义转换再比较大小，否则会出现“封顶不降光”或“侧向传播被反向阻断”。

### 4. 坐标范围限制

**问题**: Y 坐标限制在 12 位有符号范围内 (-2048 ~ 2047)。

**原因**: 使用紧凑的 64 位位置编码格式。

**解决方案**: 对于超出范围的坐标，需要额外的处理或检查。

### 5. 缓存生命周期

**问题**: 缓存启用后未正确清理会导致数据不一致。

**解决方案**: 确保每次批量计算后调用 `disableCache()` 或在下次使用前调用 `setupCaches()`。

### 6. 方向位集的正确使用

**问题**: 错误理解方向位集的语义。

**说明**:
- 当前实现中，队列里的 `directions` 表示“允许继续传播的方向集合”，不是“来源方向”。
- 从 `fromPos -> toPos` 继续传播时，通常需要排除回传方向（即来源的反方向）。

```cpp
// 示例：从当前点向东传播到邻居后，后续通常不再向西回传
DirectionBit blocked = DIR_WEST;
DirectionBit checkDirs = DirectionBits::allExcept(blocked);
```

## 性能优化

### Starlight 优化

本实现采用了 Starlight mod 的核心优化：

1. **紧凑队列条目**: 使用 `QueueEntry{pos, level, directions, flags}`，并保留完整世界坐标，避免远坐标回绕
2. **方向位集**: 使用位运算快速计算相反方向，避免遍历所有方向
3. **空区块段跳过**: 全空气区块段直接跳过光照计算
4. **缓存系统**: 扁平数组缓存区块和区块段，避免重复查找

### 缓存命中率

通过 `getCacheHitRate()` 监控缓存性能。正常情况下命中率应 > 90%。

```cpp
f32 hitRate = blockLightEngine.getCacheHitRate();
if (hitRate < 0.9f) {
    // 可能需要检查缓存设置
}
```

## 涉及的测试用例

| 测试文件 | 测试内容 |
|----------|----------|
| `tests/lighting/LightingTest.cpp` | 位置编码/解码、方向工具、光照类型 |
| `tests/lighting/LightUpdateTest.cpp` | 光照数据序列化、数据包测试 |

### 测试覆盖范围

- **NibbleArray**: 存储、索引、边界值
- **SectionPos**: 坐标转换、编码/解码
- **LightEngineUtils**:
  - `packPos` / `unpackPos`: 正负坐标
  - `offsetPos`: 各方向偏移
  - `worldToSectionPos`: 世界坐标到区块段坐标
  - `extractNibbleIndices`: 提取局部坐标
- **DirectionBit**: 方向转换、位操作
- **DirectionBits**: 相反方向、方向计数

## 参考资料

- **Minecraft 1.16.5 源码**: `net.minecraft.world.lighting.BlockLightEngine`, `net.minecraft.world.lighting.SkyLightEngine`
- **Starlight Mod**: `ca.spottedleaf.moonrise.patches.starlight.light.StarLightEngine` - 队列编码和方向位集优化
