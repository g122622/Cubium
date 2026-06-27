# 食物物品模块 (Food Items)

食物物品的实现，继承自 `Item` 基类，处理食用行为、饥饿恢复和药水效果。

## 目录结构

```
food/
├── FoodItem.hpp/cpp          # 食物基类（食用逻辑、药水效果、迷之炖菜NBT效果解析）
├── ChorusFruitItem.hpp/cpp   # 紫颂果（食用后随机传送）
├── GoldenAppleItem.hpp/cpp   # 金苹果（附魔金苹果变体，额外效果）
└── HoneyBottleItem.hpp/cpp   # 蜂蜜瓶（解毒、容器返回空瓶）
```

## 内部模块关系

```
Item (基类)
├── FoodItem                    # 食物基类
│   ├── m_food (Food*)          # 食物属性（饥饿值、饱和度、效果等）
│   ├── onItemRightClick()      # 开始进食
│   ├── onItemUseFinish()       # 完成进食：恢复饥饿、应用效果、播放音效、消耗物品
│   ├── canEat()                # 是否可食用（饱食时金苹果等仍可食用）
│   └── 迷之炖菜NBT效果解析     # 从物品标签 {Effects: [{EffectId, EffectDuration}]} 读取效果
│
├── ChorusFruitItem             # 紫颂果：食用后随机传送
├── GoldenAppleItem             # 金苹果/附魔金苹果
└── HoneyBottleItem             # 蜂蜜瓶：清除中毒效果，返回玻璃瓶
```

## 上下游依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `item/core/Item` | 物品基类 |
| `item/food/Food` | 食物属性定义（饥饿值、饱和度、效果列表） |
| `entity/effect/EffectType` | 药水效果类型枚举 |
| `entity/effect/EffectInstance` | 药水效果实例 |
| `entity/entities/player/Player` | 玩家（饥饿值、物品栏、创造模式判断） |
| `sound/SoundEvents` | 进食/打嗝音效 |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `item/Items` | 食物物品注册 |
| `entity/entities/passive/basic/MooshroomEntity` | 迷之炖菜物品的NBT效果标签写入 |

## 容易踩的坑

### 1. 迷之炖菜效果解析

`FoodItem::onItemUseFinish()` 中读取物品 NBT 标签 `{Effects: [{EffectId: byte, EffectDuration: int}]}` 来应用迷之炖菜效果。EffectDuration 单位为 tick，EffectId 为 `EffectType` 枚举值的整数形式。此 NBT 格式与 `MooshroomEntity::interactMob()` 写入的格式一致。

### 2. 金苹果效果概率

金苹果的药水效果带有概率（`Food::Effect::probability`），食用时对每个效果独立判定概率。附魔金苹果通过 `canAlwaysEat()` 标记允许饱食时食用。

### 3. 容器物品返回

食物食用后通过 `hasContainerItem()` / `containerItem()` 返回容器物品（如碗→空碗、蜂蜜瓶→玻璃瓶）。创造模式不消耗物品。
