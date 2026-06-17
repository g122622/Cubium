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
│   ├── StemBlock          # 茎类作物基类（抽象）
│   │   ├── MelonStemBlock      # 西瓜茎
│   │   └── PumpkinStemBlock    # 南瓜茎
│   └── AttachedStemBlock  # 连接茎基类（抽象）
│       ├── MelonAttachedStemBlock   # 连接西瓜茎
│       └── PumpkinAttachedStemBlock # 连接南瓜茎
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

**解决方案**：`CarvedPumpkinBlock::_checkIronGolemPattern()` 需检测两个垂直方向（东西/南北），南瓜的 FACING 属性决定傀儡朝向。

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
