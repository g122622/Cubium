# 特殊 AI 目标 (Special Goals)

## 目录结构

```
special/
├── SpecialGoals.hpp       # 特殊目标头文件
├── SpecialGoals.cpp       # 特殊目标实现
├── GuardianAttackGoal.hpp # 守卫者攻击目标头文件
├── GuardianAttackGoal.cpp # 守卫者攻击目标实现
├── BlazeFireballAttackGoal.hpp # 烈焰人火球攻击目标头文件
├── BlazeFireballAttackGoal.cpp # 烈焰人火球攻击目标实现
├── SquidGoals.hpp         # 鱿鱼目标头文件
├── SquidGoals.cpp         # 鱿鱼目标实现
├── BatGoals.hpp           # 蝙蝠目标头文件
├── BatGoals.cpp           # 蝙蝠目标实现
├── DolphinGoals.hpp       # 海豚目标头文件
├── DolphinGoals.cpp       # 海豚目标实现
├── PhantomGoals.hpp       # 幻翼目标头文件
├── PhantomGoals.cpp       # 幻翼目标实现
├── SlimeGoals.hpp         # 史莱姆目标头文件
├── SlimeGoals.cpp         # 史莱姆目标实现
├── IronGolemGoals.hpp     # 铁傀儡目标头文件
├── IronGolemGoals.cpp     # 铁傀儡目标实现
└── README.md              # 本文档
```

## 整体职责

本目录包含特定实体专用的 AI 目标，这些目标不适用于通用场景，而是为特定实体类型定制的行为。

## 文件详细介绍

### BlazeFireballAttackGoal - 烈焰人火球攻击目标

**职责**: 控制烈焰人使用小火球攻击目标。

**MC 1.16.5 参考**: `net.minecraft.entity.monster.BlazeEntity.FireballAttackGoal`

**攻击阶段**:
1. **充能阶段**: 60 ticks (3秒)，烈焰人进入燃烧状态
2. **火球阶段**: 连发最多 3 个小火球，每个间隔 6 ticks (0.3秒)
3. **冷却阶段**: 100 ticks (5秒)

**执行条件**:
- 有攻击目标
- 目标存活
- 目标在追踪范围内

**特点**:
- 使用加速度驱动的小火球（SmallFireballEntity）
- 散布计算：`spread = sqrt(sqrt(distSq)) * 0.5`
- 近战范围（< 2 格）使用物理攻击
- 视线检测控制追踪行为

**互斥标志**: `Move`, `Look`

**使用示例**:
```cpp
void BlazeEntity::registerGoals() {
    // 优先级 4: 火球攻击
    m_goalSelector.addGoal(4, std::make_unique<BlazeFireballAttackGoal>(this));
}
```

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| CHARGE_TIME | 60 | 充能时间 (ticks) |
| FIREBALL_INTERVAL | 6 | 火球间隔 (ticks) |
| COOLDOWN_TIME | 100 | 冷却时间 (ticks) |
| MAX_FIREBALLS | 3 | 最大连发火球数 |
| MELEE_RANGE_SQ | 4.0 | 近战范围平方 (2格) |

---

### CreeperSwellGoal - 苦力怕膨胀目标

**职责**: 控制苦力怕在玩家靠近时膨胀并最终爆炸的行为。

**MC 1.16.5 参考**: `net.minecraft.entity.ai.goal.CreeperSwellGoal`

**执行条件**:
- 苦力怕已经有膨胀状态 (`getCreeperState() > 0`)，或
- 攻击目标在 9 格距离内 (3x3 范围)

**tick 行为**:
- 如果攻击目标为空：取消膨胀 (`setCreeperState(-1)`)
- 如果攻击目标距离 > 49 格 (7x7 范围)：取消膨胀
- 如果无法看到攻击目标：取消膨胀
- 否则：设置膨胀状态为 1

**互斥标志**: `Move`

**使用示例**:
```cpp
void CreeperEntity::registerGoals() {
    // 优先级 2: 膨胀爆炸
    m_goalSelector.addGoal(2, std::make_unique<CreeperSwellGoal>(this));
}
```

---

### RunAroundLikeCrazyGoal - 疯狂奔跑目标

**职责**: 控制未驯服的马在被骑乘时四处乱跑，增加驯服难度。

**MC 1.16.5 参考**: `net.minecraft.entity.ai.goal.RunAroundLikeCrazyGoal`

**执行条件**:
- 马未被驯服
- 马正在被骑乘

**tick 行为**:
- 马 AI 随机移动（模拟疯狂奔跑）
- 每 tick 有 1/50 概率执行驯服检查
- 如果驯服成功：
  - 调用 `setTamedBy(player)` 设置主人
  - 发送 `EntityStatus::TamingSucceeded`（爱心粒子）
- 如果驯服失败：
  - 增加 `temper` 进度（+5）
  - 调用 `removePassengers()` 甩下玩家
  - 调用 `makeMad()` 触发扬蹄动画和愤怒音效
  - 发送 `EntityStatus::TamingFailed`（烟雾粒子）

