# 玩家实体、运行数据与生命周期

## 目录结构树

```text
player/
├── gamemode/  # 游戏模式与能力同步
├── lifecycle/  # 玩家记录、资源释放与退出通知
├── movement/  # 玩家位置和朝向更新
├── runtime/  # 玩家运行数据与连接关联
├── PlayerRespawnManager.cpp  # 模块接口与实现
├── PlayerRespawnManager.hpp  # 玩家重生管理器
├── ServerPlayer.cpp  # 模块接口与实现
└── ServerPlayer.hpp  # 服务端玩家实体。
```

## 内部模块关系

ServerPlayer 承接实体玩法；runtime 保存会话关联数据；lifecycle 管理在线记录与退出顺序；movement 更新位置；gamemode 管理模式及能力；重生流程由 PlayerRespawnManager 协调。

## 上下游外部依赖关系

应用层装配服务，网络处理器调用领域入口；世界玩家实体管理和区块同步分别处理实体可见性与订阅。

## 容易踩的坑

- PacketSender 只投递包，PlayerConnectionManager 关闭连接，PlayerManager 统一移除记录。
- 玩家移除前释放菜单、实体和区块资源，移除后通知剩余玩家；重复移除不得重复通知。
- 玩家移动与区块订阅分开，订阅统一使用 ChunkSyncManager 持有的 tracker。

### 1. ServerPlayer vs ServerPlayerData 的混淆

项目中存在两个玩家相关的类：

| 类 | 位置 | 职责 |
|----|------|------|
| `ServerPlayer` | `server/player/` | 实体类，游戏逻辑（物理、睡眠、传送、统计、成就） |
| `ServerPlayerData` | `server/core/` | 数据结构，网络同步（位置、心跳、传送确认） |

**注意**：当前两个类都在使用，职责不同，不要混淆。

### 2. 世界引用

实体通过 `world()` 获取 IWorld，服务端玩家通过 `getWorld()` 获取 ServerWorld；两者的 const 重载均返回只读世界指针。实体未绑定世界时指针可为空。

### 3. 网络连接检查

发送消息前应检查 `canReceiveMessages()` 或 `hasConnection()`：

```cpp
if (player->canReceiveMessages()) {
    player->sendSystemMessage("...");
}
```

### 4. sendStatusMessage 的 actionBar 参数

- `actionBar = true`：消息显示在 Action Bar（物品栏上方）
- `actionBar = false`：消息显示在聊天区域

### 5. 配方解锁触发成就

`unlockRecipe()` 会同时：
1. 触发 `RecipeUnlockedTrigger` 成就
2. 更新配方书（标记为新配方）

### 6. 重生点验证失败会自动清除

`determineRespawnPosition()` 验证重生点失败后会调用 `clearSpawnPoint()`，防止每次重生都检查无效重生点。

### 7. 维度传送的乘客处理

`changeDimension()` 会自动让玩家下骑乘并清除乘客，传送前需确保这是预期行为。

迁移后按目标坐标同步区块加载来源，包含未注册网络会话的模拟玩家；仅发送维度切换包不能保证目的地区块持续加载。

### 8. CHUNK_HEIGHT vs MAX_BUILD_HEIGHT

`CHUNK_HEIGHT = MAX_BUILD_HEIGHT - MIN_BUILD_HEIGHT`，当前分别为 384、320、-64。区块总高度与世界顶部坐标语义不同，不能互换。

### 9. PvP 保护调用链

`ServerPlayer::hurt()` → 检查伤害来源是否为玩家 → `attackingPlayer->canHarmPlayer(*this)` → 先检查 `IWorld::isPvpAllowed()`（读取 PVP 游戏规则）→ 再委托 `Player::canHarmPlayer()` 检查队伍友伤。驯服动物（如狼）通过 `TameableEntity::wantsToAttack()` 也调用 `canHarmPlayer()` 判断主人是否可攻击目标玩家。

**注意**：`canHarmPlayer(target)` 的调用约定是 this=攻击者, param=目标。在 `hurt()` 中的调用是 `attackingPlayer->canHarmPlayer(*this)`，即攻击者检查自己能否伤害被攻击者。

### 10. sendVelocityPacket 速度同步

`ServerPlayer::sendVelocityPacket()` 重写 Player 基类版本（返回 false），实际发送 `ir::play::SetEntityMotion`（旧 `EntityVelocityPacket` 已删除，统一走 IR）给玩家客户端并返回 true。用于两个场景：
1. `Player::causeExtraKnockback()` 中对 ServerPlayer 目标立即发送速度包，避免 EntityTracker::tick() 重复发送导致击退速度重复应用
2. `EntityTracker::tick()` 中对自身发送速度包（"AndSelf" 模式，ServerPlayer 不在自身追踪列表中）

### 11. 旁观者摄像机跟踪系统

