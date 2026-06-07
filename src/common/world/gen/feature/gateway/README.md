# 末地折跃门特征模块

## 目录结构树

```text
gateway/
├── EndGatewayFeature.hpp   # 末地折跃门特征定义（配置、特征类、工厂）
├── EndGatewayFeature.cpp   # 末地折跃门特征实现
└── README.md               # 本文档
```

## 内部模块关系

- `EndGatewayFeature` - 核心特征类，负责折跃门结构放置与目标点计算
- `EndGatewayFeatureConfig` - 配置对象，包含 `isExit` 标志和可选 `exactPosition`
- `ConfiguredEndGatewayFeature` - 配置化包装，统一接入 FeatureRegistry
- `EndGatewayFeatures` - 工厂类，提供 `end_gateway` 和 `end_gateway_exit` 两种预设

## 上下游外部依赖关系

**上游依赖（本模块依赖的模块）：**
- `ConfiguredFeature` - 配置化特征基类
- `DecorationStage` - 装饰阶段枚举
- `FeatureIds` - 特征 ID 常量（`EndSurfaceFeatureIds::EndGateway`、`EndSurfaceFeatureIds::EndGatewayExit`）
- `VanillaBlocks` - 方块状态（基岩、末地折跃门方块、空气）
- `ChunkPrimer` - 区块数据
- `IChunkGenerator` - 区块生成器接口
- `math::Random` - 随机数生成器
- `WorldGenRegion` - 世界生成区域

**下游依赖（依赖本模块的模块）：**
- `ConfiguredFeature.cpp` - 在 `FeatureRegistry::initialize()` 中调用 `EndGatewayFeatures::initialize()` 并注册到 `SurfaceStructures` 阶段
- 生物群系生成设置 - 通过 `FeatureIds` 引用折跃门特征槽位

## 容易踩的坑

- `getAllFeaturesAndClear()` 调用后静态容器会清空，重复使用前必须重新 `initialize()`
- 若放置起始 Y 直接使用区块原点（Y=0），`_canPlaceAt()` 会始终失败；必须基于高度图选择地表候选点
- `EndGatewayFeatureConfig::isExit` 当前主要是配置位，完整战斗流程触发规则尚需上层系统补齐
- `calculateTeleportTarget()` 是静态算法近似实现，具体落点校正（安全落地/岛屿检测）仍需后续细化
- 折跃门使用 `random.nextInt(256) != 0` 控制稀疏生成，避免每个区块都尝试放置
