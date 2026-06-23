# WorldGenRegion 访问窗口 Bug 交接文档

## 1. 问题描述

跑图时出现两类运行时错误：

```
[error] [WorldGenRegion] missing chunk in access window:
  requested=(-5, 3), center=(-7, -2), distance=5,
  generatingStatus=structure_references, requestedStatus=structure_starts,
  allowedStatus=structure_starts

[error] [WorldGenRegion] chunk status below request:
  requested=(-6, 2), center=(-7, 2), distance=1,
  generatingStatus=features, requestedStatus=carvers,
  allowedStatus=carvers, actualStatus=biomes
```

- **missing chunk in access window**：WorldGenRegion 窗口内某位置的区块指针为 nullptr
- **chunk status below request**：窗口内区块存在但实际状态低于请求状态

当前断言被临时禁用，所以这两个错误只打日志不崩溃。一旦断言恢复，它们会导致程序终止。

---

## 2. 相关代码文件索引

| 文件 | 关键行 | 说明 |
|------|--------|------|
| `src/common/world/gen/chunk/IChunkGenerator.cpp` | 54-63 | `actualChunkStatus()` 辅助函数 |
| 同上 | 85-100 | WorldGenRegion(ChunkStep) 构造函数 |
| 同上 | 138-201 | `getIChunk(x,z,requestedStatus)` 三重校验逻辑 |
| `src/server/world/ServerChunkManager.cpp` | 69-75 | `chunkHasCompletedStatus()` 辅助函数 |
| 同上 | 77-101 | `assertRegionSatisfiesDirectDependencies()` |
| 同上 | 332-419 | `_enqueueChunkGenerationAsync()` 异步生成路径 |
| 同上 | 571-604 | `_executeGenerationSync()` 同步生成路径 |
| 同上 | 606-631 | `_doGenerateChunkToTargetStatus()` 核心生成循环 |
| 同上 | 633-671 | `_prepareStepDependencies()` 递归生成依赖区块 |
| 同上 | 776-829 | `_collectNeighborChunks()` 收集邻居区块到窗口数组 |
| 同上 | 831-869 | `_doCreateWorldGenRegion()` 创建 WorldGenRegion |
| `src/common/world/gen/chunk/NoiseChunkGenerator.cpp` | 240-288 | `generateStructureReferences()` 扫描 -8..8 |
| `src/server/world/GenerationChunkCache.cpp` | 52-65 | `getOrCreateOwned()` 缓存越界访问风险点 |
| `src/common/world/chunk/gen/ChunkPyramid.cpp` | 43-93 | `buildAccumulatedDependencies()` 累积依赖计算 |
| 同上 | 105-123 | `addRequirement()` 直接依赖构建 |
| `tests/common/world/gen/chunk/WorldGenRegionAccessTest.cpp` | 全文 | 复现测试 |

---

## 3. 区块生成管线概览

### 3.1 ChunkStatus 与 ChunkStep

区块生成按以下 12 个阶段顺序推进：

| 序号 | ChunkStatus | directDeps 半径→状态 | accumulatedRadius |
|------|-------------|----------------------|-------------------|
| 0 | EMPTY | （无） | 0 |
| 1 | STRUCTURE_STARTS | [0:EMPTY] | 0 |
| 2 | STRUCTURE_REFERENCES | [0-8:STRUCTURE_STARTS] | 8 |
| 3 | BIOMES | [0:STRUCTURE_REFERENCES, 1-8:STRUCTURE_STARTS] | 8 |
| 4 | NOISE | [0-1:BIOMES, 2-8:STRUCTURE_STARTS] | 9 |
| 5 | SURFACE | [0:NOISE, 1:BIOMES, 2-8:STRUCTURE_STARTS] | 9 |
| 6 | CARVERS | [0:SURFACE, 1-8:STRUCTURE_STARTS] | 9 |
| 7 | FEATURES | [0-1:CARVERS, 2-8:STRUCTURE_STARTS] | 10 |
| 8 | INITIALIZE_LIGHT | [0:FEATURES] | 10 |
| 9 | LIGHT | [0-1:INITIALIZE_LIGHT] | 11 |
| 10 | SPAWN | [0:LIGHT, 1:BIOMES] | 11 |
| 11 | FULL | [0:SPAWN] | 11 |

