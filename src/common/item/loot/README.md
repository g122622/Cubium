# 战利品表系统（Loot System）

Minecraft 战利品表系统实现，包含掉落物生成、条件判断、函数修饰、谓词系统等核心模块。

## 目录结构

```
loot/
├── LootTable.hpp/cpp              # 战利品表（生成掉落物的主入口）
├── LootPool.hpp/cpp               # 战利品池（包含条目和条件的容器）
├── LootSerializers.hpp/cpp        # JSON 序列化/反序列化器
├── LootTableManager.hpp/cpp       # 战利品表管理器（注册表）
├── LootTableLoader.hpp/cpp        # 战利品表加载器（从数据包加载）
├── LootPredicateManager.hpp/cpp   # 战利品谓词管理器（命名谓词注册表）
├── LootPredicateLoader.hpp/cpp    # 战利品谓词加载器（从数据包加载）
├── StatePropertiesPredicate.hpp/cpp # 方块状态谓词（条件匹配方块属性）
├── conditions/                    # 战利品条件
│   ├── LootCondition.hpp          # 条件基类
│   ├── LootConditions.hpp         # 所有条件的统一包含头文件
│   ├── LootConditionBuilder.hpp/cpp # 条件构建器（工厂模式）
│   ├── AndCondition.hpp/cpp       # minecraft:alternative（与条件）
│   ├── OrCondition.hpp/cpp        # minecraft:or（或条件）
│   ├── NotCondition.hpp/cpp       # minecraft:inverted（取反条件）
│   ├── RandomChanceCondition.hpp/cpp          # minecraft:random_chance
│   ├── RandomChanceWithLuckCondition.hpp/cpp  # minecraft:random_chance_with_looting
│   ├── ReferenceCondition.hpp/cpp             # minecraft:reference（引用命名谓词）
│   ├── BlockStateCondition.hpp/cpp            # minecraft:block_state_property
│   ├── ToolTypeCondition.hpp/cpp              # 内部工具类型条件（基于 HarvestTool）
│   ├── MatchToolCondition.hpp/cpp             # minecraft:match_tool（使用 ItemPredicate 完整匹配工具）
│   ├── FortuneCondition.hpp/cpp               # minecraft:fortune（时运加成）
│   ├── SilkTouchCondition.hpp/cpp             # minecraft:silk_touch
│   ├── SurvivesExplosionCondition.hpp/cpp     # minecraft:survives_explosion
│   ├── KilledByPlayerCondition.hpp/cpp        # minecraft:killed_by_player
│   ├── EntityPropertiesCondition.hpp/cpp      # minecraft:entity_properties
│   ├── EntityScoresCondition.hpp/cpp          # minecraft:entity_scores
│   ├── LocationCheckCondition.hpp/cpp         # minecraft:location_check
│   ├── WeatherCheckCondition.hpp/cpp          # minecraft:weather_check
│   ├── TimeCheckCondition.hpp/cpp             # minecraft:time_check
│   ├── DamageSourcePropertiesCondition.hpp/cpp # minecraft:damage_source_properties
│   ├── FishingOpenWaterCondition.hpp/cpp      # minecraft:fishing_hook_in_open_water
│   └── TableBonusCondition.hpp/cpp            # minecraft:table_bonus
├── context/                       # 战利品上下文
│   ├── LootContext.hpp/cpp        # 掉落上下文（包含生成掉落物所需的所有信息）
│   ├── LootContextBuilder.hpp/cpp # 掉落上下文构建器（流式 API）
│   ├── LootParameter.hpp          # 参数标识符模板
│   ├── LootParameterSet.hpp/cpp   # 参数集合（定义必需和可选参数）
│   ├── LootParameterSets.hpp/cpp  # 预定义参数集合（block、entity、chest、archaeology 等）
│   └── LootParams.hpp/cpp         # 常用参数定义（BLOCK_STATE、THIS_ENTITY 等）
├── entries/                       # 战利品条目
│   ├── LootEntry.hpp/cpp          # 条目基类
│   ├── LootEntries.hpp            # 所有条目的统一包含头文件
│   ├── LootEntryBuilder.hpp/cpp   # 条目构建器
│   ├── ItemLootEntry.hpp/cpp      # minecraft:item（物品条目）
│   ├── TableLootEntry.hpp/cpp     # minecraft:loot_table（嵌套掉落表条目）
│   ├── TagLootEntry.hpp/cpp       # minecraft:tag（标签条目）
│   ├── DynamicLootEntry.hpp/cpp   # minecraft:dynamic（动态条目）
│   ├── EmptyLootEntry.hpp/cpp     # minecraft:empty（空条目）
│   ├── AlternativesLootEntry.hpp/cpp # minecraft:alternatives（备选条目）
│   ├── GroupLootEntry.hpp/cpp     # minecraft:group（组合条目）
│   └── SequenceLootEntry.hpp/cpp  # minecraft:sequence（顺序条目）
└── functions/                     # 战利品函数
    ├── LootFunction.hpp/cpp       # 函数基类
    ├── LootFunctions.hpp          # 所有函数的统一包含头文件
    ├── LootFunctionBuilder.hpp/cpp # 函数构建器
    ├── SetCountFunction.hpp/cpp          # minecraft:set_count
    ├── SetDamageFunction.hpp/cpp         # minecraft:set_damage
    ├── SetNameFunction.hpp/cpp           # minecraft:set_name
    ├── SetLoreFunction.hpp/cpp           # minecraft:set_lore
    ├── SetNbtFunction.hpp/cpp            # minecraft:set_nbt
    ├── SetAttributesFunction.hpp/cpp     # minecraft:set_attributes
    ├── SetContentsFunction.hpp/cpp       # minecraft:set_contents
    ├── SetLootTableFunction.hpp/cpp      # minecraft:set_loot_table
    ├── SetStewEffectFunction.hpp/cpp     # minecraft:set_stew_effect
    ├── EnchantRandomlyFunction.hpp/cpp   # minecraft:enchant_randomly
    ├── EnchantWithLevelsFunction.hpp/cpp # minecraft:enchant_with_levels
    ├── ApplyBonusFunction.hpp/cpp        # minecraft:apply_bonus
    ├── LootingEnchantBonusFunction.hpp/cpp # minecraft:looting_enchant
    ├── ExplosionDecayFunction.hpp/cpp    # minecraft:explosion_decay
    ├── FurnaceSmeltFunction.hpp/cpp      # minecraft:furnace_smelt
    ├── CopyNameFunction.hpp/cpp          # minecraft:copy_name
    ├── CopyNbtFunction.hpp/cpp           # minecraft:copy_nbt
    ├── CopyBlockStateFunction.hpp/cpp    # minecraft:copy_state
    ├── ExplorationMapFunction.hpp/cpp    # minecraft:exploration_map
    ├── FillPlayerHeadFunction.hpp/cpp    # minecraft:fill_player_head
    └── LimitCountFunction.hpp/cpp        # minecraft:limit_count
```

