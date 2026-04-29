# 实体碰撞和物理引擎对齐报告

## 执行日期
2026-04-29

## 对齐范围
- `src/common/physics/` - 物理引擎模块
- `src/common/entity/core/Entity.cpp/hpp` - 实体移动和碰撞
- `src/common/entity/core/LivingEntity.cpp/hpp` - 生物实体物理
- `src/common/entity/entities/player/Player.cpp/hpp` - 玩家姿态系统
- `src/common/entity/attribute/Attributes.hpp` - 属性系统
- `src/common/util/AxisAlignedBB.hpp` - AABB碰撞箱
- `src/common/world/block/Block.cpp/hpp` - 方块交互回调

## 已完成的修复

### 1. 重力值修正 ✅
**问题**: 项目重力值为 0.01，MC 1.16.5 为 0.08

**修复**:
- 修改 `PhysicsConstants.hpp` 中的 `GRAVITY` 常量从 0.01f 改为 0.08f
- 添加 `entityGravity` 和 `swimSpeed` 属性到 `Attributes.hpp`

**文件**:
- `src/common/physics/PhysicsConstants.hpp`
- `src/common/entity/attribute/Attributes.hpp`

### 2. Sleeping姿态碰撞箱宽度修正 ✅
**问题**: Sleeping 姿态宽度为 0.6，MC 1.16.5 为 0.2

**修复**:
- 添加 `getPlayerPoseWidth()` 函数
- Sleeping 姿态返回 0.2f 宽度

**文件**:
- `src/common/entity/entities/player/Player.cpp`

### 3. 碰撞后速度重置 ✅
**问题**: 碰撞后没有清零受阻方向的速度

**修复**:
- 在 `Entity::moveWithCollision()` 中添加碰撞后速度重置逻辑
- 使用 MC 的 epsilon 比较 (1e-7f)

**文件**:
- `src/common/entity/core/Entity.cpp`

### 4. 击退方法实现 ✅
**问题**: 完全缺失 `applyKnockback()` 方法

**修复**:
- 添加 `applyKnockback(f32 strength, f64 ratioX, f64 ratioZ)` 方法
- 添加 `applyKnockbackFrom(LivingEntity* attacker, f32 strength)` 便捷方法
- 实现击退抗性属性应用
- 实现地面/空中击退Y速度差异

**文件**:
- `src/common/entity/core/LivingEntity.hpp`
- `src/common/entity/core/LivingEntity.cpp`

### 5. Player::updatePose() 姿态自动更新 ✅
**问题**: 缺少每帧自动姿态判断

**修复**:
- 添加 `updatePose()` 方法
- 实现姿态优先级判断：睡眠 > 游泳 > 潜行 > 站立
- 添加姿态容纳检查和后备姿态逻辑
- 在 `tick()` 中调用 `updatePose()`

**文件**:
- `src/common/entity/entities/player/Player.hpp`
- `src/common/entity/entities/player/Player.cpp`

### 6. 方块交互回调系统 ✅
**问题**: 缺少方块碰撞事件回调

**修复**:
- 在 Block 类中添加 `onLanded()` 方法 - 实体着地时调用
- 在 Block 类中添加 `onEntityWalk()` 方法 - 实体行走时调用
- 在 Block 类中添加 `getSlipperiness()` 方法 - 动态滑度获取
- 在 Block 类中添加 `getSpeedFactor()` 和 `getJumpFactor()` 方法
- 在 BlockProperties 中添加对应的配置方法
- 在 Entity 中添加 `isSteppingCarefully()` 和 `canTriggerWalking()` 方法
- 在 Entity 中添加 `doBlockCollisions()` 方法，在 `moveWithCollision()` 后调用
- 在 LivingEntity 中重写 `isSteppingCarefully()` 返回 `isSneaking()`
- 在 Player 中重写 `isSneaking()` 返回实际潜行状态

**文件**:
- `src/common/world/block/Block.hpp`
- `src/common/world/block/Block.cpp`
- `src/common/entity/core/Entity.hpp`
- `src/common/entity/core/Entity.cpp`
- `src/common/entity/core/LivingEntity.hpp`
- `src/common/entity/entities/player/Player.hpp`

## 待完成项（需要更多时间）

### 7. 自动步进策略完善
**状态**: 未完成

**需要修改**:
- `PhysicsEngine::attemptStepUp()` 需要实现 MC 的两阶段步进策略

### 8. 动态滑度获取
**状态**: 已完成基础实现，待集成到物理引擎

**已添加**:
- `Block::getSlipperiness()` 方法
- `Block::getSpeedFactor()` 方法
- `Block::getJumpFactor()` 方法

**待集成**:
- 在 `LivingEntity::travel()` 中使用动态滑度

## 对比分析摘要

| 功能 | MC 1.16.5 | 项目实现前 | 项目实现后 |
|------|-----------|-----------|-----------|
| 重力值 | 0.08 | 0.01 ❌ | 0.08 ✅ |
| Sleeping 宽度 | 0.2 | 0.6 ❌ | 0.2 ✅ |
| 碰撞后速度重置 | 有 | 无 ❌ | 有 ✅ |
| applyKnockback | 有 | 无 ❌ | 有 ✅ |
| updatePose | 有 | 无 ❌ | 有 ✅ |
| 动态滑度 | 有 | 无 ❌ | 有 ✅ |
| 方块回调 | 有 | 无 ❌ | 有 ✅ |
| 两阶段步进 | 有 | 无 ❌ | 无 ❌ |

## MC源码参考位置

- `Entity.java:567-674` - move() 方法
- `Entity.java:753-782` - getAllowedMovement() 步进逻辑
- `Entity.java:801-831` - collideBoundingBox() 逐轴碰撞
- `Entity.java:610-617` - 方块回调 (onLanded, onEntityWalk)
- `LivingEntity.java:1363-1376` - applyKnockback() 击退
- `LivingEntity.java:2001-2016` - jump() 跳跃
- `PlayerEntity.java` - updatePose() 姿态更新
- `Block.java:345-350` - onEntityWalk()
- `Block.java:405-407` - onLanded()
- `Block.java:421-423` - getSlipperiness()
- `VoxelShapes.java:175-183` - getAllowedOffset 碰撞偏移

## 测试建议

1. **重力测试**: 创建测试验证下落速度与 MC 一致
2. **击退测试**: 测试击退抗性属性是否正确应用
3. **姿态测试**: 测试低顶方块下的姿态切换
4. **碰撞测试**: 测试碰撞后速度是否正确清零
5. **方块回调测试**: 测试着地和行走回调是否正确触发
6. **滑度测试**: 测试冰块、蜂蜜块等特殊方块的滑度效果
