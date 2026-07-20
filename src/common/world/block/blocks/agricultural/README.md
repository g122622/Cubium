# Agricultural Blocks - 农业方块模块

## 概述

本模块实现 Minecraft 中的农业相关方块，包括农作物、耕地、茎类作物、可可豆和瓜果类方块。

## 目录结构

```
agricultural/
├── BushBlock.hpp/cpp          # 灌木/植物基类（所有植物的基类）
├── CropBlock.hpp/cpp          # 农作物基类（小麦、胡萝卜、马铃薯）
├── WheatBlock.hpp/cpp         # 小麦作物
├── CarrotBlock.hpp/cpp        # 胡萝卜作物
├── PotatoBlock.hpp/cpp        # 马铃薯作物
├── BeetrootBlock.hpp/cpp      # 甜菜根作物（4阶段生长）
├── TorchflowerCropBlock.hpp/cpp # 火把花作物（2阶段，成熟后变为火把花方块）
├── PitcherCropBlock.hpp/cpp  # 瓶草作物（5阶段，AGE>=3变双格，继承DoublePlantBlock+IGrowable）
├── FarmlandBlock.hpp/cpp      # 耕地方块
├── StemBlock.hpp/cpp          # 茎类作物基类 + StemGrownBlock + AttachedStemBlock
├── CocoaBlock.hpp/cpp         # 可可豆方块（丛林原木附着）
├── MelonPumpkinBlocks.hpp/cpp # 瓜果类方块（西瓜、南瓜、雕刻南瓜、南瓜灯、茎方块）
└── README.md                  # 本文档
```

## 类层次结构

```
Block
├── BushBlock              # 植物基类
│   ├── CropBlock          # 农作物（小麦、胡萝卜、马铃薯）
│   │   ├── BeetrootBlock      # 甜菜根（4阶段）
│   │   └── TorchflowerCropBlock # 火把花作物（2阶段，成熟后变为火把花方块）
│   ├── StemBlock          # 茎类作物基类（抽象）
│   │   ├── MelonStemBlock      # 西瓜茎
│   │   └── PumpkinStemBlock    # 南瓜茎
│   └── AttachedStemBlock  # 连接茎基类（抽象）
│       ├── MelonAttachedStemBlock   # 连接西瓜茎
│       └── PumpkinAttachedStemBlock # 连接南瓜茎
│   └── DoublePlantBlock   # 双格植物基类
│       └── PitcherCropBlock # 瓶草作物（5阶段，AGE>=3变双格，IGrowable）
├── HorizontalBlock        # 水平方向方块基类
│   ├── CocoaBlock         # 可可豆（附着丛林原木）
│   ├── CarvedPumpkinBlock # 雕刻南瓜（可生成傀儡）
│   └── JackOLanternBlock  # 南瓜灯（可生成傀儡）
├── FarmlandBlock          # 耕地
└── StemGrownBlock         # 茎类果实基类
    ├── MelonBlock         # 西瓜
    └── PumpkinBlock       # 南瓜
```

## 内部模块关系

- **BushBlock** 是所有植物类方块的基类，实现 IPlantable 接口，提供放置检测和形状管理
  - BushBlock::canSustain() 委托给下方方块的 canSustainPlant() 方法，通过 PlantType 判断土壤兼容性
  - BushBlock 默认返回 PlantType::Plains，子类可重写返回其他类型
- **CropBlock** 继承 BushBlock 和 IGrowable，重写 getPlantType() 返回 PlantType::Crop，实现农作物通用生长逻辑（AGE_0_7）
  - `getGrowthChance()` 为 public static 方法，供 StemBlock 等兄弟类复用生长概率计算（3x3 耕地扫描 + 拥挤惩罚）
