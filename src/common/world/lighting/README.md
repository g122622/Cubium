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
- `common/world/block/blocks/mob/TurtleEggBlock.cpp` - 使用 `getCelestialAngleMC()` 判断黎明
- `common/world/block/blocks/redstone/DaylightDetectorBlock.cpp` - 使用天体角度计算日光探测器输出
- `client/renderer/trident/entity/util/ShadowRenderer.cpp` - 使用天体角度计算阴影

---

## 区块列状态管理

### 概述

光照引擎通过区块列（column）粒度的状态标记来控制光照数据的生命周期，对应 MC Java 的
`columnsWithSources` 和 `columnsToRetainQueuedDataFor` 集合。两个状态互相独立：

| 状态 | 集合 | 作用 | 对应 MC Java |
|------|------|------|-------------|
| 列启用 | `m_enabledColumns` | 控制该区块列是否有活跃光源 | `LayerLightSectionStorage.columnsWithSources` |
| 数据保留 | `m_columnsToRetainDataFor` | 在区块卸载时保留光照数据而非清除 | `LayerLightSectionStorage.columnsToRetainQueuedDataFor` |

### 区块列位置编码

区块列位置使用 64 位整数编码，格式与 MC Java `SectionPos.asLong()` 的列部分一致：

- 位 [63:42] — chunkX（22 位，符号扩展后与 `& 0x3FFFFF` 掩码）
- 位 [41:20] — chunkZ（22 位，符号扩展后与 `& 0x3FFFFF` 掩码）
- 位 [19:0]  — 保留（用于段 Y 坐标，列级操作中为 0）

编码公式：

```cpp
i64 columnPos = (static_cast<i64>(chunkX) & 0x3FFFFFLL) << 42
              | (static_cast<i64>(chunkZ) & 0x3FFFFFLL) << 20;
```

**一致性要求**：`enableLightSources(ChunkPos)` 中的 `pos.x/pos.z` 必须与
`_getLightEmission` 中的 `x >> CHUNK_SHIFT / z >> CHUNK_SHIFT` 产生相同的区块坐标，
否则列启用检查会因坐标不匹配而失效。

### 列启用（Column Enabled）

控制点：

1. **方块光发射门控** — `BlockStarLightEngine::_getLightEmission()` 在返回发射等级前检查
   `isColumnEnabled()`，未启用的列中发光方块的发射等级被抑制为 0（对应 MC Java
   `BlockLightEngine.getEmission()` 中的 `lightOnInSection` 检查）。

2. **天空光初始化门控** — `SkyStarLightEngine::initNibble()` 在初始化高空段时，
   仅对已启用列执行 `setFull()`（填充亮度 15），未启用列只执行 `setNonNull()`（对应
   MC Java `SkyLightSectionStorage.createDataLayer()` 中的 `lightOnInSection` 检查）。

3. **不影响的路径** — `canUseChunk()` **不**检查列启用状态，与 MC Java 保持一致。
   列启用仅影响光源发射和初始化行为，不影响区块可用性判断。

### 数据保留（Data Retention）

控制点：

- `setNibbleNull()` — 当区块段被卸载时调用。若该列标记了数据保留（`isDataRetained()`），
  则执行 `setHidden()` 而非 `setNull()`，将 nibble 数组置于 Hidden 状态而非完全清除。
  Hidden 状态保留数据但停止光照传播，当区块重新加载时可以快速恢复（对应 MC Java
  `LayerLightSectionStorage.markNewInconsistencies()` 中的保留路径）。

### 调试信息

`WorldLightManager::getDebugInfo()` 报告以下信息：

- 段状态：`0` = 有完整数据（LIGHT_AND_DATA），`1` = 仅光照（LIGHT_ONLY），`2` = 空（EMPTY）
- `[dirty]` — nibble 数据已修改未同步
- `[q:N]` — 引擎队列中有 N 个待处理更新
- `[col:on]` — 区块列已启用
- `[retained]` — 区块列数据已保留

### API 一览

#### WorldLightManager

| 方法 | 说明 |
|------|------|
| `enableLightSources(ChunkPos, bool)` | 启用/禁用区块列光源，委托至双引擎 |
| `retainData(ChunkPos, bool)` | 保留/释放区块列光照数据，委托至双引擎 |
| `getDebugInfo(LightType, SectionPos)` | 获取段级调试信息 |

#### BlockStarLightEngine / SkyStarLightEngine

| 方法 | 说明 |
|------|------|
| `setColumnEnabled(i64, bool)` | 设置列启用状态 |
| `isColumnEnabled(i64)` | 查询列启用状态 |
| `retainData(i64, bool)` | 设置数据保留标记 |
| `isDataRetained(i64)` | 查询数据保留标记 |
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
