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
- `isTameItem(const ItemStack&)` - 检查物品是否可用于驯服（虚方法，子类重写）

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

### 驯服交互

鹦鹉通过 `interactMob()` 方法处理玩家交互：

| 交互 | 条件 | 效果 |
|------|------|------|
| 喂食种子驯服 | 未驯服 + 手持种子 | 1/10 概率驯服，消耗物品 |
| 切换坐下 | 已驯服 + 主人交互 | 切换坐下/站立状态 |

**驯服流程**：
1. 玩家手持种子（小麦/南瓜/西瓜/甜菜种子）右键点击鹦鹉
2. 播放吃东西声音 (`ENTITY_PARROT_EAT`)
3. 服务端进行 1/10 概率判定
4. 成功：设置驯服状态、设置主人、广播心形粒子
5. 失败：广播烟雾粒子

```cpp
// ParrotEntity::interactMob - 驯服交互逻辑
ActionResultType ParrotEntity::interactMob(Player& player, Hand hand) {
    ItemStack itemStack = player.getHeldItem(hand);
    
    // 驯服逻辑
    if (!isTamed() && isTameItem(itemStack)) {
        // 消耗物品（非创造模式）
        if (!player.abilities().creativeMode) {
            itemStack.shrink(1);
        }
        
        // 播放吃东西声音
        playSound(SoundEvents::ENTITY_PARROT_EAT, 1.0f, 1.0f);
        
        // 服务端处理驯服逻辑
        if (!m_world->isClientSide()) {
            if (getRandom().nextInt(10) == 0) {  // 1/10 概率
                setTamed(true);
                setOwnerId(player.playerId());
                // 广播心形粒子
                m_world->broadcastEntityStatus(id(), TamingSucceeded);
            } else {
                // 广播烟雾粒子
                m_world->broadcastEntityStatus(id(), TamingFailed);
            }
        }
        return ActionResultType::Success;
    }
    
    // 切换坐下状态
    if (isTamed() && isOwner(player.playerId())) {
        toggleSitting();
        return ActionResultType::Success;
    }
    
    return ShoulderRidingEntity::interactMob(player, hand);
}
```

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

### 信任建立机制

豹猫通过喂食生鱼建立信任，有 1/3 概率成功：

```cpp
// OcelotEntity::interactMob - 玩家交互建立信任
ActionResultType OcelotEntity::interactMob(Player& player, Hand hand) {
    ItemStack itemStack = player.getHeldItem(hand);
    const Item* item = itemStack.getItem();
    
    // 条件检查：
    // 1. 诱惑目标正在运行（或为空）
    // 2. 尚未信任
    // 3. 手持繁殖物品（生鱼）
    // 4. 玩家距离 < 9.0D (3格)
    bool isTempting = (m_temptGoal == nullptr || m_temptGoal->isRunning());
    bool isBreedingFood = item != nullptr && (item == Items::COD || item == Items::SALMON);
    double distSq = player.distanceSqTo(*this);
    
    if (isTempting && !m_trusting && isBreedingFood && distSq < 9.0) {
        itemStack.shrink(1);
        
        if (!m_world->isClientSide()) {
            // MC 1.16.5: 1/3 概率建立信任
            math::Random rng = getRandom();
            if (rng.nextInt(3) == 0) {
                setPlayerTrust(player.playerId(), true);
                spawnTrustingParticles(true);  // 心形粒子
            } else {
                spawnTrustingParticles(false); // 烟雾粒子
            }
        }
        return ActionResultType::Success;
    }
    
    return AnimalEntity::interactMob(player, hand);
}
```

### AI 目标列表

豹猫实现了完整的 AI 目标系统，符合 MC 1.16.5 原版行为：

| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | SwimGoal | 游泳（最高优先级） |
| 3 | OcelotTemptGoal | 生鱼诱惑（未信任时被快速移动吓跑） |
| 4 | OcelotAvoidPlayerGoal | 避开玩家（未信任时） |
| 7 | LeapAtTargetGoal | 跳跃攻击目标 |
| 8 | OcelotAttackGoal | 近战攻击（小鸡/海龟） |
| 9 | BreedGoal | 繁殖 |
| 10 | WaterAvoidingRandomWalkingGoal | 避水随机漫步 |
| 11 | LookAtGoal | 看向玩家 |
| - | NearestAttackableTargetGoal<ChickenEntity> | 攻击目标选择器：小鸡 |
| - | NearestAttackableTargetGoal<TurtleEntity> | 攻击目标选择器：海龟 |

### 内部 AI Goal 类

豹猫实现了三个内部 Goal 类来支持特有的行为：

#### OcelotAvoidPlayerGoal

继承自 `AvoidEntityGoal`，只在未信任时执行：