**驯服机制**:
```cpp
// MC 1.16.5 驯服概率计算
i32 temper = horse.getTemper();     // 当前进度
i32 maxTemper = horse.getMaxTemper(); // 最大进度（马默认100）

if (maxTemper > 0 && random.nextInt(maxTemper) < temper) {
    // 驯服成功
    horse.setTamedBy(player);
} else {
    // 增加进度
    horse.increaseTemper(5);
}
```

**互斥标志**: `Move`

**使用示例**:
```cpp
void AbstractHorseEntity::registerGoals() {
    // 优先级 1: 疯狂奔跑（未驯服时）
    m_goalSelector.addGoal(1, std::make_unique<RunAroundLikeCrazyGoal>(this, 1.2));
}
```

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| TEMPER_INCREASE | 5 | 每次驯服失败增加的进度 |
| TAMING_CHECK_CHANCE | 1/50 | 每 tick 驯服检查概率 |

**依赖关系**:
- 需要实体实现 `AbstractHorseEntity` 接口
- 需要 `setTamedBy(Player*)` 方法
- 需要 `makeMad()` 方法（扬蹄 + 愤怒音效）
- 需要 `removePassengers()` 方法

---

### EndermanTeleportGoal - 末影人传送目标

**职责**: 控制末影人在受到攻击或看向玩家时传送。

**MC 1.16.5 参考**: `net.minecraft.entity.ai.goal.EndermanTeleportGoal`

**状态**: 占位符，待实现

---

### LlamaFollowCaravanGoal - 羊驼跟随商队目标

**职责**: 控制羊驼跟随领头的羊驼形成商队。

**MC 1.16.5 参考**: `net.minecraft.entity.ai.goal.LlamaFollowCaravanGoal`

**状态**: 占位符，待实现

---

### DolphinJumpGoal - 海豚跳跃目标

**职责**: 控制海豚跳出水面跳跃。

**MC 1.16.5 参考**: `net.minecraft.entity.ai.goal.DolphinJumpGoal`

**执行条件**:
- 随机概率触发 (1/chance)
- 检查前方跳跃路径上有足够的水
- 检查水面上方有足够的空气空间

**行为**:
- `shouldExecute()`: 随机概率检查 + 跳跃路径验证
- `startExecuting()`: 根据朝向设置跳跃速度（水平 0.6, 垂直 0.7）
- `tick()`: 在空中时调整俯仰角
- `resetTask()`: 重置俯仰角为 0

**跳跃距离检查** (`JUMP_DISTANCES = {0, 1, 4, 5, 6, 7}`):
- 检查每个距离位置是否有水
- 检查每个距离位置上方是否有空气

**互斥标志**: `Jump`, `Move`

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| JUMP_DISTANCES | {0, 1, 4, 5, 6, 7} | 跳跃距离检查点 |
| HORIZONTAL_SPEED | 0.6f | 水平跳跃速度 |
| VERTICAL_SPEED | 0.7f | 垂直跳跃速度 |

**使用示例**:
```cpp
void DolphinEntity::registerGoals() {
    // 优先级 5: 跳跃
    m_goalSelector.addGoal(5, std::make_unique<DolphinJumpGoal>(this, 10));
}
```

---

### SwimToTreasureGoal - 海豚游向宝藏目标

**职责**: 当海豚被喂食鱼后，引导玩家到附近的宝藏结构。

**MC 1.16.5 参考**: `net.minecraft.entity.passive.DolphinEntity.SwimToTreasureGoal`

**执行条件**:
- 海豚已经得到了鱼 (`hasGotFish = true`)
- 空气值 >= 100

**行为**:
- `startExecuting()`: 寻找附近的沉船或海底废墟结构
- `tick()`: 向宝藏位置游泳，如果接近目标则重新规划路径
- `resetTask()`: 到达宝藏后清除鱼的标记

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| MIN_AIR | 100 | 最小空气值要求 |
| ARRIVE_DISTANCE | 4.0f | 到达距离 |
| CLOSE_TO_TARGET_DISTANCE | 12.0f | 接近目标距离 |

**互斥标志**: `Move`, `Look`

**使用示例**:
```cpp
void DolphinEntity::registerGoals() {
    // 优先级 1: 游向宝藏
    m_goalSelector.addGoal(1, std::make_unique<SwimToTreasureGoal>(this));
}
```

---

### SwimWithPlayerGoal - 海豚与玩家同游目标

**职责**: 当玩家在水中游泳时，海豚会跟随玩家并给予"海豚的恩惠"效果。

**MC 1.16.5 参考**: `net.minecraft.entity.passive.DolphinEntity.SwimWithPlayerGoal`

**执行条件**:
- 附近有正在游泳的玩家
- 海豚的攻击目标不是该玩家

**行为**:
- `startExecuting()`: 给玩家添加海豚的恩惠效果
- `tick()`: 跟随玩家，持续添加效果
- `resetTask()`: 清除目标玩家

