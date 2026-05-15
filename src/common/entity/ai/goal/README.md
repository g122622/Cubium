# AI 目标系统 (Goal System)

## 目录结构

```
goal/
├── Goal.hpp                  # AI目标基类
├── GoalConstants.hpp         # 目标系统常量定义
├── GoalFlag.hpp              # 互斥标志枚举
├── GoalSelector.hpp          # 目标选择器
├── PrioritizedGoal.hpp       # 带优先级的目标包装器
└── goals/                    # 具体目标实现
    ├── AvoidEntityGoal.hpp/cpp   # 避开实体目标
    ├── BreedGoal.hpp/cpp         # 繁殖目标
    ├── EatGrassGoal.hpp/cpp      # 吃草目标（羊等草食动物）
    ├── FollowParentGoal.hpp/cpp  # 跟随父母目标
    ├── LookAtGoal.hpp/cpp        # 看向目标/随机看向
    ├── MeleeAttackGoal.hpp/cpp   # 近战攻击目标
    ├── PanicGoal.hpp/cpp         # 恐慌逃跑目标
    ├── RandomWalkingGoal.hpp/cpp # 随机漫步目标
    ├── SwimGoal.hpp/cpp          # 游泳目标
    ├── TemptGoal.hpp/cpp         # 食物诱惑目标
    ├── RandomSwimmingGoal.hpp/cpp # 随机游泳目标（水生生物）
    ├── FindWaterGoal.hpp/cpp     # 寻找水源目标（水生生物）
    ├── SwimUpGoal.hpp/cpp        # 向上游目标（水生生物）
    ├── AdditionalGoals.hpp/cpp   # 其他目标(占位符)
    ├── movement/                 # 移动类目标
    │   └── MovementGoals.hpp/cpp # WaterAvoidingRandomWalkingGoal, LeapAtTargetGoal
    │   └── FollowSchoolLeaderGoal.hpp/cpp # 跟随群体领导者（群游鱼类）
    ├── attack/                   # 攻击类目标
    │   └── RangedAttackGoals.hpp/cpp # RangedAttackGoal, RangedBowAttackGoal
    ├── target/                   # 目标选择目标
    │   └── TargetGoals.hpp/cpp   # TargetGoal, NearestAttackableTargetGoal, HurtByTargetGoal等
    ├── interact/                 # 交互类目标
    │   └── TameableGoals.hpp/cpp # FollowOwnerGoal, SitGoal, BegGoal
    └── special/                  # 特殊目标
        ├── SpecialGoals.hpp/cpp  # CreeperSwellGoal, EndermanTeleportGoal等
        ├── GuardianAttackGoal.hpp/cpp # 守卫者激光攻击目标
        └── MoveToLavaGoal.hpp/cpp # 移动到方块目标基类、炽足兽寻找熔岩目标
```

## 整体职责

AI 目标系统负责管理实体的智能行为决策。每个 AI 行为（如游泳、漫步、攻击、繁殖等）都被封装为一个独立的 Goal 对象，通过 GoalSelector 根据优先级和互斥标志协调多个目标的执行。

### 核心设计理念

1. **模块化**: 每个 AI 行为是独立的目标类，易于扩展和维护
2. **优先级系统**: 数值越小优先级越高，高优先级目标可以抢占低优先级目标
3. **互斥机制**: 通过 GoalFlag 标志防止冲突行为同时执行
4. **生命周期管理**: 统一的执行流程 (`shouldExecute` → `startExecuting` → `tick` → `resetTask`)

---

## 文件详细介绍

### 核心框架文件

#### Goal.hpp - AI目标基类

**职责**: 定义所有 AI 目标的抽象基类，声明生命周期的核心接口。

**主要方法**:
| 方法 | 说明 |
|------|------|
| `shouldExecute()` | 检查当前条件是否满足执行此目标（纯虚函数） |
| `shouldContinueExecuting()` | 检查是否应该继续执行，默认调用 `shouldExecute()` |
| `isPreemptible()` | 是否可被抢占，默认返回 `true` |
| `startExecuting()` | 目标开始时调用，用于初始化状态 |
| `resetTask()` | 目标被中断时调用，用于清理状态 |
| `tick()` | 每游戏刻调用，更新正在执行的目标 |
| `getMutexFlags()` | 获取互斥标志集合 |

**关键成员**:
- `m_flags`: 互斥标志集合，用于控制目标间的互斥关系

```cpp
// 示例：自定义目标
class MyGoal : public Goal {
public:
    bool shouldExecute() override {
        return m_entity->isHungry();
    }

    void startExecuting() override {
        m_entity->startEating();
    }

    void tick() override {
        m_entity->continueEating();
    }
};
```

---

#### GoalFlag.hpp - 互斥标志枚举

**职责**: 定义目标互斥标志，用于控制哪些目标不能同时运行。

**标志类型**:
| 标志 | 说明 | 典型使用场景 |
|------|------|--------------|
| `Move` | 移动控制 | 漫步、跟随、逃跑、攻击 |
| `Look` | 视线控制 | 看向玩家、看向攻击目标 |
| `Jump` | 跳跃控制 | 游泳、自动跳跃 |
| `Target` | 目标选择 | 攻击目标选择、仇恨系统 |

