# 光照系统 (Lighting System)

本目录实现了 Minecraft 1.16.5 风格的光照系统，包括天空光和方块光的传播计算。

## 目录结构

```
lighting/
├── LightType.hpp                 # 光源类型枚举（Sky/Block）
├── IChunkLightProvider.hpp       # 区块光照提供者接口（ServerWorld/ClientWorld 实现）
├── InternalLightUtils.hpp/cpp    # 内部光照工具函数（天体角度、天空减暗、月相等）
├── engine/                       # 光照引擎
│   ├── LightEngineUtils.hpp/cpp  # 光照引擎工具（坐标转换、方向枚举等）
│   ├── BaseLightEngine.hpp/cpp   # 光照引擎基类（StarLightEngine，FIFO 波前传播）
│   ├── BlockLightEngine.hpp/cpp  # 方块光引擎（发光方块传播）
│   └── SkyLightEngine.hpp/cpp    # 天空光引擎（天空光向下传播）
├── manager/                      # 光照管理
│   └── WorldLightManager.hpp/cpp # 世界光照管理器（协调双引擎）
└── storage/                      # 光照存储
    ├── SWMRNibbleArray.hpp/cpp   # 单写多读 Nibble 数组（支持 Copy-on-Write）
    └── EmptinessMap.hpp/cpp      # 空隙图（优化空区块段检测）
```

---

## 内部模块关系

```
WorldLightManager
    ├── BlockStarLightEngine (方块光引擎)
    │       └── StarLightEngine (基类)
    │               └── LightEngineUtils (工具函数)
    └── SkyStarLightEngine (天空光引擎)
            └── StarLightEngine (基类)
                    └── LightEngineUtils (工具函数)

StarLightEngine (基类)
    ├── SWMRNibbleArray (光照数据存储)
    └── EmptinessMap (空隙图)

InternalLightUtils (独立工具模块，无依赖其他 lighting 模块)
```

---

## 上下游外部依赖关系

### 本模块依赖的外部模块

- `common/world/chunk/ChunkData.hpp` - 区块数据（IChunk、ChunkSection）
- `common/world/block/BlockState.hpp` - 方块状态查询
- `common/world/block/Block.hpp` - 方块发光等级、透明度查询
- `common/util/NibbleArray.hpp` - 基础 4 位数组
- `common/world/WorldConstants.hpp` - 世界高度常量（MIN_BUILD_HEIGHT、MAX_BUILD_HEIGHT 等）

### 依赖本模块的外部模块

- `common/world/IWorld.hpp` - 使用 `InternalLightUtils::calculateSkyDarkening()` 计算天空减暗
- `common/world/dimension/Dimension.hpp` - 持有 `WorldLightManager` 实例
- `server/world/ServerWorld.hpp` - 持有 `WorldLightManager`；运行时方块变更经 `ServerLightQueue` 延迟入队，tick 时 submit `RuntimeLightTask` 到 worker 池异步传播（`checkBlocksWithProvider`），主线程 drain flush 队列调 `markLightChanged`
- `common/world/block/blocks/mob/TurtleEggBlock.cpp` - 使用 `getCelestialAngleMC()` 判断黎明
- `common/world/block/blocks/redstone/DaylightDetectorBlock.cpp` - 使用天体角度计算日光探测器输出
- `client/renderer/trident/entity/util/ShadowRenderer.cpp` - 使用天体角度计算阴影

---

## 区块列状态管理

光照引擎**不**维护任何区块列（column）粒度的持久化状态集合。光照数据纯挂在
chunk 上（`IChunk::getSkyNibbles()`/`getBlockNibbles()`），chunk 被移除即光照数据消失，
无需卸载时清理引擎内部集合。这与 Moonrise（Starlight）架构一致——Moonrise 的
`LayerLightSectionStorage` 没有 `columnsWithSources`/`columnsToRetainQueuedDataFor`
这类跨 chunk 生命周期的列集合。

### nibble 去初始化语义

`setNibbleNull()`（区块段卸载时调用）按光照类型区分：

