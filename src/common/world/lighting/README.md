# 光照系统 (Lighting System)

本目录实现了 Minecraft 1.16.5 风格的光照系统，包括天空光和方块光的传播计算。

## 目录结构

```
lighting/
├── LightType.hpp                 # 光源类型枚举
├── IChunkLightProvider.hpp       # 区块光照提供者接口
├── InternalLightUtils.hpp/cpp    # 内部光照工具函数
├── engine/                       # 光照引擎
│   ├── LightEngineUtils.hpp/cpp  # 光照引擎工具
│   ├── BaseLightEngine.hpp/cpp   # 光照引擎基类
│   ├── BlockLightEngine.hpp/cpp  # 方块光引擎
│   └── SkyLightEngine.hpp/cpp    # 天空光引擎
├── manager/                      # 光照管理
│   └── WorldLightManager.hpp/cpp # 世界光照管理器
└── storage/                      # 光照存储
    ├── SWMRNibbleArray.hpp/cpp   # 单写多读 Nibble 数组
    └── EmptinessMap.hpp/cpp      # 空隙图
```

---

## 模块详解

### 1. 光源类型 (`LightType.hpp`)

定义两种光源类型：

```cpp
enum class LightType : u8 {
    Sky = 0,    // 天空光（来自太阳/月亮）
    Block = 1   // 方块光（来自发光方块）
};
```

### 2. 光照引擎基类 (`BaseLightEngine`)

实现光照传播的核心算法：

- **FIFO 波前传播**：使用队列进行广度优先传播
- **级别存储**：天空光使用反转级别（0 = 最亮，15 = 最暗）
- **增量更新**：支持单个方块变更后的局部重算

### 3. 方块光引擎 (`BlockLightEngine`)

处理发光方块（火把、岩浆等）产生的光照：

- 从光源向外传播，亮度逐级衰减
- 不透明方块阻挡光线
- 支持动态光源更新

### 4. 天空光引擎 (`SkyLightEngine`)

处理来自天空的光照：

- 从天空向下传播
- 受不透明方块遮挡
- 使用"最低 Y"优化减少传播计算

### 5. 世界光照管理器 (`WorldLightManager`)

协调天空光和方块光引擎：

```cpp
WorldLightManager lightManager(provider, hasBlockLight, hasSkyLight);

// 方块变更时触发重算
lightManager.checkBlock(pos);

// 每 tick 处理光照更新
lightManager.tick(maxUpdates, updateSky, updateBlock);
```

### 6. 光照存储 (`SWMRNibbleArray`)

单写多读的 Nibble 数组：

- 4 位值存储（0-15 光照等级）
- 线程安全的读取
- 写入需要同步

---

## 整体职责

本模块负责 Minecraft 世界的光照计算：

1. **光照传播**：从光源向外传播光照
2. **增量更新**：方块变更后局部重算
3. **多线程支持**：读取可在任意线程进行
4. **性能优化**：预算控制、传播限制

---

## 输入和输出

### 输入

| 输入项 | 类型 | 来源 | 说明 |
|--------|------|------|------|
| 方块变更 | `BlockPos` | 世界管理器 | 触发光照更新 |
| 光照预算 | `i32` | 游戏循环 | 每 tick 最大更新数 |
| 区块数据 | `ChunkData` | 区块管理器 | 方块状态查询 |

### 输出

| 输出项 | 类型 | 目标 | 说明 |
|--------|------|------|------|
| 天空光 | `NibbleArray` | 区块数据 | 天空光照等级 |
| 方块光 | `NibbleArray` | 区块数据 | 方块光照等级 |
| 更新计数 | `i32` | 统计 | 本 tick 处理的更新数 |

---

## 依赖项

### 内部依赖

- `common/world/chunk/ChunkData.hpp` - 区块数据
- `common/world/block/BlockState.hpp` - 方块状态
- `common/util/NibbleArray.hpp` - 4 位数组

### 外部依赖

- `spdlog` - 日志
- 标准库

---

## 使用方法

### 初始化光照管理器

```cpp
#include "lighting/manager/WorldLightManager.hpp"

// 创建光照管理器
mc::world::lighting::WorldLightManager lightManager(
    chunkProvider,
    true,   // 启用方块光
    true    // 启用天空光
);
```

### 处理方块变更

```cpp
// 当方块变更时
lightManager.checkBlock(pos);

// 每 tick 处理更新
void tick() {
    lightManager.tick(
        500,   // maxUpdates: 每 tick 最大更新数
        true,  // updateSky: 更新天空光
        true   // updateBlock: 更新方块光
    );
}
```

### 查询光照等级

```cpp
// 通过区块数据查询
u8 skyLight = chunkData->getSkyLight(x, y, z);
u8 blockLight = chunkData->getBlockLight(x, y, z);
```

---

## 容易踩的坑

