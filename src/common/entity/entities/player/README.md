# 玩家实体模块

玩家实体模块负责封装玩家的移动、能力、背包、经验、脚步声和游泳声等核心状态，是客户端主循环和服务端玩家逻辑之间的共同抽象。

## 目录结构

```text
src/common/entity/entities/player/
├── ChatVisibility.hpp     # 聊天可见性枚举（全显示、仅系统、隐藏）
├── GameModeUtils.hpp      # 游戏模式能力映射工具
├── GameModeUtils.cpp      # 游戏模式能力映射实现
├── Player.hpp             # 玩家实体声明，包含状态、移动、权限等级和网络同步接口
├── Player.cpp             # 玩家实体实现，包含物理、脚步声、游泳声和序列化
├── PlayerModelPart.hpp    # 玩家皮肤部件位掩码（披风、夹克、袖子、裤腿、帽子）
├── SpawnLocationHelper.hpp # 重生点位置辅助工具
└── README.md              # 本文档
```

## 模块关系

- `Player` 继承自 `LivingEntity`，复用通用的位置、旋转、碰撞和数据管理能力。
- `Player` 在退出蹲伏、游泳和睡眠姿态时，会通过 `IWorld` 的碰撞查询判断当前空间是否允许切回站立。
- `ClientApplication` 使用 `Player` 的 `distanceWalkedModified` 等价累计值和 `cameraYaw/prevCameraYaw` 来驱动原版 `GameRenderer.applyBobbing()` 风格的视图矩阵变换，并读取脚步声/游泳声标志来播放本地音效。
- `NetworkClient` 和玩家序列化逻辑负责把服务器传来的传送、位置和状态同步到本地玩家。
- 服务端玩家管理由 `server/world/player/ServerPlayerEntityManager` 负责。
- 客户端本地玩家身份由 `client/world/player/LocalPlayerIdentity` 管理。
- `GameModeUtils` 负责把游戏模式转换为玩家能力，避免重复实现。
- `CooldownTracker`（位于 `entity/player/`）管理物品冷却，供 `Player` 持有。

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

- `entity/core/Entity.hpp`、`entity/core/LivingEntity.hpp` - 实体基类
- `entity/player/CooldownTracker.hpp` - 物品冷却追踪
- `entity/experience/ExperienceManager.hpp` - 经验管理
- `entity/inventory/PlayerInventory.hpp` - 玩家背包
- `entity/food/FoodStats.hpp` - 饥饿系统
- `entity/movement/AutoJump.hpp` - 自动跳跃
- `physics/PhysicsConstants.hpp` - 物理常量
- `world/IWorld.hpp` - 世界接口
- `world/block/BlockPos.hpp`、`world/block/BlockState.hpp` - 方块相关
- `network/packet/ProtocolPackets.hpp` - 网络同步

### 下游依赖（依赖本模块）

- `server/world/player/ServerPlayer.hpp` - 服务端玩家实体
- `client/world/player/LocalPlayer.hpp` - 客户端本地玩家
- `client/ClientApplication.hpp` - 客户端主循环
- `network/NetworkClient.hpp` - 网络同步
- 各种实体交互系统（攻击、物品使用等）

## 容易踩的坑

- **awardCustomStat 虚方法**：`Player::awardCustomStat()` 是虚方法，基类空实现（不会崩溃也不会更新统计）；仅 `ServerPlayer` 重写版本实际委托给 `StatisticsManager::incrementCustom()`。客户端调用安全但无效果。常量定义在 `common/stats/Stats.hpp`，与 `StatRegistry` 注册名必须完全一致。
- **步距统计位置误用**：不要把 `Entity::prevPosition()` 当成脚步声采样位置，它是插值/帧历史状态，不是步距累计基准。步距统计使用 `m_moveDistanceSamplePosition`。
- **传送后的步距重置**：不要在外部直接修改玩家位置后继续沿用旧的步距计数，传送和出生都应该调用 `Player::setPosition()` 来重置采样。
- **重复采样问题**：`updateMoveDistance()` 可以在同一帧里被多次调用，但每次都必须只统计"上次采样之后"的增量。
- **视野晃动与脚步声耦合**：视野晃动和脚步声共用同一套移动距离统计，统计语义错了会同时污染音效和镜头。
- **姿态切换碰撞检查**：从蹲下、游泳、睡眠切回站立时，不要直接强行改成 `Standing`；应保留 `Player::setSneaking()` / `Player::setSwimming()` / `Player::setSleeping()` 的碰撞检查结果，否则会在低顶方块下错误穿模。
- **声音事件链路**：玩家受伤和死亡声音已经接入通用实体声音链路，不要再在服务器侧手写单独广播分支。
- **视野晃动公式**：视野晃动的行走相位使用原版 `distanceWalkedModified = 水平实际位移 * 0.6`，不要再把未缩放的行走距离直接传给渲染层。
- **相机晃动条件**：`cameraYaw/prevCameraYaw` 是原版平滑晃动强度，只有站在地面、未死亡、未游泳时根据水平速度趋近，骑乘时应清零。
- **输入与物理分离**：`handleMovementInput()` 只缓存当前输入，不再直接修改速度；客户端必须由 `ClientApplication` 按 20TPS 调用 `updatePhysics()` 消费输入。测试或逻辑里调用 `handleMovementInput()` 后，需要执行一次 `updatePhysics()` 才会看到速度和位置变化。
- **能力同步来源**：能力同步以 `Player::abilities()` 为运行时事实来源；`PlayerAbilitiesPacket::fromPlayer()` 不会再根据 GameMode 重新推导，避免覆盖飞行状态或自定义 walk/fly speed。
- **挖掘速度公式**：最终挖掘速度 = 基础速度 × 效率附魔加成 × 急迫效果乘数 × 挖掘疲劳乘数 × 水下惩罚 × 空中惩罚。各乘数叠加顺序影响结果精度。
- **攻击冷却判定**：横扫攻击需要玩家"几乎静止"（`distanceWalkedModified - prevDistanceWalkedModified < aiMoveSpeed()`），否则不会触发横扫效果。
- **权限等级与游戏模式分离**：`m_permissionLevel`（0-4）独立于游戏模式存储，`setGameMode()` 会重置 `m_abilities` 但不会重置 `m_permissionLevel`。`canUseGameMasterBlocks()` 要求同时满足 `creativeMode` 和 `permissionLevel >= 2`。
- **权限等级网络同步**：服务端 `/op`/`/deop` 后会通过 `EntityStatusPacket`（status byte = 24 + level）通知客户端权限等级变更，客户端收到后在 `ClientApplicationNetwork` 的 `onEntityStatus` 回调中更新本地玩家的 `m_permissionLevel`。
