# Enchantment 附魔系统

本目录实现了 Minecraft 1.16.5 风格的附魔系统，包含全部 34 种原版附魔。

## 目录结构

```
enchantment/
├── Enchantment.hpp              # 附魔基类定义
├── Enchantment.cpp              # 附魔基类实现
├── EnchantmentContainer.hpp     # 附魔容器（存储物品上的附魔）
├── EnchantmentContainer.cpp     # 附魔容器实现
├── EnchantmentHelper.hpp        # 附魔查询工具类
├── EnchantmentHelper.cpp        # 附魔查询工具类实现
├── EnchantmentRegistry.hpp      # 附魔注册表
├── EnchantmentRegistry.cpp      # 附魔注册表实现
├── README.md                    # 本文件
└── enchantments/                # 具体附魔实现
    ├── AllEnchantments.hpp      # 所有附魔统一包含
    ├── AllEnchantments.cpp      # 所有附魔注册
    ├── FortuneEnchantment.hpp   # 时运附魔
    ├── SilkTouchEnchantment.hpp # 精准采集附魔
    ├── protection/              # 保护类附魔（11种）
    │   ├── ProtectionEnchantment.hpp/cpp    # 保护基类
    │   ├── AllProtectionEnchantment.hpp     # 全保护
    │   ├── FireProtectionEnchantment.hpp    # 火焰保护
    │   ├── FeatherFallingEnchantment.hpp    # 摔落保护
    │   ├── BlastProtectionEnchantment.hpp   # 爆炸保护
    │   ├── ProjectileProtectionEnchantment.hpp # 弹射物保护
    │   ├── ThornsEnchantment.hpp/cpp        # 荆棘
    │   ├── RespirationEnchantment.hpp       # 水下呼吸
    │   ├── AquaAffinityEnchantment.hpp      # 水下速掘
    │   ├── DepthStriderEnchantment.hpp/cpp  # 深海探索者
    │   └── FrostWalkerEnchantment.hpp/cpp   # 冰霜行者
    ├── weapon/                  # 武器类附魔（7种）
    │   ├── DamageEnchantment.hpp/cpp        # 伤害附魔基类
    │   ├── SharpnessEnchantment.hpp         # 锋利
    │   ├── SmiteEnchantment.hpp             # 亡灵杀手
    │   ├── BaneOfArthropodsEnchantment.hpp  # 节肢杀手
    │   ├── KnockbackEnchantment.hpp         # 击退
    │   ├── FireAspectEnchantment.hpp        # 火焰附加
    │   ├── LootingEnchantment.hpp           # 抢夺
    │   └── SweepingEnchantment.hpp          # 横扫之刃
    ├── tool/                    # 工具类附魔（4种）
    │   ├── EfficiencyEnchantment.hpp        # 效率
    │   ├── UnbreakingEnchantment.hpp/cpp    # 耐久
    │   ├── FortuneEnchantment.cpp           # 时运
    │   └── SilkTouchEnchantment.cpp         # 精准采集
    ├── bow/                     # 弓类附魔（4种）
    │   ├── PowerEnchantment.hpp             # 力量
    │   ├── PunchEnchantment.hpp             # 冲击
    │   ├── FlameEnchantment.hpp             # 火矢
    │   └── InfinityEnchantment.hpp          # 无限
    ├── fishing/                 # 钓鱼类附魔（2种）
    │   ├── LuckOfTheSeaEnchantment.hpp      # 海之眷顾
    │   └── LureEnchantment.hpp              # 饵钓
    ├── trident/                 # 三叉戟附魔（4种）
    │   ├── LoyaltyEnchantment.hpp           # 忠诚
    │   ├── ImpalingEnchantment.hpp          # 穿刺
    │   ├── RiptideEnchantment.hpp           # 激流
    │   └── ChannelingEnchantment.hpp        # 引雷
    ├── crossbow/                # 弩类附魔（3种）
    │   ├── MultishotEnchantment.hpp         # 多重射击
    │   ├── QuickChargeEnchantment.hpp       # 快速装填
    │   └── PiercingEnchantment.hpp          # 穿透
    └── special/                 # 特殊附魔（4种）
        ├── MendingEnchantment.hpp           # 经验修补
        ├── VanishingCurseEnchantment.hpp    # 消失诅咒
        ├── BindingCurseEnchantment.hpp      # 绑定诅咒
        └── SoulSpeedEnchantment.hpp         # 灵魂疾行
```

## 已实现附魔（34种）

### 保护类附魔（11种）

