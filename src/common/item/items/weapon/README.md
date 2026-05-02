# 武器物品模块

本目录包含所有武器类物品的实现。

## 文件结构

```
weapon/
├── BowItem.hpp/cpp       # 弓物品 (完整实现)
├── CrossbowItem.hpp/cpp  # 弩物品 (完整实现)
├── TridentItem.hpp/cpp   # 三叉戟物品 (完整实现)
├── ThrowableItem.hpp/cpp # 投掷物品基类
├── ThrowableItems.hpp/cpp # 具体投掷物品(雪球/鸡蛋/末影珍珠等)
├── ArrowItem.hpp/cpp     # 箭矢物品
├── ShieldItem.hpp/cpp    # 盾牌物品 (框架实现)
├── FishingRodItem.hpp/cpp # 钓鱼竿 (完整实现)
└── README.md             # 本文件
```

## 已实现的物品

### BowItem（弓）- 完整实现

弓是可蓄力的远程武器，蓄力时间影响箭矢速度和伤害。

**蓄力机制：**
- 最小发射阈值: 速度 >= 0.1（约 3 tick）
- 满蓄力时间: 20 tick（1 秒）
- 最大速度: 3.0（满蓄力时）

**速度计算公式（MC 1.16.5）：**
```
f = charge / 20.0
velocity = (f * f + f * 2.0) / 3.0
```

**附魔支持：**
| 附魔 | 效果 |
|------|------|
| 力量 Power | 每级 +0.5 伤害 + 0.5 基础 |
| 冲击 Punch | 每级 +1 击退等级 |
| 火矢 Flame | 箭矢点燃目标 5 秒（100 tick） |
| 无限 Infinity | 不消耗普通箭矢（光灵箭/药水箭除外） |

**音效支持：** 完整实现 ENTITY_ARROW_SHOOT

**关键方法：**
- `getArrowVelocity(int chargeTicks)` - 计算箭矢速度因子
- `onItemRightClick()` - 开始蓄力
- `onPlayerStoppedUse()` - 发射箭矢
- `findAmmo()` - 查找箭矢（副手 → 主手 → 背包）
- `isInfiniteArrow()` - 检查箭矢是否无限

### CrossbowItem（弩）- 完整实现

弩是可以预先装填箭矢的远程武器。

**装填机制：**
- 基础装填时间: 25 tick（1.25秒）
- 快速装填附魔: 每级减少 5 tick
- 装填过程中播放音效（开始、中间、结束）

**发射机制：**
- 箭矢速度: 3.15（烟花 1.6）
- 支持多重射击: 发射 3 支箭矢
- 支持穿透: 箭矢可穿透实体
- 支持烟花火箭: 作为弹药

**附魔支持：**
- 多重射击 (Multishot): 同时发射 3 支箭矢
- 穿透 (Piercing): 箭矢可穿透实体
- 快速装填 (Quick Charge): 减少装填时间

**音效支持：**
- ITEM_CROSSBOW_LOADING_END - 装填完成音效
- ITEM_CROSSBOW_SHOOT - 箭矢发射音效
- ITEM_CROSSBOW_ROCKET - 烟花发射音效

**关键方法：**
- `isCharged()` / `setCharged()` - 装填状态管理
- `getChargeTime()` - 计算装填时间
- `findAmmo()` - 查找弹药（箭矢/烟花）
- `loadProjectiles()` - 装填弹丸
- `fireProjectiles()` - 发射弹丸
- `getChargedProjectiles()` - 获取已装填弹丸

**NBT 结构：**
- `Charged`: 布尔值，是否已装填
- `ChargedProjectiles`: 数组，存储装填的弹丸

### TridentItem（三叉戟）- 完整实现

**已实现功能：**
- 近战攻击（耐久消耗）
- 投掷逻辑（实体生成）
- 激流冲刺计算
- 忠诚附魔设置
- 附魔能力返回值
- 方块硬度检测（onBlockDestroyed）
- isWet 检测（水中或雨中）

**音效支持：**
- ITEM_TRIDENT_THROW - 投掷音效
- ITEM_TRIDENT_RIPTIDE_1/2/3 - 激流音效（按等级）

