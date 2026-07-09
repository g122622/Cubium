# Jigsaw 拼图结构组装系统

## 概述

`src/common/world/gen/jigsaw` 目录实现了 Minecraft 1.21 风格的 Jigsaw 拼图结构组装系统，用于动态生成复杂结构（如村庄、掠夺者前哨站、堡垒遗迹、试炼密室）。通过优先级队列算法从模板池中选择并连接拼图块，逐步构建完整结构，并使用 VoxelShape 空间追踪保证结构不重叠、不越界。

## 目录结构

```
jigsaw/
├── JigsawTypes.hpp              # 枚举（JigsawPlacementBehaviour/JigsawJointType）+ JigsawJoint 结构 + MaxDistance 结构
├── JigsawOrientation.hpp        # Jigsaw 方块方向枚举（12 种方向组合）及工具函数
├── JigsawMatcher.hpp            # 连接点匹配工具（名称匹配 + 方向匹配）
├── JigsawJunction.hpp           # 连接点交叉数据结构，记录地形适配信息
├── JigsawPiece.hpp/.cpp         # 拼图块基类（virtual place() 多态分发）
├── EmptyJigsawPiece.hpp/.cpp    # 空拼图块（clone 返回 nullptr）
├── SingleJigsawPiece.hpp/.cpp   # 单模板拼图块 + LegacySingleJigsawPiece（紧密继承族同文件）
├── ListJigsawPiece.hpp/.cpp     # 列表拼图块（递归放置子块）
├── FeatureJigsawPiece.hpp/.cpp  # 地物拼图块（调用 ConfiguredFeature 放置）
├── TemplatePool.hpp/.cpp        # 模板池（原 JigsawPattern，管理拼图块权重随机选择）
├── TemplatePoolRegistry.hpp/.cpp # 模板池注册表（原 JigsawPatternRegistry）
├── TemplatePoolLoader.hpp/.cpp  # 模板池 JSON 加载器（数据包）
├── AssemblyTypes.hpp            # PlacedPiece + PendingJoint 中间数据结构
├── JigsawAssembler.hpp/.cpp     # 组装算法（assemble/tryPlacePiece/优先级队列/PoolAliasBinding/VoxelShape）
├── JigsawPlacer.hpp/.cpp        # 放置器（virtual place() 多态分发 + 回退方块）
├── JigsawTransform.hpp/.cpp     # 坐标/连接点变换、边界框计算
├── SequencedPriorityIterator.hpp # 按 placementPriority 降序的优先级迭代器
├── PoolAliasBinding.hpp/.cpp    # 池别名绑定（Random/RandomGroup/Direct）
├── PoolAliasLookup.hpp          # 池别名一次性预解析查找表
├── ProcessorListRegistry.hpp/.cpp # 处理器列表注册表（按 ResourceLocation 查找）
├── ProcessorListLoader.hpp/.cpp # 处理器列表 JSON 加载器（数据包）
├── JigsawLoaderUtils.hpp        # 加载器共用工具（stripMinecraftPrefix）
└── README.md                    # 本文档
```

## 内部模块关系

```
JigsawPiece ──包含──> JigsawJoint ──使用──> JigsawOrientation
    │                                         └──> JigsawMatcher（连接点匹配）
    ├──被组装──> JigsawAssembler ──使用──> TemplatePool / TemplatePoolRegistry
    │                │              ├──使用──> PoolAliasBinding / PoolAliasLookup
    │                │              ├──使用──> SequencedPriorityIterator
    │                │              ├──使用──> VoxelShape（空间追踪）
    │                │              └──使用──> JigsawTransform（坐标变换）
    │                └──被放置──> JigsawPlacer ──多态分发──> 各 JigsawPiece 子类
    │                                   └──使用──> ProcessorListRegistry（处理器列表）
    └──被加载──> TemplatePoolLoader ──使用──> JigsawLoaderUtils
                                    └──使用──> ProcessorListLoader（内联处理器列表）
```

