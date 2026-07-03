# 武器物品模块

本目录包含所有武器类物品的实现。

## 目录结构

```
weapon/
├── ArrowItem.hpp/cpp           # 普通箭矢物品，实现 ProjectileItem 接口
├── BowItem.hpp/cpp             # 弓物品，可蓄力远程武器
├── CrossbowItem.hpp/cpp        # 弩物品，可预装填的远程武器
├── FireChargeItem.hpp/cpp      # 火焰弹物品，实现 ProjectileItem 接口
├── FireworkRocketItem.hpp/cpp  # 烟花火箭物品，实现 ProjectileItem 接口
├── FishingRodItem.hpp/cpp      # 钓鱼竿物品
├── ShieldItem.hpp/cpp          # 盾牌物品，格挡攻击（框架实现）
├── SpearItem.hpp/cpp           # 长矛物品，按材质分层，近战与投掷结合
├── SpectralArrowItem.hpp/cpp   # 光灵箭物品，继承 ArrowItem
├── ThrowableItem.hpp/cpp       # 投掷物品基类（实现 ProjectileItem 接口）
├── ThrowableItems.hpp/cpp      # 具体投掷物品（雪球/鸡蛋/末影珍珠/经验瓶）
├── TippedArrowItem.hpp/cpp     # 药水箭物品，带药水效果的箭矢
├── TridentItem.hpp/cpp         # 三叉戟物品，近战与投掷结合
└── README.md                   # 本文件
```

## 内部模块关系

```
ThrowableItem (基类，实现 ProjectileItem 接口)
├── ThrowableItems (SnowballItem/EggItem/EnderPearlItem)
│   └── 默认 getDispenseConfig() → defaults()（power=1.1, uncertainty=6.0）
│   └── 默认 shoot() → 委托 ProjectileEntity::shoot()
├── ExperienceBottleItem
│   └── 覆写 getDispenseConfig() → potion()（power=1.375, uncertainty=3.0）
│   └── 子类需实现 createProjectileEntity()
└── [ThrowablePotionItem → 见 items/potion/ 模块]

ArrowItem (基类，实现 ProjectileItem 接口)
├── SpectralArrowItem（光灵箭，覆写 asProjectile 创建 SpectralArrowEntity）
└── TippedArrowItem（药水箭，覆写 asProjectile 应用药水效果）

FireChargeItem (实现 ProjectileItem 接口)
└── getDispenseConfig() → fireCharge()（power=1.0, uncertainty=6.0）
└── shoot() 为空操作，asProjectile 中设置加速度

FireworkRocketItem (实现 ProjectileItem 接口)
└── getDispenseConfig() → fireworkRocket()（power=0.5, uncertainty=1.0）
└── asProjectile 中设置 FireworkRocketEntity 的烟花数据

BowItem ──────→ AbstractArrowEntity (创建箭矢实体)
CrossbowItem ─→ AbstractArrowEntity / FireworkRocketEntity
TridentItem ──→ TridentEntity
SpearItem ────→ SpearEntity
FishingRodItem → FishingBobberEntity
```

## 上下游外部依赖关系

**本目录依赖：**
- `item/core/` - Item 基类、ItemStack、ActionResult、UseAction、ProjectileItem 接口、ProjectileDispenseConfig
- `item/tag/ItemTags.hpp` - ARROWS 标签（箭矢检测）
- `item/enchantment/EnchantmentHelper.hpp` - 附魔查询
- `entity/projectile/` - AbstractArrowEntity、TridentEntity、FishingBobberEntity、ProjectileItemEntity、SmallFireballEntity、FireworkRocketEntity
- `entity/Entity.hpp` - LivingEntity、Player
- `entity/effect/` - EffectInstance（药水箭）
- `potion/Potion.hpp` - 药水系统（药水箭）
- `world/IWorld.hpp` - 世界接口
- `sound/SoundEvents.hpp` - 音效事件

**被依赖：**
- `item/Items.hpp` - 注册所有武器物品
- `world/block/dispense/DispenseItemBehaviorRegistry.cpp` - 通过 ProjectileItem 接口注册发射行为
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

### 4. 三叉戟激流条件