**互斥原理**: 如果两个目标共享相同的标志，则它们不能同时运行。例如，`RandomWalkingGoal` 和 `MeleeAttackGoal` 都使用 `Move` 标志，因此当实体攻击时不会同时漫步。

```cpp
// 示例：设置互斥标志
setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
```

---

#### GoalConstants.hpp - 目标系统常量

**职责**: 定义目标系统使用的常量，避免魔法数字。

**常量分类**:

| 类别 | 常量 | 值 | 说明 |
|------|------|-----|------|
| 距离 | `DEFAULT_FOLLOW_DISTANCE` | 10.0f | 默认跟随距离 |
| 距离 | `BREED_DISTANCE` | 3.0f | 繁殖所需距离 |
| 距离 | `TEMPT_RANGE` | 10.0f | 诱惑检测范围 |
| 距离 | `AVOID_DETECTION_RANGE` | 16.0f | 避开检测距离 |
| 距离 | `MELEE_ATTACK_REACH` | 2.0f | 近战攻击范围 |
| 时间 | `MELEE_ATTACK_COOLDOWN` | 20 tick | 攻击冷却 (1秒) |
| 时间 | `PATH_RECALCULATE_INTERVAL` | 5 tick | 路径重算间隔 |
| 时间 | `MAX_WALK_TIME` | 600 tick | 最大漫步时间 (30秒) |
| 概率 | `DEFAULT_LOOK_CHANCE` | 0.02f | 默认看向概率 (2%) |
| 速度 | `AVOID_NEAR_SPEED` | 1.5 | 近距离逃跑速度 |

---

#### GoalSelector.hpp - 目标选择器

**职责**: 管理实体的所有 AI 目标，负责选择和执行当前应该运行的目标。

**核心流程**:

```mermaid
flowchart TD
    A[tick 调用] --> B[清理已停止的目标]
    B --> C{目标是否应该继续?}
    C -->|否| D[调用 resetTask]
    D --> E[清除 flagGoals 映射]
    C -->|是| F[保留目标]
    E --> G[选择新目标]
    F --> G
    G --> H{遍历所有目标}
    H --> I{目标未运行且可启动?}
    I -->|是| J[停止冲突的低优先级目标]
    J --> K[启动新目标]
    I -->|否| L[跳过]
    K --> M[更新 flagGoals 映射]
    L --> M
    M --> N[更新所有运行中的目标 tick]
```

**主要方法**:
| 方法 | 说明 |
|------|------|
| `addGoal(priority, goal)` | 添加目标，数值越小优先级越高 |
| `removeGoal(goal)` | 移除指定目标 |
| `removeAllGoals()` | 移除所有目标 |
| `tick()` | 每刻更新，选择和执行目标 |
| `disableFlag(flag)` | 禁用指定标志的目标 |
| `enableFlag(flag)` | 启用指定标志的目标 |
| `forEachRunningGoal(func)` | 遍历所有运行中的目标 |

**关键成员**:
- `m_goals`: 所有目标的列表（按添加顺序）
- `m_flagGoals`: 标志到正在运行的目标的映射
- `m_disabledFlags`: 被禁用的标志集合

---

#### PrioritizedGoal.hpp - 带优先级的目标包装器

**职责**: 包装 Goal 对象并添加优先级信息，实现优先级比较和抢占逻辑。

**主要方法**:
| 方法 | 说明 |
|------|------|
| `getPriority()` | 获取优先级（数值越小优先级越高） |
| `isRunning()` | 是否正在运行 |
| `isPreemptedBy(other)` | 是否可以被另一个目标抢占 |
| `getGoal()` | 获取内部目标指针 |

**抢占规则**:
```cpp
bool isPreemptedBy(const PrioritizedGoal& other) const {
    return isPreemptible() && other.m_priority < m_priority;
}
```
只有当目标可抢占且另一个目标优先级更高时，才会被抢占。

---

### 具体目标实现

#### AvoidEntityGoal - 避开实体目标

**职责**: 使生物避开特定类型的实体（如村民躲避僵尸）。

**执行条件**: 检测到威胁实体在避开距离内

**行为**:
1. 寻找最近的威胁实体
2. 计算逃跑方向（远离威胁）
3. 根据距离调整速度（近距离跑更快）

**互斥标志**: `Move`

**关键参数**:
- `m_avoidDistance`: 避开检测距离
- `m_farSpeed`: 远距离逃跑速度
- `m_nearSpeed`: 近距离逃跑速度

---

#### BreedGoal - 繁殖目标

**职责**: 使两只动物靠近并繁殖。

**执行条件**: 动物处于爱心状态（被喂食后）

**行为**:
1. 寻找附近同类型且同样处于爱心状态的配偶
2. 移动向配偶
3. 距离足够近时生成幼体

**互斥标志**: `Move`, `Look`

**关键参数**:
- `m_speed`: 移动速度
- `SPAWN_BABY_DELAY`: 繁殖延迟 (60 tick)

---

#### FollowParentGoal - 跟随父母目标

**职责**: 幼体动物跟随成年动物。

**执行条件**: 实体是幼体且有附近的成年同类

