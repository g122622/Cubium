# AI工具模块

本目录包含AI系统的工具类和辅助功能。

## 目录结构

```
util/
├── RandomPositionGenerator.hpp  # 随机位置生成器
├── RandomPositionGenerator.cpp  # 随机位置生成器实现
└── README.md                    # 本文档
```

## RandomPositionGenerator

为AI目标生成智能随机位置的工具类，参考MC 1.16.5的`RandomPositionGenerator`。

### 主要方法

| 方法 | 描述 |
|------|------|
| `findRandomTarget(creature, xzRange, yRange, outPos)` | 在指定范围内生成随机可行走位置 |
| `findRandomTargetBlockAwayFrom(creature, xzRange, yRange, avoidPos, outPos)` | 生成远离指定位置的目标 |
| `findRandomTargetTowards(creature, xzRange, yRange, targetPos, outPos)` | 生成朝向指定位置的随机目标 |
| `findRandomTargetTowardsScaled(creature, xzRange, yRange, targetPos, angleRange, outPos)` | 生成朝向指定位置的缩放随机目标（海豚寻宝用） |
| `findRandomTargetBlock(creature, xzRange, yRange, avoidPos, outPos)` | 生成随机方块目标位置（飞行实体用） |
| `findRandomTargetBlockTowards(creature, xzRange, yRange, targetPos, outPos)` | 生成朝向目标位置的方块目标（水生生物用） |
| `getLandPos(creature, xzRange, yRange, outPos)` | 获取陆地位置 |
| `findRandomTargetAvoidWater(creature, xzRange, yRange, outPos)` | 生成避开水域的随机位置 |

### 辅助方法

| 方法 | 描述 |
|------|------|
| `isPositionWalkable(creature, x, y, z)` | 检查位置是否可行走 |
| `getGroundHeight(world, x, startY, z)` | 获取地面高度 |
| `calculatePositionScore(creature, pos)` | 计算位置评分 |

### 新增方法详解

#### findRandomTargetTowardsScaled

**职责**: 在目标方向生成一个缩放后的随机目标位置，角度限制在指定范围内。

**MC 1.16.5 参考**: `net.minecraft.entity.ai.RandomPositionGenerator.findRandomTargetTowardsScaled`

**用途**: 海豚寻宝时向宝藏位置导航，在目标方向的一定角度范围内选择随机位置。

**参数**:
| 参数 | 类型 | 说明 |
|------|------|------|
| creature | CreatureEntity* | 生物实体 |
| xzRange | i32 | 水平搜索范围（格） |
| yRange | i32 | 垂直搜索范围（格） |
| targetPos | const Vector3& | 目标位置（宝藏位置） |
| angleRange | f64 | 方向角限制范围（弧度） |
| outPos | Vector3& | 输出位置 |

**返回**: 是否找到有效位置

**使用示例**:
```cpp
// 海豚向宝藏位置导航
Vector3 targetPos;
if (RandomPositionGenerator::findRandomTargetTowardsScaled(
        dolphin, xzRange, yRange, treasurePos, PI / 8.0, targetPos)) {
    navigator->moveTo(targetPos.x, targetPos.y, targetPos.z, speed);
}
```

#### findRandomTargetBlockTowards

**职责**: 在朝向目标的方向选择一个方块位置，不要求位置可行走。

**MC 1.16.5 参考**: `net.minecraft.entity.ai.RandomPositionGenerator.findRandomTargetBlockTowards`

**用途**: 水生生物（如海豚）在朝向目标的方向选择方块位置，检查是否是水或可通过的方块。

**参数**:
| 参数 | 类型 | 说明 |
|------|------|------|
| creature | CreatureEntity* | 生物实体 |
| xzRange | i32 | 水平搜索范围（格） |
| yRange | i32 | 垂直搜索范围（格） |
| targetPos | const Vector3& | 目标位置 |
| outPos | Vector3& | 输出位置 |

**返回**: 是否找到有效位置

**使用示例**:
```cpp
// 水生生物向目标方向选择方块位置
Vector3 targetPos;
if (RandomPositionGenerator::findRandomTargetBlockTowards(
        dolphin, xzRange, yRange, playerPos, targetPos)) {
    navigator->moveTo(targetPos.x, targetPos.y, targetPos.z, speed);
}
```

### 位置评分算法

`calculatePositionScore` 方法使用以下因素计算位置评分：

1. **基础评分**: +10.0f
2. **距离评分**:
   - 太近（< 2.5 格）: -50.0f
   - 太远（> 20 格）: -10.0f
