# Mob 模块

本模块提供 Minecraft 中 AI 生物实体的基础类层次结构。

## 目录结构

```
src/common/entity/mob/
├── MobEntity.hpp         # AI生物实体基类
├── MobEntity.cpp
├── CreatureEntity.hpp    # 可移动生物实体基类
├── CreatureEntity.cpp
├── AgeableEntity.hpp     # 可成长实体基类
└── AgeableEntity.cpp
```

## 类层次结构

```
LivingEntity                    (生物实体基类，提供生命值、属性、装备等)
    │
    └── MobEntity               (AI生物基类，提供AI目标系统、控制器、寻路)
            │
            └── CreatureEntity  (可移动生物，提供移动能力)
                    │
                    └── AgeableEntity (可成长实体，提供年龄/繁殖系统)
                            │
                            └── AnimalEntity  (动物实体，在animal目录)
```

## 文件详解

### MobEntity.hpp / MobEntity.cpp

**职责**: 所有 AI 生物的基类，包括怪物和动物。

**核心功能**:

1. **AI 目标系统**
   - `m_goalSelector`: 行为目标选择器（如漫步、看向玩家等）
   - `m_targetSelector`: 目标选择器（如攻击目标选择）
   - `registerGoals()`: 虚方法，子类重写以注册特定 AI 目标

2. **控制器系统**
   - `m_lookController`: 视线控制器，控制实体看向目标
   - `m_moveController`: 移动控制器，控制实体的移动方向和速度
   - `m_jumpController`: 跳跃控制器，控制跳跃时机

3. **寻路系统**
   - `m_navigator`: 路径导航器，提供 A* 寻路能力

4. **攻击目标**
   - `m_attackTarget`: 当前攻击目标指针

5. **AI 状态**
   - `m_aiEnabled`: AI 是否启用
   - `m_idleTime`: 空闲时间计数器（用于随机漫步等）

**关键方法**:

```cpp
// AI 目标管理
GoalSelector& goalSelector();
GoalSelector& targetSelector();
virtual void registerGoals();

// 控制器访问
LookController* lookController();
MovementController* moveController();
JumpController* jumpController();

// 寻路
PathNavigator* navigator();
void clearNavigation();

// 视线控制
void lookAt(const Entity& target, f32 deltaYaw = 10.0f, f32 deltaPitch = 10.0f);
void lookAt(f64 x, f64 y, f64 z, f32 deltaYaw = 10.0f, f32 deltaPitch = 10.0f);

// 工具方法
math::Random getRandom() const;
bool isBeingRidden() const;
i32 idleTime() const;
```

**tick() 流程**:
1. 调用 `LivingEntity::tick()`
2. 更新 `m_idleTime`（移动时重置，静止时递增）
3. 如果 AI 启用，执行 `m_goalSelector.tick()` 和 `m_targetSelector.tick()`
4. 更新控制器：`lookController->tick()`, `moveController->tick()`, `jumpController->tick()`
5. 调用 `aiStep()` 执行 AI 移动物理更新

---

### CreatureEntity.hpp / CreatureEntity.cpp

**职责**: 可移动的生物实体基类，提供基础的移动能力。

**核心功能**:

1. **移动控制**
   - `m_moveSpeed`: 移动速度倍率
   - `tryMoveTo()`: 尝试移动到目标位置

2. **生成条件**
   - `getPathWeight()`: 获取路径权重（用于生成条件判断）
   - `canSpawnAt()`: 检查是否可以在该位置生成

**关键方法**:

```cpp
// 移动
bool tryMoveTo(f64 x, f64 y, f64 z, f64 speed);
f64 moveSpeed() const;
void setMoveSpeed(f64 speed);

// 生成条件
virtual f32 getPathWeight(f32 x, f32 y, f32 z) const;
virtual bool canSpawnAt(f32 x, f32 y, f32 z) const;
```

**移动逻辑**:
1. 优先尝试使用 `navigator->moveTo()` 进行寻路移动
2. 如果导航失败或不可用，直接使用 `moveController->setMoveTo()` 简单移动

---

### AgeableEntity.hpp / AgeableEntity.cpp

**职责**: 支持幼体/成体状态的实体基类，实现年龄系统和繁殖功能。

**核心功能**:

1. **年龄系统**
   - `m_growingAge`: 年龄值（负数 = 幼体，0 或正数 = 成体）
   - `BABY_AGE = -24000`: 幼体起始年龄（-24000 tick = 20 分钟）
   - 成长速度倍率 `m_growthSpeed`

