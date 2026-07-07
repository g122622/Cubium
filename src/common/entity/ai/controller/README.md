# AI 控制器模块 (controller)

本模块实现了 Minecraft 生物 AI 的底层控制器系统，负责管理实体的视线、移动和跳跃行为。

## 目录结构

```
controller/
├── LookController.hpp/cpp          # 视线控制器，控制实体头部旋转
├── MovementController.hpp/cpp      # 移动控制器，控制地面移动行为
├── JumpController.hpp/cpp          # 跳跃控制器，管理跳跃状态（支持多态派生）
├── FlyingMovementController.hpp/cpp # 通用飞行控制器，三维飞行移动（凋灵、鹦鹉等）
├── VexMovementController.hpp/cpp   # 恼鬼飞行控制器，穿墙飞行
├── GhastMovementController.hpp/cpp # 恶魂飞行控制器，带碰撞检测飞行
├── PhantomMovementController.hpp/cpp # 幻翼飞行控制器，直接操控速度向量的飞行
├── PhantomLookController.hpp       # 幻翼视线控制器（空操作，朝向由移动控制器控制）
├── DrownedMoveControl.hpp/cpp      # 溺尸两栖控制器，水中游泳+陆地行走
├── RabbitJumpControl.hpp/cpp       # 兔子跳跃控制器，canJump/wantJump 状态机
├── RabbitMoveControl.hpp/cpp       # 兔子移动控制器，控制跳跃速度与地面静止
└── README.md
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                      MobEntity (生物实体)                    │
│   ┌─────────────┐  ┌──────────────────┐  ┌─────────────┐   │
│   │LookController│  │MovementController│  │JumpController│  │
│   └──────┬──────┘  └────────┬─────────┘  └──────┬──────┘   │
│          │                  │ (触发跳跃)          │          │
│          └──────────────────┼────────────────────┘          │
│                             ▼                                │
│                    ┌─────────────────┐                       │
│                    │  PathNavigator  │ (提供移动路径)        │
│                    └─────────────────┘                       │
└─────────────────────────────────────────────────────────────┘

继承关系：
- FlyingMovementController ──继承──> MovementController
- VexMovementController ──继承──> MovementController
- GhastMovementController ──继承──> MovementController
- PhantomMovementController ──继承──> MovementController
- DrownedMoveControl ──继承──> MovementController
- RabbitMoveControl ──继承──> MovementController
- RabbitJumpControl ──继承──> JumpController
- PhantomLookController ──继承──> LookController
```

**控制器调用顺序**（在 MobEntity::tick() 中）：
1. GoalSelector.tick() - AI 决策，设置控制器目标
2. LookController.tick() - 更新头部旋转
3. MovementController.tick() - 更新移动方向
4. JumpController.tick() - 应用跳跃状态
5. aiStep() - 执行物理移动

## 上下游外部依赖关系

### 上游依赖（本模块使用）
- `mc::entity::MobEntity` - 生物实体基类
- `mc::entity::LivingEntity` - 生物实体基类（属性访问）
- `mc::entity::attribute::Attributes` - 属性系统（移动速度等）
- `mc::util::math` - 数学工具（角度计算、clamp 等）

### 下游依赖（使用本模块）
- `MobEntity` - 持有并管理控制器实例
- AI Goal 系统 - 通过控制器执行行为：
  - `LookAtGoal` / `LookRandomlyGoal` → LookController
  - `RandomWalkingGoal` / `MeleeAttackGoal` / `TemptGoal` → MovementController
  - `SwimGoal` → JumpController
  - `VexEntity` → VexMovementController
  - `GhastEntity` → GhastMovementController
  - `WitherEntity` → FlyingMovementController(this, 10, false)
  - `PhantomEntity` → PhantomMovementController + PhantomLookController（空操作）
  - `DrownedEntity` → DrownedMoveControl
  - `RabbitEntity` → RabbitJumpControl + RabbitMoveControl
