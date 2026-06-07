# Combat 模块

战斗系统模块，提供攻击上下文管理、玩家攻击辅助、伤害计算规则和难度相关计算功能。

## 目录结构

```
src/common/entity/combat/
├── AttackContext.hpp/cpp      # 攻击上下文（封装攻击者、目标、伤害类型、伤害计算）
├── CombatRules.hpp/cpp        # 战斗规则工具类（护甲减伤、附魔保护、抗性减伤、吸收值计算）
├── PlayerAttackHelper.hpp/cpp # 玩家攻击辅助（暴击判定、伤害计算、击退、冷却、附魔加成）
├── DifficultyHelper.hpp/cpp   # 难度工具类（难度伤害缩放、饥饿伤害限制、特殊机制）
└── README.md                  # 本文档
```

## 模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                      combat 模块                             │
│                                                             │
│  PlayerAttackHelper ──创建──► AttackContext                 │
│         │                         │                         │
│         │                         │ 调用                     │
│         ▼                         ▼                         │
│    CombatRules ◄──────────── 计算伤害                       │
│         │                         │                         │
│         │                         ▼                         │
│         └────────────────► DamageSource (damage 模块)       │
│                                                             │
│  DifficultyHelper (独立工具类)                              │
└─────────────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

### 上游依赖（本模块使用）

| 模块 | 具体依赖 |
|------|----------|
| `core/Types.hpp` | `Difficulty` 枚举、基本类型 |
| `entity/core/Entity.hpp` | `Entity` 基类 |
| `entity/core/LivingEntity.hpp` | `LivingEntity` 生物实体 |
| `entity/entities/player/Player.hpp` | `Player` 玩家实体 |
| `item/ItemStack.hpp` | `ItemStack` 物品堆 |
| `entity/damage/DamageSource.hpp` | `DamageSource` 伤害来源 |
| `entity/attribute/Attributes.hpp` | 属性常量（攻击伤害等） |
| `item/enchant/KnockbackEnchantment.hpp` | 击退附魔计算 |

### 下游依赖（使用本模块）

| 模块 | 使用方式 |
|------|----------|
| `entity/core/LivingEntity` | 受伤计算调用 `CombatRules` |
| `entity/entities/player/Player` | 攻击逻辑使用 `PlayerAttackHelper`、`AttackContext` |
| `entity/entities/monster/*` | 攻击伤害使用 `DifficultyHelper` |

## 容易踩的坑

### 1. 攻击冷却伤害衰减是平方衰减

```cpp
// 错误：以为是线性衰减
f32 damage = baseDamage * cooldownProgress;  // 错误！

// 正确：MC 1.16.5 使用平方衰减
f32 damage = baseDamage * (0.2f + cooldownProgress * cooldownProgress * 0.8f);
// 或使用工具方法
f32 damage = PlayerAttackHelper::applyCooldown(baseDamage, cooldownProgress);
```

### 2. Easy 难度伤害缩放不是简单的 0.5 倍

```cpp
// 错误：Easy 难度不是简单减半
f32 damage = baseDamage * 0.5f;  // 错误！

// 正确：Easy 难度公式是 min(damage/2 + 1, damage)
f32 damage = DifficultyHelper::adjustPlayerDamage(Difficulty::Easy, baseDamage);
// 例如：damage=10 -> min(5+1, 10) = 6，而不是 5
```

### 3. 暴击判定的 6 个条件必须全部满足

暴击条件缺一不可：
1. 玩家正在下落（velocity.y < 0）
2. 玩家不在地面（!onGround）
3. 玩家不在水中（!inWater）
4. 玩家不在梯子/藤蔓上
5. 玩家没有失明效果
6. 玩家没有骑乘

使用 `PlayerAttackHelper::isCriticalHit(player)` 而非手动判断。

### 4. 护甲减伤公式有韧性参数

```cpp
// 错误：忽略韧性参数
f32 reduced = damage * (1 - armor / 25.0f);  // 错误！

// 正确：韧性影响高伤害时的护甲效果
f32 reduced = CombatRules::getDamageAfterAbsorb(damage, armor, toughness);
// 公式：f = 2 + toughness/4
//       g = clamp(armor - damage/f, armor*0.2, 20)
//       final = damage * (1 - g/25)
```

### 5. 击退附魔加成使用 KnockbackEnchantment 方法

击退附魔加成不是硬编码的，应该使用 `KnockbackEnchantment::getKnockbackBonus()` 方法。

### 6. DifficultyHelper 中玩家伤害与怪物伤害的计算不同

- 玩家受伤：使用 `adjustPlayerDamage()` - 倍率计算
- 怪物攻击：使用 `getMobDamageAdjustment()` - 固定调整值（-2/0/+2）

### 7. 冷却阈值判断

只有冷却进度 >= 0.9 才算"完全充能"，使用 `PlayerAttackHelper::isCooldownReady(cooldownProgress)` 判断。
