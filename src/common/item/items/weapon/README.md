# 武器物品模块

本目录包含所有武器类物品的实现。

## 文件结构

```
weapon/
├── BowItem.hpp/cpp       # 弓物品
├── CrossbowItem.hpp/cpp  # 弩物品 (TODO)
├── TridentItem.hpp/cpp   # 三叉戟物品 (TODO)
├── ShieldItem.hpp/cpp    # 盾牌物品 (TODO)
└── README.md             # 本文件
```

## 已实现的物品

### BowItem（弓）

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

## 待实现的物品

### CrossbowItem（弩）

- 可以预先装填箭矢
- 支持多重射击、穿透、快速装填附魔
- 发射烟花火箭

### TridentItem（三叉戟）

- 近战武器（伤害与钻石剑相同）
- 可投掷（忠诚附魔回收）
- 激流附魔（水中冲刺）
- 引雷附魔（雷暴天气召唤闪电）

### ShieldItem（盾牌）

- 格挡伤害
- 斧头可破盾
- 耐久度消耗

## 依赖关系

```
BowItem
├── Item (基类)
├── ItemStack
├── Player
├── AbstractArrowEntity (箭矢实体)
├── EnchantmentHelper (附魔辅助)
└── IWorld
```

## 参考

- MC 1.16.5: `net.minecraft.item.BowItem`
- MC 1.16.5: `net.minecraft.item.CrossbowItem`
- MC 1.16.5: `net.minecraft.item.TridentItem`
- MC 1.16.5: `net.minecraft.item.ShieldItem`
