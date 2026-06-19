# GameRule 模块

游戏规则系统，用于控制 Minecraft 世界的行为参数（如 mobGriefing、doDaylightCycle 等）。

## 目录结构

```
gamerule/
├── GameRule.hpp/cpp     # 游戏规则基础类型：RuleKey（键）、RuleType（类型定义）、RuleValue（运行时值）
├── GameRules.hpp/cpp    # 游戏规则容器类，管理所有规则的运行时值，支持 NBT 序列化
└── README.md            # 本文件
```

## 内部模块关系

```
GameRule.hpp (基础类型定义)
    │
    ├── GameRuleKey<T> ──── 规则唯一标识符（名称 + 分类）
    │
    ├── GameRuleType<T> ─── 规则类型定义（默认值 + 变更监听器）
    │
    └── GameRuleValue<T> ── 规则运行时值（支持序列化）
           │
           ▼
GameRules.hpp (容器类)
    │
    ├── 持有所有 BooleanGameRuleValue 实例
    ├── 持有所有 IntegerGameRuleValue 实例
    ├── 提供统一的 get/set API
    └── NBT 序列化/反序列化
```

## 上下游外部依赖关系

### 本模块依赖

- `common/core/Types.hpp` - 基础类型定义（i32, u8 等）
- `common/util/nbt/Nbt.hpp` - NBT 序列化支持

### 依赖本模块的模块

- `common/world/IWorld.hpp` - 世界接口提供 `getGameRules()` 访问入口
- `server/world/ServerWorld.hpp` - 服务端世界持有 GameRules 实例
- `server/command/commands/GameRuleCommand` - `/gamerule` 命令实现
- **实体行为** - 检查 mobGriefing、doMobLoot 等规则：
  - `entity/ai/goal/goals/EatGrassGoal` - 草食行为检查 mobGriefing
  - `entity/ai/goal/goals/special/EndermanGoals` - 末影人搬方块检查 mobGriefing
  - `entity/ai/goal/goals/special/SilverfishGoals` - 蠹虫钻方块检查 mobGriefing
  - `entity/entities/boss/WitherEntity` - 凋灵破坏检查 mobGriefing
  - `entity/entities/monster/basic/CreeperEntity` - 苦力怕爆炸检查 mobGriefing
  - `entity/entities/monster/illager/RavagerEntity` - 劫掠兽破坏检查 mobGriefing
  - `entity/entities/passive/golem/SnowGolemEntity` - 雪傀儡放置雪检查 mobGriefing
  - `entity/entities/projectile/AbstractFireballEntity` - 火球点燃检查 mobGriefing
  - `entity/entities/misc/MiscEntities` - TNTEntity 爆炸检查 tntExplodes；实体掉落检查 doEntityDrops
  - `entity/entities/passive/special/PandaEntity` - 熊猫掉落检查 doMobLoot
  - `entity/entities/player/Player` - 玩家自然回血检查 naturalRegeneration
  - `entity/entities/vehicle/MinecartEntity` - TNT 矿车引爆和爆炸检查 tntExplodes
  - `entity/inventory/IRecipeHolder` - 限制合成检查 doLimitedCrafting
- **方块行为** - `block/blocks/mob/TurtleEggBlock` 检查 mobGriefing
- **TNT 行为** - 以下检查 tntExplodes：
  - `block/blocks/redstone/TNTBlock` - 点燃、爆炸和连锁爆炸检查 tntExplodes
  - `block/dispense/IDispenseItemBehavior` - 打火石发射器点燃 TNT 检查 tntExplodes
  - `block/dispense/DispenseItemBehaviorRegistry` - TNT 发射器行为检查 tntExplodes
- **降水系统** - `server/world/ServerWorld::tickPrecipitation()` 检查 snowAccumulationHeight

## 容易踩的坑

1. **规则值的字符串解析不严格**：`GameRuleValue::fromString()` 对布尔值接受 "true"/"TRUE"/"1" 和 "false"/"FALSE"/"0"，解析失败时会默认设为 false 并返回 false，调用方需要检查返回值。

2. **变更监听器需要 server 参数**：`GameRuleValue::set()` 只有在传入 `MinecraftServer*` 时才会触发变更监听器，单机模式下通过 `IWorld::getGameRules()` 获取的规则容器在设置时需要确保能获取到 server 实例。

3. **GameRules 容器的默认构造**：`IWorld` 基类提供了默认的 `getGameRules()` 实现（返回静态默认实例），派生类（如 `ServerWorld`）需要持有自己的 `GameRules` 成员并 override 这两个方法。

4. **规则键的唯一性**：`GameRuleKey` 通过名称字符串判等，不同规则键的名称不能重复，否则会覆盖。

5. **整数规则无范围校验**：`IntegerGameRuleValue` 的 `fromString()` 只做 `std::stoi` 解析，不做值域校验，负数或超大值都可能被接受。

6. **模板特化在 cpp 中**：`GameRuleValue<bool>` 和 `GameRuleValue<i32>` 的 `toString()`/`fromString()` 特化在 `GameRule.cpp` 中实现，链接时需要确保该编译单元被正确链接。

7. **snowAccumulationHeight 雪层积累规则**：`MAX_SNOW_ACCUMULATION_HEIGHT`（MC Java 对应 `snowAccumulationHeight`）是整数规则，控制雪层的最大堆积层数。值为 0 时不允许雪层放置，值为 1-8 时限制雪层的最大层数（SnowBlock 的 LAYERS 属性范围 1-8）。此规则影响：
   - `ServerWorld::tickPrecipitation()`：降雪时检查此规则决定是否放置雪层以及最大层数
   - `Biome::shouldSnow()`：仅检查温度/光照/支撑等条件，不检查此规则
   - 默认值为 1（与 MC Java 一致）
   - 注意：此规则只影响**降雪**时雪层的放置，不影响通过物品手动放置雪层
