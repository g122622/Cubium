# Entity System

实体系统是 Cubium 项目中所有游戏实体（玩家、生物、物品等）的核心模块。

## 目录结构

```
src/common/entity/
├── core/                           # 核心实体框架
│   ├── Entity.hpp/cpp              # 实体基类（含骑乘系统、压力板触发等虚方法）
│   ├── LivingEntity.hpp/cpp        # 生物实体基类
│   ├── MobEntity.hpp/cpp           # AI生物基类（覆写 getLootTableId() 支持自定义 DeathLootTable）
│   ├── CreatureEntity.hpp/cpp      # 陆地生物基类
│   ├── AgeableEntity.hpp/cpp       # 可成长实体基类
│   ├── FlyingEntity.hpp/cpp        # 飞行生物基类
│   ├── EntityType.hpp/cpp          # 实体类型定义
│   ├── EntityRegistry.hpp          # 实体类型注册表
│   ├── EntityClassification.hpp/cpp # 实体分类
│   ├── EntityDataManager.hpp       # 实体数据同步管理
│   ├── EntityPose.hpp              # 实体姿态枚举
│   ├── EntitySize.hpp              # 实体尺寸
│   ├── EntitySpawnPlacementRegistry.hpp/cpp # 实体生成放置规则
│   ├── EntityUtils.hpp             # 模板型实体工具函数（搜索、距离）
│   ├── DataParameter.hpp           # 数据参数定义
│   ├── MoverType.hpp               # 移动类型枚举
│   ├── BoostHelper.hpp             # 加速辅助类（鞍和加速状态管理）
│   ├── VanillaEntities.hpp         # 原版实体注册
│
├── tag/                            # 实体类型标签
│   ├── EntityTypeTag.hpp/cpp       # 实体类型标签类
│   ├── EntityTypeTags.hpp/cpp      # 内置标签集合（IMPACT_PROJECTILES 等）
│   ├── EntityTypeTagLoader.hpp/cpp # 数据包 JSON 加载器
│   └── README.md                   # 标签模块说明
│
├── utils/                          # 非模板实体工具
│   ├── ItemDropHelper.hpp/cpp      # 物品掉落工具类（统一随机速度、生成物品实体）
│   └── README.md                   # 工具模块说明
│
├── player/                         # 玩家相关模块
│   ├── CooldownTracker.hpp/cpp     # 物品冷却追踪器
│   ├── SleepResult.hpp             # 睡眠结果枚举
│   ├── SleepManager.hpp/cpp        # 睡眠管理器
│   ├── SpawnPointValidator.hpp/cpp # 重生点验证器
│   └── README.md                   # 模块说明
│
├── entities/                       # 具体实体实现
│   ├── passive/                    # 被动/中立生物
│   │   ├── basic/                  # 普通动物（AnimalEntity, PigEntity, CowEntity 等）
│   │   ├── tamable/                # 可驯服动物（TameableEntity, WolfEntity, CatEntity 等）
│   │   ├── special/                # 特殊动物（FoxEntity, PandaEntity, BeeEntity 等）
│   │   ├── horse/                  # 马类（AbstractHorseEntity, HorseEntity 等）
│   │   ├── fish/                   # 鱼类（AbstractFishEntity, CodEntity 等）
│   │   ├── water/                  # 水生生物（WaterMobEntity, SquidEntity, DolphinEntity, AxolotlEntity）
│   │   ├── ambient/                # 环境生物（AmbientEntity, BatEntity）
│   │   └── golem/                  # 傀儡（GolemEntity, IronGolemEntity, SnowGolemEntity）
│   │
│   ├── monster/                    # 敌对生物
│   │   ├── MonsterEntity.hpp/cpp   # 敌对生物基类
│   │   ├── undead/                 # 亡灵类（ZombieEntity, SkeletonEntity 等）
│   │   ├── arthropod/              # 节肢类（SpiderEntity, SilverfishEntity 等）
│   │   ├── nether/                 # 地狱生物（BlazeEntity, GhastEntity 等）
│   │   ├── end/                    # 末地生物（EndermanEntity, ShulkerEntity）
│   │   ├── basic/                  # 基础怪物（CreeperEntity, SlimeEntity 等）
│   │   ├── ocean/                  # 海洋怪物（GuardianEntity, ElderGuardianEntity）
│   │   └── illager/                # 灾厄村民（AbstractIllagerEntity, EvokerEntity 等）
│   │
│   ├── boss/                       # Boss生物（EnderDragonEntity, WitherEntity）
│   ├── villager/                   # 村民/商人（AbstractVillagerEntity, VillagerEntity）
│   ├── projectile/                 # 投掷物（ProjectileEntity, ThrowableEntity, AbstractArrowEntity 等）
│   ├── vehicle/                    # 交通工具（BoatEntity, MinecartEntity）
│   ├── item/                       # 物品相关实体（ItemEntity）
│   ├── hanging/                    # 悬挂实体（HangingEntity）
│   ├── effect/                     # 效果实体（闪电、末影水晶等）
│   ├── misc/                       # 杂项实体（下落方块、TNT、不祥物品生成器等）
│   └── player/                     # 玩家实体（Player, GameModeUtils）
│
├── interfaces/                     # 实体接口
│   ├── IAngerable.hpp              # 愤怒接口
│   ├── IRideable.hpp/cpp           # 可骑乘接口（猪、炽足兽、马等）
│   ├── IShearable.hpp              # 可剪毛接口（羊、雪傀儡、哞菇）
│   ├── IRangedAttackMob.hpp        # 远程攻击接口
│   ├── ICrossbowUser.hpp           # 弩使用者接口
│   ├── IFlyingAnimal.hpp           # 飞行动物接口
│   ├── IJumpingMount.hpp           # 可跳跃骑乘接口
│   └── IEquipable.hpp              # 可装备接口
│
├── ai/                             # AI 系统
│   ├── controller/                 # 控制器（LookController, MovementController, JumpController）
│   ├── goal/                       # AI 目标系统
│   │   ├── Goal.hpp                # 目标基类
│   │   ├── GoalFlag.hpp            # 目标互斥标志
│   │   ├── GoalConstants.hpp       # 目标常量
│   │   ├── GoalSelector.hpp        # 目标选择器
│   │   ├── PrioritizedGoal.hpp     # 优先级目标包装
│   │   └── goals/                  # 具体 AI 目标（SwimGoal, RandomWalkingGoal 等）
│   │
│   ├── brain/                      # 大脑系统
│   │   ├── Brain.hpp               # Brain主类（模板）
│   │   ├── memory/                 # 记忆模块（Memory, MemoryModuleType 等）
│   │   ├── sensor/                 # 传感器（Sensor, SensorType）
│   │   ├── task/                   # 任务（Task, tasks/ 任务实现）
│   │   └── schedule/               # 日程（Activity, Schedule）
│   │
│   └── pathfinding/                # 寻路系统
│       ├── Path.hpp                # 路径表示
│       ├── PathPoint.hpp/cpp       # 路径点
│       ├── PathHeap.hpp            # 路径堆（A* 算法）
│       ├── PathFinder.hpp/cpp      # A* 寻路器
│       ├── PathNavigator.hpp/cpp   # 路径导航器
│       ├── PathNodeType.hpp        # 路径节点类型
│       ├── NodeProcessor.hpp       # 节点处理器接口
│       ├── WalkNodeProcessor.hpp/cpp # 行走节点处理器
│       └── Region.hpp              # 世界区域接口
│
├── attribute/                     # 属性系统
│   ├── Attribute.hpp              # 属性定义
│   ├── AttributeInstance.hpp      # 属性实例
│   ├── AttributeMap.hpp           # 属性映射表
│   ├── AttributeModifier.hpp      # 属性修饰符
│   └── Attributes.hpp             # 标准属性常量
│
├── combat/                        # 战斗系统
│   ├── AttackContext.hpp/cpp      # 攻击上下文
│   └── PlayerAttackHelper.hpp/cpp # 玩家攻击辅助
│
├── enchantment/                   # 附魔效果追踪
│   ├── LocationEnchantmentTracker.hpp/cpp # 位置依赖附魔跟踪器
│   └── README.md                  # 模块说明
│
├── damage/                        # 伤害系统
│   ├── DamageSource.hpp           # 伤害来源（含 DamageType 枚举和 DamageSource::is(DamageTypeTag)）
│   ├── CombatEntry.hpp/cpp        # 战斗记录条目
│   ├── CombatTracker.hpp/cpp      # 战斗追踪器
│   └── tag/                       # 伤害类型标签系统（DamageTypeTags）
│       ├── DamageTypeTag.hpp/cpp       # 标签类 + DamageTypeNames 映射
│       ├── DamageTypeTags.hpp/cpp      # 34 个内置标签注册表
│       └── DamageTypeTagLoader.hpp/cpp # 数据包 JSON 加载器
│
├── inventory/                     # 背包系统
│   ├── IInventory.hpp             # 背包接口
│   ├── Container.hpp/cpp          # 容器
│   ├── ContainerTypes.hpp         # 容器类型
│   ├── PlayerInventory.hpp/cpp    # 玩家背包
│   ├── CraftingInventory.hpp/cpp  # 合成背包
│   ├── AbstractContainerMenu.hpp/cpp  # 容器菜单
│   └── Slot.hpp/cpp               # 槽位
│
├── movement/                      # 移动系统
│   ├── AutoJump.hpp/cpp           # 自动跳跃
│   └── AutoJumpConstants.hpp      # 自动跳跃常量
│
└── loot/                          # 掉落表系统
    ├── LootTable.hpp/cpp          # 掉落表
    ├── LootPool.hpp/cpp           # 掉落池
    ├── LootEntry.hpp/cpp          # 掉落条目
    ├── LootContext.hpp/cpp        # 掉落上下文
    ├── LootConditions.hpp/cpp     # 掉落条件
    ├── LootFunctions.hpp/cpp      # 掉落函数
    ├── LootSerializers.hpp/cpp    # JSON 序列化器
    └── README.md                  # 模块说明
```