**效果**:
- 给予玩家 `DolphinsGrace` 效果 (游泳加速)
- 效果持续时间: 100 ticks (5秒)
- 效果刷新间隔: 每 6 ticks

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| SEARCH_RADIUS | 10.0f | 搜索玩家半径 |
| CLOSE_DISTANCE_SQ | 6.25f | 接近距离平方 (2.5²) |
| MAX_DISTANCE_SQ | 256.0f | 最大距离平方 (16²) |
| EFFECT_DURATION | 100 | 效果持续时间 (ticks) |
| EFFECT_INTERVAL | 6 | 效果刷新间隔 (ticks) |

**互斥标志**: `Move`, `Look`

**使用示例**:
```cpp
void DolphinEntity::registerGoals() {
    // 优先级 2: 与玩家同游
    m_goalSelector.addGoal(2, std::make_unique<SwimWithPlayerGoal>(this, 4.0));
}
```

---

### PlayWithItemsGoal - 海豚玩物品目标

**职责**: 海豚会拾取水中的物品并扔出来玩。

**MC 1.16.5 参考**: `net.minecraft.entity.passive.DolphinEntity.PlayWithItemsGoal`

**执行条件**:
- 冷却时间已过
- 附近有可拾取的物品实体（在水中）
- 或海豚正在手中持有物品

**行为**:
- `startExecuting()`: 向物品移动
- `tick()`: 拾取物品或扔出物品
- `resetTask()`: 扔出手中物品

**物品选择条件**:
- 物品必须在水中
- 物品可以被拾取

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| SEARCH_RADIUS | 8.0f | 搜索物品半径 |
| THROW_VELOCITY | 0.3f | 扔出速度 |
| PICKUP_DELAY | 40 | 扔出物品的拾取延迟 (ticks) |
| MIN_COOLDOWN | 100 | 最小冷却时间 (ticks) |

**互斥标志**: `Move`, `Look`

**使用示例**:
```cpp
void DolphinEntity::registerGoals() {
    // 优先级 8: 玩物品
    m_goalSelector.addGoal(8, std::make_unique<PlayWithItemsGoal>(this));
}
```

---

### GuardianAttackGoal - 守卫者攻击目标

**职责**: 控制守卫者使用激光攻击目标。

**MC 1.16.5 参考**: `net.minecraft.entity.monster.GuardianEntity.AttackGoal`

**攻击阶段**:
1. **准备阶段**: tickCounter 从 -10 计数到 0
2. **充能动画**: tickCounter 从 0 计数到 80，在 tickCounter == 0 时发送 EntityStatus::GuardianAttack (21) 触发客户端音效
3. **发射阶段**: tickCounter >= 80 时造成伤害

**执行条件**:
- 有攻击目标
- 目标存活
- 目标在视线范围内

**攻击机制**:
- 魔法伤害 (4.0) + 物理伤害 (基于 ATTACK_DAMAGE 属性)
- 远古守卫者额外 +2.0 伤害
- 困难模式额外 +2.0 伤害 (TODO)
- 使用 `broadcastEntityStatus()` 发送状态21触发客户端攻击音效

**目标选择**:
- 玩家或鱿鱼
- 距离 > 3 格（距离平方 > 9.0）
- 非创造模式/观察者模式的玩家

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| ATTACK_DURATION | 80 | 攻击周期 (ticks) |
| ATTACK_RANGE | 15.0 | 攻击范围 |
| LASER_DAMAGE | 4.0 | 激光基础伤害 |
| ELDER_BONUS_DAMAGE | 2.0 | 远古守卫者额外伤害 |

**互斥标志**: `Move`, `Look`

**使用示例**:
```cpp
void GuardianEntity::registerGoals() {
    // 优先级 4: 激光攻击
    m_goalSelector.addGoal(4, std::make_unique<GuardianAttackGoal>(this));
}
```

---

### PuffGoal - 河豚膨胀目标

**职责**: 控制河豚在检测到敌对生物或玩家靠近时触发膨胀行为。

**MC 1.16.5 参考**: `net.minecraft.entity.passive.fish.PufferfishEntity.PuffGoal`

**执行条件**:
- 河豚存活
- 检测到碰撞箱向外扩展 2 格范围内的威胁实体

**威胁判定 (isEnemy)**:
- **玩家**: 非旁观者模式且非创造模式视为威胁
- **其他生物**: 非水生生物视为威胁（通过 LegacyEntityType 检查）
  - 水生生物（不是威胁）: Cod, Salmon, Pufferfish, TropicalFish, Squid, Dolphin, Turtle
  - 其他所有生物都是威胁

**行为流程**:
1. `shouldExecute()`: 检测范围内是否有威胁实体
2. `shouldContinueExecuting()`: 持续检测（与 shouldExecute 相同逻辑）
3. `startExecuting()`: 调用 `startPuffTimer()` 启动膨胀计时器
4. `resetTask()`: 调用 `resetPuffTimer()` 重置计时器

**PufferfishEntity.tick() 状态转换**:
```
Deflated → SemiPuffed: puffTimer == 1
SemiPuffed → FullyPuffed: puffTimer > 40

FullyPuffed → SemiPuffed: deflateTimer > 60
SemiPuffed → Deflated: deflateTimer > 100
```

