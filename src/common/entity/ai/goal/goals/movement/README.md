# 移动类目标 (Movement Goals)

## 目录结构

```
movement/
├── MovementGoals.hpp/cpp       # 水避让随机漫步、跳跃攻击目标
├── FollowSchoolLeaderGoal.hpp/cpp  # 跟随群体领导者目标（群游鱼类）
└── README.md                   # 本文档
```

## 文件详细介绍

### MovementGoals.hpp/cpp

**职责**: 提供水避让随机漫步和跳跃攻击目标。

#### WaterAvoidingRandomWalkingGoal

使生物随机漫步，但会避开水和岩浆。

**关键参数**:
- `m_speed`: 移动速度
- `m_probability`: 执行概率

**行为**:
1. 检测当前位置是否在水中或岩浆中
2. 如果前方是水/岩浆，选择另一个方向
3. 否则正常随机漫步

#### LeapAtTargetGoal

使生物向目标跳跃攻击。

**关键参数**:
- `m_leapHeight`: 跳跃高度

**行为**:
1. 检测目标距离
2. 如果距离合适，向目标方向跳跃

#### MoveTowardsTargetGoal

使生物向攻击目标移动。

**MC 1.16.5 参考**: `net.minecraft.entity.ai.goal.MoveTowardsTargetGoal`

**关键参数**:
- `m_creature`: 生物实体
- `m_speed`: 移动速度
- `m_maxTargetDistance`: 最大目标距离

**执行条件**:
- `shouldExecute()`: 有攻击目标 且 目标存活 且 目标在最大距离内
- `shouldContinueExecuting()`: 目标存活 且 未超过最大距离

**行为**:
1. `startExecuting()`: 使用 `RandomPositionGenerator` 生成向目标移动的位置
2. `tick()`: 持续向目标位置移动

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| 无 | - | 使用传入参数 |

**互斥标志**: `Move`

**使用示例**:
```cpp
void IronGolemEntity::registerGoals() {
    // 优先级 2: 向目标移动（MC 1.16.5: 速度 0.9, 最大距离 32）
    m_goalSelector.addGoal(2, std::make_unique<MoveTowardsTargetGoal>(this, 0.9, 32.0f));
}
```

**依赖**:
- 需要 `RandomPositionGenerator::findRandomTargetTowards()` 生成移动位置
- 需要 `CreatureEntity` 提供导航和路径追踪能力

#### MoveTowardsRestrictionGoal

使生物向家范围移动。当生物离开其家范围限制时，向家位置移动。

**MC 1.16.5 参考**: `net.minecraft.entity.ai.goal.MoveTowardsRestrictionGoal`

**关键参数**:
- `m_creature`: 生物实体
- `m_speed`: 移动速度

**执行条件**:
- `shouldExecute()`: 当前位置不在家范围内（`!isWithinHomeDistanceCurrentPosition()`）
- `shouldContinueExecuting()`: 导航器还有路径

**行为**:
1. `shouldExecute()`: 检查是否在家范围内，如果不在则生成向家位置的随机目标
2. `startExecuting()`: 开始向目标位置移动
3. `tick()`: 持续移动直到到达目标或路径结束

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| `XZ_RANGE` | 16 | 水平搜索范围（格） |
| `Y_RANGE` | 7 | 垂直搜索范围（格） |

**互斥标志**: `Move`

**使用示例**:
```cpp
void GuardianEntity::registerGoals() {
    // 优先级 5: 向限制区域移动（MC 1.16.5: 速度 1.0）
    // 守卫者有移动限制区域（海底神殿附近）
    m_goalSelector.addGoal(5, std::make_unique<MoveTowardsRestrictionGoal>(this, 1.0));
}
```

**家范围系统**:
- `MobEntity::setHomePosAndDistance(pos, distance)`: 设置家位置和范围
- `MobEntity::homePosition()`: 获取家位置
- `MobEntity::maximumHomeDistance()`: 获取家范围半径
- `MobEntity::isWithinHomeDistanceCurrentPosition()`: 检查当前位置是否在家范围内
- `MobEntity::hasHome()`: 检查是否设置了家范围
- `MobEntity::clearHome()`: 清除家范围限制

**依赖**:
- 需要 `RandomPositionGenerator::findRandomTargetTowards()` 生成移动位置
- 需要 `CreatureEntity` 提供家范围系统和导航能力

---

### FollowSchoolLeaderGoal.hpp/cpp

**职责**: 使群游鱼类（鳕鱼、鲑鱼、热带鱼）跟随群体领导者。

**MC 1.16.5 参考**: `net.minecraft.entity.ai.goal.FollowSchoolLeaderGoal`

#### 关键常量

