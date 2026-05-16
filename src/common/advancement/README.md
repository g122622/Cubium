# 成就系统 (Advancement System)

## 概述

本模块实现了 Minecraft 1.16.5 的成就系统核心架构，包括：
- 成就定义和 JSON 加载
- 条件触发器系统框架
- 进度追踪
- 触发器条件谓词

## 目录结构

```
advancement/
├── Advancement.hpp/cpp           # 成就定义（不可变）
├── AdvancementDisplay.hpp/cpp    # 显示信息（图标、标题、描述）
├── AdvancementFrame.hpp          # 框架类型枚举（Task/Challenge/Goal）
├── AdvancementList.hpp/cpp       # 成就列表管理（父子关系）
├── AdvancementLoader.hpp/cpp     # JSON 加载器
├── AdvancementManager.hpp/cpp    # 成就注册表（单例）
├── AdvancementProgress.hpp/cpp   # 进度追踪
├── AdvancementRewards.hpp/cpp    # 奖励定义
├── Criterion.hpp/cpp             # 条件定义
├── MinMaxBounds.hpp              # 范围谓词（IntBounds, DoubleBounds 等）
├── README.md                     # 本文件
│
└── trigger/                      # 触发器系统
    ├── CriterionTrigger.hpp      # 触发器接口（ICriterionTrigger, AbstractCriterionTrigger）
    ├── CriterionTriggers.hpp/cpp # 触发器注册表
    │
    ├── conditions/               # 触发器条件谓词
    │   ├── ItemPredicate.hpp/cpp     # 物品匹配
    │   ├── EntityPredicate.hpp/cpp   # 实体匹配
    │   ├── LocationPredicate.hpp/cpp # 位置匹配
    │   ├── BlockPredicate.hpp/cpp    # 方块匹配
    │   └── MobEffectsPredicate.hpp/cpp # 效果匹配
    │
    └── impl/                     # 触发器实现
        ├── ImpossibleTrigger.hpp      # 不可能触发器（手动授予）
        └── InventoryChangedTrigger.hpp/cpp  # 物品栏变化触发器
```

## 核心类

### Advancement

成就定义，不可变对象。包含：
- ID（ResourceLocation）
- 父成就ID（可选）
- 显示信息（可选）
- 奖励（可选）
- 条件映射
- 需求矩阵

使用 Builder 模式构建：
```cpp
auto advancement = Advancement::Builder(id)
    .parent(parentId)
    .display(displayInfo)
    .criterion("diamond", triggerInstance)
    .build();
```

### AdvancementProgress

追踪玩家在特定成就上的进度：
- 条件完成状态
- 完成时间戳
- 百分比计算
- JSON 序列化/反序列化

### CriterionTrigger

触发器系统采用模板设计：
- `ICriterionInstance` - 触发器实例基类
- `ICriterionTrigger<T>` - 触发器接口
- `AbstractCriterionTrigger<T>` - 监听器管理基类

创建新触发器：
```cpp
// 1. 定义触发器实例
class MyTriggerInstance : public CriterionInstance<MyTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:my_trigger";
    
    Result<void> fromJson(const nlohmann::json& json) override;
    nlohmann::json conditionsToJson() const override;
    bool test(...) const;  // 条件检测
};

// 2. 定义触发器
class MyTrigger : public AbstractCriterionTrigger<MyTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:my_trigger";
    
    ResourceLocation getId() const override;
    Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;
    void trigger(ServerPlayer& player, ...);  // 触发检测
};

// 3. 注册
CriterionTriggers::instance().registerTrigger(std::make_unique<MyTrigger>());
```

### AdvancementManager

全局成就注册表（单例）：
- 管理成就列表
- 提供查询接口
- 支持热重载

## 已实现的触发器

| 触发器 | ID | 状态 |
|--------|-----|------|
| ImpossibleTrigger | `minecraft:impossible` | 完整实现 |
| InventoryChangedTrigger | `minecraft:inventory_changed` | 完整实现（含服务端集成） |
| TameAnimalTrigger | `minecraft:tame_animal` | 完整实现（含服务端集成） |
| PlayerKilledEntityTrigger | `minecraft:player_killed_entity` | 完整实现（含服务端集成） |
| EntityKilledPlayerTrigger | `minecraft:entity_killed_player` | 完整实现 |
| PlayerInteractedWithEntityTrigger | `minecraft:player_interacted_with_entity` | 完整实现（含服务端集成） |
| LocationTrigger | `minecraft:location` | 完整实现（含服务端集成） |
| SleptInBedTrigger | `minecraft:slept_in_bed` | 完整实现（含服务端集成） |
| HeroOfTheVillageTrigger | `minecraft:hero_of_the_village` | 完整实现（含服务端集成） |
| VoluntaryExileTrigger | `minecraft:voluntary_exile` | 完整实现（含服务端集成） |
| ConsumeItemTrigger | `minecraft:consume_item` | 完整实现 |
| ItemDurabilityTrigger | `minecraft:item_durability_changed` | 完整实现 |
| EnchantedItemTrigger | `minecraft:enchanted_item` | 完整实现 |
| FilledBucketTrigger | `minecraft:filled_bucket` | 完整实现 |
| PlacedBlockTrigger | `minecraft:placed_block` | 完整实现（含服务端集成） |
| CuredZombieVillagerTrigger | `minecraft:cured_zombie_villager` | 完整实现（含服务端集成） |
| EffectsChangedTrigger | `minecraft:effects_changed` | 完整实现（含服务端集成） |

## 条件谓词

### ItemPredicate

