# Loot System - 掉落表系统

本目录实现了 Minecraft 1.16.5 风格的掉落表系统，用于控制实体死亡、方块破坏等情况下的物品掉落。

## 目录结构

```
src/common/entity/loot/
├── LootContext.hpp/cpp      # 掉落上下文，包含生成掉落物所需的所有信息
├── LootConditions.hpp/cpp   # 掉落条件，控制条目是否生效
├── LootFunctions.hpp/cpp    # 掉落函数，修改生成的物品堆
├── LootEntry.hpp/cpp        # 掉落条目，定义单个掉落项
├── LootPool.hpp/cpp         # 掉落池，按权重随机选择条目
├── LootTable.hpp/cpp        # 掉落表，管理多个池
├── LootSerializers.hpp/cpp  # JSON 序列化器，从 JSON 解析掉落表
├── StatePropertiesPredicate.hpp/cpp  # 方块状态属性匹配谓词
└── (RandomRanges 已移至 common/util/math/random/)
```

## 文件详解

### LootContext.hpp/cpp - 掉落上下文

**职责**: 包含生成掉落物所需的所有上下文信息，作为掉落生成过程的参数容器。

**核心类**:

| 类 | 描述 |
|---|---|
| `LootParameter<T>` | 类型安全的参数标识符模板类 |
| `LootParameterSet` | 参数集合，定义必需/可选参数 |
| `LootContext` | 掉落上下文，存储参数、随机数生成器、世界引用等 |
| `LootContextBuilder` | 构建器模式，简化 LootContext 创建 |

**预定义参数** (`LootParams` 命名空间):
- `THIS_ENTITY` - 当前实体
- `KILLER_PLAYER` - 击杀玩家
- `KILLER_ENTITY` - 击杀实体
- `DIRECT_KILLER` - 直接击杀者
- `DAMAGE_SOURCE` - 伤害来源
- `LUCK` - 幸运值
- `BLOCK_STATE` - 被破坏的方块状态
- `BLOCK_POS` - 方块位置
- `TOOL` - 使用的工具
- `FORTUNE_LEVEL` - 时运附魔等级
- `SILK_TOUCH_LEVEL` - 精准采集附魔等级

**关键功能**:
- 参数存储和访问（类型安全的 `get<T>()` / `set<T>()`）
- 拥有所有权的值存储（`setOwnedValue()` 用于简单值类型）
- 循环引用检测（`pushLootTable()` / `popLootTable()`）
- 掉落表解析器（支持嵌套掉落表引用）

**参考**: `net.minecraft.loot.LootContext`

---

### LootConditions.hpp/cpp - 掉落条件

**职责**: 定义控制掉落条目是否生效的条件系统。

**条件类型**:

| 条件 | 类型标识 | 描述 |
|------|---------|------|
| `SilkTouchCondition` | `silk_touch` | 精准采集附魔条件 |
| `FortuneCondition` | `fortune` | 时运附魔条件 |
| `RandomChanceCondition` | `random_chance` | 随机概率条件 |
| `RandomChanceWithLuckCondition` | `random_chance_with_luck` | 受幸运影响的随机概率 |
| `NotCondition` | `inverted` | 取反条件 |
| `AndCondition` | `alternative` | 与条件（所有条件都满足） |
| `OrCondition` | `or` | 或条件（任一条件满足） |
| `BlockStateCondition` | `block_state_property` | 方块状态条件（支持属性匹配） |
| `ToolTypeCondition` | `match_tool` | 工具类型条件 |

**BlockStateCondition 属性匹配**:

`BlockStateCondition` 支持通过 `StatePropertiesPredicate` 检查方块属性：

```cpp
// 创建属性匹配谓词
StatePropertiesPredicate properties;
properties.addExactMatch("lit", "true");       // 精确匹配
properties.addRangeMatch("age", "5", "7");     // 范围匹配

// 创建条件
BlockStateCondition condition("minecraft:redstone_lamp", std::move(properties));
```

