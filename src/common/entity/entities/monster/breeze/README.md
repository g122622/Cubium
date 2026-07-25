# 旋风人 (Breeze)

MC 1.21 新增的敌对生物，在试炼密室中生成。

## 目录结构

```
breeze/
├── BreezeEntity.hpp/cpp   # 旋风人实体（风弹攻击、滑行、长跳、动画状态机）
└── README.md
```

## TODO 待办

- **TODO(client_renderer)**: 旋风人客户端模型与渲染器尚未实现。
  服务端 `BreezeEntity` 已维护 `m_idleAnim`/`m_slideAnim`/`m_slideBackAnim`/
  `m_longJumpAnim`/`m_shootAnim`/`m_inhaleAnim` 六个 `AnimationState` 字段
  （镜像 MC 1.21.11 `Breeze.java`），但 Cubium 的客户端 `ClientEntity` 是与
  服务端实体分离的独立类，目前尚无 `BreezeModel`/`BreezeRenderer` 读取这些
  动画状态。未来实现客户端渲染器时，需要：
  1. 在 `ClientEntity` 中根据同步的 Pose 启动对应的 `AnimationState`
     （参考 MC `Breeze.onSyncedDataUpdated` 与 `resetAnimations`）
  2. `BreezeModel` 根据各 `AnimationState` 的 `startTick` 计算动画进度
  3. 客户端渲染器通过 `idleAnimation()`/`slideAnimation()`/等访问器读取动画状态
  在客户端渲染器实现之前，这些 `AnimationState` 字段暂时没有渲染侧消费者，
  但服务端会持续维护它们，确保未来渲染器实现后可直接接入。

## 内部模块关系

```
MonsterEntity (敌对生物基类)
  └── BreezeEntity — 旋风人
        ├── tick() — 每 tick 根据 Pose 发射粒子、推进动画状态、播放呼啸音效
        ├── shootWindCharge() — 发射风弹
        ├── deflection() — 偏转投射物（重写 Entity::deflection，风弹除外，播放偏转音效）
        ├── die() — 死亡掉落狂风杖（仅被玩家击杀时，1-2个，受抢夺附魔影响）
        ├── updateSlideBackAnimation() — 滑行→站立过渡时触发 slideBack 动画
        ├── emitGroundParticles(count) — 在脚下生成 BLOCK 粒子（携带方块状态）
        ├── emitJumpTrailParticles() — 长跳轨迹粒子（前 5 tick 每 tick 3 个）
        ├── playWhirlSound() — 随机呼啸音效（音量/音调带扰动）
        └── AI 行为目标（注册在 registerGoals()）
```

## 动画状态机

旋风人的客户端动画基于 `EntityPose` 驱动，服务端通过 `setPose()` 切换姿态，
客户端在 Pose 同步后启动对应的 `AnimationState`。Pose 转换由各 AI Goal 触发：

```
              ┌──────────┐
        ┌────→│ Standing │←──────────────────────────────┐
        │     └──────────┘                                │
        │         │                                       │
        │         │ BreezeShootGoal.start                 │
        │         ↓                                       │
        │     ┌──────────┐                                │
        │     │ Shooting │── BreezeShootGoal.resetTask ───┘
        │     └──────────┘
        │
        │         │ BreezeLongJumpGoal.start
        │         ↓
        │     ┌──────────┐
        │     │ Inhaling │── isFinishedInhaling(成功) ──┐
        │     └──────────┘                              │
        │         │                                     ↓
        │         │ isFinishedInhaling(失败/无向量)  ┌────────────┐
        │         ↓                                  │ LongJumping│
        │     ┌──────────┐                          └────────────┘
        │     │ Standing │── isFinishedJumping(着陆)──┘
        │     └──────────┘
        │
        │         │ BreezeSlideGoal.start
        │         ↓
        │     ┌──────────┐
        └────│ Sliding  │── BreezeSlideGoal.resetTask
              └──────────┘
```

各 Goal 中的 Pose 转换（参考 MC 1.21.11 `Shoot.java` / `LongJump.java` / `BreezeAi.SlideToTargetSink`）：

