# 下界怪物模块

本目录包含下界维度中的怪物实体实现。

## 目录结构

```
src/common/entity/entities/monster/nether/
├── BlazeEntity.hpp/cpp      # 烈焰人
├── NetherEntities.hpp/cpp   # 其他下界怪物（恶魂、岩浆怪、猪灵等）
└── README.md
```

## 怪物列表

### BlazeEntity（烈焰人）

生活在下界的火焰怪物，可以飞行并发射火球。

**继承层次**：
```
Entity → LivingEntity → MobEntity → MonsterEntity → BlazeEntity
                                            ↓
                                    IRangedAttackMob（接口）
```

**特性**：
- **飞行能力**：可以飞行并悬浮
- **火球攻击**：发射小火球攻击目标
- **火焰免疫**：免疫火焰伤害
- **弱水性**：接触水会受伤
- **发光效果**：亮度始终为 1.0

**核心属性**（MC 1.16.5）：
| 属性 | 值 |
|------|-----|
| 生命值 | 20.0 |
| 移动速度 | 0.23 |
| 攻击伤害 | 6.0 |
| 追踪范围 | 48.0 |
| 经验值 | 10 |

**AI 目标系统**（MC 1.16.5）：
| 优先级 | 目标类型 | 目标类 | 说明 |
|--------|----------|--------|------|
| 4 | 攻击 | BlazeFireballAttackGoal | 火球攻击 |
| 7 | 移动 | WaterAvoidingRandomWalkingGoal | 避水随机行走 |
| 8 | 看向 | LookAtGoal | 看向玩家 |
| 8 | 看向 | LookRandomlyGoal | 随机看向 |

**目标选择器**：
| 优先级 | 目标类型 | 目标类 | 说明 |
|--------|----------|--------|------|
| 1 | 反击 | HurtByTargetGoal | 被攻击后反击，呼唤同伴 |
| 2 | 目标选择 | NearestAttackableTargetGoal<Player> | 攻击玩家 |

### GhastEntity（恶魂）

下界的飞行敌对生物，发射火球。

**继承层次**：
```
Entity → LivingEntity → MobEntity → MonsterEntity → GhastEntity
```

**特性**：
- **飞行能力**：在下界中自由飞行
- **火球攻击**：发射爆炸火球
- **极远追踪**：追踪范围 100 格

**核心属性**（MC 1.16.5）：
| 属性 | 值 |
|------|-----|
| 生命值 | 10.0 |
| 移动速度 | 0.0 |
| 飞行速度 | 0.9 |
| 追踪范围 | 100.0 |
| 火球爆炸威力 | 1 |

**AI 目标系统**：
| 优先级 | 目标类型 | 目标类 | 说明 |
|--------|----------|--------|------|
| 5 | 移动 | GhastRandomFlyGoal | 随机飞行 |
| 7 | 看向 | GhastLookAroundGoal | 环顾四周 |
| 7 | 攻击 | GhastFireballAttackGoal | 火球攻击 |

### MagmaCubeEntity（岩浆怪）

下界的史莱姆变种，继承自 SlimeEntity。

**继承层次**：
```
Entity → LivingEntity → MobEntity → CreatureEntity → MonsterEntity → SlimeEntity → MagmaCubeEntity
```

**与史莱姆的差异**：

| 特性 | 史莱姆 | 岩浆怪 |
|------|--------|--------|
| 跳跃延迟 | 10-30 tick | 40-120 tick（4倍） |
| 挤压动画衰减 | 0.6 | 0.9（更慢） |
| 护甲 | 0 | size * 3 |
| 攻击伤害 | size | 属性值 + 2.0 |
| 小型伤害玩家 | ❌ 否 | ✅ 是 |
| 着地粒子 | ItemSlime | Flame |
| 火焰免疫 | ❌ 否 | ✅ 是 |

**重写的虚函数**：

| 虚函数 | 说明 | 岩浆怪实现 |
|--------|------|------------|
| `setSlimeSize(size, resetHealth)` | 设置尺寸 | 调用父类 + 设置护甲 = size * 3 |
| `canDamagePlayer()` | 是否可伤害玩家 | `world() != nullptr && !world()->isClientSide()` |
| `getJumpDelay()` | 获取跳跃延迟 | 父类值 * 4 |
| `alterSquishAmount()` | 更新挤压动画 | `squishAmount *= 0.9f` |
| `getSquishParticle()` | 获取着地粒子类型 | `ParticleTypeId::Flame` |
| `getHurtSound(source)` | 获取受伤声音 | ENTITY_MAGMA_CUBE_HURT[_SMALL] |
| `getDeathSound()` | 获取死亡声音 | ENTITY_MAGMA_CUBE_DEATH[_SMALL] |
| `getSquishSound()` | 获取挤压声音 | ENTITY_MAGMA_CUBE_SQUISH[_SMALL] |
| `getJumpSound()` | 获取跳跃声音 | ENTITY_MAGMA_CUBE_JUMP |
| `isImmuneToFire()` | 火焰免疫 | `true` |
| `registerAttributes()` | 注册属性 | 调用父类 + 注册护甲属性 |

