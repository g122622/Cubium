# 光照系统 (Lighting System)

本目录实现了 Minecraft 1.16.5 风格的光照系统，参考 Starlight 优化实现。

## 目录结构

```
lighting/
├── LightType.hpp              # 光照类型枚举和常量
├── IChunkLightProvider.hpp    # 区块光照提供者接口
├── InternalLight.hpp/cpp      # 内部光照计算工具
├── engine/                    # 光照引擎
│   ├── LevelBasedGraph.hpp/cpp        # 基于级别的传播图（Starlight优化版）
│   ├── LightEngineCache.hpp/cpp       # 光照引擎缓存系统
│   ├── LightEngineUtils.hpp/cpp       # 光照引擎工具类
│   ├── BlockLightEngine.hpp/cpp       # 方块光照引擎
│   └── SkyLightEngine.hpp/cpp         # 天空光照引擎
├── manager/                   # 光照管理器
│   └── WorldLightManager.hpp/cpp      # 世界光照管理器
└── storage/                   # 光照存储
    ├── SWMRNibbleArray.hpp/cpp        # 单写多读Nibble数组
    ├── EmptinessMap.hpp/cpp           # 空区块段映射
    ├── SectionLightStorage.hpp        # 区块段光照存储基类
    ├── BlockLightStorage.hpp/cpp      # 方块光照存储
    ├── SkyLightStorage.hpp/cpp        # 天空光照存储
    └── SWMRLightDataMap.hpp           # SWMR光照数据映射
```

## 模块职责

### 根目录文件

#### LightType.hpp

定义光照类型枚举和相关常量：

```cpp
enum class LightType : u8 {
    SKY = 0,    // 天空光照（露天位置默认15级，向下传播不衰减）
    BLOCK = 1   // 方块光照（火把14级，萤石15级，所有方向衰减1级）
};

namespace LightConstants {
    constexpr u8 MAX_LIGHT = 15;           // 最大光照等级
    constexpr u8 MIN_LIGHT = 0;            // 最小光照等级
    constexpr u8 SKY_LIGHT_DEFAULT = 15;   // 天空光照默认值
    constexpr u8 BLOCK_LIGHT_DEFAULT = 0;  // 方块光照默认值
    constexpr u8 PROPAGATION_DECAY = 1;    // 光照传播衰减值
}
```

#### IChunkLightProvider.hpp

区块光照提供者接口，ServerWorld 和 ClientWorld 实现此接口：

```cpp
class IChunkLightProvider {
public:
    // 区块访问
    virtual IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) = 0;
    virtual const BlockState* getBlockStateForLight(const BlockPos& pos) const = 0;

    // 世界信息
    virtual IWorld* getWorld() = 0;
    virtual bool hasSkyLight() const = 0;
    virtual i32 getMinBuildHeight() const = 0;
    virtual i32 getMaxBuildHeight() const = 0;

    // 光照通知
    virtual void markLightChanged(LightType type, const SectionPos& pos) = 0;
};
```

#### InternalLight.hpp/cpp

内部光照计算工具，用于游戏机制（生物生成、农作物生长等）：

| 函数 | 说明 |
|------|------|
| `calculateSkyDarkening(dayTime, isRaining, isThundering)` | 计算天空减暗因子 (0-11) |
| `calculateInternalLight(blockLight, skyLight)` | 计算内部光照等级 |
| `calculateRawBrightness(blockLight, skyLight, skyDarkening)` | 计算原始亮度 |
| `isDarkEnoughForSpawning(rawBrightness)` | 检查是否足够黑暗以生成敌对生物 |
| `getCelestialAngle(dayTime)` | 获取天体角度 (0.0-1.0) |
| `getMoonPhase(dayTime)` | 获取月相索引 (0-7) |

### engine/ 目录 - 光照引擎

#### LevelBasedGraph.hpp/cpp

基于级别的传播图，实现 Starlight 优化的 BFS 光照传播算法。

**核心特性**：
- 显式队列条目（完整世界坐标 + 方向位集 + 标志位）
- 方向位集优化，跳过反向传播
- 双队列设计：增亮队列和减亮队列
- FIFO 波前处理顺序（队列按写入顺序处理，避免 LIFO 造成的传播震荡）