- **StemBlock** 继承 BushBlock 和 IGrowable，重写 getPlantType() 返回 PlantType::Crop，实现茎类作物生长和果实在成逻辑
  - `randomTick` 使用 `CropBlock::getGrowthChance()` 公式计算生长概率（3x3 耕地扫描 + 拥挤惩罚），而非硬编码
  - 成熟后（AGE=7）的 randomTick 仍受光照和概率控制再尝试结果，而非直接调用 tryGrowFruit
  - 骨粉催熟至最大年龄后调用 `randomTick`（而非 `tryGrowFruit`），确保果实生成受概率控制
  - `tryGrowFruit` 使用 `BlockTags::DIRT` 标签检查果实下方支撑（而非硬编码 FARMLAND/DIRT/GRASS_BLOCK）
- **StemGrownBlock** 是果实方块的基类，通过 `getStem()` 和 `getAttachedStem()` 关联茎方块
- **AttachedStemBlock** 在果实生成后替换普通茎，指向果实方向
- **FarmlandBlock** 独立实现耕地湿润逻辑，重写 canSustainPlant() 接受 PlantType::Crop 和 PlantType::Plains
  - hasCrops() 使用 BlockTags::MAINTAINS_FARMLAND 标签检测上方是否有作物
- **CocoaBlock** 继承 HorizontalBlock 和 IGrowable，实现丛林原木侧面附着生长

## 上下游外部依赖关系

### 上游依赖（本模块使用的其他模块）

| 依赖模块 | 依赖内容 |
|---------|---------|
| `world/block/Block.hpp` | Block 基类 |
| `world/block/IGrowable.hpp` | 骨粉生长接口 |
| `world/block/blocks/HorizontalBlock.hpp` | 水平方向方块基类 |
| `world/block/BlockStateProperties.hpp` | AGE_0_7、AGE_0_2、AGE_0_3、HORIZONTAL_FACING、MOISTURE_0_7 等属性 |
| `world/block/BlockTags.hpp` | JUNGLE_LOGS 标签（可可豆附着检测）、DIRT 标签（果实支撑判定） |
| `world/block/Material.hpp` | PLANTS、EARTH 材质 |
| `physics/collision/CollisionShape.hpp` | 碰撞形状 |
| `entity/Entity.hpp` | 实体基类（耕地跳跃破坏检测） |
| `entity/utils/ItemDropHelper.hpp` | 物品掉落工具（南瓜雕刻种子掉落） |
| `util/math/random/Random.hpp` | 随机数生成器 |

### 下游依赖（使用本模块的其他模块）

| 下游模块 | 使用内容 |
|---------|---------|
| `world/block/VanillaBlocks.hpp` | 注册所有农业方块实例 |
| `world/block/BlockTags.hpp` | CROPS、MAINTAIN_FARMLAND 等标签 |
| `world/item/ItemRegistry.hpp` | 种子物品、作物物品注册 |
| `world/feature/` | 树木生成特性（树苗支撑检测） |
| `world/tick/` | 随机刻调度系统 |

## 容易踩的坑

### 1. CropBlock 骨粉增长随机数

**问题**：骨粉增长使用全局 `rand()` 会导致不确定性，同一世界内结果不可复现。

**解决方案**：`getBonemealAgeIncrease()` 必须从世界种子和方块位置派生随机数，使用 `world.getRandom()` 或 `PositionalRandom`。

### 2. FarmlandBlock 降雨补湿条件

**问题**：降雨补湿只检查 `isRaining()` 会导致测试世界伪阳性，因为沙漠等无降水生物群系也会"下雨"。

**解决方案**：降雨补湿要同时检查 `isRaining()` 和 `canRainAt(pos.up())`。

### 3. StemGrownBlock 循环依赖

**问题**：MelonBlock/PumpkinBlock 与 MelonStemBlock/PumpkinStemBlock 相互引用导致构造顺序问题。

**解决方案**：使用 `setStem()` 和 `setAttachedStem()` 方法延迟注入茎方块指针，在 `VanillaBlocks::initialize()` 中完成双向关联。