2. **繁殖系统**
   - `m_loveTimer`: 爱心状态计时器
   - `m_loveCause`: 使其进入爱心状态的玩家 ID
   - `LOVE_TIMER_MAX = 600`: 爱心状态持续时间（30 秒）

3. **强制成长**
   - `m_forcedAge`: 强制成长值
   - `m_forcedAgeTimer`: 强制成长计时器

**关键方法**:

```cpp
// 年龄管理
i32 getGrowingAge() const;
void setGrowingAge(i32 age);
bool isChild() const;              // m_growingAge < 0
void setChild(bool child);
void ageUp(i32 seconds);           // 加速成长
void addGrowingAge(i32 amount);

// 繁殖状态
i32 getLoveTimer() const;
bool canBreed() const;             // 成体且不在爱心状态
bool isInLove() const;             // m_loveTimer > 0
void setInLove(u64 playerInLove = 0);
void resetLove();

// 成长速度
f32 getGrowthSpeed() const;
void setGrowthSpeed(f32 speed);

// 回调
virtual void onGrowUp();           // 幼体变成成体时调用
```

**tick() 流程**:
1. 调用 `CreatureEntity::tick()`
2. 调用 `updateAge()`: 更新年龄（幼体成长 / 成体繁殖冷却）
3. 调用 `updateLove()`: 更新爱心计时器

**年龄更新逻辑**:
```cpp
void AgeableEntity::updateAge() {
    if (isChild()) {
        // 幼体成长
        i32 growth = static_cast<i32>(m_growthSpeed);
        if (m_forcedAgeTimer > 0) {
            --m_forcedAgeTimer;
            growth += m_forcedAge / LOVE_TIMER_MAX;
        }
        m_growingAge += growth;
        if (m_growingAge >= 0) {
            m_growingAge = 0;
            onGrowUp();  // 触发成长回调
        }
    } else {
        // 成体繁殖冷却
        if (m_growingAge > 0) {
            --m_growingAge;
        }
    }
}
```

---

## 模块整体分析

### 整体职责

本模块定义了 AI 生物实体的类层次结构，负责：

1. **AI 行为管理**: 目标选择、优先级、互斥标志
2. **运动控制**: 视线、移动、跳跃控制器
3. **寻路导航**: A* 寻路算法
4. **生命周期**: 年龄成长、繁殖系统
5. **目标跟踪**: 攻击目标管理

### 输入

| 输入项 | 来源 | 说明 |
|--------|------|------|
| 世界状态 | World/ServerWorld | 方块、实体、生物群系等 |
| 玩家输入 | PlayerEntity | 交互、喂食 |
| 时间流逝 | tick() 调用 | 年龄成长、AI 更新 |
| 属性值 | AttributeSystem | 生命值、移动速度等 |

### 输出

| 输出项 | 目标 | 说明 |
|--------|------|------|
| 移动指令 | MovementController | 移动方向和速度 |
| 视线方向 | LookController | 头部朝向 |
| 跳跃指令 | JumpController | 跳跃时机 |
| 繁殖结果 | World | 生成幼体实体 |
| 状态同步 | Network | 实体数据包 |

### 依赖项

```
MobEntity
├── LivingEntity              (父类)
├── entity::ai::GoalSelector  (AI 目标系统)
├── entity::ai::controller::
│   ├── LookController        (视线控制)
│   ├── MovementController    (移动控制)
│   └── JumpController        (跳跃控制)
├── entity::ai::pathfinding::
│   └── PathNavigator         (寻路导航)
├── math::Random              (随机数生成)
└── core::Types               (基础类型)

CreatureEntity
├── MobEntity                 (父类)
└── PathNavigator, MovementController

AgeableEntity
├── CreatureEntity            (父类)
└── ItemStack                 (繁殖物品判断)
```

### 使用方法

#### 1. 创建自定义动物

