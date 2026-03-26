# Living 模块

`src/common/entity/living` 目录包含生物实体（Living Entity）的核心实现，是所有有生命值实体的基类。

## 目录结构

```
src/common/entity/living/
├── LivingEntity.hpp    # 生物实体类声明
└── LivingEntity.cpp    # 生物实体类实现
```

## 文件详细介绍

### LivingEntity.hpp

**职责**：定义 `LivingEntity` 类，作为所有有生命值实体（玩家、怪物、动物等）的基类。

**主要内容**：

#### 枚举类型
- `EquipmentSlot`：装备槽位枚举，包含 6 个槽位
  - `MainHand`：主手
  - `OffHand`：副手
  - `Feet`：靴子
  - `Legs`：护腿
  - `Chest`：胸甲
  - `Head`：头盔

#### 核心属性
- **生命值系统**：`m_health`、`m_maxHealth`、`m_absorption`（吸收值）
- **属性系统**：`AttributeMap m_attributes` 管理实体的所有属性
- **装备系统**：`std::array<ItemStack, 6> m_equipment` 存储所有装备
- **受伤无敌帧**：`m_hurtTime`、`m_maxHurtTime`、`m_lastDamage`、`m_lastDamageSource`
- **死亡处理**：`m_deathTime` 死亡动画计时
- **渲染插值属性**：
  - 步态动画：`m_limbSwing`、`m_limbSwingAmount`
  - 攻击动画：`m_swingProgress`、`m_swingInProgress`
  - 身体旋转：`m_renderYawOffset`、`m_rotationYawHead`
- **跳跃系统**：`m_isJumping`、`m_jumpTicks`、`m_jumpUpwardsMotion`
- **移动系统**：`m_moveStrafing`、`m_moveForward`、`m_landMovementFactor`

#### 主要方法
| 方法 | 功能 |
|------|------|
| `health()` / `setHealth()` | 获取/设置当前生命值 |
| `maxHealth()` | 获取最大生命值（从属性读取） |
| `heal()` | 治疗实体 |
| `hurt()` | 受伤处理（含无敌帧检查） |
| `die()` | 死亡处理 |
| `getAttributeValue()` / `setAttributeBaseValue()` | 属性操作 |
| `getEquipment()` / `setEquipment()` | 装备操作 |
| `jump()` | 执行跳跃 |
| `travel()` | 核心物理移动逻辑 |
| `aiStep()` | AI 步进更新 |
| `tick()` | 主刻更新 |
| `handleFallDamage()` | 摔落伤害处理 |

### LivingEntity.cpp

**职责**：实现 `LivingEntity` 类的所有方法。

**主要实现细节**：

#### 数据参数注册
```cpp
// 注册同步参数
LIVING_FLAGS_PARAM   (i8)   // 生物标志
HEALTH_PARAM         (f32)  // 当前生命值
POTION_EFFECTS_PARAM (i32)  // 药水效果颜色
ARROW_COUNT_PARAM    (i32)  // 箭矢数量
```

#### 默认属性注册
- `MAX_HEALTH`：最大生命值（默认 20.0）
- `KNOCKBACK_RESISTANCE`：击退抗性（默认 0.0）
- `MOVEMENT_SPEED`：移动速度（默认 0.7）
- `ARMOR`：护甲值（默认 0.0）
- `ARMOR_TOUGHNESS`：护甲韧性（默认 0.0）

#### hurt() 方法流程
1. 检查无敌帧，若在无敌帧期间则返回 false
2. 吸收值优先扣除（金苹果效果）
3. 扣除生命值，设置无敌帧
4. 记录伤害到 `CombatTracker`
5. 保存伤害来源用于死亡消息

#### travel() 方法（核心移动逻辑）
遵循 MC 1.16.5 的物理移动公式：
1. 获取移动速度属性
2. 根据地面/空中选择移动因子
3. 计算移动向量（根据实体偏航角）
4. 应用重力和空气阻力
5. 执行碰撞移动
6. 应用地面摩擦

#### jump() 方法
- 设置垂直速度为 `m_jumpUpwardsMotion`（默认 0.42，MC 标准值）
- 设置 `m_onGround = false`

## 模块关系

```
                    ┌─────────────┐
                    │   Entity    │ (基类)
                    └──────┬──────┘
                           │ 继承
                    ┌──────▼──────┐
                    │ LivingEntity│
                    └──────┬──────┘
                           │
         ┌─────────────────┼─────────────────┐
         │                 │                 │
   ┌─────▼─────┐    ┌─────▼─────┐    ┌─────▼─────┐
   │   Mob     │    │  Player   │    │  (其他)   │
   └───────────┘    └───────────┘    └───────────┘
```

