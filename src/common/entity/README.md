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
│   ├── EntityUtils.hpp             # 实体工具函数
│   ├── DataParameter.hpp           # 数据参数定义
│   ├── MoverType.hpp               # 移动类型枚举
│   └── VanillaEntities.hpp         # 原版实体注册
│
├── entities/                       # 具体实体实现
│   ├── passive/                    # 被动/中立生物
│   │   ├── basic/                  # 普通动物
│   │   │   ├── AnimalEntity.hpp/cpp # 动物基类
│   │   │   ├── PigEntity.hpp/cpp   # 猪
│   │   │   ├── CowEntity.hpp/cpp   # 牛
│   │   │   ├── SheepEntity.hpp/cpp # 羊
│   │   │   └── ChickenEntity.hpp/cpp # 鸡
│   │   ├── tamable/                # 可驯服动物
│   │   │   └── TameableEntity.hpp/cpp # 可驯服基类
│   │   ├── special/                # 特殊动物
│   │   ├── horse/                  # 马类
│   │   ├── fish/                   # 鱼类
│   │   ├── water/                  # 水生生物
│   │   ├── ambient/                # 环境生物
│   │   └── golem/                  # 傀儡
│   │
│   ├── monster/                    # 敌对生物
│   │   ├── MonsterEntity.hpp/cpp   # 敌对生物基类
│   │   ├── undead/                 # 亡灵类
│   │   ├── arthropod/              # 节肢类
│   │   ├── nether/                 # 地狱生物
│   │   ├── end/                    # 末地生物
│   │   ├── basic/                  # 基础怪物
│   │   ├── ocean/                  # 海洋怪物
│   │   └── illager/                # 灾厄村民
│   │
│   ├── boss/                       # Boss生物
│   ├── villager/                   # 村民/商人
│   ├── projectile/                 # 投掷物
│   ├── vehicle/                    # 交通工具
│   ├── item/                       # 物品相关实体
│   │   └── ItemEntity.hpp/cpp      # 掉落物品
│   ├── hanging/                    # 悬挂实体
│   ├── effect/                     # 效果实体
│   ├── equipment/                  # 装备实体
│   └── player/                     # 玩家实体
│       ├── Player.hpp/cpp          # 玩家实体
│       ├── PlayerManager.hpp/cpp   # 玩家管理器
│       └── GameModeUtils.hpp/cpp   # 游戏模式工具
│
├── interfaces/                     # 实体接口
│   ├── IAngerable.hpp              # 愤怒接口
│   ├── IRideable.hpp               # 可骑乘接口
│   ├── IShearable.hpp              # 可剪毛接口
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
│   ├── brain/                      # 大脑系统（预留）
│   │   ├── memory/                 # 记忆模块
│   │   ├── sensor/                 # 传感器
│   │   ├── task/                   # 任务
│   │   └── schedule/               # 日程
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
├── living/                        # 生物实体
│   └── LivingEntity.hpp/cpp       # 生物基类（生命值、属性、装备）
│
├── mob/                           # Mob 实体
│   ├── MobEntity.hpp/cpp          # Mob 基类（AI 系统、控制器）
│   ├── AgeableEntity.hpp/cpp      # 可成长实体
│   └── CreatureEntity.hpp/cpp     # 生物实体（陆地移动）
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
    └── RandomRanges.hpp/cpp       # 随机范围
```

## 模块职责

### 核心实体层

#### Entity (实体基类)

所有游戏实体的基类，提供：
- **位置与运动**：位置、速度、旋转、碰撞箱
- **物理系统**：重力、碰撞检测、地面检测
- **环境检测**：水中、岩浆中、着火状态
- **乘客/骑乘系统**：多乘客支持
- **数据同步**：EntityDataManager 数据参数

```cpp
// 创建实体
auto pigType = EntityRegistry::instance().getType("minecraft:pig");
auto pig = pigType->create(world);

// 实体操作
entity->setPosition(100.0f, 64.0f, 200.0f);
entity->setVelocity(1.0f, 0.0f, 0.0f);
entity->tick();
```

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

## 涉及的测试用例

测试文件位于 `tests/entity/` 和 `tests/common/entity/` 目录：

| 测试文件 | 测试内容 |
|---------|---------|
| `EntityCoreTests.cpp` | Entity 基类核心功能 |
| `LivingEntityTests.cpp` | LivingEntity 生命值、属性、装备 |
| `AttributeTests.cpp` | 属性系统、修饰符计算 |
| `GoalTests.cpp` | AI 目标选择器、优先级、互斥标志 |
| `PathfindingTests.cpp` | A* 寻路算法、路径导航 |
| `RandomWalkingGoalTest.cpp` | 随机漫步目标 |
| `LootTest.cpp` | 掉落表生成、条件判断 |
| `LootConditionTest.cpp` | 掉落条件系统 |
| `AutoJumpTest.cpp` | 自动跳跃检测 |
| `PlayerMovementTest.cpp` | 玩家移动物理 |
| `EntitySpawnPlacementRegistryTest.cpp` | 实体生成放置规则 |
| `CraftingInventoryTest.cpp` | 合成背包功能 |
| `AnimalModelTests.cpp` | 动物渲染模型 |

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