**队列元素结构**：
```
QueueEntry {
    pos:        世界坐标编码（LightEngineUtils::packPos，完整 X/Y/Z）
    level:      光照级别（0-15）
    directions: 传播方向位集（DIR_*）
    flags:      WRITE_LEVEL / RECHECK_LEVEL / HAS_SIDED_TRANSPARENT
}
```

说明：当前实现不再使用低位截断坐标压缩，避免大坐标或跨区块传播时的坐标回绕问题。

**核心虚方法**（子类实现）：
```cpp
virtual bool isRoot(i64 pos) const = 0;                          // 检查是否为根节点
virtual i32 computeLevel(i64 pos, i64 excludedSource, i32 level) = 0;  // 计算新级别
virtual void notifyNeighbors(i64 pos, i32 level, bool isDecreasing, u8 directionBits) = 0;  // 通知相邻位置
virtual i32 getLevel(i64 pos) const = 0;                          // 获取当前级别
virtual void setLevel(i64 pos, i32 level) = 0;                    // 设置级别
virtual i32 getEdgeLevel(i64 fromPos, i64 toPos, i32 startLevel) = 0;  // 计算边缘级别
```

#### LightEngineCache.hpp/cpp

光照引擎缓存系统，避免重复的区块查找。

**缓存范围**：以中心区块为中心的 5x5 区块区域

**缓存内容**：
- 区块指针缓存 (25个)
- 区块段数据缓存
- NibbleArray 缓存
- 空区块段映射缓存

**关键方法**：
```cpp
void setupCaches(i32 centerX, i32 centerY, i32 centerZ, bool relaxed, bool loadTwoRadius);
void destroyCaches();
const IChunk* getChunk(i32 chunkX, i32 chunkZ) const;
bool isSectionEmpty(i32 sectionX, i32 sectionY, i32 sectionZ) const;
```

#### LightEngineUtils.hpp/cpp

光照引擎工具类，提供共享工具方法：

**方向位集常量**：
```cpp
enum DirectionBit : u8 {
    DIR_NONE   = 0,
    DIR_DOWN   = 1 << 0,  // Y-
    DIR_UP     = 1 << 1,  // Y+
    DIR_NORTH  = 1 << 2,  // Z-
    DIR_SOUTH  = 1 << 3,  // Z+
    DIR_WEST   = 1 << 4,  // X-
    DIR_EAST   = 1 << 5,  // X+
    DIR_ALL    = 0x3F,    // 所有6个方向
};
```

**关键方法**：
```cpp
static constexpr i64 ROOT_POS = LONG_MAX;  // 根节点位置标记

static constexpr i64 packPos(i32 x, i32 y, i32 z);  // 世界位置编码
static constexpr void unpackPos(i64 packed, i32& x, i32& y, i32& z);  // 世界位置解码
static i64 worldToSectionPos(i64 worldPos);  // 世界位置转区块段位置

static const BlockState* getBlockAndOpacity(const IChunk* chunk, i64 worldPos, i32* opacityOut);
static bool facesHaveOcclusion(IWorld* world, const BlockState& stateA, ...);
static bool blocksLightInDirection(const BlockState& state, Direction dir);
```

#### BlockLightEngine.hpp/cpp

方块光照引擎，处理火把、萤石等方块光源。

**继承自**：`LevelBasedGraph`

**特殊逻辑**：
- 光源方块发出初始光照等级
- 向所有6个方向传播时都衰减1级
- 检测方块的透明度来决定传播衰减

**核心方法**：
```cpp
void checkLight(const BlockPos& pos);              // 检查位置光照
void onBlockEmissionIncrease(const BlockPos& pos, i32 lightLevel);  // 光源放置
u8 getLightFor(const BlockPos& pos) const;         // 获取光照值
void updateSectionStatus(const SectionPos& pos, bool isEmpty);  // 更新区块段状态
```

#### SkyLightEngine.hpp/cpp

天空光照引擎，处理来自天空的自然光。

**继承自**：`LevelBasedGraph`

