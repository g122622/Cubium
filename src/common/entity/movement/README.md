# Movement 模块

移动模块实现了实体的自动跳跃功能，遵循 Minecraft Java 1.16.5 的行为规范。

## 目录结构

```
movement/
├── AutoJumpConstants.hpp   # 自动跳跃常量定义
├── AutoJump.hpp            # 自动跳跃系统头文件
├── AutoJump.cpp            # 自动跳跃系统实现
└── README.md               # 本文档
```

## 文件详解

### AutoJumpConstants.hpp

**职责**: 定义自动跳跃系统使用的所有常量参数。

**内容**:

| 常量 | 值 | 说明 | MC 源码参考 |
|------|-----|------|-------------|
| `BASE_JUMP_HEIGHT` | 1.2f | 基础最大跳跃高度（方块数） | `f7 = 1.2F` |
| `JUMP_BOOST_PER_LEVEL` | 0.75f | 每级跳跃提升效果增加的高度 | `(amplifier + 1) * 0.75F` |
| `MIN_JUMP_HEIGHT` | 0.5f | 触发自动跳跃的最小障碍物高度 | `if (f14 <= 0.5F) return` |
| `FORWARD_THRESHOLD` | -0.15f | 向前移动判断阈值（点积） | `if (f13 < -0.15F) return` |
| `DETECTION_DISTANCE_MULTIPLIER` | 7.0f | 检测距离乘数 | `Math.max(f * 7.0F, ...)` |
| `DETECTION_HEIGHT_OFFSET` | 0.51f | 检测线起点高度偏移 | `getPosY() + 0.51D` |
| `AUTO_JUMP_COOLDOWN` | 1 | 跳跃冷却 ticks | `this.autoJumpTime = 1` |
| `MOVEMENT_THRESHOLD_SQ` | 0.001f | 移动检测阈值平方 | `f1 <= 0.001F` |
| `JUMP_FACTOR_THRESHOLD` | 1.0 | 跳跃因子阈值（蜂蜜块检测） | `getJumpFactor() >= 1.0D` |
| `LINE_OFFSET_RATIO` | 0.5f | 检测线偏移比例 | `f9 * 0.5F` |
| `HEAD_SPACE_CHECK_HEIGHT` | 2 | 头部空间检测高度（格） | 检查上方两格 |

---

### AutoJump.hpp

**职责**: 自动跳跃系统的核心接口定义。

**主要组件**:

#### `AutoJumpResult` 结构体

```cpp
struct AutoJumpResult {
    bool shouldJump = false;      // 是否应该触发自动跳跃
    f32 obstacleHeight = 0.0f;    // 检测到的障碍物高度
};
```

#### `AutoJump` 类

**配置方法**:
| 方法 | 说明 |
|------|------|
| `setEnabled(bool)` | 启用/禁用自动跳跃 |
| `isEnabled()` | 检查是否启用 |
| `setJumpBoostLevel(i32)` | 设置跳跃提升效果等级 |
| `jumpBoostLevel()` | 获取跳跃提升效果等级 |

**状态管理**:
| 方法 | 说明 |
|------|------|
| `autoJumpTime()` | 获取冷却计时器 |
| `tick()` | 每帧更新（递减冷却） |
| `resetCooldown()` | 重置冷却计时器 |

**核心检测**:
| 方法 | 说明 |
|------|------|
| `check(Player, PhysicsEngine, Vector2)` | 主检测方法，返回 `AutoJumpResult` |

**静态工具方法**:
| 方法 | 说明 |
|------|------|
| `calculateMovementDirection(Player, Vector2)` | 计算移动方向 |
| `isMovingForward(Vector3, Vector3)` | 检查是否向前移动 |
| `calculateMaxJumpHeight()` | 计算最大跳跃高度 |

---

### AutoJump.cpp

**职责**: 自动跳跃系统的完整实现。

**核心算法流程**:

```
check() 方法流程:
│
├── 1. 检查触发条件 (shouldCheckForAutoJump)
│   ├── 自动跳跃已启用
│   ├── 冷却已过 (autoJumpTime == 0)
│   ├── 玩家在地面 (onGround)
│   ├── 玩家未潜行 (!isSneaking)
│   ├── 玩家未骑乘 (!isRiding)
│   ├── 有移动输入
│   ├── 跳跃因子正常 (jumpFactor >= 1.0)
│   └── 未飞行 (!flying)
│
├── 2. 计算移动方向 (calculateMovementDirection)
│   └── 使用速度或移动输入+yaw
│
├── 3. 检查向前移动 (isMovingForward)
│   └── 点积 > FORWARD_THRESHOLD (-0.15)
│
├── 4. 检查头部空间 (hasHeadSpace)
│   └── 玩家眼睛上方两格无障碍物
│
├── 5. 计算检测距离
│   └── max(moveSpeed * 7.0, 1.0 / movementLength)
│
├── 6. 构建左右检测线
│   ├── 起点: 脚部 + 0.51 格高度
│   ├── 偏移: 玩家宽度的一半 × 垂直方向
│   └── 方向: 移动方向
│
├── 7. 收集检测区域碰撞箱
│
├── 8. 沿检测线查找障碍物 (detectObstacleHeight)
│   ├── 检查高度范围: [playerY + 0.5, playerY + maxJumpHeight]
│   ├── 检查检测线与碰撞箱的 XZ 投影相交
│   └── 检查玩家能否站在障碍物上
│
└── 9. 返回结果
    └── 如果找到合适障碍物，触发跳跃
```