物品匹配条件：
- 物品ID
- 数量范围
- 耐久范围
- 药水类型

### EntityPredicate

实体匹配条件：
- 实体类型
- 距离范围
- 位置

### DamageSourcePredicate

伤害源匹配条件（定义在 `EntityPredicate.hpp/cpp` 中）：
- 投射物标志（is_projectile）- 箭矢、三叉戟等
- 爆炸标志（is_explosion）- TNT、苦力怕等
- 火焰标志（is_fire）- 火焰、岩浆等
- 魔法标志（is_magic）- 药水、凋零等
- 闪电标志（is_lightning）- 闪电伤害
- 护甲穿透（bypasses_armor）- 溺水、摔落等
- 无敌穿透（bypasses_invulnerability）- 虚空伤害
- 魔法穿透（bypasses_magic）- 饥饿伤害

```cpp
// JSON 示例
{
  "is_fire": true,
  "bypasses_armor": false
}

// 代码示例
auto result = DamageSourcePredicate::fromJson(json);
if (result.success() && result.value().test(damageSource)) {
    // 伤害源匹配
}
```

**注意**：`EnvironmentalDamage` 的 `isProjectile()` 和 `isExplosion()` 始终返回 false，
投射物和爆炸伤害需要使用 `EntityDamageSource` 或 `IndirectEntityDamageSource`。

### LocationPredicate

位置匹配条件：
- 坐标范围
- 维度
- 生物群系

### BlockPredicate

方块匹配条件：
- 方块ID（通过 BlockRegistry 查找）
- 方块标签（通过 BlockTags 检查）
- 状态属性（复用 `mc::StatePropertiesPredicate`）

**注意**：BlockPredicate 复用了 `mc::StatePropertiesPredicate`（位于 `common/entity/loot/StatePropertiesPredicate.hpp`）
来实现状态属性匹配，避免代码重复。该类支持精确匹配和范围匹配。

### FluidPredicate

流体匹配条件（定义在 `BlockPredicate.hpp/cpp` 中）：
- 流体ID（通过 `Fluid::getFluid()` 获取）
- 状态属性（复用 `mc::StatePropertiesPredicate`）

流体匹配使用 `Fluid::isEquivalentTo()` 方法比较流体等效性，这意味着：
- `minecraft:water` 谓词同时匹配水源方块和流动水方块
- `minecraft:lava` 谓词同时匹配岩浆源方块和流动岩浆方块

```cpp
// JSON 示例
{
    "fluid": "minecraft:water"
}

// 代码示例
auto predicate = FluidPredicate::fromJson(json);
if (predicate.test(blockState)) {
    // 方块包含水
}
```

### MobEffectsPredicate

效果匹配条件（位于 `trigger/conditions/MobEffectsPredicate.hpp/cpp`）：
- 效果类型（如 `minecraft:speed`）
- 效果等级范围（amplifier）
- 持续时间范围（duration）
- 是否为环境效果（ambient）
- 是否显示粒子（visible）

```cpp
// JSON 示例
{
    "minecraft:speed": {
        "amplifier": {"min": 1},
        "duration": {"min": 200}
    },
    "minecraft:regeneration": {}
}

// 代码示例
auto result = MobEffectsPredicate::fromJson(json);
if (result.success() && result.value().test(livingEntity)) {
    // 实体拥有所需的效果
}
```

**注意**：只有 `LivingEntity` 有效果，非 `LivingEntity` 对效果谓词返回 false（除非谓词为空）。

## JSON 格式

成就 JSON 格式示例：

```json
{
  "parent": "minecraft:story/root",
  "display": {
    "icon": {
      "item": "minecraft:diamond"
    },
    "title": "Diamonds!",
    "description": "Acquire diamonds",
    "frame": "task",
    "show_toast": true,
    "announce_to_chat": true
  },
  "rewards": {
    "experience": 100
  },
  "criteria": {
    "diamond": {
      "trigger": "minecraft:inventory_changed",
      "conditions": {
        "items": [
          { "item": "minecraft:diamond" }
        ]
      }
    }
  },
  "requirements": [
    ["diamond"]
  ]
}
```

## 使用示例

### 加载成就

```cpp
AdvancementLoader loader;
auto result = loader.loadFromDirectory("data/minecraft/advancements");
if (result.success()) {
    spdlog::info("Loaded {} advancements", result.value().successCount);
}
```

### 查询成就

```cpp
auto& manager = AdvancementManager::instance();
auto advancement = manager.get(ResourceLocation("minecraft:story/mine_stone"));
if (advancement) {
    spdlog::info("Found: {}", advancement->getId().toString());
}
```

### 进度追踪

```cpp
AdvancementProgress progress(advancement);
progress.grantCriterion("diamond");
if (progress.isDone()) {
    spdlog::info("Advancement completed!");
}
```

## 待实现功能

1. **更多触发器** - LocationTrigger, PlayerKilledEntityTrigger 等
2. **网络同步** - AdvancementInfoPacket, SeenAdvancementsPacket
3. **客户端 UI** - AdvancementsScreen, AdvancementToast

## 服务端集成

服务端成就系统位于 `src/server/advancement/`，包含：
- **PlayerAdvancements** - 玩家成就进度管理
- **TriggerInstantiation** - 触发器实例化工具
- **AdvancementEventHandler** - 事件处理器，订阅服务端事件触发成就

详见 `src/server/advancement/README.md`。

## 设计参考

- Minecraft 1.16.5: `net.minecraft.advancements.*`
- Minecraft Wiki: https://minecraft.fandom.com/wiki/Advancement
