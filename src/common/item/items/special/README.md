# 特殊物品模块 (Special Items)

特殊物品模块提供功能性物品的实现。

## 目录结构

```
special/
├── README.md           # 本文档
├── BoneMealItem.cpp/hpp # 骨粉
├── SpawnEggItem.cpp/hpp # 生成蛋
├── FishBucketItem.cpp/hpp # 鱼桶
```

## 物品类型

| 类名 | 说明 | 实现进度 |
|------|------|----------|
| `BoneMealItem` | 骨粉（肥料） | 基础框架 |
| `SpawnEggItem` | 生成蛋 | 实体生成完成 |
| `FishBucketItem` | 鱼桶 | 放置水和鱼完成 |

## 核心机制

### SpawnEggItem (MC 1.16.5)
- 支持自定义实体类型和颜色
- `onItemUse`: 在方块上生成实体
- `onItemRightClick`: 在玩家位置生成实体
- 通过 EntityRegistry 创建实体
- 消耗物品（非创造模式）

### FishBucketItem (MC 1.16.5)
- 支持自定义鱼类型（通过实体类型名称）
- `onItemUse`: 放置水方块并生成鱼
- `onItemRightClick`: 在水中生成鱼
- 返回空桶（非创造模式）

## 已注册的鱼桶物品

| 物品 ID | 实体类型 | 注册位置 |
|---------|----------|----------|
| minecraft:cod_bucket | COD | Items::COD_BUCKET |
| minecraft:salmon_bucket | SALMON | Items::SALMON_BUCKET |
| minecraft:pufferfish_bucket | PUFFERFISH | Items::PUFFERFISH_BUCKET |
| minecraft:tropical_fish_bucket | TROPICAL_FISH | Items::TROPICAL_FISH_BUCKET |

## 使用方法

```cpp
// 创建生成蛋
auto codSpawnEgg = std::make_unique<SpawnEggItem>(
    EntityType::COD,
    12691306,  // 主颜色 (浅灰)
    15058059,  // 副颜色 (白色)
    ItemProperties().maxStackSize(64)
);

// 鱼桶已在 Items.cpp 中注册，直接使用静态指针
Item* codBucket = Items::COD_BUCKET;
```

## 待实现的水域更新生成蛋

| 物品 ID | 实体类型 | 主颜色 | 副颜色 |
|---------|----------|--------|--------|
| minecraft:turtle_spawn_egg | TURTLE | 15198183 | 44975 |
| minecraft:phantom_spawn_egg | PHANTOM | 4411786 | 8978176 |
| minecraft:dolphin_spawn_egg | DOLPHIN | 2243405 | 16382457 |
| minecraft:drowned_spawn_egg | DROWNED | 9433559 | 7969893 |
| minecraft:cod_spawn_egg | COD | 12691306 | 15058059 |
| minecraft:salmon_spawn_egg | SALMON | 10489616 | 951412 |
| minecraft:pufferfish_spawn_egg | PUFFERFISH | 16167425 | 3654642 |
| minecraft:tropical_fish_spawn_egg | TROPICAL_FISH | 15690005 | 16775663 |

## 依赖项

| 模块 | 用途 |
|------|------|
| `item/core/Item` | 物品基类 |
| `entity/core/EntityType` | 实体类型 |
| `entity/core/EntityRegistry` | 实体注册表 |
| `world/IWorld` | 世界接口 |
| `entity/entities/player/Player` | 玩家接口 |

## 参考

- MC 1.16.5: net.minecraft.item.SpawnEggItem
- MC 1.16.5: net.minecraft.item.FishBucketItem