## 内部模块关系

```
                    ┌─────────────────┐
                    │     Entity      │
                    │   (基类)        │
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
      ┌───────────┐  ┌───────────┐  ┌───────────┐
      │LivingEntity│  │ ItemEntity│  │EntityType │
      └─────┬─────┘  └───────────┘  └───────────┘
            │
            ├───────────────────────┐
            │                       │
            ▼                       ▼
    ┌───────────┐          ┌───────────────┐
    │ MobEntity │          │  attribute/   │
    └─────┬─────┘          │  damage/      │
          │                │  inventory/   │
          ├────────────────┤
          │                │
          ▼                ▼
    ┌────────────┐  ┌────────────┐
    │ai/goal/    │  │ai/pathfind │
    │ai/controll │  │            │
    └────────────┘  └────────────┘
```

**核心依赖链**：
- `Entity` → `LivingEntity` → `MobEntity` → `CreatureEntity` / `AgeableEntity` → 具体实体
- `MobEntity` 依赖 `ai/goal/`（目标选择）、`ai/controller/`（行为控制）、`ai/pathfinding/`（寻路）
- `LivingEntity` 依赖 `attribute/`（属性系统）、`damage/`（伤害系统）、`inventory/`（装备）

## 上下游外部依赖关系

### 上游依赖（谁依赖了这个目录）