**攻击机制 (attackNearbyEnemies)**:
- 膨胀状态时检测碰撞箱扩展 0.3 格范围内的敌人
- 伤害 = 1 + puffState (1-3)
- 中毒持续时间 = 60 * puffState ticks (60/120/180)
- 播放刺击音效 (ENTITY_PUFFER_FISH_STING)

**互斥标志**: 无（不与其他目标互斥）

**使用示例**:
```cpp
void PufferfishEntity::registerGoals() {
    AbstractFishEntity::registerGoals();
    // 优先级 1: 膨胀目标
    m_goalSelector.addGoal(1, std::make_unique<PuffGoal>(this));
}
```

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| DETECTION_RANGE | 2.0f | 检测范围（碰撞箱向外扩展） |
| PUFF_SEMI_THRESHOLD | 40 | 膨胀到半膨胀的阈值 (ticks) |
| DEFLATE_FULL_TO_SEMI | 60 | 完全膨胀到半膨胀的延迟 |
| DEFLATE_SEMI_TO_DEFLATE | 100 | 半膨胀到未膨胀的延迟 |

**碰撞箱尺寸**:
| 状态 | 缩放因子 | 碰撞箱尺寸 |
|------|----------|-----------|
| Deflated | 0.5 | 0.35 x 0.35 |
| SemiPuffed | 0.7 | 0.49 x 0.49 |
| FullyPuffed | 1.0 | 0.7 x 0.7 |

---

### SquidMoveRandomGoal - 鱿鱼随机游泳目标

**职责**: 控制鱿鱼在水中进行随机游泳移动。

**MC 1.16.5 参考**: `net.minecraft.entity.passive.SquidEntity.MoveRandomGoal`

**执行条件**:
- `shouldExecute()` 始终返回 true（鱿鱼随时可以游泳）

**tick 行为**:
1. 如果空闲时间 > 100 tick：停止移动（设置移动向量为零）
2. 否则以 1/50 概率，或不在水中，或没有移动向量时，生成新的随机移动向量：
   - 角度：随机 [0, 2π)
   - X = cos(角度) × 0.2
   - Y = -0.1 + random × 0.2 (范围 [-0.1, 0.1])
   - Z = sin(角度) × 0.2

**互斥标志**: 无（不与其他目标互斥）

**使用示例**:
```cpp
void SquidEntity::registerGoals() {
    // 优先级 0: 随机游泳（最高优先级）
    m_goalSelector.addGoal(0, std::make_unique<SquidMoveRandomGoal>(this));
}
```

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| IDLE_THRESHOLD | 100 | 空闲 tick 阈值 |
| RANDOM_CHANCE | 50 | 1/50 概率触发新方向 |
| HORIZONTAL_SPEED | 0.2f | 水平移动向量大小 |
| VERTICAL_MIN | -0.1f | 垂直移动向量最小值 |
| VERTICAL_RANGE | 0.2f | 垂直移动向量范围 |

**依赖**:
- 需要 SquidEntity 提供 `idleTime()`, `isInWater()`, `hasMovementVector()`, `setMovementVector()` 方法

---

### SquidFleeGoal - 鱿鱼逃跑目标

**职责**: 控制鱿鱼在受到攻击时向相反方向逃跑。

**MC 1.16.5 参考**: `net.minecraft.entity.passive.SquidEntity.FleeGoal`

**执行条件**:
- 鱿鱼必须在水中 (`isInWater()`)
- 必须有复仇目标 (`getLastHurtBy() != nullptr`)
- 复仇目标距离必须 < 10 格 (距离平方 < 100)

**tick 行为**:
1. 计算远离敌人的方向向量
2. 根据距离调整逃跑速度：
   - 基础速度 = 3.0
   - 距离 > 5 格时：速度 = 3.0 - (距离 - 5) / 5
3. 如果目标是空气，移除 Y 分量避免跳出水面
4. 设置移动向量（除以 20 转换为每 tick 速度）
5. 每 10 tick 的第 5 tick 产生气泡粒子

**互斥标志**: 无（不与其他目标互斥）

**使用示例**:
```cpp
void SquidEntity::registerGoals() {
    // 优先级 1: 逃跑目标（受攻击时逃跑）
    m_goalSelector.addGoal(1, std::make_unique<SquidFleeGoal>(this));
}
```

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| FLEE_DISTANCE_SQ | 100.0 | 触发逃跑的距离平方阈值 (10²) |
| BASE_FLEE_SPEED | 3.0f | 基础逃跑速度 |
| DISTANCE_THRESHOLD | 5.0 | 速度衰减开始距离 |
| SPEED_SCALE | 20.0f | 速度缩放因子 |
| BUBBLE_INTERVAL | 10 | 气泡粒子产生间隔 |
| BUBBLE_OFFSET | 5 | 气泡粒子产生偏移 |

**依赖**:
- 需要 SquidEntity 提供 `isInWater()`, `getLastHurtBy()`, `distanceSqTo()`, `x()`, `y()`, `z()`, `setMovementVector()` 方法

---

### BatRandomFlyGoal - 蝙蝠随机飞行目标

