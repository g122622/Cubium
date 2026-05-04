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
├── DifficultyHelper.cpp   # 难度工具类实现
└── README.md              # 本文档
```

## 文件详解

### AttackContext.hpp / AttackContext.cpp

**职责**：封装攻击行为的完整上下文信息，作为攻击计算的参数容器和结果计算器。

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
| | `m_bypassArmor` | 是否穿透护甲 |
| 攻击冷却 | `m_cooldownProgress` | 冷却进度（0-1，默认 1.0） |

**核心方法**：

```cpp
// 计算最终伤害（MC 1.16.5 伤害计算流程）
f32 calculateFinalDamage() const;

// 创建伤害来源对象
std::unique_ptr<DamageSource> createDamageSource() const;
```

**伤害计算流程**（MC 1.16.5）：

1. 基础伤害
2. 力量药水加成（每级 +3）
3. 虚弱药水减益（每级 -4）
4. 暴击加成（×1.5）
5. **攻击冷却影响**（`damage * (0.2 + progress² * 0.8)`，冷却为0时伤害为20%）
6. 护甲减伤
7. 抗性药水减伤
8. 附魔保护减伤

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

**注意**：击退附魔加成使用 `KnockbackEnchantment::getKnockbackBonus()` 方法计算。

**静态方法**：

#### 暴击判定

```cpp
static bool isCriticalHit(const Player& player);
```

MC 1.16.5 暴击条件（必须全部满足）：
1. 玩家正在下落（垂直速度 < 0）
2. 玩家不在地面
3. 玩家不在水中
4. 玩家不在梯子/藤蔓上
5. 玩家没有失明效果
6. 玩家没有骑乘

#### 伤害计算

```cpp
static f32 calculateDamage(const Player& player, f32 baseDamage, f32 cooldownProgress);
```

计算流程：
1. 应用攻击冷却影响（冷却²衰减）
2. 力量药水加成（每级 +3）
3. 虚弱药水减益（每级 -4）
4. 附魔伤害加成（锋利、亡灵杀手、节肢杀手）

#### 击退计算

```cpp
static f32 calculateKnockback(const LivingEntity& attacker,
                               const LivingEntity& target,
                               f32 baseKnockback = 1.0f,
                               bool isSprinting = false,
                               i32 knockbackLevel = 0);

static void applyKnockback(LivingEntity& target,
                           const LivingEntity& attacker,
                           f32 strength);
```

击退计算：
- 基础击退 + 疾跑加成（+0.5）
- 击退附魔加成（每级 +0.5）
- 目标击退抗性减伤（在 applyKnockback 中处理）

MC 1.16.5 击退公式：
```
strength *= (1 - knockbackResistance)
newVelX = currentVelX / 2 - knockbackX
newVelY = onGround ? min(0.4, currentVelY / 2 + strength) : currentVelY
newVelZ = currentVelZ / 2 - knockbackZ
```

#### 攻击冷却

```cpp
static f32 applyCooldown(f32 damage, f32 cooldownProgress);
static bool isCooldownReady(f32 cooldownProgress, f32 threshold = 0.9f);
static f32 getCooldownProgress(i32 ticksSinceLastAttack, f32 attackSpeed);
```

MC 1.16.5 冷却机制：
- 攻击间隔 = 20 / attackSpeed（tick）
- 冷却进度 = ticksSinceLastAttack / 攻击间隔
- **冷却不足时伤害 = 原伤害 × 冷却²**（平方衰减！）
- 只有冷却 >= 0.9 才能造成完整伤害

#### 火焰附加

```cpp
static bool applyFireAspect(LivingEntity& target, i32 fireAspectLevel);
```

火焰持续时间 = 80 × 附魔等级（tick），每级 4 秒。

#### 横扫攻击

```cpp
static f32 getSweepingDamageRatio(i32 sweepingLevel);
```

横扫之刃伤害比例：
- I: 50%, II: 67%, III: 75%
- 公式: `1 - 1/(level + 1)`

#### 附魔伤害加成

```cpp
static f32 getEnchantmentDamageBonus(const ItemStack& weapon,
                                      CreatureAttribute targetCreatureType);
```

计算锋利、亡灵杀手、节肢杀手的附加伤害：
- 锋利: 0.5 + level × 0.5
- 亡灵杀手（对亡灵）: level × 2.5
- 节肢杀手（对节肢动物）: level × 2.5

#### 创建攻击上下文

```cpp
static AttackContext createContext(Player& player,
                                    LivingEntity& target,
                                    f32 cooldownProgress);