**尺寸与属性对应**（MC 1.16.5）：

| 尺寸 | 生命值 | 护甲 | 移动速度 | 攻击伤害 |
|------|--------|------|----------|----------|
| 1 | 1 | 3 | 0.3 | 3 |
| 2 | 4 | 6 | 0.4 | 4 |
| 4 | 16 | 12 | 0.6 | 6 |

**实现示例**：

```cpp
// 创建岩浆怪
auto magmaCube = std::make_unique<MagmaCubeEntity>(LegacyEntityType::MagmaCube, EntityId(0));
magmaCube->setWorld(&world);
magmaCube->setSlimeSize(4, true);  // 设置为大岩浆怪

// 检查属性
i32 size = magmaCube->getSlimeSize();        // 4
f64 armor = magmaCube->getAttributeValue(Attributes::ARMOR, 0.0);  // 12.0
bool immuneToFire = magmaCube->isImmuneToFire();  // true

// 小型岩浆怪也能伤害玩家
bool canDamage = magmaCube->canDamagePlayer();  // true（只要在服务端）
```

### AbstractPiglinEntity（猪灵基类）

猪灵和猪灵蛮兵的共同基类。

**继承层次**：
```
Entity → LivingEntity → MobEntity → MonsterEntity → AbstractPiglinEntity
```

**特性**：
- **火焰免疫**：默认火焰免疫
- **转化机制**：在主世界会转化为僵尸猪灵

### PiglinEntity（猪灵）

下界的敌对/中立生物，可进行交易。

**继承层次**：
```
Entity → LivingEntity → MobEntity → MonsterEntity → AbstractPiglinEntity → PiglinEntity
                                                                    ↓
                                                            ICrossbowUser（接口）
```

**核心属性**（MC 1.16.5）：
| 属性 | 值 |
|------|-----|
| 生命值 | 16.0 |
| 移动速度 | 0.35 |
| 攻击伤害 | 5.0 |

**特性**：
- 使用弩进行远程攻击
- 幼年猪灵不攻击玩家
- 对金装备有特殊兴趣

### PiglinBruteEntity（猪灵蛮兵）

下界堡垒的强力敌对生物。

**继承层次**：
```
Entity → LivingEntity → MobEntity → MonsterEntity → AbstractPiglinEntity → PiglinBruteEntity
```

**核心属性**（MC 1.16.5）：
| 属性 | 值 |
|------|-----|
| 生命值 | 50.0 |
| 移动速度 | 0.35 |
| 攻击伤害 | 7.0（金斧额外 +4） |

**特性**：
- 不使用弩，近战攻击
- 更高的生命值和攻击力

### ZombifiedPiglinEntity（僵尸猪灵）

下界的中立生物，被攻击后会激怒所有附近的僵尸猪灵。

**继承层次**：
```
Entity → LivingEntity → MobEntity → MonsterEntity → ZombifiedPiglinEntity
```

**核心属性**（MC 1.16.5）：
| 属性 | 值 |
|------|-----|
| 生命值 | 20.0 |
| 移动速度 | 0.23 |
| 攻击伤害 | 5.0 |

**特性**：
- **愤怒机制**：被攻击后激怒附近所有僵尸猪灵
- **火焰免疫**：免疫火焰伤害

### HoglinEntity（疣猪兽）

下界的敌对生物（成年）或中立生物（幼年）。

**继承层次**：
```
Entity → LivingEntity → MobEntity → MonsterEntity → HoglinEntity
                                            ↓
                                    IFlinging（接口）
```

**核心属性**（MC 1.16.5）：
| 属性 | 成年疣猪兽 | 幼年疣猪兽 |
|------|------------|------------|
| 生命值 | 40.0 | - |
| 移动速度 | 0.3 | - |
| 击退抗性 | 0.6 | - |
| 攻击击退 | 1.0 | - |
| 攻击伤害 | 6.0 | - |

**特性**：
- **击退攻击**：攻击时击退目标
- **攻击动画**：有攻击动画 ticks
- **火焰免疫**：免疫火焰伤害

### ZoglinEntity（僵尸疣兽）

疣猪兽在主世界的僵尸化变体。

**继承层次**：
```
Entity → LivingEntity → MobEntity → MonsterEntity → ZoglinEntity
                                            ↓
                                    IFlinging（接口）
```

**核心属性**（MC 1.16.5）：
| 属性 | 成年僵尸疣兽 | 幼年僵尸疣兽 |
|------|--------------|--------------|
| 生命值 | 40.0 | - |
| 移动速度 | 0.3 | - |
| 击退抗性 | 0.6 | - |
| 攻击击退 | 1.0 | - |
| 攻击伤害 | 6.0 | - |

**特性**：
- **击退攻击**：攻击时击退目标
- **敌对性**：攻击所有生物（不含亡灵）

## 实现状态