**行为**:
1. 搜索附近的成年动物
2. 保持跟随但维持最小距离
3. 定期更新路径

**互斥标志**: `Move`

**关键参数**:
- `FOLLOW_PARENT_MIN_DISTANCE`: 最小跟随距离 (3.0f)
- `FOLLOW_PARENT_MAX_DISTANCE`: 最大跟随距离 (10.0f)

---

#### LookAtGoal - 看向目标

**职责**: 使生物看向附近的实体。

**执行条件**: 以一定概率触发

**行为**:
1. 寻找附近的实体
2. 使用 LookController 看向目标
3. 持续一定时间

**互斥标志**: `Look`

**关键参数**:
- `m_maxDistance`: 最大观看距离
- `m_chance`: 执行概率
- `DEFAULT_LOOK_CHANCE`: 默认概率 (0.02f, 2%)

**构造函数重载**:

| 构造函数 | 说明 |
|----------|------|
| `LookAtGoal(mob, maxDistance)` | 看向任意 LivingEntity，默认概率 |
| `LookAtGoal(mob, maxDistance, chance)` | 看向任意 LivingEntity，指定概率 |
| `LookAtGoal(mob, maxDistance, chance, filter)` | 看向自定义过滤的实体 |
| `LookAtGoal(mob, maxDistance, chance, TypeFilter<T>{})` | 看向特定类型的实体 |

**类型过滤使用示例**:
```cpp
// 看向附近的炽足兽（MC 1.16.5: new LookAtGoal(this, StriderEntity.class, 8.0F)）
m_goalSelector.addGoal(9, std::make_unique<LookAtGoal>(
    this, 8.0f, LookAtGoal::DEFAULT_LOOK_CHANCE, TypeFilter<StriderEntity>{}));

// 看向附近的玩家
m_goalSelector.addGoal(6, std::make_unique<LookAtGoal>(
    this, 8.0f, LookAtGoal::DEFAULT_LOOK_CHANCE, TypeFilter<Player>{}));

// 看向附近的村民
m_goalSelector.addGoal(7, std::make_unique<LookAtGoal>(
    this, 10.0f, 0.05f, TypeFilter<VillagerEntity>{}));

// 使用自定义过滤条件
m_goalSelector.addGoal(8, std::make_unique<LookAtGoal>(
    this, 12.0f, 0.02f, [](const LivingEntity* entity) {
        // 只看向成年动物
        const AnimalEntity* animal = dynamic_cast<const AnimalEntity*>(entity);
        return animal != nullptr && !animal->isChild();
    }));
```

**派生类**: `LookRandomlyGoal` - 随机看向某个方向

---

#### MeleeAttackGoal - 近战攻击目标

**职责**: 使生物攻击目标实体。

**执行条件**: 有攻击目标且目标存活

**行为**:
1. 追踪攻击目标
2. 定期更新路径
3. 攻击冷却结束后攻击
4. 应用伤害和击退

**互斥标志**: `Move`, `Look`

**关键参数**:
- `m_speed`: 追踪速度
- `m_useLongMemory`: 是否使用长期记忆（目标丢失后继续追踪）
- `ATTACK_COOLDOWN_TICKS`: 攻击冷却 (20 tick)

---

#### PanicGoal - 恐慌逃跑目标

**职责**: 当实体受到攻击或着火时，随机逃跑。

**执行条件**: 实体受到攻击或着火

**行为**:
1. 如果着火，尝试寻找水源
2. 否则随机选择逃跑方向
3. 持续逃跑直到安全

**互斥标志**: `Move`

**关键参数**:
- `m_speed`: 逃跑速度
- `PANIC_ESCAPE_MIN_DISTANCE`: 最小逃跑距离 (5.0f)
- `PANIC_ESCAPE_MAX_DISTANCE`: 最大逃跑距离 (10.0f)

---

#### RandomWalkingGoal - 随机漫步目标

**职责**: 使生物随机选择一个方向并移动。

**执行条件**: 概率触发，且实体空闲

**行为**:
1. 检查概率和空闲时间
2. 选择随机目标位置
3. 移动到目标位置
4. 超时后停止

**互斥标志**: `Move`

**关键参数**:
- `m_speed`: 移动速度
- `m_executionChance`: 执行概率倒数
- `MAX_WALK_TIME`: 最大漫步时间 (600 tick)

---

#### SwimGoal - 游泳目标

**职责**: 当实体在水中或岩浆中时，尝试向上游动。

**执行条件**: 实体在水中或岩浆中

**行为**:
1. 检测是否在液体中
2. 随机触发跳跃以游泳

**互斥标志**: `Jump`

**特点**: 非常简单的目标，只控制跳跃行为。

---

#### TemptGoal - 食物诱惑目标

**职责**: 当玩家手持特定物品时，动物会被诱惑跟随玩家。

**执行条件**: 附近有玩家手持诱惑物品

**行为**:
1. 检测玩家手持物品
2. 跟随玩家移动
3. 保持适当距离
4. 可选：被玩家移动吓跑

**互斥标志**: `Move`, `Look`

**关键参数**:
- `m_speed`: 跟随速度
- `m_scaredByMovement`: 是否被玩家移动吓跑
- `TEMPT_COOLDOWN`: 诱惑冷却 (100 tick)