```cpp
class OcelotAvoidPlayerGoal : public AvoidEntityGoal {
public:
    OcelotAvoidPlayerGoal(OcelotEntity* ocelot, f32 avoidDistance, 
                          f64 farSpeed, f64 nearSpeed);
    bool shouldExecute() override;           // 只在未信任时返回 true
    bool shouldContinueExecuting() override; // 只在未信任时返回 true
};
```

- **检测距离**: 16 格
- **远距离逃避速度**: 0.8
- **近距离逃避速度**: 1.33（更快）
- **动态管理**: 通过 `setupTrustingAI()` 在信任建立时移除

#### OcelotTemptGoal

继承自 `TemptGoal`，被快速移动吓跑的行为根据信任状态变化：

```cpp
class OcelotTemptGoal : public TemptGoal {
public:
    OcelotTemptGoal(OcelotEntity* ocelot, f64 speed, 
                    ItemPredicate itemPredicate, bool scaredByMovement);
protected:
    bool isScaredByPlayerMovement() const override; // 未信任时才害怕
};
```

- **诱惑速度**: 0.6
- **诱惑物品**: COD、SALMON
- **scaredByMovement**: true（但信任后不再害怕）

#### OcelotAttackGoal

继承自 `Goal`，实现豹猫特有的跳跃攻击：

```cpp
class OcelotAttackGoal : public Goal {
public:
    explicit OcelotAttackGoal(OcelotEntity* ocelot);
    bool shouldExecute() override;
    bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;
};
```

- **互斥标志**: `Move`, `Look`
- **攻击冷却**: 20 ticks
- **停止追踪距离**: 15 格
- **攻击范围**: `width * 2`

### 动态 AI 管理

豹猫实现了 `setupTrustingAI()` 方法来动态管理 AI：

```cpp
void OcelotEntity::setupTrustingAI() {
    // MC 1.16.5: OcelotEntity.func_213529_dV()
    if (m_avoidPlayerGoal == nullptr) {
        m_avoidPlayerGoal = new OcelotAvoidPlayerGoal(
            this, AVOID_DISTANCE, AVOID_FAR_SPEED, AVOID_NEAR_SPEED);
    }
    
    // 先移除已有的 AvoidPlayerGoal
    m_goalSelector.removeGoal(m_avoidPlayerGoal);
    
    // 如果未信任，添加避开玩家目标
    if (!m_trusting) {
        m_goalSelector.addGoal(4, m_avoidPlayerGoal);
    }
}
```

此方法在信任状态改变时调用，确保信任后豹猫不再逃避玩家。

### 特殊属性

| 属性 | 值 | 说明 |
|------|-----|------|
| 生命值 | 10.0 | MC 1.16.5 豹猫生命值 |
| 移动速度 | 0.3 | MC 1.16.5 豹猫移动速度 |
| 攻击伤害 | 3.0 | MC 1.16.5 豹猫攻击伤害 |
| 眼睛高度（成体） | 0.6 | 成年豹猫眼睛高度 |
| 眼睛高度（幼体） | 0.3 | 幼年豹猫眼睛高度 |
| 摔落伤害免疫 | 是 | 豹猫免疫摔落伤害 |
| 消失条件 | 信任后不消失 | 未信任豹猫 2400 tick 后可消失 |

## 猫实体系统（CatEntity）

猫是可驯服的猫科动物，具有多种皮肤和独特的行为模式。

### 驯服物品

猫使用**生鱼**驯服和繁殖：

| 物品 | 说明 |
|------|------|
| 生鳕鱼 (`COD`) | 可驯服、可繁殖 |
| 生鲑鱼 (`SALMON`) | 可驯服、可繁殖 |

### 皮肤类型

猫有 11 种皮肤类型（MC 1.16.5）：

| 类型 | 值 | 说明 |
|------|-----|------|
| Tabby | 0 | 虎斑猫 |
| Black | 1 | 黑猫 |
| Red | 2 | 红猫/姜黄猫 |
| Siamese | 3 | 暹罗猫 |
| BritishShorthair | 4 | 英国短毛猫 |
| Calico | 5 | 三花猫 |
| Persian | 6 | 波斯猫 |
| Ragdoll | 7 | 布偶猫 |
| White | 8 | 白猫 |
| Jellie | 9 | Jellie猫（社区投票） |
| AllBlack | 10 | 全黑猫（万圣节） |

### 内部 AI Goal 类

猫实现了两个内部 Goal 类来支持特有的行为：

#### CatTemptGoal

继承自 `TemptGoal`，只在未驯服时执行：

```cpp
class CatTemptGoal : public TemptGoal {
public:
    CatTemptGoal(CatEntity* cat, f64 speed, ItemPredicate itemPredicate, bool scaredByMovement);
    bool shouldExecute() override;  // 只在未驯服时返回 true
};
```

- **诱惑速度**: 0.6（比其他动物慢）
- **诱惑物品**: COD、SALMON
- **scaredByMovement**: true（会被玩家快速移动吓跑）

#### CatAvoidPlayerGoal