旁观者模式下玩家可以通过 `/spectate <target>` 命令或攻击实体来跟踪目标实体视角。核心方法：

| 方法 | 说明 |
|------|------|
| `setCamera(Entity* target)` | 设置旁观目标，委托 `setCameraEntityId()` 触发 `onCameraEntityChanged()` |
| `resetCamera()` | 停止旁观，恢复自身视角（调用 `setCamera(nullptr)`） |
| `onCameraEntityChanged(oldId, newId)` | 重写 Player 基类虚方法：传送玩家到目标位置 + 发送 `ir::play::SetCamera` |
| `tickSpectator()` | 每 tick 同步旁观者位置到目标；目标消失或玩家潜行时自动停止 |
| `attack(Entity& target)` 重写 | 旁观者模式下攻击实体 = 设置旁观目标 |

**数据流**：
1. SpectateCommand / 旁观者攻击 → `setCameraEntityId()` → `onCameraEntityChanged()` → ServerPlayer 发送 `ir::play::SetCamera` + 传送到目标
2. `setGameMode()` 离开旁观模式时 → `setCameraEntityId(nullopt)` → `onCameraEntityChanged()` → ServerPlayer 发送 `ir::play::SetCamera`
3. `ServerPlayer::tick()` → `tickSpectator()` → 同步位置、检查有效性、潜行退出
4. 客户端收到 `ir::play::SetCamera` → 设置 `Player::m_cameraEntityId` → 渲染循环跟随目标实体

**注意**：
- `setCameraEntityId()` 内含相等性检查，值未变化时不触发 `onCameraEntityChanged()`，避免重复发包
- `Player::attack()` 基类中旁观者路径通过 `setCameraEntityId()` → `onCameraEntityChanged()` 虚方法自动触发 ServerPlayer 网络同步，无需手动发包
- `Player::tick()` 不包含旁观者位置同步逻辑，由 `ServerPlayer::tickSpectator()` 统一处理
- 客户端旁观目标眼高已通过 `ClientEntity::eyeHeight()` 接口实现，根据实体类型和姿态返回正确的眼高值

### 12. 单元测试

PvP 保护机制的单元测试位于：

| 测试文件 | 覆盖范围 |
|---------|---------|
| `tests/entity/PlayerCanHarmPlayerTest.cpp` | `Player::canHarmPlayer()` 队友友伤检查（10 个测试） |
| `tests/common/world/gamerule/PvpGameRuleTest.cpp` | PVP 游戏规则默认值、设置、重置、序列化（17 个测试） |
| `tests/server/player/ServerPlayerPvpTest.cpp` | `ServerPlayer::canHarmPlayer()` PvP 规则 + 队友友伤组合检查（14 个测试）、`ServerPlayer::hurt()` PvP 拦截检查（12 个测试） |
| `tests/common/entity/player/PlayerSpectatorTest.cpp` | Player 旁观者模式状态、noclip、camera 清除、旁观攻击（13 个测试） |

> 旧的 `tests/network/SetCameraPacketTest.cpp` 随 `SetCameraPacket` 类删除已移除；SetCamera 现走 IR `ir::play::SetCamera`，序列化由 IR codec 统一覆盖。

### 13. 客户端 tick 运动量（knownMovement）由 client_tick_end 收口

`ServerPlayer::m_lastKnownClientMovement` 是"客户端上报的每 tick 位移"，只由两条路径写入：
`MovementHandler::handlePlayerMovePacket`（玩家移动包被采纳后）与 `handleMoveVehiclePacket`（自控载具位移），
两者都经 `recordClientMovement()`；收口只发生在 `MovementHandler::handleClientTickEndPacket` → `endClientTick()`。

**坑**：不要在 `ServerPlayer::tick()` 里清零该字段。客户端站着不动时**不发**移动包，服务端要等 `client_tick_end`
才知道"本 tick 没有上报"，此时才清零；若在服务端 tick 里盲目清零，会把"玩家本 tick 的移动"提前抹掉。
被反飞行闸拒绝（回弹）的移动包不记账，否则会把外挂位移当成合法运动量。

**用途**：横扫攻击的"几乎静止"判定（`Player::isSweepStationary()`）。该判定必须用客户端上报位移而非服务端自身
速度——玩家位置由客户端权威申报，服务端速度不含其行走。无客户端连接的实体（`SimulatedPlayer`）覆写
`getKnownMovement()` 回退到自身速度。

服务器链接和对话框通过 ServerPlayer 的 sendServerLinks/showDialog/clearDialog 发送；对话框可用命名资源或直接定义，整数 NBT 标签须保持 Int/Long。

### 玩家世界引用

ServerPlayer 的 typed 世界视图与 Entity::world 共用同一指针，跨维度更新必须立即体现在脚本、实体行为及服务端访问中。禁止额外缓存同名世界成员造成状态分裂。