- `PathNavigator` - 通过 MovementController 执行路径移动

## 容易踩的坑

### 1. 控制器 tick() 调用顺序

控制器的 `tick()` 调用顺序影响行为。正确顺序：
1. GoalSelector.tick() - AI 决策先执行，设置目标
2. 各控制器 tick() - 执行目标
3. aiStep() - 物理移动

如果顺序错误，可能导致 AI 目标设置的目标位置未被正确执行。

### 2. JumpController 跳跃状态重置

`JumpController::tick()` 会自动重置跳跃状态。每次跳跃请求只在一帧内有效，AI Goal 需要在跳跃完成前持续请求：

```cpp
// 错误：只请求一次
void SwimGoal::startExecuting() {
    m_mob->jumpController()->setJumping();
}

// 正确：持续请求直到跳出水面
void SwimGoal::tick() {
    if (m_mob->isInWater()) {
        m_mob->jumpController()->setJumping();
    }
}
```

### 3. 旋转速度限制

过大的旋转速度会导致实体瞬移视角。建议值：
- 默认转头速度：`10.0f`（度/tick）
- 快速转头：`30.0f`（度/tick）
- 缓慢转头：`5.0f`（度/tick）

### 4. MoveAction::Jumping 状态转换

`MoveAction::Jumping` 状态需要正确转换回 `MoveTo`。当前实现：着陆后（onGround）才转换回 MoveTo 状态。

### 5. 空指针检查

控制器可能为 `nullptr`（实体未正确初始化）。始终检查控制器指针：
```cpp
if (auto* ctrl = m_mob->lookController()) {
    ctrl->setLookPosition(x, y, z);
}
```

### 6. FlyingMovementController 通用飞行控制器

与 VexMovementController 和 GhastMovementController 不同，FlyingMovementController 是通用飞行控制器：
- 通过 `setMoveTo()` 接收目标坐标后，设置实体的朝向和移动输入
- 支持俯仰角（pitch）旋转，每tick最大旋转角度可配置（`maxTurn`）
- 空闲时可选保持无重力悬停或恢复重力（`hoversInPlace`）
- 使用 FLYING_SPEED 属性（飞行时）或 MOVEMENT_SPEED 属性（地面时）作为速度
- 凋灵使用 `FlyingMovementController(this, 10, false)`：俯仰角每tick最大旋转10度，空闲时恢复重力缓慢下落

### 7. VexMovementController vs GhastMovementController

两者都是飞行控制器，但有关键区别：
- **VexMovementController**：可穿墙飞行，无碰撞检测
- **GhastMovementController**：有碰撞检测，路径不安全时停止移动

使用场景：VexEntity 使用前者，GhastEntity 使用后者。

### 8. PhantomMovementController 幻翼飞行控制器

幻翼使用专用的飞行移动控制器，直接操控速度向量：
- 水平碰撞时自动180度转向并降低速度
- 根据 orbitOffset（移动目标点）计算目标方向
- 平滑调整偏航角（4度/tick），接近目标方向时加速到1.8，远离时减速到0.2
- 直接设置俯仰角为飞行方向
- 使用20%惯性混合（速度混合比例0.2）
- 不使用 setMoveForward/setAIMoveSpeed，直接修改 velocity

PhantomLookController 是空操作（tick() 无实现），因为幻翼的朝向完全由 PhantomMovementController 控制。

### 8. 身体/头部朝向字段（LivingEntity 通用机制）

LivingEntity 提供通用的身体/头部朝向字段与虚方法，所有生物实体子类自动继承：
- `m_renderYawOffset`（对应 MC `yBodyRot`）— 身体旋转偏移，通过 `renderYawOffset()` / `setRenderYawOffset()` 或虚方法 `setYBodyRot()` 访问
- `m_rotationYawHead`（对应 MC `yHeadRot`）— 头部旋转，通过 `rotationYawHead()` / `setRotationYawHead()` 或虚方法 `setYHeadRot()` 访问
- `Entity` 基类的 `setYBodyRot` / `setYHeadRot` 为空实现，`LivingEntity` 重写后写入上述字段

