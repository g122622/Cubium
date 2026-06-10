# Jigsaw 拼图结构组装系统

## 概述

`src/common/world/gen/jigsaw` 目录实现了 Minecraft 1.21 风格的 Jigsaw 拼图结构组装系统，用于动态生成复杂结构（如村庄、掠夺者前哨站、堡垒遗迹）。通过 BFS 算法从模板池中选择并连接拼图块，逐步构建完整结构。

## 目录结构

```
jigsaw/
├── JigsawJunction.hpp           # 连接点交叉数据结构，记录地形适配信息
├── JigsawOrientation.hpp        # Jigsaw 方块方向枚举（12 种方向组合）及工具函数
├── JigsawPiece.hpp              # 拼图块基类和实现（Empty/Single/LegacySingle/List/Feature）
├── JigsawPiece.cpp              # 拼图块实现
├── JigsawPattern.hpp            # 模板池和模板池注册表
├── JigsawPattern.cpp            # 模板池实现
├── JigsawManager.hpp            # 拼图组装管理器（核心算法）
├── JigsawManager.cpp            # 拼图组装实现
├── ProcessorListRegistry.hpp    # 处理器列表注册表（按 ResourceLocation 查找 StructureProcessorList）
├── ProcessorListRegistry.cpp    # 处理器列表注册表实现
├── ProcessorListLoader.hpp      # 处理器列表 JSON 加载器（数据包）
├── ProcessorListLoader.cpp      # 从数据包加载处理器列表
├── PoolAliasBinding.hpp         # 池别名绑定基类和实现（Random/RandomGroup）
├── PoolAliasBinding.cpp         # 池别名绑定实现
├── TemplatePoolLoader.hpp       # 模板池 JSON 加载器
└── TemplatePoolLoader.cpp       # 从数据包加载模板池
```

## 内部模块关系

```
JigsawPiece ──包含──> JigsawJoint ──使用──> JigsawOrientation
    │
    └──被包含──> JigsawPattern ──管理──> JigsawPatternRegistry
                      │
                      └──被查询──> JigsawManager ──使用──> PoolAliasBinding
                                       │
                                       ├──使用──> ProcessorListRegistry
                                       │               └──查找──> StructureProcessorList
                                       │
                                       └──使用──> TemplatePoolLoader
                                                       └──解析──> processors 字段
```

- **JigsawPiece**：拼图块类型系统，包含连接点信息（JigsawJoint）
- **JigsawOrientation**：Jigsaw 方块的 12 种方向组合及旋转/镜像变换
- **JigsawPattern**：模板池，管理拼图块的权重随机选择
- **JigsawManager**：核心组装算法，BFS 递归扩展；放置时组合处理器链
- **ProcessorListRegistry**：按 ResourceLocation 注册和查找 StructureProcessorList
- **PoolAliasBinding**：池别名绑定，实现结构生成时的池随机化（如试炼密室刷怪笼类型）
- **TemplatePoolLoader**：从数据包 JSON 加载模板池，支持所有原版元素类型
- **ProcessorListLoader**：从数据包 JSON 加载处理器列表，支持所有原版处理器类型

## 拼图块类型

| 类型 | 类 | 说明 |
|------|-----|------|
| `single_pool_element` | `SingleJigsawPiece` | 标准模板放置，忽略结构方块 |
| `legacy_single_pool_element` | `LegacySingleJigsawPiece` | 旧版模板放置，忽略结构方块+空气 |
| `list_pool_element` | `ListJigsawPiece` | 子元素列表，随机选一个放置 |
| `feature_pool_element` | `FeatureJigsawPiece` | 配置化地物放置（树木、装饰等） |
| `empty_pool_element` | `EmptyJigsawPiece` | 空占位，不放置任何内容 |

### Legacy vs Standard

`LegacySingleJigsawPiece` 继承 `SingleJigsawPiece`，唯一行为差异在放置时的方块忽略列表：
- **Standard** (`SingleJigsawPiece`)：忽略结构方块（`BlockIgnoreProcessor.STRUCTURE_BLOCK`）
- **Legacy** (`LegacySingleJigsawPiece`)：忽略结构方块+空气（`BlockIgnoreProcessor.STRUCTURE_AND_AIR`）

原版 datapack 中村庄和前哨站大量使用 `legacy_single_pool_element`，因为旧版结构模板中空气方块是显式放置的，需要忽略以避免覆盖已有地形。

### Processors