### 关键模块

- **JigsawPiece**：拼图块基类，`virtual place()` 多态分发到各子类（Single/List/Feature/Empty）。子类在 `place()` 中构造处理器链并调用 `Template::place()`。
- **JigsawAssembler**：核心组装算法。使用 `SequencedPriorityIterator`（按 placementPriority 降序）替代 FIFO 队列；通过 VoxelShape 空间追踪（`freeShape`）保证结构不重叠不越界；集成 `PoolAliasBinding` 实现池别名随机化。
- **JigsawPlacer**：放置器，遍历 `PlacedPiece` 调用 `piece->place()`（多态分发）。提供 `placePiece`（单块）和 `placePieces`（批量）入口。
- **JigsawTransform**：坐标/连接点变换、边界框计算（从原 JigsawManager 提取）。
- **TemplatePool**：模板池（原 JigsawPattern），管理拼图块的权重随机选择。
- **TemplatePoolRegistry**：模板池注册表（原 JigsawPatternRegistry，已分离到独立文件）。
- **PoolAliasBinding/PoolAliasLookup**：池别名绑定，实现结构生成时的池随机化（如试炼密室刷怪笼类型）。`PoolAliasLookup::create` 一次性预解析为不可变查找表。
- **SequencedPriorityIterator**：按 placementPriority 降序的优先级迭代器（对应 MC 1.21 的 SequencedPriorityIterator）。
- **ProcessorListRegistry/Loader**：处理器列表注册表与 JSON 加载器，支持所有原版处理器类型。
- **JigsawLoaderUtils**：加载器共用工具（`stripMinecraftPrefix` 消除 5+ 处重复的 `minecraft:` 前缀剥离样板）。

## 拼图块类型

| 类型 | 类 | 说明 |
|------|-----|------|
| `single_pool_element` | `SingleJigsawPiece` | 标准模板放置，忽略结构方块 |
| `legacy_single_pool_element` | `LegacySingleJigsawPiece` | 旧版模板放置，忽略结构方块+空气 |
| `list_pool_element` | `ListJigsawPiece` | 子元素列表，递归放置所有子块 |
| `feature_pool_element` | `FeatureJigsawPiece` | 配置化地物放置（树木、仙人掌、干草堆等） |
| `empty_pool_element` | `EmptyJigsawPiece` | 空占位，不放置任何内容 |

### Legacy vs Standard

`LegacySingleJigsawPiece` 继承 `SingleJigsawPiece`，唯一行为差异在放置时的方块忽略列表：
- **Standard** (`SingleJigsawPiece`)：忽略结构方块（`STRUCTURE_BLOCK`）
- **Legacy** (`LegacySingleJigsawPiece`)：忽略结构方块+空气（`STRUCTURE_AND_AIR`）

原版 datapack 中村庄和前哨站大量使用 `legacy_single_pool_element`，因为旧版结构模板中空气方块是显式放置的，需要忽略以避免覆盖已有地形。

### Processors

`SingleJigsawPiece` 和 `LegacySingleJigsawPiece` 可携带 `m_processorListId`（类型为 `std::optional<ResourceLocation>`），指向 `ProcessorListRegistry` 中注册的处理器列表。放置时处理器链按 MC 源码顺序组合：

1. `BlockIgnoreStructureProcessor`（legacy=STRUCTURE_AND_AIR，standard=STRUCTURE_BLOCK）
2. `JigsawReplacementStructureProcessor`（替换 jigsaw 方块为 final_state）
3. Piece 自带处理器（从 `ProcessorListRegistry` 查找并克隆；也支持内联处理器列表）
4. `GravityStructureProcessor`（仅 `terrain_matching` 投影，offset 固定 -1，对应 `WORLD_SURFACE_WG`）

### FeatureJigsawPiece::place()

