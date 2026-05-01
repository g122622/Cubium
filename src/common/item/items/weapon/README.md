# 武器物品模块

本目录包含所有武器类物品的实现。

## 文件结构

```
weapon/
├── BowItem.hpp/cpp       # 弓物品 (完整实现)
├── CrossbowItem.hpp/cpp  # 弩物品 (部分实现)
├── TridentItem.hpp/cpp   # 三叉戟物品 (基本实现)
├── ThrowableItem.hpp/cpp # 投掷物品基类
├── ThrowableItems.hpp/cpp # 具体投掷物品(雪球/鸡蛋/末影珍珠等)
├── ArrowItem.hpp/cpp     # 箭矢物品
├── ShieldItem.hpp/cpp    # 盾牌物品 (框架)
├── FishingRodItem.hpp/cpp # 钓鱼竿
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

**关键方法：**
- `getArrowVelocity(int chargeTicks)` - 计算箭矢速度因子
- `onItemRightClick()` - 开始蓄力
- `onPlayerStoppedUse()` - 发射箭矢
- `findAmmo()` - 查找箭矢（副手 → 主手 → 背包）
- `isInfiniteArrow()` - 检查箭矢是否无限

### CrossbowItem（弩）- 部分实现

**已实现功能：**
- 装填机制和状态管理
- 装填时间计算（含快速装填附魔）
- 箭矢发射基础逻辑
- 穿透附魔支持
- 多重射击角度计算
- NBT弹丸存储

**待完善功能：**
- 烟花火箭发射支持
- 弹药查找优化
- 音效播放

### TridentItem（三叉戟）- 基本实现

**已实现功能：**
- 近战攻击（耐久消耗）
- 投掷逻辑（实体生成）
- 激流冲刺计算
- 忠诚附魔设置
- 附魔能力返回值

**待完善功能：**
- isInWater/isInRain检测
- 激流音效
- 方块硬度获取

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
├── AbstractArrowEntity (箭矢实体)
├── EnchantmentHelper (附魔辅助)
└── IWorld

TridentItem
├── Item (基类)
├── TridentEntity (三叉戟实体)
├── EnchantmentHelper
└── Player

ThrowableItem
├── Item (基类)
├── ProjectileItemEntity (投掷物实体)
└── Player
```

## 测试覆盖

- `tests/common/item/weapon/ThrowableItemTest.cpp` - 投掷物品测试（17个测试用例）

## 参考

- MC 1.16.5: `net.minecraft.item.BowItem`
- MC 1.16.5: `net.minecraft.item.CrossbowItem`
- MC 1.16.5: `net.minecraft.item.TridentItem`
- MC 1.16.5: `net.minecraft.item.ShieldItem`