| 光照类型 | 行为 | 原因 |
|---------|------|------|
| 天空光 | 无条件 `setNull()` | 方块破坏只会增加天空光（方块阻挡消除），增亮传播可正确穿过 null 段，无需保留数据 |
| 方块光 | 无条件 `setHidden()` | 方块光减亮通常因方块被移除，减亮传播需要保留数据才能正确计算；Hidden 保留数据但停止传播 |

### nibble 初始化语义

`SkyStarLightEngine::initNibble()` 在最高非空区块段之上**无条件** `setFull()`（填充
亮度 15）——天空光对未遮挡列填满。方块光引擎无此初始化路径（方块光仅由发光方块
自身发射，`BlockStarLightEngine::_getLightEmission()` 始终按方块自身亮度返回，无门控）。

### 调试信息

`WorldLightManager::getDebugInfo()` 报告以下信息：

- 段状态：`0` = 有完整数据（LIGHT_AND_DATA），`1` = 仅光照（LIGHT_ONLY），`2` = 空（EMPTY）
- `[dirty]` — nibble 数据已修改未同步
- `[q:N]` — 引擎队列中有 N 个待处理更新

### API 一览

#### WorldLightManager

| 方法 | 说明 |
|------|------|
| `getDebugInfo(LightType, SectionPos)` | 获取段级调试信息 |
| `checkBlocks(chunkX, chunkZ, positions)` | 批量重算一个区块内多个方块变更的光照（一次 setupCaches/destroyCaches），供运行时延迟队列 fallback 同步路径调用 |
| `checkBlocksWithProvider(provider, chunkX, chunkZ, positions)` | 同 `checkBlocks` 但用传入的 worker provider（`RuntimeLightingProvider`），持 `m_mutex` 串行化 nibble 单写者；供 `RuntimeLightTask` 在 worker 线程调用 |

#### BlockStarLightEngine / SkyStarLightEngine

| 方法 | 说明 |
|------|------|
| `getData(SectionPos) const` | 获取段 nibble 数据（只读） |

---

## 容易踩的坑

### 1. 天空光内部级别反转

**问题**：天空光引擎内部存储的级别是反转的（`0` 最亮，`15` 最暗）。

**解决**：从原版/Starlight 代码移植逻辑时，必须转换级别。只有外部 API（如 `getSkyLight()`）返回的是正确级别（15 = 最亮）。

### 2. 队列处理顺序

**问题**：使用 LIFO（后进先出）处理光照队列会导致复杂遮挡下的振荡和延迟收敛。

**解决**：`StarLightEngine` 队列处理必须使用 FIFO（先进先出）。不要切换回 LIFO（`--length` 弹出末尾）。

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

**问题**：旧的队列使用 6 位 X/Z 截断，在远离原点时会导致天空/方块光严重不同步。

**解决**：光照队列条目现在携带完整的世界坐标；不要重新引入截断坐标的旧语义。

### 11. 客户端光包处理

**问题**：将客户端光包视为立即网格重建触发器，会导致重复的网格重建任务。

**解决**：`ClientWorld` 使用 `meshRebuildPending` 合并同一区块的重复 `onLightUpdate()` 调用。

### 12. WorldLightManager tick 预算消耗

**问题**：天空光和方块光预算分配不当会导致某一类更新饥饿。

**解决**：`tick(...)` 依赖有序的预算消耗，不要恢复五五开分配。

### 13. BlockPos 包装器重载

**问题**：在 `WorldLightManager` 方块更新或光照查询入口点上重新引入 `BlockPos` 包装器重载，会导致接口膨胀。

**解决**：原始坐标现在是光照调度的规范接口，不要添加 `BlockPos` 重载。

### 14. 测试未初始化世界

**问题**：测试 `ServerWorld::setBlockState()` 时，未初始化的世界会触发光照更新断言路径（`MC_ASSERT_RELEASE(false)`），因为 `m_lightManager` 为 null。

**解决**：测试前先初始化世界。

### 15. 两种天体角度函数

`InternalLightUtils` 提供两种天体角度计算，用途不同：

- `getCelestialAngle()` - 简化线性映射，用于光照计算
- `getCelestialAngleMC()` - MC 1.16.5 原版公式，用于游戏机制（如海龟蛋孵化判断黎明）

**坑**：两者返回值在相同 `dayTime` 下不同，使用时需确认场景。
