# Entity System

实体系统是 Minecraft Reborn 项目中所有游戏实体（玩家、生物、物品等）的核心模块。

## 目录结构

```
src/common/entity/
├── core/                           # 核心实体框架
│   ├── Entity.hpp/cpp              # 实体基类
│   ├── LivingEntity.hpp/cpp        # 生物实体基类
│   ├── MobEntity.hpp/cpp           # AI生物基类
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
│   └── VanillaEntities.hpp         # 原版实体注册
│
├── utils/                          # 非模板实体工具
│   ├── ItemDropHelper.hpp/cpp      # 物品掉落工具类（统一随机速度、生成物品实体）
│   ├── EntityUtils.hpp/cpp         # LegacyEntityType -> typeId 映射
│   └── README.md                   # 工具模块说明
│
├── player/                     # 玩家相关模块
│   ├── CooldownTracker.hpp/cpp # 物品冷却追踪器
│   ├── SleepResult.hpp         # 睡眠结果枚举
│   ├── SleepManager.hpp/cpp    # 睡眠管理器
│   ├── SpawnPointValidator.hpp/cpp # 重生点验证器
│   └── README.md               # 模块说明
│
├── entities/                       # 具体实体实现
│   ├── passive/                    # 被动/中立生物
│   │   ├── basic/                  # 普通动物
│   │   │   ├── AnimalEntity.hpp/cpp # 动物基类
│   │   │   ├── PigEntity.hpp/cpp   # 猪
│   │   │   ├── CowEntity.hpp/cpp   # 牛
│   │   │   ├── SheepEntity.hpp/cpp # 羊
│   │   │   ├── ChickenEntity.hpp/cpp # 鸡
│   │   │   ├── RabbitEntity.hpp/cpp # 兔子
│   │   │   └── MooshroomEntity.hpp/cpp # 哞菇
│   │   ├── tamable/                # 可驯服动物
│   │   │   ├── TameableEntity.hpp/cpp # 可驯服基类
│   │   │   ├── WolfEntity.hpp/cpp  # 狼
│   │   │   ├── CatEntity.hpp/cpp   # 猫
│   │   │   ├── OcelotEntity.hpp/cpp # 豹猫
│   │   │   └── ParrotEntity.hpp/cpp # 鹦鹉
│   │   ├── special/                # 特殊动物
│   │   │   ├── FoxEntity.hpp/cpp   # 狐狸
│   │   │   ├── PandaEntity.hpp/cpp # 熊猫
│   │   │   ├── PolarBearEntity.hpp/cpp # 北极熊
│   │   │   ├── TurtleEntity.hpp/cpp # 海龟
│   │   │   ├── BeeEntity.hpp/cpp   # 蜜蜂
│   │   │   └── StriderEntity.hpp/cpp # 炽足兽
│   │   ├── horse/                  # 马类
│   │   │   ├── AbstractHorseEntity.hpp/cpp # 马类基类
│   │   │   ├── HorseEntity.hpp/cpp # 马
│   │   │   ├── DonkeyEntity.hpp/cpp # 驴
│   │   │   ├── MuleEntity.hpp/cpp  # 骡
│   │   │   ├── SkeletonHorseEntity.hpp/cpp # 骷髅马
│   │   │   ├── ZombieHorseEntity.hpp/cpp # 僵尸马
│   │   │   └── LlamaEntity.hpp/cpp # 羊驼
│   │   ├── fish/                   # 鱼类
│   │   │   ├── AbstractFishEntity.hpp/cpp # 鱼类基类
│   │   │   ├── CodEntity.hpp/cpp   # 鳕鱼
│   │   │   ├── SalmonEntity.hpp/cpp # 鲑鱼
│   │   │   ├── PufferfishEntity.hpp/cpp # 河豚
│   │   │   └── TropicalFishEntity.hpp/cpp # 热带鱼
│   │   ├── water/                  # 水生生物
│   │   │   ├── WaterMobEntity.hpp/cpp # 水生生物基类
│   │   │   ├── SquidEntity.hpp/cpp # 鱿鱼
│   │   │   └── DolphinEntity.hpp/cpp # 海豚
│   │   ├── ambient/                # 环境生物
│   │   │   ├── AmbientEntity.hpp/cpp # 环境生物基类
│   │   │   └── BatEntity.hpp/cpp   # 蝙蝠
│   │   └── golem/                  # 傀儡
│   │       ├── GolemEntity.hpp/cpp # 傀儡基类
│   │       ├── IronGolemEntity.hpp/cpp # 铁傀儡
│   │       └── SnowGolemEntity.hpp/cpp # 雪傀儡
│   │
│   ├── monster/                    # 敌对生物
│   │   ├── MonsterEntity.hpp/cpp   # 敌对生物基类
│   │   ├── undead/                 # 亡灵类
│   │   │   ├── ZombieEntity.hpp/cpp # 僵尸
│   │   │   ├── HuskEntity.hpp/cpp  # 尸壳
│   │   │   ├── DrownedEntity.hpp/cpp # 溺尸
│   │   │   ├── ZombieVillagerEntity.hpp/cpp # 僵尸村民
│   │   │   ├── SkeletonEntity.hpp/cpp # 骷髅
│   │   │   ├── StrayEntity.hpp/cpp # 流浪者
│   │   │   ├── WitherSkeletonEntity.hpp/cpp # 凋灵骷髅
│   │   │   └── PhantomEntity.hpp/cpp # 幻翼
│   │   ├── arthropod/              # 节肢类
│   │   │   ├── SpiderEntity.hpp/cpp # 蜘蛛
│   │   │   ├── CaveSpiderEntity.hpp/cpp # 洞穴蜘蛛
│   │   │   ├── SilverfishEntity.hpp/cpp # 蠹虫
│   │   │   └── EndermiteEntity.hpp/cpp # 末影螨
│   │   ├── nether/                 # 地狱生物
│   │   │   ├── BlazeEntity.hpp/cpp # 烈焰人
│   │   │   ├── GhastEntity.hpp/cpp # 恶魂
│   │   │   ├── MagmaCubeEntity.hpp/cpp # 岩浆怪
│   │   │   └── NetherEntities.hpp  # 猪灵等(集合文件)
│   │   ├── end/                    # 末地生物
│   │   │   ├── EndermanEntity.hpp/cpp # 末影人
│   │   │   └── ShulkerEntity.hpp/cpp # 潜影贝
│   │   ├── basic/                  # 基础怪物
│   │   │   ├── CreeperEntity.hpp/cpp # 苦力怕
│   │   │   ├── SlimeEntity.hpp/cpp # 史莱姆
│   │   │   ├── GiantEntity.hpp/cpp # 巨人
│   │   │   └── PhantomEntity.hpp/cpp # 幻翼
│   │   ├── ocean/                  # 海洋怪物
│   │   │   ├── GuardianEntity.hpp/cpp # 守卫者
│   │   │   └── ElderGuardianEntity.hpp/cpp # 远古守卫者
│   │   └── illager/                # 灾厄村民
│   │       ├── AbstractIllagerEntity.hpp/cpp # 灾厄村民基类
│   │       ├── AbstractRaiderEntity.hpp/cpp # 袭击者基类
│   │       ├── EvokerEntity.hpp/cpp # 唤魔者
│   │       ├── IllusionerEntity.hpp/cpp # 幻术师
│   │       ├── RavagerEntity.hpp/cpp # 劫掠兽
│   │       ├── VexEntity.hpp/cpp   # 恼鬼
│   │       ├── WitchEntity.hpp/cpp # 女巫
│   │       └── IllagerEntities.hpp # 掠夺者、卫道士
│   │
│   ├── boss/                       # Boss生物
│   │   ├── EnderDragonEntity.hpp/cpp # 末影龙
│   │   └── WitherEntity.hpp/cpp    # 凋灵
│   │
│   ├── villager/                   # 村民/商人
│   │   ├── AbstractVillagerEntity.hpp/cpp # 村民基类
│   │   └── VillagerEntity.hpp/cpp  # 村民、流浪商人
│   │
│   ├── projectile/                 # 投掷物
│   │   ├── ProjectileEntity.hpp/cpp # 投掷物基类
│   │   ├── ThrowableEntity.hpp/cpp # 可投掷物基类
│   │   ├── AbstractArrowEntity.hpp/cpp # 箭矢基类
│   │   ├── ProjectileItemEntity.hpp/cpp # 物品投掷物基类
│   │   ├── AbstractFireballEntity.hpp/cpp # 火球基类
│   │   ├── OtherProjectiles.hpp/cpp # 其他投掷物
│   │   └── README.md
│   │
│   ├── vehicle/                    # 交通工具
│   │   ├── BoatEntity.hpp/cpp      # 船
│   │   ├── MinecartEntity.hpp/cpp  # 矿车及变种
│   │   └── README.md
│   │
│   ├── item/                       # 物品相关实体
│   │   └── ItemEntity.hpp/cpp      # 掉落物品
│   │
│   ├── hanging/                    # 悬挂实体
│   │   ├── HangingEntity.hpp/cpp   # 悬挂实体基类
│   │   └── README.md
│   │
│   ├── effect/                     # 效果实体
│   │   ├── EffectEntities.hpp/cpp  # 闪电、末影水晶等
│   │   └── README.md
│   │
│   ├── misc/                       # 杂项实体
│   │   ├── MiscEntities.hpp/cpp    # 下落方块、TNT等
│   │   └── README.md
│   │
│   └── player/                     # 玩家实体
│       ├── Player.hpp/cpp          # 玩家实体
│       ├── GameModeUtils.hpp/cpp   # 游戏模式工具
│       └── README.md               # 玩家模块说明
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
│   ├── controller/                 # 控制器
│   │   ├── LookController.hpp/cpp  # 视线控制器
│   │   ├── MovementController.hpp/cpp # 移动控制器
│   │   └── JumpController.hpp/cpp  # 跳跃控制器
│   │
│   ├── goal/                       # AI 目标系统
│   │   ├── Goal.hpp                # 目标基类
│   │   ├── GoalFlag.hpp            # 目标互斥标志
│   │   ├── GoalConstants.hpp       # 目标常量
│   │   ├── GoalSelector.hpp        # 目标选择器
│   │   ├── PrioritizedGoal.hpp     # 优先级目标包装
│   │   └── goals/                  # 具体 AI 目标
│   │       ├── SwimGoal.cpp        # 游泳
│   │       ├── RandomWalkingGoal.cpp # 随机漫步
│   │       ├── LookAtGoal.cpp      # 看向目标
│   │       ├── PanicGoal.cpp       # 恐慌逃跑
│   │       ├── BreedGoal.cpp       # 繁殖
│   │       ├── FollowParentGoal.cpp # 跟随父母
│   │       ├── TemptGoal.cpp       # 诱惑
│   │       ├── AvoidEntityGoal.cpp # 避开实体
│   │       ├── MeleeAttackGoal.cpp # 近战攻击
│   │       ├── movement/           # 移动类Goal
│   │       ├── attack/             # 攻击类Goal
│   │       ├── target/             # 目标选择Goal
│   │       ├── interact/           # 交互类Goal
│   │       └── special/            # 特殊Goal
│   │
│   ├── brain/                      # 大脑系统
│   │   ├── Brain.hpp               # Brain主类（模板）
│   │   ├── memory/                 # 记忆模块
│   │   │   ├── Memory.hpp          # 记忆容器
│   │   │   ├── MemoryModuleType.hpp/cpp # 85+种记忆类型
│   │   │   └── MemoryModuleStatus.hpp # 记忆状态
│   │   ├── sensor/                 # 传感器
│   │   │   ├── Sensor.hpp          # 传感器基类
│   │   │   ├── SensorType.hpp      # 传感器类型工厂
│   │   │   └── Sensors.hpp         # 具体传感器(框架)
│   │   ├── task/                   # 任务
│   │   │   ├── Task.hpp            # 任务基类
│   │   │   └── tasks/              # 任务实现(框架)
│   │   │       ├── movement/       # 移动任务
│   │   │       ├── action/         # 行动任务
│   │   │       └── interact/       # 互动任务
│   │   └── schedule/               # 日程
│   │       ├── Activity.hpp/cpp    # 15种活动类型
│   │       └── Schedule.hpp/cpp    # 日程系统
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
├── damage/                        # 伤害系统
│   ├── DamageSource.hpp           # 伤害来源
│   ├── CombatEntry.hpp/cpp        # 战斗记录条目
│   └── CombatTracker.hpp/cpp      # 战斗追踪器
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

### utils/ (非模板工具)

非模板实体工具函数：

- **ItemDropHelper**：物品掉落工具类
  - 统一的随机速度计算（方块掉落、简单掉落、玩家丢弃、发射器高斯）
  - 物品实体生成接口（spawnItemEntity、spawnItemAtEntity、spawnItemEntities）
  - 参考 MC 1.16.5 `InventoryHelper.spawnItemStack()`、`Entity.entityDropItem()`

- **EntityUtils**：旧实体类型映射
  - `legacyTypeToTypeId()` 将 LegacyEntityType 转换为 `minecraft:*` 字符串

> **注意**: `living/` 和 `mob/` 目录已被整合到 `core/` 目录中。
> - `LivingEntity` 现在位于 `core/LivingEntity.hpp/cpp`
> - `MobEntity` 现在位于 `core/MobEntity.hpp/cpp`
> - `AgeableEntity` 现在位于 `core/AgeableEntity.hpp/cpp`
> - `CreatureEntity` 现在位于 `core/CreatureEntity.hpp/cpp`
```

