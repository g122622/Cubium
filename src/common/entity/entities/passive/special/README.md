# 特殊动物

包含特殊行为的被动/中立动物。

## 目录结构

```
special/
├── BeeEntity.hpp/cpp         # 蜜蜂（授粉、蜂巢记忆、螫刺后死亡，实现 IFlyingAnimal + IAngerable）
├── FoxEntity.hpp/cpp         # 狐狸（信任机制、叼物品、皮肤变体）
├── PandaEntity.hpp/cpp       # 熊猫（7种性格基因、打喷嚏、打滚）
├── PolarBearEntity.hpp/cpp   # 北极熊（保护幼崽、站立警告，实现 IAngerable）
├── StriderEntity.hpp/cpp     # 炽足兽（熔岩行走、可骑乘，实现 IRideable + IEquipable）
├── TurtleEntity.hpp/cpp      # 海龟（出生地记忆、产卵）
└── README.md                 # 本文档
```

## 内部模块关系

```
AnimalEntity (passive/basic/AnimalEntity.hpp)
├── BeeEntity
│   ├── IFlyingAnimal (entity/interfaces/IFlyingAnimal.hpp)
│   └── IAngerable (entity/interfaces/IAngerable.hpp)
├── FoxEntity
│   └── 信任系统（独立实现，非 TameableEntity）
├── PandaEntity
│   └── 基因系统（主基因 + 隐藏基因）
├── PolarBearEntity
│   └── IAngerable (entity/interfaces/IAngerable.hpp)
├── StriderEntity
│   ├── IRideable (entity/interfaces/IRideable.hpp)
│   └── IEquipable (entity/interfaces/IEquipable.hpp)
└── TurtleEntity
    └── 出生地记忆系统
```

## 上下游外部依赖关系

### 依赖的上游模块
- `entity/core/` - Entity, LivingEntity, MobEntity, CreatureEntity, AgeableEntity, AnimalEntity 基类
- `entity/interfaces/` - IAngerable, IRideable, IEquipable, IFlyingAnimal 接口
- `entity/ai/` - Goal 系统（SwimGoal, PanicGoal, BreedGoal, TemptGoal, FollowParentGoal 等）
- `entity/attributes/` - 属性系统（MAX_HEALTH, MOVEMENT_SPEED, FOLLOW_RANGE 等）
- `entity/utils/ItemDropHelper.hpp` - 物品掉落工具
- `world/IWorld.hpp` - 世界接口
- `world/block/BlockPos.hpp` - 方块位置
- `block/BlockTags.hpp` - 方块标签（如 BlockTags::SAND 海龟产卵检测）
- `item/Items.hpp` - 物品定义
- `item/ItemTags.hpp` - 物品标签（如 ItemTags::FLOWERS 蜜蜂繁殖）

### 被下游模块依赖
- `entity/VanillaEntities.hpp` - 实体类型注册
- `server/` - 服务器端实体生成、AI 调度
- `client/` - 客户端实体渲染（模型、动画）
- `world/spawn/` - 生物群系生成时的实体放置

## 容易踩的坑

### BeeEntity 蜜蜂
1. **水下溺水**：蜜蜂无法在水下呼吸，需要追踪 `m_underWaterTimer`，超过 20 tick 后每 tick 造成溺水伤害。
2. **螫刺后死亡**：螫刺后蜜蜂会在 0-1200 tick 内随机死亡，概率随时间增加。
3. **愤怒系统**：实现 IAngerable 接口，愤怒时召唤周围蜜蜂群攻。

### FoxEntity 狐狸
1. **信任机制非 TameableEntity**：狐狸使用独立的信任系统（最多信任 2 个玩家），不继承 TameableEntity。
2. **叼物品**：狐狸可以叼起物品，需要正确处理 `m_heldItem` 的同步和掉落。
3. **睡眠状态**：白天睡觉、晚上活动，状态切换需要考虑被打断的情况。

### PandaEntity 熊猫
1. **基因表达规则**：好斗基因是显性的，懒惰+好斗组合也会表达为好斗。繁殖时子代基因需要从父母各随机继承一个，每个基因有 1/32 变异概率。
2. **打喷嚏**：幼年熊猫可能打喷嚏掉落粘液球（1/700 概率），需要检查 `doMobLoot` 游戏规则。
3. **打滚物理**：打滚持续 32 ticks，第 7、15、23 tick 执行小跳。

### PolarBearEntity 北极熊
1. **站立动画同步**：使用 `DATA_STANDING_PARAM` 同步站立状态，客户端需要插值计算 `clientSideStandAnimation` 实现平滑过渡。
2. **保护幼崽**：成年熊会攻击靠近幼熊的玩家，需要正确设置攻击目标。
3. **不可繁殖**：北极熊 `isBreedingItem()` 返回 false，`spawnBaby()` 返回 nullptr。

### StriderEntity 炽足兽
1. **熔岩行走**：需要正确设置 `onGround = true` 当在熔岩表面时，否则会沉入熔岩。
2. **寒冷状态**：离开熔岩会进入寒冷状态，速度大幅降低，需要追踪 `m_coldTimer`。
3. **鞍系统**：实现 IEquipable 接口，但只存储布尔值 `m_saddled`，不存储实际 ItemStack。死亡时需要检查并掉落鞍。
4. **骑乘偏移**：`getMountedYOffset()` 返回值包含步态动画波动，用于模拟行走起伏。

### TurtleEntity 海龟
1. **出生地继承**：幼龟孵化后需要继承父母的出生地位置，否则无法返回产卵。
2. **有蛋时不可繁殖**：`canBreed()` 需要额外检查 `!hasEgg()`。
3. **产卵检测**：`_layEgg()` 需要检测脚下是否为沙子类方块（BlockTags::SAND），当前位置是否为空气。
4. **水陆速度差异**：`travel()` 中水中速度正常，陆地速度减半（最低 0.06），幼体水中速度再降低。
