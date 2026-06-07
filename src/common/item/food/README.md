# Food 系统

本目录实现了食物属性定义，包括 Food 类和原版食物常量。

## 目录结构

```
src/common/item/food/
├── Food.hpp              # 食物属性类（饥饿值、饱和度、药水效果）
├── Food.cpp              # 食物属性实现
├── Foods.hpp             # 原版食物定义常量（MC 1.16.5 全部食物）
├── Foods.cpp             # 原版食物定义实现
└── README.md             # 本文件
```

## 内部模块关系

```
Food.hpp ─────────────────────────────────────┐
│ 食物属性数据类                               │
│ - 饥饿值、饱和度修正值                        │
│ - isMeat/fastEat/alwaysEdible 标记          │
│ - 药水效果列表                               │
└─────────────────────────────────────────────┘
                    │
                    ▼
Foods.hpp ────────────────────────────────────┐
│ 原版食物常量定义                             │
│ - 基础食物（苹果、面包、肉类等）              │
│ - 金苹果、附魔金苹果                         │
│ - 汤类（蘑菇汤、甜菜汤等）                   │
│ - 特殊鱼类（河豚、热带鱼）                   │
│ - 需要 initialize() 初始化                  │
└─────────────────────────────────────────────┘
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

- `common/core/Types.hpp` - 基础类型（i32, f32）
- `common/entity/effect/EffectType.hpp` - 药水效果类型
- `common/entity/effect/EffectInstance.hpp` - 药水效果实例（前向声明）

### 下游依赖（依赖本模块）

- `item/items/food/FoodItem.hpp` - 食物物品基类，使用 Food* 构造
- `item/Items.cpp` - 注册食物物品时引用 Foods 常量
- `entity/entities/passive/tamable/WolfEntity.cpp` - 狼是否可喂食（isMeat 检查）
- `entity/entities/passive/water/DolphinEntity.cpp` - 海豚喂食
- `entity/entities/passive/tamable/CatEntity.cpp` - 猫喂食
- `entity/entities/passive/horse/AbstractHorseEntity.cpp` - 马匹喂食

## 容易踩的坑

1. **饱和度计算公式**：实际饱和度 = `food * saturationModifier * 2.0`，不是直接使用 modifier 值。例如苹果（food=4, modifier=0.3）提供 4 × 0.3 × 2 = 2.4 饱和度。

2. **Foods 需要初始化**：`Foods::initialize()` 必须在 `Items::initialize()` 之前调用，否则食物物品注册时会访问未初始化的常量。

3. **FoodItem 不在本目录**：食物物品类 `FoodItem` 位于 `item/items/food/`，本目录只定义食物属性数据。

4. **isMeat 不影响进食动作**：`isMeat` 标记仅用于判断狼是否能食用，所有食物的 `getUseAction()` 都返回 `Eat`。

5. **药水效果概率**：通过 `Food::addEffect()` 添加效果时可指定概率（0.0-1.0），食用时需随机判定是否触发。

6. **容器物品返回**：蘑菇汤、甜菜汤、兔肉汤返回碗，蜂蜜瓶返回玻璃瓶——这是 FoodItem 的逻辑，不是 Food 类的职责。