## 模块职责

### 核心实体层

#### Entity (实体基类)

所有游戏实体的基类，提供：
- **位置与运动**：位置、速度、旋转、碰撞箱
- **物理系统**：重力、碰撞检测、地面检测
- **环境检测**：水中、岩浆中、着火状态
- **乘客/骑乘系统**：多乘客支持、下车机制
- **数据同步**：EntityDataManager 数据参数
- **传送门系统**：传送门计时、传送冷却、维度切换
- **传送系统**：安全传送、随机传送、位置验证

```cpp
// 创建实体
auto pigType = EntityRegistry::instance().getType("minecraft:pig");
auto pig = pigType->create(world);

// 实体操作
entity->setPosition(100.0f, 64.0f, 200.0f);
entity->setVelocity(1.0f, 0.0f, 0.0f);
entity->tick();

// 传送门状态
entity->setInPortal(true);           // 标记在传送门中
entity->setPortalPos(blockPos);      // 记录传送门方块位置
int timer = entity->getPortalTime(); // 获取传送计时
entity->triggerPortalCooldown();     // 触发传送冷却

// 传送系统
entity->attemptTeleport(x, y, z, true);      // 安全传送到指定位置
entity->randomTeleport(16.0, true, true);    // 16格范围内随机传送
```

#### 传送系统