---

#### BegGoal - 乞求目标

**职责**: 当玩家手持食物或驯服物品时，动物会看向玩家并乞求。主要用于狼（狗）的行为。

**执行条件**: 附近有玩家手持驯服物品（仅已驯服动物）或繁殖物品（所有动物）

**行为**:
1. 检测玩家手持物品
2. 看向玩家
3. 播放乞求动画（头部摆动）

**互斥标志**: `Look`

**关键参数**:
- `m_maxDistance`: 最大乞求距离 (默认 8.0f)
- `m_targetPlayer`: 当前乞求目标

**驯服物品检查逻辑** (MC 1.16.5 `BegGoal.hasTemptationItemInHand`):
```cpp
bool isPlayerHoldingFood(const Player* player) const {
    // 检查主手和副手
    for (Hand hand : {MainHand, OffHand}) {
        ItemStack stack = player->getHeldItem(hand);
        if (stack.isEmpty()) continue;
        
        // 已驯服的动物：对驯服物品和繁殖物品都乞求
        if (m_entity->isTamed() && m_entity->isTameItem(stack)) {
            return true;
        }
        // 所有动物：对繁殖物品乞求
        if (m_entity->isBreedingItem(stack)) {
            return true;
        }
    }
    return false;
}
```

**使用示例**:
```cpp
void WolfEntity::registerGoals() {
    // 优先级 9: 乞求目标（看向手持骨头或肉类的玩家）
    // 参考 MC 1.16.5 WolfEntity.registerGoals() - BegGoal 优先级为 9
    m_goalSelector.addGoal(9, std::make_unique<BegGoal>(this, 8.0f));
}
```

**与 TemptGoal 的区别**:
| 特性 | BegGoal | TemptGoal |
|------|---------|-----------|
| 行为 | 只看向玩家，不移动 | 跟随玩家移动 |
| 适用动物 | 狼 | 牛、猪、羊等 |
| 互斥标志 | `Look` | `Move`, `Look` |
| 驯服物品支持 | 已驯服动物对驯服物品乞求 | 不检查驯服状态 |

**参考**: MC 1.16.5 `net.minecraft.entity.ai.goal.BegGoal`

---

#### EatGrassGoal - 吃草目标

**职责**: 使草食动物（如羊）吃草方块或草丛。

**执行条件**: 概率触发（幼年 1/50，成年 1/1000），且当前位置或下方有草

**行为**:
1. 概率检查决定是否尝试吃草
2. 检测当前位置的草/高草丛或下方的草方块
3. 播放吃草动画（40 ticks）
4. 第 4 tick 时执行吃草动作：
   - 检查 `mobGriefing` 游戏规则
   - 如果 `mobGriefing=true`：
     - 草方块：播放破坏效果，转换为泥土
     - 草/高草丛：移除（不掉落物品）
   - 如果 `mobGriefing=false`：不改变方块
   - 无论规则如何，都调用 `eatGrassBonus` 回调
5. 调用回调函数通知实体（如羊重新长毛、幼体加速成长）

**互斥标志**: `Move`, `Look`, `Jump`

**关键参数**:
- `EAT_DURATION`: 吃草动画持续时间 (40 ticks)
- `EAT_TICK`: 执行吃草动作的时机 (第 4 tick)
- `CHILD_CHANCE`: 幼年动物触发概率倒数 (50)
- `ADULT_CHANCE`: 成年动物触发概率倒数 (1000)

**游戏规则检查**:
- 检查 `GameRuleKeys::MOB_GRIEFING`
- 参考 MC 1.16.5: `ForgeEventFactory.getMobGriefingEvent(world, entity)`
- 使用 `world->getGameRules().getBoolean(GameRuleKeys::MOB_GRIEFING)`

**使用示例**:
```cpp
void SheepEntity::registerGoals() {
    // ...其他目标...
    
    // 优先级 5: 吃草
    m_goalSelector.addGoal(5, std::make_unique<EatGrassGoal>(
        this,
        [this]() { this->eatGrassBonus(); },  // 吃草回调
        [this]() { return this->isChild(); }   // 是否幼年检查
    ));
}
```

**参考**: MC 1.16.5 `net.minecraft.entity.ai.goal.EatGrassGoal`

---

### TargetPredicate - 目标筛选谓词

**职责**: 用于筛选可以作为攻击目标的实体，允许自定义目标选择逻辑。

**类型定义** (`TargetGoals.hpp`):
```cpp
using TargetPredicate = std::function<bool(const LivingEntity*)>;
```

**使用场景**:
- `NearestAttackableTargetGoal` 的自定义目标筛选
- 限制攻击目标类型（如只攻击玩家）
- 添加额外的目标验证逻辑

