# 可驯服动物

可被玩家驯服的动物实体。

## 文件列表

| 文件 | 说明 |
|------|------|
| TameableEntity.hpp/cpp | 可驯服实体基类 |
| WolfEntity.hpp/cpp | 狼实体 |

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

## AI目标

| Goal | 优先级 | 说明 |
|------|--------|------|
| SitGoal | 1 | 坐下时保持不动 |
| FollowOwnerGoal | 3 | 跟随主人 |
| BegGoal | 7 | 乞求食物 |
| OwnerHurtByTargetGoal | - | 主人被攻击时反击 |
| OwnerHurtTargetGoal | - | 攻击主人攻击的目标 |
