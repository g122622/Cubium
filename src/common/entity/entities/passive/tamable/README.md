# 可驯服动物

可被玩家驯服的动物实体。

## 文件列表

| 文件 | 说明 |
|------|------|
| TameableEntity.hpp/cpp | 可驯服实体基类 |
| ShoulderRidingEntity.hpp/cpp | 肩膀乘坐实体基类 |
| WolfEntity.hpp/cpp | 狼实体 |
| CatEntity.hpp/cpp | 猫实体 |
| OcelotEntity.hpp/cpp | 豹猫实体 |
| ParrotEntity.hpp/cpp | 鹦鹉实体 |

## 继承

```
MobEntity
└── CreatureEntity
    └── AgeableEntity
        └── AnimalEntity
            └── TameableEntity
                ├── WolfEntity (狼)
                ├── CatEntity (猫)
                ├── OcelotEntity (豹猫)
                └── ParrotEntity (鹦鹉)
```

## 特性

### 驯服系统
- `isTamed()` - 检查是否驯服
- `setTamed(bool)` - 设置驯服状态
- `getOwnerId()` - 获取主人ID
- `setOwnerId(u64)` - 设置主人

### 坐下/站起
- `isSitting()` - 检查是否坐下
- `setSitting(bool)` - 设置坐下状态
- `toggleSitting()` - 切换坐下状态

### 愤怒系统
- 实现 `IAngerable` 接口
- `setAttackTarget()` - 设置攻击目标
- `isAngry()` - 检查是否愤怒

## 使用示例

```cpp
// 驯服狼
if (!wolf->isTamed() && player->hasItem(Items::BONE)) {
    if (random.nextFloat() < 0.33f) {
        wolf->setTamed(true);
        wolf->setOwnerId(player->entityId());
        // 显示爱心效果
    }
}

// 命令坐下
if (wolf->isOwner(player->entityId())) {
    wolf->toggleSitting();
}

// 设置攻击目标
wolf->setAttackTarget(enemy);
```

## 狼的食物系统（WolfEntity）

狼支持多种食物交互，符合 MC 1.16.5 原版行为：

### 驯服物品
- **骨头** (`Items::BONE`) - 驯服狼的唯一物品

### 繁殖和治疗物品
狼可以用任何**肉类**繁殖和治疗，包括：

| 物品 | 饥饿恢复 | 治疗（狼） |
|------|---------|-----------|
| 生猪排 (`PORKCHOP`) | 3 | 3 HP |
| 熟猪排 (`COOKED_PORKCHOP`) | 8 | 8 HP |
| 生牛肉 (`BEEF`) | 3 | 3 HP |
| 熟牛排 (`COOKED_BEEF`) | 8 | 8 HP |
| 生鸡肉 (`CHICKEN`) | 2 | 2 HP |
| 熟鸡肉 (`COOKED_CHICKEN`) | 6 | 6 HP |
| 生兔肉 (`RABBIT`) | 3 | 3 HP |
| 熟兔肉 (`COOKED_RABBIT`) | 5 | 5 HP |
| 生羊肉 (`MUTTON`) | 2 | 2 HP |
| 熟羊肉 (`COOKED_MUTTON`) | 6 | 6 HP |
| **腐肉** (`ROTTEN_FLESH`) | 4 | 4 HP |

**注意**：狼吃腐肉**不会**获得饥饿效果，因为狼的治疗逻辑只调用 `heal(getFood().getHealing())`，不应用食物附带效果。

### 代码实现
```cpp
// WolfEntity::isBreedingItem - 判断是否可用于繁殖
bool WolfEntity::isBreedingItem(const ItemStack& itemStack) const {
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::PORKCHOP
        || item == Items::COOKED_PORKCHOP
        || item == Items::BEEF
        || item == Items::COOKED_BEEF
        || item == Items::CHICKEN
        || item == Items::COOKED_CHICKEN
        || item == Items::RABBIT
        || item == Items::COOKED_RABBIT
        || item == Items::MUTTON
        || item == Items::COOKED_MUTTON
        || item == Items::ROTTEN_FLESH;
}

// WolfEntity::isFoodItem - 判断是否可用于治疗（与繁殖物品相同）
bool WolfEntity::isFoodItem(const ItemStack& itemStack) const {
    return isBreedingItem(itemStack);
}
```

**参考**: `net.minecraft.entity.passive.WolfEntity.isBreedingItem()`

## 鹦鹉驯服系统（ParrotEntity）

鹦鹉是唯一可以停在玩家肩膀上的可驯服动物，也是唯一不能繁殖的可驯服动物。

### 驯服物品

鹦鹉使用**种子**驯服，符合 MC 1.16.5 原版行为：

