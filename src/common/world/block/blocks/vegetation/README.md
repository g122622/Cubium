# 植被方块模块 (Vegetation Blocks)

植被方块模块提供所有植物类方块的实现。

## 目录结构

```
vegetation/
├── README.md                 # 本文档
├── DoublePlantBlock.hpp/cpp  # 双格植物基类（向日葵、丁香等）
├── TallGrassBlock.hpp/cpp    # 高草/蕨类
├── FlowerBlock.hpp/cpp       # 花朵（单格/双格）
├── SaplingBlock.hpp/cpp      # 树苗
├── MushroomBlock.hpp/cpp     # 蘑菇/巨型蘑菇
├── CactusBlock.hpp/cpp       # 仙人掌
├── SugarCaneBlock.hpp/cpp    # 甘蔗
├── VineBlock.hpp/cpp         # 藤蔓
└── LilyPadBlock.hpp/cpp      # 睡莲
```

## 类层次结构

```
Block
├── BushBlock              # 植物基类（来自 agricultural 目录）
│   ├── DoublePlantBlock   # 双格植物
│   │   ├── LilacBlock     # 丁香
│   │   ├── RoseBushBlock  # 玫瑰丛
│   │   ├── PeonyBlock     # 牡丹
│   │   └── SunflowerBlock # 向日葵
│   ├── TallGrassBlock     # 高草
│   │   └── FernBlock      # 蕨类
│   ├── FlowerBlock        # 花朵
│   ├── SaplingBlock       # 树苗
│   └── LilyPadBlock       # 睡莲
├── MushroomBlock          # 蘑菇
├── HugeMushroomBlock      # 巨型蘑菇
├── CactusBlock            # 仙人掌
├── SugarCaneBlock         # 甘蔗
└── VineBlock              # 藤蔓
```

## 方块状态属性

### DoublePlantBlock（双格植物）
- `HALF`: DoubleBlockHalf (UPPER, LOWER) - 上半部分/下半部分

### SaplingBlock（树苗）
- `STAGE_0_1`: 生长阶段 (0-1)

### CactusBlock（仙人掌）
- `AGE_0_15`: 年龄 (0-15)

### SugarCaneBlock（甘蔗）
- `AGE_0_15`: 年龄 (0-15)

### VineBlock（藤蔓）
- `UP`: 是否向上延伸
- `NORTH/SOUTH/EAST/WEST`: 各方向是否附着

## 核心机制

### 双格植物放置
1. 放置时检查上方是否有空间
2. 下半部分由玩家放置
3. 上半部分自动生成
4. 破坏下半部分时整个植物消失

### 树苗生长
1. 使用 STAGE_0_1 属性表示生长阶段
2. 随机 tick 时检查生长条件
3. 成熟后调用树木生成器
4. 生成树后移除树苗

### 仙人掌生长
1. 检查周围是否有固体方块（不能有）
2. 高度限制为 3 格
3. 随机 tick 时有概率生长
4. 接触造成伤害

### 甘蔗生长
1. 必须靠近水源
2. 高度限制为 3 格
3. 随机 tick 时有概率生长
4. 可放置在草方块、泥土、沙子等上

### 藤蔓附着
1. 检查相邻固体方块
2. 可以向下延伸
3. 可以攀爬
4. 随机蔓延

## 使用方法

### 创建树苗

```cpp
// 创建橡树树苗
auto oakSapling = std::make_unique<SaplingBlock>(
    [](IWorld& world, const BlockPos& pos, math::IRandom& rng) {
        // 生成橡树
        TreeFeatures::createOakTree()->place(world, pos, rng);
    },
    BlockProperties(Materials::PLANTS())
        .hardness(0.0f)
        .noCollision()
);
```

### 创建花朵

```cpp
// 创建蒲公英
auto dandelion = std::make_unique<FlowerBlock>(
    BlockProperties(Materials::PLANTS())
        .hardness(0.0f)
        .noCollision(),
    0,  // 无可疑炖汤效果
    0
);
```

### 创建双格植物

```cpp
// 创建向日葵
auto sunflower = std::make_unique<SunflowerBlock>(
    BlockProperties(Materials::PLANTS())
        .hardness(0.0f)
        .noCollision()
);
```

## 依赖项

### 内部依赖

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/blocks/BushBlock` | 植物基类 |
| `world/block/Material` | 材质系统 |
| `world/IWorld` | 世界接口 |
| `util/property/Properties` | 方块属性 |
| `physics/collision/CollisionShape` | 碰撞形状 |

### 外部依赖

- `glm` - 数学库
- `spdlog` - 日志

## 容易踩的坑

### 1. 双格植物状态丢失

**问题**: 只更新了下半部分，忘记更新上半部分。

**解决方案**: 使用 `DoublePlantBlock::placeAt()` 静态方法。

```cpp
// 错误：只放置下半部分
world.setBlockState(pos, &lowerState, 2);

// 正确：同时放置两部分
DoublePlantBlock::placeAt(world, pos, lowerState, 2);
```

### 2. 仙人掌周围固体检测

**问题**: 仙人掌周围检测不正确导致仙人掌被错误移除。

**解决方案**: 只检测水平四个方向，不检测上下。

```cpp
// 检测水平方向
for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
    // 检测逻辑...
}
```

### 3. 藤蔓附着检测

**问题**: 藤蔓在更新时检测错误导致消失。

**解决方案**: 正确检测固体方块的侧面。

```cpp
bool VineBlock::canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const {
    BlockPos adjPos = pos.offset(direction);
    const BlockState* adjState = world.getBlockState(adjPos.x, adjPos.y, adjPos.z);
    return adjState != nullptr && adjState->isSolid();
}
```

### 4. 睡莲水面放置

**问题**: 睡莲需要检查下方是否为水，但也要考虑上方是否为空。

**解决方案**: 同时检查下方是水和当前方块为空气。

```cpp
bool LilyPadBlock::isValidPosition(...) const {
    // 下方必须是水
    // 当前位置必须是空气
    return belowIsWater && currentIsAir;
}
```

## 参考文档

- MC 1.16.5 Source - DoublePlantBlock
- MC 1.16.5 Source - SaplingBlock
- MC 1.16.5 Source - CactusBlock
- MC 1.16.5 Source - SugarCaneBlock
- MC 1.16.5 Source - VineBlock
- MC 1.16.5 Source - LilyPadBlock
