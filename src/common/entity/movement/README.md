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

## 内部模块关系

```
┌─────────────────────┐
│ AutoJumpConstants   │  ← 常量定义
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│     AutoJump        │  ← 核心检测逻辑
│  - check()          │    主入口，返回 AutoJumpResult
│  - tick()           │    冷却更新
│  - 静态工具方法      │
└─────────────────────┘
```

`AutoJump` 类是核心组件，负责检测是否应该触发自动跳跃。`AutoJumpConstants` 命名空间定义所有常量参数，便于测试和修改。

## 上下游外部依赖关系

### 上游依赖（本模块依赖的）

| 依赖 | 用途 |
|------|------|
| `Player` | 获取玩家状态（位置、速度、朝向、能力等） |
| `PhysicsEngine` | 碰撞检测、收集碰撞箱 |
| `AxisAlignedBB` | 碰撞箱数据结构 |
| `Vector2/Vector3` | 向量计算 |
| `MathUtils` | 数学常量（DEG_TO_RAD） |

### 下游依赖（依赖本模块的）

| 模块 | 用途 |
|------|------|
| `Player` 类 | 持有 `AutoJump` 实例，在物理更新后调用检测 |

## 容易踩的坑

### 1. 冷却计时器必须每帧更新

```cpp
// 错误：只在 tick() 中更新（客户端可能不调用 tick）
void Player::tick() { m_autoJump.tick(); }

// 正确：在物理更新中更新
void Player::updatePhysics() { m_autoJump.tick(); }
```

### 2. 检测时机必须在移动后

自动跳跃检测需要在玩家移动后进行，否则无法正确判断障碍物。应在 `moveWithCollision()` 后调用 `check()`。

### 3. 移动方向计算

当速度很小时使用移动输入 + yaw 计算方向，不要直接使用移动输入作为方向。

### 4. 跳跃提升效果等级

等级从 0 开始（0 = 无效果）。MC 中的 amplifier 是 0-based，效果显示为 I 级时 amplifier=0。

### 5. 飞行模式不触发自动跳跃

检查时需要排除飞行状态，潜行状态也不触发。

### 6. 蜂蜜块检测

蜂蜜块会让 `getJumpFactor()` 返回 0.5，`AutoJump::shouldCheckForAutoJump()` 中检测跳跃因子 < 1.0 时禁用自动跳跃。当前 `Player::getJumpFactor()` 默认返回 1.0，待蜂蜜块实现后需修改。