```

工厂方法，自动配置：
- 攻击者信息
- 攻击冷却
- 暴击判定
- 击退强度（含疾跑和附魔加成）
- 火焰附加

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
│           │               │  - 附魔伤害加成          │  │
│           ▼               └─────────────────────────┘  │
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
   - 暴击伤害加成（×1.5）
   - **攻击冷却影响（冷却²衰减）**
   - 护甲减伤
   - 药水效果

3. **玩家攻击机制**
   - 暴击判定（跳跃攻击）
   - 攻击冷却系统
   - 击退计算（含击退抗性）
   - 火焰附加应用
   - 附魔伤害加成

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
| 玩家状态 | Player& | 下落、疾跑、骑乘等状态 |

### 输出

| 输出 | 类型 | 说明 |
|------|------|------|
| 最终伤害 | f32 | 经过所有修正后的伤害值 |
| DamageSource | std::unique_ptr | 用于 ApplyDamage 的伤害来源对象 |
| 击退向量 | void (副作用) | 直接修改目标速度 |

---

## 与 MC 1.16.5 对齐情况

### 已对齐

| 功能 | 状态 | 说明 |
|------|------|------|
| 暴击判定 | ✅ 已对齐 | 6 个条件全部实现 |
| 攻击冷却伤害衰减 | ✅ 已对齐 | 冷却² 衰减公式 |
| 击退计算 | ✅ 已对齐 | 含疾跑、附魔、击退抗性 |
| 护甲减伤公式 | ✅ 已对齐 | CombatRules 实现 |
| 抗性药水减伤 | ✅ 已对齐 | 每级 20% 减伤 |
| 附魔保护减伤 | ✅ 已对齐 | EPF 上限 20 |
| 力量药水加成 | ✅ 已对齐 | 每级 +3 伤害 |
| 虚弱药水减益 | ✅ 已对齐 | 每级 -4 伤害 |
| 火焰附加 | ✅ 已对齐 | 每级 4 秒 |
| 横扫之刃 | ✅ 已对齐 | I:50%, II:67%, III:75% |
| 附魔伤害加成 | ✅ 已对齐 | 锋利、亡灵杀手、节肢杀手 |
| DamageSource 字段 | ✅ 已对齐 | hungerDamage, isDifficultyScaled, isThornsDamage, isDamageAbsolute |
| MobEntity.attackEntityAsMob() | ✅ 已对齐 | 附魔伤害、击退、火焰附加 |
| ZombieEntity 燃烧传递 | ✅ 已对齐 | 区域难度影响点燃概率 |
| Easy 难度伤害缩放 | ✅ 已对齐 | min(damage/2 + 1, damage) |

### 待实现

| 功能 | 状态 | 说明 |
|------|------|------|
| 玩家 attack() 方法 | ⏳ TODO | 需要在 Player 类中实现完整的 attack() 方法 |
| 攻击冷却追踪 | ⏳ TODO | Player 类需要 m_ticksSinceLastAttack 字段 |
| 盾牌格挡 | ⏳ TODO | Player::canBlockDamageSource() 需要实现 |
| 盾牌损坏 | ⏳ TODO | Player::damageShield() 需要实现 |
| 横扫攻击范围检测 | ⏳ TODO | 需要在 attack() 中检测横扫目标 |

---

## 测试用例

**建议测试覆盖**：

```cpp
// tests/common/entity/combat/PlayerAttackHelperTest.cpp

// 1. 暴击判定
TEST(PlayerAttackHelperTest, CriticalHit_WhenFalling) {
    // 下落 + 不在地面 + 不在水中 + 不在梯子 + 无失明 + 无骑乘
}

// 2. 攻击冷却伤害衰减
TEST(PlayerAttackHelperTest, CooldownDamageReduction) {
    // 冷却 0.5 时，伤害 = 原伤害 × 0.25
    EXPECT_FLOAT_EQ(PlayerAttackHelper::applyCooldown(10.0f, 0.5f), 2.5f);
}

// 3. 击退计算
TEST(PlayerAttackHelperTest, KnockbackCalculation) {
    // 疾跑击退 = 1.0 + 0.5 = 1.5
    // 击退 II = 1.0 + 0.5 * 2 = 2.0
}

// 4. 附魔伤害加成
TEST(PlayerAttackHelperTest, EnchantmentDamageBonus) {
    // 锋利 V = 0.5 + 5 * 0.5 = 3.0
    // 亡灵杀手 III（对亡灵）= 3 * 2.5 = 7.5
}
```

---

## 参考

- MC 1.16.5 `net.minecraft.entity.player.PlayerEntity` - 玩家攻击逻辑
- MC 1.16.5 `net.minecraft.entity.LivingEntity` - 生物受伤逻辑
- MC 1.16.5 `net.minecraft.util.DamageSource` - 伤害来源
- MC 1.16.5 `net.minecraft.entity.ai.attributes.Attributes` - 属性系统
- MC 1.16.5 `net.minecraft.util.CombatRules` - 战斗规则公式