Entity 提供完整的传送功能实现，支持安全传送和随机传送：

**核心方法**：
- `attemptTeleport(x, y, z, playEffects)` - 安全传送到指定坐标
- `randomTeleport(range, playEffects, avoidFluid)` - 在范围内随机传送
- `findSafeTeleportPosition(x, y, z, avoidFluid)` - 查找安全传送位置
- `isSafeTeleportPosition(x, y, z, avoidFluid)` - 检查位置是否安全可传送

**传送算法**（参考 MC 1.16.5）：
1. 位置采样：在 `[x-range, x+range] × [y-8, y+8] × [z-range, z+range]` 范围内随机采样
2. 地面查找：从采样点向下遍历，找到第一个非空气方块
3. 安全检查：检查碰撞箱是否与方块碰撞、是否在流体中
4. 传送执行：更新实体位置、重置运动向量、触发世界事件

**使用示例**：
```cpp
// 紫颂果随机传送
bool success = player.randomTeleport(16.0, false, true);
if (success) {
    world.playSound(SoundEvents::ITEM_CHORUS_FRUIT_TELEPORT, ...);
}

// 末影人安全传送
bool teleported = enderman.attemptTeleport(targetX, targetY, targetZ, true);

// 避开水/岩浆传送
auto safePos = entity.findSafeTeleportPosition(x, y, z, true);
if (safePos.has_value()) {
    entity.setPosition(safePos.value());
}
```

