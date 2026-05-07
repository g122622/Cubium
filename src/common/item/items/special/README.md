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
```

## 物品类型

| 类名 | 说明 | 实现进度 |
|------|------|----------|
| `BoneMealItem` | 骨粉（肥料） | 基础框架 |
| `BucketItem` | 桶（空/水/岩浆） | 完成 |
| `SpawnEggItem` | 生成蛋 | 实体生成完成 |
| `FishBucketItem` | 鱼桶 | 放置水和鱼完成 |

## 核心机制

### BucketItem (MC 1.16.5)

桶物品，支持空桶、水桶、岩浆桶的功能：
- **空桶**: 从水源方块或含水方块中取出流体，也可对成年牛挤奶
- **装满的桶**: 放置流体方块或向含水方块注入流体
- **牛奶桶**: 由空桶对牛挤奶获得，饮用清除所有药水效果

主要方法：
- `onItemUse`: 在方块上使用桶
- `onItemRightClick`: 右键使用桶
- `itemInteractionForEntity`: 对实体交互（挤奶）
- `tryPlaceContainedLiquid`: 尝试放置流体
- `getFilledBucket`: 根据流体类型获取对应的桶物品
- `getEmptyBucket`: 获取空桶物品

**实体交互（挤奶）**：
- 空桶可以对成年牛（CowEntity）挤奶
- 幼年牛不能被挤奶
- 挤奶后播放音效（ENTITY_COW_MILK）
- 非创造模式下消耗空桶，添加牛奶桶到背包

已注册物品：
| 物品 ID | 说明 | 注册位置 |
|---------|------|----------|
| minecraft:bucket | 空桶 | Items::BUCKET |
| minecraft:water_bucket | 水桶 | Items::WATER_BUCKET |
| minecraft:lava_bucket | 岩浆桶 | Items::LAVA_BUCKET |
| minecraft:milk_bucket | 牛奶桶 | Items::MILK_BUCKET |

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

## 集成状态

### 已完成 ✅
- `Item::itemInteractionForEntity()` 虚方法接口已定义
- `BucketItem::itemInteractionForEntity()` 实现挤奶逻辑
- `ShearsItem::itemInteractionForEntity()` 实现剪毛逻辑
- `NameTagItem::itemInteractionForEntity()` 实现命名逻辑
- `Player::interactOn()` 方法调用 `itemInteractionForEntity`
- `UseEntityPacket` 网络包定义和序列化
- `PacketHandler::handleUseEntity()` 处理实体交互请求
- 单元测试验证核心逻辑正确

### 调用链路

```
客户端 UseEntityPacket
        ↓
服务端 PacketHandler::handleUseEntity()
        ↓ (INTERACT 动作)
Player::interactOn(entity, hand)
        ↓
Entity::processInitialInteract() [待实现]
        ↓ (如果返回 PASS)
Item::itemInteractionForEntity() [已完成]
        ↓
BucketItem / ShearsItem / NameTagItem 具体实现
```

## 参考

- MC 1.16.5: net.minecraft.item.SpawnEggItem
- MC 1.16.5: net.minecraft.item.FishBucketItem
