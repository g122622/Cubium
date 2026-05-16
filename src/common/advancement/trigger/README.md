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
│   ├── BlockPredicate.hpp/cpp     # 方块匹配条件 + 流体匹配条件
│   └── DistancePredicate.hpp      # 距离匹配条件（在LocationPredicate中）
│
└── impl/                          # 触发器实现
    ├── ImpossibleTrigger.hpp      # 不可能触发器
    ├── TickTrigger.hpp/cpp        # Tick触发器
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
    
    virtual void addListener(mc::server::PlayerAdvancements& advancements, const Listener& listener) = 0;
    virtual void removeListener(mc::server::PlayerAdvancements& advancements, const Listener& listener) = 0;
    virtual void removeAllListeners(mc::server::PlayerAdvancements& advancements) = 0;
};
```

### AbstractCriterionTrigger

提供监听器管理的通用实现：

```cpp
template<typename T>
class AbstractCriterionTrigger : public ICriterionTrigger<T> {
protected:
    template<typename PredicateT>
    void trigger(mc::server::PlayerAdvancements& advancements, PredicateT&& predicate);
    
    const std::set<Listener>& getListeners(mc::server::PlayerAdvancements& advancements) const;
    bool hasListeners(mc::server::PlayerAdvancements& advancements) const;
};
```

## 已实现的触发器

### 基础触发器

| 触发器 | ID | 说明 | 状态 |
|--------|-----|------|------|
| `ImpossibleTrigger` | `minecraft:impossible` | 无法自动完成，需手动授予 | ✅ 完整实现 |
| `InventoryChangedTrigger` | `minecraft:inventory_changed` | 物品栏变化 | ✅ 条件检测完成，待服务端事件集成 |
| `TickTrigger` | `minecraft:tick` | 每tick触发 | ✅ 完整实现 |

### InventoryChangedTrigger 详细说明

`InventoryChangedTrigger` 用于检测玩家物品栏变化，支持以下条件：

- `slots.occupied`: 占用槽位数量范围
- `slots.full`: 满槽位数量范围
- `slots.empty`: 空槽位数量范围
- `items`: 物品谓词列表

```cpp
// 条件检测示例
InventoryChangedTriggerInstance instance = ...;

// 使用 testWithInventory 方法检测
bool matches = instance.testWithInventory(
    PlayerInventory::TOTAL_SIZE,  // 41
    [&inventory](i32 slot) -> const ItemStack& {
        return inventory.getItem(slot);
    }
);
```

**服务端集成**：触发器的实际触发需要在服务端事件系统中完成。
详见 `src/server/advancement/TriggerInstantiation.hpp` 和事件系统集成文档。

### 位置触发器

| 触发器 | ID | 说明 |
|--------|-----|------|
| `LocationTrigger` | `minecraft:location` | 位置检测 |
| `SleptInBedTrigger` | `minecraft:slept_in_bed` | 睡觉 |
| `HeroOfTheVillageTrigger` | `minecraft:hero_of_the_village` | 村庄英雄 |
| `VoluntaryExileTrigger` | `minecraft:voluntary_exile` | 不祥之兆 |

### 实体触发器

| 触发器 | ID | 说明 | 状态 |
|--------|-----|------|------|
| `PlayerKilledEntityTrigger` | `minecraft:player_killed_entity` | 玩家击杀实体 | ✅ 完整实现，已注册 |
| `EntityKilledPlayerTrigger` | `minecraft:entity_killed_player` | 实体击杀玩家 | ✅ 完整实现，已注册 |
| `TameAnimalTrigger` | `minecraft:tame_animal` | 驯服动物 | ✅ 完整实现 |
| `BredAnimalsTrigger` | `minecraft:bred_animals` | 繁殖动物 | ⏳ 条件检测完成，待事件集成 |
| `SummonedEntityTrigger` | `minecraft:summoned_entity` | 召唤实体 | ⏳ 条件检测完成，待事件集成 |
| `CuredZombieVillagerTrigger` | `minecraft:cured_zombie_villager` | 治愈僵尸村民 | ⏳ 条件检测完成，待事件集成 |
| `VillagerTradeTrigger` | `minecraft:villager_trade` | 村民交易 | ⏳ 条件检测完成，待事件集成 |

#### PlayerKilledEntityTrigger 详细说明

`PlayerKilledEntityTrigger` 用于检测玩家击杀实体的事件，支持以下条件：

- `entity`: 实体谓词，匹配被击杀的实体类型
- `killing_blow`: 伤害源谓词，匹配击杀方式

```cpp
// 触发检测（服务端）
void onPlayerKillEntity(const PlayerKillEntityEvent& e) {
    auto* trigger = CriterionTriggers::instance().getTrigger<PlayerKilledEntityTrigger>();
    if (trigger && e.victim && e.cause) {
        trigger->AbstractCriterionTrigger<PlayerKilledEntityTriggerInstance>::trigger(
            *player.getAdvancements(),
            [&e](const PlayerKilledEntityTriggerInstance& instance) {
                return instance.test(*e.victim, *e.cause);
            }
        );
    }
}
```

**使用示例** (JSON 成就条件):
```json
{
    "criteria": {
        "killed_zombie": {
            "trigger": "minecraft:player_killed_entity",
            "conditions": {
                "entity": {
                    "type": "minecraft:zombie"
                }
            }
        }
    }
}
```

#### TameAnimalTrigger 详细说明

`TameAnimalTrigger` 用于检测玩家驯服动物的事件，支持条件检测：

- `entity`: 实体谓词，匹配被驯服的动物类型

```cpp
// 触发驯服事件（服务端）
void onAnimalTamed(ServerPlayer& player, AnimalEntity* animal) {
    auto* trigger = CriterionTriggers::instance().getTrigger<TameAnimalTrigger>();
    if (trigger && trigger->hasListeners(*player.getAdvancements())) {
        trigger->trigger(*player.getAdvancements(), animal);
    }
}
```

**使用示例** (JSON 成就条件):
```json
{
    "criteria": {
        "tamed_horse": {
            "trigger": "minecraft:tame_animal",
            "conditions": {
                "entity": {
                    "type": "minecraft:horse"
                }
            }
        }
    }
}
```

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

ItemPredicate 支持以下字段：
- `item`: 物品ID（如 `minecraft:diamond`）
- `count`: 数量（精确值或范围）
- `durability`: 耐久度范围
- `potion`: 药水类型
- `nbt`: NBT数据匹配
- `enchantments`: 附魔匹配

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

### FluidPredicate

匹配流体的条件（定义在 `BlockPredicate.hpp/cpp` 中）：

```cpp
FluidPredicate predicate = FluidPredicate::fromJson({
    {"fluid", "minecraft:water"}
});

