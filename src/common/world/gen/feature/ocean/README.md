# 海洋特征模块 (Ocean Features)

该目录实现海洋生态相关的世界生成特征，负责在海底与不同温区海洋中补充海带、海草、海泡菜、活/失活珊瑚、蓝冰，以及海洋遗迹风格装饰物。

## 目录结构树

```text
ocean/
├── README.md                       # 本文档
├── KelpFeature.hpp/cpp             # 海带特征（冷/暖海带变体）
├── SeagrassFeature.hpp/cpp         # 海草特征（普通/高海草，多温区变体）
├── SeaPickleFeature.hpp/cpp        # 海泡菜特征（堆叠1-4个）
├── CoralFeature.hpp/cpp            # 珊瑚特征（树形/蘑菇形/爪形，活体/失活）
└── BlueIceFeature.hpp/cpp          # 蓝冰簇特征（冷水/冻洋）
```

## 内部模块关系

所有特征类均采用 `XxxFeature` + `ConfiguredXxxFeature` 的两层结构：
- `XxxFeature` - 核心放置逻辑
- `ConfiguredXxxFeature` - 实现 `ConfiguredFeatureBase` 接口，由数据包 JSON 注册到 `ConfiguredFeatureRegistry`（按 `ResourceLocation` 索引）

各特征按职责独立，除 `CoralFeature` 内部分支调用 `CoralTreeFeature`/`CoralMushroomFeature`/`CoralClawFeature` 外，其余特征互不依赖。

## 上下游外部依赖关系

**依赖方（上游）：**
- `Feature.hpp` / `ConfiguredFeature.hpp` - 特征基类与接口
- `world/chunk/ChunkPrimer.hpp` - 区块数据
- `world/gen/chunk/IChunkGenerator.hpp` - 区块生成器接口
- `world/block/VanillaBlocks.hpp` - 方块状态获取
- `world/block/blocks/coral/CoralBlock.hpp` - 珊瑚颜色枚举（仅 CoralFeature）
- `util/Direction.hpp` - 方向枚举
- `util/math/random/Random.hpp` - 随机数生成

**被依赖方（下游）：**
- `ConfiguredFeatureRegistry` - 数据驱动注册所有 `ConfiguredXxxFeature`
- `BiomeGenerationSettings` - 生物群系生成设置以 `ResourceLocation` 引用 placed_feature
- placed_feature id（如 `minecraft:kelp`、`minecraft:seagrass_simple`、`minecraft:sea_pickle`、`minecraft:coral_*`、`minecraft:blue_ice`）

## 容易踩的坑

- **起始坐标误用**：传入的起始坐标通常是区块原点（y=0），不能直接用其 y 做向下扫描起点，需通过 `OceanFloor` 高度图回推真实海底高度。
- **海泡菜放置条件**：要求当前位置在水中，且下方有可支撑方块。两个条件缺一不可。
- **高海草状态完整性**：高海草需要同时设置上下半状态（`HALF` 属性），缺失任一状态会退化为普通海草或直接失败。
- **温区变体同步**：海带与海草的冷/暖/深海/河流/沼泽等变体由数据包 `configured_feature`/`placed_feature` JSON 分别定义，生物群系通过 `features` 数组按 `ResourceLocation` 引用对应变体。
- **珊瑚墙扇方向**：使用 `FACING` 指向支撑面方向，方向传反会导致后续掉落。
- **蓝冰放置条件**：需要在海平面以下、邻接打包冰条件下扩散蓝冰，不是任意水下位置都能生成。
