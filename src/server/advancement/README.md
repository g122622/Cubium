# 服务端成就系统 (Server Advancement System)

## 目录结构树

```
server/advancement/
├── PlayerAdvancements.hpp/cpp     # 玩家成就进度管理（追踪、持久化、授予/撤销、监听器注册/注销、奖励发放）
├── TriggerInstantiation.hpp       # 触发器模板方法实例化（供 common/advancement 使用）
├── AdvancementEventHandler.hpp    # 事件处理器（订阅服务端事件触发成就，含 ServerTickEvent 驱动 TickTrigger）
└── README.md
```

## 内部模块关系

```
┌──────────────────────────────────────────────────────────────────────┐
│                        AdvancementEventHandler                       │
│                     （事件订阅与触发器调度）                           │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │ 订阅事件：InventoryChangedEvent、PlayerKillEntityEvent、        │  │
│  │ BlockPlaceEvent、CuredZombieVillagerEvent、ServerTickEvent 等  │  │
│  └────────────────────────────────────────────────────────────────┘  │
└───────────────────────────────┬──────────────────────────────────────┘
                                │ 触发
                                ▼
┌──────────────────────────────────────────────────────────────────────┐
│                        PlayerAdvancements                            │
│                 （玩家成就进度管理器）                                 │
│  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐        │
│  │ 进度追踪/查询    │ │ 可见性管理      │ │ 持久化          │        │
│  ├─────────────────┤ ├─────────────────┤ ├─────────────────┤        │
│  │ 监听器注册/注销  │ │ 奖励发放        │ │ ServerPlayer关联│        │
│  └─────────────────┘ └─────────────────┘ └─────────────────┘        │
│  registerListeners → ICriterionTriggerBase::addListenerForCriterion  │
│  unregisterListeners → ICriterionTriggerBase::removeListenerForCriterion │
│  grantCriterion → _grantRewards + 注销已完成条件的监听器              │
└───────────────────────────────┬──────────────────────────────────────┘
                                │ 使用
                                ▼
┌──────────────────────────────────────────────────────────────────────┐
│                      TriggerInstantiation                            │
│                  （触发器模板方法实例化）                              │
│              提供 CriterionListener::grantCriterion()                │
│              和 AbstractCriterionTrigger::trigger() 实现             │
└──────────────────────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

### 本模块依赖的外部模块

- `common/advancement/` - 成就系统核心（触发器定义、条件谓词、AdvancementManager、ICriterionTriggerBase 类型擦除接口）
- `server/event/` - 服务端事件总线（订阅事件，含 ServerTickEvent）
- `server/application/IServer.hpp` - 服务器接口（获取 ServerWorld、ServerPlayerEntityManager）
- `server/core/PlayerManager.hpp` - 玩家管理器（UUID 查找）
- `server/player/ServerPlayer.hpp` - 服务端玩家实体（经验值发放、配方解锁）
- `server/world/ServerWorld.hpp` - 服务端世界
- `server/world/player/ServerPlayerEntityManager.hpp` - 玩家实体管理
- `common/world/village/` - 村庄系统（声望更新）
- `common/entity/inventory/PlayerInventory.hpp` - 玩家物品栏

### 依赖本模块的外部模块

- `server/application/MinecraftServer.cpp` - 初始化和关闭 AdvancementEventHandler，调用 `CriterionTriggers::registerBuiltinTriggers()`
- `server/player/ServerPlayer.hpp` - 持有 PlayerAdvancements 实例，通过 `setServerPlayer()`/`getServerPlayer()` 关联
- `common/advancement/trigger/*.cpp` - 触发器实现（包含 TriggerInstantiation.hpp）

## 容易踩的坑

1. **获取 ServerPlayer 的正确路径**：事件可能只携带 `PlayerId` 或 `UUID`。通过 `PlayerId` 获取 `ServerPlayer` 需要走 `IServer::getPlayerWorld()` → `IServer::playerEntityManager()` → `ServerPlayerEntityManager::getPlayerEntity()` → `Player::asServerPlayer()`。不要用 `PlayerManager::getPlayer()`，它返回的是网络会话数据 `ServerPlayerData`，不持有 `ServerPlayer` 引用。

2. **UUID vs PlayerId**：`CuredZombieVillagerEvent` 携带的是 UUID 字符串而非 `PlayerId`，需先通过 `PlayerManager::findByUuid()` 转换。

3. **初始化顺序**：`AdvancementEventHandler::setServer()` 必须在 `initialize()` 之前调用，否则事件处理器无法获取玩家实体。`CriterionTriggers::registerBuiltinTriggers()` 必须在成就加载之前调用，否则触发器实例无法从 JSON 反序列化。

4. **村庄声望更新条件**：治愈僵尸村民更新声望前，必须检查村民是否在村庄范围内（`villageManager->getVillageAt()`），只有村内治愈才更新声望。

5. **触发器调用方式**：触发成就必须通过 `trigger->trigger(*advancements, predicate)` 或 `trigger->triggerWithPredicate(*advancements, predicate)`，直接调用不会生效。

6. **TriggerInstantiation 模板实例化**：触发器模板方法（`CriterionListener::grantCriterion()` 和 `AbstractCriterionTrigger::trigger()`）在 `.cpp` 文件中实例化，需包含 `TriggerInstantiation.hpp`。

7. **事件中的空指针检查**：事件携带的指针可能为空（如 `BlockPlaceEvent::state`、`InventoryChangedEvent::inventory`），触发前必须检查。

8. **监听器注册/注销与 grantCriterion 的交互**：`grantCriterion()` 创建新进度时自动注册所有未完成条件的监听器；完成单个条件后注销该条件的监听器；成就全部完成时注销整个成就的所有监听器。手动注册/注销监听器时应避免重复操作。

9. **loadFromJson 中的监听器注册**：`loadFromJson()` 会为所有已加载但未完成的成就注册监听器，加载完成后无需额外调用 `registerListeners()`。

10. **flushAdvancements 用于新玩家初始化**：新玩家首次加入时没有存档数据，需调用 `flushAdvancements(manager)` 遍历 `AdvancementManager` 中所有成就，创建空进度条目并注册监听器。`_onPlayerLogin` 事件处理器已实现此调用。

11. **revokeCriterion 自动重新注册监听器**：撤销条件后，如果成就从完成变为未完成，会自动重新注册所有未完成条件的监听器；如果仅撤销单个条件，会为该条件重新注册监听器，确保后续触发器事件可以重新触发。

12. **onAdvancementsReloaded 必须先注销再清空**：重载时必须先调用 `unregisterListeners()` 注销旧监听器再清空进度，否则旧的监听器指针会悬空。

13. **PlayerAdvancements 与 ServerPlayer 的关联时机**：`setServerPlayer()` 必须在 `ServerPlayer` 构造后尽早调用，因为 `_grantRewards()` 依赖 `m_player` 发放经验值和解锁配方。

14. **ServerPlayerData::advancements 是废弃字段**：该字段始终为 nullptr，成就系统通过 `ServerPlayer::getAdvancements()` 获取 `PlayerAdvancements`。命令系统和实体选择器已改用 `ServerPlayerEntityManager → Player::asServerPlayer() → ServerPlayer::getAdvancements()` 路径。