---

## 模块概览

### 整体职责

自动跳跃模块实现了 Minecraft 1.16.5 中玩家走近可跳上的障碍物时自动触发跳跃的功能。这允许玩家无需手动按跳跃键就能自动攀登最多 1.2 格高的障碍物（含跳跃提升效果可更高）。

### 输入和输出

**输入**:
- 玩家实体状态（位置、速度、朝向、是否在地面、是否潜行等）
- 物理引擎（用于碰撞检测）
- 移动输入向量（forward, strafe）
- 跳跃提升效果等级

**输出**:
- `AutoJumpResult`: 包含是否应该跳跃以及检测到的障碍物高度

### 依赖项

| 依赖 | 用途 |
|------|------|
| `Player` | 获取玩家状态（位置、速度、朝向、能力等） |
| `PhysicsEngine` | 碰撞检测、收集碰撞箱 |
| `AxisAlignedBB` | 碰撞箱数据结构 |
| `Vector2/Vector3` | 向量计算 |
| `MathUtils` | 数学常量（DEG_TO_RAD） |

### 使用方法

```cpp
// 在 Player 类中
#include "entity/movement/AutoJump.hpp"

class Player : public Entity {
    // ...
private:
    entity::movement::AutoJump m_autoJump;
};

// 初始化时同步设置
void initPlayer(bool autoJumpEnabled) {
    m_autoJump.setEnabled(autoJumpEnabled);
}

// 每 tick 更新冷却
void Player::updatePhysics() {
    m_autoJump.tick();  // 递减冷却计时器
    
    // ... 物理更新 ...
    
    // 移动后检测自动跳跃
    if (m_autoJump.isEnabled() && !m_abilities.flying && m_onGround) {
        Vector2 movementInput(forward, strafe);
        auto result = m_autoJump.check(*this, *m_physicsEngine, movementInput);
        if (result.shouldJump) {
            jump();
        }
    }
}

// 跳跃提升效果变化时更新
void onJumpBoostEffectAdded(i32 level) {
    m_autoJump.setJumpBoostLevel(level);
}

void onJumpBoostEffectRemoved() {
    m_autoJump.setJumpBoostLevel(0);
}
```

### 容易踩的坑

1. **冷却计时器必须每帧更新**: 
   ```cpp
   // 错误：只在 tick() 中更新（客户端可能不调用 tick）
   void Player::tick() { m_autoJump.tick(); }
   
   // 正确：在物理更新中更新
   void Player::updatePhysics() { m_autoJump.tick(); }
   ```

2. **检测时机必须在移动后**: 
   - 自动跳跃检测需要在玩家移动后进行，否则无法正确判断障碍物
   - 应在 `moveWithCollision()` 后调用 `check()`

3. **移动方向计算**:
   - 当速度很小时使用移动输入 + yaw 计算方向
   - 不要直接使用移动输入作为方向

4. **跳跃提升效果等级**:
   - 等级从 0 开始（0 = 无效果）
   - MC 中的 amplifier 是 0-based，效果显示为 I 级时 amplifier=0

5. **飞行模式不触发自动跳跃**:
   - 检查时需要排除飞行状态
   - 潜行状态也不触发

6. **蜂蜜块检测**:
   - 当前 `getJumpFactor()` 在 Player 中默认返回 1.0
   - 实现蜂蜜块后需要修改此方法返回 0.5

### 涉及的测试用例

测试文件: `tests/common/entity/movement/AutoJumpTest.cpp`

| 测试套件 | 测试用例 | 说明 |
|----------|----------|------|
| AutoJumpTest | DefaultEnabled | 默认启用状态 |
| AutoJumpTest | CanEnableDisable | 启用/禁用切换 |
| AutoJumpTest | JumpBoostLevelAffectsMaxHeight | 跳跃提升等级影响最大高度 |
| AutoJumpTest | CooldownMechanics | 冷却机制 |
| AutoJumpTest | IsMovingForwardDetection | 向前移动检测 |
| AutoJumpTest | MaxJumpHeightCalculation | 最大跳跃高度计算 |
| AutoJumpConstantsTest | VerifyValues | 常量值验证 |
| AutoJumpTest | ZeroInputReturnsEmptyDirection | 零输入边界条件 |
| AutoJumpTest | CooldownPreventsMultipleJumps | 冷却防止多次跳跃 |
| AutoJumpTest | DisabledDoesNotTrigger | 禁用时不触发 |
| AutoJumpTest | ForwardThresholdEdgeCases | 方向阈值边界情况 |

**测试覆盖**:
- 配置测试：启用/禁用、跳跃提升等级
- 冷却机制：计时器递减、重置
- 方向计算：向前移动检测、边界情况
- 高度计算：跳跃提升效果影响
- 常量验证：确保与 MC 源码一致
- 边界条件：零输入、阈值边界

---

## 参考

### Minecraft 1.16.5 源码参考

| 类 | 方法 | 说明 |
|----|------|------|
| `ClientPlayerEntity` | `func_228356_eG_()` | 条件检查 |
| `ClientPlayerEntity` | `updateAutoJump()` | 核心检测算法 |

### 设计决策

1. **常量分离**: 将常量放入 `AutoJumpConstants` 命名空间，便于测试和修改
2. **静态工具方法**: `calculateMovementDirection`、`isMovingForward` 等方法设为静态，便于单元测试
3. **冷却机制**: 使用 1 tick 冷却防止连续触发
4. **双检测线设计**: 沿玩家左右边缘分别检测，提高检测精度