## 核心架构

### 掉落物生成流程

```
LootTable::generate(context)
  └─ 遍历每个 LootPool
       ├─ 检查池条件 (testConditions)
       │    └─ 调用每个 LootCondition::test(context)
       │         └─ ReferenceCondition 通过 context.getPredicate() 查找命名谓词
       ├─ 计算 rolls 数量
       └─ 遍历条目
            ├─ 检查条目条件
            ├─ 生成基础物品
            └─ 应用 LootFunction 修饰
```

### 谓词系统

谓词（Predicate）是命名的 `LootCondition` 实例，存储在数据包的 `predicates/` 目录中：

```
数据包路径映射：
data/<namespace>/predicates/<path>.json → <namespace>:<path>
```

**关键组件**：

- **`LootPredicateManager`**：谓词注册表，管理 `id → LootCondition` 映射
- **`LootPredicateLoader`**：从数据包加载谓词 JSON 文件，委托 `LootSerializers::parseCondition()` 解析
- **`ReferenceCondition`**：`minecraft:reference` 条件类型，通过 `LootContext` 的谓词解析器查找命名谓词
- **`LootContext`**：提供 `PredicateResolver`、`getPredicate()`、`pushPredicate()/popPredicate()` 循环检测
- **`LootContextBuilder`**：提供 `withPredicateResolver()` 流式构建方法

**谓词查找链**：

```
ReferenceCondition::test(context)
  → context.getPredicate(name)
    → m_predicateResolver(name)  // 由 LootPredicateManager 支持
      → LootPredicateManager::getPredicate(name)
        → 返回命名的 LootCondition*
```

**循环引用检测**：

当谓词 A 引用谓词 B，而 B 又引用 A 时，会形成无限循环。`LootContext` 使用 `pushPredicate()/popPredicate()` 访问栈检测循环：

```
ReferenceCondition::test(context)
  → predicate = context.getPredicate(name)
  → if (!context.pushPredicate(predicate)) return false  // 检测到循环
  → result = predicate->test(context)                      // 可能触发嵌套引用
  → context.popPredicate(predicate)
  → return result
```

**集成方式**：

- `MinecraftServer` 在初始化时创建 `LootPredicateManager` 并通过 `LootPredicateLoader` 加载谓词
- `LootTableManager` 持有 `LootPredicateManager` 指针，提供 `getPredicate()` 便捷方法
- 所有 `LootContext` 构建点（`BlockDropHandler`、`LootCommand`、`Explosion` 等）均配置了谓词解析器