`FeatureJigsawPiece` 不应用结构处理器，直接调用 `ConfiguredFeatureBase::place(region, chunk, generator, rng, pos)` 放置配置化地物。`m_featureId` 是 configured_feature 的 `ResourceLocation` 字符串（如 `minecraft:pale_oak`），通过 `ConfiguredFeatureRegistry::instance().get(ResourceLocation::parse(m_featureId))` 按 id 解析为 `const ConfiguredFeatureBase*`（未找到则 warn 并跳过放置）。`IWorldWriter` 向下转型为 `WorldGenRegion`（`dynamic_cast`），`chunk`/`generator` 通过 `place()` 链路透传。

## 上下游外部依赖

### 上游依赖（本目录依赖）

| 依赖项 | 来源 | 用途 |
|--------|------|------|
| BlockPos | world/block/ | 位置坐标 |
| StructureBoundingBox | world/gen/structure/ | 边界框计算 |
| TemplateManager | world/gen/feature/template/ | 模板加载 |
| StructureProcessorList | world/gen/feature/template/ | 处理器列表（通过 ProcessorListRegistry 查找） |
| VoxelShape/Shapes | world/physics/shape/ | 空间追踪（freeShape 减法/碰撞检测） |
| ConfiguredFeatureBase/ConfiguredFeatureRegistry | world/gen/feature/ | FeatureJigsawPiece 地物放置（按 ResourceLocation 解析） |
| IChunkGenerator/ChunkPrimer | world/gen/chunk/ | TerrainMatching 高度计算 + FeatureJigsawPiece 放置 |
| Random | util/math/random/ | 随机数 |
| ResourceLocation | resource/ | 资源定位 |
| IResourcePack | resource/ | 资源包 |
| DataPackRepository | resource/ | 数据包列表 |
| Direction | util/ | 方向枚举 |
| Result | core/ | 错误处理 |

### 下游依赖（谁依赖本目录）

| 模块 | 用途 |
|------|------|
| JigsawStructure + 4 结构（Village/Bastion/Fortress/TrialChambers） | 使用 JigsawAssembler 组装结构，JigsawPlacer 放置 |
| MinecraftServer | 初始化 Jigsaw 系统（TemplateManager/TemplatePoolRegistry） |
| ProcessorLists | 启动时向 ProcessorListRegistry 注册内置处理器列表 |
| ChunkGenerator（Noise/Flat） | 通过 Structure::placeInChunk 触发结构生成 |
| SingleJigsawPiece | 放置时查找 ProcessorListRegistry 处理器列表 |

## 容易踩的坑

### 1. 模板池未注册导致崩溃

调用 `assemble()` 前必须检查模板池是否存在且非空：

```cpp
const TemplatePool* startPool = registry.getPool(startPoolLocation);
if (!startPool || startPool->isEmpty()) {
    return {};
}
```

### 2. 连接点名称不匹配

连接点名称必须完全匹配。源连接点的 `targetName` 必须等于目标连接点的 `sourceName`。使用 `JigsawMatcher::canMatch` / `canMatchByName` 检查。

### 3. 深度限制设置

推荐值：村庄 6-8，要塞 7-10，试炼密室 20，简单结构 4。过小结构不完整，过大过度扩展。组装由 `maxDepth` + VoxelShape 空间追踪共同限制（无 maxPieces 硬编码上限）。

### 4. 权重系统实现原理

权重通过复制实现：`pool->addPiece(piece, 3)` 添加 3 个副本。`getTotalWeight()` 返回权重之和而非拼图块种类数。

### 5. EmptyJigsawPiece 是单例

`EmptyJigsawPiece::instance()` 返回单例引用。不要对单例调用 `clone()`，它会返回 `nullptr`。

### 6. 坐标变换

不要手动计算变换后的连接点位置，使用 `JigsawTransform::getTransformedJoints()` 获取变换后的连接点。

### 7. JigsawOrientation 方向工具