| 附魔 | ID | 最大等级 | 稀有度 | 效果 |
|------|-----|---------|--------|------|
| 保护 | minecraft:protection | IV | 普通 | 减少所有伤害 |
| 火焰保护 | minecraft:fire_protection | IV | 稀有 | 减少火焰伤害 |
| 摔落保护 | minecraft:feather_falling | IV | 稀有 | 减少摔落伤害 |
| 爆炸保护 | minecraft:blast_protection | IV | 罕见 | 减少爆炸伤害 |
| 弹射物保护 | minecraft:projectile_protection | IV | 稀有 | 减少弹射物伤害 |
| 荆棘 | minecraft:thorns | III | 极罕见 | 攻击者受反伤 |
| 水下呼吸 | minecraft:respiration | III | 罕见 | 延长水下呼吸时间 |
| 水下速掘 | minecraft:aqua_affinity | I | 罕见 | 水下挖掘不减慢 |
| 深海探索者 | minecraft:depth_strider | III | 罕见 | 增加水下移动速度 |
| 冰霜行者 | minecraft:frost_walker | II | 罕见 | 水面行走生成冰 |

### 武器类附魔（7种）

| 附魔 | ID | 最大等级 | 稀有度 | 效果 |
|------|-----|---------|--------|------|
| 锋利 | minecraft:sharpness | V | 普通 | 增加对所有生物伤害 |
| 亡灵杀手 | minecraft:smite | V | 稀有 | 增加对亡灵伤害 |
| 节肢杀手 | minecraft:bane_of_arthropods | V | 稀有 | 增加对节肢伤害 |
| 击退 | minecraft:knockback | II | 稀有 | 增加击退距离 |
| 火焰附加 | minecraft:fire_aspect | II | 罕见 | 目标燃烧 |
| 抢夺 | minecraft:looting | III | 罕见 | 增加掉落物 |
| 横扫之刃 | minecraft:sweeping | III | 罕见 | 增加横扫伤害 |

### 工具类附魔（4种）

| 附魔 | ID | 最大等级 | 稀有度 | 效果 |
|------|-----|---------|--------|------|
| 效率 | minecraft:efficiency | V | 普通 | 增加挖掘速度 |
| 耐久 | minecraft:unbreaking | III | 稀有 | 减少耐久消耗 |
| 时运 | minecraft:fortune | III | 罕见 | 增加方块掉落 |
| 精准采集 | minecraft:silk_touch | I | 极罕见 | 采集方块本身 |

### 弓类附魔（4种）

| 附魔 | ID | 最大等级 | 稀有度 | 效果 |
|------|-----|---------|--------|------|
| 力量 | minecraft:power | V | 普通 | 增加箭矢伤害 |
| 冲击 | minecraft:punch | II | 罕见 | 增加箭矢击退 |
| 火矢 | minecraft:flame | I | 罕见 | 箭矢点燃目标 |
| 无限 | minecraft:infinity | I | 极罕见 | 不消耗箭矢 |

### 钓鱼类附魔（2种）

| 附魔 | ID | 最大等级 | 稀有度 | 效果 |
|------|-----|---------|--------|------|
| 海之眷顾 | minecraft:luck_of_the_sea | III | 罕见 | 增加宝藏概率 |
| 饵钓 | minecraft:lure | III | 罕见 | 减少等待时间 |

### 三叉戟附魔（4种）

| 附魔 | ID | 最大等级 | 稀有度 | 效果 |
|------|-----|---------|--------|------|
| 忠诚 | minecraft:loyalty | III | 稀有 | 三叉戟返回 |
| 穿刺 | minecraft:impaling | V | 罕见 | 增加对水生生物伤害 |
| 激流 | minecraft:riptide | III | 罕见 | 水中冲刺 |
| 引雷 | minecraft:channeling | I | 极罕见 | 雷暴时召唤闪电 |

### 弩类附魔（3种）

| 附魔 | ID | 最大等级 | 稀有度 | 效果 |
|------|-----|---------|--------|------|
| 多重射击 | minecraft:multishot | I | 罕见 | 射出三支箭 |
| 快速装填 | minecraft:quick_charge | III | 稀有 | 减少装填时间 |
| 穿透 | minecraft:piercing | IV | 普通 | 箭矢穿透目标 |

### 特殊附魔（4种）

| 附魔 | ID | 最大等级 | 稀有度 | 效果 |
|------|-----|---------|--------|------|
| 经验修补 | minecraft:mending | I | 罕见 | 经验修复耐久 |
| 消失诅咒 | minecraft:vanishing_curse | I | 极罕见 | 死亡时消失 |
| 绑定诅咒 | minecraft:binding_curse | I | 极罕见 | 无法取下 |
| 灵魂疾行 | minecraft:soul_speed | III | 极罕见 | 灵魂沙上加速 |

## 核心枚举

### EnchantmentType 附魔类型