- `src/server/` - 服务端逻辑（ServerPlayer、NaturalSpawner、EntityTracker、DespawnManager 等）
- `src/client/` - 客户端逻辑（ClientEntity、渲染器、模型）
- `src/common/world/` - 世界系统（EntityManager、BlockEntity、Explosion、ChunkData 等）
- `src/common/item/` - 物品系统（Items.cpp、BucketItem、ShearsItem 等）
- `src/common/world/block/` - 方块系统（各种方块与实体交互）

### 下游依赖（这个目录依赖了谁）

- `src/common/core/` - 核心类型、错误处理
- `src/common/util/math/` - 数学工具（Vector3, Random）
- `src/common/util/AxisAlignedBB.hpp` - 碰撞箱
- `src/common/item/ItemStack.hpp` - 物品堆
- `src/common/network/packet/` - 网络包
- `src/common/world/IWorld.hpp` - 世界接口
- `src/common/physics/PhysicsEngine.hpp` - 物理引擎

## 容易踩的坑

### 1. 实体生命周期管理

```cpp
// 错误：裸指针悬空
Entity* entity = world.getEntity(id);
// ... 一些操作后，实体可能已被删除
entity->tick();  // 崩溃！

// 正确：使用 EntityId 检查
EntityId id = entity->id();
if (auto* entity = world.getEntity(id)) {
    if (!entity->isRemoved()) {
        entity->tick();
    }
}
```