- **directDeps**：本步骤直接要求的邻居状态，按 Chebyshev 距离索引
- **accumulatedRadius**：合并所有前序步骤后，需要的最大邻居半径

### 3.2 生成入口

生成有两个入口：

1. **同步路径** `_executeGenerationSync()`（:571-604）：
   - 创建 `GenerationChunkCache(centerX, centerZ, cacheRadius)`，`cacheRadius = targetStep.accumulatedRadius()`
   - 对于目标 FULL，cacheRadius = 11，覆盖 23×23 区块
   - 调用 `_doGenerateChunkToTargetStatus(primer, FULL, cache)`

2. **异步路径** `_enqueueChunkGenerationAsync()`（:332-419）：
   - 同样计算 cacheRadius，在 worker 线程中执行生成

### 3.3 生成循环

`_doGenerateChunkToTargetStatus()`（:606-631）按状态顺序遍历：

```cpp
for (const auto& status : allStatuses) {
    if (status.ordinal() > targetStatus.ordinal()) break;
    if (chunk.hasCompletedStatus(status)) continue;

    const ChunkStep& step = pyramid.getStepTo(status);
    _prepareStepDependencies(chunk, step, cache);          // ← 递归生成依赖区块
    const i32 regionRadius = step.accumulatedRadius() > 0 ? step.accumulatedRadius() : 0;
    auto context = _doCreateWorldGenRegion(chunk, regionRadius, &cache, &step);  // ← 构建窗口
    _executeStepTask(chunk, status, *context.region);        // ← 执行生成
    chunk.setPersistedStatus(status);
    chunk.setChunkStatus(status);
}
```

### 3.4 依赖准备

`_prepareStepDependencies()`（:633-671）遍历 directDependencies 的每个半径环：

```cpp
for (i32 radius = 0; radius < deps.size(); ++radius) {
    const ChunkStatus* requiredStatus = deps.get(radius);
    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            if (dx == 0 && dz == 0) continue;
            // 1. 从 cache 获取
            ChunkPrimer* dependency = cache.get(nx, nz);
            if (dependency == nullptr) {
                // 2. 从内存获取已加载区块
                if (auto loadedChunk = tryToGetChunkSharedInMem(nx, nz)) {
                    MC_ASSERT_RELEASE(chunkHasCompletedStatus(*loadedChunk, *requiredStatus));
                    continue;  // ← 已加载区块，跳过递归
                }
                // 3. 创建新 Primer 并递归生成
                dependency = &cache.getOrCreateOwned(nx, nz);
            }
            if (!dependency->hasCompletedStatus(*requiredStatus)) {
                _doGenerateChunkToTargetStatus(*dependency, *requiredStatus, cache);  // ← 递归
            }
        }
    }
}
```

### 3.5 邻居收集

`_collectNeighborChunks()`（:776-829）按优先级填充 WorldGenRegion 的区块数组：

```
1. cache->get(nx, nz)           — 当前任务的缓存（状态正确）
2. getGeneratingPrimer(nx, nz)  — 其他正在生成的任务的 Primer（状态不确定！）
3. tryToGetChunkSharedInMem()   — 已完成的 ChunkData（状态 FULL）
4. nullptr                       — 三者都没有
```

### 3.6 WorldGenRegion 访问校验

`getIChunk(x, z, requestedStatus)`（:138-201）执行三重检查：

1. **状态权限检查**：`allowedStatus = step.directDependencies().get(distance)`，`requestedStatus` 必须 `isOrBefore(allowedStatus)`
2. **空指针检查**：如果 `allowedStatus != nullptr` 但区块指针为 nullptr → **"missing chunk in access window"**
3. **状态不足检查**：区块存在但 `actualStatus < requestedStatus` → **"chunk status below request"**

