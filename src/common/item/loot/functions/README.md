# 战利品函数 (Loot Functions)

战利品函数用于修改生成的物品堆，如设置数量、应用附魔、创建探险地图等。函数在条件检查之后、物品返回之前按顺序执行。

## 目录结构

```
functions/
├── LootFunction.hpp/cpp             # 函数基类（条件管理、apply 接口）
├── LootFunctions.hpp                 # 所有函数的统一包含头文件
├── LootFunctionBuilder.hpp/cpp       # 函数构建器（工厂方法）
├── SetCountFunction.hpp/cpp          # minecraft:set_count（设置数量）
├── SetDamageFunction.hpp/cpp         # minecraft:set_damage（设置耐久度）
├── SetNameFunction.hpp/cpp           # minecraft:set_name（设置名称）
├── SetLoreFunction.hpp/cpp           # minecraft:set_lore（设置描述）
├── SetNbtFunction.hpp/cpp            # minecraft:set_nbt（设置NBT）
├── SetAttributesFunction.hpp/cpp     # minecraft:set_attributes（设置属性修饰符）
├── SetContentsFunction.hpp/cpp       # minecraft:set_contents（设置容器内容物）
├── SetLootTableFunction.hpp/cpp      # minecraft:set_loot_table（设置内嵌战利品表）
├── SetStewEffectFunction.hpp/cpp     # minecraft:set_stew_effect（设置迷之炖菜效果）
├── EnchantRandomlyFunction.hpp/cpp   # minecraft:enchant_randomly（随机附魔）
├── EnchantWithLevelsFunction.hpp/cpp # minecraft:enchant_with_levels（按等级附魔）
├── ApplyBonusFunction.hpp/cpp        # minecraft:apply_bonus（附魔加成）
├── LootingEnchantBonusFunction.hpp/cpp # minecraft:looting_enchant（抢夺附魔加成）
├── ExplosionDecayFunction.hpp/cpp    # minecraft:explosion_decay（爆炸衰减）
├── FurnaceSmeltFunction.hpp/cpp      # minecraft:furnace_smelt（熔炉熔炼）
├── CopyNameFunction.hpp/cpp          # minecraft:copy_name（复制名称）
├── CopyNbtFunction.hpp/cpp           # minecraft:copy_nbt（复制NBT）
├── CopyBlockStateFunction.hpp/cpp    # minecraft:copy_state（复制方块状态）
├── ExplorationMapFunction.hpp/cpp    # minecraft:exploration_map（探险地图）
├── FillPlayerHeadFunction.hpp/cpp    # minecraft:fill_player_head（填充玩家头颅）
└── LimitCountFunction.hpp/cpp        # minecraft:limit_count（限制数量范围）
```

## 上下游外部依赖关系

### 本模块依赖

```
loot/LootPool.hpp            # 函数的容器（池）
loot/context/LootContext.hpp # 掉落上下文（提供世界、随机数、位置等）
loot/context/LootParams.hpp  # 上下文参数定义（BLOCK_POS、THIS_ENTITY 等）
world/map/MapDecoration.hpp  # DecorationType 枚举（ExplorationMapFunction）
world/gen/structure/Structure.hpp # ResourceLocation（ExplorationMapFunction 结构定位）
```

### 外部对本模块的依赖

```
loot/LootSerializers.hpp/cpp  # 从 JSON 解析函数，调用各 _parse* 方法
loot/LootFunctionBuilder      # 通过工厂方法创建函数实例
```

## 容易踩的坑

### ExplorationMapFunction 的 destination 与 decoration 关系

`ExplorationMapFunction` 有两个相关字段：`destination`（目标结构类型）和 `decoration`（地图标记图标）。当 JSON 只指定 `destination` 时，`decoration` 为 `nullopt`，`getEffectiveDecoration()` 会从 destination 自动推导：Mansion→MANSION、Monument→MONUMENT、其余→RED_X。当 JSON 同时指定 `decoration` 时，显式值优先。

### ExplorationMapFunction 的 apply() 要求 BLOCK_POS

`apply()` 方法需要 `LootParams::BLOCK_POS` 参数来确定搜索起始位置。如果上下文中没有方块位置（例如从实体掉落而非宝箱），函数会原样返回物品，不执行搜索。

### FillPlayerHeadFunction 仅对玩家头颅物品生效

`FillPlayerHeadFunction::apply()` 首先检查物品是否为 `Items::PLAYER_HEAD`（引用相等性比较，与 MC Java 的 `stack.is(Items.PLAYER_HEAD)` 一致）。非玩家头颅物品（如骷髅头颅、凋灵骷髅头颅等）会被直接跳过，不写入 SkullOwner 标签。此检查依赖 `Items::PLAYER_HEAD` 在 `Items::initialize()` 中完成注册。
