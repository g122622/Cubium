# Combat 模块

战斗系统模块，提供攻击上下文管理、玩家攻击辅助、伤害计算规则和难度相关计算功能。

## 目录结构

```
src/common/entity/combat/
├── AttackContext.hpp      # 攻击上下文头文件
├── AttackContext.cpp      # 攻击上下文实现
├── CombatRules.hpp        # 战斗规则工具类头文件
├── CombatRules.cpp        # 战斗规则工具类实现
├── PlayerAttackHelper.hpp # 玩家攻击辅助类头文件
├── PlayerAttackHelper.cpp # 玩家攻击辅助类实现
├── DifficultyHelper.hpp   # 难度工具类头文件
└── DifficultyHelper.cpp   # 难度工具类实现
```

## 文件详解

### AttackContext.hpp / AttackContext.cpp

**职责**：封装攻击行为的完整上下文信息，作为攻击计算的参数容器和结果计算器。

**主要内容**：

#### AttackType 枚举

定义五种攻击类型：

| 枚举值 | 说明 |
|--------|------|
| `Melee` | 近战攻击 |
| `Ranged` | 远程攻击（箭矢、三叉戟等） |
| `Magic` | 魔法攻击 |
| `Explosion` | 爆炸攻击 |
| `Thorns` | 荆棘反伤 |

#### AttackContext 类

**成员变量**：

| 类别 | 变量 | 说明 |
|------|------|------|
| 攻击者信息 | `m_attacker` | 攻击者实体指针 |
| | `m_attackerPlayer` | 玩家类型的攻击者 |
| | `m_attackerLiving` | 生物类型的攻击者 |
| | `m_weapon` | 使用的武器 |
| 目标信息 | `m_target` | 目标生物 |
| 攻击属性 | `m_baseDamage` | 基础伤害（默认 1.0） |
| | `m_attackType` | 攻击类型（默认 Melee） |
| 攻击修饰 | `m_critical` | 是否暴击 |
| | `m_criticalMultiplier` | 暴击倍率（默认 1.5） |
| | `m_knockback` | 是否造成击退 |
| | `m_knockbackStrength` | 击退强度 |
| | `m_fireDamage` | 是否造成火焰伤害 |
| | `m_fireDuration` | 火焰持续时间 |
| 攻击冷却 | `m_cooldownProgress` | 冷却进度（0-1，默认 1.0） |

**核心方法**：

```cpp
// 计算最终伤害
f32 calculateFinalDamage() const;

// 创建伤害来源对象
std::unique_ptr<DamageSource> createDamageSource() const;
```

**伤害计算流程**：
1. 从基础伤害开始
2. 如果是暴击，乘以暴击倍率
3. 如果冷却不足（< 1.0），乘以冷却进度
4. （TODO）附魔加成、药水效果、护甲减伤

**伤害来源创建**：
根据攻击类型创建对应的 `DamageSource`：
- `Melee` → `EntityDamageSource` (MobAttack/OnFire)
- `Ranged` → `IndirectEntityDamageSource` (Arrow)
- `Magic` → `EnvironmentalDamage` (Magic)
- `Explosion` → `EnvironmentalDamage` (Explosion)
- `Thorns` → `EnvironmentalDamage` (Thorns)

---

### CombatRules.hpp / CombatRules.cpp

**职责**：提供伤害计算相关的公式和工具方法，实现 MC 1.16.5 CombatRules 的精确计算逻辑。

**常量定义**：

| 常量 | 值 | 说明 |
|------|-----|------|
| `ARMOR_MAX_EFFECTIVE` | 20.0f | 有效护甲上限 |
| `ARMOR_MIN_RATIO` | 0.2f | 护甲最小比例（20%） |
| `ARMOR_DIVISOR` | 25.0f | 护甲减伤除数 |
| `TOUGHNESS_FACTOR` | 4.0f | 韧性因子 |
| `TOUGHNESS_BASE` | 2.0f | 韧性基数 |
| `EPF_MAX` | 20.0f | EPF 上限（80% 减伤） |
| `RESISTANCE_FACTOR` | 0.2f | 抗性因子（每级 20% 减伤） |
| `RESISTANCE_MAX_LEVEL` | 5 | 抗性最大等级 |