#### 骑乘系统 (Riding System)

Entity 提供完整的骑乘系统实现，支持多乘客和嵌套骑乘：

**核心方法**：
- `startRiding(Entity& vehicle)` - 开始骑乘车辆
- `stopRiding()` - 停止骑乘（调用 dismount）
- `dismount()` - 内部下车方法
- `addPassenger(Entity& passenger)` - 添加乘客
- `removePassenger(Entity& passenger)` - 移除乘客
- `removePassengers()` - 移除所有乘客

**骑乘检查**：
- `isRiding()` - 是否正在骑乘
- `hasPassengers()` - 是否有乘客
- `getVehicle()` - 获取骑乘的车辆 ID
- `getPassengers()` - 获取乘客列表
- `canBeRidden(const Entity& vehicle)` - 是否可以被骑乘
- `getLowestRidingEntity()` - 获取最底层骑乘实体（支持嵌套）

**乘客位置**：
- `updatePassengerPosition(Entity& passenger)` - 更新乘客位置
- `applyOrientationToEntity(Entity& passenger)` - 应用朝向到乘客

```cpp
// 玩家骑乘马
player.startRiding(horse);

// 检查骑乘状态
if (player.isRiding()) {
    EntityId vehicleId = player.getVehicle();
    Entity* vehicle = world.getEntity(vehicleId);
}

// 下车
player.stopRiding();

// 车辆移除所有乘客
boat.removePassengers();
```

**网络同步**：
- `SetPassengersPacket` - 同步乘客列表
- `MoveVehiclePacket` - 客户端发送载具位置
- `VehicleMovePacket` - 服务端校正载具位置
- `PlayerInputPacket` - 客户端发送骑乘输入（移动、跳跃）
- `EntityActionPacket` - 实体动作（马跳跃蓄力、下马）

#### EntityType (实体类型)

定义实体类型的配置：
- **工厂函数**：创建实体实例
- **分类**：怪物、动物、环境、杂项
- **尺寸**：宽度、高度
- **追踪距离**：网络同步范围
- **更新间隔**：网络同步频率
- **标志**：免疫火焰、可召唤等

```cpp
// 注册实体类型
EntityRegistry::instance().registerType(
    "minecraft:pig",
    EntityType::Builder(&PigEntity::create, EntityClassification::Creature)
        .size(0.9f, 0.9f)
        .trackingRange(10)
        .updateInterval(3)
        .canSummon(true)
        .build()
);
```

### 继承层次

```
Entity
├── LivingEntity        # 有生命值的实体
│   ├── MobEntity       # 有 AI 的生物
│   │   ├── AgeableEntity  # 可成长
│   │   │   └── AnimalEntity  # 可繁殖的动物
│   │   │       ├── PigEntity
│   │   │       ├── CowEntity
│   │   │       ├── SheepEntity
│   │   │       └── ChickenEntity
│   │   └── CreatureEntity  # 陆地生物
│   └── PlayerEntity    # 玩家（服务端/客户端特定）
└── ItemEntity          # 物品实体
```

### AI 系统

#### 目标系统 (Goal System)