**使用示例**:
```cpp
// 示例 1: 只攻击玩家
m_targetSelector.addGoal(1, std::make_unique<NearestAttackableTargetGoal<LivingEntity>>(
    this,
    true,    // checkSight
    0,       // chance
    [](const LivingEntity* entity) -> bool {
        if (!entity) return false;
        return entity->legacyType() == LegacyEntityType::Player;
    }
));

// 示例 2: 攻击非队友目标
m_targetSelector.addGoal(2, std::make_unique<NearestAttackableTargetGoal<LivingEntity>>(
    this,
    true,
    0,
    [this](const LivingEntity* entity) -> bool {
        if (!entity) return false;
        // 检查是否为队友
        return !isOnSameTeam(entity);
    }
));

// 示例 3: 攻击可见的敌对生物
m_targetSelector.addGoal(3, std::make_unique<NearestAttackableTargetGoal<MobEntity>>(
    this,
    true,
    10,  // 每 10 tick 检查一次
    [](const LivingEntity* entity) -> bool {
        if (!entity) return false;
        auto* mob = dynamic_cast<const MobEntity*>(entity);
        return mob != nullptr && mob->isHostile();
    }
));
```

**MC 1.16.5 参考**: `net.minecraft.entity.ai.goal.NearestAttackableTargetGoal` 的 `targetPredicate` 参数

---

## 文件关系图

```mermaid
graph TB
    subgraph "核心框架"
        Goal[Goal.hpp<br/>目标基类]
        GoalFlag[GoalFlag.hpp<br/>互斥标志]
        GoalConstants[GoalConstants.hpp<br/>常量定义]
        GoalSelector[GoalSelector.hpp<br/>目标选择器]
        PrioritizedGoal[PrioritizedGoal.hpp<br/>优先级包装器]
    end

    subgraph "具体目标"
        AvoidEntityGoal[AvoidEntityGoal<br/>避开实体]
        BreedGoal[BreedGoal<br/>繁殖]
        FollowParentGoal[FollowParentGoal<br/>跟随父母]
        LookAtGoal[LookAtGoal<br/>看向目标]
        MeleeAttackGoal[MeleeAttackGoal<br/>近战攻击]
        PanicGoal[PanicGoal<br/>恐慌逃跑]
        RandomWalkingGoal[RandomWalkingGoal<br/>随机漫步]
        SwimGoal[SwimGoal<br/>游泳]
        TemptGoal[TemptGoal<br/>食物诱惑]
    end

    Goal --> PrioritizedGoal
    PrioritizedGoal --> GoalSelector
    GoalFlag --> Goal
    GoalConstants --> AvoidEntityGoal
    GoalConstants --> BreedGoal
    GoalConstants --> MeleeAttackGoal
    GoalConstants --> RandomWalkingGoal
    GoalConstants --> TemptGoal

    Goal --> AvoidEntityGoal
    Goal --> BreedGoal
    Goal --> FollowParentGoal
    Goal --> LookAtGoal
    Goal --> MeleeAttackGoal
    Goal --> PanicGoal
    Goal --> RandomWalkingGoal
    Goal --> SwimGoal
    Goal --> TemptGoal
```

---

## 输入和输出

### 输入

| 输入来源 | 说明 |
|----------|------|
| 实体状态 | 实体的位置、生命值、状态效果等 |
| 世界环境 | 周围实体、方块、光照等 |
| 玩家交互 | 玩家手持物品、攻击等 |
| 控制器 | MoveController、LookController、JumpController |

### 输出

| 输出类型 | 说明 |
|----------|------|
| 移动指令 | 通过 `tryMoveTo()` 设置移动目标 |
| 视线控制 | 通过 `lookAt()` 设置看向目标 |
| 跳跃指令 | 通过 `JumpController` 触发跳跃 |
| 状态变更 | 繁殖、攻击伤害等 |

---

## 依赖项

### 外部依赖

```cpp
// 实体系统
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"

// 控制器
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/controller/JumpController.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"

// 工具
#include "common/util/math/random/Random.hpp"
#include "common/core/EnumSet.hpp"
```

### 被依赖

```cpp
// 动物实体
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/passive/basic/CowEntity.hpp"
// ...其他动物

// 敌对生物
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/monster/undead/SkeletonEntity.hpp"
// ...其他生物
```

---

## 使用方法

### 1. 创建自定义目标

```cpp
class MyCustomGoal : public mc::entity::ai::Goal {
public:
    MyCustomGoal(MobEntity* mob, f64 speed)
        : m_mob(mob)
        , m_speed(speed)
    {
        // 设置互斥标志
        setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
    }

    bool shouldExecute() override {
        if (!m_mob) return false;
        // 检查执行条件
        return m_mob->isHungry();
    }

    bool shouldContinueExecuting() override {
        // 检查是否继续
        return m_mob->isHungry() && m_targetFood != nullptr;
    }

    void startExecuting() override {
        // 初始化状态
        m_targetFood = findFood();
    }

    void resetTask() override {
        // 清理状态
        m_targetFood = nullptr;
        m_mob->clearNavigation();
    }

    void tick() override {
        // 每刻更新
        if (m_targetFood) {
            m_mob->tryMoveTo(
                m_targetFood->x(),
                m_targetFood->y(),
                m_targetFood->z(),
                m_speed
            );
        }
    }

    std::string getTypeName() const override {
        return "MyCustomGoal";
    }

private:
    MobEntity* m_mob;
    f64 m_speed;
    Entity* m_targetFood = nullptr;
};
```

### 2. 为实体添加目标