**特殊逻辑**：
- 向下传播不衰减
- 向其他方向传播衰减1级
- 追踪表面位置
- 区块列启用/禁用管理

**核心方法**：
```cpp
void checkLight(const BlockPos& pos);              // 检查位置光照
u8 getLightFor(const BlockPos& pos) const;         // 获取光照值
void setColumnEnabled(i64 columnPos, bool enabled);  // 启用/禁用区块列
bool isAtSurfaceTop(i64 worldPos) const;           // 检查是否在表面顶部
```

### manager/ 目录 - 光照管理器

#### WorldLightManager.hpp/cpp

世界光照管理器，协调方块光照和天空光照引擎。

**职责**：
- 统一的光照更新接口
- 根据维度配置选择光照引擎（下界无天空光）
- 处理区块加载/卸载时的光照同步

**核心方法**：
```cpp
void checkBlock(const BlockPos& pos);              // 检查方块光照
void onBlockEmissionIncrease(const BlockPos& pos, i32 lightLevel);  // 光源放置
bool hasLightWork() const;                         // 检查是否有待处理工作
i32 tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight);  // 处理更新

void updateSectionStatus(const SectionPos& pos, bool isEmpty);  // 更新区块段状态
void enableLightSources(const ChunkPos& pos, bool enable);  // 启用/禁用光源

i32 getLightSubtracted(const BlockPos& pos, i32 skyDarkening) const;  // 获取实际亮度
u8 getBlockLight(const BlockPos& pos) const;
u8 getSkyLight(const BlockPos& pos) const;

void setData(LightType type, const SectionPos& pos, const NibbleArray& array, bool retain);
SWMRNibbleArray* getData(LightType type, const SectionPos& pos);
```

### storage/ 目录 - 光照存储

#### SWMRNibbleArray.hpp/cpp

单写多读Nibble数组，参考Starlight设计。

**核心特性**：
- 写时复制 (Copy-on-Write)
- 延迟初始化
- 线程安全的读取
- 对象池减少内存分配

**状态管理**：
```cpp
enum class State : u8 {
    Null = 0,    // 不存在，所有读取返回0
    Uninit = 1,  // 未初始化（全零），不分配内存
    Init = 2,    // 已初始化，有实际数据
    Hidden = 3   // 已初始化但隐藏，对Vanilla视为NULL
};
```

**数据大小**：
- 数组大小：2048字节（4096个4位值）
- 最大值：15（4位）

**核心方法**：
```cpp
u8 getUpdating(i32 x, i32 y, i32 z) const;  // 更新侧读取
void set(i32 x, i32 y, i32 z, u8 value);    // 更新侧写入
u8 getVisible(i32 x, i32 y, i32 z) const;   // 可见侧读取（线程安全）
bool updateVisible();                        // 同步更新到可见侧

void setFull();   // 设置全亮 (15)
void setZero();   // 设置全零
void setNull();   // 设置为null状态
```

#### EmptinessMap.hpp/cpp

空区块段映射，用于优化光照计算。

**职责**：追踪每个区块中哪些区块段是全空气的，用于快速跳过空区块段。

**核心方法**：
```cpp
bool isSectionEmpty(i32 sectionY) const;      // 检查区块段是否为空
void setSectionEmpty(i32 sectionY, bool empty);  // 设置区块段空状态
bool updateFromChunk(const IChunk& chunk);    // 从区块更新映射
```

#### SectionLightStorage.hpp

区块段光照存储基类模板。

**管理内容**：
- 光照数据数组 (SWMRLightDataMap)
- 活跃区块段集合
- 待更新区块段队列
- 区块段状态变更追踪

**核心方法**：
```cpp
bool hasSection(i64 sectionPos) const;
SWMRNibbleArray* getArray(i64 sectionPos);
void setData(i64 sectionPos, SWMRNibbleArray&& array, bool retain);
void updateSectionStatus(i64 sectionPos, bool isEmpty);
void processAllLevelUpdates();  // 处理待更新
void updateAndNotify();  // 同步并通知变更
```

#### BlockLightStorage.hpp/cpp