激流附魔只有在玩家潮湿（水中或雨中）时才能触发投掷。`_isWet()` 方法检测玩家是否在水中或雨中。不在水中时，有激流附魔的三叉戟无法投掷。

### 5. ProjectileItem 接口与发射器行为

实现了 `ProjectileItem` 接口的物品（ArrowItem、ThrowableItem、FireChargeItem、WindChargeItem、FireworkRocketItem）会自动被 `DispenseItemBehaviorRegistry::registerProjectileBehavior()` 注册到发射器行为注册表中，无需手动编写 lambda 工厂函数。新增投掷物物品时只需实现 ProjectileItem 接口并在 `initDefaultBehaviors()` 中调用 `registerProjectileBehavior()` 即可。

### 6. FireChargeItem 的 shoot() 为空操作

FireChargeItem 的 `shoot()` 方法被覆写为空操作，因为火焰弹在 `asProjectile()` 中已通过 `setAcceleration()` 设置了加速度。DamagingProjectileEntity 的 tick() 方法每帧将加速度叠加到速度上，如果再调用 shoot() 设置速度会导致速度叠加错误。

FireChargeItem 同时实现了 `onItemUse()` 方法，支持玩家右键使用火焰弹：
- 点燃含 `LIT` 属性的未点燃方块（如营火、蜡烛等），含水方块不可点燃
- 否则在点击面的相邻空气位置放置火焰（普通火或灵魂火，取决于下方方块是否在 `SOUL_FIRE_BASE_BLOCKS` 标签中）
- 使用后消耗一个火焰弹（创造模式不消耗）
- 播放 `ITEM_FIRECHARGE_USE` 音效

### 7. 投掷物品速度参数

`ThrowableItem` 的 `getThrowVelocity()` 默认返回 1.5f，`getThrowInaccuracy()` 默认返回 0.0f。子类可以重写这些方法调整投掷参数。

### 8. ThrowableItem 实现 ProjectileItem 接口

`ThrowableItem` 同时继承 `Item` 和 `ProjectileItem`，提供：
- `asProjectile()`：调用子类的 `createProjectileEntity()` 创建弹射物实体，设置位置，但不添加到世界。调用方负责将实体添加到世界、设置发射者和调用 `shoot()`。
- `getDispenseConfig()`：默认返回 `ProjectileDispenseConfig::defaults()`（power=1.1, uncertainty=6.0）。ExperienceBottleItem 和 ThrowablePotionItem 覆写返回 `potion()` 配置。
- `shoot()`：默认委托给 `ProjectileEntity::shoot()`。

`ThrowableItem` 新增纯虚方法 `createProjectileEntity()`，子类必须实现以创建对应类型的弹射物实体。此方法供 `asProjectile()` 和 `createProjectile()` 共用，消除了两处创建弹射物实体的重复代码。

### 9. ExperienceBottleItem 的发射器配置

`ExperienceBottleItem` 覆写 `getDispenseConfig()` 返回 `ProjectileDispenseConfig::potion()`（power=1.375, uncertainty=3.0），与药水投掷物使用相同的发射器参数。这是因为它和药水一样需要更精确的发射轨迹。

### 10. SpearItem 与 TridentItem 的区别

长矛（SpearItem）和三叉戟（TridentItem）都是近战+投掷结合的武器，但有重要区别：
- **分层 vs 单一**：SpearItem 继承 TieredItem，按材质分层（木/石/铜/铁/金/钻石/下界合金），近战伤害随层级变化；TridentItem 是单一物品，固定 8 伤害。
- **附魔支持**：TridentItem 支持忠诚/激流/引雷/穿刺；SpearItem 不支持这些附魔（数据包未将 spears 加入 trident 可附魔标签），长矛专属附魔 Lunge 暂未实现。
- **投掷伤害**：长矛投掷伤害固定 8.0（与三叉戟一致），不随层级变化。
- **投掷实体**：长矛使用 SpearEntity（不支持忠诚返回），三叉戟使用 TridentEntity（支持忠诚返回）。
- **耐久消耗**：长矛近战消耗 1，破坏方块消耗 2（与剑一致）；三叉戟近战消耗 1，破坏方块消耗 2。
