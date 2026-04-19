# BiomeGenerationSettings 缺失补全清单

本清单用于记录 `src/common/world/biome/BiomeGenerationSettings.cpp` 及其上下游仍未完全对齐 Minecraft Java 1.16.5 的项目项，便于逐项补齐与回归验证。

## 1. 结构性缺失

- [ ] 仍是“按生物群系手写 feature id 列表”的简化模型，没有完整承载原版 `BiomeGenerationSettings` 的 surface builder / carver / structure starts / feature lists 结构。
- [ ] `DecorationStage` 仍只有粗粒度阶段，缺少与原版 `GenerationStage.Decoration` 完整一致的内容分布映射。
- [ ] 当前各个 `createXxx()` 仍是预设模板，不是原版那种基于可复用默认块组合出来的 builder 体系。

## 2. 海洋生成缺口

- [ ] 海带、海草、海泡菜、珊瑚、蓝冰、海洋装饰之间的组合还不够细，仍有一部分分支只是近似实现，缺少与原版一致的完整温度分支。
- [x] 已补齐 `kelp_cold` / `kelp_warm`、`seagrass_cold` / `seagrass_deep_cold` / `seagrass_normal` / `seagrass_river` / `seagrass_deep` / `seagrass_swamp` / `seagrass_warm` / `seagrass_deep_warm` 的注册与编排。
- [ ] 冻洋与深冻洋仍只做了近似实现，原版的冰山/蓝冰组合还没有完全落到现有 feature 表里。
- [ ] `OceanDecorationFeature` 仍属于项目自定义扩展，不是原版 1.16.5 的直接对应项，是否保留需要和原版复刻目标统一。

## 3. 其他 biome TODO

- [ ] 下界：`createNether()` 仍缺少下界要塞、堡垒遗迹等结构层级。
- [ ] 下界：`createSoulSandValley()` 仍缺少下界化石等结构补齐。
- [ ] 末地：`createSmallEndIslands()` 仍是空壳。
- [ ] 末地：`createEndHighlands()` 仍缺少末地城与紫颂树等内容。

## 4. 测试缺口

- [ ] 需要为每个海洋分支补齐更精确的 feature id 断言，而不是只验证“有海草/有海带/有海洋装饰”。
- [ ] 需要补充特征注册顺序与 `FeatureIds.hpp` 偏移量的回归测试。
- [ ] 需要为原版海洋分支补齐“温度 -> feature 组合”断言，防止后续再退化成粗粒度模板。

## 5. 当前优先级

1. 先补海洋 feature 的原版变体注册。
2. 再把 `BiomeGenerationSettings.cpp` 的海洋分支改成对应组合。
3. 最后同步测试与文档，确保注册顺序和偏移量不再漂移。