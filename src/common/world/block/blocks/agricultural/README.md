# Agricultural Blocks - 农业方块模块

## 概述

本模块实现 Minecraft 中的农业相关方块，包括农作物、耕地、茎类作物和可可豆。

## 目录结构

```
agricultural/
├── BushBlock.hpp/cpp       # 灌木/植物基类
├── CropBlock.hpp/cpp       # 农作物基类（小麦、胡萝卜、马铃薯）
├── FarmlandBlock.hpp/cpp   # 耕地方块
├── StemBlock.hpp/cpp       # 茎类作物（西瓜茎、南瓜茎）
├── CocoaBlock.hpp/cpp      # 可可豆方块（丛林原木附着）
└── README.md               # 本文档
```

## 类层次结构

```
Block
├── BushBlock              # 植物基类
│   ├── CropBlock          # 农作物（小麦、胡萝卜、马铃薯）
│   ├── StemBlock          # 茎类作物（西瓜茎、南瓜茎）
│   └── AttachedStemBlock  # 连接茎（西瓜连接茎、南瓜连接茎）
├── HorizontalBlock        # 水平方向方块基类
│   └── CocoaBlock         # 可可豆（附着丛林原木）
├── FarmlandBlock          # 耕地
└── StemGrownBlock         # 茎类果实（西瓜、南瓜）
```

## 方块状态属性

### CropBlock（农作物）
- `AGE_0_7`: 生长阶段（0-7，共8个阶段）

### FarmlandBlock（耕地）
- `MOISTURE_0_7`: 湿润等级（0-7，7为最湿润）

### StemBlock（茎类作物）
- `AGE_0_7`: 生长阶段（0-7）

### AttachedStemBlock（连接茎）
- `HORIZONTAL_FACING`: 朝向（指向果实方向）

### CocoaBlock（可可豆）
- `HORIZONTAL_FACING`: 朝向（指向丛林原木方向）
- `AGE_0_2`: 生长阶段（0-2，共3个阶段）

## 核心机制

### 农作物生长

1. **光照检查**: 光照等级 >= 9 时才能生长
2. **随机 Tick**: 未成熟时每 tick 有 1/25 概率生长
3. **生长速度**: 受周围耕地湿润度影响
4. **骨粉加速**: 使用骨粉可立即增长 2-5 个阶段，增长值由世界种子和方块位置派生的确定性随机数生成

### 耕地湿润

1. **水源检测**: 4 格范围内的水源会湿润耕地
2. **雨水湿润**: 下雨时耕地会变湿润，判定通过 `world.isRaining()` + `world.canRainAt(pos.up())` 完成
3. **湿润衰减**: 无水无雨时湿润度每 tick 降低
4. **变回泥土**: 干燥且无作物时耕地会变回泥土

### 茎类作物

1. **生长阶段**: 0-7，与普通作物类似
2. **果实生成**: 成熟时在相邻空位生成西瓜/南瓜
3. **茎变形**: 果实生成后茎变为连接茎
4. **连接茎**: 指向果实方向，不再生长

### 可可豆（CocoaBlock）

1. **附着检测**: 只能附着在丛林原木（JUNGLE_LOGS 标签）的侧面
2. **放置逻辑**: 尝试玩家朝向的各个水平方向，找到第一个有效附着面
3. **生长阶段**: 0-2 共3个阶段，每个阶段形状逐渐变大
4. **随机生长**: 光照 >= 9 时，每次随机 tick 有 1/5 概率增长
5. **骨粉催熟**: 骨粉使可可豆增长一个阶段（如果未成熟）
6. **形状变化**: AGE 0 为小可可豆（4x5x4像素），AGE 2 为大可可豆（8x9x8像素）

## 使用示例

```cpp
// 创建小麦作物
BlockProperties wheatProps = BlockProperties::create()
    .material(Materials::PLANTS())
    .hardness(0.0f)
    .noCollision();

auto wheatBlock = std::make_unique<CropBlock>(wheatProps);

// 创建耕地
BlockProperties farmlandProps = BlockProperties::create()
    .material(Materials::EARTH())
    .hardness(0.6f);

auto farmlandBlock = std::make_unique<FarmlandBlock>(farmlandProps);

// 创建西瓜茎
BlockProperties stemProps = BlockProperties::create()
    .material(Materials::PLANTS())
    .hardness(0.0f)
    .noCollision();

auto melonStemBlock = std::make_unique<StemBlock>(melonBlock, stemProps);
```

## 待实现

- [x] 小麦作物（WheatBlock）- 已创建 CropBlocks.hpp/cpp
- [x] 胡萝卜作物（CarrotBlock）
- [x] 马铃薯作物（PotatoBlock）
- [x] 甜菜根作物（BeetrootBlock）
- [x] 可可豆（CocoaBlock）
- [x] 西瓜/南瓜果实块（MelonBlock, PumpkinBlock, CarvedPumpkinBlock, JackOLanternBlock）
- [ ] 甘蔗（SugarCaneBlock）
- [ ] 仙人掌（CactusBlock）
- [ ] 竹子（BambooBlock）

## 参考

- net.minecraft.block.BushBlock
- net.minecraft.block.CropsBlock
- net.minecraft.block.FarmlandBlock
- net.minecraft.block.StemBlock
- net.minecraft.block.AttachedStemBlock
- net.minecraft.block.StemGrownBlock