**职责**: 控制蝙蝠在空中进行随机飞行移动。

**MC 1.16.5 参考**: `net.minecraft.entity.passive.BatEntity` 第142-159行

**执行条件**:
- `shouldExecute()`: 蝙蝠不在休息状态时返回 true
- `shouldContinueExecuting()`: 蝙蝠不在休息状态时继续

**tick 行为**:
1. 检查是否需要选择新目标点：
   - 无目标时选择新目标
   - 目标不可用（非空气或Y<1）时选择新目标
   - 1/30 概率随机更换目标
   - 到达目标点（距离<2）时选择新目标
2. 选择随机目标点：
   - X: 当前位置 ±7 格
   - Y: 当前位置 -2 到 +4 格
   - Z: 当前位置 ±7 格
3. 平滑转向朝目标点飞行：
   - 计算方向向量 (signum * 0.5)
   - Y轴调整更强 (0.7 而非 0.5)
   - 速度调整因子 0.1
   - 更新偏航角

**互斥标志**: `Move`

**使用示例**:
```cpp
void BatEntity::registerGoals() {
    // 优先级 0: 随机飞行目标
    m_goalSelector.addGoal(0, std::make_unique<BatRandomFlyGoal>(this));
}
```

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| TARGET_RANGE_XZ | 7 | X/Z方向目标范围 |
| TARGET_RANGE_Y_MIN | -2 | Y方向目标范围下限 |
| TARGET_RANGE_Y_MAX | 4 | Y方向目标范围上限 |
| TARGET_REACH_DISTANCE | 2.0f | 到达目标的距离阈值 |
| DIRECTION_FACTOR | 0.5 | 水平方向因子 |
| VERTICAL_FACTOR | 0.7 | 垂直方向因子（更强） |
| VELOCITY_ADJUST | 0.1 | 速度调整因子 |
| TARGET_CHANGE_CHANCE | 30 | 1/30 概率更换目标 |
| MAX_TARGET_ATTEMPTS | 20 | 目标搜索最大尝试次数 |

**依赖**:
- 需要 BatEntity 提供 `isResting()`, `position()`, `velocity()`, `setVelocity()`, `yaw()`, `setRotation()`, `world()`, `getRandom()` 方法
- 需要 IWorld 提供 `getBlockState()` 方法
- 需要 BlockState 提供 `getBlock().isAir()` 方法

---

### BatRestGoal - 蝙蝠挂墙休息目标

**职责**: 控制蝙蝠在白天挂墙休息的行为。

**MC 1.16.5 参考**: `net.minecraft.entity.passive.BatEntity` 第125-163行

**执行条件**:
- `shouldExecute()`:
  - 白天时间 (dayTime < 12000)
  - 1/100 概率
  - 上方有固体方块可以倒挂
  - 蝙蝠当前不在休息状态
- `shouldContinueExecuting()`:
  - 仍在休息状态
  - 未被唤醒

**唤醒条件**:
- 夜间 (dayTime >= 12000)
- 玩家靠近（4格内）- TODO: 需要 world()->getClosestPlayer() 实现
- 失去支撑（上方不再是固体方块）

**startExecuting 行为**:
1. 设置休息状态为 true
2. 设置飞行状态为 false
3. 清除速度
4. 对齐位置到方块下方
5. 初始化转头计时器

**tick 行为**:
1. 1/200 概率随机选择新的转头角度
2. 平滑转向目标角度
3. 保持静止（速度清零）

**互斥标志**: `Move`, `Look`

**使用示例**:
```cpp
void BatEntity::registerGoals() {
    // 优先级 1: 挂墙休息目标
    m_goalSelector.addGoal(1, std::make_unique<BatRestGoal>(this));
}
```

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| REST_CHANCE | 100 | 1/100 概率尝试休息 |
| TURN_CHANCE | 200 | 1/200 概率随机转头 |
| DAY_TIME_THRESHOLD | 12000 | 白天时间阈值 |
| PLAYER_WAKE_DISTANCE | 4.0f | 玩家唤醒距离（TODO） |

**依赖**:
- 需要 BatEntity 提供 `isResting()`, `setResting()`, `setFlying()`, `position()`, `yaw()`, `pitch()`, `setRotation()`, `setVelocity()`, `height()`, `world()`, `getRandom()` 方法
- 需要 IWorld 提供 `getBlockState()`, `dayTime()` 方法
- 需要 BlockState 提供 `getBlock().isSolid()` 方法

---

### PhantomAttackPlayerTargetGoal - 幻翼攻击玩家目标选择器

**职责**: 为幻翼寻找并锁定攻击目标（玩家）。

**MC 1.16.5 参考**: `net.minecraft.entity.monster.PhantomEntity.AttackPlayerGoal`

**执行条件**:
- 幻翼存活
- 搜索延迟已过
- 64 格范围内存在可攻击的玩家

**行为**:
- `shouldExecute()`: 搜索范围内最近的可攻击玩家
- `shouldContinueExecuting()`: 确认攻击目标仍然有效
- `resetTask()`: 清除攻击目标