方块光照存储，继承自 `SectionLightStorage<BlockLightDataMap>`。

**核心方法**：
```cpp
u8 getLightOrDefault(i64 worldPos) const;  // 不存在返回0
u8 getLight(i64 worldPos) const;
void setLight(i64 worldPos, u8 light);
```

#### SkyLightStorage.hpp/cpp

天空光照存储，继承自 `SectionLightStorage<SkyLightDataMap>`。

**额外功能**：
- 表面高度追踪
- 区块列启用/禁用
- 待添加/移除区块段管理

**核心方法**：
```cpp
u8 getLightOrDefault(i64 worldPos) const;  // 不存在返回15
bool isSectionEnabled(i64 sectionPos) const;
bool isAboveWorld(i64 sectionPos) const;
bool isAtSurfaceTop(i64 worldPos) const;
void setColumnEnabled(i64 columnPos, bool enabled);
```

#### SWMRLightDataMap.hpp

SWMR光照数据映射，管理区块段到光照数组的映射。

**核心方法**：
```cpp
SWMRNibbleArray* getArray(i64 sectionPos);
bool hasArray(i64 sectionPos) const;
SWMRNibbleArray& getOrCreateArray(i64 sectionPos);
void setArray(i64 sectionPos, SWMRNibbleArray array);
void removeArray(i64 sectionPos);
u8 getLight(i64 worldPos) const;
void setLight(i64 worldPos, u8 light);
void updateVisible();  // 同步所有数组到可见侧
```

## 模块关系图

```
                    ┌─────────────────────┐
                    │  WorldLightManager  │
                    │   (统一接口)         │
                    └──────────┬──────────┘
                               │
              ┌────────────────┼────────────────┐
              │                │                │
              ▼                ▼                ▼
    ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
    │ BlockLightEngine│ │ SkyLightEngine  │ │ InternalLight   │
    │  (方块光照)      │ │  (天空光照)      │ │  (游戏机制)     │
    └────────┬────────┘ └────────┬────────┘ └─────────────────┘
             │                   │
             │    ┌──────────────┴──────────────┐
             │    │                             │
             ▼    ▼                             ▼
    ┌─────────────────┐               ┌─────────────────┐
    │ LevelBasedGraph │               │LightEngineUtils │
    │  (BFS传播基类)   │               │   (工具方法)    │
    └────────┬────────┘               └─────────────────┘
             │
             ▼
    ┌─────────────────┐
    │LightEngineCache │
    │   (区块缓存)     │
    └─────────────────┘
             │
    ┌────────┴────────┐
    │                 │
    ▼                 ▼
┌─────────────────┐ ┌─────────────────┐
    │BlockLightStorage│ │ SkyLightStorage │
    │  (方块光照存储)  │ │  (天空光照存储)  │
    └────────┬────────┘ └────────┬────────┘
             │                   │
             └─────────┬─────────┘
                       │
                       ▼
           ┌─────────────────────┐
           │  SWMRLightDataMap   │
           │   (数据映射基类)     │
           └──────────┬──────────┘
                      │
                      ▼
           ┌─────────────────────┐
           │  SWMRNibbleArray    │
           │  (单写多读Nibble数组)│
           └─────────────────────┘
```

## 数据流

```
方块放置/移除
    │
    ▼
WorldLightManager.checkBlock(pos)
    │
    ├──────────────────────────┐
    │                          │
    ▼                          ▼
BlockLightEngine          SkyLightEngine
.checkLight(pos)          .checkLight(pos)
    │                          │
    ▼                          ▼
LevelBasedGraph           LevelBasedGraph
.scheduleUpdate(pos)      .scheduleUpdate(pos)
    │                          │
    ▼                          ▼
增亮/减亮队列              增亮/减亮队列
    │                          │
    ▼                          ▼
processUpdates()          processUpdates()
    │                          │
    ├──────────────────────────┤
    │                          │
    ▼                          ▼
BlockLightStorage         SkyLightStorage
.setLevel(pos, level)     .setLight(pos, light)
    │                          │
    ▼                          ▼
SWMRNibbleArray           SWMRNibbleArray
.set(x, y, z, value)      .set(x, y, z, value)
    │                          │
    ▼                          ▼
.updateVisible()          .updateVisible()
    │                          │
    └──────────┬───────────────┘
               │
               ▼
    IChunkLightProvider.markLightChanged()
               │
               ▼
        网络同步/渲染更新
```