**静态方法**：

#### 护甲减伤计算

```cpp
static f32 getDamageAfterAbsorb(f32 damage, f32 totalArmor, f32 toughness);
```

MC 1.16.5 公式：
```
f = 2 + toughness / 4
g = clamp(armor - damage / f, armor * 0.2, 20)
final = damage * (1 - g / 25)
```

护甲减伤上限为 80%（当 effectiveArmor = 20 时）。

#### 附魔保护减伤计算

```cpp
static f32 getDamageAfterMagicAbsorb(f32 damage, f32 enchantmentProtectionFactor);
```

MC 1.16.5 公式：
```
f = clamp(epf, 0, 20)
final = damage * (1 - f / 25)
```

附魔保护减伤上限为 80%（当 EPF = 20 时）。

#### 抗性药水减伤计算

```cpp
static f32 getDamageAfterResistance(f32 damage, i32 resistanceLevel);
```

MC 1.16.5 公式：
```
final = damage * max(0, 1 - level * 0.2)
```

抗性药水减伤上限为 80%（抗性 V）。

#### 吸收值消耗计算

```cpp
static std::pair<f32, f32> applyAbsorption(f32 damage, f32 absorption);
```

返回 `<消耗的吸收值, 剩余伤害>`。

**伤害计算顺序**（MC 1.16.5）：
1. 盾牌格挡
2. 无敌帧检查
3. 护甲减伤 → `getDamageAfterAbsorb()`
4. 药水/附魔减伤 → `getDamageAfterResistance()` + `getDamageAfterMagicAbsorb()`
5. 吸收值消耗 → `applyAbsorption()`
6. 实际扣血

---

### PlayerAttackHelper.hpp / PlayerAttackHelper.cpp

**职责**：提供玩家攻击相关的静态辅助函数，实现 MC 1.16.5 的攻击机制。

**常量定义**：

| 常量 | 值 | 说明 |
|------|-----|------|
| `CRITICAL_MULTIPLIER` | 1.5f | 暴击伤害倍率 |
| `SPRINT_KNOCKBACK_BONUS` | 0.5f | 疾跑击退加成 |
| `FIRE_ASPECT_DURATION` | 80 (4秒) | 火焰附加基础持续时间 |
| `MIN_COOLDOWN_THRESHOLD` | 0.9f | 最小冷却阈值 |

**静态方法**：

#### 暴击判定

```cpp
static bool isCriticalHit(const PlayerEntity& player);
```

暴击条件（MC 1.16.5）：
1. 玩家正在下落（垂直速度 < 0）
2. 玩家不在地面
3. 玩家不在水中
4. 玩家不在梯子/藤蔓上
5. 玩家没有失明效果
6. 玩家没有骑乘

#### 伤害计算

```cpp
static f32 calculateDamage(const PlayerEntity& player, f32 baseDamage, f32 cooldownProgress);
```

计算流程：
1. 应用攻击冷却影响
2. （TODO）附魔加成（锋利、亡灵杀手、节肢杀手）
3. （TODO）药水效果加成（力量、虚弱）
4. （TODO）目标护甲减伤

#### 击退计算

```cpp
static f32 calculateKnockback(const LivingEntity& attacker,
                               const LivingEntity& target,
                               f32 baseKnockback = 1.0f,
                               bool isSprinting = false);
```

击退计算：
- 基础击退 + 疾跑加成（+0.5）
- （TODO）击退附魔加成
- （TODO）目标击退抗性减伤

#### 攻击冷却

```cpp
static f32 applyCooldown(f32 damage, f32 cooldownProgress);
static bool isCooldownReady(f32 cooldownProgress, f32 threshold = 0.9f);
static f32 getCooldownProgress(i32 ticksSinceLastAttack, f32 attackSpeed);
```

冷却机制：
- 冷却进度 < 0.9 时，伤害 = 原伤害 × 冷却进度²
- 攻击间隔 = 20 / 攻击速度（tick）
- 冷却进度 = 已过时间 / 攻击间隔

#### 火焰附加

```cpp
static bool applyFireAspect(LivingEntity& target, i32 fireAspectLevel);
```

