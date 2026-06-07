# Client World 模块

该模块负责客户端世界状态维护，包括区块数据接入、网格任务调度、天气时间同步、实体管理和基础查询接口。

## 目录结构树

```text
src/client/world/
├── ClientWorld.hpp/cpp                 # 客户端世界核心管理器（区块同步、网格调度、天气时间）
├── ClientWeather.hpp                   # 天气插值状态（雨强、雷强、闪电闪烁）
├── ClientMapDataCache.hpp/cpp          # 地图数据缓存
├── color/                              # 生物群系颜色解析与混合
│   ├── BiomeColors.hpp/cpp             # 颜色解析器实现（草、树叶、水）
│   ├── ColorResolver.hpp               # 颜色解析器抽象接口
│   ├── blend/                          # 颜色混合缓存与访问器
│   │   ├── BiomeColorBlender.hpp/cpp   # 颜色混合器（核心算法）
│   │   ├── BiomeColorCache.hpp/cpp     # 颜色缓存（按区块缓存）
│   │   ├── ChunkBiomeAccessor.hpp/cpp  # 区块生物群系访问器
│   │   └── blend.hpp                   # 统一头文件
│   └── README.md
├── entity/                             # 客户端实体管理
│   ├── ClientEntity.hpp/cpp            # 客户端实体类（位置插值、动画状态）
│   ├── ClientEntityManager.hpp/cpp     # 客户端实体管理器
│   └── README.md
├── player/                             # 本地玩家相关
│   ├── LocalPlayerIdentity.hpp/cpp     # 本地玩家身份（playerId ↔ entityId 映射）
│   ├── ClientPlayerPredictor.hpp/cpp   # 客户端玩家预测器（移动预测、位置校正）
│   └── README.md
└── README.md
```

## 内部模块关系

```
NetworkClient callbacks
        │
        ▼
ClientWorld ─────────────────────────────────────┐
        │                                         │
        ├── onChunkData/onChunkUnload             ├── 天气/时间同步
        ▼                                         ▼
MeshBuildScheduler ────→ MeshWorkerPool      ClientWeather
        │
        ▼
ChunkMesher ──→ BiomeColorBlender ──→ BiomeColorCache
        │
        ▼
ClientChunk.solidMesh/transparentMesh
        │
        ▼
ChunkRenderer GPU upload

ClientEntityManager ◄──── NetworkClient (实体包)
        │
        ├── 管理所有 ClientEntity
        └── 特殊处理本地玩家（跳过网络位置更新）

ClientPlayerPredictor ◄──── LocalPlayerIdentity
        │
        └── 本地玩家位置预测与校正
```

**核心流程**：
1. 区块包 → `ClientWorld.onChunkData()` → 更新 `ChunkData` → 调度网格重建
2. 网格任务 → `MeshBuildScheduler` → `MeshWorkerPool` → `ChunkMesher` → GPU 上传
3. 实体包 → `ClientEntityManager` → 创建/更新 `ClientEntity` → 渲染层读取插值位置
4. 本地玩家 → `LocalPlayerIdentity` 识别 → `ClientPlayerPredictor` 预测位置

## 上下游外部依赖关系

### 上游依赖（本模块依赖的模块）

```
common/world/chunk/ChunkData.hpp           # 区块数据结构
common/world/biome/BiomeRegistry.hpp       # 生物群系注册表
common/network/sync/ChunkSync.hpp          # 区块同步协议
client/renderer/mesh/MeshBuildScheduler.hpp  # 网格构建调度器
client/renderer/mesh/MeshWorkerPool.hpp      # 网格工作线程池
client/renderer/trident/chunk/ChunkMesher.hpp # 区块网格生成器
```

### 下游依赖（依赖本模块的模块）

```
ClientApplication      # 持有 ClientWorld、ClientEntityManager 实例
NetworkClient          # 网络回调时调用 onChunkData/onChunkUnload 等
ChunkRenderer          # 读取 ClientChunk 的网格数据进行 GPU 上传
EntityRenderer 系列    # 读取 ClientEntity 的插值位置和动画状态渲染实体
```

## 容易踩的坑

### 区块与网格

- **`update()` 必须传完整 `MeshSchedulerViewState`**：不能只传相机位置，否则视锥优先与取消策略失效。
- **维度切换顺序不能乱**：正确顺序是先 `setDimensionId()`，再 `clearChunks()`，这样迟到的旧维度区块包才会被丢弃。
- **区块卸载时要先取消调度任务**：已在 `onChunkUnload()` 中调用 `MeshBuildScheduler::cancelChunk`。
- **不要假设每次 `onChunkData` 都是新建区块**：同一坐标重发时会替换 `ChunkData` 并触发新代际网格任务。
- **`processMeshBuildResults()` 只处理最新任务结果**：旧结果会在调度器层丢弃。
- **光照包不要直接当成"立即重建"事件处理**：`onLightUpdate()` 会先标记 `meshRebuildPending`，如果同一 chunk 的网格任务还在路上，就等当前任务结束后再补提，避免单个 chunk 被光照更新线性打爆。
- **被取消的任务可能留下脏 `activeMeshTaskId`**：`ClientWorld::update()` 会用 `isTaskTracked()` 回收并补提，避免区块长期不出网格。

### 实体与玩家

- **位置更新必须区分"立即设置"和"目标设置"**：
  - `setPosition()` - 立即设置，不插值（用于出生、传送）
  - `setTargetPosition()` - 设置目标位置，触发平滑插值（用于移动）
  - 用错会导致实体瞬移或漂移
- **本地玩家由预测器管理**：本地玩家的位置不应该从网络包直接更新，通过 `isLocalPlayer(entityId)` 判断并跳过。
- **渲染时必须使用插值位置**：不要直接用 `position()`，必须用 `getInterpolatedPosition(partialTick)`。
- **永远不要将 EntityId 强转为 PlayerId**：这是导致相机绑定到错误实体的根本原因。
- **角度环绕处理**：Yaw 角度在 -180 到 180 之间，插值时要选择最短路径（代码已处理，手动修改时需注意）。
- **不能移除本地玩家实体**：`removeEntity()` 对本地玩家返回 false，必须先调用 `clearLocalPlayer()`。

### 颜色混合

- **缓存失效时机**：区块卸载时必须调用 `BiomeColorCache::invalidateChunk()`，否则缓存会持有无效引用。
- **邻居区块缺失**：混合采样跨越区块边界时，邻居区块可能未加载，`ChunkBiomeAccessor::getBiome()` 会返回 `nullptr`，混合算法会自动跳过。
- **Colormap 设置**：`BiomeColorBlender` 需要外部设置 `grassColorMap` 和 `foliageColorMap` 指针（由 ChunkMesher 管理）。

### 其他

- **`ClientWorld` 不是 `IWorld` 实现**：它只提供自己的 xyz 查询接口，调试屏幕和客户端工具代码不要假设这里存在 `BlockPos` overload。
- **固定 Tick 累加器防止螺旋死亡**：`fixedTick()` 有 `MAX_TICKS_PER_FRAME = 5` 限制，如果帧率过低会丢弃部分 tick。