### 2. AI 目标互斥冲突

多个目标共享同一 GoalFlag 时，GoalSelector 会自动处理冲突。优先级设置要合理，高优先级目标会打断低优先级目标。

### 3. 寻路性能问题

```cpp
// 错误：频繁重新计算路径
void tick() {
    navigator.moveTo(targetX, targetY, targetZ);  // 每帧都重新计算！
}

// 正确：检查路径状态
void tick() {
    if (!navigator.hasPath() || navigator.isDone()) {
        navigator.moveTo(targetX, targetY, targetZ);
    }
}
```

### 4. 属性修饰符未移除

效果结束时必须移除修饰符：
```cpp
void onEffectEnd() {
    instance.removeModifier(boost.getId());
}
```

### 5. 掉落表内存管理

不要持有掉落表原始指针，每次使用时从 LootManager 获取。

### 6. 线程安全

实体操作必须在主线程进行，异步任务应通过队列转发到主线程。

### 7. EntityFlags 枚举类型

`Entity::EntityFlags` 是实体运行时状态（燃烧、潜行等），`entity::EntityFlags` 是实体类型特性（免疫火焰等），两者不同。

### 8. 碰撞箱与姿态刷新

`Entity::refreshDimensions()` 是尺寸变化后的统一刷新入口，运行时改变体型的实体应在尺寸改变后立即调用。玩家从蹲下、游泳、睡眠切回站立时，需要先做碰撞可容纳性检查。

### 9. 声音链路

`Entity::playSound(...)` 会把声音事件交给当前 `IWorld`，实体层不直接做广播。`LivingEntity` 统一处理受伤和死亡声音，`MobEntity` 统一处理环境声。

### 10. TemptGoal 物品过滤

`TemptGoal` 需要正确过滤真正的 `Player` 实体，通过主手/副手物品堆过滤。

### 11. PanicGoal 和 WaterAvoidingRandomWalkingGoal

这些目标直接查询 `IWorld::isWaterAt(...)` / `isLavaAt(...)`，保持测试与世界查询接口一致。

### 12. ChickenEntity 鸡蛋生成

鸡的 `tick()` 生成鸡蛋物品实体后必须立即重置计时器，否则会批量发射鸡蛋。

### 13. Entity::entityType() 类型比较

旧代码使用 `LegacyEntityType` 枚举进行类型比较，现已废弃。使用 `entityType()` 方法和 `VanillaEntityTypeKeys` 命名空间（`const EntityType*` 指针别名，指针比较）：
```cpp
// 新代码
if (entity->entityType() == entity::VanillaEntityTypeKeys::PIG) {
    // ...
}
```

**注意**：`VanillaEntityTypeKeys` 指针在 `VanillaEntities::registerAll()` 后由 `initialize()` 填充。`entityType()` 懒查询注册表并缓存，返回的指针与 `VanillaEntityTypeKeys::*` 同源（均来自 `EntityRegistry::m_types`），可安全指针比较。

### 14. EntityMetadataPacket 同步

`EntityTracker` 负责 spawn 内联 metadata 和 dirty metadata packet，`ClientEntity::setMetadata()` 负责把原始数据写进本地数据管理器。新增字段时三处必须一起改。

### 15. CactusBlock 碰撞伤害

仙人掌碰撞伤害应仅针对活体实体，使用 `LivingEntity::hurt()` 和 `DamageSources::cactus()`。

### 16. DyeableArmorItem 颜色清除

清除颜色时也必须清除空的 `display` 标签，否则盔甲堆将停止按预期合并。

### 17. 玩家站起过渡

玩家从蹲伏/游泳/睡眠切换时，必须检查目标姿势盒子是否适合，不要用原始站立姿势更改绕过 `setSneaking()` / `setSwimming()` / `setSleeping()`。

### 18. Player::updateMoveDistance 状态管理

不要把 `prevPosition` 当成脚步声或视野晃动的采样基准，它是插值历史状态。`Player::updateMoveDistance()` 使用专用的采样位置。

### 19. Player::updatePhysics 轻量级测试