**时运加成计算** (`FortuneCondition::applyFortuneBonus`):
```
MC 1.16.5 时运公式:
- Fortune I: 33% 概率 +1
- Fortune II: 25% 概率 +1, 25% 概率 +2
- Fortune III: 20% 概率 +1, 20% 概率 +2, 20% 概率 +3
```

**构建器**: `LootConditionBuilder` 提供流畅的静态工厂方法。

**参考**: `net.minecraft.loot.conditions.*`

---

### LootFunctions.hpp/cpp - 掉落函数

**职责**: 定义修改生成物品堆的函数系统，在条件检查之后、物品返回之前执行。

**函数类型**:

| 函数 | 类型标识 | 描述 |
|------|---------|------|
| `SetCountFunction` | `set_count` | 设置物品数量 |
| `ApplyBonusFunction` | `apply_bonus` | 时运加成（矿石掉落算法） |
| `LootingEnchantBonusFunction` | `looting_enchant` | 掠夺附魔加成 |
| `SetDamageFunction` | `set_damage` | 设置耐久度损坏 |
| `SetNameFunction` | `set_name` | 设置自定义名称 |
| `SetLoreFunction` | `set_lore` | 设置物品描述 |
| `LimitCountFunction` | `limit_count` | 限制数量范围 |
| `FurnaceSmeltFunction` | `furnace_smelt` | 熔炼函数 |
| `EnchantWithLevelsFunction` | `enchant_with_levels` | 按等级附魔 |
| `EnchantRandomlyFunction` | `enchant_randomly` | 随机附魔 |
| `ExplosionDecayFunction` | `explosion_decay` | 爆炸衰减函数 |
| `SetNbtFunction` | `set_nbt` | 设置NBT标签函数 |
| `CopyNameFunction` | `copy_name` | 复制名称函数 |
| `CopyBlockStateFunction` | `copy_block_state` | 复制方块状态函数 |
| `CopyNbtFunction` | `copy_nbt` | 复制NBT函数 |
| `FillPlayerHeadFunction` | `fill_player_head` | 填充玩家头颅函数 |
| `SetAttributesFunction` | `set_attributes` | 设置属性函数 |
| `SetContentsFunction` | `set_contents` | 设置内容物函数 |
| `SetLootTableFunction` | `set_loot_table` | 设置掉落表函数 |
| `ExplorationMapFunction` | `exploration_map` | 探险地图函数 |
| `SetStewEffectFunction` | `set_stew_effect` | 设置炖菜效果函数 |

**MC 1.16.5 时运矿石掉落算法** (`ApplyBonusFunction::calculateOreDrops`):
```
时运等级 > 0 时:
  i = random.nextInt(fortune + 2) - 1
  if (i < 0) i = 0
  return baseCount * (i + 1)

时运等级 = 0 时:
  return baseCount

示例:
  Fortune I: random.nextInt(3) - 1 → 结果范围 [0, 1, 2]
  Fortune II: random.nextInt(4) - 1 → 结果范围 [0, 1, 2, 3]
  Fortune III: random.nextInt(5) - 1 → 结果范围 [0, 1, 2, 3, 4]
```

**掠夺附魔加成** (`LootingEnchantBonusFunction`):
```
bonus = lootingLevel * count.generateFloat(random)
stack.grow(round(bonus))
if (limit > 0 && stack.count > limit) stack.count = limit
```

**构建器**: `LootFunctionBuilder` 提供流畅的静态工厂方法。

**时运集成状态**：
- `ApplyBonusFunction` 已实现三种时运公式（OreDrops、Uniform、Binomial）
- 通过 `LootContext` 的 `FORTUNE_LEVEL` 参数获取时运等级
- **已实现**：`LootEntry` 现在支持添加 `LootFunction`，函数在条件检查后、物品返回前执行
- **掉落表集成**：钻石矿、煤矿等矿石掉落表已使用 `ApplyBonusFunction` 支持时运加成

