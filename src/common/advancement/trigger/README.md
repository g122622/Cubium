# 触发器系统 (Trigger System)

## 概述

本模块实现了 Minecraft 1.16.5 的成就触发器系统，用于检测游戏事件并授予成就进度。

## 目录结构

```
trigger/
├── CriterionTrigger.hpp           # 触发器接口和基类
├── CriterionTrigger.cpp           # 实现
├── CriterionTriggers.hpp          # 触发器注册表
├── CriterionTriggers.cpp          # 注册表实现
│
├── conditions/                    # 条件谓词
│   ├── ItemPredicate.hpp/cpp      # 物品匹配条件
│   ├── EntityPredicate.hpp/cpp    # 实体匹配条件
│   ├── LocationPredicate.hpp/cpp  # 位置匹配条件
│   ├── BlockPredicate.hpp/cpp     # 方块匹配条件
│   └── DistancePredicate.hpp      # 距离匹配条件（在LocationPredicate中）
│
└── impl/                          # 触发器实现
    ├── ImpossibleTrigger.hpp      # 不可能触发器
    ├── InventoryChangedTrigger.hpp/cpp  # 物品栏变化
    ├── LocationTrigger.hpp/cpp          # 位置触发器
    ├── PlayerKilledEntityTrigger.hpp/cpp # 玩家击杀实体
    ├── BlockTriggers.hpp/cpp           # 方块相关触发器
    ├── ItemTriggers.hpp/cpp            # 物品相关触发器
    ├── EntityTriggers.hpp/cpp          # 实体相关触发器
    └── EffectTriggers.hpp/cpp          # 效果相关触发器
```

## 核心接口

### ICriterionTrigger

```cpp
template<typename T>
class ICriterionTrigger : public ICriterionTriggerBase {
public:
    using Listener = CriterionListener<T>;
    
    virtual void addListener(PlayerAdvancements& advancements, const Listener& listener) = 0;
    virtual void removeListener(PlayerAdvancements& advancements, const Listener& listener) = 0;
    virtual void removeAllListeners(PlayerAdvancements& advancements) = 0;
};
```

### AbstractCriterionTrigger

提供监听器管理的通用实现：

```cpp
template<typename T>
class AbstractCriterionTrigger : public ICriterionTrigger<T> {
protected:
    template<typename PredicateT>
    void trigger(PlayerAdvancements& advancements, PredicateT&& predicate);
    
    const std::set<Listener>& getListeners(PlayerAdvancements& advancements) const;
    bool hasListeners(PlayerAdvancements& advancements) const;
};
```

## 已实现的触发器

### 基础触发器

| 触发器 | ID | 说明 | 状态 |
|--------|-----|------|------|
| `ImpossibleTrigger` | `minecraft:impossible` | 无法自动完成，需手动授予 | ✅ 完整实现 |
| `InventoryChangedTrigger` | `minecraft:inventory_changed` | 物品栏变化 | ✅ 框架完成，待集成事件 |
| `TickTrigger` | `minecraft:tick` | 每tick触发 | ✅ 完整实现 |

### 位置触发器

| 触发器 | ID | 说明 |
|--------|-----|------|
| `LocationTrigger` | `minecraft:location` | 位置检测 |
| `SleptInBedTrigger` | `minecraft:slept_in_bed` | 睡觉 |
| `HeroOfTheVillageTrigger` | `minecraft:hero_of_the_village` | 村庄英雄 |
| `VoluntaryExileTrigger` | `minecraft:voluntary_exile` | 不祥之兆 |

### 实体触发器

| 触发器 | ID | 说明 |
|--------|-----|------|
| `PlayerKilledEntityTrigger` | `minecraft:player_killed_entity` | 玩家击杀实体 |
| `EntityKilledPlayerTrigger` | `minecraft:entity_killed_player` | 实体击杀玩家 |
| `TameAnimalTrigger` | `minecraft:tame_animal` | 驯服动物 |
| `BredAnimalsTrigger` | `minecraft:bred_animals` | 繁殖动物 |
| `SummonedEntityTrigger` | `minecraft:summoned_entity` | 召唤实体 |
| `CuredZombieVillagerTrigger` | `minecraft:cured_zombie_villager` | 治愈僵尸村民 |
| `VillagerTradeTrigger` | `minecraft:villager_trade` | 村民交易 |