```cpp
void PigEntity::registerGoals() {
    // 优先级 0: 最高（游泳）
    m_goalSelector.addGoal(0, std::make_unique<SwimGoal>(this));

    // 优先级 1: 恐慌逃跑
    m_goalSelector.addGoal(1, std::make_unique<PanicGoal>(this, 1.5));

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, std::make_unique<BreedGoal>(this, 1.0));

    // 优先级 3: 食物诱惑
    m_goalSelector.addGoal(3, std::make_unique<TemptGoal>(
        this, 1.2, isFoodItem, false));

    // 优先级 4: 跟随父母
    m_goalSelector.addGoal(4, std::make_unique<FollowParentGoal>(this, 1.0));

    // 优先级 5: 随机漫步
    m_goalSelector.addGoal(5, std::make_unique<RandomWalkingGoal>(this, 1.0));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, std::make_unique<LookAtGoal>(this, 8.0f));
}
```

### 3. 在游戏循环中更新

```cpp
void MobEntity::tick() {
    // 更新 AI 目标
    m_goalSelector.tick();

    // 更新控制器
    if (m_moveController) m_moveController->tick();
    if (m_lookController) m_lookController->tick();
    if (m_jumpController) m_jumpController->tick();
}
```

---

## 容易踩的坑

### 1. 优先级数值混淆

**问题**: 以为数值越大优先级越高。

**正确**: 数值越小优先级越高。

```cpp
// 正确：高优先级用小数值
m_goalSelector.addGoal(0, swimGoal);      // 最高优先级
m_goalSelector.addGoal(10, wanderGoal);   // 低优先级
```

### 2. 忘记设置互斥标志

**问题**: 两个目标同时修改实体状态导致冲突。

**解决**: 始终设置正确的互斥标志。

```cpp
// 错误：没有设置互斥标志
class MyGoal : public Goal {
    MyGoal() { }  // 可能与其他移动目标冲突
};

// 正确：设置互斥标志
class MyGoal : public Goal {
    MyGoal() {
        setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
    }
};
```

### 3. `shouldContinueExecuting` 默认行为

**问题**: 依赖默认实现 `return shouldExecute()`，导致目标过早结束。

**解决**: 根据需要重写 `shouldContinueExecuting()`。

```cpp
// 可能有问题
bool shouldExecute() override {
    return m_target != nullptr;  // 检查目标存在
}
// 默认 shouldContinueExecuting() 也检查目标存在
// 但可能检查时机不对

// 更好的实现
bool shouldContinueExecuting() override {
    return m_target != nullptr &&
           m_target->isAlive() &&
           m_attackCooldown > 0;  // 额外条件
}
```

### 4. 空指针检查缺失

**问题**: 实体指针在目标执行期间可能失效。

**解决**: 在每个方法开始检查空指针。

```cpp
void tick() override {
    if (!m_mob) return;  // 防御性检查
    if (!m_target || !m_target->isAlive()) return;

    // 正常逻辑...
}
```

### 5. 目标状态未清理

**问题**: `resetTask()` 未清理所有状态。

**解决**: 确保 `resetTask()` 清理所有临时状态。

```cpp
void resetTask() override {
    m_target = nullptr;
    m_timer = 0;
    m_pathRecalculateCounter = 0;
    if (m_mob) {
        m_mob->clearNavigation();  // 清理导航
    }
}
```

### 6. 距离比较使用 sqrt

**问题**: 频繁调用 `sqrt()` 影响性能。

**解决**: 使用距离平方比较。

```cpp
// 低效
f32 distance = std::sqrt(dx * dx + dy * dy + dz * dz);
if (distance < maxDistance) { }

// 高效
f32 distSq = dx * dx + dy * dy + dz * dz;
if (distSq < maxDistance * maxDistance) { }

// 或使用预定义的平方常量
if (distSq < MAX_DISTANCE_SQ) { }
```

### 7. 目标间竞争条件

**问题**: 多个相同优先级的目标可能竞争执行。

**解决**: 使用不同的优先级或互斥标志。

---

## 涉及的测试用例

### GoalTests.cpp

| 测试名称 | 说明 |
|----------|------|
| `EnumSetTest.DefaultConstruction` | EnumSet 默认构造 |
| `EnumSetTest.InitializerListConstruction` | 初始化列表构造 |
| `EnumSetTest.SetAndReset` | 设置和重置标志 |
| `EnumSetTest.Operators` | 并集、交集、差集操作 |
| `EnumSetTest.Intersects` | 交集检查 |
| `EnumSetTest.ForEach` | 遍历操作 |
| `GoalTest.DefaultFlags` | Goal 默认标志为空 |
| `GoalTest.SetMutexFlags` | 设置互斥标志 |
| `GoalTest.ConstructWithFlags` | 构造时设置标志 |
| `GoalTest.DefaultPreemptible` | 默认可抢占 |
| `PrioritizedGoalTest.Priority` | 优先级获取 |
| `PrioritizedGoalTest.Preemption` | 抢占规则测试 |
| `PrioritizedGoalTest.NonPreemptible` | 不可抢占测试 |
| `PrioritizedGoalTest.RunningState` | 运行状态管理 |
| `PrioritizedGoalTest.DelegatesToInner` | 委托到内部目标 |
| `GoalSelectorTest.AddAndRemoveGoal` | 添加和移除目标 |
| `GoalSelectorTest.PriorityOrdering` | 优先级排序 |
| `GoalSelectorTest.MutexFlags` | 互斥标志测试 |
| `GoalSelectorTest.DisableFlags` | 禁用标志测试 |
| `GoalSelectorTest.TickRunningGoals` | tick 更新运行目标 |
| `GoalSelectorTest.StopWhenShouldNotContinue` | 停止条件测试 |
| `GoalSelectorTest.RemoveAllGoals` | 移除所有目标 |
| `GoalSelectorTest.ForEachRunningGoal` | 遍历运行目标 |
| `GoalFlagTest.AllFlags` | 所有标志测试 |
| `TargetPredicateTest.CanBeCreatedFromLambda` | TargetPredicate 从 lambda 创建 |
| `TargetPredicateTest.CanBeUsedForConditionChecking` | TargetPredicate 条件检查 |
| `TargetPredicateTest.CanBeEmpty` | TargetPredicate 空谓词 |
| `TargetPredicateTest.CanCaptureExternalState` | TargetPredicate 捕获外部状态 |