基于优先级和互斥标志的 AI 行为调度：

```cpp
// 在 MobEntity 中注册 AI 目标
void PigEntity::registerGoals() {
    m_goalSelector.addGoal(0, std::make_unique<SwimGoal>(this));
    m_goalSelector.addGoal(1, std::make_unique<PanicGoal>(this, 1.25));
    m_goalSelector.addGoal(2, std::make_unique<BreedGoal>(this, 1.0));
    m_goalSelector.addGoal(3, std::make_unique<TemptGoal>(this, 1.0, false, {Items::CARROT}));
    m_goalSelector.addGoal(4, std::make_unique<FollowParentGoal>(this, 1.1));
    m_goalSelector.addGoal(5, std::make_unique<RandomWalkingGoal>(this, 1.0));
    m_goalSelector.addGoal(6, std::make_unique<LookAtGoal>(this, PlayerEntity::class, 6.0f));
}
```

**互斥标志** (GoalFlag)：
- `MOVE` - 移动相关目标
- `LOOK` - 视线相关目标
- `JUMP` - 跳跃相关目标
- `TARGET` - 目标选择相关

#### 控制器 (Controllers)

处理实体的具体行为：
- **LookController**：控制头部旋转，看向目标
- **MovementController**：控制移动方向和速度
- **JumpController**：控制跳跃时机

#### 寻路系统 (Pathfinding)

A* 算法实现的寻路系统：

```cpp
// 使用寻路器
PathNavigator navigator(std::make_unique<PathFinder>(
    std::make_unique<WalkNodeProcessor>()));
navigator.setEntity(mobEntity);
navigator.setCanSwim(true);

// 移动到目标位置
if (navigator.moveTo(100.0, 64.0, 200.0, 1.0)) {
    // 路径找到，开始导航
}

// 每帧更新
navigator.tick();
```

### 属性系统

实体属性用于存储可修改的数值属性：

```cpp
// 获取属性值
f64 maxHealth = livingEntity.getAttributeValue(Attributes::MAX_HEALTH, 20.0);
f64 speed = livingEntity.getAttributeValue(Attributes::MOVEMENT_SPEED, 0.25);

// 设置属性基础值
livingEntity.setAttributeBaseValue(Attributes::MOVEMENT_SPEED, 0.3);

// 添加属性修饰符（如药水效果）
AttributeModifier modifier("speed_boost", 0.2, AttributeModifier::Operation::MULTIPLY_TOTAL);
attributeInstance.addModifier(modifier);
```

**标准属性**：
- `MAX_HEALTH` - 最大生命值
- `FOLLOW_RANGE` - 跟随范围
- `KNOCKBACK_RESISTANCE` - 击退抗性
- `MOVEMENT_SPEED` - 移动速度
- `ATTACK_DAMAGE` - 攻击伤害
- `ATTACK_SPEED` - 攻击速度
- `ARMOR` - 护甲值
- `ARMOR_TOUGHNESS` - 护甲韧性

### 伤害系统

定义各种伤害类型和来源：

```cpp
// 环境伤害
auto damage = DamageSources::lava();
auto damage = DamageSources::fall();
auto damage = DamageSources::drown();

// 实体伤害
auto damage = DamageSources::playerAttack(player);
auto damage = DamageSources::mobAttack(mob);

// 应用伤害
livingEntity.hurt(damage, 10.0f);
```

**伤害类型**：
- 环境伤害：火焰、岩浆、摔落、溺水、饥饿、仙人掌、虚空、魔法
- 实体伤害：玩家攻击、生物攻击、箭矢、三叉戟、火球、爆炸

### 背包系统

#### PlayerInventory

玩家背包实现：
- 快捷栏（9 槽位）
- 主背包（27 槽位）
- 护甲槽（4 槽位）
- 副手槽（1 槽位）

```cpp
PlayerInventory inventory;

// 物品操作
inventory.setSelectedSlot(0);
ItemStack selected = inventory.getSelectedStack();
inventory.add(itemStack);
inventory.swapSlots(0, 10);

// 护甲操作
inventory.setHelmet(ironHelmet);
inventory.setChestplate(ironChestplate);
```

### 掉落表系统

定义实体死亡、方块破坏时的物品掉落：

```cpp
// 创建掉落表
LootTableBuilder builder;
builder.id("minecraft:entities/pig");
builder.pool(std::make_unique<LootPool>(
    RandomValueRange(1.0f, 3.0f),  // 掷骰次数
    std::vector<std::unique_ptr<LootEntry>>{
        std::make_unique<ItemLootEntry>("minecraft:porkchop", RandomValueRange(1.0f, 3.0f), 100, 1)
    }
));

auto table = builder.build();

// 生成掉落物
auto context = LootContextBuilder(world)
    .withEntity(pig)
    .withDamageSource(damageSource)
    .build();
auto drops = table->generate(*context);
```

### 自动跳跃系统

