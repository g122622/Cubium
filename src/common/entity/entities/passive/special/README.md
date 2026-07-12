# 特殊动物

包含特殊行为的被动/中立动物。

## 目录结构

```
special/
├── BeeEntity.hpp/cpp         # 蜜蜂（授粉、蜂巢记忆、螫刺后死亡，实现 IFlyingAnimal + IAngerable）
├── FoxEntity.hpp/cpp         # 狐狸（信任机制、叼物品、皮肤变体）
├── PandaEntity.hpp/cpp       # 熊猫（7种性格基因、打喷嚏、打滚）
├── PolarBearEntity.hpp/cpp   # 北极熊（保护幼崽、站立警告，实现 IAngerable）
├── SnifferEntity.hpp/cpp     # 嗅探兽（蛋孵化获得、状态机、繁殖掉蛋，幼年期 48000 tick）
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
├── SnifferEntity
│   ├── 状态机（Idling/FeelingHappy/Scenting/Sniffing/Searching/Digging/Rising）
│   ├── setChild 覆盖（幼年期 48000 tick，是普通动物的两倍）
│   └── 繁殖掉落嗅探兽蛋物品（当前为 spawnBaby 占位实现）
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
- `entity/effect/` - 状态效果系统（`EffectType::Poison`、`EffectInstance`，被 `EyeblossomBlock::onEntityCollision` 通过 `BeeEntity` 施加）
- `entity/utils/ItemDropHelper.hpp` - 物品掉落工具
- `world/IWorld.hpp` - 世界接口
- `world/block/BlockPos.hpp` - 方块位置
- `block/BlockTags.hpp` - 方块标签（`BlockTags::SAND` 海龟产卵检测、`BlockTags::BEE_ATTRACTIVE` 蜜蜂吸引物判定、`BlockTags::BEEHIVES` 蜂巢验证）
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
4. **服务端冷却递减**：`tick()` 中的三个冷却计时器（`m_stayOutOfHiveCountdown`、`m_remainingCooldownBeforeLocatingNewHive`、`m_remainingCooldownBeforeLocatingNewFlower`）仅在服务端递减（`!m_world->isClientSide()` 守卫），客户端保持不变。
5. **授粉状态管理**：`BeePollinateGoal::startExecuting()` 调用 `setPollinating(true)`，`resetTask()` 调用 `setPollinating(false)` 并设置花朵冷却 200 tick。
6. **天气/夜间回巢逻辑（BEES_STAY_IN_HIVE 等效）**：MC 原版通过 `EnvironmentAttributes.BEES_STAY_IN_HIVE` 统一管理蜜蜂在雨天/雷暴/夜间回巢的行为，当前项目使用 `isRaining()`/`isThundering()`/`!isDaytime()` 等效替代。该逻辑体现在两处：
   - `BeeEntity::wantsToEnterHive()`：雨天/雷暴/夜间蜜蜂想回巢
   - `BeehiveBlockEntity::_releaseOccupant()`：雨天/雷暴/夜间不放出蜜蜂（紧急释放除外）
7. **寻找花蜜阈值**：`isTiredOfLookingForNectar()` 使用 3600 tick 阈值（MC 原版值），`BeeFindFlowerGoal` 使用 600 tick 阈值触发寻找已知花朵（MC 原版 `wantsToGoToKnownFlower` 逻辑）。
8. **导航方法**：`pathfindRandomlyTowards()` 和 `pathfindDirectlyTowards()` 实现了蜜蜂特有的漂移飞行导航：
   - `pathfindRandomlyTowards()` 对应 MC 的 `Bee.pathfindRandomlyTowards()`，在目标方向 18 度锥形内生成随机空中航点（使用 `findAirPositionTowards`），近距离时自动缩小搜索范围，产生蜜蜂漂移飞行效果。被 `BeeFindHiveGoal`（远距离）和 `BeeFindFlowerGoal` 调用。
   - `pathfindDirectlyTowards()` 对应 MC 的 `BeeGoToHiveGoal.pathfindDirectlyTowards()`，近距离（16 格内）精确导航到蜂巢，3 格内用 1 倍速度否则 2 倍速度。被 `BeeFindHiveGoal` 调用。
   - **注意**：`PathNavigator` 已实现 `setMaxVisitedNodesMultiplier()` / `resetMaxVisitedNodesMultiplier()`，蜜蜂导航方法已正确使用：`pathfindRandomlyTowards()` 设置 0.5F（降低寻路开销），`pathfindDirectlyTowards()` 设置 10.0F（提高寻路精度），Goal 的 resetTask() 中重置为 1.0F。
9. **花朵吸引判定（attractsBees）**：`BeeEntity::attractsBees(const BlockState&)` 是 MC 1.21.11 `Bee.attractsBees` 的对应实现，用于判定方块是否吸引蜜蜂：
   - 依赖 `BlockTags::BEE_ATTRACTIVE` 标签（包含蒲公英、开放眼眸花、向日葵等 29 种花朵，闭合眼眸花**不在**此标签中）
   - 含水（`waterlogged=true`）的可水合花朵不吸引蜜蜂
   - 向日葵仅上半部分（`DoubleBlockHalf::Upper`）吸引蜜蜂
   - 被 `EyeblossomBlock::onEntityCollision` 用于判定蜜蜂是否在接触眼眸花时中毒（开放眼眸花 → 25 tick Poison I，闭合眼眸花不触发）
   - 后续蜜蜂 AI 的授粉目标过滤、繁殖物品判定等也可复用此方法，避免重复实现标签 + 含水 + 向日葵半身的过滤逻辑

### FoxEntity 狐狸
1. **信任机制非 TameableEntity**：狐狸使用独立的信任系统（最多信任 2 个玩家），不继承 TameableEntity。
2. **叼物品**：狐狸可以叼起物品，需要正确处理 `m_heldItem` 的同步和掉落。
3. **睡眠状态**：白天睡觉、晚上活动，状态切换需要考虑被打断的情况。
4. **猎物目标选择器**：狐狸的目标选择器已注册以下猎物攻击目标：
   - 优先级 3: `FoxRevengeGoal` — 保卫信任玩家，当信任玩家被攻击时反击
   - 优先级 4: `NearestAttackableTargetGoal<LivingEntity>` — 攻击小鸡和兔子（通过谓词过滤）
   - 优先级 4: `NearestAttackableTargetGoal<TurtleEntity>` — 攻击幼年海龟（仅陆地上的幼体）
   - 优先级 6: `NearestAttackableTargetGoal<LivingEntity>` — 攻击群居鱼类（鳕鱼、鲑鱼、热带鱼）

### PandaEntity 熊猫
1. **基因表达规则**：好斗基因是显性的，懒惰+好斗组合也会表达为好斗。繁殖时子代基因需要从父母各随机继承一个，每个基因有 1/32 变异概率。
2. **打喷嚏**：幼年熊猫可能打喷嚏掉落粘液球（1/700 概率），需要检查 `doMobLoot` 游戏规则。
3. **打滚物理**：打滚持续 32 ticks，第 7、15、23 tick 执行小跳。

### PolarBearEntity 北极熊
1. **站立动画同步**：使用 `DATA_STANDING_PARAM` 同步站立状态，客户端需要插值计算 `m_clientSideStandAnimation` 实现平滑过渡。
2. **动态碰撞箱**：`getDimensions()` 被重写，站立动画期间高度随动画进度逐渐增大（`baseHeight * (1.0 + animationProgress / 6.0)`），完全站立时高度翻倍（1.4→2.8）。客户端 `tick()` 中动画值变化时调用 `refreshDimensions()` 更新碰撞箱。
3. **基础尺寸**：`getBaseWidth()` 返回 1.4，`getBaseHeight()` 返回 1.4，眼高为 `1.4 * 0.85 = 1.19`（幼熊按 `BABY_SCALE` 缩放）。
4. **保护幼崽**：成年熊会攻击靠近幼熊的玩家，需要正确设置攻击目标。
5. **不可繁殖**：北极熊 `isBreedingItem()` 返回 false，`spawnBaby()` 返回 nullptr。

### StriderEntity 炽足兽
1. **熔岩行走**：需要正确设置 `onGround = true` 当在熔岩表面时，否则会沉入熔岩。
2. **寒冷状态**：离开熔岩会进入寒冷状态，速度大幅降低，需要追踪 `m_coldTimer`。
3. **鞍系统**：实现 IEquipable 接口，但只存储布尔值 `m_saddled`，不存储实际 ItemStack。死亡时需要检查并掉落鞍。
4. **骑乘偏移**：`getMountedYOffset()` 返回值包含步态动画波动，用于模拟行走起伏。
5. **玩家交互**（`interactMob()`）：完整实现了 MC 原版 Strider.mobInteract 的交互优先级链：
   - 非食物 + 已装备鞍 + 无乘客 + 玩家未蹲下 → 玩家骑乘（返回 Success）
   - 手持食物（诡异菌）→ 喂食逻辑：成年可繁殖 → 进入爱心模式 + 播放吃食音效；幼年 → 加速成长 + 播放吃食音效；成年已爱心 → 服务端返回 Pass / 客户端返回 Consume
   - 手持鞍 → 返回 Pass，委托给 SaddleItem::itemInteractionForEntity() 处理
   - 其他 → 返回 Pass
   - 创造模式下喂食不消耗物品，静默实体不播放音效

### SnifferEntity 嗅探兽
1. **幼年期长度特殊**：嗅探兽幼年期为 48000 tick（40 分钟），是普通动物（24000 tick / 20 分钟）的两倍。`setChild(bool)` 覆盖了 `AgeableEntity::setChild`，设置 `SNIFFER_BABY_AGE_TICKS = -48000`。`AgeableEntity::setChild` 因此被改为 `virtual` 以允许此覆盖。
2. **状态机同步**：`DATA_STATE_PARAM` 使用 `DataParameter<i8>`（对齐 MC SNIFFER_STATE BYTE 序列化器），`getState()`/`setState()` 负责 `State↔i8` 转换。`transitionTo(state)` 在切换状态时播放对应音效（happy/scenting/sniffing/digging_stop）。
3. **繁殖物品**：`isBreedingItem` 识别 `TORCHFLOWER_SEEDS` 和 `PITCHER_POD`（MC 原版使用 `ItemTags.SNIFFER_FOOD` 标签，项目无物品标签系统故直接判断）。
4. **繁殖状态限制**：`canMateWith` 要求双方 `SnifferEntity` 且状态在 `{Idling, Scenting, FeelingHappy}` 集合内才可繁殖。
5. **繁殖掉蛋而非幼体**：MC 原版 `Sniffer.spawnChildFromBreeding` 实际掉落 `SNIFFER_EGG` 物品而非直接生成幼体。当前项目 `Items::SNIFFER_EGG` 尚未实现，`spawnBaby` 返回幼体嗅探兽作为占位实现（TODO 待物品实现后改为掉落蛋物品）。
6. **挖掘 AI 未实现**：MC 原版使用 Brain + MemoryModuleType.SNIFFER_EXPLORED_POSITIONS 管理挖掘状态，项目无 Brain 系统且挖掘 Goal 未实现。`transitionTo(Digging)` 中的 `DATA_DROP_SEED_AT_TICK` 设置与粒子广播留有 TODO 注释。
7. **孵化入口**：`SnifferEggBlock::randomTick`（位于 `world/block/blocks/functional/TrailsBlocks.cpp`）在孵化等级达到 2 时调用 `setChild(true)` + `setPosition(pos.center())` + `setRotation(wrapDegrees(random*360), 0)` + `finalizeSpawn` + `spawnEntity` 生成幼体嗅探兽。
8. **属性常量**：`MOVEMENT_SPEED = 0.1`、`MAX_HEALTH = 14.0`、`FOLLOW_RANGE = 16.0`、`SNIFFER_STEP_VOLUME = 0.15`，均对齐 MC 1.21.11 `Sniffer.createAttributes` 与 `playStepSound`。
9. **NBT 键名**：`state`（i8，0-6 状态枚举）与 `drop_seed_at_tick`（i32，挖掘掉落种子 tick），定义于 `entity/serialization/EntityNbtKeys.hpp`，与 MC 1.21.11 `Sniffer.addAdditionalSaveData` 一致。

### TurtleEntity 海龟
1. **出生地继承**：幼龟孵化后需要继承父母的出生地位置，否则无法返回产卵。
2. **有蛋时不可繁殖**：`canBreed()` 需要额外检查 `!hasEgg()`。
3. **产卵检测**：`_layEgg()` 需要检测脚下是否为沙子类方块（BlockTags::SAND），当前位置是否为空气。
4. **水陆速度差异**：`travel()` 中水中速度正常，陆地速度减半（最低 0.06），幼体水中速度再降低。