### 4. AttachedStemBlock 果实破坏检测

**问题**：果实被破坏时连接茎不会变回普通茎，导致世界状态不一致。

**解决方案**：`AttachedStemBlock::updatePostPlacement()` 必须检测指向方向的方块是否仍为对应果实，若不是则变回普通茎（AGE=7）。

### 5. CocoaBlock 放置朝向

**问题**：可可豆放置时朝向计算错误会导致玩家朝向与实际放置方向不一致。

**解决方案**：`getStateForPlacement()` 需遍历玩家朝向的各个水平方向，找到第一个有效附着面（JUNGLE_LOGS 标签方块），而不是直接使用玩家朝向。

### 6. BeetrootBlock 骨粉增长范围

**问题**：甜菜根只有 4 个生长阶段（AGE_0_3），但 `getBonemealAgeIncrease()` 返回值可能溢出。

**解决方案**：甜菜根需重写 `getBonemealAgeIncrease()` 返回较小范围（0-2），并在 `grow()` 中使用 `min(age, getMaxAge())` 限制。

### 7. 傀儡生成方向一致性

**问题**：铁傀儡 T 形结构检测时，手臂方向与南瓜朝向不一致会导致生成失败。

**解决方案**：`CarvedPumpkinBlock::checkIronGolemPattern()` 需检测两个垂直方向（东西/南北），南瓜的 FACING 属性决定傀儡朝向。

**铜傀儡生成**（MC 1.21.11）：`CarvedPumpkinBlock` 同时支持雪/铁/铜三种傀儡模式，优先级为 雪 > 铁 > 铜。铜傀儡模式为垂直 2 格（南瓜 + 任意 `BlockTags::COPPER` 标签内的铜块），生成后会用对应氧化等级的铜箱子替换铜块位置（`CopperChestBlock::getFromCopperBlock`）。铜傀儡的氧化等级由铜块状态推导：直接实现 `IOxidizableBlock` 的铜块取其氧化等级；涂蜡变种通过 `HoneycombItem::getWaxOffMap` 查找未涂蜡变种后取等级。`canSpawnGolem(IWorld, BlockPos)` 公共 API 仅检查身体部分（雪/铁/铜），头部由调用方提供。

### 8. 农作物支撑检测

**问题**：农作物只能种植在耕地上，但 `BushBlock::canSustain()` 默认返回 false，导致所有作物无法放置。

**解决方案**：`CropBlock::canSustain()` 重写返回 PlantType::Crop，BushBlock::canSustain() 委托给下方方块的 canSustainPlant()，FarmlandBlock::canSustainPlant() 接受 Crop 类型。

### 9. IPlantable / PlantType 集成

**架构说明**：所有植物方块通过 IPlantable 接口报告其 PlantType，土壤方块通过 canSustainPlant() 方法判断是否支撑该植物类型。

| PlantType | 含义 | 对应植物 | 可种植土壤 |
|-----------|------|---------|-----------|
| Plains | 平原植物 | 花草、树苗等 BushBlock 子类 | DIRT 标签 + FARMLAND |
| Crop | 农作物 | 小麦、胡萝卜、马铃薯等 | FARMLAND |
| Desert | 沙漠植物 | 仙人掌 | SAND 标签 |
| Beach | 海滩植物 | 甘蔗、竹子 | DIRT 标签 + SAND 标签 |
| Cave | 洞穴植物 | 蘑菇 | MYCELIUM + PODZOL |
| Water | 水生植物 | 睡莲、海草、海带 | 由植物自身 isValidPosition 处理 |
| Nether | 下界植物 | 地狱疣、菌索 | CRIMSON_NYLIUM + WARPED_NYLIUM + MYCELIUM + SOUL_SOIL + DIRT 标签 + FARMLAND |

### 10. StemBlock 果实支撑判定必须使用 BlockTags::DIRT