**目标选择条件**:
- 玩家存活
- 非旁观者模式
- 非创造模式
- 距离 ≤ 64 格
- 距离 > 20 格（不会太近）

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| SEARCH_RANGE | 64.0 | 搜索玩家范围 |
| 初始搜索延迟 | 20 ticks | 首次搜索延迟 |
| 成功后延迟 | 60 ticks | 找到目标后的搜索间隔 |

**互斥标志**: 无（目标选择器不设置互斥标志）

**使用示例**:
```cpp
void PhantomEntity::registerGoals() {
    // 优先级 1: 目标选择器
    m_targetSelector.addGoal(1, std::make_unique<PhantomAttackPlayerTargetGoal>(this));
}
```

---

### PhantomOrbitPointGoal - 幻翼环绕飞行目标

**职责**: 控制幻翼在目标上方环绕飞行。

**MC 1.16.5 参考**: `net.minecraft.entity.monster.PhantomEntity.OrbitPointGoal`

**执行条件**:
- 幻翼没有攻击目标，或
- 幻翼处于环绕阶段（CIRCLE）

**行为**:
- `startExecuting()`: 初始化环绕半径、高度偏移、方向
- `tick()`: 更新环绕角度，计算目标位置，移动幻翼

**环绕参数**:
- 环绕半径: 5.0 + random(10.0) = [5, 15)
- 高度偏移: -4.0 + random(9.0) = [-4, 5)
- 环绕方向: 1.0 或 -1.0（随机）

**tick 逻辑**:
1. 更新环绕角度: `angle += 0.05 * direction`
2. 计算环绕偏移:
   - X = radius * cos(angle)
   - Z = radius * sin(angle)
   - Y = heightOffset
3. 设置目标位置: 目标位置 + 环绕偏移
4. 移动幻翼向目标位置

**互斥标志**: `Move`

**使用示例**:
```cpp
void PhantomEntity::registerGoals() {
    // 优先级 3: 环绕飞行
    m_goalSelector.addGoal(3, std::make_unique<PhantomOrbitPointGoal>(this));
}
```

---

### PhantomPickAttackGoal - 幻翼攻击阶段选择目标

**职责**: 在环绕（CIRCLE）和俯冲（SWOOP）阶段之间切换。

**MC 1.16.5 参考**: `net.minecraft.entity.monster.PhantomEntity.PickAttackGoal`

**执行条件**:
- 有攻击目标
- 攻击目标存活
- 攻击延迟已过

**行为**:
- `startExecuting()`: 设置为环绕阶段，更新环绕位置
- `tick()`: 管理攻击阶段切换
- `resetTask()`: 更新环绕位置

**阶段切换逻辑**:
1. 环绕阶段：等待合适时机
2. 当接近目标且满足条件时，切换到俯冲阶段
3. 俯冲完成后，切换回环绕阶段

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| 攻击延迟 | 可变 | 切换攻击阶段的延迟 |

**互斥标志**: 无

**使用示例**:
```cpp
void PhantomEntity::registerGoals() {
    // 优先级 1: 攻击阶段选择
    m_goalSelector.addGoal(1, std::make_unique<PhantomPickAttackGoal>(this));
}
```

---

### PhantomSweepAttackGoal - 幻翼俯冲攻击目标

**职责**: 执行俯冲攻击，对目标造成伤害。

**MC 1.16.5 参考**: `net.minecraft.entity.monster.PhantomEntity.SweepAttackGoal`

**执行条件**:
- 有攻击目标
- 幻翼处于俯冲阶段（SWOOP）

**继续执行条件**:
- 攻击目标存活
- 附近没有猫（猫会驱赶幻翼）

**行为**:
- `tick()`: 向目标俯冲，检测碰撞造成伤害
- `resetTask()`: 切换回环绕阶段

**俯冲机制**:
- 直接向目标飞行
- 撞击目标造成攻击伤害
- 碰撞后切换回环绕阶段

**猫检测**:
- 每 20 tick 检测一次
- 附近有猫时停止攻击并逃离

**互斥标志**: `Move`

**使用示例**:
```cpp
void PhantomEntity::registerGoals() {
    // 优先级 2: 俯冲攻击
    m_goalSelector.addGoal(2, std::make_unique<PhantomSweepAttackGoal>(this));
}
```

---

### ShowVillagerFlowerGoal - 铁傀儡给村民展示花朵目标

**职责**: 铁傀儡在白天随机向村民展示罂粟花。

**MC 1.16.5 参考**: `net.minecraft.entity.passive.IronGolemEntity.ShowVillagerFlowerGoal`

**执行条件**:
- 白天时间 (isDaytime)
- 1/8000 概率触发
- 6 格范围内有村民

**行为**:
- `shouldExecute()`: 检查白天、概率和附近村民
- `startExecuting()`: 设置持花状态，看向时间 400 ticks
- `tick()`: 看向村民
- `resetTask()`: 清除持花状态

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| SEARCH_RANGE | 6.0f | 搜索村民范围 |
| SEARCH_HEIGHT | 2.0f | 搜索村民高度 |
| LOOK_DURATION | 400 | 看向持续时间 (ticks = 20秒) |
| CHANCE | 8000 | 执行概率倒数 (1/8000) |