火焰持续时间 = 80 × 附魔等级（tick）

#### 创建攻击上下文

```cpp
static AttackContext createContext(PlayerEntity& player,
                                    LivingEntity& target,
                                    f32 cooldownProgress);
```

工厂方法，自动配置：
- 攻击者信息
- 攻击冷却
- 暴击判定
- （TODO）基础伤害（从武器）
- （TODO）火焰附加

---

### DifficultyHelper.hpp / DifficultyHelper.cpp

**职责**: 提供难度相关的游戏机制计算，实现 MC 1.16.5 难度系统。

**常量定义**:

| 常量 | 值 | 说明 |
|------|-----|------|
| `EASY_PLAYER_DAMAGE_MULT` | 0.5f | 简单模式玩家受伤倍率 |
| `NORMAL_PLAYER_DAMAGE_MULT` | 1.0f | 普通模式玩家受伤倍率 |
| `HARD_PLAYER_DAMAGE_MULT` | 1.5f | 困难模式玩家受伤倍率 |
| `EASY_MOB_DAMAGE_ADJ` | -2.0f | 简单模式怪物伤害调整 |
| `NORMAL_MOB_DAMAGE_ADJ` | 0.0f | 普通模式怪物伤害调整 |
| `HARD_MOB_DAMAGE_ADJ` | 2.0f | 困难模式怪物伤害调整 |
| `EASY_STARVATION_MIN` | 10.0f | 简单模式饥饿最小生命值 |
| `NORMAL_STARVATION_MIN` | 1.0f | 普通模式饥饿最小生命值 |
| `HARD_STARVATION_MIN` | 0.0f | 困难模式饥饿最小生命值 |

**静态方法**:

#### 玩家伤害倍率

```cpp
static f32 getPlayerDamageMultiplier(Difficulty difficulty);
```

返回玩家受到怪物伤害的倍率：
- Peaceful: 0.0（和平模式玩家不受怪物伤害）
- Easy: 0.5（伤害减半）
- Normal: 1.0（无调整）
- Hard: 1.5（伤害增加 50%）

#### 怪物伤害调整

```cpp
static f32 getMobDamageAdjustment(Difficulty difficulty);
static f32 getMobDamageAdjustment(i32 difficultyId);
```

返回怪物攻击伤害的调整值：
- Peaceful: 0.0（和平模式怪物不攻击）
- Easy: -2.0（伤害减少 2）
- Normal: 0.0（无调整）
- Hard: +2.0（伤害增加 2）

#### 饥饿最小生命值

```cpp
static f32 getStarvationMinHealth(Difficulty difficulty);
```

返回饥饿伤害不能降至以下的最小生命值：
- Peaceful: 20.0（最大生命值，不受饥饿伤害）
- Easy: 10.0（5 颗心）
- Normal: 1.0（半颗心）
- Hard: 0.0（可饿死）

#### 火焰机制

```cpp
static f32 getFireDurationMultiplier(Difficulty difficulty);
static i32 getFireSpreadBonus(Difficulty difficulty);
```

- `getFireDurationMultiplier()`: 火焰燃烧持续时间倍率
- `getFireSpreadBonus()`: 火焰蔓延概率加成（difficulty * 7）

#### 特殊机制

```cpp
static bool canZombieReinforce(Difficulty difficulty);
static f32 getVillagerInfectionChance(Difficulty difficulty);
static i32 getRaidWaves(Difficulty difficulty);
static bool allowsMobSpawning(Difficulty difficulty);
static f32 getRegionalDifficultyBase(Difficulty difficulty);
```

- `canZombieReinforce()`: 只有困难模式僵尸才能召唤增援
- `getVillagerInfectionChance()`: 村民被僵尸杀死时的感染概率（Easy/Peaceful: 0%, Normal: 50%, Hard: 100%）
- `getRaidWaves()`: 袭击波次数（Peaceful: 0, Easy: 3, Normal: 5, Hard: 7）
- `allowsMobSpawning()`: 和平模式不允许怪物生成
- `getRegionalDifficultyBase()`: 区域难度基值

---

## 模块关系图

