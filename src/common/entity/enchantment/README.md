# Entity Enchantment 位置依赖附魔效果模块

本目录实现了实体层面与位置相关的附魔效果追踪机制。

## 目录结构

```
entity/enchantment/
├── LocationEnchantmentTracker.hpp  # 位置依赖附魔效果跟踪器
├── LocationEnchantmentTracker.cpp  # 跟踪器实现
└── README.md                       # 本文件
```

## 模块职责

`LocationEnchantmentTracker` 追踪每个装备槽位上当前活跃的位置依赖附魔效果。
当实体移动到新的方块位置时，系统会重新评估附魔是否应激活/停用。

对应 MC Java 的 `LivingEntity.activeLocationDependentEnchantments`。

## 内部模块关系

```
┌─────────────────────────────────────────────────────┐
│           LocationEnchantmentTracker                 │
│  （按槽位追踪活跃的位置依赖附魔）                     │
│  • isActive(slot, enchantmentId) → bool              │
│  • hasActiveEnchantments() → bool                    │
│  • setActive(slot, enchantmentId)                    │
│  • setInactive(slot, enchantmentId) → bool           │
│  • clearSlot(slot) → set<string>                     │
│  • clearAll()                                        │
└──────────────────────┬──────────────────────────────┘
                       │ 被 LivingEntity 持有
                       ▼
┌─────────────────────────────────────────────────────┐
│                LivingEntity                          │
│  • m_locationEnchantmentTracker                     │
│  • onChangedBlock() → 调用 EnchantmentHelper        │
│  • stopLocationBasedEffects() → 停用所有位置效果     │
└──────────────────────┬──────────────────────────────┘
                       │ 调用
                       ▼
┌─────────────────────────────────────────────────────┐
│            EnchantmentHelper                         │
│  • runLocationChangedEffects(entity)                │
│  • runLocationChangedEffects(entity, stack, slot)   │
│  • stopLocationBasedEffects(entity, stack, slot)    │
│  • stopAllLocationBasedEffects(entity)              │
└──────────────────────┬──────────────────────────────┘
                       │ 调用附虚方法
                       ▼
┌─────────────────────────────────────────────────────┐
│          Enchantment (基类)                          │
│  • onLocationChanged() → bool（是否应激活）          │
│  • onLocationEffectDeactivated()（停用时清理）       │
├─────────────────────────────────────────────────────┤
│  FrostWalkerEnchantment: 在水面上放置霜冰            │
│  SoulSpeedEnchantment: 在灵魂沙/土上增加移动速度     │
└─────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖的模块）

| 依赖 | 路径 | 用途 |
|------|------|------|
| Types | `common/core/Types.hpp` | i32 等基础类型 |
| Enchantment | `common/item/enchantment/Enchantment.hpp` | 附魔 ID 字符串键 |

### 下游依赖（依赖本模块的模块）

| 模块 | 路径 | 用途 |
|------|------|------|
| LivingEntity | `common/entity/core/LivingEntity.hpp` | 持有 tracker 实例，在 tick 和装备变更时调用 |
| EnchantmentHelper | `common/item/enchantment/EnchantmentHelper.hpp` | 在效果评估时查询和更新 tracker |

## 容易踩的坑

### 1. 激活/停用状态必须成对

当 `onLocationChanged()` 返回值从 `true` 变为 `false` 时，系统会自动调用
`onLocationEffectDeactivated()`。如果附魔覆写了 `onLocationChanged()`，
**必须**同时覆写 `onLocationEffectDeactivated()` 来清理效果（如移除属性修饰符），
即使不需要清理也应提供空实现以保证设计一致性。

### 2. 冰霜行者不需要停用清理

`FrostWalkerEnchantment::onLocationEffectDeactivated()` 是空实现，
因为霜冰由 `FrostedIceBlock` 自行融化。但仍然覆写此方法以保证接口一致性。

### 3. 灵魂疾行需要停用清理

`SoulSpeedEnchantment::onLocationEffectDeactivated()` 必须移除 `MOVEMENT_SPEED`
属性修饰符，否则速度加成会一直残留。

### 4. 实体死亡时清理

`LivingEntity::die()` 中调用 `EnchantmentHelper::stopAllLocationBasedEffects()`
停用所有活跃的位置依赖附魔效果，防止属性修饰符在实体死亡后残留。

### 5. 装备变更触发

当装备发生变更时（`detectEquipmentUpdates()`），新装备会立即执行一次
`runLocationChangedEffects()`，确保穿上附魔靴子后立即检测当前位置。
旧装备会通过 `stopLocationBasedEffects()` 停用所有活跃效果。

### 6. 方块位置变化检测

`LivingEntity::tick()` 中通过比较 `m_lastBlockPos` 与当前 `BlockPos` 来检测
方块位置变化。只有实际跨越方块边界时才触发 `onChangedBlock()`，避免每 tick
都执行位置检测。

此外，当实体有活跃的位置依赖附魔但未移动时，每 20 tick 周期性调用
`onChangedBlock()` 重新评估，确保脚下方块被破坏/替换后附魔效果能正确停用
（如实体站在灵魂沙上不动，灵魂沙被挖走后灵魂疾行的速度修饰符需要被移除）。
此周期性检查通过 `LocationEnchantmentTracker::hasActiveEnchantments()` 判断
是否有活跃附魔，避免无附魔时的不必要开销。