**互斥标志**: `Move`, `Look`

**使用示例**:
```cpp
void IronGolemEntity::registerGoals() {
    // 优先级 5: 给村民展示罂粟花
    m_goalSelector.addGoal(5, std::make_unique<ShowVillagerFlowerGoal>(this));
}
```

**依赖**:
- 需要 IronGolemEntity 提供 `setHoldingRose()`, `world()`, `position()`, `boundingBox()`, `lookController()` 方法
- 需要 VillagerEntity 存在
- 需要 EntityUtils::findClosestEntity() 函数

---

## 依赖关系

```mermaid
graph TD
    A[Goal 基类] --> B[CreeperSwellGoal]
    A --> C[RunAroundLikeCrazyGoal]
    A --> D[GuardianAttackGoal]
    A --> E[BlazeFireballAttackGoal]
    A --> F[PuffGoal]
    A --> G[SquidMoveRandomGoal]
    A --> H[SquidFleeGoal]
    A --> I[DolphinJumpGoal]
    A --> J[SwimToTreasureGoal]
    A --> K[SwimWithPlayerGoal]
    A --> L[PlayWithItemsGoal]
    A --> M[BatRandomFlyGoal]
    A --> N[BatRestGoal]
    A --> O[PhantomAttackPlayerTargetGoal]
    A --> P[PhantomOrbitPointGoal]
    A --> Q[PhantomPickAttackGoal]
    A --> R[PhantomSweepAttackGoal]

    B --> S[CreeperEntity]
    C --> T[AbstractHorseEntity]
    D --> U[GuardianEntity]
    E --> V[BlazeEntity]
    F --> W[PufferfishEntity]
    G --> X[SquidEntity]
    H --> X
    I --> Y[DolphinEntity]
    J --> Y
    K --> Y
    L --> Y
    M --> Z[BatEntity]
    N --> Z
    O --> AA[PhantomEntity]
    P --> AA
    Q --> AA
    R --> AA
```

---

## 使用方法

### 1. 在实体中注册特殊目标

```cpp
void MyEntity::registerGoals() {
    // 调用父类方法
    ParentEntity::registerGoals();

    // 注册特殊目标
    m_goalSelector.addGoal(2, std::make_unique<CreeperSwellGoal>(this));
}
```

### 2. 实现新的特殊目标

```cpp
class MySpecialGoal : public Goal {
public:
    explicit MySpecialGoal(MyEntity* entity)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
        , m_entity(entity)
    {
        MC_ASSERT(entity != nullptr);
    }

    bool shouldExecute() override {
        if (!m_entity) return false;
        // 检查执行条件
        return m_entity->someCondition();
    }

    void startExecuting() override {
        // 初始化状态
    }

    void tick() override {
        // 更新逻辑
    }

    void resetTask() override {
        // 清理状态
    }

private:
    MyEntity* m_entity;
};
```

---

## 容易踩的坑

### 1. 忘记设置互斥标志

**问题**: 特殊目标与其他目标冲突。

**解决**: 始终设置正确的互斥标志。

```cpp
// 正确：设置互斥标志
CreeperSwellGoal::CreeperSwellGoal(CreeperEntity* creeper)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_creeper(creeper)
{
}
```

### 2. 空指针检查缺失

**问题**: 实体指针在目标执行期间可能失效。

**解决**: 在每个方法开始检查空指针。

```cpp
void CreeperSwellGoal::tick() {
    if (!m_creeper) return;
    if (!m_attackTarget || !m_attackTarget->isAlive()) {
        m_creeper->setCreeperState(-1);
        return;
    }
    // 正常逻辑...
}
```

### 3. 距离比较使用 sqrt

**问题**: 频繁调用 `sqrt()` 影响性能。

**解决**: 使用距离平方比较。

```cpp
// 低效
f32 distance = std::sqrt(dx * dx + dy * dy + dz * dz);
if (distance < 7.0f) { }

// 高效
f32 distSq = dx * dx + dy * dy + dz * dz;
if (distSq < 49.0f) { }  // 7 * 7 = 49
```

---

## EvokerGoals - 唤魔者专用目标

包含唤魔者的尖牙攻击和召唤恼鬼目标。

### EvokerAttackSpellGoal - 尖牙攻击目标

**职责**: 控制唤魔者对目标发动尖牙攻击。

**MC 1.16.5 参考**: `net.minecraft.entity.monster.SpellcastingIllagerEntity.SpellGoal` 子类

**攻击模式**:

| 模式 | 条件 | 尖牙排列 |
|------|------|----------|
| 近距离 | 目标距离 < 3 格 | 双圈（内圈5个，外圈8个） |
| 远距离 | 目标距离 >= 3 格 | 直线16个尖牙 |

**近距攻击参数**:
| 参数 | 值 | 说明 |
|------|-----|------|
| INNER_RADIUS | 1.5f | 内圈半径 |
| INNER_COUNT | 5 | 内圈尖牙数 |
| OUTER_RADIUS | 2.5f | 外圈半径 |
| OUTER_COUNT | 8 | 外圈尖牙数 |