**关键方法：**
- `onItemRightClick()` - 检查湿润状态，开始蓄力
- `onPlayerStoppedUse()` - 投掷或激流冲刺
- `isWet()` - 检测玩家是否湿润（水中或雨中）

### ShieldItem（盾牌）- 框架实现

**已实现功能：**
- 格挡状态（UseAction::Block）
- 使用时间（72000 tick）
- 盾牌检测方法 `isShield()`

**待完善功能：**
- 盾牌格挡伤害计算
- 斧头破盾机制（100 tick 冷却）
- 盾牌修复（木板）
- 旗帜染色支持

### FishingRodItem（钓鱼竿）- 完整实现

**已实现功能：**
- 抛杆/收杆逻辑
- FishingBobberEntity 钓鱼浮标实体
- 钓鱼附魔支持（海之眷顾、饵钓）
- 开放水域检测
- 咬钩状态机
- Player.fishingBobber 字段集成

**音效支持：**
- ENTITY_FISHING_BOBBER_THROW - 抛杆音效
- ENTITY_FISHING_BOBBER_RETRIEVE - 收杆音效

**钓鱼机制：**
- 等待时间: 100-600 tick（5-30秒）
- 饵钓附魔: 每级减少 100 tick（5秒）
- 咬钩窗口: 20-40 tick（1-2秒）
- 开放水域: 增加宝藏概率

**关键方法：**
- `hasBobber()` - 检查是否有浮标
- `getBobber()` - 获取浮标实体
- `onItemRightClick()` - 抛杆/收杆

### ThrowableItems（投掷物品）- 完整实现

| 物品 | 功能 |
|------|------|
| SnowballItem | 雪球，对烈焰人造成3点伤害 |
| EggItem | 鸡蛋，12.5%概率孵化小鸡 |
| EnderPearlItem | 末影珍珠，传送并造成5点摔落伤害 |
| ExperienceBottleItem | 经验瓶，生成3-11个经验球 |
| PotionItem | 药水（待完善药水系统） |

## 依赖关系

```
BowItem
├── Item (基类)
├── ItemStack
├── Player
├── PlayerInventory
├── AbstractArrowEntity (箭矢实体)
├── EnchantmentHelper (附魔辅助)
├── SoundEvents (音效)
└── IWorld

CrossbowItem
├── Item (基类)
├── ItemStack
├── Player
├── PlayerInventory
├── AbstractArrowEntity
├── FireworkRocketEntity (烟花实体)
├── EnchantmentHelper
├── SoundEvents (音效)
└── IWorld

TridentItem
├── Item (基类)
├── TridentEntity (三叉戟实体)
├── EnchantmentHelper
├── Entity (isWet方法)
├── SoundEvents (音效)
└── Player

ShieldItem
├── Item (基类)
├── Player
└── LivingEntity

FishingRodItem
├── Item (基类)
├── Player (fishingBobber 字段)
├── FishingBobberEntity (钓鱼浮标实体)
├── EnchantmentHelper
├── SoundEvents (音效)
└── IWorld

ThrowableItem
├── Item (基类)
├── ProjectileItemEntity (投掷物实体)
└── Player
```

## 测试覆盖

- `tests/common/item/weapon/WeaponItemTest.cpp` - 武器物品测试（19个测试用例）
  - BowItem: 注册检查、使用时间、使用动作、箭矢速度计算、弹药检测
  - CrossbowItem: 注册检查、装填时间、装填状态、弹药检测
  - ArrowItem: 注册检查、耐久度
  - TridentItem: 使用动作、使用时间、耐久度

## 参考

- MC 1.16.5: `net.minecraft.item.BowItem`
- MC 1.16.5: `net.minecraft.item.CrossbowItem`
- MC 1.16.5: `net.minecraft.item.TridentItem`
- MC 1.16.5: `net.minecraft.item.ShieldItem`
- MC 1.16.5: `net.minecraft.item.FishingRodItem`
- MC 1.16.5: `net.minecraft.entity.projectile.FishingBobberEntity`