`SingleJigsawPiece` 和 `LegacySingleJigsawPiece` 可携带 `m_processorListId`（类型为 `std::optional<ResourceLocation>`），指向 `ProcessorListRegistry` 中注册的处理器列表。在放置时，处理器链按 MC 源码顺序组合：

1. `BlockIgnoreStructureProcessor`（legacy=STRUCTURE_AND_AIR, standard=STRUCTURE_BLOCK）
2. `JigsawReplacementStructureProcessor`
3. Piece 自带处理器（从 `ProcessorListRegistry` 查找并克隆）
4. `GravityStructureProcessor`（仅 `terrain_matching` 投影）

## 上下游外部依赖

### 上游依赖（本目录依赖）

| 依赖项 | 来源 | 用途 |
|--------|------|------|
| BlockPos | world/block/ | 位置坐标 |
| StructureBoundingBox | world/gen/structure/ | 边界框计算 |
| TemplateManager | world/gen/feature/template/ | 模板加载 |
| StructureProcessorList | world/gen/feature/template/ | 处理器列表（通过 ProcessorListRegistry 查找） |
| Random | util/math/random/ | 随机数 |
| ResourceLocation | resource/ | 资源定位 |
| IResourcePack | resource/ | 资源包 |
| DataPackList | resource/ | 数据包列表 |
| Direction | util/ | 方向枚举 |
| Result | core/ | 错误处理 |

### 下游依赖（谁依赖本目录）

| 模块 | 用途 |
|------|------|
| JigsawStructure | 使用 JigsawManager 组装结构 |
| StructureManager | 注册模板池、初始化 Jigsaw 系统 |
| ProcessorLists | 启动时向 ProcessorListRegistry 注册内置处理器列表 |
| ChunkGenerator | 触发结构生成 |

## 容易踩的坑

### 1. 模板池未注册导致崩溃

调用 `assemble()` 前必须检查模板池是否存在且非空：

```cpp
const JigsawPattern* startPool = registry.getPattern(startPoolLocation);
if (!startPool || startPool->isEmpty()) {
    return {};
}
```

### 2. 连接点名称不匹配

连接点名称必须完全匹配。源连接点的 `targetName` 必须等于目标连接点的 `sourceName`。

### 3. 深度限制设置

推荐值：村庄 6-8，要塞 7-10，简单结构 4。过小结构不完整，过大过度扩展。

### 4. 权重系统实现原理

权重通过复制实现：`pool->addPiece(piece, 3)` 添加 3 个副本。`getNumberOfPieces()` 返回权重之和而非拼图块种类数。

### 5. EmptyJigsawPiece 是单例

`EmptyJigsawPiece::instance()` 返回单例引用。不要对单例调用 `clone()`，它会返回 `nullptr`。

### 6. 坐标变换

不要手动计算变换后的连接点位置，使用 `JigsawManager::getTransformedJoints()` 获取变换后的连接点。

### 7. JigsawOrientation 连接规则

两个 Jigsaw 方块连接条件：
- `facing` 方向必须相反（面对面）
- `rollable=true` 时只检查 facing 相反
- `rollable=false` 时 rotation 必须相同

### 8. PoolAliasBinding 用途

池别名绑定用于结构生成时的随机化，典型场景是试炼密室的刷怪笼类型随机化。模板池引用虚拟池名，组装时通过别名绑定随机替换为实际池。

### 9. TemplatePoolLoader 加载路径

JSON 文件路径：`data/<namespace>/worldgen/template_pool/<path>.json`

### 10. 资源包设置

组装前必须调用 `JigsawManager::setResourcePack(&resourcePack)`，否则无法加载模板文件。

### 11. ProcessorListRegistry 初始化

`ProcessorLists::initialize()` 会向 `ProcessorListRegistry` 注册所有内置处理器列表。必须在 `JigsawManager::placePieceRecursive()` 之前完成注册，否则 piece 自带处理器会找不到。

### 12. 处理器链顺序

放置时处理器链的顺序必须严格遵循 MC 源码：BlockIgnore → JigsawReplacement → Piece processors → Gravity。顺序错误会导致村庄道路、僵尸化效果、前哨站腐坏等异常。

### 13. LegacySingleJigsawPiece 的空气忽略

`LegacySingleJigsawPiece` 在放置时会忽略空气方块（不覆盖已有方块），这对村庄旧版模板至关重要。如果误用 `SingleJigsawPiece`，空气方块会覆盖已有地形。