| Goal | 方法 | Pose 转换 |
|---|---|---|
| BreezeShootGoal | startExecuting | Standing → Shooting |
| BreezeShootGoal | resetTask | Shooting → Standing（带条件守卫） |
| BreezeLongJumpGoal | startExecuting | * → Inhaling |
| BreezeLongJumpGoal | tick（吸气完成且有跳跃向量） | Inhaling → LongJumping |
| BreezeLongJumpGoal | tick（吸气完成但无跳跃向量） | Inhaling → Standing |
| BreezeLongJumpGoal | tick（着陆） | LongJumping → Standing |
| BreezeLongJumpGoal | resetTask | LongJumping/Inhaling → Standing（带条件守卫） |
| BreezeSlideGoal | startExecuting | * → Sliding |
| BreezeSlideGoal | resetTask | Sliding → Standing |

`tick()` 中根据当前 Pose 发射粒子并推进动画：

| Pose | 粒子 | 动画 |
|---|---|---|
| Standing / Shooting / Inhaling | 地面粒子 1 + nextInt(1) 个 | idle.startIfStopped |
| Sliding | 地面粒子 20 个 | idle.startIfStopped；离开 Sliding 时启动 slideBack |
| LongJumping | 跳跃轨迹粒子（前 5 tick 每 tick 3 个） | longJump.startIfStopped |

AnimationState 字段说明：

| 字段 | 含义 | 触发时机 |
|---|---|---|
| m_idleAnim | 空闲动画 | 每 tick `startIfStopped` |
| m_slideAnim | 滑行动画 | Pose 切换到 Sliding 时由客户端启动 |
| m_slideBackAnim | 滑行回弹动画 | Pose 离开 Sliding 且 slide 已启动时启动 |
| m_longJumpAnim | 长跳动画 | Pose 为 LongJumping 时由 tick 启动 |
| m_shootAnim | 射击动画 | Pose 切换到 Shooting 时由客户端启动 |
| m_inhaleAnim | 吸气动画 | Pose 切换到 Inhaling 时由客户端启动 |

> 注意：Cubium 架构中服务端 `BreezeEntity` 与客户端 `ClientEntity` 是分离的两个类
> （与 MC 单类设计不同）。服务端 `BreezeEntity` 维护 `AnimationState` 字段用于
> 镜像 MC 设计，并为未来客户端渲染器查询做好准备；客户端的 Pose 同步与动画
> 启动由 `ClientEntity` 在收到 EntityDataManager 更新时处理。

## 上下游外部依赖关系

**依赖本模块的地方：**
- `VanillaEntities::registerAll()` — 注册旋风人实体类型
- `VanillaEntityTypeKeys` — 旋风人实体类型指针缓存

**本模块依赖：**
- `MonsterEntity` — 敌对生物基类
- `WindChargeEntity` — 风弹弹射物实体
- `ProjectileEntity` — 弹射物基类（deflection 参数类型）
- `ProjectileDeflection` — 弹射物偏转类型枚举
- `AnimationState` — 动画状态机工具类
- `EntityPose` — 实体姿态枚举（新增 Sliding/Shooting/Inhaling/LongJumping）
- `SoundEvents` / `SoundCategory` — 音效播放
- `IWorld` — 世界接口（spawnEntity、playSound、addBlockParticle）
- `Items::BREEZE_ROD` — 狂风杖物品（死亡掉落）
- `ItemDropHelper` — 物品掉落工具
- `EnchantmentHelper` — 抢夺附魔查询
- `Player` — 玩家实体（判断击杀者、获取武器附魔）
- `ParticleTypes` / `ParticleTypeId` — 粒子类型
- `BlockState` — 方块状态（粒子携带）

## 容易踩的坑

1. **旋风人发射位置偏移**：`shootWindCharge()` 中发射 Y 坐标为 `y() + height() * 0.5f + 0.3f`（身体中心偏上 0.3 格），对齐 MC 原版 `Breeze.getFiringYPosition()`。

