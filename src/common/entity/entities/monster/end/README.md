# 末影人实体模块

末影人（Enderman）是末地的中立型怪物，具有瞬移、搬运方块和被注视时激怒的特性。

## 目录结构

```text
src/common/entity/entities/monster/end/
├── EndermanEntity.hpp    # 末影人实体声明
├── EndermanEntity.cpp    # 末影人实体实现
├── ShulkerEntity.hpp     # 潜影贝实体声明
├── ShulkerEntity.cpp     # 潜影贝实体实现
└── README.md             # 本文档
```

## 文件介绍

### EndermanEntity.hpp

声明 `EndermanEntity` 类以及末影人专用接口：

- **IAngerable 接口实现**:
  - `getAttackTarget()` / `setAttackTarget()` - 攻击目标管理
  - `getRevengeTarget()` / `setRevengeTarget()` - 复仇目标管理
  - `isAngry()` / `setAngry()` - 愤怒状态管理
  - `getAngerTime()` / `setAngerTime()` - 愤怒计时器

- **注视检测**:
  - `isScreaming()` / `setScreaming()` - 被注视状态
  - `shouldAttackPlayer(player)` - 检查玩家是否正在注视末影人

- **瞬移系统**:
  - `teleport()` - 随机瞬移
  - `teleportToTarget()` - 瞬移到目标附近
  - `teleportAwayFromWater()` - 瞬移避开水

- **搬方块系统**:
  - `isHoldingBlock()` - 是否拿着方块
  - `getHeldBlockState()` / `setHeldBlockState()` - 拿着的方块状态
  - `placeHeldBlock()` - 放下方块
  - `pickUpBlock()` - 拾取方块

### EndermanEntity.cpp

实现末影人的核心行为：

- **AI 目标注册**:
  - EndermanStareGoal (优先级 1) - 注视玩家
  - MeleeAttackGoal (优先级 2) - 近战攻击
  - LookAtGoal (优先级 7) - 看向玩家
  - LookRandomlyGoal (优先级 8) - 随机看向
  - EndermanFindPlayerGoal (目标选择器优先级 1) - 查找注视玩家

- **注视检测逻辑**:
  - 检查玩家是否戴着南瓜头
  - 计算玩家视线与玩家到末影人向量的点积
  - 根据距离调整阈值
  - 检查视线是否被方块阻挡

- **水伤害逻辑**:
  - 在水中或雨中受到伤害
  - 自动瞬移避开水

## 模块关系

- `EndermanEntity` 继承自 `MonsterEntity` 并实现 `IAngerable` 接口
- `EndermanStareGoal` 和 `EndermanFindPlayerGoal` 在 `ai/goal/goals/special/EndermanGoals.cpp` 中实现
- 依赖 `Player::isLookingAt()` 和 `Player::isWearingPumpkin()` 进行注视检测

## 整体职责

1. **中立行为**: 默认不攻击玩家
2. **注视激怒**: 玩家注视眼睛时被激怒
3. **瞬移能力**: 受攻击或特定条件下瞬移
4. **方块搬运**: 可以拾取和放置方块
5. **水敏感**: 在水中或雨中受到伤害并瞬移

## 注视检测机制

### 触发条件

1. 玩家未戴南瓜头
2. 玩家视线指向末影人眼睛
3. 视线无方块阻挡

### 点积计算

```cpp
// 玩家视线方向向量
Vector3 lookVec = player.getLookVector();

// 玩家眼睛到末影人眼睛的向量
Vector3 toTarget = endermanEyePos - playerEyePos;
toTarget = toTarget.normalized();

// 点积计算
f32 dotProduct = lookVec.dot(toTarget);

// 根据距离调整阈值：距离越远，阈值越高
f32 threshold = 1.0f - 0.025f / distance;
bool isLooking = dotProduct > threshold;
```

### MC 1.16.5 参考

- `EndermanEntity.shouldAttackPlayer()` - 注视检测
- `Entity.getLook()` - 视线方向计算
- `ItemStack.isEnderMask()` - 南瓜头检测

## 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| TELEPORT_COOLDOWN | 50 | 瞬移冷却 (ticks) |
| ANGER_DURATION | 600 | 愤怒持续时间 (ticks, 30秒) |
| TELEPORT_RANGE | 64.0 | 瞬移范围 |
| WATER_DAMAGE | 1.0 | 水伤害值 |

## 属性

| 属性 | 值 | 说明 |
|------|-----|------|
| MAX_HEALTH | 40.0 | 最大生命值 |
| MOVEMENT_SPEED | 0.3 | 移动速度 |
| ATTACK_DAMAGE | 7.0 | 攻击伤害 |
| FOLLOW_RANGE | 64.0 | 追踪范围 |

## 使用方法

```cpp
// 创建末影人
auto enderman = std::make_unique<EndermanEntity>(LegacyEntityType::Enderman, entityId);

// 检查玩家是否激怒末影人
if (enderman->shouldAttackPlayer(player)) {
    // 玩家正在注视末影人
}

// 设置愤怒状态
enderman->setAngry(true);
enderman->setAttackTarget(&player);

// 触发瞬移
enderman->teleport();
```

## 容易踩的坑

- **注视检测需要完整的世界环境**: `canSee()` 方法需要世界支持，测试时需要 mock
- **眼睛高度差异**: 玩家眼睛高度 (1.62) 和末影人眼睛高度 (2.55) 不同，计算注视向量时需要使用眼睛位置
- **瞬移冷却**: 瞬移后需要等待冷却时间才能再次瞬移
- **愤怒状态同步**: `setScreaming()` 和 `setAngry()` 需要同步设置

## 测试用例

- [tests/common/entity/entities/monster/end/EndermanStareDetectionTest.cpp](../../../../../../../tests/common/entity/entities/monster/end/EndermanStareDetectionTest.cpp)
- `PlayerLookVectorTest` - 视线方向向量计算测试
- `PlayerEyePositionTest` - 眼睛位置测试
- `PlayerPumpkinTest` - 南瓜头检测测试
- `PlayerLookingAtTest` - 注视目标检测测试
- `EndermanStareGoalTest` - 注视目标 AI 测试
- `EndermanFindPlayerGoalTest` - 查找玩家目标选择器测试
- `EndermanShouldAttackPlayerTest` - 激怒条件测试
- `EndermanConstantsTest` - 常量验证测试
- `LookVectorPrecisionTest` - 精度测试

## 近期补全

- **已实现注视检测**（2026-05-16）：
  - `Player::getLookVector()` - 根据yaw/pitch计算视线方向
  - `Player::getEyePosition()` - 获取眼睛位置
  - `Player::isWearingPumpkin()` - 检查南瓜头
  - `Player::isLookingAt()` - 注视目标检测
  - `EndermanEntity::shouldAttackPlayer()` - 综合判断激怒条件
  - `EndermanStareGoal` - 注视玩家目标
  - `EndermanFindPlayerGoal` - 查找注视玩家目标选择器
  - 参考 MC 1.16.5 `EndermanEntity.shouldAttackPlayer()` 和 `Entity.getLook()`