```cpp
class MyAnimal : public AgeableEntity {
public:
    MyAnimal(LegacyEntityType type, EntityId id)
        : AgeableEntity(type, id)
    {
        // 设置移动速度
        setMoveSpeed(0.25);
        // 注册 AI 目标
        registerGoals();
    }

    void registerGoals() override {
        AgeableEntity::registerGoals();  // 调用父类

        // 添加自定义目标
        goalSelector().addGoal(5, new MyCustomGoal(this));
    }

    bool isBreedingItem(const ItemStack& stack) const override {
        // 定义繁殖物品
        return stack.item()->getId() == ItemId::WHEAT;
    }

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override {
        return std::make_unique<MyAnimal>(m_type, generateNewId());
    }

protected:
    void onGrowUp() override {
        // 幼体成长为成体时的处理
        // 例如：更新尺寸、属性等
    }
};
```

#### 2. 控制实体行为

```cpp
// 禁用/启用 AI
mobEntity.setAiEnabled(false);

// 设置攻击目标
mobEntity.setAttackTarget(targetEntity);

// 让实体看向某位置
mobEntity.lookAt(x, y, z);

// 清除导航路径
mobEntity.clearNavigation();

// 让生物移动
creatureEntity.tryMoveTo(targetX, targetY, targetZ, 1.0);

// 年龄控制
ageableEntity.setChild(true);        // 设置为幼体
ageableEntity.ageUp(60);             // 加速成长 60 秒
ageableEntity.setInLove(playerId);   // 进入繁殖状态
```

### 容易踩的坑

1. **AI 目标互斥标志**
   - 问题：多个 AI 目标使用相同的互斥标志（如 `GoalFlag::Move`）时会冲突
   - 解决：合理设置 `setMutexFlags()`，确保不冲突的目标可以并行执行

   ```cpp
   // 错误：两个目标都用 Move 标志，会互相冲突
   goalSelector().addGoal(5, new WanderGoal(this));  // 默认 Move 标志
   goalSelector().addGoal(5, new FollowGoal(this));  // 默认 Move 标志

   // 正确：使用不同优先级或调整标志
   goalSelector().addGoal(5, new WanderGoal(this));
   goalSelector().addGoal(3, new FollowGoal(this));  // 高优先级
   ```

2. **控制器更新顺序**
   - 问题：控制器更新顺序错误会导致移动异常
   - 解决：必须按照 `look -> move -> jump -> aiStep` 的顺序更新

3. **年龄值理解错误**
   - 问题：年龄值是负数表示幼体，正数表示成体繁殖冷却
   - 解决：使用 `isChild()` 判断是否为幼体，不要直接比较 `getGrowingAge()`

   ```cpp
   // 错误
   if (entity.getGrowingAge() < 0) { ... }

   // 正确
   if (entity.isChild()) { ... }
   ```

4. **繁殖冷却时间**
   - 问题：成体的 `getGrowingAge() > 0` 表示繁殖冷却中
   - 解决：使用 `canBreed()` 判断是否可繁殖

5. **指针生命周期**
   - 问题：`m_attackTarget` 是原始指针，可能悬空
   - 解决：使用前检查实体是否仍然存活（TODO: 改为弱引用）

6. **随机数种子**
   - 问题：`getRandom()` 基于实体 ID 和 tick 生成种子，同一 tick 多次调用结果相同
   - 解决：如果需要不同随机值，缓存 Random 对象或使用额外种子

7. **AI 启用状态**
   - 问题：`m_aiEnabled = false` 时 AI 目标不会执行，但控制器仍会更新
   - 解决：完全禁用行为时需要额外处理

### 涉及的测试用例

本模块没有直接的单元测试文件，但相关测试覆盖：

| 测试文件 | 测试内容 |
|----------|----------|
| `tests/entity/GoalTests.cpp` | GoalSelector, PrioritizedGoal, GoalFlag |
| `tests/entity/LivingEntityTests.cpp` | LivingEntity 基类测试 |
| `tests/entity/PathfindingTests.cpp` | PathNavigator, NodeProcessor |
| `tests/entity/RandomWalkingGoalTest.cpp` | RandomWalkingGoal 行为测试 |

**建议添加的测试**:
- `MobEntityTests.cpp`: 控制器初始化、AI 目标注册、tick 流程
- `CreatureEntityTests.cpp`: tryMoveTo 行为、生成条件
- `AgeableEntityTests.cpp`: 年龄成长、繁殖状态、onGrowUp 回调

---

## 参考

本模块参考 Minecraft 1.16.5 的实体系统设计：

- `MobEntity`: net.minecraft.entity.MobEntity
- `CreatureEntity`: net.minecraft.entity.passive.CreatureEntity
- `AgeableEntity`: net.minecraft.entity.AgeableEntity