3. **安全性评分**:
   - 岩浆附近: -100.0f
   - 水中: -5.0f
4. **可达性评分**:
   - 可行走: +20.0f
   - 不可行走: -50.0f
5. **路径权重**: 使用 `creature->getPathWeight()` 获取，放大 10 倍

### getPathWeight 方法

`CreatureEntity::getPathWeight()` 方法用于获取实体对位置的偏好程度：

| 实体类型 | 偏好条件 | 权重值 |
|---------|---------|--------|
| **AnimalEntity** | 草地 | 10.0F |
| **AnimalEntity** | 非草地 | `亮度 - 0.5F` |
| **MonsterEntity** | 黑暗环境 | `0.5F - 亮度` |
| **CreatureEntity** | 默认 | 0.0F |

子类可以重写 `getPathWeight()` 方法来实现特定的位置偏好：

- **水生生物** (如 GuardianEntity): 水中返回 10.0F + 亮度调整
- **岩浆生物** (如 StriderEntity): 岩浆中返回 10.0F
- **飞行生物** (如 BeeEntity): 空气中返回 10.0F
- **特定栖息地生物** (如 MooshroomEntity): 菌丝上返回 10.0F

### 使用示例

```cpp
#include "entity/ai/util/RandomPositionGenerator.hpp"

// RandomWalkingGoal 中使用
Vector3 targetPos;
if (RandomPositionGenerator::findRandomTarget(creature, 10, 7, targetPos)) {
    navigator->moveTo(targetPos.x, targetPos.y, targetPos.z, speed);
}

// PanicGoal 中寻找远离危险的位置
Vector3 escapePos;
if (RandomPositionGenerator::findRandomTargetBlockAwayFrom(
        creature, 16, 7, dangerPos, escapePos)) {
    navigator->moveTo(escapePos.x, escapePos.y, escapePos.z, speed);
}

// AvoidEntityGoal 中远离威胁实体
Vector3 avoidPos(threat->x(), threat->y(), threat->z());
Vector3 escapePos;
if (RandomPositionGenerator::findRandomTargetBlockAwayFrom(
        creature, 16, 7, avoidPos, escapePos)) {
    navigator->moveTo(escapePos.x, escapePos.y, escapePos.z, speed);
}
```

### 与MC 1.16.5的对应关系

| MC 1.16.5 方法 | C++ 方法 |
||----------------|----------|
| `findRandomTarget(creature, xz, y)` | `findRandomTarget()` |
| `findRandomTargetBlockAwayFrom(creature, xz, y, avoidPos)` | `findRandomTargetBlockAwayFrom()` |
| `findRandomTargetTowards(creature, xz, y, targetPos)` | `findRandomTargetTowards()` |
| `getLandPos(creature, xz, y)` | `getLandPos()` |
| `getWaterPos(creature, xz, y)` | 待实现 |

### 算法细节

1. **位置候选生成**：生成多个候选位置（默认10个）
2. **评分系统**：基于安全性、可达性、距离、路径权重等因素评分
3. **最佳选择**：选择评分最高的候选位置
4. **地面检测**：自动寻找可行走的地面高度
5. **危险规避**：避开岩浆、深水等危险方块

## 依赖关系

```
RandomPositionGenerator
├── CreatureEntity (生物实体)
│   └── getPathWeight() - 获取路径权重
├── IWorld (世界接口)
│   ├── getBrightness() - 获取亮度
│   ├── isLavaAt() - 检查岩浆
│   ├── isWaterAt() - 检查水
│   └── getBlockState() - 获取方块状态
├── PathNavigator (路径导航器)
├── Random (随机数)
└── MathUtils (数学工具)
```

## 相关Goals

以下Goal使用`RandomPositionGenerator`：
- `RandomWalkingGoal` - 随机漫步
- `PanicGoal` - 恐慌逃跑
- `AvoidEntityGoal` - 避开实体
- `WaterAvoidingRandomWalkingGoal` - 避水漫步
- `FleeSunGoal` - 逃离阳光

## 测试用例

测试文件：`tests/common/entity/ai/util/RandomPositionGeneratorTest.cpp`

| 测试用例 | 描述 |
|----------|------|
| `FindRandomTarget` | 测试随机目标生成 |
| `FindRandomTargetAwayFrom` | 测试远离位置生成 |
| `GetLandPos` | 测试地面检测 |
| `AvoidWater` | 测试避水功能 |
| `PositionScore` | 测试位置评分 |
| `AnimalPathWeight` | 测试动物路径权重（草地偏好） |
| `MonsterPathWeight` | 测试怪物路径权重（黑暗偏好） |