```
                    ┌─────────────────┐
                    │   PlayerEntity  │
                    │   LivingEntity  │
                    │      Entity     │
                    └────────┬────────┘
                             │
                             ▼
┌────────────────────────────────────────────────────────┐
│                    combat 模块                          │
│  ┌─────────────────────┐  ┌─────────────────────────┐  │
│  │   AttackContext     │◄─│  PlayerAttackHelper     │  │
│  │   - 攻击上下文       │  │  - 暴击判定              │  │
│  │   - 伤害计算         │  │  - 伤害计算              │  │
│  │   - DamageSource创建 │  │  - 击退计算              │  │
│  └─────────────────────┘  │  - 冷却管理              │  │
│           │               │  - 火焰附加              │  │
│           │               └─────────────────────────┘  │
│           ▼                                             │
│  ┌─────────────────────┐                               │
│  │    DamageSource     │                               │
│  │    (damage 模块)     │                               │
│  └─────────────────────┘                               │
└────────────────────────────────────────────────────────┘
```

---

## 整体职责

combat 模块负责：

1. **攻击上下文管理**
   - 封装攻击的所有相关信息
   - 提供统一的攻击参数访问接口
   - 支持多种攻击类型

2. **伤害计算**
   - 基础伤害计算
   - 暴击伤害加成
   - 攻击冷却影响

3. **玩家攻击机制**
   - 暴击判定（跳跃攻击）
   - 攻击冷却系统
   - 击退计算
   - 火焰附加应用

4. **伤害来源创建**
   - 根据攻击类型创建对应的 DamageSource
   - 支持直接和间接实体伤害

---

## 输入和输出

### 输入

| 输入 | 来源 | 说明 |
|------|------|------|
| 攻击者实体 | Entity* | 可以是玩家、生物或 null（环境伤害） |
| 目标实体 | LivingEntity* | 被攻击的生物 |
| 基础伤害 | f32 | 武器基础伤害值 |
| 攻击冷却进度 | f32 | 当前攻击冷却（0-1） |
| 攻击类型 | AttackType | 近战/远程/魔法等 |
| 玩家状态 | PlayerEntity& | 下落、疾跑、骑乘等状态 |

### 输出

| 输出 | 类型 | 说明 |
|------|------|------|
| 最终伤害 | f32 | 经过所有修正后的伤害值 |
| DamageSource | std::unique_ptr | 用于 ApplyDamage 的伤害来源对象 |
| 击退向量 | void (副作用) | 直接修改目标速度 |

---

## 依赖项

### 内部依赖

| 模块 | 依赖内容 |
|------|----------|
| `common/core/Types.hpp` | 基础类型定义（f32, i32, u8 等） |
| `common/entity/damage/DamageSource.hpp` | DamageSource 及其子类 |
| `common/util/math/MathUtils.hpp` | math::clamp 函数 |

### 外部依赖（前向声明）

| 类 | 用途 |
|----|------|
| `Entity` | 攻击者实体基类 |
| `LivingEntity` | 目标实体，具有生命值 |
| `PlayerEntity` | 玩家实体，提供状态查询 |
| `ItemStack` | 武器物品，获取基础伤害和附魔 |

---

## 使用方法

### 基本攻击流程

```cpp
#include "entity/combat/PlayerAttackHelper.hpp"
#include "entity/combat/AttackContext.hpp"

// 在玩家攻击处理中
void onPlayerAttack(PlayerEntity& player, LivingEntity& target) {
    // 1. 获取攻击冷却
    f32 cooldownProgress = PlayerAttackHelper::getCooldownProgress(
        player.getTicksSinceLastAttack(),
        player.getAttackSpeed()
    );

    // 2. 创建攻击上下文
    AttackContext context = PlayerAttackHelper::createContext(
        player,
        target,
        cooldownProgress
    );

    // 3. 设置武器基础伤害
    context.setBaseDamage(player.getHeldItem().getAttackDamage());

    // 4. 计算最终伤害
    f32 damage = context.calculateFinalDamage();

    // 5. 创建伤害来源并应用伤害
    auto damageSource = context.createDamageSource();
    target.hurt(*damageSource, damage);

    // 6. 应用击退
    if (context.causesKnockback()) {
        PlayerAttackHelper::applyKnockback(
            target,
            player,
            context.getKnockbackStrength()
        );
    }

    // 7. 应用火焰附加
    if (context.causesFireDamage()) {
        target.setFire(context.getFireDuration());
    }
}
```