`Player::updatePhysics()` 可以在没有物理引擎的轻量级测试世界中运行，代码会回退到直接移动。

### 20. m_player->isInWater() 客户端权威性

不要假设 `m_player->isInWater()` 在客户端是权威的，这个值只在 `Entity::baseTick()` / `updateEnvironmentState()` 或本地物理刷新路径里更新。

### 21. ClientWorld::entityManager() 返回类型

`ClientWorld::entityManager()` 返回 `ClientEntityManager`，客户端本地 `Player` 不会在这条链路里跑 `Player::tick()`。

### 22. SlimeEntity 分裂时机和经验值

- 分裂逻辑应在 `remove()` 中执行，而非 `die()`，因为 `die()` 时实体还未被标记为移除
- `Entity::remove()` 是虚函数，允许子类重写
- 经验值更新应在 `updateSizeAttributes()` 中执行，确保初始化时也能正确设置

### 23. 世界边界伤害检测

`Player::tick()` 中添加了世界边界伤害检测逻辑，只有非观察者模式、非无敌状态的玩家受到边界伤害。`IWorld::worldBorder()` 提供边界访问接口，`damageBuffer` 默认值 5.0，玩家在边界外 5 格内不受伤。

### 24. Entity::getRandom() 持久化随机数生成器

`Entity::getRandom()` 返回 `math::Random&`（持久化引用），不再按值返回临时对象。旧 `MobEntity::getRandom()` 已移除。所有实体共享 `Entity` 基类的实现。使用引用赋值 `math::Random& rng = entity->getRandom()` 而非值赋值。详见 `core/README.md` 中的"实体随机数生成器"条目。

### 24. MonsterEntity 光照等级检查 (isValidLightLevel)

`MonsterEntity::isValidLightLevel()` 实现了 MC 1.16.5 的两阶段光照检查：
- 第一阶段：天空光照 > random(0-31) 则太亮
- 第二阶段：综合光照 <= random(0-7) 则足够暗
- 雷暴天气使用固定的天空减暗值 10，允许怪物在白天生成

### 25. 步进高度（stepHeight）

实体可以自动走上多高的方块（无需跳跃）：

| 实体类型 | stepHeight |
|---------|------------|
| LivingEntity（默认） | 0.6f |
| IronGolemEntity | 1.0f |
| AbstractHorseEntity | 1.0f |
| EndermanEntity | 1.0f |
| DrownedEntity | 1.0f |
| RavagerEntity | 1.0f |
| TurtleEntity | 1.0f |

### 26. VanillaEntityTypeKeys 初始化时机

`VanillaEntityTypeKeys` 指针别名在 `VanillaEntities::registerAll()` 后由 `VanillaEntityTypeKeys::initialize()` 填充，确保在实体类型注册后使用。

### 27. 铁傀儡攻击/持花状态同步

铁傀儡的攻击动画（举臂）和持花状态涉及服务端、网络包、客户端三端同步，修改时三处必须一起改：
- **服务端**：`IronGolemEntity::attackEntityAsMob()` 通过 `IWorld::broadcastEntityStatus()` 广播 `EntityStatusPacket::Status::IronGolemAttack(4)` / `IronGolemHoldRose(11)` / `IronGolemStopRose(34)`；`setHoldingRose()` 同理
- **网络包**：`EntityPackets.hpp` 中 `EntityStatusPacket::Status` 枚举必须包含这三个值
- **客户端**：`ClientApplicationNetwork.cpp` 的 `onEntityStatus` 回调必须处理这三个状态值，分别设置 `ClientEntity` 的 `ironGolemAttackTimer`、`ironGolemArmsRaised`、`ironGolemHoldingRose`

新增或修改铁傀儡的任何动画/状态时，三端缺一不可。

### 28. TNT矿车引燃状态同步

TNT矿车引燃涉及服务端、网络包、客户端三端同步，与铁傀儡状态同步模式一致：
- **服务端**：`TNTMinecartEntity::_ignite()` 通过 `IWorld::broadcastEntityStatus()` 广播 `EntityStatusPacket::Status::EatBlock(10)`，并调用 `Entity::playSound(ENTITY_TNT_PRIMED)` 播放音效
- **网络包**：`EntityPackets.hpp` 中 `EntityStatusPacket::Status::EatBlock` 被羊吃草和TNT矿车引燃共用（status code 10）
- **客户端**：`ClientApplicationNetwork.cpp` 的 `onEntityStatus` 回调根据 `entityType() == VanillaEntityTypeKeys::TNT_MINECART` 区分：TNT矿车调用 `setFuseTimer(80)`，羊调用 `setEatAnimationTimer(40)`