## 使用方法

### 初始化

```cpp
// 创建世界光照管理器
auto lightManager = std::make_unique<WorldLightManager>(
    chunkProvider,  // 实现 IChunkLightProvider 的对象
    true,           // hasBlockLight
    true            // hasSkyLight (主世界)
);
```

### 区块加载时初始化光照

```cpp
// 加载区块时
void onChunkLoad(ChunkPos pos) {
    // 启用光照源
    lightManager->enableLightSources(pos, true);

    // 更新区块段状态
    for (int y = minSection; y <= maxSection; ++y) {
        SectionPos sectionPos(pos.x, y, pos.z);
        bool isEmpty = chunk->getSection(y)->isEmpty();
        lightManager->updateSectionStatus(sectionPos, isEmpty);
    }
}

// 卸载区块时
void onChunkUnload(ChunkPos pos) {
    lightManager->enableLightSources(pos, false);
}
```

### 方块变化时更新光照

```cpp
void onBlockChange(BlockPos pos, const BlockState* oldState, const BlockState* newState) {
    // 检查光照
    lightManager->checkBlock(pos);

    // 如果是光源方块
    if (newState && newState->lightLevel() > 0) {
        lightManager->onBlockEmissionIncrease(pos, newState->lightLevel());
    }
}
```

### 每Tick处理光照更新

```cpp
void tick() {
    // 检查是否有待处理工作
    if (lightManager->hasLightWork()) {
        // 处理最多 512 个更新
        lightManager->tick(512, hasSkyLight, hasBlockLight);
    }
}
```

### 查询光照

```cpp
// 获取原始光照值
u8 blockLight = lightManager->getBlockLight(pos);
u8 skyLight = lightManager->getSkyLight(pos);

// 获取实际亮度（考虑天空减暗）
i32 skyDarkening = InternalLight::calculateSkyDarkening(dayTime, isRaining, isThundering);
i32 brightness = lightManager->getLightSubtracted(pos, skyDarkening);

// 检查是否可以生成敌对生物
if (InternalLight::isDarkEnoughForSpawning(brightness)) {
    // 可以生成
}
```

## 依赖项

- `common/core/Types.hpp` - 基础类型定义
- `common/util/Direction.hpp` - 方向枚举
- `common/util/NibbleArray.hpp` - 标准Nibble数组
- `common/world/block/Block.hpp` - 方块定义
- `common/world/chunk/ChunkData.hpp` - 区块数据
- `common/world/chunk/ChunkPos.hpp` - 区块坐标
- `common/physics/collision/CollisionShape.hpp` - 碰撞形状

## 容易踩的坑

### 1. SWMRNibbleArray 状态管理

**问题**：忘记调用 `updateVisible()` 导致可见侧数据不同步。

**解决方案**：在每次批量修改光照数据后，必须调用 `updateVisible()` 同步到可见侧。

```cpp
// 错误示例
nibbleArray->set(x, y, z, value);
// 忘记 updateVisible()，客户端看到的是旧值

// 正确示例
nibbleArray->set(x, y, z, value);
nibbleArray->updateVisible();
```

### 2. 空区块段优化

**问题**：不更新空区块段映射导致光照计算跳过非空区块段。

**解决方案**：区块段内容变化时，必须调用 `updateSectionStatus()`。

```cpp
// 区块段从空变为非空
if (section->isEmpty() != wasEmpty) {
    lightManager->updateSectionStatus(sectionPos, section->isEmpty());
}
```

### 3. 天空光照表面追踪

**问题**：天空光照需要在表面初始化为15，但忘记启用区块列。

**解决方案**：区块加载时必须调用 `enableLightSources(pos, true)`。

```cpp
void onChunkLoad(ChunkPos pos) {
    // 重要：启用区块列以正确计算天空光照
    lightManager->enableLightSources(pos, true);
}
```