2. **目标 Y 瞄准点按骑乘状态区分（partialY 0.3 / 0.8）**：`shootWindCharge()` 中目标 Y 坐标对齐 MC 1.21.11 `Shoot.tick()`：
   ```
   d1 = livingentity.getY(livingentity.isPassenger() ? 0.8 : 0.3) - breeze.getFiringYPosition();
   ```
   - 非骑乘目标：`target->getY(0.3)`（瞄准躯干下部），补偿风弹无重力补偿的抛物线下坠，避免瞄准过高。
   - 骑乘目标：`target->getY(0.8)`（瞄准接近头部），避开载具碰撞盒遮挡，确保风弹不会命中载具而非乘客。
   - 项目中 MC 的 `isPassenger()` 对应 `isRiding()`（本实体正在骑乘其他实体，无参版本），**不是** `isPassenger(EntityId)`（判断指定实体是否为本实体的乘客）。
   - `getY(partialY)` 由 `Entity` 基类提供（= `position.y + height * partialY`），见 `entity/core/README.md` 的"位置与高度偏移访问器"小节。

3. **风弹散布按难度计算**：`shootWindCharge()` 中风弹散布（inaccuracy）由 `DifficultyHelper::getBreezeWindChargeInaccuracy(difficulty)` 计算，对应 MC 1.21.11 `Shoot.tick()` 公式 `5 - difficulty.getId() * 4`。各难度散布值：Peaceful=5、Easy=1、Normal=-3、Hard=-7。Normal/Hard 为负数，由 `ProjectileEntity::shoot` 内部取 `std::abs` 处理，散布效果等效于 3/7。注意此公式与弓/弩散布公式（`14 - id*4`）不同，不可混用。

4. **风弹不被偏转**：`deflection()` 对 `WindChargeEntity` 返回 `ProjectileDeflection::None`，其他投射物返回 `ProjectileDeflection::Reverse`（前提是实体类型属于 `DEFLECTS_PROJECTILES` 标签）。这确保旋风人不会偏转自己发射的风弹。偏转时播放 `ENTITY_BREEZE_DEFLECT` 音效。

5. **AI 行为目标**：`registerGoals()` 中已实现旋风人特有的四个 AI 目标：
   - `BreezeShootGoal`（优先级2）：向目标投掷风弹，充能15 ticks后发射，恢复4 ticks，冷却10 ticks
   - `BreezeLongJumpGoal`（优先级3）：长跳移动，吸气10 ticks后跳跃，着陆后设置射击许可
   - `BreezeShootWhenStuckGoal`（优先级4）：卡住时（水中/骑乘/飘浮）紧急射击
   - `BreezeSlideGoal`（优先级5）：地面滑行移动，内圈逃跑或中圈/目标身后移动，结束后设置射击许可
   - `shootWindCharge()` 方法由 BreezeShootGoal 调用触发

6. **Pose 转换必须带条件守卫**：`resetTask` 中切换回 `Standing` 前必须检查当前 Pose 是否仍为该 Goal 拥有的姿态（如 `Shooting`/`LongJumping`/`Inhaling`/`Sliding`），避免误覆盖其他 Goal 设置的 Pose。这与 MC 原版 `Shoot.stop`、`LongJump.stop` 的实现保持一致。

7. **粒子携带方块状态**：`emitGroundParticles` 和 `emitJumpTrailParticles` 都发射 `ParticleTypeId::Block` 类型粒子，需要携带方块状态用于纹理渲染。这通过 `IWorld::addBlockParticle` → `ServerWorld` 广播回调 → `MinecraftServer::broadcastBlockParticleInRange` → 构造 `ir::play::LevelParticles` → `ClientPlayVisitor` → `ClientApplicationNetwork` 调用 `BlockRegistry::getBlockState(stateId)` 还原 → 客户端世界 `addBlockParticle` 的链路完成。

8. **呼啸音效随机间隔**：`m_soundTick == 0` 时触发并重新随机化（1-80 ticks），否则每 tick 递减。音量 = `0.8 + 0.2 * nextFloat`，音调 = `0.7 + 0.4 * nextFloat`。

9. **长跳轨迹粒子持续 5 tick**：`m_jumpTrailStartedTick` 从 0 开始，进入 `LongJumping` Pose 后每 tick 自增并发射 3 个粒子，超过 `JUMP_TRAIL_DURATION_TICKS`（5）后停止。Pose 切换回非 `LongJumping` 时由 `resetJumpTrail()` 重置计数器。