### 依赖关系

**直接依赖**：
| 模块 | 用途 |
|------|------|
| `../Entity.hpp` | 基类，提供位置、速度、碰撞等基础功能 |
| `../attribute/AttributeMap.hpp` | 属性管理 |
| `../attribute/Attributes.hpp` | 标准属性定义 |
| `../damage/DamageSource.hpp` | 伤害来源 |
| `../damage/CombatTracker.hpp` | 战斗追踪 |
| `../../item/ItemStack.hpp` | 装备物品堆 |
| `../../physics/PhysicsConstants.hpp` | 物理常量（重力、阻力等） |
| `../../util/math/MathUtils.hpp` | 数学工具 |

**被依赖**：
- `entity/mob/Mob.hpp` - 怪物基类
- `entity/animal/` - 动物实体
- `server/player/ServerPlayer.hpp` - 服务端玩家
- `client/player/ClientPlayer.hpp` - 客户端玩家

## 模块整体职责

`LivingEntity` 是所有"有生命"实体的中间抽象层，介于基础 `Entity` 类和具体实体类型（玩家、怪物、动物）之间。它扩展了基础实体功能，添加了：

1. **生命值系统**：生命值管理、伤害处理、死亡逻辑
2. **属性系统**：通过 `AttributeMap` 管理实体的各种数值属性
3. **装备系统**：管理主手、副手和防具栏的装备
4. **战斗追踪**：通过 `CombatTracker` 记录战斗历史，用于死亡消息
5. **动画系统**：步态、攻击、身体旋转等渲染插值属性
6. **移动系统**：`travel()` 和 `aiStep()` 实现 AI 驱动的物理移动

## 输入和输出

### 输入
- **构造参数**：`LegacyEntityType`、`EntityId`、`IWorld*`
- **属性修改**：通过 `AttributeMap` 修改属性值和添加修改器
- **伤害输入**：`hurt(DamageSource&, f32)` 接受伤害
- **治疗输入**：`heal(f32)` 接受治疗量
- **装备输入**：`setEquipment()` 设置装备
- **移动指令**：`setMoveStrafing()` / `setMoveForward()` 设置移动方向

### 输出
- **生命值状态**：`health()`、`maxHealth()`、`isDead()`、`isDying()`
- **属性值**：`getAttributeValue()` 获取计算后的属性值
- **装备状态**：`getEquipment()` 获取装备
- **动画数据**：`limbSwing()`、`swingProgress()` 等渲染插值属性
- **战斗数据**：`combatTracker()` 获取战斗记录

## 使用方法

### 创建自定义生物实体

```cpp
class CustomMob : public LivingEntity {
public:
    CustomMob(EntityId id, IWorld* world)
        : LivingEntity(LegacyEntityType::Mob, id, world)
    {
        // 注册自定义属性
        registerAttributes();

        // 设置自定义最大生命值
        setAttributeBaseValue(Attributes::MAX_HEALTH, 50.0);
        setHealth(50.0f);
    }

    // 重写受伤逻辑
    bool hurt(DamageSource& source, f32 amount) override {
        // 自定义伤害计算
        return LivingEntity::hurt(source, amount);
    }

    // 重写死亡逻辑
    void die(DamageSource& cause) override {
        // 自定义死亡处理（掉落物品等）
        LivingEntity::die(cause);
    }
};
```

### 属性修改

```cpp
// 获取属性值
f32 speed = entity.getAttributeValue(Attributes::MOVEMENT_SPEED, 0.7);

// 设置基础值
entity.setAttributeBaseValue(Attributes::MAX_HEALTH, 30.0);

// 添加修改器（如药水效果）
entity.attributes().addModifier(
    Attributes::MOVEMENT_SPEED,
    AttributeModifier("speed-boost", "Speed Boost", 0.2, Operation::MultiplyBase)
);
```

### 伤害处理

```cpp
// 创建伤害来源
EnvironmentalDamage fireDamage(DamageType::OnFire);
EntityDamageSource attack(DamageType::PlayerAttack, attacker);

// 造成伤害
entity.hurt(fireDamage, 5.0f);

// 检查死亡
if (entity.isDead()) {
    entity.die(fireDamage);
}
```

## 容易踩的坑

### 1. 属性注册顺序
**问题**：在构造函数中调用 `registerAttributes()` 之前访问属性会失败。
**解决**：确保在构造函数初始化列表中完成基类构造后立即调用属性注册。