// 检查方块是否包含指定流体
if (predicate.test(blockState)) {
    // 方块包含水（包括水源和流动水）
}
```

FluidPredicate 使用 `Fluid::isEquivalentTo()` 比较流体等效性：
- `minecraft:water` 同时匹配水源 (`minecraft:water`) 和流动水 (`minecraft:flowing_water`)
- `minecraft:lava` 同时匹配岩浆源 (`minecraft:lava`) 和流动岩浆 (`minecraft:flowing_lava`)

**实现细节**：
- 通过 `BlockState::getFluidState()` 获取方块的流体状态
- 使用 `FluidState::isEmpty()` 检查是否为空流体
- 使用 `Fluid::getFluid(ResourceLocation)` 获取期望的流体
- 使用 `Fluid::isEquivalentTo()` 进行等效比较

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

### 触发检测（服务端）

在服务端模块中，使用 `TriggerInstantiation.hpp` 来完成触发：

```cpp
#include "server/advancement/TriggerInstantiation.hpp"

// 当玩家物品栏变化时
void onInventoryChanged(ServerPlayer& player, const PlayerInventory& inventory) {
    auto* trigger = CriterionTriggers::instance().getTrigger<InventoryChangedTrigger>();
    if (trigger && trigger->hasListeners(*player.getAdvancements())) {
        // 使用模板方法触发
        trigger->trigger(*player.getAdvancements(), [&](const auto& instance) {
            return instance.testWithInventory(
                PlayerInventory::TOTAL_SIZE,
                [&inventory](i32 slot) -> const ItemStack& {
                    return inventory.getItem(slot);
                }
            );
        });
    }
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
    
    void trigger(ServerPlayer& player, ...);
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

## 架构说明

### 模块划分

触发器系统分为两个模块：

1. **Common 模块** (`mc::advancement`)
   - 触发器接口和基类定义
   - 条件谓词实现
   - JSON 解析和序列化
   - `test()` 方法实现

2. **Server 模块** (`mc::server`)
   - `PlayerAdvancements`: 玩家进度管理
   - `TriggerInstantiation.hpp`: 模板方法实例化
   - 事件系统集成

### 命名空间注意事项

- `mc::advancement::PlayerAdvancements` 是前向声明，实际定义在 `mc::server::PlayerAdvancements`
- 所有触发器接口使用 `mc::server::PlayerAdvancements&` 作为参数类型
- 服务端集成代码必须包含 `server/advancement/TriggerInstantiation.hpp`

## 参考

- Minecraft 1.16.5: `net.minecraft.advancements.criterion.*`
- Minecraft Wiki: https://minecraft.fandom.com/wiki/Advancement/JSON_format