### 4. 光照更新顺序

**问题**：方块光照和天空光照更新顺序不一致导致闪烁。

**解决方案**：使用 `WorldLightManager::tick()` 统一管理更新配额分配。

### 5. 缓存未清理

**问题**：`LightEngineCache` 使用后未禁用导致内存泄漏。

**解决方案**：光照计算完成后调用 `disableCache()`。

```cpp
// 光照计算前
lightEngine->enableCache(centerX, centerY, centerZ);

// ... 执行光照计算 ...

// 光照计算后（重要！）
lightEngine->disableCache();
```

### 6. 方块透明度

**问题**：自定义方块未正确设置透明度导致光照穿透。

**解决方案**：确保所有方块都正确实现了 `getOpacity()` 方法。

```cpp
// 透明方块
u8 getOpacity() const override { return 0; }

// 部分透明方块（如水）
u8 getOpacity() const override { return 1; }

// 完全不透明方块
u8 getOpacity() const override { return 15; }
```

## 性能优化

### Starlight 优化实现

本光照系统参考了 [Starlight](https://github.com/PaperMC/Starlight) 的优化设计：

1. **方向位集优化**：使用位集表示传播方向，快速获取相反方向
2. **显式队列条目**：使用完整世界坐标 + 方向位集 + 标志位，避免坐标截断回绕
3. **空区块段跳过**：使用 `EmptinessMap` 快速跳过全空气区块段
4. **区块缓存**：5x5区块缓存减少重复区块查找

### 更新配额系统

```cpp
// 每tick限制光照更新数量，防止卡顿
i32 remaining = lightManager->tick(512, hasSkyLight, hasBlockLight);
if (remaining < 512) {
    // 更新未完成，下个tick继续
}
```

## 测试用例

测试文件位于 `tests/lighting/` 目录：

| 测试文件 | 测试内容 |
|---------|---------|
| `LightingTest.cpp` | NibbleArray、SectionPos、LightType、InternalLight、LightEngineUtils |
| `LightUpdateTest.cpp` | LightUpdatePacket 序列化/反序列化、ChunkSection 光照数据、ChunkData 光照存储 |
| `SkyLightRegressionTest.cpp` | 天空侧向补光、封顶变暗、开洞恢复、负Y表面检测 |
| `LevelBasedGraphQueueTest.cpp` | FIFO 队列顺序、tick预算边界、取消更新行为 |
| `BlockLightRegressionTest.cpp` | 发光方块增亮、移除减亮、遮挡削弱与解除恢复 |

### 关键测试用例

```cpp
// 天空减暗计算
TEST_F(InternalLightTest, CalculateSkyDarkening) {
    // 正午（dayTime = 6000）- 最亮，减暗为0
    i32 noonDarkening = InternalLight::calculateSkyDarkening(6000, false, false);
    EXPECT_EQ(noonDarkening, 0);

    // 下雨 - 天空变暗
    i32 rainDarkening = InternalLight::calculateSkyDarkening(6000, true, false);
    EXPECT_GT(rainDarkening, noonDarkening);
}

// NibbleArray 读写
TEST_F(NibbleArrayTest, SetAndGet) {
    array.set(5, 5, 5, 7);
    EXPECT_EQ(array.get(5, 5, 5), 7);
}

// 位置编码解码
TEST_F(LightEngineUtilsTest, PackUnpackPos) {
    i32 x = 100, y = 64, z = 50;
    i64 packed = LightEngineUtils::packPos(x, y, z);

    i32 unpackedX, unpackedY, unpackedZ;
    LightEngineUtils::unpackPos(packed, unpackedX, unpackedY, unpackedZ);

    EXPECT_EQ(unpackedX, x);
    EXPECT_EQ(unpackedY, y);
    EXPECT_EQ(unpackedZ, z);
}
```

## 参考

- [Minecraft 1.16.5 光照系统](https://minecraft.fandom.com/wiki/Light)
- [Starlight 优化实现](https://github.com/PaperMC/Starlight)
- `D:\Minecraft\MC研究\Minecraft1.16.5源码\net\minecraft\world\lighting`
