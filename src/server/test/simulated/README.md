# simulated/ — 原生 SimulatedPlayer

GameTest 框架的模拟玩家实现，`ServerPlayer` 子类（对齐基岩 `SimulatedPlayer`）。作为 `GameTestHelper::spawnSimulatedPlayer` 的返回值对外可见，但其头仅供 `facade/`/`simulated/`/`tests/` 内部使用。

## 目录结构

```
simulated/
└── SimulatedPlayer.hpp/.cpp   # ServerPlayer 子类 + GameTestHelper 回指 + 最小可用集 + TODO stub
```

## 内部模块关系

- `SimulatedPlayer : public mc::ServerPlayer`（`mc` 命名空间，`ServerPlayer` 在 `mc` 顶层非 `mc::server`）。
- 持非拥有 `GameTestHelper* m_helper` 回指：`spawn` 时绑定，用于 `worldBlockPosition` 把结构相对坐标转世界绝对坐标、`world()` 取 `ServerWorld&`。
- 静态工厂 `spawn(helper, name, relativePos, gameMode)` 封装构造→注入→`ServerWorld::spawnEntity` 全流程，所有权归 `EntityManager`，调用方持裸指针。
- `moveToLocation`/`lookAtLocation`/`lookAtEntity` 用 `setRotation` + `handleMovementInput` 手动驱动（Player 非 Mob，无 `MobEntity::lookAt`/`navigator()`）；`chat` 经 `ServerWorld::executeCommand`。

## 上下游外部依赖关系

**上游（本目录依赖）**：
- `mc::ServerPlayer`/`mc::Player`/`mc::Entity`（`server/player/`、`common/entity/`）：继承链、`setServer`/`setWorld`/`setConnection`/`setPlayerId`/`setPosition`/`setGameMode`/`handleMovementInput`/`setRotation`/`setYHeadRot`/`discard`/`respawn`。
- `facade/GameTestHelper`：`world()`/`worldBlockPosition()`。
- `mc::server::ServerWorld`：`spawnEntity`/`executeCommand`/`server()`。
- `mc::math::toDegrees`（`common/util/math/MathUtils.hpp`）。

**下游（依赖本目录）**：
- `facade/GameTestHelper.cpp`：`spawnSimulatedPlayer` 调 `SimulatedPlayer::spawn`，`removeSimulatedPlayer` 调 `discard`。
- `tests/test_simulated_player.cpp`（1I）：经 `GameTestHelper::spawnSimulatedPlayer` 取实例，断言 `moveToLocation`/`lookAt*` 行为。

## 容易踩的坑

1. **Player 不是 MobEntity**——继承链 `ServerPlayer→Player→LivingEntity→Entity`，`MobEntity` 是 `LivingEntity` 的兄弟分支。`MobEntity::lookAt`/`navigator()`/AI goal 体系对 `SimulatedPlayer` 不可用。`moveToLocation` 当前是 `handleMovementInput(1,0,false,false)` 直线驱动，**不绕障**，遇墙卡住（TODO: 接 `PathNavigator` 适配非 Mob 拥有者实现真实寻路）。
2. **`ServerPlayer` 在 `mc` 顶层命名空间**（非 `mc::server`）。`StatisticsManager.hpp` 有 `mc::server::ServerPlayer` 前向声明是历史遮蔽坑（见内存 `network-fwd-decl-namespace-shadow`），勿混淆；本类继承 `mc::ServerPlayer`。
3. **`setConnection(nullptr)` 安全**——`hasConnection()` 判 `!= nullptr && isConnected()`，所有发包路径（`sendChatMessage`/`sendSystemMessage`/`sendStatusMessage`/IR 包）在 null 连接下 no-op，无头模拟玩家无副作用。
4. **构造后必须依次注入再 spawnEntity**——`setPlayerId(0)`/`setPosition`/`setServer(world.server())`/`setWorld(&world)`/`setConnection(nullptr)`/`setGameMode`，顺序错则 `tick()` 内访问空 `m_server`/`m_world` 崩。`spawn()` 工厂已封装此序列，外部不应直接构造。
5. **`spawnEntity` 返回 `INVALID_ENTITY_ID=0` 表失败**——此时 `unique_ptr` 已被接管并销毁，`raw` 悬垂，须立即返 `nullptr`，勿再访问。
6. **MC yaw 坐标系**：yaw=0→+Z，yaw=90→-X，故朝目标 `(dx,dz)` 的 yaw = `toDegrees(atan2(-dx, dz))`；pitch 正值向下看，`pitch = -toDegrees(atan2(dy, horizDist))`（dy>0 目标在上方应得负 pitch）。算错则玩家背向目标。
7. **`respawn()` 不传送**——`Player::respawn` 非虚仅重置生命/饥饿/经验，完整流程需先 `determineRespawnPosition`（TODO: 接 `ServerPlayer` 重生位置确定）。当前阶段 `respawn` 只清状态，不动位置。
8. **`tick()` 透传**——`SimulatedPlayer` 不重写 `tick()`，复用 `ServerPlayer::tick`（含 `inventory().tick()` 地图上色等），保证背包/成就/同步逻辑正常。
9. **回指生命周期**——`m_helper` 非拥有，`GameTestHelper` 由 `BaseGameTestInstance` 拥有，instance 由 batch runner 拥有，helper 生命周期 ≥ SimulatedPlayer（SimulatedPlayer 由 `EntityManager` 持有，测试结束 `killAllEntities` 或 `discard` 回收，先于 instance 析构）。无需 `MC_ASSERT` 守 null（`spawn` 必绑定），但便捷方法入口断言 `m_helper != nullptr` 以防误用未 spawn 的实例。
