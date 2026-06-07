# Jigsaw 拼图结构组装系统

## 概述

`src/common/world/gen/jigsaw` 目录实现了 Minecraft 1.16.5 风格的 Jigsaw 拼图结构组装系统，用于动态生成复杂结构（如村庄、要塞）。通过 BFS 算法从模板池中选择并连接拼图块，逐步构建完整结构。

## 目录结构

```
jigsaw/
├── JigsawJunction.hpp           # 连接点交叉数据结构，记录地形适配信息
├── JigsawOrientation.hpp        # Jigsaw 方块方向枚举（12 种方向组合）及工具函数
├── JigsawPiece.hpp              # 拼图块基类和实现（Empty/Single/List）
├── JigsawPiece.cpp              # 拼图块实现
├── JigsawPattern.hpp            # 模板池和模板池注册表
├── JigsawPattern.cpp            # 模板池实现
├── JigsawManager.hpp            # 拼图组装管理器（核心算法）
├── JigsawManager.cpp            # 拼图组装实现
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
                                       └──使用──> TemplatePoolLoader
```

- **JigsawPiece**：拼图块类型系统，包含连接点信息（JigsawJoint）
- **JigsawOrientation**：Jigsaw 方块的 12 种方向组合及旋转/镜像变换
- **JigsawPattern**：模板池，管理拼图块的权重随机选择
- **JigsawManager**：核心组装算法，BFS 递归扩展
- **PoolAliasBinding**：池别名绑定，实现结构生成时的池随机化（如试炼密室刷怪笼类型）
- **TemplatePoolLoader**：从数据包 JSON 加载模板池

## 上下游外部依赖

### 上游依赖（本目录依赖）

| 依赖项 | 来源 | 用途 |
|--------|------|------|
| BlockPos | world/block/ | 位置坐标 |
| StructureBoundingBox | world/gen/structure/ | 边界框计算 |
| TemplateManager | world/gen/feature/template/ | 模板加载 |
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