**参考**: `net.minecraft.loot.functions.*`

---

### LootEntry.hpp/cpp - 掉落条目

**职责**: 定义掉落表中的单个条目，可以是物品、空条目或掉落表引用。

**条目类型**:

| 条目 | 类型枚举 | 描述 |
|------|---------|------|
| `EmptyLootEntry` | `Empty` | 空条目，不生成物品 |
| `ItemLootEntry` | `Item` | 物品条目，生成指定物品 |
| `TableLootEntry` | `Table` | 掉落表引用条目 |
| `AlternativesLootEntry` | `Alternatives` | 替代条目，尝试多个直到成功 |
| `SequenceLootEntry` | `Sequence` | 序列条目，按顺序执行直到失败 |
| `GroupLootEntry` | `Group` | 组条目，执行所有子条目 |

**核心属性**:
- `weight` - 权重（用于加权随机选择）
- `quality` - 质量（幸运值加成系数）
- `conditions` - 条件列表（所有条件必须满足才能生成）
- `functions` - 函数列表（条件检查后、物品返回前执行，用于修改物品）

**函数执行流程**:
1. 条件检查 (`testConditions`)
2. 创建物品堆
3. 应用函数 (`applyFunctions`) - 按顺序执行，前一个输出作为后一个输入
4. 传递物品给消费者

**权重计算**: `effectiveWeight = weight + luck * quality`

**参考**: `net.minecraft.loot.LootEntry`

---

### LootPool.hpp/cpp - 掉落池

**职责**: 管理多个掉落条目，按权重随机选择。

**核心属性**:
- `rolls` - 掷骰次数范围（`RandomValueRange`）
- `bonusRolls` - 额外掷骰次数范围（受幸运值影响）
- `entries` - 条目列表

**生成逻辑**:
```cpp
// 计算掷骰次数 = 基础次数 + 幸运值加成
i32 rollCount = rolls.generateInt(random) + 
               static_cast<i32>(bonusRolls.generateFloat(random) * luck);

// 每次掷骰按权重选择条目
for (i32 i = 0; i < rollCount; ++i) {
    generateRoll(consumer, context);
}
```

**构建器**: `LootPoolBuilder` 提供流畅的构建接口。

**参考**: `net.minecraft.loot.LootPool`

---

### LootTable.hpp/cpp - 掉落表

**职责**: 管理多个掉落池，作为掉落系统的顶层容器。

**核心功能**:
- 池管理（添加、移除、查询）
- 参数集验证
- 循环引用检测
- 内置掉落表初始化

**内置掉落表** (`LootTableManager::initializeDefaultTables()`):
- 实体掉落: 猪、牛、羊、鸡
- 方块掉落: 钻石矿、石头、煤矿、铁矿、金矿、红石矿、青金石矿、圆石、下界金矿

**生成流程**:
```cpp
// 1. 创建上下文
auto context = LootContextBuilder(world)
    .withRandom(random)
    .withParameter(LootParams::BLOCK_STATE, &blockState)
    .withParameter(LootParams::TOOL, &tool)
    .build();

// 2. 生成掉落物
auto drops = lootTable.generate(*context);
```

**参考**: `net.minecraft.loot.LootTable`

---

### RandomRanges（已移动）

随机值范围工具类已移动到 `common/util/math/random/RandomRanges.hpp`。

**类**:

| 类 | 描述 |
|---|---|
| `mc::math::RandomValueRange` | 均匀分布随机范围 [min, max] |
| `mc::math::BinomialRange` | 二项分布范围，n 次试验，p 概率成功 |
| `mc::math::ConstantRange` | 固定值范围 |

**向后兼容**:
- `mc::loot::RandomValueRange` 等类型别名已保留，指向 `mc::math` 中的类。

**参考**: `net.minecraft.loot.RandomValueRange`

---

### LootSerializers.hpp/cpp - JSON 序列化器

**职责**: 提供 JSON 解析和序列化功能，完全兼容 Minecraft 1.16.5 数据包格式。

