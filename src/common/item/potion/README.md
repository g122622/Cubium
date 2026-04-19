# 药水系统 (Potion System)

## 概述

此模块实现了 Minecraft 1.16.5 的药水系统，包括药水类型、效果、酿造配方等。

## 目录结构

```
src/common/potion/
├── Potion.hpp/cpp           # 药水类型类
├── PotionType.hpp           # 药水ID枚举
├── PotionRegistry.hpp/cpp   # 药水注册表
├── Potions.hpp/cpp          # 原版药水定义
├── PotionBrewing.hpp/cpp    # 酿造配方管理
├── PotionUtils.hpp/cpp      # 药水工具类
└── README.md                # 本文件
```

## 核心类

### Potion
药水类型类，定义一种药水的效果组合。

```cpp
// 创建药水
Potion nightVision("", {EffectInstance(EffectType::NightVision, 3600)});

// 创建延长版药水
Potion longNightVision("night_vision", {EffectInstance(EffectType::NightVision, 9600)});

// 创建多效果药水
Potion turtleMaster("turtle_master", {
    EffectInstance(EffectType::Slowness, 400, 3),
    EffectInstance(EffectType::Resistance, 400, 2)
});
```

### PotionRegistry
药水注册表，管理所有药水类型的注册和查找。

```cpp
// 注册药水
auto* potion = PotionRegistry::instance().registerPotion(
    ResourceLocation("minecraft:night_vision"),
    Potion("", {EffectInstance(EffectType::NightVision, 3600)})
);

// 通过ID查找药水
const Potion* potion = PotionRegistry::instance().getPotion(
    ResourceLocation("minecraft:night_vision")
);
```

### Potions
原版药水静态引用，提供所有原版药水的快速访问。

```cpp
// 初始化
Potions::initialize();

// 使用
const Potion* nightVision = Potions::NIGHT_VISION;
auto effects = nightVision->effects();
```

### PotionBrewing
酿造配方管理，处理药水酿造逻辑。

```cpp
// 初始化
PotionBrewing::initialize();

// 检查是否可酿造
bool can = PotionBrewing::canBrew(potionStack, reagentStack);

// 执行酿造
ItemStack result = PotionBrewing::brew(potionStack, reagentStack);
```

### PotionUtils
药水工具类，提供药水物品操作。

```cpp
// 获取药水
const Potion* potion = PotionUtils::getPotion(stack);

// 获取效果
auto effects = PotionUtils::getEffects(stack);

// 创建药水物品
ItemStack potionItem = PotionUtils::createPotionItem(Potions::NIGHT_VISION);

// 获取颜色
u32 color = PotionUtils::getColor(potion);
```

### GlassBottleItem
玻璃瓶会沿玩家视线采样，并优先识别可装瓶的水源方块与已装水的炼药锅；命中后返回水瓶。

## 药水效果持续时间

| 药水类型 | 普通 (tick) | 延长 (tick) | 加强 (tick) |
|---------|-------------|-------------|-------------|
| 夜视 | 3600 (3:00) | 9600 (8:00) | - |
| 隐身 | 3600 | 9600 | - |
| 跳跃提升 | 3600 | 9600 | 1800 (1:30) |
| 防火 | 3600 | 9600 | - |
| 速度 | 3600 | 9600 | 1800 |
| 缓慢 | 1800 (1:30) | 4800 (4:00) | 400 (0:20) |
| 水下呼吸 | 3600 | 9600 | - |
| 中毒 | 900 (0:45) | 1800 (1:30) | 432 (0:21) |
| 生命恢复 | 900 | 1800 | 450 (0:22) |
| 力量 | 3600 | 9600 | 1800 |
| 虚弱 | 1800 | 4800 | - |
| 缓降 | 1800 | 4800 | - |
| 海龟大师 | 400 (0:20) | 800 (0:40) | 400 |

## 酿造配方

### 基础药水
- 水瓶 + 下界疣 → 尴尬的药水
- 水瓶 + 荧石粉 → 浓稠的药水
- 水瓶 + 红石 → 平凡的药水
- 水瓶 + 其他材料 → 平凡的药水

### 效果药水
从尴尬的药水酿造：
- 尴尬的药水 + 金胡萝卜 → 夜视药水
- 夜视药水 + 红石 → 长效夜视药水
- 夜视药水 + 发酵蛛眼 → 隐身药水
- 尴尬的药水 + 岩浆膏 → 防火药水
- 尴尬的药水 + 兔子脚 → 跳跃提升药水
- 尴尬的药水 + 糖 → 速度药水
- 尴尬的药水 + 河豚 → 水下呼吸药水
- 尴尬的药水 + 闪烁的西瓜 → 瞬间治疗药水
- 尴尬的药水 + 蜘蛛眼 → 中毒药水
- 尴尬的药水 + 恶魂之泪 → 生命恢复药水
- 尴尬的药水 + 烈焰粉 → 力量药水
- 尴尬的药水 + 幻翼膜 → 缓降药水

### 升级
- 任意药水 + 红石 → 延长版
- 任意药水 + 荧石粉 → 加强版（不适用于所有药水）

### 容器转换
- 药水 + 火药 → 喷溅药水
- 喷溅药水 + 龙息 → 滞留药水
- 玻璃瓶 + 水源/装水炼药锅 → 水瓶

## 与外部系统的集成

### 物品系统
- `PotionItem`: 普通药水物品
- `SplashPotionItem`: 喷溅药水物品
- `LingeringPotionItem`: 滞留药水物品
- `GlassBottleItem`: 玻璃瓶物品

### 方块实体
- `BrewingStandEntity`: 酿造台方块实体

### 效果系统
- `EffectInstance`: 效果实例类
- `EffectType`: 效果类型枚举

## 参考
- net.minecraft.potion.Potion
- net.minecraft.potion.Potions
- net.minecraft.potion.PotionBrewing
- net.minecraft.potion.PotionUtils

## 测试用例

| 文件 | 说明 |
|------|------|
| `tests/common/item/potion/GlassBottleItemTest.cpp` | 验证玻璃瓶对水源和炼药锅的装水逻辑 |