LookController 在每 tick 中通过 `setRotationYawHead()` 更新头部朝向到当前关注目标；身体朝向（`m_renderYawOffset`）则由结构模板放置（`Template::placeInWorld`）、NBT 加载（`Entity::readFromNBT`）、方块实体生成（如 `CopperGolemStatueBlockEntity::removeStatue`）等场景通过 `setYBodyRot()` 设置首帧值。详见 `src/common/entity/core/README.md` 中 "setYBodyRot / setYHeadRot 虚方法" 章节。

### 9. MovementController 到达检测

地面移动控制器使用**水平距离**检测到达（忽略 Y 轴），阈值为 0.5 格。
飞行控制器使用 **3D 距离**检测到达，阈值为碰撞箱平均边长。

### 8. MovementController 不直接修改 velocity

地面移动控制器通过设置 `moveForward`/`moveStrafe` 间接影响移动，由 `LivingEntity::aiStep()` 实际执行物理移动。飞行控制器才直接修改 velocity。

### 9. LookController 的 shouldResetPitch()

默认实现返回 true，俯仰角会在某些情况下重置。子类可重写此方法改变行为。

### 10. DrownedMoveControl 两栖移动

溺尸使用 `DrownedMoveControl` 实现水中/陆地双模式移动：
- **水中模式**（`wantsToSwim() && isInWater()`）：直接修改 velocity 实现三维游泳移动，Y 方向使用 0.1 缩放因子（比水平方向 0.005 大 20 倍），提供足够垂直推力；旋转速度限制 90°/tick；身体朝向与头部一致
- **陆地模式**（不在水中或不想游泳）：添加微小重力（-0.008），委托基类 `MovementController::tick()` 处理地面行走
- `searchingForLand` 标志由 `DrownedSwimUpGoal` 设置、`DrownedGoToBeachGoal` 清除，控制是否施加额外向上推力

### 11. RabbitJumpControl / RabbitMoveControl 兔子专属控制器

兔子使用专属的跳跃和移动控制器实现 MC 1.21.11 的跳跃移动行为：

**RabbitJumpControl**（继承 JumpController）：
- 维护 `canJump`/`wantJump` 状态机：`wantJump` 由 AI Goal 通过 `setJumping()` 设置，`canJump` 由 `RabbitEntity::updateAITasks()` 通过 `enableJumpControl()`/`disableJumpControl()` 控制
- `tick()` 仅在 `wantJump` 为 true 时调用 `rabbit.startJumping()` 启动跳跃动画，然后清除标志
- 与通用 `JumpController` 的差异：不在 `wantJump=false` 时调用 `setJumping(false)`，动画结束由 `RabbitEntity::aiStep()` 控制
- 着陆延迟期间 `canJump=false`，由 `checkLandingDelay()` → `disableJumpControl()` 设置

**RabbitMoveControl**（继承 MovementController）：
- `tick()` 在地面且未跳跃且未请求跳跃时设置速度为 0（避免地面滑行）
- 有移动目标或处于 Jumping 状态时应用 `nextJumpSpeed` 作为移动速度
- `setMoveTo()` 在水中时将速度倍率提升至 1.5（对应 MC `setWantedPosition` 水中逻辑）
- `nextJumpSpeed` 仅在 `speed > 0` 时更新，保留之前的值

**JumpController 多态支持**：
- `JumpController` 基类提供 `virtual ~JumpController() = default`、`virtual void setJumping()` 和 `virtual void tick()` 支持派生
- `m_isJumping` 字段为 `protected`，允许派生类直接读写以实现自定义状态机
- 基类 `tick()` 调用 `m_mob->setJumping(m_isJumping)` 后清零；派生类可完全重写