**核心类**:
- `LootSerializers` - 静态方法类，提供 JSON 解析和序列化

**解析方法**:

| 方法 | 描述 |
|-----|------|
| `parseRandomValueRange(json)` | 解析随机值范围（数字或范围对象） |
| `parseBinomialRange(json)` | 解析二项分布范围 |
| `parseConstantRange(json)` | 解析常量范围 |
| `parseRandomRange(json)` | 自动识别类型解析 IRandomRange |
| `parseCondition(json)` | 解析掉落条件 |
| `parseConditions(json)` | 解析条件数组 |
| `parseFunction(json)` | 解析掉落函数 |
| `parseFunctions(json)` | 解析函数数组 |
| `parseEntry(json)` | 解析掉落条目 |
| `parseEntries(json)` | 解析条目数组 |
| `parsePool(json)` | 解析掉落池 |
| `parsePools(json)` | 解析池数组 |
| `parseLootTable(json)` | 解析掉落表 |
| `toJson(range)` | 序列化范围到 JSON |
| `toJson(condition)` | 序列化条件到 JSON |
| `toJson(function)` | 序列化函数到 JSON |
| `toJson(entry)` | 序列化条目到 JSON |
| `toJson(pool)` | 序列化池到 JSON |
| `toJson(table)` | 序列化掉落表到 JSON |

**支持的条件类型**:
- `minecraft:silk_touch` → SilkTouchCondition
- `minecraft:table_bonus` / `minecraft:fortune` → FortuneCondition
- `minecraft:random_chance` → RandomChanceCondition
- `minecraft:random_chance_with_looting` → RandomChanceWithLuckCondition
- `minecraft:inverted` → NotCondition
- `minecraft:alternative` → OrCondition
- `minecraft:block_state_property` → BlockStateCondition
- `minecraft:match_tool` → ToolTypeCondition
- `minecraft:killed_by_player` → 占位实现
- `minecraft:entity_properties` → 占位实现
- `minecraft:survives_explosion` → 占位实现

**支持的函数类型**:
- `minecraft:set_count` → SetCountFunction
- `minecraft:apply_bonus` → ApplyBonusFunction
- `minecraft:looting_enchant` → LootingEnchantBonusFunction
- `minecraft:set_damage` → SetDamageFunction
- `minecraft:set_name` → SetNameFunction
- `minecraft:set_lore` → SetLoreFunction
- `minecraft:limit_count` → LimitCountFunction
- `minecraft:furnace_smelt` → FurnaceSmeltFunction
- `minecraft:enchant_with_levels` → EnchantWithLevelsFunction
- `minecraft:enchant_randomly` → EnchantRandomlyFunction
- `minecraft:explosion_decay` → ExplosionDecayFunction
- `minecraft:set_nbt` → SetNbtFunction
- `minecraft:copy_name` → CopyNameFunction
- `minecraft:copy_block_state` → CopyBlockStateFunction
- `minecraft:copy_nbt` → CopyNbtFunction
- `minecraft:fill_player_head` → FillPlayerHeadFunction
- `minecraft:set_attributes` → SetAttributesFunction
- `minecraft:set_contents` → SetContentsFunction
- `minecraft:set_loot_table` → SetLootTableFunction
- `minecraft:exploration_map` → ExplorationMapFunction
- `minecraft:set_stew_effect` → SetStewEffectFunction

**支持的条目类型**:
- `minecraft:empty` → EmptyLootEntry
- `minecraft:item` → ItemLootEntry
- `minecraft:loot_table` → TableLootEntry
- `minecraft:alternatives` → AlternativesLootEntry
- `minecraft:sequence` → SequenceLootEntry
- `minecraft:group` → GroupLootEntry