### 方块触发器

| 触发器 | ID | 说明 |
|--------|-----|------|
| `EnterBlockTrigger` | `minecraft:enter_block` | 进入方块 |
| `PlacedBlockTrigger` | `minecraft:placed_block` | 放置方块 |
| `SlideDownBlockTrigger` | `minecraft:slide_down_block` | 滑落方块 |
| `BeeNestDestroyedTrigger` | `minecraft:bee_nest_destroyed` | 破坏蜂巢 |

### 效果触发器

| 触发器 | ID | 说明 |
|--------|-----|------|
| `EffectsChangedTrigger` | `minecraft:effects_changed` | 效果变化 |
| `BrewedPotionTrigger` | `minecraft:brewed_potion` | 酿造药水 |

## 条件谓词

### ItemPredicate

匹配物品的条件：

```cpp
ItemPredicate predicate = ItemPredicate::fromJson({
    {"item", "minecraft:diamond"},
    {"count", 5},
    {"durability", {"min", 100}}
});

if (predicate.test(itemStack)) {
    // 条件满足
}
```

### EntityPredicate

匹配实体的条件：

```cpp
EntityPredicate predicate = EntityPredicate::fromJson({
    {"type", "minecraft:zombie"}
});
```

### LocationPredicate

匹配位置的条件：

```cpp
LocationPredicate predicate = LocationPredicate::fromJson({
    {"biome", "minecraft:plains"},
    {"dimension", "minecraft:overworld"},
    {"position", {
        {"x", {"min", 100, "max", 200}},
        {"y", {"min", 60, "max", 80}}
    }}
});
```

### BlockPredicate

匹配方块的条件：

```cpp
BlockPredicate predicate = BlockPredicate::fromJson({
    {"block", "minecraft:stone"},
    {"state", {{"facing", "north"}}}
});
```

## 使用示例

### 注册触发器

```cpp
// 在服务器初始化时注册所有内置触发器
CriterionTriggers::instance().registerBuiltinTriggers();
```

### 创建成就条件

```cpp
// JSON格式
{
    "criteria": {
        "diamond": {
            "trigger": "minecraft:inventory_changed",
            "conditions": {
                "items": [
                    {"item": "minecraft:diamond"}
                ]
            }
        }
    }
}
```

### 触发检测

```cpp
// 当玩家物品栏变化时
auto& triggers = CriterionTriggers::instance();
auto* trigger = triggers.getTrigger<InventoryChangedTrigger>();
if (trigger) {
    trigger->trigger(player, inventory);
}
```

## 扩展指南

### 添加新触发器

1. 创建新的触发器类，继承 `AbstractCriterionTrigger<T>`：

```cpp
class MyTrigger : public AbstractCriterionTrigger<MyTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:my_trigger";
    
    class Instance : public CriterionInstance<Instance> {
        // 条件字段
        Result<void> fromJson(const nlohmann::json& json) override;
        nlohmann::json conditionsToJson() const override;
        
        // 条件检测
        bool test(...) const;
    };
    
    ResourceLocation getId() const override;
    Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json) override;
    
    void trigger(...);
};
```

2. 在 `CriterionTriggers::registerBuiltinTriggers()` 中注册：

```cpp
registerTrigger(std::make_unique<MyTrigger>());
```

3. 在 `triggers` 命名空间中添加 ID 常量：

```cpp
constexpr const char* MY_TRIGGER = "minecraft:my_trigger";
```

## 参考

- Minecraft 1.16.5: `net.minecraft.advancements.criterion.*`
- Minecraft Wiki: https://minecraft.fandom.com/wiki/Advancement/JSON_format