### 手动创建攻击上下文

```cpp
// 生物攻击
AttackContext context(mobEntity, target);
context.setBaseDamage(5.0f);
context.setAttackType(AttackType::Melee);

// 箭矢攻击
AttackContext context(arrowShooter, target);
context.setBaseDamage(4.0f);
context.setAttackType(AttackType::Ranged);

// 爆炸伤害
AttackContext context(nullptr, target);  // 无攻击者
context.setBaseDamage(10.0f);
context.setAttackType(AttackType::Explosion);
```

### 暴击判定

```cpp
if (PlayerAttackHelper::isCriticalHit(player)) {
    // 触发暴击效果
    context.setCritical(true);
    // 暴击倍率默认 1.5，可自定义
    context.setCriticalMultiplier(1.5f);
}
```

### 攻击冷却

```cpp
// 检查是否可以造成完整伤害
if (PlayerAttackHelper::isCooldownReady(cooldownProgress)) {
    // 完整伤害
} else {
    // 冷却不足时伤害降低
    f32 reducedDamage = PlayerAttackHelper::applyCooldown(damage, cooldownProgress);
}
```

---

## 容易踩的坑

### 1. 空指针攻击者

**问题**：`AttackContext` 的 `attacker` 可以为 `nullptr`（环境伤害）。

**解决方案**：
```cpp
// 在 createDamageSource 中检查
if (m_attacker) {
    return std::make_unique<EntityDamageSource>(...);
}
// 返回通用伤害
return std::make_unique<EnvironmentalDamage>(DamageType::Generic);
```

### 2. 攻击冷却计算

**问题**：冷却不足时伤害按平方衰减（MC 1.9+ 机制）。

**注意**：
```cpp
// 冷却影响的计算是 progress²
damage *= cooldownProgress * cooldownProgress;  // 非线性衰减
```

### 3. 暴击条件

**问题**：暴击需要满足多个条件，目前实现不完整。

**当前状态**：
- `isCriticalHit()` 暂时返回 `false`
- 等待 `PlayerEntity` 实现相关方法：
  - `isOnGround()`
  - `isInWater()`
  - `isOnLadder()`
  - `hasEffect(Effects::BLINDNESS)`
  - `isPassenger()`

### 4. 击退方向

**问题**：`applyKnockback()` 目前未实现。

**需要实现**：
- 根据攻击者朝向计算击退方向
- 根据强度设置目标速度
- 考虑目标的击退抗性

### 5. 伤害来源类型匹配

**问题**：`createDamageSource()` 根据攻击类型创建 DamageSource，但需要正确设置 `m_fireDamage` 标志。

**正确用法**：
```cpp
context.setFireDamage(true);  // 会创建 OnFire 类型的 DamageSource
```

### 6. 前向声明类型转换

**问题**：`createContext` 中使用 `void*` 转换。

**代码**：
```cpp
AttackContext context(static_cast<Entity*>(static_cast<void*>(&player)), &target);
```

**说明**：这是由于前向声明限制，未来应改进为使用 proper 类型系统。

---

## 待实现功能

| 功能 | 状态 | 说明 |
|------|------|------|
| 暴击判定完整实现 | TODO | 需要 PlayerEntity 状态查询 |
| 附魔伤害加成 | TODO | 锋利、亡灵杀手、节肢杀手 |
| 药水效果加成 | TODO | 力量、虚弱效果 |
| 护甲减伤 | TODO | 目标护甲和韧性计算 |
| 击退附魔 | TODO | 击退 I/II 加成 |
| 击退抗性 | TODO | 目标的击退抗性属性 |
| 完整击退逻辑 | TODO | 方向和速度计算 |
| 火焰附加应用 | TODO | 需要目标 setFire 方法 |
| 武器伤害获取 | TODO | 从 ItemStack 获取基础伤害 |

---

## 测试用例

**当前状态**：无测试文件

**建议测试覆盖**：