| 物品 | 说明 |
|------|------|
| 小麦种子 (`WHEAT_SEEDS`) | 可驯服 |
| 南瓜种子 (`PUMPKIN_SEEDS`) | 可驯服 |
| 西瓜种子 (`MELON_SEEDS`) | 可驯服 |
| 甜菜种子 (`BEETROOT_SEEDS`) | 可驯服 |

### 繁殖

鹦鹉**不能繁殖**，这是 MC 1.16.5 原版行为：
- `isBreedingItem()` 始终返回 `false`
- `spawnBaby()` 始终返回 `nullptr`

### 特殊能力

| 特性 | 说明 |
|------|------|
| 飞行 | 可以飞行 (`IFlyingAnimal` 接口) |
| 肩膀乘坐 | 可以停在玩家肩膀上 (`ShoulderRidingEntity`) |
| 声音模仿 | 可以模仿附近敌对生物的声音 |
| 变种 | 5 种颜色变种（红蓝、蓝、绿、黄蓝、灰） |

### 代码实现

```cpp
bool ParrotEntity::isTameItem(const ItemStack& itemStack) const {
    // MC 1.16.5: 鹦鹉用种子驯服
    // 参考: net.minecraft.entity.passive.ParrotEntity.TAME_ITEMS
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item == Items::WHEAT_SEEDS
        || item == Items::PUMPKIN_SEEDS
        || item == Items::MELON_SEEDS
        || item == Items::BEETROOT_SEEDS;
}
```

**参考**: `net.minecraft.entity.passive.ParrotEntity`

## 豹猫信任与繁殖系统（OcelotEntity）

豹猫是丛林中的害羞动物，具有独特的信任机制而非传统驯服系统。

### 信任系统

豹猫采用信任机制而非完全驯服：
- `isTrusting()` - 检查是否已建立信任
- `setTrusting(bool)` - 设置信任状态
- `trustsPlayer(u64)` - 检查是否信任特定玩家
- `setPlayerTrust(u64, bool)` - 设置对玩家的信任

### 繁殖物品

豹猫使用**生鱼**繁殖，符合 MC 1.16.5 原版行为：

| 物品 | 说明 |
|------|------|
| 生鳕鱼 (`COD`) | 可繁殖 |
| 生鲑鱼 (`SALMON`) | 可繁殖 |

**注意**：熟鱼（熟鳕鱼、熟鲑鱼）**不能**用于繁殖豹猫。

### 繁殖行为

```cpp
// OcelotEntity::isBreedingItem - 判断是否可用于繁殖
bool OcelotEntity::isBreedingItem(const ItemStack& itemStack) const {
    // MC 1.16.5: 豹猫使用生鳕鱼和生鲑鱼繁殖
    // BREEDING_ITEMS = Ingredient.fromItems(Items.COD, Items.SALMON)
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item == Items::COD || item == Items::SALMON;
}

// OcelotEntity::spawnBaby - 生成幼体
std::unique_ptr<AnimalEntity> OcelotEntity::spawnBaby(AnimalEntity& /*partner*/) {
    // MC 1.16.5: OcelotEntity.func_241840_a (createChild)
    // 创建一个新的豹猫实体，不需要继承父母特征
    auto baby = std::make_unique<OcelotEntity>(LegacyEntityType::Unknown, 0);
    baby->setChild(true);
    baby->setPosition(x(), y(), z());
    return baby;
}
```

### 特殊属性

| 属性 | 值 | 说明 |
|------|-----|------|
| 生命值 | 10.0 | MC 1.16.5 豹猫生命值 |
| 移动速度 | 0.3 | MC 1.16.5 豹猫移动速度 |
| 眼睛高度（成体） | 0.6 | 成年豹猫眼睛高度 |
| 眼睛高度（幼体） | 0.3 | 幼年豹猫眼睛高度 |

### 豹猫类型

豹猫支持多种皮肤类型（MC 1.16.5 中这些类型用于驯服后的猫，豹猫本身只有野生类型）：

| 类型 | 值 | 说明 |
|------|-----|------|
| Wild | 0 | 野生豹猫 |
| Tuxedo | 1 | 黑白猫 |
| Tabby | 2 | 虎斑猫 |
| Red | 3 | 红猫 |
| Siamese | 4 | 暹罗猫 |
| British | 5 | 英短 |
| Calico | 6 | 三花猫 |
| Persian | 7 | 波斯猫 |
| Ragdoll | 8 | 布偶猫 |
| White | 9 | 白猫 |
| Jellie | 10 | Jellie猫 |

**参考**: `net.minecraft.entity.passive.OcelotEntity`

## AI目标

| Goal | 优先级 | 说明 |
|------|--------|------|
| SitGoal | 1 | 坐下时保持不动 |
| FollowOwnerGoal | 3 | 跟随主人 |
| BegGoal | 7 | 乞求食物 |
| OwnerHurtByTargetGoal | - | 主人被攻击时反击 |
| OwnerHurtTargetGoal | - | 攻击主人攻击的目标 |