| 实体 | 基础结构 | 属性系统 | AI 目标 | 特殊行为 |
|------|----------|----------|---------|----------|
| 烈焰人 | ✅ | ✅ | ✅ | 🔄 火球攻击 |
| 恶魂 | ✅ | ✅ | ✅ | 🔄 火球攻击 |
| 岩浆怪 | ✅ | ✅ | ✅（继承） | ✅ |
| 猪灵 | ✅ | ✅ | 🔄 | 🔄 |
| 猪灵蛮兵 | ✅ | ✅ | 🔄 | 🔄 |
| 僵尸猪灵 | ✅ | ✅ | 🔄 | 🔄 |
| 疣猪兽 | ✅ | ✅ | 🔄 | 🔄 |
| 僵尸疣兽 | ✅ | ✅ | 🔄 | 🔄 |

## 依赖关系

```
MonsterEntity（基类）
    ├── attribute/Attributes.hpp    # 属性系统
    ├── damage/DamageSource.hpp     # 伤害系统
    ├── world/IWorld.hpp            # 世界接口
    ├── sound/SoundEvents.hpp       # 音效事件
    ├── util/math/Random.hpp        # 随机数生成
    ├── interfaces/IRangedAttackMob.hpp  # 远程攻击接口
    ├── interfaces/ICrossbowUser.hpp     # 弩使用者接口
    ├── interfaces/IFlinging.hpp         # 击退攻击接口
    └── basic/SlimeEntity.hpp        # 史莱姆基类（岩浆怪继承）
```

## 使用示例

```cpp
// 创建烈焰人
auto blaze = std::make_unique<BlazeEntity>(LegacyEntityType::Blaze, EntityId(0));
blaze->setWorld(&world);

// 创建岩浆怪
auto magmaCube = std::make_unique<MagmaCubeEntity>(LegacyEntityType::MagmaCube, EntityId(0));
magmaCube->setWorld(&world);
magmaCube->setSlimeSize(4, true);  // 设置为大岩浆怪

// 创建恶魂
auto ghast = std::make_unique<GhastEntity>(LegacyEntityType::Ghast, EntityId(0));
ghast->setWorld(&world);
ghast->setFireballStrength(1);  // 设置火球爆炸威力
```

## 容易踩的坑

### 1. 岩浆怪继承史莱姆

```cpp
// 错误：重复注册 AI 目标
void MagmaCubeEntity::registerGoals() {
    // SlimeEntity::registerGoals() 已经注册了史莱姆 AI
    // 不需要重新注册跳跃、攻击等目标
    MonsterEntity::registerGoals();  // 错误！应该调用 SlimeEntity::registerGoals()
}

// 正确：调用父类注册
void MagmaCubeEntity::registerGoals() {
    SlimeEntity::registerGoals();  // 复用史莱姆 AI
    // 如果需要添加岩浆怪特有 AI，在这里添加
}
```

### 2. 护甲属性注册

```cpp
// 错误：在 setSlimeSize 中注册护甲属性
void MagmaCubeEntity::setSlimeSize(i32 size, bool resetHealth) {
    SlimeEntity::setSlimeSize(size, resetHealth);
    // 护甲属性可能未注册
    m_attributes.setBaseValue(Attributes::ARMOR, size * 3);  // 可能崩溃！
}

// 正确：在 registerAttributes 中注册护甲属性
void MagmaCubeEntity::registerAttributes() {
    SlimeEntity::registerAttributes();
    // 先注册护甲属性
    m_attributes.registerAttribute(*Attributes::armor());
    m_attributes.setBaseValue(Attributes::ARMOR, 3.0);
}
```

### 3. isClientSide() const 问题

```cpp
// 错误：isClientSide() 不是 const 方法
bool MagmaCubeEntity::canDamagePlayer() const {
    return world() != nullptr && !world()->isClientSide();  // 编译错误！
}

// 正确：使用 const_cast
bool MagmaCubeEntity::canDamagePlayer() const {
    auto* nonConstWorld = const_cast<IWorld*>(world());
    return nonConstWorld != nullptr && !nonConstWorld->isClientSide();
}
```

## 测试用例

- `tests/common/entity/entities/monster/SlimeEntityTest.cpp` - 史莱姆测试（岩浆怪继承测试）
- `tests/common/entity/entities/monster/MagmaCubeEntityTest.cpp` - 岩浆怪专用测试
- `tests/common/entity/entities/monster/BlazeEntityTest.cpp` - 烈焰人测试

## 参考

- MC 1.16.5 `net.minecraft.entity.monster.BlazeEntity`
- MC 1.16.5 `net.minecraft.entity.monster.GhastEntity`
- MC 1.16.5 `net.minecraft.entity.monster.MagmaCubeEntity`
- MC 1.16.5 `net.minecraft.entity.monster.AbstractPiglinEntity`
- MC 1.16.5 `net.minecraft.entity.monster.PiglinEntity`
- MC 1.16.5 `net.minecraft.entity.monster.PiglinBruteEntity`
- MC 1.16.5 `net.minecraft.entity.monster.ZombifiedPiglinEntity`
- MC 1.16.5 `net.minecraft.entity.monster.HoglinEntity`
- MC 1.16.5 `net.minecraft.entity.monster.ZoglinEntity`
