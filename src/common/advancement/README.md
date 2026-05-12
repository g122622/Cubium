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
    │   └── BlockPredicate.hpp/cpp    # 方块匹配
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
| InventoryChangedTrigger | `minecraft:inventory_changed` | 框架完成，TODO: 实际检测逻辑 |
| TickTrigger | `minecraft:tick` | 完整实现 |
| RecipeUnlockedTrigger | `minecraft:recipe_unlocked` | 完整实现（2026-05-12） |
| EffectsChangedTrigger | `minecraft:effects_changed` | 框架完成，TODO: MobEffectsPredicate |
| BrewedPotionTrigger | `minecraft:brewed_potion` | 框架完成，TODO: 酿造系统集成 |

### RecipeUnlockedTrigger

配方解锁触发器，当玩家解锁配方时触发。

**条件参数**：
- `recipe` - 配方ID（可选，不指定则匹配任何配方）

**使用示例**：
```json
{
  "trigger": "minecraft:recipe_unlocked",
  "conditions": {
    "recipe": "minecraft:diamond_sword"
  }
}
```

**实现细节**：
- 通过 `ServerPlayer::unlockRecipe()` 方法触发
- 支持匹配特定配方或任何配方（`m_recipe.path().empty()` 时）
- 已在 `CriterionTriggers::registerBuiltinTriggers()` 中注册

**测试覆盖**：
- 条件实例创建和匹配测试
- JSON 反序列化测试
- 序列化测试
- "any" 配方匹配测试

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

### LocationPredicate

位置匹配条件：
- 坐标范围
- 维度
- 生物群系

### BlockPredicate

方块匹配条件：
- 方块ID
- 方块标签
- 状态属性

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
4. **事件集成** - 与服务端事件系统集成

## 设计参考

- Minecraft 1.16.5: `net.minecraft.advancements.*`
- Minecraft Wiki: https://minecraft.fandom.com/wiki/Advancement