```cpp
enum class EnchantmentType : u8 {
    Armor,          // 护甲（头盔、胸甲、护腿、靴子）
    ArmorFeet,      // 仅靴子
    ArmorHead,      // 仅头盔
    ArmorChest,     // 仅胸甲
    Weapon,         // 武器（剑）
    Digger,         // 挖掘工具（镐、斧、铲、锄）
    FishingRod,     // 钓鱼竿
    Breakable,      // 可破坏物品
    Bow,            // 弓
    Wearable,       // 可穿戴物品
    Crossbow,       // 弩
    Trident,        // 三叉戟
    Vanishable,     // 可消失物品
    All             // 所有物品
};
```

### EnchantmentRarity 附魔稀有度

```cpp
enum class EnchantmentRarity : u8 {
    Common,     // 普通（权重10）
    Uncommon,   // 稀有（权重5）
    Rare,       // 罕见（权重2）
    VeryRare    // 极罕见（权重1）
};
```

## 使用方法

### 初始化附魔系统

```cpp
// 游戏启动时初始化（自动注册所有34种附魔）
mc::item::enchant::EnchantmentRegistry::initialize();
```

### 查询附魔

```cpp
using namespace mc::item::enchant;

// 获取附魔定义
const Enchantment* sharpness = EnchantmentRegistry::get("minecraft:sharpness");
if (sharpness) {
    i32 maxLevel = sharpness->maxLevel();  // 5
    i32 minCost = sharpness->getMinCost(1); // 1
    f32 damage = sharpness->getDamageBonus(5, 0); // 3.0
}

// 按类型查询
auto weaponEnchants = EnchantmentRegistry::getByType(EnchantmentType::Weapon);

// 检查兼容性
const Enchantment* smite = EnchantmentRegistry::get("minecraft:smite");
bool compatible = sharpness->isCompatibleWith(*smite);  // false（锋利与亡灵杀手互斥）
```

### 使用附魔容器

```cpp
EnchantmentContainer container;

// 添加附魔
container.set("minecraft:sharpness", 5);
container.set("minecraft:looting", 3);
container.set("minecraft:unbreaking", 3);

// 查询
i32 level = container.getLevel("minecraft:sharpness");  // 5
bool has = container.has("minecraft:looting");          // true

// 兼容性检查
bool canAdd = container.canAdd("minecraft:smite");  // false（与锋利互斥）
```

### 使用工具类

```cpp
// 便捷查询
bool hasSilkTouch = EnchantmentHelper::hasSilkTouch(stack);
i32 fortuneLevel = EnchantmentHelper::getFortuneLevel(stack);

// 计算伤害加成
f32 damageBonus = EnchantmentHelper::getTotalDamageBonus(stack, entityType);

// 计算保护值
i32 protection = EnchantmentHelper::getTotalProtection(stack, damageType);
```

### 时运计算

```cpp
math::Random random(seed);

// 钻石矿掉落（时运 III）
i32 diamonds = FortuneEnchantment::applyBonus(1, 3, random);  // 1-4

// 煤矿石掉落（均匀分布）
i32 coal = 1 + FortuneEnchantment::applyUniformBonus(3, random);  // 1-4

// 红石矿掉落
i32 redstone = FortuneEnchantment::applyOreDropBonus(4, 5, 3, random);  // 4-8
```

### 耐久计算

```cpp
math::Random random(seed);

// 工具耐久检查
bool shouldDamage = UnbreakingEnchantment::shouldConsumeDurability(3, random);

// 盔甲耐久检查
bool shouldArmorDamage = UnbreakingEnchantment::shouldArmorConsumeDurability(3, random);
```

## 附魔互斥关系

| 附魔组 | 互斥附魔 |
|--------|----------|
| 保护类 | 保护、火焰保护、爆炸保护、弹射物保护互斥（摔落保护除外） |
| 伤害类 | 锋利、亡灵杀手、节肢杀手互斥 |
| 时运/精准采集 | 时运与精准采集互斥 |
| 弓附魔 | 无限与经验修补互斥 |
| 三叉戟 | 激流与忠诚、引雷互斥 |
| 弩附魔 | 多重射击与穿透互斥 |
| 靴子附魔 | 深海探索者与冰霜行者互斥 |

## 扩展指南

### 添加新附魔

1. 在 `enchantments/` 适当子目录创建附魔类：

```cpp
// enchantments/weapon/ExampleEnchantment.hpp
#pragma once

#include "../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

class ExampleEnchantment : public Enchantment {
public:
    [[nodiscard]] String id() const override {
        return "minecraft:example";
    }

    [[nodiscard]] i32 maxLevel() const override { return 3; }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Weapon;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Rare;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        return 10 + (level - 1) * 5;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
```

2. 在 `AllEnchantments.hpp` 中添加头文件和静态成员
3. 在 `AllEnchantments.cpp` 中注册

## 参考

- Minecraft Java Edition 1.16.5 源码：`net.minecraft.enchantment`
- Minecraft Wiki：https://minecraft.fandom.com/wiki/Enchanting