| 常量 | 值 | 说明 |
|------|-----|------|
| `SEARCH_RANGE` | 8.0f | 搜索范围（格） |
| `NAVIGATE_TIMER_INTERVAL` | 10 | 导航重算间隔（ticks） |

#### 成员变量

| 变量 | 类型 | 说明 |
|------|------|------|
| `m_fish` | `AbstractGroupFishEntity*` | 群游鱼类实体 |
| `m_leader` | `AbstractGroupFishEntity*` | 群首引用 |
| `m_navigateTimer` | `i32` | 导航计时器 |
| `m_cooldown` | `i32` | 搜索冷却时间 |

#### 核心方法

##### `shouldExecute()`

执行条件：
1. 如果自己是首领 → 不执行
2. 如果已有首领 → 继续执行
3. 冷却中 → 不执行
4. 冷却结束 → 搜索附近鱼群：
   - 找到可扩群的首领或自己成为首领
   - 招募无首领的鱼加入群体
   - 自己加入首领的群体

搜索算法（MC 1.16.5）：
```cpp
auto nearbyFish = EntityUtils::findEntities<AbstractGroupFishEntity>(
    world, m_fish->position(), SEARCH_RANGE, m_fish,
    [](AbstractGroupFishEntity* fish) {
        return fish->canGroupGrow() || !fish->hasGroupLeader();
    }
);
```

##### `shouldContinueExecuting()`

继续条件：
- 有群首 且 在跟随范围内（默认 11 格）

##### `tick()`

每帧更新：
1. 看向群首
2. 每 10 ticks 导航到群首位置

##### `resetTask()`

重置时离开群体。

#### 冷却机制

MC 1.16.5: `200 + random.nextInt(200) % 20`
结果范围：200~219 ticks（约 10~11 秒）

---

## 使用示例

### 为群游鱼类注册目标

```cpp
// CodEntity.cpp
void CodEntity::registerGoals() {
    AbstractFishEntity::registerGoals();
    
    // 群游目标 - 优先级 3
    m_goalSelector.addGoal(3, std::make_unique<FollowSchoolLeaderGoal>(this));
}
```

### 群游鱼群体行为

```cpp
// 检查是否是群首
if (cod.isGroupLeader()) {
    // 群首逻辑
}

// 检查是否跟随群首
if (cod.hasGroupLeader()) {
    auto* leader = cod.getGroupLeader();
    // 跟随逻辑
}

// 加入群体
follower.joinGroup(leader);

// 离开群体
follower.leaveGroup();
```

---

## 依赖关系

```
FollowSchoolLeaderGoal
    ├── AbstractGroupFishEntity  # 群游鱼类实体
    │   ├── AbstractFishEntity   # 鱼类基类
    │   │   └── WaterMobEntity   # 水生生物基类
    │   └── ...
    ├── EntityUtils::findEntities<T>()  # 实体查询
    ├── PathNavigator            # 路径导航
    ├── LookController           # 视线控制
    └── IWorld::getRandom()      # 随机数生成器
```

---

## 测试用例

测试文件位于 `tests/entity/FishSupportTypesTest.cpp`：

| 测试名称 | 说明 |
|----------|------|
| `ShouldNotExecuteWhenIsGroupLeader` | 首领不执行跟随 |
| `ShouldExecuteWhenHasGroupLeader` | 有首领时执行 |
| `ShouldContinueExecutingWhenInRange` | 范围内继续跟随 |
| `ShouldNotContinueExecutingWhenOutOfRange` | 超出范围停止 |
| `ShouldLeaveGroupOnReset` | 重置时离开群体 |
| `ShouldFindGroupToJoin` | 搜索并加入群体 |
| `ShouldRespectMaxGroupSize` | 遵守最大群体限制 |
| `RecruitFollowersWorks` | 招募功能正常 |
| `MoveToGroupLeaderWorks` | 导航功能正常 |

---

## 容易踩的坑

### 1. 冷却机制

**问题**: 每帧都搜索会导致性能问题。

**解决**: 使用 200-219 ticks 的冷却时间。

### 2. 首领选举逻辑

**问题**: 错误地将所有鱼都设为跟随者。

**解决**: 第一个找到的可扩群首领或自己成为首领。

### 3. 群体大小限制

**问题**: 无限加入导致群体过大。

**解决**: 使用 `canGroupGrow()` 检查是否还能扩群。

### 4. 实体查询类型

**问题**: 使用 `EntityUtils::findEntities<Entity>()` 会返回所有实体。

**解决**: 使用 `EntityUtils::findEntities<AbstractGroupFishEntity>()` 只返回群游鱼类。

---

## 参考资料

- MC 1.16.5 `net.minecraft.entity.ai.goal.FollowSchoolLeaderGoal`
- MC 1.16.5 `net.minecraft.entity.passive.fish.AbstractGroupFishEntity`
