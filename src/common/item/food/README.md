# Food 系统

本目录实现了食物系统，包括食物属性定义和食物物品类。

## 文件说明

| 文件 | 职责 |
|------|------|
| `Food.hpp/cpp` | 食物属性类，定义饥饿值、饱和度、药水效果等 |
| `Foods.hpp/cpp` | 原版食物定义常量，包含所有MC 1.16.5食物 |
| `README.md` | 本文件 |

## 食物属性

### 基本属性

| 属性 | 类型 | 说明 |
|------|------|------|
| hunger | i32 | 恢复的饥饿值 (0-20) |
| saturation | f32 | 恢复的饱和度 |
| isMeat | bool | 是否为肉类（可喂狼） |
| fastEat | bool | 是否快速食用（16ticks vs 32ticks） |
| alwaysEdible | bool | 是否可在饱食时食用 |
| effects | vector | 药水效果列表 |

### 使用示例

```cpp
// 创建简单食物
Food apple(4, 0.3f);

// 创建肉类食物
Food cookedBeef(8, 0.8f).setMeat();

// 创建可随时食用的食物
Food goldenApple(4, 1.2f).setAlwaysEdible();

// 创建带效果的食物
Food pufferfish(1, 0.1f);
pufferfish.addEffect(&Effects::POISON, 1.0f);
```

## FoodItem 类

食物物品基类，继承自 Item。

### 主要方法

| 方法 | 说明 |
|------|------|
| `getUseDuration()` | 获取食用时间（16或32ticks） |
| `getUseAction()` | 获取使用动作（Eat/Drink） |
| `onItemRightClick()` | 右键使用，设置玩家正在食用 |
| `onItemUseFinish()` | 食用完成，恢复饥饿值、应用效果 |
| `canEat()` | 检查是否可以食用 |

### 注册食物

```cpp
// 注册苹果
auto& apple = ItemRegistry::instance().registerItem<FoodItem>(
    ResourceLocation("minecraft:apple"),
    ItemProperties().maxStackSize(64).food(&Foods::APPLE)
);
```

## 依赖关系

```
Food
  ├── 依赖 entity/effect/PotionEffect (药水效果)
  └── 被 FoodItem 使用

FoodItem
  ├── 继承 Item
  ├── 使用 Food
  └── 依赖 Player, LivingEntity, World
```

## MC 1.16.5 食物列表

### 基础食物 (22种)
APPLE, BAKED_POTATO, BEETROOT, BREAD, CARROT, CHORUS_FRUIT,
COOKED_CHICKEN, COOKED_COD, COOKED_MUTTON, COOKED_PORKCHOP,
COOKED_RABBIT, COOKED_SALMON, COOKIE, DRIED_KELP, HONEY_BOTTLE,
MELON_SLICE, MUSHROOM_STEW, POISONOUS_POTATO, BEEF, CHICKEN,
COD, MUTTON, PORKCHOP, RABBIT, SALMON, ROTTEN_FLESH, SPIDER_EYE,
SWEET_BERRIES

### 金苹果 (2种)
GOLDEN_APPLE, ENCHANTED_GOLDEN_APPLE

### 汤类 (3种)
BEETROOT_SOUP, RABBIT_STEW, SUSPICIOUS_STEW

### 特殊鱼类 (2种)
PUFFERFISH, TROPICAL_FISH

## 注意事项

1. **食物效果应用**：需要在 `onItemUseFinish()` 中调用 `Player::getFoodStats().eat()`
2. **容器物品**：蘑菇汤等返回碗，蜂蜜瓶返回玻璃瓶
3. **药水效果**：需要实现 PotionEffect 系统
4. **玩家饥饿系统**：需要实现 FoodStats 类

## 测试用例

相关测试文件：
- `tests/common/item/FoodTest.cpp`
- `tests/common/item/FoodItemTest.cpp`
