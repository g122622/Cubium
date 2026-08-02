# 花园觉醒方块 (Garden Awakening Blocks)

1.21.4+ 花园觉醒更新特有的方块实现。

## 目录结构

```
garden/
├── BushPlantBlock.hpp       # 灌木方块 minecraft:bush（继承 BushBlock，走默认 #dirt canSurvive）
├── BushPlantBlock.cpp
├── CactusFlowerBlock.hpp    # 仙人掌花方块（继承 FlowerBlock，自定义 canSustain：仙人掌/耕地/实心顶面）
├── CactusFlowerBlock.cpp
├── DryVegetationBlock.hpp   # 干草类基类（继承 BushBlock，重写 canSustain 查 #dry_vegetation_may_place_on）
├── DryVegetationBlock.cpp
├── FireflyBushBlock.hpp     # 萤火虫灌木 minecraft:firefly_bush（继承 BushBlock，走默认 #dirt canSurvive，亮度2）
├── FireflyBushBlock.cpp
├── FlowerBedBlock.hpp       # 花瓣床方块（粉红色花瓣/野花，可堆叠放置）
├── FlowerBedBlock.cpp
├── LeafLitterBlock.hpp      # 枯叶方块（可分段堆叠 1-4 段）
├── LeafLitterBlock.cpp
├── WaterlilyBlock.hpp       # 睡莲方块 minecraft:lily_pad（继承 BushBlock，重写 canSustain：下方水/冰 + 上方无流体）
├── WaterlilyBlock.cpp
└── README.md                # 本文件
```

## 内部模块关系

```
Block
└── BushBlock                          # 植物基类，canSustain 默认委托 canSustainPlant（查 #dirt + FARMLAND）
    ├── FlowerBlock
    │   └── CactusFlowerBlock          # 自定义 canSustain：仙人掌/耕地/实心顶面
    ├── FireflyBushBlock               # 不重写 canSustain，走默认 #dirt（修复浮空 bug）
    ├── BushPlantBlock                 # 不重写 canSustain，走默认 #dirt（修复浮空 bug）
    ├── DryVegetationBlock             # 重写 canSustain 查 #dry_vegetation_may_place_on（沙/陶瓦/泥土/耕地）
    ├── WaterlilyBlock                 # 重写 canSustain：下方水/冰 + 上方无流体（修复浮空 bug）
    └── FlowerBedBlock                 # 花瓣床：FACING + FLOWER_AMOUNT 状态，可堆叠放置/骨粉催熟
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `world/block/blocks/agricultural/BushBlock` | 植物基类（提供 isValidPosition/canSustain 默认委托 + updatePostPlacement） |
| `world/block/BlockTags` | DryVegetationBlock 查询 #dry_vegetation_may_place_on 标签 |
| `world/block/IGrowable` | 骨粉催熟接口（FlowerBedBlock 实现） |
| `world/block/BlockItemRegistry` | 堆叠放置时检查手持物品是否为同类型 BlockItem |
| `entity/utils/ItemDropHelper` | 骨粉催熟满4时弹出物品实体 |
| `entity/entities/player/Player` | isReplaceable 中检查玩家是否潜行 |
| `world/block/IWorld` | 世界接口 |

### 下游依赖（谁依赖本模块）

| 模块 | 用途 |
|------|------|
| `world/block/registry/GardenBlocks` | 注册 WILDFLOWERS/LEAF_LITTER/SHORT_DRY_GRASS/TALL_DRY_GRASS/CACTUS_FLOWER/FIREFLY_BUSH/BUSH |

## 容易踩的坑

1. **装饰植物必须继承 BushBlock 或 DryVegetationBlock，不可用 SimpleBlock**：SimpleBlock 不重写 isValidPosition，走 Block 基类默认实现恒返回 true，导致 SimpleBlockFeature 的 canSurvive 终判失效，placed feature 用 MOTION_BLOCKING 高度图（水算阻挡，返回水面 Y+1）时就会把植物放在水面上一格空气处——表现为浮空。firefly_bush/bush 曾因此 bug 浮空于水面，已改为继承 BushBlock；干草类需在沙/陶瓦上生成，故走 DryVegetationBlock。全仓 `registerBlock<SimpleBlock>` 审计另发现 dead_bush（注册于 NaturalBlocks，改用 DryVegetationBlock）和 lily_pad（注册于 NaturalBlocks，新建 WaterlilyBlock 重写 canSustain 复刻 vanilla WaterlilyBlock.mayPlaceOn：下方水/冰 + 上方无流体）两处同类问题，均已收敛。新增同类装饰植物务必照此办理。

2. **CactusFlowerBlock 的 canSustain 逻辑**: 仙人掌花比普通花朵放置条件更广，不仅限于泥土/耕地，还可以放置在仙人掌上方和任何具有实心顶面的方块上。不要使用 FlowerBlock 默认的 `material.isSolid()` 检查。

3. **FlowerBedBlock 的堆叠放置**: 右键已有花瓣床时 AMOUNT+1（最多4）。堆叠逻辑由 `isReplaceable()` 虚方法控制，需同时满足三个条件：玩家未潜行、手持同类型物品、AMOUNT < 4。`getStateForPlacement()` 负责处理具体的堆叠状态计算（保持原朝向、AMOUNT+1）。

4. **FlowerBedBlock 的骨粉行为**: AMOUNT < 4 时增加1；AMOUNT = 4 时弹出一个自身物品而非继续增加。掉落数量由 loot table 中的 `flower_amount` block_state_property 条件控制。

5. **FlowerBedBlock 的形状计算**: 形状由 FACING 和 AMOUNT 共同决定（4x4=16种），每个花瓣段为 8x3x8 像素盒子，逆时针旋转叠加。

6. **干草类 canSurvive 走 #dry_vegetation_may_place_on 标签**：该标签 = SAND + TERRACOTTA + DIRT + FARMLAND（是 #dirt 的超集），比普通植物的 #dirt 更宽松以支持沙漠/恶地生成。项目 BlockTag 不支持 #tag 嵌套引用，故该标签在 BlockTags.cpp 用合并模式（同 lava_pool_stone_cannot_replace）把 SAND/TERRACOTTA/DIRT 三个标签成员合并后再 add FARMLAND。新增依赖该标签的代码无需关心合并细节，直接用 `BlockTags::DRY_VEGETATION_MAY_PLACE_ON().contains(state)`。

7. **野花地物生成**: MC Java 中野花通过 `WildflowerFeature` 在世界生成时放置（初始 AMOUNT 为1-4随机，4 朝向 × 4 数量共 16 种状态等权重）。本项目复用 `FlowerFeature` 实现，通过 `FlowerFeatures::createWildflowersBirchForest()`（tries=64）和 `FlowerFeatures::createWildflowersMeadow()`（tries=8，稀疏分布）两个预设配置，分别在白桦森林和草甸生物群系中生成。