**JSON 示例**:
```json
{
  "type": "minecraft:block",
  "pools": [
    {
      "rolls": 1,
      "bonus_rolls": {
        "min": 0,
        "max": 1
      },
      "entries": [
        {
          "type": "minecraft:item",
          "name": "minecraft:diamond",
          "weight": 10,
          "functions": [
            {
              "function": "minecraft:set_count",
              "count": {
                "min": 1,
                "max": 3
              }
            }
          ],
          "conditions": [
            {
              "condition": "minecraft:random_chance",
              "chance": 0.5
            }
          ]
        }
      ]
    }
  ]
}
```

**参考**: `net.minecraft.loot.LootSerializers`

---

## 类关系图

```
┌─────────────────────────────────────────────────────────────┐
│                      LootTableManager                        │
│  - 管理所有注册的掉落表                                        │
│  - 提供内置掉落表初始化                                        │
└─────────────────────────────────────────────────────────────┘
                              │
                              │ 管理
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                     LootSerializers                          │
│  - 从 JSON 解析掉落表、池、条目、条件、函数                     │
│  - 序列化掉落表到 JSON                                         │
│  - 兼容 MC 1.16.5 数据包格式                                   │
└─────────────────────────────────────────────────────────────┘
                              │
                              │ 解析/序列化
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                        LootTable                             │
│  - 管理多个 LootPool                                         │
│  - 执行循环引用检测                                           │
│  - 提供构建器 LootTableBuilder                               │
│  - 管理掉落函数列表                                           │
└─────────────────────────────────────────────────────────────┘
                              │
                              │ 包含
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                        LootPool                              │
│  - 定义掷骰次数（rolls + bonusRolls）                        │
│  - 按权重选择条目                                             │
│  - 提供构建器 LootPoolBuilder                                │
│  - 管理掉落函数列表                                           │
└─────────────────────────────────────────────────────────────┘
                              │
                              │ 包含
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                        LootEntry                             │
│  - 定义权重（weight）和质量（quality）                       │
│  - 管理条件列表                                               │
│  ┌─────────────────────────────────────────────────────────┐│
│  │ 子类:                                                    ││
│  │  - EmptyLootEntry    (空条目)                           ││
│  │  - ItemLootEntry     (物品条目)                         ││
│  │  - TableLootEntry    (掉落表引用)                       ││
│  │  - AlternativesLootEntry (替代条目)                     ││
│  │  - SequenceLootEntry  (序列条目)                        ││
│  │  - GroupLootEntry     (组条目)                          ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │
                              │ 使用条件和函数
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                     LootCondition                            │
│  - 测试条件是否满足                                          │
│  ┌─────────────────────────────────────────────────────────┐│
│  │ 子类:                                                    ││
│  │  - SilkTouchCondition     (精准采集)                    ││
│  │  - FortuneCondition       (时运)                        ││
│  │  - RandomChanceCondition  (随机概率)                    ││
│  │  - RandomChanceWithLuckCondition (受幸运影响)           ││
│  │  - NotCondition           (取反)                        ││
│  │  - AndCondition           (与)                          ││
│  │  - OrCondition            (或)                          ││
│  │  - BlockStateCondition    (方块状态)                    ││
│  │  - ToolTypeCondition      (工具类型)                    ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                     LootFunction                             │
│  - 修改生成的物品堆                                          │
│  ┌─────────────────────────────────────────────────────────┐│
│  │ 子类:                                                    ││
│  │  - SetCountFunction       (设置数量)                    ││
│  │  - ApplyBonusFunction     (时运加成)                    ││
│  │  - LootingEnchantBonusFunction (掠夺加成)               ││
│  │  - SetDamageFunction      (设置耐久度)                  ││
│  │  - SetNameFunction        (设置名称)                    ││
│  │  - SetLoreFunction        (设置描述)                    ││
│  │  - LimitCountFunction     (限制数量)                    ││
│  │  - FurnaceSmeltFunction   (熔炼)                        ││
│  │  - EnchantWithLevelsFunction (等级附魔)                 ││
│  │  - EnchantRandomlyFunction (随机附魔)                   ││
│  │  - ExplosionDecayFunction  (爆炸衰减)                   ││
│  │  - SetNbtFunction         (设置NBT标签)                 ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │
                              │ 依赖
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      LootContext                             │
│  - 存储 IWorld 引用和 Random 引用                           │
│  - 存储各种参数（实体、工具、方块等）                         │
│  - 提供幸运值和掠夺附魔等级                                   │
│  - 提供掉落表解析器                                          │
│  - 循环引用检测栈                                            │
└─────────────────────────────────────────────────────────────┘
```