### CreeperSwellGoalTest.cpp

| 测试名称 | 说明 |
|----------|------|
| `CreeperSwellGoalBasicTest.GoalFlag_MoveFlagValue` | GoalFlag 枚举值验证 |
| `CreeperSwellGoalBasicTest.EnumSet_CanStoreGoalFlags` | EnumSet 存储 GoalFlag |
| `CreeperSwellGoalBasicTest.EnumSet_InitializerList` | EnumSet 初始化列表 |
| `CreeperSwellGoalBasicTest.SwellDistances_AreCorrect` | 膨胀距离常量验证 |
| `CreeperSwellGoalBasicTest.CreeperEntityConstants_AreCorrect` | 苦力怕实体常量验证 |
| `CreeperSwellGoalBasicTest.AllGoalFlags_ReturnsAllFlags` | allGoalFlags 函数测试 |
| `CreeperSwellGoalBasicTest.EnumSet_Operators` | EnumSet 运算符测试 |
| `CreeperSwellGoalBasicTest.EnumSet_Intersects` | EnumSet 交集测试 |

### BegGoalTest.cpp

| 测试名称 | 说明 |
|----------|------|
| `BegGoalTestFixture.Wolf_IsPlayerHoldingFood_UntamedWolf_BoneReturnsFalse` | 未驯服狼对骨头不乞求 |
| `BegGoalTestFixture.Wolf_IsPlayerHoldingFood_TamedWolf_BoneReturnsTrue` | 已驯服狼对骨头乞求 |
| `BegGoalTestFixture.Wolf_IsPlayerHoldingFood_AnyWolf_MeatReturnsTrue` | 所有狼对肉类乞求 |
| `BegGoalTestFixture.Cat_IsTameItem_CodAndSalmon` | 猫驯服物品测试 |
| `BegGoalTestFixture.Parrot_IsTameItem_Seeds` | 鹦鹉驯服物品测试 |
| `BegGoalTestFixture.TameableEntity_DefaultIsTameItem_ReturnsFalse` | 基类默认实现测试 |
| `BegGoalTestFixture.BegGoal_Construction_ValidParameters` | BegGoal 构造测试 |
| `BegGoalTestFixture.BegGoal_ShouldExecute_NoWorld_ReturnsFalse` | 无世界时返回 false |
| `BegGoalTestFixture.IsTameItem_Override_CompileTimeCheck` | override 关键字多态测试 |
| `BegGoalTestFixture.Wolf_TameItem_Vs_BreedingItem_Distinction` | 狼驯服/繁殖物品区分测试 |
| `BegGoalTestFixture.Cat_TameItem_And_BreedingItem_Same` | 猫驯服/繁殖物品相同测试 |
| `BegGoalTestFixture.Parrot_TameItem_Only_NoBreeding` | 鹦鹉只有驯服物品测试 |
| `CreeperSwellGoalBasicTest.EnumSet_ForEach` | EnumSet 遍历测试 |

### GuardianAttackGoalTest.cpp

| 测试名称 | 说明 |
|----------|------|
| `GuardianAttackGoalTest.AttackDistances_AreCorrect` | 攻击距离常量验证 (15格范围, 60tick充能, 20tick冷却) |
| `GuardianAttackGoalTest.TargetSelectionDistance_IsCorrect` | 目标选择距离验证 (>3格) |
| `GuardianAttackGoalTest.TargetTypes_PlayerAndSquidOnly` | 目标类型筛选 (只攻击玩家和鱿鱼) |
| `GuardianAttackGoalTest.GoalFlags_MoveAndLook` | GoalFlag 配置验证 (Move+Look) |
| `GuardianAttackGoalTest.DistanceCheck_GreaterThanThree` | 距离检查逻辑验证 |
| `GuardianAttackGoalTest.AttackRange_WithinFifteen` | 攻击范围验证 (15格) |
| `GuardianAttackGoalTest.Timing_SequenceCorrect` | 攻击时序验证 (充能60tick+冷却20tick) |
| `GuardianAttackGoalTest.EntityUtils_FindClosestEntity_Predicate` | EntityUtils 目标搜索谓词验证 |
| `GuardianAttackGoalTest.TargetSelector_ConfigurationCorrect` | 目标选择器配置验证 (checkInterval=10, checkSight=true) |
| `GuardianAttackGoalTest.ElderGuardian_DamageBonus` | 远古守卫者伤害加成验证 |
| `GuardianAttackGoalTest.GoalSelector_PrioritiesCorrect` | 行为目标优先级验证 |
| `GuardianAttackGoalTest.EnumSet_BasicOperations` | EnumSet 基本操作测试 |
| `GuardianAttackGoalTest.EnumSet_InitializerList` | EnumSet 初始化列表测试 |
| `GuardianAttackGoalTest.AllGoalFlags_ReturnsAllFlags` | allGoalFlags 函数测试 |