---

## 4. 根本原因分析

### 4.1 "missing chunk in access window" 的根因

**场景复现**：中心区块 C(-7,-2) 生成到 STRUCTURE_REFERENCES 阶段。

1. `_prepareStepDependencies(C, STRUCTURE_REFERENCES)` 遍历半径 0-8
2. 对于每个邻居 N，从 cache 获取或递归生成到 STRUCTURE_STARTS
3. `_doCreateWorldGenRegion(C, radius=8)` → `_collectNeighborChunks(C, 8, cache, step)`
4. `_collectNeighborChunks` 对每个位置尝试 cache→getGeneratingPrimer→tryToGetChunkSharedInMem→nullptr

**Bug 发生条件**：当邻居 N 不在 cache 中（`cache.get()` 返回 nullptr），同时 `getGeneratingPrimer()` 也返回 nullptr，且 `tryToGetChunkSharedInMem()` 也返回 nullptr 时，该位置设为 nullptr。

**为什么 N 不在 cache 中？** `_prepareStepDependencies` 应该已经递归生成了所有依赖区块。但有一个关键疏漏：

**`_prepareStepDependencies` 只遍历 `step.directDependencies()`，而 `_doCreateWorldGenRegion` 使用 `step.accumulatedRadius()` 构建窗口。**

对于 STRUCTURE_REFERENCES 步骤，这两者恰好一致（都是半径 8），所以不是问题。但递归生成的依赖区块自身可能需要更大范围的缓存——详见 4.3。

### 4.2 "chunk status below request" 的根因

**场景复现**：中心区块 C(-7,2) 生成到 FEATURES 阶段。

1. `_prepareStepDependencies(C, FEATURES)` 要求距离 0-1 的邻居达到 CARVERS
2. 对邻居 N(-6,2)（距离 1），cache 获取或递归生成到 CARVERS
3. `_doCreateWorldGenRegion(C, radius=10)` → `_collectNeighborChunks(C, 10, cache, step)`
4. **关键**：`_collectNeighborChunks` 先查 `cache->get(-6,2)`，如果 cache 中有则用它（正确状态）
5. 但如果 N 正在另一个线程生成（异步路径），`getGeneratingPrimer(-6,2)` 可能返回一个只到 BIOMES 的 Primer
6. 如果 cache.get() 返回 nullptr（不在当前任务缓存中），则 fallback 到 getGeneratingPrimer()
7. getGeneratingPrimer() 返回一个 BIOMES 状态的 Primer → **"chunk status below request"**

**根本原因**：`_collectNeighborChunks` 的第二优先级 `getGeneratingPrimer()` 会返回状态不确定的 Primer，而 `_prepareStepDependencies` 没有考虑这个来源，因此没有确保这些 Primer 达到所需状态。

### 4.3 缓存越界风险

当递归生成依赖区块时，依赖区块自身可能需要更大范围的邻居。例如：

- 中心 C 生成到 FULL，缓存半径 = 11，覆盖 C±11
- 递归到 C 的邻居 N（距 C 为 8）生成到 STRUCTURE_STARTS，N 的 STRUCTURE_STARTS 步骤 accumulatedRadius = 0，无需额外邻居 ✓
- 但如果 N 需要生成到 BIOMES（比如 C 的 NOISE 步骤需要 N 达到 BIOMES），N 的 BIOMES 步骤 directDeps 半径为 8，需要 N±8 的邻居达到 STRUCTURE_STARTS。N 距 C 为 8，N+8 = C+16，**超出缓存范围 C±11**！
- 此时 `_prepareStepDependencies` 中的 `cache.contains(nx, nz)` 返回 false，`MC_ASSERT_RELEASE(cache.contains(nx, nz))` 断言失败（当前被禁用），`cache.getOrCreateOwned()` 对越界坐标触发越界写入

这是一个潜在的**内存安全风险**。当前断言被禁用所以不会崩溃，但可能导致缓存数组越界访问。