MC 1.16.5 风格的自动跳跃检测：

```cpp
AutoJump autoJump;
autoJump.setEnabled(settings.autoJump.get());

// 每帧更新
autoJump.tick();

// 移动后检测
Vector2 movementInput(forward, strafe);
auto result = autoJump.check(player, physicsEngine, movementInput);
if (result.shouldJump) {
    player.jump();
}
```

## 文件间关系

### 核心依赖图

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

### AI 系统数据流

```
                    ┌─────────────────┐
                    │   MobEntity     │
                    │  (目标选择器)   │
                    └────────┬────────┘
                             │ tick()
                             ▼
                    ┌─────────────────┐
                    │  GoalSelector   │
                    │  (选择目标)     │
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
      ┌───────────┐  ┌───────────┐  ┌───────────┐
      │ SwimGoal  │  │PanicGoal  │  │RandomWalk │
      └───────────┘  └───────────┘  └───────────┘
              │              │              │
              └──────────────┼──────────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │  Controllers    │
                    │ Look/Move/Jump  │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │ PathNavigator   │
                    │  (寻路)         │
                    └─────────────────┘
```

## 输入与输出

### 输入

1. **世界数据**：通过 `IWorld` 接口获取方块、维度信息
2. **玩家输入**：移动、跳跃、攻击等输入事件
3. **网络数据**：实体同步、背包操作等网络包
4. **时间更新**：每 tick 调用 `tick()` 方法

### 输出

1. **实体状态更新**：位置、速度、生命值等
2. **网络同步**：实体位置、背包、属性同步到客户端
3. **事件触发**：死亡、受伤、繁殖等事件
4. **掉落物生成**：实体死亡时生成物品

## 依赖项

### 内部依赖

- `common/core/` - 核心类型、错误处理
- `common/util/math/` - 数学工具（Vector3, Random）
- `common/util/AxisAlignedBB.hpp` - 碰撞箱
- `common/item/ItemStack.hpp` - 物品堆
- `common/network/packet/` - 网络包
- `common/world/IWorld.hpp` - 世界接口
- `common/physics/PhysicsEngine.hpp` - 物理引擎

### 外部依赖

- 无外部库依赖（仅标准库）

## 使用方法

### 注册新实体类型

```cpp
// 1. 定义实体类
class MyEntity : public MobEntity {
public:
    MyEntity(LegacyEntityType type, EntityId id)
        : MobEntity(type, id) {
        registerGoals();
    }

    static std::unique_ptr<Entity> create(IWorld* world) {
        return std::make_unique<MyEntity>(LegacyEntityType::Unknown, 0);
    }

protected:
    void registerGoals() override {
        m_goalSelector.addGoal(0, std::make_unique<SwimGoal>(this));
        // 添加更多目标...
    }
};

// 2. 注册实体类型
EntityRegistry::instance().registerType(
    "minecraft:my_entity",
    EntityType::Builder(&MyEntity::create, EntityClassification::Creature)
        .size(1.0f, 1.0f)
        .trackingRange(10)
        .build()
);
```

### 创建自定义 AI 目标

```cpp
class MyCustomGoal : public Goal {
public:
    MyCustomGoal(MobEntity* mob, f64 speed)
        : Goal(GoalFlag::MOVE)  // 设置互斥标志
        , m_mob(mob)
        , m_speed(speed)
    {}

    bool shouldExecute() override {
        // 检查是否应该执行
        return m_mob != nullptr && /* 条件 */;
    }

    bool shouldContinueExecuting() override {
        // 检查是否应该继续
        return shouldExecute() && /* 条件 */;
    }

    void startExecuting() override {
        // 初始化状态
        m_mob->getNavigator()->moveTo(m_targetX, m_targetY, m_targetZ, m_speed);
    }

    void resetTask() override {
        // 清理状态
        m_mob->getNavigator()->clearPath();
    }

    void tick() override {
        // 每帧更新
    }

private:
    MobEntity* m_mob;
    f64 m_speed;
    f32 m_targetX, m_targetY, m_targetZ;
};
```

### 使用属性系统

```cpp
// 注册新属性
Attribute myAttribute("generic.my_attribute", 10.0, 0.0, 100.0);

// 在 LivingEntity 中使用
void LivingEntity::registerAttributes() {
    m_attributes.registerAttribute(Attributes::MAX_HEALTH);
    m_attributes.registerAttribute(Attributes::MOVEMENT_SPEED);
    // 设置基础值
    m_attributes.setBaseValue(Attributes::MAX_HEALTH, 20.0);
}

// 添加修饰符（药水效果等）
AttributeModifier speedBoost("speed_potion", 0.2, AttributeModifier::MULTIPLY_TOTAL);
m_attributes.getOrCreateInstance(Attributes::MOVEMENT_SPEED).addModifier(speedBoost);
```

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