---

## 模块职责

### 整体职责

掉落表系统负责在游戏中的各种场景（实体死亡、方块破坏、钓鱼、宝箱等）生成物品掉落。该系统提供：

1. **声明式掉落规则** - 通过组合条件、条目和池定义复杂的掉落逻辑
2. **权重随机选择** - 基于权重的加权随机，支持幸运值加成
3. **条件系统** - 灵活的条件判断（附魔、概率、方块类型等）
4. **嵌套引用** - 支持掉落表引用其他掉落表
5. **循环引用检测** - 防止无限递归

### 输入和输出

**输入**:
- `LootContext` - 包含世界、随机数生成器、参数（实体、工具、方块等）
- 掉落表定义（可通过代码构建或 JSON 加载）

**输出**:
- `std::vector<ItemStack>` - 生成的物品堆列表

---

## 依赖项

### 外部依赖

| 依赖 | 用途 |
|-----|------|
| `common/core/Types.hpp` | 基础类型定义 |
| `common/core/Result.hpp` | 结果类型 |
| `common/util/math/random/Random.hpp` | 随机数生成器 |
| `common/item/ItemStack.hpp` | 物品堆 |
| `common/item/ItemRegistry.hpp` | 物品注册表 |
| `common/item/tool/ToolItem.hpp` | 工具物品 |
| `common/world/IWorld.hpp` | 世界接口 |
| `common/world/block/Block.hpp` | 方块定义 |
| `common/world/block/BlockPos.hpp` | 方块位置 |

### 内部依赖

```
LootContext.hpp   ←──┐
LootConditions.hpp ←─┤
LootEntry.hpp ←──────┤
LootPool.hpp ←───────┤
LootTable.hpp ←──────┘
(RandomRanges.hpp 已移至 common/util/math/random/)
```

---

## 使用方法

### 基本用法

```cpp
#include "entity/loot/LootTable.hpp"
#include "entity/loot/LootPool.hpp"
#include "entity/loot/LootEntry.hpp"
#include "entity/loot/LootConditions.hpp"
#include "entity/loot/LootContext.hpp"

using namespace mc::loot;

// 1. 创建掉落表
LootTable table;

// 2. 创建池
auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 3.0f));  // 1-3 次掷骰

// 3. 创建条目
auto diamond = std::make_unique<ItemLootEntry>(
    "minecraft:diamond",              // 物品ID
    RandomValueRange(1.0f, 2.0f),    // 数量范围
    10,                               // 权重
    1                                 // 质量（幸运值加成）
);

// 4. 添加条件（可选）
diamond->addCondition(std::make_unique<SilkTouchCondition>());

pool->addEntry(std::move(diamond));
table.addPool(std::move(pool));

// 5. 创建上下文
auto context = LootContextBuilder(world)
    .withRandom(random)
    .withLuck(1.0f)
    .withParameter(LootParams::BLOCK_STATE, &blockState)
    .withParameter(LootParams::TOOL, &tool)
    .build();

// 6. 生成掉落物
auto drops = table.generate(*context);
for (const auto& drop : drops) {
    // 处理掉落物
}
```

### 使用构建器

```cpp
// 使用 LootTableBuilder
auto table = LootTableBuilder()
    .id("minecraft:blocks/diamond_ore")
    .pool(LootPoolBuilder()
        .rolls(1)
        .item("minecraft:diamond_ore", 1, 1)
        .build())
    .build();
```

### 使用 LootTableManager