### 掉落表管理

- **`LootTableManager`**：掉落表注册表，同时持有 `LootPredicateManager` 指针以支持谓词查找
- **`LootTableLoader`**：从数据包加载掉落表 JSON 文件
- **`LootSerializers`**：统一的 JSON 序列化/反序列化器，支持所有条件和函数类型

## 上下游依赖

### 上游依赖

| 模块 | 依赖内容 |
|------|---------|
| `common/world/` | `IWorld`、`Block`、`BlockState`、`Fluid` |
| `common/entity/` | `Entity`、`LivingEntity`、`Player` |
| `common/resource/` | `ResourcePackList`、`DataPackList`、`ResourceLocation` |
| `common/util/math/random/` | `Random`、`RandomRanges` |
| `common/core/` | `Types`、`Result`、`ErrorCode` |

### 下游依赖

| 模块 | 依赖内容 |
|------|---------|
| `server/world/drop/` | `BlockDropHandler`（方块掉落物生成） |
| `server/command/` | `LootCommand`（/loot 命令） |
| `common/world/explosion/` | `Explosion`（爆炸掉落） |
| `common/world/blockentity/` | `LootableContainerBlockEntity`（容器随机战利品）、`BrushableBlockEntity`（考古刷扫掉落，使用 `archaeology` 参数集） |
| `common/entity/projectile/` | `OtherProjectiles`（投射物掉落） |
| `common/world/fluid/` | `WaterFluid`（钓鱼战利品） |

## 容易踩的坑

### 1. ReferenceCondition 需要谓词解析器

`ReferenceCondition::test()` 需要通过 `LootContext` 的谓词解析器查找命名谓词。如果构建 `LootContext` 时未设置谓词解析器（`withPredicateResolver()`），引用条件将始终返回 `false`。

### 2. 循环引用检测

谓词之间可以相互引用，但不应形成循环。`LootContext` 的 `pushPredicate()/popPredicate()` 机制会检测循环并返回 `false`，但不会抛出异常。在调试时需要注意 `spdlog::warn` 日志。

### 3. LootPredicateLoader 的 clearBeforeLoad

默认情况下，`LootPredicateLoader` 在加载前会清空已有谓词（`clearBeforeLoad=true`）。如果需要增量加载，需要显式设置 `loader.setClearBeforeLoad(false)`。

### 4. 掉落表嵌套引用

`TableLootEntry` 可以引用其他掉落表，形成嵌套结构。`LootContext` 使用 `pushLootTable()/popLootTable()` 检测掉落表的循环引用（与谓词循环检测机制类似）。

### 5. LootContext 参数验证

`LootContextBuilder::build(paramSet)` 在构建上下文时会执行参数验证（Empty 和 Generic 类型除外，因为它们没有必需参数也没有可选参数限制）：
1. 检查提供的参数中是否包含参数集不允许的参数（`unexpectedParams`）
2. 检查是否缺少参数集必需的参数（`missingParams`）
验证失败时不会中断构建，而是通过 `spdlog::warn` 记录缺失/多余的参数信息，便于调试。

### 6. MC 1.21+ 数据包目录命名兼容

MC 1.21+ 数据包使用单数目录名（`loot_table/`、`predicate/`），而旧版使用复数（`loot_tables/`、`predicates/`）。

- **LootTableLoader**：路径过滤和 `pathToLootTableId()` 同时匹配 `loot_table/` 和 `loot_tables/`，确保兼容两种目录命名。
- **LootPredicateLoader**：路径过滤和 `pathToLootPredicateId()` 同时匹配 `predicate/` 和 `predicates/`。
- **ItemTagLoader**：不受影响，使用 `listResourceStacks(namespace + "/tags/item")` 精确定位目录，不做路径子串匹配。
- **FunctionLoader**：同样已修复，同时匹配 `function/` 和 `functions/`。

### 7. `minecraft:loot_table` entry 的 value 字段（MC 1.21+）

MC 1.21+ 把 `minecraft:loot_table` 类型 entry 的标识字段从 `name` 改成了 `value`，且 `value` 是 `Either<ResourceKey, LootTable>`：

- 字符串：外部掉落表引用 ID（如 `"value": "minecraft:entities/sheep/white"`），由 `context.getLootTable()` 解析。
- 对象：内联完整掉落表（如 `"value": {"pools": [...]}`），解析期直接构造嵌套 `LootTable`，不走解析器。

`TableLootEntry` 用 `isInline()` 区分两种形态。**其他 entry（`minecraft:item`/`minecraft:tag`/`minecraft:dynamic`）的标识字段仍是 `name`，未改动。** 解析器只认 `value`，不再接受旧 `name`。