修改引燃逻辑或新增共用 status code 的实体状态时，三端必须同步更新。

### 29. Entity::getComparatorOutput() 比较器信号

`Entity` 基类提供 `virtual i32 getComparatorOutput() const` 方法，返回 0-15 的红石比较器信号强度。默认返回 0，需要输出信号的实体重写此方法。

**已实现的实体信号：**

| 实体 | 信号来源 | 说明 |
|------|---------|------|
| ItemFrameEntity | 物品堆叠数/地图编号 | 空物品框返回0，地图返回地图编号(1-15)，普通物品返回 min(stackSize, 1) |
| ChestMinecartEntity | 容器填充率 | 使用 `RedstoneHelper::calcRedstoneFromInventory()` 计算 |
| HopperMinecartEntity | 容器填充率 | 使用 `RedstoneHelper::calcRedstoneFromInventory()` 计算 |
| CommandBlockMinecartEntity | 成功计数 | 直接返回 `m_successCount` |

**信号计算公式**（`RedstoneHelper::calcRedstoneFromInventory()`）：
```
signal = floor(fillRatio * 14) + (hasItems ? 1 : 0)
```
其中 `fillRatio = occupiedSlots / totalSlots`（按槽位计算，非按物品数量）。

**集成路径**：
- `DetectorRailBlock::getComparatorInputOverride()` 通过 `RedstoneHelper::getEntitySignal()` 查询矿车实体信号
- `RedstoneComparatorBlock` 对物品框直接调用 `ItemFrameEntity::getComparatorOutput()`

### 30. 僵尸增援系统

`ZombieEntity` 实现了 MC 1.21.11 的僵尸增援（Reinforcement）机制，当僵尸在困难模式下受到伤害时有概率召唤新的增援僵尸：

**触发条件**（`ZombieEntity::hurt()`）：
- 难度必须为 Hard（由 `DifficultyHelper::canZombieReinforce()` 判断）
- 随机数 < `zombie.spawn_reinforcements` 属性值
- 存在攻击目标（`attackTarget()` 或 `source.getEntity()`）

**生成逻辑**（`ZombieEntity::_trySpawnReinforcement()`）：
1. 检查 `doMobSpawning` 游戏规则
2. 查找同类型实体（`EntityType` 注册表查找）
3. 50 次尝试内随机选择生成位置：各轴偏移 = `nextInt(7, 40) * nextInt(-1, 1)`（MC 1.21.11 原版公式，`nextInt(-1, 1)` 产生 {-1, 0, 1}，偏移可为 0）
4. 位置验证：世界边界、附近无存活玩家（7格）、无实体碰撞、无方块碰撞、非液体（`containsAnyLiquid`）
5. 生成同类型僵尸并调用 `finalizeSpawn()`
6. 设置攻击目标，通过 `spawnEntity()` 加入世界
7. 召唤者获得 `reinforcement_caller_charge` 修饰符（-0.05 Addition，累加）
8. 被召唤者获得 `reinforcement_callee_charge` 修饰符（-0.05 Addition）

**注意**：当前实现中位置验证包含额外的地面支撑和固体方块检查（MC 原版使用 `SpawnPlacements.isSpawnPositionOk` + `checkSpawnRules`），代码中已标注 TODO，待 SpawnPlacements 系统完善后对齐。

**公共入口**（`ZombieEntity::trySummonReinforcements()`）：
- 完整的公共接口，内部进行 null 检查、难度检查、概率检查后委托给 `_trySpawnReinforcement()`
- 适用于外部调用（如命令触发增援）

**关键属性**：
- `zombie.spawn_reinforcements`：增援概率，基础值 0.0，`finalizeSpawn()` 中根据区域难度随机设置（0.0~0.1）
- 修饰符累加：每次增援后 caller/callee 各 -0.05，逐渐降低后续增援概率