### 4.4 `_prepareStepDependencies` 与 `_collectNeighborChunks` 的不一致

| 查找源 | `_prepareStepDependencies` | `_collectNeighborChunks` |
|--------|---------------------------|--------------------------|
| GenerationChunkCache | ✅ cache.get() | ✅ cache->get() |
| getGeneratingPrimer | ❌ 不查 | ✅ 第二优先级 |
| tryToGetChunkSharedInMem | ✅ 已加载区块 | ✅ 第三优先级 |
| cache.getOrCreateOwned | ✅ 创建新 Primer | ❌ 不创建 |

这个不一致导致：
- `_prepareStepDependencies` 为一个位置创建了新 Primer 并递归生成到正确状态
- 但 `_collectNeighborChunks` 可能拾取了 `getGeneratingPrimer()` 返回的另一个低状态 Primer

---

## 5. GenerationChunkCache 越界访问细节

`GenerationChunkCache::_inBounds()`（:99-103）检查坐标是否在缓存范围内。`getOrCreateOwned()`（:52-65）在入口处断言 `_inBounds`，但断言被禁用时，越界坐标会通过 `_index()` 计算出越界索引，导致未定义行为。

当递归生成依赖区块时，如果依赖区块的步骤需要超出主缓存范围的邻居，就会触发这个越界。具体场景：

- 主缓存：中心 C，半径 11（23×23）
- 递归生成 C+8 处的邻居 N 到 CARVERS
- N 的 NOISE 步骤需要 N±8 的邻居达到 STRUCTURE_STARTS
- N+8 = C+16，超出 C+11 的缓存范围

---

## 6. 对标 Minecraft 1.21.11 原版实现

MC 1.21.11 的区块生成使用完全不同的架构（`ChunkGenerationTask` + `StaticCache2D<GenerationChunkHolder>`）：

1. **预分配完整缓存**：创建 ChunkGenerationTask 时，一次性预分配 `accumulatedRadius` 范围内的所有 GenerationChunkHolder（相当于我们的缓存满载，不会有空洞）
2. **逐层推进**：`scheduleLayer()` 按状态层推进，每层遍历缓存内所有区块，对每个区块调用 `applyStep()` 并等待完成
3. **无 getGeneratingPrimer fallback**：所有区块都在同一个 StaticCache2D 中，不存在跨任务查找
4. **每层独立缓存半径**：`getRadiusForLayer()` 返回该层在 FULL 的 accumulatedDependencies 中的半径，确保窗口覆盖所有前序步骤的依赖

关键代码参考：
- `ChunkGenerationTask.create()`：`StaticCache2D.create(pos.x, pos.z, accumulatedRadiusOf_EMPTY, ...)`，半径 = FULL 的 accumulatedDependencies 中 EMPTY 的半径
- `scheduleLayer()`：按状态层遍历缓存中所有区块
- `getRadiusForLayer()`：`generationPyramid.getStepTo(targetStatus).getAccumulatedRadiusOf(layerStatus)`

---

## 7. 修复方案

### 方案 A：最小修复（快速止血）

修改 `_collectNeighborChunks`，**消除 getGeneratingPrimer fallback**，只从 cache 和已加载区块获取：

```cpp
// 修改 _collectNeighborChunks 中邻居查找逻辑
// 1. 从缓存获取
if (cache != nullptr) {
    ChunkPrimer* cachedPrimer = cache->get(nx, nz);
    if (cachedPrimer != nullptr) {
        neighbors[index] = cachedPrimer;
        continue;
    }
}

// 2. 从内存获取已加载区块（状态一定是 FULL）
if (auto loadedChunk = tryToGetChunkSharedInMem(nx, nz)) {
    loadedNeighbors[index] = std::move(loadedChunk);
    neighbors[index] = loadedNeighbors[index].get();
    continue;
}

// 3. 不再有 getGeneratingPrimer fallback
// 如果到这里，说明该位置没有缓存也没有已加载区块 → nullptr
neighbors[index] = nullptr;
```

