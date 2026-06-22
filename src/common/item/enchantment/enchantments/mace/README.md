# 重锤附魔模块

本目录包含重锤(Mace)专属附魔的实现。

## 目录结构

```
mace/
├── BreachEnchantment.hpp   # 破甲附魔（降低目标护甲有效率）
├── BreachEnchantment.cpp   # 破甲附魔互斥逻辑
├── DensityEnchantment.hpp  # 致密附魔（增加下落攻击每格伤害）
├── DensityEnchantment.cpp  # 致密附魔互斥逻辑
├── WindBurstEnchantment.hpp # 风爆附魔（下落攻击命中后弹起攻击者）
├── WindBurstEnchantment.cpp # 风爆附魔互斥逻辑
└── README.md               # 本文件
```

## 内部模块关系

- 三种附魔均继承自 `Enchantment` 基类
- 致密和破甲互相排斥，属于 DAMAGE_EXCLUSIVE 组
- 风爆不与任何伤害附魔互斥，可与致密或破甲共存
- 致密的伤害加成由 `MaceItem::getSmashAttackDamageBonus()` 调用
- 破甲的护甲削减由 `AttackContext::calculateFinalDamage()` 调用
- 风爆的弹起效果由 `Player::attack()` 调用

## 上下游外部依赖关系

### 上游依赖

| 依赖 | 路径 | 用途 |
|------|------|------|
| Enchantment | `../../Enchantment.hpp` | 附魔基类 |
| EnchantmentHelper | `../../EnchantmentHelper.hpp` | 附魔等级查询 |
| DamageEnchantment | `../weapon/DamageEnchantment.hpp` | 互斥性检查 |
| ImpalingEnchantment | `../trident/ImpalingEnchantment.hpp` | 互斥性检查 |

### 下游依赖

| 模块 | 路径 | 用途 |
|------|------|------|
| MaceItem | `items/trial/MaceItem.cpp` | 致密伤害加成计算 |
| AttackContext | `entity/combat/AttackContext.cpp` | 破甲护甲削减计算 |
| Player | `entity/entities/player/Player.cpp` | 风爆弹起效果 |
| AllEnchantments | `../AllEnchantments.cpp` | 注册所有附魔实例 |

## 容易踩的坑

### 1. DAMAGE_EXCLUSIVE 互斥组需双向添加

新增互斥关系时，必须在双方的 `isCompatibleWith()` 中都添加检查。例如致密排斥穿刺，则 `DensityEnchantment::isCompatibleWith()` 和 `ImpalingEnchantment::isCompatibleWith()` 都需要对应检查。

### 2. 破甲的护甲削减在伤害计算中应用

破甲不是独立的伤害加成，而是修改护甲有效率。计算顺序为：先计算原始护甲减伤比，再应用破甲修正（每级 -0.15），结果 clamp 到 [0, 1]。

### 3. 风爆当前为简化实现

风爆附魔目前仅施加向上速度，未使用完整的爆炸系统（ExplodeEffect）。完整实现需包含爆炸粒子、对周围实体的击退等。
