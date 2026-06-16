# 武器物品模块

本目录包含所有武器类物品的实现。

## 目录结构

```
weapon/
├── ArrowItem.hpp/cpp           # 普通箭矢物品，用于弓和弩的弹药
├── BowItem.hpp/cpp             # 弓物品，可蓄力远程武器
├── CrossbowItem.hpp/cpp        # 弩物品，可预装填的远程武器
├── FishingRodItem.hpp/cpp      # 钓鱼竿物品
├── ShieldItem.hpp/cpp          # 盾牌物品，格挡攻击（框架实现）
├── ThrowableItem.hpp/cpp       # 投掷物品基类
├── ThrowableItems.hpp/cpp      # 具体投掷物品（雪球/鸡蛋/末影珍珠/经验瓶）
├── TippedArrowItem.hpp/cpp     # 药水箭物品，带药水效果的箭矢
├── TridentItem.hpp/cpp         # 三叉戟物品，近战与投掷结合
└── README.md                   # 本文件
```

## 内部模块关系

```
ThrowableItem (基类)
└── ThrowableItems (SnowballItem/EggItem/EnderPearlItem/ExperienceBottleItem)

ArrowItem (基类)
└── TippedArrowItem (继承 ArrowItem)

BowItem ──────→ AbstractArrowEntity (创建箭矢实体)
CrossbowItem ─→ AbstractArrowEntity / FireworkRocketEntity
TridentItem ──→ TridentEntity
FishingRodItem → FishingBobberEntity
```

## 上下游外部依赖关系

**本目录依赖：**
- `item/core/` - Item 基类、ItemStack、ActionResult、UseAction
- `item/tag/ItemTags.hpp` - ARROWS 标签（箭矢检测）
- `item/enchantment/EnchantmentHelper.hpp` - 附魔查询
- `entity/projectile/` - AbstractArrowEntity、TridentEntity、FishingBobberEntity、ProjectileItemEntity
- `entity/Entity.hpp` - LivingEntity、Player
- `entity/effect/` - EffectInstance（药水箭）
- `potion/Potion.hpp` - 药水系统（药水箭）
- `world/IWorld.hpp` - 世界接口
- `sound/SoundEvents.hpp` - 音效事件

**被依赖：**
- `item/Items.hpp` - 注册所有武器物品
- `entity/player/PlayerInventory.hpp` - 弹药查找
- `entity/player/Player.hpp` - fishingBobber 字段（钓鱼浮标）
- `tests/common/item/weapon/` - 武器物品测试

## 容易踩的坑

### 1. 弓蓄力阈值

弓的最小发射阈值是速度 >= 0.1（约 3 tick），而不是 0。过早松开右键不会发射箭矢。速度计算公式 `f = charge / 20.0; velocity = (f * f + f * 2.0) / 3.0`。

### 2. 弓附魔委托

BowItem 通过 `arrow->applyBowEnchantments(shooter)` 将附魔效果委托给 AbstractArrowEntity 处理，而非在 BowItem 中内联处理。该方法读取射手主手武器的附魔等级并应用到箭矢：力量（每级 +0.5 伤害 + 基础 0.5）、冲击（每级 +1 击退强度）、火焰（着火 100 ticks）。

### 3. 弩 NBT 结构

弩使用 `Charged` 布尔值存储装填状态，使用 `ChargedProjectiles` 数组存储已装填的弹丸。发射后需要清除这些标签，否则会残留旧数据。

### 3. 三叉戟激流条件

激流附魔只有在玩家潮湿（水中或雨中）时才能触发投掷。`_isWet()` 方法检测玩家是否在水中或雨中。不在水中时，有激流附魔的三叉戟无法投掷。

### 4. 药水箭不受益于无限附魔

MC 1.16.5 中，药水箭（TippedArrowItem）总是返回 `isInfinite() = false`，即使玩家拥有无限附魔也会消耗箭矢。只有普通箭受益于无限附魔。

### 5. 钓鱼竿不重写 getUseDuration/getUseAction

钓鱼竿是即时使用物品，不重写 `getUseDuration()`（默认返回 0）和 `getUseAction()`（默认返回 NONE）。这与弓/弩/三叉戟不同，它们都有使用动画。

### 6. 盾牌格挡逻辑未完全实现

ShieldItem 目前只有框架实现：`getUseDuration()` 返回 72000，`getUseAction()` 返回 `UseAction::Block`。格挡伤害计算、斧头破盾机制、盾牌修复等功能待完善。

### 7. 投掷物品速度参数

`ThrowableItem` 的 `getThrowVelocity()` 默认返回 1.5f，`getThrowInaccuracy()` 默认返回 0.0f。子类可以重写这些方法调整投掷参数。