同时在 `_prepareStepDependencies` 中，确保所有 directDependencies 范围内的区块都被递归生成并放入缓存。但这只解决了"状态不足"问题，**不解决缓存越界和缓存空洞问题**。

**风险**：移除 getGeneratingPrimer 后，某些依赖区块可能找不到（其他任务正在生成但尚未完成），导致更多 "missing chunk" 错误。需要在 `_prepareStepDependencies` 中等待其他任务的生成完成。

### 方案 B：中等修复（解决缓存越界）

在方案 A 基础上，修复缓存越界问题：

1. `_prepareStepDependencies` 中递归生成依赖区块时，如果 `cache.contains(nx, nz)` 返回 false（超出缓存范围），改用 `tryToGetChunkSharedInMem()` 和 `getGeneratingPrimer()` 获取已存在区块，不再通过 `cache.getOrCreateOwned()` 创建新 Primer
2. 如果超出缓存范围的邻居既没有已加载区块也没有正在生成的 Primer，**等待其他任务完成该区块的生成**（类似 MC 的 `acquireGeneration` 机制）

### 方案 C：架构重构（推荐，对齐 MC 1.21.11）

对齐 MC 1.21.11 的 `StaticCache2D` 模式，完整重构区块生成调度：

1. **预分配完整缓存**：创建生成任务时，一次性为 `accumulatedRadius` 范围内的所有位置分配 GenerationChunkHolder，确保无空洞
2. **消除 getGeneratingPrimer 全局查找**：所有区块引用都通过任务局部缓存获取
3. **逐层推进**：每步对缓存内所有区块执行生成，等待前序步骤完成后才推进下一步
4. **等待机制**：对超出缓存范围的依赖，通过 GenerationChunkHolder 的 CompletableFuture 等待其他任务完成

### 方案选择建议

| 方案 | 工作量 | 风险 | 彻底性 |
|------|--------|------|--------|
| A: 移除 getGeneratingPrimer fallback | 小 | 中（可能引入更多 missing chunk） | 低 |
| B: A + 修复缓存越界 | 中 | 中 | 中 |
| C: 对齐 MC StaticCache2D | 大 | 低（原版验证） | 高 |

**建议**：如果需要快速止血，先实施方案 A（移除 fallback），并增加等待其他任务完成的逻辑。长期应实施方案 C。

---

## 8. 累积依赖半径参考表

手动计算验证的 Generation Pyramid 各步骤 accumulatedDependencies：

| 步骤 | directDeps (半径→状态) | accumulatedDeps (半径→状态) | accumulatedRadius |
|------|----------------------|---------------------------|-------------------|
| EMPTY | [] | [] | 0 |
| STRUCTURE_STARTS | [0:EMPTY] | [0:EMPTY] | 0 |
| STRUCTURE_REFERENCES | [0-8:SS] | [0-8:SS] | 8 |
| BIOMES | [0:SR, 1-8:SS] | [0:SR, 1-8:SS] | 8 |
| NOISE | [0-1:BIO, 2-8:SS] | [0:BIO, 1:BIO, 2-9:SS] | 9 |
| SURFACE | [0:NOI, 1:BIO, 2-8:SS] | [0:NOI, 1:BIO, 2-9:SS] | 9 |
| CARVERS | [0:SUR, 1-8:SS] | [0:SUR, 1:BIO, 2-9:SS] | 9 |
| FEATURES | [0-1:CAR, 2-8:SS] | [0:CAR, 1:CAR, 2:BIO, 3-10:SS] | 10 |
| INITIALIZE_LIGHT | [0:FEAT] | [0:FEAT, 1:CAR, 2:BIO, 3-10:SS] | 10 |
| LIGHT | [0-1:IL] | [0:IL, 1:IL, 2:CAR, 3:BIO, 4-11:SS] | 11 |
| SPAWN | [0:LIG, 1:BIO] | [0:LIG, 1:IL, 2:CAR, 3:BIO, 4-11:SS] | 11 |
| FULL | [0:SPAWN] | [0:SPAWN, 1:IL, 2:CAR, 3:BIO, 4-11:SS] | 11 |