```cpp
// tests/common/entity/combat/AttackContextTest.cpp

// 1. 基本伤害计算
TEST(AttackContextTest, BasicDamageCalculation) {
    AttackContext context(nullptr, nullptr);
    context.setBaseDamage(10.0f);
    EXPECT_FLOAT_EQ(context.calculateFinalDamage(), 10.0f);
}

// 2. 暴击伤害
TEST(AttackContextTest, CriticalHitMultiplier) {
    AttackContext context(nullptr, nullptr);
    context.setBaseDamage(10.0f);
    context.setCritical(true);
    context.setCriticalMultiplier(1.5f);
    EXPECT_FLOAT_EQ(context.calculateFinalDamage(), 15.0f);
}

// 3. 攻击冷却影响
TEST(AttackContextTest, CooldownReduction) {
    AttackContext context(nullptr, nullptr);
    context.setBaseDamage(10.0f);
    context.setCooldownProgress(0.5f);
    EXPECT_FLOAT_EQ(context.calculateFinalDamage(), 5.0f);
}

// 4. 暴击 + 冷却
TEST(AttackContextTest, CriticalWithCooldown) {
    AttackContext context(nullptr, nullptr);
    context.setBaseDamage(10.0f);
    context.setCritical(true);
    context.setCriticalMultiplier(1.5f);
    context.setCooldownProgress(0.8f);
    // 10 * 1.5 * 0.8 = 12.0
    EXPECT_FLOAT_EQ(context.calculateFinalDamage(), 12.0f);
}

// 5. DamageSource 创建
TEST(AttackContextTest, CreateDamageSourceMelee) {
    Entity attacker;
    AttackContext context(&attacker, nullptr);
    context.setAttackType(AttackType::Melee);
    auto source = context.createDamageSource();
    EXPECT_EQ(source->type(), DamageType::MobAttack);
}

// tests/common/entity/combat/PlayerAttackHelperTest.cpp

// 1. 冷却进度计算
TEST(PlayerAttackHelperTest, CooldownProgressCalculation) {
    // 攻击速度 4.0，间隔 = 20/4 = 5 tick
    EXPECT_FLOAT_EQ(PlayerAttackHelper::getCooldownProgress(0, 4.0f), 0.0f);
    EXPECT_FLOAT_EQ(PlayerAttackHelper::getCooldownProgress(5, 4.0f), 1.0f);
    EXPECT_FLOAT_EQ(PlayerAttackHelper::getCooldownProgress(10, 4.0f), 1.0f);
}

// 2. 冷却阈值检查
TEST(PlayerAttackHelperTest, CooldownThreshold) {
    EXPECT_TRUE(PlayerAttackHelper::isCooldownReady(0.9f));
    EXPECT_TRUE(PlayerAttackHelper::isCooldownReady(1.0f));
    EXPECT_FALSE(PlayerAttackHelper::isCooldownReady(0.89f));
}

// 3. 冷却伤害衰减
TEST(PlayerAttackHelperTest, CooldownDamageReduction) {
    EXPECT_FLOAT_EQ(PlayerAttackHelper::applyCooldown(10.0f, 1.0f), 10.0f);
    EXPECT_FLOAT_EQ(PlayerAttackHelper::applyCooldown(10.0f, 0.5f), 2.5f); // 10 * 0.5²
}

// 4. 击退计算
TEST(PlayerAttackHelperTest, KnockbackCalculation) {
    f32 base = PlayerAttackHelper::calculateKnockback(
        /*attacker=*/{}, /*target=*/{}, 1.0f, false);
    EXPECT_FLOAT_EQ(base, 1.0f);

    f32 sprint = PlayerAttackHelper::calculateKnockback(
        /*attacker=*/{}, /*target=*/{}, 1.0f, true);
    EXPECT_FLOAT_EQ(sprint, 1.5f);
}
```

---

## 参考

- MC 1.16.5 `net.minecraft.entity.player.PlayerEntity` - 玩家攻击逻辑
- MC 1.16.5 `net.minecraft.entity.LivingEntity` - 生物受伤逻辑
- MC 1.16.5 `net.minecraft.util.DamageSource` - 伤害来源
- MC 1.16.5 `net.minecraft.entity.ai.attributes.Attributes` - 属性系统
