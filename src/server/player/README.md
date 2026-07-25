# Server Player Module

服务端玩家实体模块，定义服务端专用的玩家实体类。

## 目录结构

```
src/server/player/
├── ServerPlayer.hpp    # 服务端玩家实体头文件（网络同步、睡眠、传送、统计、成就）
└── ServerPlayer.cpp    # 服务端玩家实体实现
```

## 内部模块关系

ServerPlayer 继承自 `Player` 基类，是服务端玩家实体的核心实现：

- **网络通信**：消息发送（聊天、系统、状态栏）、经验同步
- **睡眠系统**：床交互、睡眠验证、重生点设置
- **维度传送**：传送门触发、坐标转换、传送门搜索/创建
- **统计系统**：合成统计追踪、配方解锁触发器
- **成就系统**：PlayerAdvancements 管理、配方书
- **队伍系统**：通过记分板获取队伍信息
- **PvP 保护**：通过 `canHarmPlayer()` 和 `hurt()` 重写实现 PvP 规则和队伍友伤检查
- **旁观者跟踪**：摄像机实体跟踪、位置同步、旁观/退出旁观逻辑

## 上下游外部依赖关系

### 上游依赖（本模块使用的组件）

| 模块 | 用途 |
|------|------|
| `common/entity/entities/player/Player.hpp` | 玩家基类 |
| `server/network/ServerNetwork.hpp` | ServerClientConnection（网络连接接口） |
| `common/network/ir/packets/play/*` | IR 网络包（`SystemChat`/`SetActionBarText`/`SetExperience`/`SetTitleText`/`SetSubtitleText`/`SetCamera` 等，经 `connection.send(ir::IrPacket{...})` 发送） |
| `common/entity/player/SleepManager.hpp` | 睡眠管理器 |
| `common/entity/player/SpawnPointValidator.hpp` | 重生点验证 |
| `common/world/dimension/teleport/Teleporter.hpp` | 传送器 |
| `server/advancement/PlayerAdvancements.hpp` | 玩家成就进度 |
| `server/stats/StatisticsManager.hpp` | 统计管理器 |
| `server/scoreboard/ServerScoreboard.hpp` | 服务端记分板 |

### 下游依赖（使用本模块的组件）

| 模块 | 用途 |
|------|------|
| `server/world/ServerWorld.hpp` | 世界中的玩家实体管理 |
| `server/core/PlayerManager.hpp` | 玩家创建、销毁、查询 |
| `server/dimension/ServerDimension.hpp` | 维度传送时更新玩家世界引用 |
| `server/application/MinecraftServer.hpp` | 服务器主类持有玩家引用 |

## 容易踩的坑

### 1. ServerPlayer vs ServerPlayerData 的混淆

项目中存在两个玩家相关的类：

| 类 | 位置 | 职责 |
|----|------|------|
| `ServerPlayer` | `server/player/` | 实体类，游戏逻辑（物理、睡眠、传送、统计、成就） |
| `ServerPlayerData` | `server/core/` | 数据结构，网络同步（位置、心跳、传送确认） |

**注意**：当前两个类都在使用，职责不同，不要混淆。

### 2. 世界指针可能为空

`getWorld()` 可能返回 `nullptr`，使用前必须检查：

```cpp
if (ServerWorld* world = player->getWorld()) {
    // 使用世界
}
```

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

### 8. CHUNK_HEIGHT vs MAX_BUILD_HEIGHT

根据 PROJECT_CONVENTIONS.md，这两个常量目前值相同但语义不同。未来 MIN_BUILD_HEIGHT 可能从 0 改为 -64，届时 CHUNK_HEIGHT 将不等于 MAX_BUILD_HEIGHT。

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
