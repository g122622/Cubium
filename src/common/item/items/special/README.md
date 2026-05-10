# 特殊物品模块 (Special Items)

特殊物品模块提供功能性物品的实现。

## 目录结构

```
special/
├── README.md           # 本文档
├── BoneMealItem.cpp/hpp # 骨粉
├── BucketItem.cpp/hpp   # 桶（空桶、水桶、岩浆桶）
├── SpawnEggItem.cpp/hpp # 生成蛋
├── FishBucketItem.cpp/hpp # 鱼桶
├── MilkBucketItem.cpp/hpp # 牛奶桶
├── NameTagItem.cpp/hpp   # 命名牌
├── SaddleItem.cpp/hpp    # 鞍
├── ShearsItem.cpp/hpp    # 剪刀
```

## 物品类型

| 类名 | 说明 | 实现进度 |
|------|------|----------|
| `BoneMealItem` | 骨粉（肥料，海草生成） | 完成 |
| `BucketItem` | 桶（空/水/岩浆） | 完成 |
| `SpawnEggItem` | 生成蛋 | 实体生成完成 |
| `FishBucketItem` | 鱼桶 | 完成 |
| `MilkBucketItem` | 牛奶桶 | 完成 |
| `NameTagItem` | 命名牌 | 完成 |
| `SaddleItem` | 鞍 | 完成 |
| `ShearsItem` | 剪刀 | 完成 |

## 核心机制

### BoneMealItem (MC 1.16.5)

骨粉物品，用于加速植物生长和生成海草：

**主要功能：**
- **onItemUse()**: 对 IGrowable 方块使用，加速生长
- **applyBonemeal()**: 静态方法，应用骨粉效果
- **growSeagrass()**: 在水下生成海草（MC 1.16.5 完整实现）
- **spawnBonemealParticles()**: 生成快乐村民粒子效果

**海草生成逻辑 (MC 1.16.5 对齐)：**
- 检查目标位置是否为完整水源方块（流体等级 == 8）
- 128 次循环随机偏移位置，尝试放置海草
- 根据生物群系决定生成内容：
  - 普通海洋：生成海草
  - 温暖海洋：有机会生成珊瑚扇或墙珊瑚
- 已有海草有 10% 概率升级为高海草
- 使用 `WALL_CORALS` 和 `UNDERWATER_BONEMEALS` 方块标签

```cpp
// 对 IGrowable 方块使用骨粉
ItemStack boneMealStack(Items::BONE_MEAL, 1);
bool success = BoneMealItem::applyBonemeal(boneMealStack, world, pos, player);

// 在水下生成海草
math::Random random(world.seed());
bool placed = BoneMealItem::growSeagrass(world, pos, random);
```

**IGrowable 集成：**
- 对 `SeagrassBlock` 使用骨粉会将其变成 `TallSeagrassBlock`（高海草）
- 需要上方有水源方块才能成功
- 参考 MC 1.16.5: `net.minecraft.item.BoneMealItem.growSeagrass()`

### BucketItem (MC 1.16.5)