```cpp
// 正确
CustomMob::CustomMob() : LivingEntity(...) {
    registerAttributes();  // 在访问属性之前
    setHealth(maxHealth());
}

// 错误
CustomMob::CustomMob() : LivingEntity(...) {
    setHealth(maxHealth());  // maxHealth() 返回默认值 20，因为属性未注册
    registerAttributes();
}
```

### 2. 生命值边界处理
**问题**：`setHealth()` 会自动限制在 `[0, maxHealth]` 范围内，但 `maxHealth()` 依赖属性。
**解决**：确保在设置生命值前已设置最大生命值属性。

### 3. 无敌帧机制
**问题**：连续调用 `hurt()` 可能不会造成伤害。
**解决**：检查 `hurtTime()` 是否为 0，或在调用后检查返回值。

```cpp
if (entity.hurt(damage, 5.0f)) {
    // 伤害成功
} else {
    // 实体处于无敌帧
}
```

### 4. travel() 坐标系
**问题**：MC 使用左手坐标系，yaw=0 看向 +Z，yaw=90 看向 -X。
**解决**：`travel()` 方法已处理坐标系转换，直接传入移动方向即可。

### 5. 装备槽位索引
**问题**：`EquipmentSlot` 枚举值与数组索引对应，但顺序不直观。
**解决**：使用 `getMainHandItem()` / `setMainHandItem()` 等便捷方法，避免手动计算索引。

### 6. 死亡处理时机
**问题**：`hurt()` 只是扣血，不会自动调用 `die()`。
**解决**：在游戏循环中检查 `isDead()` 并调用 `die()`，或在 `hurt()` 返回后检查。

```cpp
// 在 tick() 中检查
void tick() override {
    LivingEntity::tick();
    if (isDead() && !isDying()) {
        DamageSource* cause = lastDamageSource();
        if (cause) {
            die(*cause);
        }
    }
}
```

### 7. 渲染属性插值
**问题**：`prevXxx` 属性在 `tick()` 开始时保存，在 `tick()` 期间会被修改。
**解决**：渲染时使用 `prevXxx` 和 `xxx` 进行插值，不要在 `tick()` 中间读取。

```cpp
// 渲染时的正确插值
f32 partialTicks = getPartialTicks();
f32 currentSwing = entity.swingProgress();
f32 prevSwing = entity.prevSwingProgress();
f32 interpolatedSwing = prevSwing + (currentSwing - prevSwing) * partialTicks;
```

## 测试用例

测试文件位于 `tests/entity/LivingEntityTests.cpp`，包含以下测试：

### 生命值测试
| 测试名称 | 验证内容 |
|----------|----------|
| `Construction` | 默认生命值为 20，最大生命值为 20 |
| `SetHealth` | 生命值边界限制（负值、超最大值） |
| `Heal` | 治疗功能，死亡实体不能治疗 |
| `Hurt` | 伤害扣血功能 |
| `HurtInvulnerability` | 受伤后无敌帧机制 |
| `Death` | 死亡状态和死亡动画 |
| `IsDead` | 死亡判定 |

### 属性测试
| 测试名称 | 验证内容 |
|----------|----------|
| `DefaultAttributes` | 默认注册的属性（生命、速度、护甲等） |
| `GetAttributeValue` | 获取属性值，包括默认值 |
| `SetAttributeBaseValue` | 设置属性基础值 |
| `AttributeModifier` | 属性修改器效果 |

### 装备测试
| 测试名称 | 验证内容 |
|----------|----------|
| `EquipmentSlots` | 所有装备槽位功能 |

### 无敌帧测试
| 测试名称 | 验证内容 |
|----------|----------|
| `HurtTime` | 受伤后无敌时间 |
| `HurtTimeDecreases` | 无敌帧随 tick 递减 |

### 伤害来源测试
| 测试名称 | 验证内容 |
|----------|----------|
| `EnvironmentalDamage` | 环境伤害类型判断 |
| `EntityDamage` | 实体伤害类型判断 |
| `IndirectEntityDamage` | 间接实体伤害（投射物） |
| `DamageTypes` | 特定伤害类型判断（摔落、溺水、饥饿） |
| `BypassesArmor` | 护甲穿透判断 |
| `DeathMessageKeys` | 死亡消息键 |
| `DamageSourcesFactory` | 伤害来源工厂方法 |

## 参考

本模块基于 Minecraft Java Edition 1.16.5 的 `net.minecraft.entity.LivingEntity` 类实现。

主要差异：
1. C++ 不支持多重继承的默认实现，使用组合模式替代部分功能
2. 药水效果系统尚未实现（标记为 TODO）
3. 部分复杂的移动物理逻辑进行了简化