### 1. 天空光内部级别反转

**问题**：天空光引擎内部存储的级别是反转的（`0` 最亮，`15` 最暗）。

**解决**：从原版/Starlight 代码移植逻辑时，必须转换级别。只有外部 API（如 `getSkyLight()`）返回的是正确级别（15 = 最亮）。

### 2. 队列处理顺序

**问题**：使用 LIFO（后进先出）处理光照队列会导致复杂遮挡下的振荡和延迟收敛。

**解决**：`BaseLightEngine` 队列处理必须使用 FIFO（先进先出）。不要切换回 LIFO（`--length` 弹出末尾）。

### 3. 未初始化区块的光照引导

**问题**：从未初始化的区块 nibble 数组引导光照会导致错误。

**解决**：
1. 从临时 NULL 状态 nibble 开始
2. 运行 `handleEmptySectionChanges(..., isUnlit=true)`
3. 执行 `lightChunk(...)`
4. 将生成的 nibble 写回区块

### 4. 空隙缓存种子

**问题**：未初始化引导时从陈旧的默认映射种子空隙缓存，会导致 `SkyStarLightEngine::initNibble(...)` 计算错误的 `lowestY`。

**解决**：先将缓存种子设为 null，再执行光照引导。

### 5. 光照 API 默认参数

**问题**：默认参数会导致数据流难以追踪。

**解决**：如果调用模式需要简化形式，添加重载而不是默认参数。不要为光照子系统重新引入默认参数。

### 6. 世界坐标到段坐标转换

**问题**：在存储映射中重新引入临时位解码逻辑，可能静默地将写入/读取路由到错误的段。

**解决**：通过 `LightEngineUtils::worldToSectionPos(...)` 保持世界->段转换集中化。

### 7. 源面遮挡判断

**问题**：盲目地将源面阻止应用于所有方块光传播，会导致全立方体发光源无法向外传播。

**解决**：源面遮挡检查应针对条件形状，全立方体发光源仍需要向外传播。

### 8. 减少传播边缘情况

**问题**：在减少传播期间处理 `currentLevel < targetLevel` 时，阻塞边缘（`target=darkest`）情况不能总是被视为"安全存活的源"。

**解决**：首先强制清除 + 减少级联，并将相邻的增加重检排队；否则在 FIFO 波前执行下侧面天空光可能会丢失。

### 9. 天空光屋顶闭合

**问题**：将不透明源门控应用于减少路径，会导致屋顶下的陈旧光永远不会被移除。

**解决**：对于天空光屋顶闭合修复，只从不透明源方块阻止增加传播，不要将相同的门控应用于减少路径。

### 10. 队列条目坐标

**问题**：旧的 BaseLightEngine 队列使用 6 位 X/Z 截断，在远离原点时会导致天空/方块光严重不同步。

**解决**：光照队列条目现在携带完整的世界坐标；不要重新引入截断坐标的旧语义。

### 11. 客户端光包处理

**问题**：将客户端光包视为立即网格重建触发器，会导致重复的网格重建任务。

**解决**：`ClientWorld` 现在使用 `meshRebuildPending` 来合并同一区块的重复 `onLightUpdate()` 调用，而任务仍处于活动状态。

### 12. WorldLightManager tick 预算消耗

**问题**：天空光和方块光预算分配不当会导致某一类更新饥饿。

**解决**：`WorldLightManager::tick(...)` 现在依赖有序的预算消耗。除非重新设计预算模型并匹配测试，否则不要恢复之前的五五开分配。

### 13. BlockPos 包装器重载

**问题**：在 `WorldLightManager` 方块更新或光照查询入口点上重新引入 `BlockPos` 包装器重载，会导致接口膨胀。

**解决**：原始坐标现在是光照调度的规范接口。不要添加 `BlockPos` 重载。

### 14. 测试未初始化世界

**问题**：测试 `ServerWorld::setBlock()` 时，未初始化的世界会触发光照更新断言路径（`MC_ASSERT_RELEASE(false)`），因为 `m_lightManager` 为 null。

**解决**：测试前先初始化世界。

---

## 涉及的测试用例

| 测试文件 | 测试内容 |
|----------|----------|
| `tests/common/world/lighting/` | 光照引擎单元测试 |
| `LightEngineTest.cpp` | 光照传播基础测试 |
| `SkyLightTest.cpp` | 天空光传播测试 |
| `BlockLightTest.cpp` | 方块光传播测试 |

---

## 参考资料

- Minecraft 1.16.5 源码：`D:\Minecraft\MC研究\Minecraft1.16.5源码\net\minecraft\world\light`
- Starlight 光照引擎优化
- MC Wiki 光照：https://minecraft.fandom.com/wiki/Light

---

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| 1.0.0 | 2025-04 | 初始版本，实现 Starlight 风格光照引擎 |