### RandomWalkingGoalTest.cpp

| 测试名称 | 说明 |
|----------|------|
| `RandomWalkingGoalTest.ShouldExecuteReturnsFalseWhenNullCreature` | 空实体检查 |
| `RandomWalkingGoalTest.ShouldExecuteReturnsTrueWhenConditionsMet` | 条件满足时执行 |
| `RandomWalkingGoalTest.ShouldExecuteReturnsFalseWhenIdleTimeTooHigh` | 空闲时间检查 |
| `RandomWalkingGoalTest.ShouldContinueExecutingReturnsFalseWhenNullCreature` | 空实体继续检查 |
| `RandomWalkingGoalTest.ShouldContinueExecutingReturnsTrueWhenActive` | 活动状态继续 |
| `RandomWalkingGoalTest.StartExecutingSetsTargetPosition` | 启动时设置目标位置 |
| `RandomWalkingGoalTest.ResetTaskClearsNavigation` | 重置清理导航 |
| `RandomWalkingGoalTest.TickDecrementsTimeoutCounter` | tick 递减超时计数 |
| `RandomWalkingGoalTest.MakeUpdateForcesNextExecution` | 强制下次执行 |
| `RandomWalkingGoalTest.SetExecutionChance` | 设置执行概率 |
| `CreatureEntityMoveTest.*` | CreatureEntity 移动测试 |
| `MovementControllerTest.*` | MovementController 测试 |
| `RandomWalkingGoalIntegrationTest.*` | 集成测试 |

---

## 类图

```mermaid
classDiagram
    class Goal {
        <<abstract>>
        -EnumSet~GoalFlag~ m_flags
        +shouldExecute() bool
        +shouldContinueExecuting() bool
        +isPreemptible() bool
        +startExecuting() void
        +resetTask() void
        +tick() void
        +setMutexFlags(flags) void
        +getMutexFlags() EnumSet~GoalFlag~
        +getTypeName() std::string
    }

    class PrioritizedGoal {
        -int m_priority
        -unique_ptr~Goal~ m_inner
        -bool m_running
        +isPreemptedBy(other) bool
        +getPriority() int
        +isRunning() bool
        +getGoal() Goal*
    }

    class GoalSelector {
        -vector~PrioritizedGoal~ m_goals
        -unordered_map~GoalFlag, PrioritizedGoal*~ m_flagGoals
        -EnumSet~GoalFlag~ m_disabledFlags
        -int m_tickRate
        +addGoal(priority, goal) void
        +removeGoal(goal) void
        +tick() void
        +disableFlag(flag) void
        +enableFlag(flag) void
    }

    class GoalFlag {
        <<enumeration>>
        Move
        Look
        Jump
        Target
        Count
    }

    Goal <|-- PrioritizedGoal
    PrioritizedGoal --> GoalSelector : managed by
    GoalSelector --> GoalFlag : uses

    Goal <|-- AvoidEntityGoal
    Goal <|-- BreedGoal
    Goal <|-- FollowParentGoal
    Goal <|-- LookAtGoal
    Goal <|-- MeleeAttackGoal
    Goal <|-- PanicGoal
    Goal <|-- RandomWalkingGoal
    Goal <|-- SwimGoal
    Goal <|-- TemptGoal
    Goal <|-- LookRandomlyGoal
```

---

## 执行流程时序图

```mermaid
sequenceDiagram
    participant GS as GoalSelector
    participant PG as PrioritizedGoal
    participant G as Goal
    participant E as Entity

    loop Every Tick
        GS->>GS: 清理已停止的目标

        loop 所有目标
            GS->>PG: isRunning()?
            alt 目标运行中
                GS->>PG: shouldContinueExecuting()
                alt 不应继续
                    GS->>G: resetTask()
                    GS->>GS: clearFlagGoals(flags)
                end
            end
        end

        loop 所有目标(按优先级)
            GS->>PG: isRunning()?
            alt 目标未运行
                GS->>GS: canStartGoal()
                GS->>G: shouldExecute()
                alt 应该执行
                    GS->>G: startExecuting()
                    GS->>GS: updateFlagGoals(flags)
                end
            end
        end

        loop 运行中的目标
            GS->>G: tick()
            G->>E: 更新实体状态
        end
    end
```

---

## 参考资料

- Minecraft Java 1.16.5 `net.minecraft.entity.ai.goal` 包
- 本项目 CLAUDE.md 文档