```cpp
LootTableManager manager;
manager.initializeDefaultTables();

// 获取内置掉落表
const LootTable* pigDrops = manager.getTable("minecraft:entities/pig");
if (pigDrops) {
    auto drops = pigDrops->generate(*context);
}
```

### 条件组合示例

```cpp
// 复杂条件：精准采集 OR 铁镐以上
auto ironPickaxeOrBetter = LootConditionBuilder::or_({
    LootConditionBuilder::silkTouch(),
    LootConditionBuilder::toolType(static_cast<u8>(HarvestTool::Pickaxe))
});

// 无精准采集时才掉落普通矿石
auto normalDrop = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f));
normalDrop->addCondition(
    LootConditionBuilder::not_(LootConditionBuilder::silkTouch())
);
```

---

## 容易踩的坑

### 1. 参数生命周期

`LootContext::set()` 存储的是指针，调用方需要确保参数在 `LootContext` 生命周期内有效。对于简单值类型，应使用 `setOwnedValue()`:

```cpp
// 错误：临时值会失效
context.set(LootParams::FORTUNE_LEVEL, &fortuneLevel);  // fortuneLevel 需要在 context 生命周期内有效

// 正确：使用 setOwnedValue 存储值副本
context.setOwnedValue(LootParams::FORTUNE_LEVEL, 3);
```

### 2. 循环引用

当使用 `TableLootEntry` 引用其他掉落表时，可能导致循环引用。系统已内置检测机制，但需要正确设置掉落表解析器:

```cpp
context->setLootTableResolver([&manager](const std::string& id) -> const LootTable* {
    return manager.getTable(id);
});
```

### 3. 权重为 0 的条目

权重为 0 或负数的条目不会被选择:

```cpp
// 这个条目永远不会被选择
auto entry = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 0, 0);
```

### 4. 空池或空表

空池或空表不会生成任何物品，但不会报错:

```cpp
LootTable emptyTable;
auto drops = emptyTable.generate(*context);  // drops 为空
```

### 5. 随机数种子

使用相同种子会得到相同结果（用于测试或回放）:

```cpp
math::Random rng1(12345);
math::Random rng2(12345);
// rng1 和 rng2 会生成相同的随机序列
```

### 6. JSON 解析已实现

`LootTable::fromJson()` 和 `LootTable::toJson()` 已实现，支持从 JSON 加载掉落表：

```cpp
// 从 JSON 字符串解析
auto result = LootTable::fromJson(jsonStr);
if (result.success()) {
    auto table = result.value();
    // 使用掉落表
} else {
    // 处理错误
    std::cerr << result.error().message() << std::endl;
}

// 序列化为 JSON 字符串
std::string json = table->toJson();
// 或带格式化
std::string prettyJson = table->toJson(2);
```

支持解析的 JSON 格式完全兼容 Minecraft 1.16.5 数据包格式。

**支持的 JSON 字段**:
- `type` - 参数集类型
- `pools` - 掉落池数组
  - `rolls` - 掷骰次数（数字或范围对象）
  - `bonus_rolls` - 额外掷骰次数
  - `entries` - 条目数组
    - `type` - 条目类型（`item`, `empty`, `loot_table`, `alternatives`, `sequence`, `group`）
    - `name` - 物品/掉落表ID
    - `weight` - 权重
    - `quality` - 质量
    - `count` - 数量范围
    - `conditions` - 条件数组
    - `children` - 子条目（用于组合条目）
  - `functions` - 函数数组（Pool级别，当前未支持）

### 条件 JSON 格式

### 7. 条件在条目生成时检查

条件在 `LootEntry::generate()` 时检查，而不是在 `expand()` 时:

```cpp
// expand() 将条目添加到候选列表
// generate() 时才检查条件
entry.expand(context, [](LootEntry& e) { /* e 会被添加 */ });
entry.generate(consumer, context);  // 条件在这里检查
```

---

## 测试用例

### 测试文件