```cpp
// 错误：多个目标共享同一标志，优先级设置不当
m_goalSelector.addGoal(0, std::make_unique<SwimGoal>(this));  // MOVE 标志
m_goalSelector.addGoal(1, std::make_unique<RandomWalkingGoal>(this, 1.0));  // MOVE 标志
// 两个目标可能冲突

// 正确：检查互斥标志，合理设置优先级
// SwimGoal 使用 MOVE 标志，优先级最高
// RandomWalkingGoal 也使用 MOVE 标志，但优先级较低
// GoalSelector 会自动处理冲突
```

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

```cpp
// 错误：修饰符添加后未移除
AttributeModifier boost("temp_boost", 1.0, AttributeModifier::ADD);
instance.addModifier(boost);
// 效果结束后未移除

// 正确：效果结束时移除
void onEffectEnd() {
    instance.removeModifier(boost.getId());
}
```

### 5. 掉落表内存管理

```cpp
// 错误：持有原始指针
LootTable* table = lootManager.getTable("minecraft:entities/pig");
// manager 可能释放掉落表

// 正确：每次使用时获取
const LootTable* table = lootManager.getTable("minecraft:entities/pig");
if (table) {
    auto drops = table->generate(context);
}
```

### 6. 线程安全

```cpp
// 错误：在多线程中直接访问实体
void asyncTask() {
    entity->setPosition(x, y, z);  // 可能与其他线程冲突
}

// 正确：在主线程更新
void asyncTask() {
    mainThreadQueue.push([entity, x, y, z]() {
        entity->setPosition(x, y, z);
    });
}
```

### 7. EntityFlags 枚举类型

```cpp
// 注意：Entity 类和 entity::EntityType 各有自己的 flags 枚举
// Entity::EntityFlags - 实体运行时状态（燃烧、潜行等）
// entity::EntityFlags - 实体类型特性（免疫火焰等）
```

### 8. 碰撞箱与姿态刷新

- `Entity::refreshDimensions()` 现在是尺寸变化后的统一刷新入口，内部会同步 `EntitySize` 和 `AxisAlignedBB` 缓存。
- 运行时会改变体型的实体子类，应在尺寸改变后立即调用刷新函数，避免旧碰撞箱继续参与移动和地面检测。
- 玩家从蹲下、游泳、睡眠切回站立时，需要先做碰撞可容纳性检查，不能直接无条件切换到 `Standing`。

### 9. 声音链路

- `Entity::playSound(...)` 会把声音事件交给当前 `IWorld`，实体层不直接做广播。
- `LivingEntity` 统一处理受伤和死亡声音，`MobEntity` 统一处理环境声，`Player` 也沿用同一条链路。
- `ServerWorld` 再把这些事件挂到服务器广播回调上，最后由 `MinecraftServer` 发给附近玩家。

### 10. TemptGoal 物品过滤

**问题**：`TemptGoal` 需要正确过滤真正的 `Player` 实体。

**解决方案**：`TemptGoal` 现在通过主手/副手物品堆过滤真正的 `Player` 实体，不要仅通过 stub 移动来伪造这些目标。

### 11. PanicGoal 和 WaterAvoidingRandomWalkingGoal

**问题**：这些目标需要正确查询世界状态。

**解决方案**：`PanicGoal` / `WaterAvoidingRandomWalkingGoal` 现在直接查询 `IWorld::isWaterAt(...)` / `isLavaAt(...)`；保持测试与世界查询接口一致。

### 12. ChickenEntity 鸡蛋生成

**问题**：鸡的 `tick()` 在计时器到期时生成鸡蛋物品实体，生成后未重置计时器会导致批量发射鸡蛋。

**解决方案**：生成后立即重置计时器，否则鸡会批量发射鸡蛋。

### 13. Entity::getTypeId() 类型注入

**问题**：依赖 `LegacyEntityType` 单独决定网络实体类型，很多工厂构造仍传 `LegacyEntityType::Unknown`。

**解决方案**：`Entity::getTypeId()` 现在优先使用在 `EntityType::create(...)` 期间注入的显式运行时 `typeId`。保证实体通过注册表创建时注入注册名，繁殖等旁路也要显式继承父类型。

### 14. EntityMetadataPacket 同步

**问题**：`EntityMetadataPacket` / `EntityMetadataSerializer` 需要同时供给服务器跟踪和客户端实体应用。

**解决方案**：`EntityTracker` 负责 spawn 内联 metadata 和 dirty metadata packet，`ClientEntity::setMetadata()` 负责把原始数据写进本地数据管理器；新增字段时三处必须一起改。

### 15. CactusBlock 碰撞伤害

**问题**：仙人掌碰撞伤害应仅针对活体实体。

**解决方案**：使用 `LivingEntity::hurt()` 和 `DamageSources::cactus()`；非活体碰撞应保持无操作。

### 16. DyeableArmorItem 颜色清除