**远距攻击参数**:
| 参数 | 值 | 说明 |
|------|-----|------|
| FANG_COUNT | 16 | 直线尖牙数 |
| FANG_SPACING | 1.25f | 尖牙间距 |

**施法参数**:
| 参数 | 值 | 说明 |
|------|-----|------|
| WARMUP_DELAY | 0 | 尖牙攻击无预热延迟 |
| CASTING_TIME | 40 | 施法时间 (ticks) |
| COOLDOWN | 100 | 冷却时间 (ticks) |

### EvokerSummonSpellGoal - 召唤恼鬼目标

**职责**: 控制唤魔者召唤恼鬼助战。

**MC 1.16.5 参考**: `net.minecraft.entity.monster.EvokerEntity.SummonSpellGoal`

**召唤条件**:
- 周围恼鬼数量 < 8 个
- 施法冷却已过

**召唤参数**:
| 参数 | 值 | 说明 |
|------|-----|------|
| VEX_SUMMON_COUNT | 3 | 每次召唤数量 |
| VEX_SEARCH_RANGE | 16.0f | 搜索恼鬼范围 |
| MAX_VEX_COUNT | 8 | 最大恼鬼数量 |
| MIN_LIFE_TIME | 600 | 最短生命 (ticks, 30秒) |
| MAX_LIFE_TIME | 2400 | 最长生命 (ticks, 120秒) |
| SPAWN_OFFSET_MIN | -2 | 生成位置偏移最小值 |
| SPAWN_OFFSET_MAX | 2 | 生成位置偏移最大值 |
| CASTING_TIME | 100 | 施法时间 (ticks) |
| COOLDOWN | 340 | 冷却时间 (ticks) |

**countNearbyVexes() 实现**:
使用 `IWorld::getEntitiesInAABB()` 统计唤魔者周围 16 格内的恼鬼数量：

```cpp
i32 EvokerSummonSpellGoal::countNearbyVexes() const
{
    if (m_evoker == nullptr || m_evoker->world() == nullptr) {
        return 0;
    }
    IWorld* world = m_evoker->world();
    AxisAlignedBB searchBox = m_evoker->boundingBox().grow(16.0f);
    std::vector<Entity*> entities = world->getEntitiesInAABB(searchBox, m_evoker);
    i32 vexCount = 0;
    for (Entity* entity : entities) {
        if (entity == nullptr || entity->isRemoved()) continue;
        if (entity->legacyType() == LegacyEntityType::Vex) vexCount++;
    }
    return vexCount;
}
```

**互斥标志**: `Move`, `Look`

---

## 涉及的测试用例

| 测试名称 | 说明 |
|----------|------|
| GoalTest.* | Goal 基础测试 |
| GoalSelectorTest.* | 目标选择器测试 |
| PrioritizedGoalTest.* | 优先级目标测试 |
| CreeperSwellGoalBasicTest.* | 苦力怕膨胀目标常量测试 |
| BlazeFireballAttackGoalBasicTest.* | 烈焰人火球攻击目标常量测试 |
| PufferfishEntityTest.* | 河豚实体膨胀状态、计时器、尺寸测试 |
| PuffGoalTest.* | 河豚膨胀目标构造和类型名称测试 |
| SquidGoalsTest.* | 鱿鱼目标测试（移动向量、AI目标执行条件） |
| BatGoalsTest.* | 蝙蝠目标测试（状态切换、飞行目标、休息目标） |
| DolphinGoalsTest.* | 海豚目标测试（跳跃、寻宝、与玩家同游、玩物品） |
| PhantomGoalsTest.* | 幻翼目标测试（攻击阶段切换、环绕飞行、俯冲攻击） |
| SlimeGoalsTest.* | 史莱姆目标测试（漂浮、攻击、随机转向） |
| IronGolemGoalsTest.* | 铁傀儡目标测试（展示花朵、移动追踪、重置愤怒） |

---

## 参考资料

- Minecraft Java 1.16.5 `net.minecraft.entity.ai.goal.CreeperSwellGoal`
- Minecraft Java 1.16.5 `net.minecraft.entity.ai.goal.RunAroundLikeCrazyGoal`
- Minecraft Java 1.16.5 `net.minecraft.entity.monster.GuardianEntity.GuardianAttackGoal`
- Minecraft Java 1.16.5 `net.minecraft.entity.monster.BlazeEntity.FireballAttackGoal`
- Minecraft Java 1.16.5 `net.minecraft.entity.passive.BatEntity` (蝙蝠飞行和休息逻辑)
- Minecraft Java 1.16.5 `net.minecraft.entity.passive.DolphinEntity` (海豚跳跃、寻宝、与玩家同游)
- Minecraft Java 1.16.5 `net.minecraft.entity.monster.PhantomEntity` (幻翼环绕、俯冲攻击)
- Minecraft Java 1.16.5 `net.minecraft.entity.passive.IronGolemEntity.ShowVillagerFlowerGoal` (铁傀儡送花)
- 本项目 CLAUDE.md 文档