`JigsawOrientations` 提供 `fromFacingAndRotation`/`getFacing`/`getRotation`/`rotate`/`mirror`/`opposite`/`fromName`/`toString` 等工具函数。连接匹配逻辑在 `JigsawMatcher::canConnectOrientation` 中（facing 相反 + rotation 规则）。

### 8. PoolAliasBinding 用途

池别名绑定用于结构生成时的随机化，典型场景是试炼密室的刷怪笼类型随机化。模板池引用虚拟池名，组装时通过 `PoolAliasLookup`（一次性预解析）随机替换为实际池。

### 9. TemplatePoolLoader 加载路径

JSON 文件路径：`data/<namespace>/worldgen/template_pool/<path>.json`。支持 `single_pool_element`/`legacy_single_pool_element`/`list_pool_element`/`feature_pool_element`/`empty_pool_element` 五种元素类型，以及内联处理器列表（`processors` 字段为对象或数组时通过 `ProcessorListLoader::parseInlineProcessorList` 解析并注册为合成资源位置）。

### 10. 资源包设置

组装前必须调用 `JigsawAssembler::setResourcePack(&resourcePack)` 设置模板管理器的资源包，否则无法加载模板文件。`JigsawAssembler::getTemplateManager()` 提供静态访问点。

### 11. ProcessorListRegistry 初始化

处理器列表有两个注册来源：
1. **硬编码注册**：`ProcessorLists::initialize()` 在启动时注册内置处理器列表（村庄、堡垒遗迹等）
2. **数据包加载**：`ProcessorListLoader::loadFromDataPackRepository()` 在 `MinecraftServer::initializeRegistries()` 中被调用，加载 `data/<namespace>/worldgen/processor_list/*.json`

数据包加载会无条件覆盖同名的硬编码注册（`ProcessorListRegistry::registerList()` 通过 `m_lists[id] = ...` 写入，后注册者覆盖先注册者）。因此同一 ID 的处理器列表最终以数据包版本为准。

必须在 `JigsawPlacer::placePieces()` 之前完成注册，否则 piece 自带处理器会找不到。

### 12. 处理器链顺序

放置时处理器链的顺序必须严格遵循 MC 源码：BlockIgnore → JigsawReplacement → Piece processors → Gravity。顺序错误会导致村庄道路、僵尸化效果、前哨站腐坏等异常。

### 13. LegacySingleJigsawPiece 的空气忽略

`LegacySingleJigsawPiece` 在放置时会忽略空气方块（不覆盖已有方块），这对村庄旧版模板至关重要。如果误用 `SingleJigsawPiece`，空气方块会覆盖已有地形。

### 14. VoxelShape 空间追踪（freeShape）

`JigsawAssembler` 使用两层级 `freeShape`（对应 MC 1.21 的 `mutableobject`/`mutableobject1`）追踪可放置空间：
- 连接点在父块 BoundingBox 内时使用局部 `freeShape`
- 否则使用全局 `freeShape`
- 放置前用 `Shapes::joinIsNotEmpty(freeShape, deflatedAABB, OnlySecond)` 检查碰撞（deflate 0.25f）
- 放置后用 `Shapes::joinUnoptimized(freeShape, newAABB, OnlyFirst)` 更新

兄弟拼图块共享同一个 `shared_ptr<VoxelShape>` 持有者，放置时通过 `*holder = ...` 原地更新，使兄弟块能看到彼此的更新。

### 15. TerrainMatching 高度计算

`tryPlacePiece` 根据 RIGID/TERRAIN_MATCHING 投影类型计算新块基线 Y：
- parent==RIGID && child==RIGID：`newPieceBaseY = parentBoundingBox.minY + deltaY`
- 否则：`newPieceBaseY = generator.getHeight(x, z, WorldSurfaceWG) - childJunctionLocalY`

`GravityStructureProcessor` offset 固定为 -1（对应 `WORLD_SURFACE_WG` 高度图），不依赖 groundLevelDelta。groundLevelDelta 仅用于 JigsawJunction 的 deltaY 计算。