**问题**：`tryGrowFruit` 中检查果实下方支撑方块时，不能硬编码 `FARMLAND`/`DIRT`/`GRASS_BLOCK` 三个方块指针。原版使用 `BlockTags.DIRT` 标签，覆盖 podzol、coarse_dirt、mycelium、rooted_dirt、moss_block、mud、muddy_mangrove_roots、pale_moss_block 等所有泥土类方块。

**解决方案**：使用 `BlockTags::DIRT().contains(*belowFruitState)` 替代硬编码检查。FARMLAND 已在 DIRT 标签中，无需额外检查。

### 11. StemBlock 成熟后的 randomTick 仍受光照和概率控制

**问题**：原版 `StemBlock.randomTick` 中，AGE=7 时尝试结果仍需通过 `getRawBrightness >= 9` 和 `nextInt(25/f + 1) == 0` 的概率检查，而非直接调用 `tryGrowFruit`。同样，骨粉催熟至 AGE=7 后调用 `blockstate.randomTick()`，而非直接结果。

**解决方案**：`randomTick` 采用统一流程（光照→概率→年龄分支），`grow` 方法在 `newAge == maxAge` 时调用 `randomTick` 而非 `tryGrowFruit`。

### 12. PitcherCropBlock Ravager 破坏作物逻辑

**行为**：`PitcherCropBlock::onEntityCollision` 实现 MC Java `PitcherCropBlock#entityInside` 的 Ravager 破坏作物逻辑。当满足以下全部条件时，方块会被破坏并掉落物品：

1. 服务端执行（`world.isClientSide() == false`，对应 MC Java `world instanceof ServerLevel`）
2. 实体为 Ravager（`entity.entityType() == VanillaEntityTypeKeys::RAVAGER`，对应 `entity instanceof Ravager`）
3. `mobGriefing` 游戏规则为 `true`

**实现要点**：项目无 `IWorld::destroyBlock` 方法，采用 `setBlockState(pos, air, 3) + spawnAfterBreak(...)` 的等价模式，与 `RavagerEntity::_breakLeavesOnCollision` / `EnderDragonEntity::_destroyBlocksInAABB` 一致。破坏前需保存 `state.getBlock()` 引用，因为 `setBlockState` 后 `state` 引用可能失效。

**注意**：无论作物是否成熟（AGE 0-4 均可）、是单格还是双格状态，Ravager 都会破坏。双格状态下，破坏下半部分不会自动清除上半部分（与 MC Java 一致，`destroyBlock` 只破坏指定位置）。

### 13. 农民村民对作物的种植与收获行为（MC 1.21.11 HarvestFarmland）

**种植**：农民村民通过 `ItemTags::VILLAGER_PLANTABLE_SEEDS` 标签判断可种植物品，包含 6 种种子：小麦种子、胡萝卜、马铃薯、甜菜种子、火把花种子、瓶草荚果。种植路径通过 `BlockItem::block()` 获取对应作物方块并放置默认状态（age=0），不要求方块继承 `CropBlock`，因此瓶草作物（`PitcherCropBlock : DoublePlantBlock`）也可被种植。

**收获**：`FarmerWorkGoal::_isCropMatureAt()` 通过 `dynamic_cast<CropBlock*>` 判断可收获方块，因此村民**仅收获 `CropBlock` 子类的作物**（小麦、胡萝卜、马铃薯、甜菜根、火把花作物）。瓶草作物继承自 `DoublePlantBlock` 而非 `CropBlock`，村民**不会收获**——这与 MC 1.21.11 原版行为一致（`HarvestFarmland` 通过 `instanceof CropBlock` 判断）。

**设计说明**：`PitcherCropBlock::getCropItem()` / `getSeedItem()` 当前未被 `FarmerWorkGoal` 调用，保留供未来扩展使用（例如其他 AI 或统计需要查询作物产物）。瓶草作物的掉落由战利品表驱动（`minecraft:blocks/pitcher_crop`）。