继承自 `AvoidEntityGoal`，只在未驯服时执行：

```cpp
class CatAvoidPlayerGoal : public AvoidEntityGoal {
public:
    CatAvoidPlayerGoal(CatEntity* cat, f32 avoidDistance, f64 farSpeed, f64 nearSpeed);
    bool shouldExecute() override;           // 只在未驯服时返回 true
    bool shouldContinueExecuting() override; // 只在未驯服时返回 true
};
```

- **检测距离**: 16 格
- **远距离逃避速度**: 0.8
- **近距离逃避速度**: 1.33（更快）
- **动态管理**: 通过 `setupTamedAI()` 在驯服时移除

### AI 目标列表

| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 游泳（最高优先级） |
| 1 | PanicGoal | 恐慌逃跑 |
| 1 | SitGoal | 坐下（驯服后） |
| 2 | BreedGoal | 繁殖 |
| 3 | CatTemptGoal | 食物诱惑（未驯服） |
| 4 | CatAvoidPlayerGoal | 避开玩家（未驯服） |
| 5 | FollowParentGoal | 跟随父母 |
| 6 | FollowOwnerGoal | 跟随主人（驯服后） |
| 10 | WaterAvoidingRandomWalkingGoal | 避水随机漫步 |
| 12 | LookAtGoal | 看向玩家 |
| 13 | LookRandomlyGoal | 随机看向 |

### 动态 AI 管理

猫实现了 `setupTamedAI()` 方法来动态管理 AI：

```cpp
void CatEntity::setupTamedAI()
{
    // 创建 AvoidPlayerGoal（如果尚未创建）
    if (m_avoidPlayerGoal == nullptr) {
        m_avoidPlayerGoal = new CatAvoidPlayerGoal(this, 16.0f, 0.8, 1.33);
    }
    
    // 先移除已有的 AvoidPlayerGoal
    m_goalSelector.removeGoal(m_avoidPlayerGoal);
    
    // 如果未驯服，添加避开玩家目标
    if (!isTamed()) {
        m_goalSelector.addGoal(4, m_avoidPlayerGoal);
    }
}
```

此方法在 `onTamed()` 回调中被调用，确保驯服后猫不再逃避玩家。

**参考**: `net.minecraft.entity.passive.CatEntity`

## AI目标

### 狼实体 (WolfEntity)

狼实体实现了完整的 AI 目标系统，包括行为目标和目标选择器：

#### 行为目标 (goalSelector)

| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 游泳 |
| 1 | PanicGoal | 恐慌逃跑 |
| 1 | SitGoal | 坐下（驯服后） |
| 2 | BreedGoal | 繁殖 |
| 3 | AvoidEntityGoal | 未驯服时避开羊驼（基于强度判定） |
| 4 | LeapAtTargetGoal | 跳跃攻击 |
| 5 | MeleeAttackGoal | 近战攻击 |
| 6 | FollowOwnerGoal | 跟随主人 |
| 4 | TemptGoal | 食物诱惑 |
| 5 | FollowParentGoal | 跟随父母 |
| 6 | WaterAvoidingRandomWalkingGoal | 避水随机行走 |
| 7 | LookAtGoal | 看向玩家 |
| 8 | LookRandomlyGoal | 随机看向 |
| 9 | BegGoal | 乞求食物 |

#### 目标选择器 (targetSelector)

| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | OwnerHurtByTargetGoal | 主人被攻击时反击 |
| 2 | OwnerHurtTargetGoal | 攻击主人正在攻击的目标 |
| 3 | HurtByTargetGoal(true) | 被攻击后反击并呼叫同伴 |
| 5 | NearestAttackableTargetGoal | 攻击羊/兔子/狐狸（未驯服时） |
| 6 | NonTamedTargetGoal<TurtleEntity> | 攻击幼海龟（未驯服时） |
| 7 | NearestAttackableTargetGoal | 攻击骷髅类怪物 |

#### 羊驼躲避逻辑

狼会根据羊驼的强度决定是否躲避：

```cpp
// 未驯服的狼会避开羊驼
// 羊驼强度 >= 随机值(0-4) 时，狼会躲避
// 强度1: 20%概率吓跑，强度4: 80%概率吓跑
math::Random rng = getRandom();
return llama->getStrength() >= rng.nextInt(5);
```

#### 驯服前后行为变化

| 行为 | 未驯服 | 驯服后 |
|------|--------|--------|
| 攻击羊/兔子/狐狸 | 是 | 否 |
| 攻击幼海龟 | 是 | 否 |
| 攻击骷髅类怪物 | 是 | 是 |
| 保护主人 | 否 | 是 |
| 跟随主人 | 否 | 是 |
| 坐下/站起 | 否 | 是 |
| 躲避羊驼 | 是 | 否 |
| 生命值 | 8 | 20 |
| 攻击力 | 2 | 4 |

**参考**: `net.minecraft.entity.passive.WolfEntity`