- `tests/common/entity/loot/LootTest.cpp` - 核心功能测试
- `tests/common/entity/loot/LootConditionTest.cpp` - 条件系统测试
- `tests/common/entity/loot/LootSerializersTest.cpp` - JSON 序列化测试

### 测试覆盖

| 测试类别 | 测试内容 |
||---------|---------|
| RandomValueRange | 固定值、范围值生成 |
| BinomialRange | 二项分布生成、边界条件 |
| LootContext | 构建器、幸运值、掠夺附魔 |
| EmptyLootEntry | 空条目生成 |
| ItemLootEntry | 权重、质量计算 |
| LootTable | 空表、池管理 |
| LootTableManager | 注册、查询、内置表 |
| RandomChanceCondition | 概率测试 |
| NotCondition | 取反逻辑 |
| AndCondition | 与逻辑、空条件 |
| OrCondition | 或逻辑、空条件 |
| FortuneCondition | 等级获取、加成计算 |
| BlockStateCondition | 方块ID匹配、属性匹配、属性不匹配、构建器方法 |
| StatePropertiesPredicate | 空谓词、布尔属性匹配、整数属性匹配、多属性匹配、克隆、JSON序列化 |
| LootConditionBuilder | 工厂方法 |
| EntryCondition | 条件克隆、多条件 |
| PoolCondition | 池条件测试 |
| EdgeCases | 边界情况 |
| CopyNameFunction | 创建、克隆、来源类型 |
| CopyBlockStateFunction | 创建、属性、克隆 |
| CopyNbtFunction | 创建、操作添加、克隆 |
| FillPlayerHeadFunction | 创建、克隆 |
| SetAttributesFunction | 创建、修饰符添加 |
| SetContentsFunction | 创建 |
| SetLootTableFunction | 创建、克隆 |
| ExplorationMapFunction | 创建、目的地类型 |
| SetStewEffectFunction | 创建、效果添加 |
| LootFunctionBuilder | 所有新函数工厂方法 |
| LootSerializers | RandomValueRange/IRandomRange 解析、条件解析、函数解析、条目解析、池解析、掉落表解析、序列化、往返测试、BlockStateCondition JSON解析（含properties字段） |

### 运行测试

```powershell
./build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter="Loot*"
```

---

## 参考实现

本系统参考 Minecraft Java Edition 1.16.5 的掉落表系统:

- `net.minecraft.loot.LootTable`
- `net.minecraft.loot.LootPool`
- `net.minecraft.loot.LootEntry`
- `net.minecraft.loot.LootContext`
- `net.minecraft.loot.conditions.*`
- `net.minecraft.loot.RandomValueRange`

---

## 未来计划

1. ~~**JSON 解析** - 实现从数据包加载掉落表~~ ✅ 已完成
2. **更多条件** - 添加实体属性、生物群系、天气等条件
3. **更多条目** - 标签条目、动态条目
4. **缓存优化** - 掉落表缓存和预编译
5. **桩实现完善** - 完善以下函数的实际实现（当前为桩）：
   - ~~CopyNameFunction~~ ✅ 已完成（从实体/方块实体复制名称）
   - ~~CopyBlockStateFunction~~ ✅ 已完成（复制 BlockState 属性到 ItemStack 的 BlockStateTag）
   - CopyNbtFunction（需要 NBT 路径解析）
   - FillPlayerHeadFunction（需要玩家皮肤系统）
   - SetAttributesFunction（需要属性系统）
   - SetContentsFunction（需要容器物品支持）
   - ExplorationMapFunction（需要地图数据系统）
   - SetStewEffectFunction（需要药水效果系统）
   - ~~SetLootTableFunction~~ ✅ 已完成（设置掉落表到 BlockEntityTag）
6. ~~**FurnaceSmeltFunction 熔炼函数** - 已通过 RecipeManager 实现完整功能~~ ✅ 已完成
7. **JSON 函数解析** - 为 Pool 和 Table 级别的函数提供完整支持