**问题**：`DyeableArmorItem` 将颜色存储在 `ItemStack` 的结构化标签树中，清除颜色时未清除空的 `display` 标签会导致元数据相等性发散。

**解决方案**：清除颜色时也必须清除空的 `display` 标签，否则盔甲堆将停止按预期合并。

### 17. 玩家站起过渡

**问题**：玩家从蹲伏/游泳/睡眠切换时，无条件切换到站立姿势可能导致低天花板下的碰撞问题。

**解决方案**：玩家站起过渡现在在从蹲伏/游泳/睡眠切换之前检查目标姿势盒子是否适合。当需要原版风格的低天花板行为时，不要用原始站立姿势更改绕过 `setSneaking()` / `setSwimming()` / `setSleeping()`。

### 18. Player::updateMoveDistance 状态管理

**问题**：把 `prevPosition` 当成脚步声或视野晃动的采样基准会导致重复计数。

**解决方案**：`Player::updateMoveDistance()` 现在使用专用的采样位置，`Player::setPosition()` 重置移动/晃动状态；不要再把 `prevPosition` 当成脚步声或视野晃动的采样基准，它是插值历史状态，多次物理更新会把同一段位移重复计数。

### 19. Player::updatePhysics 轻量级测试

**问题**：在没有物理引擎的轻量级测试世界中，`Player::updatePhysics()` 的行为需要正确处理。

**解决方案**：`Player::updatePhysics()` 可以在没有物理引擎的轻量级测试世界中运行；在这种情况下，代码会回退到直接移动，所以测试应该验证状态刷新，而不是假设存在完整的碰撞求解器。

### 20. m_player->isInWater() 客户端权威性

**问题**：假设 `m_player->isInWater()` 在客户端是权威的会导致错误判断。

**解决方案**：不要假设 `m_player->isInWater()` 在客户端是权威的；这个值只会在 `Entity::baseTick()` / `updateEnvironmentState()` 或本地物理刷新路径里更新，它对本地玩家可用，但仍不是服务端权威结果。

### 21. ClientWorld::entityManager() 返回类型

**问题**：`ClientWorld::entityManager()` 返回 `ClientEntityManager`，而不是共享的 `common::EntityManager`。

**解决方案**：客户端本地 `Player` 不会在这条链路里跑 `Player::tick()`；客户端只会 tick 代理实体和本地物理。

## 涉及的测试用例

测试文件位于 `tests/entity/` 和 `tests/common/entity/` 目录：

| 测试文件 | 测试内容 |
|---------|---------|
| `EntityCoreTests.cpp` | Entity 基类核心功能 |
| `LivingEntityTests.cpp` | LivingEntity 生命值、属性、装备、受伤、死亡、环境声发声链路 |
| `AttributeTests.cpp` | 属性系统、修饰符计算 |
| `GoalTests.cpp` | AI 目标选择器、优先级、互斥标志 |
| `PathfindingTests.cpp` | A* 寻路算法、路径导航 |
| `RandomWalkingGoalTest.cpp` | 随机漫步目标 |
| `LootTest.cpp` | 掉落表生成、条件判断 |
| `LootConditionTest.cpp` | 掉落条件系统 |
| `LootSerializersTest.cpp` | JSON 序列化、解析和往返测试 |
| `AutoJumpTest.cpp` | 自动跳跃检测 |
| `PlayerMovementTest.cpp` | 玩家移动物理、受伤和死亡声音事件 |
| `PlayerPoseCollisionTest.cpp` | 玩家姿态切换与碰撞箱可容纳性 |
| `EntitySpawnPlacementRegistryTest.cpp` | 实体生成放置规则 |
| `CraftingInventoryTest.cpp` | 合成背包功能 |
| `AnimalModelTests.cpp` | 动物渲染模型 |
| `ServerWorldTest.cpp` | 服务端世界声音回调转发 |
| `tests/common/item/tool/ShearsItemTest.cpp` | 剪刀物品与羊剪毛交互测试 |
| `tests/common/item/special/BucketItemTest.cpp` | 桶物品与牛挤奶交互测试 |
| `tests/common/entity/utils/ItemDropHelperTest.cpp` | 物品掉落速度和实体生成测试 |

## 参考

本实体系统参考 MC 1.16.5 源码实现，主要参考类：

- `net.minecraft.entity.Entity`
- `net.minecraft.entity.LivingEntity`
- `net.minecraft.entity.MobEntity`
- `net.minecraft.entity.ai.goal.Goal`
- `net.minecraft.entity.ai.brain.Brain`
- `net.minecraft.entity.ai.controller.LookController`
- `net.minecraft.entity.ai.controller.MovementController`
- `net.minecraft.pathfinding.PathNavigator`
- `net.minecraft.pathfinding.PathFinder`
- `net.minecraft.entity.ai.attributes.Attribute`
- `net.minecraft.util.DamageSource`
- `net.minecraft.entity.player.PlayerInventory`
- `net.minecraft.loot.LootTable`
