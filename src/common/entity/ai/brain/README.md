# Brain AI 系统

Brain系统是Minecraft 1.16.5引入的高级AI控制框架，用于更复杂的行为管理（如村民、猪灵等）。

## 目录结构

```
brain/
├── Brain.hpp                    # Brain主类 - 高级AI控制器
├── memory/                      # 记忆模块
│   ├── Memory.hpp               # 内存存储容器(带TTL)
│   ├── MemoryModuleStatus.hpp   # 内存状态枚举
│   ├── MemoryModuleType.hpp     # 内存类型定义 (85+种)
│   ├── MemoryModuleType.cpp     # 内存类型注册
│   ├── IPositionTarget.hpp      # 位置目标接口
│   ├── WalkTarget.hpp           # 行走目标
│   └── BlockPosTarget.hpp       # 方块位置目标
├── schedule/                    # 日程系统
│   ├── Activity.hpp             # 活动类型定义 (15种)
│   ├── Activity.cpp             # 活动类型实现
│   ├── Schedule.hpp             # 日程安排
│   ├── Schedule.cpp             # 日程实现 (4种预定义日程)
│   └── DutyTime.hpp             # 值班时间定义
├── sensor/                      # 传感器系统
│   ├── Sensor.hpp               # 传感器基类
│   ├── SensorType.hpp           # 传感器类型
│   ├── Sensors.hpp              # 传感器声明 (8种)
│   └── Sensors.cpp              # 传感器实现
├── task/                        # 任务系统
│   ├── Task.hpp                 # 任务基类
│   └── tasks/                   # 具体任务实现
│       ├── movement/            # 移动相关任务
│       ├── action/              # 行动相关任务
│       └── interact/            # 互动相关任务
└── README.md
```

## 实现状态

### ✅ 已完成
| 组件 | 状态 | 说明 |
|------|------|------|
| Brain | ✅ | 模板类，支持记忆、传感器、任务、日程 |
| Memory | ✅ | 支持永久记忆和带TTL的临时记忆 |
| MemoryModuleType | ✅ | 85+种记忆类型 |
| Activity | ✅ | 15种活动类型 |
| Schedule | ✅ | 4种预定义日程 |
| Sensor | ✅ | 8种传感器，已完整实现 |
| Task | ✅ | 基类完成 |
| Task实现 | ⚠️ | 20种任务，框架完成，TODO需填充 |

### ✅ 传感器实现状态
| 传感器 | 状态 | 说明 |
|--------|------|------|
| NearestPlayersSensor | ✅ | 检测附近玩家，设置 NEAREST_PLAYERS、NEAREST_VISIBLE_PLAYER、NEAREST_VISIBLE_TARGETABLE_PLAYER 记忆 |
| NearestVisibleLivingEntitySensor | ✅ | 检测可见生物，设置 VISIBLE_MOBS 记忆 |
| HurtBySensor | ✅ | 检测伤害来源，设置 HURT_BY、HURT_BY_ENTITY 记忆 |
| MobSensor | ✅ | 检测附近生物和敌对目标，设置 MOBS、NEAREST_HOSTILE 记忆 |
| WorkStationSensor | ✅ | 检测工作站点，设置 JOB_SITE、POTENTIAL_JOB_SITE 记忆 |
| VillagePoiSensor | ✅ | 检测床和集会点，设置 HOME、MEETING_POINT、NEAREST_BED 记忆 |
| BabySensor | ✅ | 检测幼年和成年实体，设置 VISIBLE_VILLAGER_BABIES、NEAREST_VISIBLE_ADULT 记忆 |
| AvoidEntitySensor | ✅ | 检测避险目标，设置 AVOID_TARGET 记忆 |

### ⚠️ 待完善
| 问题 | 说明 |
|------|------|
| Task实现 | 所有任务的update()方法只有TODO注释 |
| 实体集成 | PiglinEntity未使用Brain |

## 核心组件

### 1. Memory (记忆模块)

存储实体的"记忆"，支持TTL(生存时间)：

```cpp
// 创建永久记忆
auto memory = Memory<Player*>::permanent(player);

// 创建临时记忆(100 ticks后过期)
auto tempMemory = Memory<BlockPos>::timed(pos, 100);

// 每tick调用
memory.tick();

// 检查是否过期
if (memory.isExpired()) {
    // 记忆已过期
}
```

### 2. MemoryModuleType (内存类型)

定义不同类型的内存：

```cpp
// 常用内存类型
MemoryModuleTypes::ATTACK_TARGET   // 攻击目标
MemoryModuleTypes::HOME            // 家的位置
MemoryModuleTypes::WALK_TARGET     // 行走目标
MemoryModuleTypes::NEAREST_HOSTILE // 最近敌对生物
```

