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
├── ShearsItem.cpp/hpp    # 剪刀
```

## 物品类型

| 类名 | 说明 | 实现进度 |
|------|------|----------|
| `BoneMealItem` | 骨粉（肥料） | 基础框架 |
| `BucketItem` | 桶（空/水/岩浆） | 完成 |
| `SpawnEggItem` | 生成蛋 | 实体生成完成 |
| `FishBucketItem` | 鱼桶 | 完成 |
| `MilkBucketItem` | 牛奶桶 | 完成 |
| `NameTagItem` | 命名牌 | 完成 |
| `ShearsItem` | 剪刀 | 完成 |

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
- **FromBucket 标签**：生成的鱼设置 `FromBucket=true`，防止消失

#### 返回空桶逻辑

```cpp
void FishBucketItem::returnEmptyBucket(Player& player, ItemStack& stack) const {
    // 如果物品堆已空，直接替换为空桶
    if (stack.isEmpty() && Items::BUCKET != nullptr) {
        stack = ItemStack(Items::BUCKET, 1);
        return;
    }
    
    // 否则尝试将空桶添加到背包
    if (Items::BUCKET != nullptr) {
        ItemStack bucketStack(Items::BUCKET, 1);
        i32 remaining = player.inventory().add(bucketStack);
        
        // 如果背包满了，掉落到地面
        if (remaining > 0 && !bucketStack.isEmpty()) {
            ItemDropHelper::spawnItemAtEntity(&player, bucketStack, 0.5f, rng);
        }
    }
}
```

#### FromBucket 机制

生成的鱼会设置 `FromBucket` 标签，使其永远不会消失：

```cpp
bool FishBucketItem::spawnFish(IWorld& world, const BlockPos& pos) const {
    // ... 创建鱼实体 ...
    
    // 设置 FromBucket 标签，防止消失
    auto* abstractFish = dynamic_cast<AbstractFishEntity*>(fish.get());
    if (abstractFish != nullptr) {
        abstractFish->setFromBucket(true);
    }
    
    world.spawnEntity(std::move(fish));
    return true;
}
```

### MilkBucketItem (MC 1.16.5)
- 饮用时间：32 ticks
- 使用动作：Drink（饮用）
- 饮用完成：清除所有药水效果，返回空桶
- 背包满时：使用 `ItemDropHelper` 掉落空桶到地面

#### 饮用完成逻辑

```cpp
ItemStack MilkBucketItem::onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) {
    // 清除所有药水效果
    LivingEntity* livingEntity = dynamic_cast<LivingEntity*>(&entity);
    if (livingEntity != nullptr) {
        livingEntity->removeAllEffects();
    }
    
    Player* player = dynamic_cast<Player*>(&entity);
    if (player != nullptr) {
        player->playSound(SoundEvents::ENTITY_PLAYER_BURP, 0.5f, 1.0f);
        
        if (!player->isCreative()) {
            stack.shrink(1);
        }
        
        // 返回空桶
        if (Items::BUCKET != nullptr && !stack.isEmpty()) {
            ItemStack bucketStack(Items::BUCKET, 1);
            i32 remaining = player->inventory().add(bucketStack);
            
            // 背包满时掉落到地面
            if (remaining > 0 && !bucketStack.isEmpty()) {
                ItemDropHelper::spawnItemAtEntity(player, bucketStack, 0.5f, rng);
            }
        }
    }
    
    if (stack.isEmpty() && Items::BUCKET != nullptr) {
        return ItemStack(Items::BUCKET, 1);
    }
    return stack;
}

## 已注册的鱼桶物品

| 物品 ID | 实体类型 | 注册位置 |
|---------|----------|----------|
| minecraft:cod_bucket | COD | Items::COD_BUCKET |
| minecraft:salmon_bucket | SALMON | Items::SALMON_BUCKET |
| minecraft:pufferfish_bucket | PUFFERFISH | Items::PUFFERFISH_BUCKET |
| minecraft:tropical_fish_bucket | TROPICAL_FISH | Items::TROPICAL_FISH_BUCKET |

## 测试用例

- `tests/common/item/special/BucketItemTest.cpp`
  - 桶物品注册验证
  - 空桶/水桶/岩浆桶类型验证
  - 挤奶逻辑测试

- `tests/common/item/special/FishBucketItemTest.cpp`
  - 鱼桶物品注册验证
  - 各类鱼桶的类型名称验证
  - FromBucket 标签与消失机制关联测试
  - ItemDropHelper 生成物品测试
  - 牛奶桶存在性验证

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