缩写：SS=STRUCTURE_STARTS, SR=STRUCTURE_REFERENCES, BIO=BIOMES, NOI=NOISE, SUR=SURFACE, CAR=CARVERS, FEAT=FEATURES, IL=INITIALIZE_LIGHT, LIG=LIGHT, SPAWN=SPAWN

---

## 9. 复现测试

已有测试文件 `tests/common/world/gen/chunk/WorldGenRegionAccessTest.cpp`，包含 13 个测试用例：

| 测试名 | 验证内容 |
|--------|----------|
| `ChunkStep_StructureReferences_DirectDeps` | STRUCTURE_REFERENCES 半径 8 全部要求 SS |
| `ChunkStep_Features_DirectDeps` | FEATURES 半径 0-1=CARVERS, 2-8=SS |
| `MissingChunkInAccessWindow_StructureReferences_NullAtDistance5` | 复现 distance=5 nullptr |
| `MissingChunkInAccessWindow_StructureReferences_MultipleNulls` | 多位置 nullptr |
| `ChunkStatusBelowRequest_Features_CarversNotReached` | 复现 actualStatus=BIOMES < CARVERS |
| `ChunkStatusBelowRequest_Noise_BiomesNotReached` | 复现 actualStatus=SR < BIOMES |
| `CorrectAccess_StructureReferences_AllChunksPresent` | 正确场景验证 |
| `CorrectAccess_Features_AllChunksCorrectStatus` | 正确场景验证 |
| `InvalidAccess_BeyondDependencyRange` | 超出依赖范围检测 |
| `StructureReferencesFullScan_AllChunksPresent` | 完整 17×17 扫描 |
| `StructureReferencesFullScan_WithNullsAtBoundary` | 边界 nullptr |
| `ReproduceLog_CenterNeg7Neg2_Distance5Missing` | 精确复现日志第一组 |
| `ReproduceLog_CenterNeg7Neg2_Distance7Missing` | 精确复现日志第二组 |

这些测试构造了手动创建的 WorldGenRegion，不涉及 ServerChunkManager 的生成管线，只验证 WorldGenRegion 的校验逻辑。要测试完整的集成 Bug，需要在 ServerChunkManager 级别编写集成测试。

---

## 10. 容易踩的坑

1. **断言被临时禁用**：当前 `MC_ASSERT_RELEASE_MSG` 被注释或禁用，所以运行时错误只打日志不崩溃。恢复断言后这两个 Bug 会导致程序终止。

2. **GenerationChunkCache 越界写入**：`getOrCreateOwned()` 的 `_inBounds` 断言被禁用后，越界坐标会写入 `m_entries` 和 `m_ownedEntries` 数组之外的内存，这是严重的内存安全问题。

3. **`getGeneratingPrimer()` 线程安全**：该方法通过 `std::mutex` 保护，但只保护了 map 的访问，不保护 Primer 的状态。返回 Primer 后，另一个线程可能正在修改其状态。

4. **缓存半径计算**：缓存半径使用 `targetStep.accumulatedRadius()`（入口函数中），而 WorldGenRegion 窗口半径使用 `step.accumulatedRadius()`（每步的）。对于 FULL 目标，入口缓存半径 = 11，但每步的窗口半径不同（STRUCTURE_REFERENCES = 8, NOISE = 9, FEATURES = 10 等）。缓存必须覆盖最大窗口半径。

5. **accumulatedDependencies 的级联效应**：每个步骤的 accumulatedRadius 可能比 directRadius 大得多。例如 LIGHT 的 directRadius = 1（只需半径 1 的 INITIALIZE_LIGHT），但 accumulatedRadius = 11（因为间接依赖传播）。

6. **递归生成时的缓存一致性**：同一个区块可能被多次递归生成（不同的中心区块从不同的缓存访问它），如果状态判断不一致（`hasCompletedStatus` 与 `getChunkStatus` 的差异），可能导致重复生成或跳过生成。