### 3. Activity (活动)

定义实体的行为状态：

```cpp
Activity::IDLE      // 空闲
Activity::WORK      // 工作
Activity::PLAY      // 玩耍
Activity::REST      // 休息
Activity::PANIC     // 恐慌
Activity::FIGHT     // 战斗
Activity::AVOID     // 逃避
```

### 4. Schedule (日程)

基于时间的活动安排：

```cpp
// 创建村民日程
Schedule schedule;
schedule.add(0, Activity::IDLE)
        .add(2000, Activity::WORK)
        .add(9000, Activity::MEET)
        .add(12000, Activity::REST);

// 获取当前时间的活动
Activity current = schedule.getScheduledActivity(dayTime);
```

### 5. Sensor (传感器)

自动感知环境并更新记忆：

```cpp
class NearestPlayersSensor : public Sensor<VillagerEntity> {
public:
    void update(ServerWorld* world, VillagerEntity* entity) override {
        // 感知附近玩家并更新记忆
        auto players = findNearbyPlayers(world, entity);
        entity->getBrain().setMemory(MemoryModuleTypes::NEAREST_PLAYERS, players);
    }

    std::unordered_set<const MemoryModuleTypeBase*> getUsedMemories() const override {
        return { MemoryModuleTypes::NEAREST_PLAYERS };
    }
};
```

### 6. Task (任务)

Brain任务类似于Goal，但使用记忆系统：

```cpp
class MoveToTargetTask : public Task<VillagerEntity> {
public:
    MoveToTargetTask()
        : Task({
            {MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_PRESENT}
        }, 150, 200) {}

protected:
    bool shouldExecute(ServerWorld* world, VillagerEntity* entity) override {
        auto target = entity->getBrain().getMemory(MemoryModuleTypes::WALK_TARGET);
        return target.has_value();
    }

    void updateTask(ServerWorld* world, VillagerEntity* entity, i64 gameTime) override {
        // 移动逻辑
    }
};
```

## 使用示例

### 创建Brain

```cpp
class VillagerEntity : public CreatureEntity {
public:
    VillagerEntity() {
        // 注册内存模块
        m_brain.registerMemory(MemoryModuleTypes::HOME);
        m_brain.registerMemory(MemoryModuleTypes::JOB_SITE);
        m_brain.registerMemory(MemoryModuleTypes::NEAREST_PLAYERS);

        // 注册传感器
        m_brain.registerSensor(std::make_unique<NearestPlayersSensor>());

        // 设置日程
        m_brain.setSchedule(Schedule::VILLAGER_DEFAULT);

        // 注册活动
        m_brain.registerActivity(
            Activity::WORK, 1,
            {std::make_unique<WorkTask>()},
            {{MemoryModuleTypes::JOB_SITE, MemoryModuleStatus::VALUE_PRESENT}}
        );
    }

    void tick() override {
        CreatureEntity::tick();
        m_brain.tick(world, this, gameTime, dayTime);
    }

private:
    Brain<VillagerEntity> m_brain;
};
```

## 与Goal系统的区别

| 特性 | Goal系统 | Brain系统 |
|------|----------|-----------|
| 复杂度 | 简单 | 复杂 |
| 记忆 | 无 | 有(TTL支持) |
| 传感器 | 无 | 有(自动感知) |
| 日程 | 无 | 有(时间活动) |
| 适用实体 | 大多数生物 | 村民、猪灵等 |

## 依赖关系

```
Brain.hpp
    ├── memory/Memory.hpp
    ├── memory/MemoryModuleType.hpp
    ├── schedule/Activity.hpp
    ├── schedule/Schedule.hpp
    ├── sensor/Sensor.hpp
    └── task/Task.hpp
```

## 参考

- Minecraft 1.16.5 `net.minecraft.entity.ai.brain`
 
## 近期补全

- 已新增 `schedule/DutyTime.hpp`，并将 `Schedule` / `ScheduleDuties` 修正为 MC 1.16.5 的离散 duty 时间片语义，不再使用错误的连续插值语义。
- 已新增 `memory/IPositionTarget.hpp`、`memory/BlockPosTarget.hpp`、`memory/WalkTarget.hpp`，用于承接 `LOOK_TARGET` / `WALK_TARGET` 的真实位置目标类型。
- `MemoryModuleTypes::WALK_TARGET` 现为 `WalkTarget`，`MemoryModuleTypes::LOOK_TARGET` 现为 `std::shared_ptr<IPositionTarget>`，不再是 `void` 占位。
- 回归测试位于 `tests/entity/BrainScheduleTests.cpp`，覆盖 schedule 边界、target 中心点和 typed memory 存取。
