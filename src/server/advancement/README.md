# 服务端成就系统 (Server Advancement System)

## 目录结构树

```
server/advancement/
├── PlayerAdvancements.hpp/cpp     # 玩家成就进度管理（追踪、持久化、授予/撤销、监听器注册/注销、奖励发放、可见性评估）
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
│  │ BlockPlaceEvent、CuredZombieVillagerEvent、ServerTickEvent、    │  │
│  │ VillagerTradeEvent、BredAnimalsEvent、ConsumeItemEvent、       │  │
│  │ TameAnimalEvent、SummonedEntityEvent 等                        │  │
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
│                                                                      │
│  可见性评估：                                                         │
│  _ensureVisibility → AdvancementVisibilityEvaluator::evaluateVisi-  │
│                      bilityFromNode（从变更成就的根节点重新计算整棵树） │
│  _updateVisibility → AdvancementVisibilityEvaluator::evaluateVisi-  │
│                      bility（从每棵树的根重新计算全部可见性）          │
│  _shouldShow → 回退的简化可见性判定（无 manager 时使用）              │
│                                                                      │
│  奖励发放：                                                           │
│  _grantRewards → 经验值（Player::addExperience）                      │
│              → 配方解锁（Player::unlockRecipes）                      │
│              → 战利品表（LootTable::generate + PlayerInventory::add  │
│                + ItemDropHelper::spawnItemEntity 处理溢出）           │
│              → 函数执行（FunctionManager::execute，通过 ServerCommandSource 执行） │
│                                                                      │
│  持久化：                                                             │
│  onAdvancementsReloaded → toJson() → loadFromJson() → flushAdvanc-  │
│                            ements()（保存进度、注销监听器、清空状态、  │
│                            恢复进度、初始化新成就）                    │
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

- `common/advancement/` - 成就系统核心（触发器定义、条件谓词、AdvancementManager、AdvancementVisibilityEvaluator、ICriterionTriggerBase 类型擦除接口）
- `common/entity/inventory/PlayerInventory.hpp` - 玩家物品栏（战利品表奖励发放）
- `common/entity/utils/ItemDropHelper.hpp` - 物品掉落（物品栏满时溢出处理）
- `common/item/loot/LootTable.hpp` - 战利品表（奖励发放）
- `common/item/loot/LootTableManager.hpp` - 战利品表管理器（查找战利品表）
- `common/item/loot/context/LootContextBuilder.hpp` - 战利品上下文构建器
- `common/item/loot/context/LootParameterSets.hpp` - 战利品参数集（chest 参数集）
- `common/item/loot/context/LootParams.hpp` - 战利品参数（THIS_ENTITY、KILLER_PLAYER）
- `common/util/math/random/Random.hpp` - 随机数生成器
- `common/world/IWorld.hpp` - 世界接口（获取世界种子）
- `server/event/` - 服务端事件总线（订阅事件，含 ServerTickEvent）
- `server/application/IServer.hpp` - 服务器接口（获取 LootTableManager、ServerWorld、ServerPlayerEntityManager）
- `server/core/PlayerManager.hpp` - 玩家管理器（UUID 查找）
- `server/player/ServerPlayer.hpp` - 服务端玩家实体（经验值发放、配方解锁）
- `server/world/ServerWorld.hpp` - 服务端世界
- `server/world/player/ServerPlayerEntityManager.hpp` - 玩家实体管理
- `common/world/village/` - 村庄系统（声望更新）

### 依赖本模块的外部模块

- `server/application/MinecraftServer.cpp` - 初始化和关闭 AdvancementEventHandler，调用 `CriterionTriggers::registerBuiltinTriggers()`
- `server/player/ServerPlayer.hpp` - 持有 PlayerAdvancements 实例，通过 `setServerPlayer()`/`getServerPlayer()` 关联
- `common/advancement/trigger/*.cpp` - 触发器实现（包含 TriggerInstantiation.hpp）

## 可见性算法

`PlayerAdvancements` 使用 `AdvancementVisibilityEvaluator`（位于 `common/advancement/`）实现与 MC Java 版一致的可见性递归算法。核心规则：

1. **已完成成就始终可见**（VisibilityRule::Show）
2. **无 display 的成就标记为 Hide**（VisibilityRule::Hide，如配方解锁等技术成就），但 anyChildDone 机制可能覆盖此规则（见下方说明）
3. **隐藏成就（hidden=true）在完成前不可见**（VisibilityRule::Hide）
4. **非隐藏且未完成的成就**（VisibilityRule::NoChange）：向上回溯 `VISIBILITY_DEPTH=2` 层祖先
   - 遇到 Show → 可见
   - 遇到 Hide → 不可见（阻断传播）
   - 全部 NoChange → 不可见
5. **子树中有已完成的成就会使祖先链可见**（anyChildDone 机制）

**重要：可见性判定仅使用 `isDone`，不使用 `hasProgress`。** 这与 MC Java 原版一致：部分完成（有进度但未完成）不影响可见性。`AdvancementVisibilityEvaluator` 和 `_shouldShow` 都只检查 `isDone`。

**关于无 display 成就的 anyChildDone 行为：** 当无 display 的成就有已完成的子成就时，anyChildDone=true 会使其在算法层面被标记为"可见"。这与 MC Java 一致——MC Java 的 `PlayerAdvancements.updateTreeVisibility` 也会将无 display 节点添加到 visible 集合中。客户端/UI 层负责过滤不渲染无 display 的成就。`_shouldShow` 作为简化回退路径，对无 display 节点直接返回不可见。

可见性计算分为三个方法：
- `_ensureVisibility(advancement)`：单个成就状态变化时调用，通过 `evaluateVisibilityFromNode` 从变更成就的根节点重新评估整棵树的可见性，同时跟踪 `m_visibilityChanged` 集合用于网络同步
- `_updateVisibility(manager)`：全量刷新可见性，遍历所有根成就调用 `evaluateVisibility`，用于 `loadFromJson` 等场景
- `_shouldShow(advancement, manager)`：简化的单成就可见性判定回退路径，当 `m_manager` 为空时使用，仅使用 `isDone` 不使用 `hasProgress`

## 奖励发放

`_grantRewards` 在成就完成时（`!wasDone && isDone`）被调用，支持四种奖励类型：

1. **经验值**：调用 `ServerPlayer::addExperience()`，无 ServerPlayer 时跳过
2. **配方解锁**：调用 `ServerPlayer::unlockRecipes()`，无 ServerPlayer 时跳过
3. **战利品表**：通过 `LootTableManager` 获取战利品表，使用 `LootContextBuilder` 构建 `chest` 参数集上下文（THIS_ENTITY=玩家, KILLER_PLAYER=玩家），生成物品后添加到玩家物品栏，溢出部分通过 `ItemDropHelper::spawnItemEntity()` 掉落在玩家位置
4. **函数执行**：通过 `IServer::functionManager()` 获取 `FunctionManager`，使用 `gamemaster` 权限等级的 `ServerCommandSource` 执行函数中的命令。函数不存在时输出警告。

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

12. **onAdvancementsReloaded 必须先注销再清空**：重载时必须先调用 `unregisterListeners()` 注销旧监听器再清空进度，否则旧的监听器指针会悬空。当前实现流程：`toJson()` 保存 → 逐个 `unregisterListeners()` → 清空状态 → `loadFromJson()` 恢复 → `flushAdvancements()` 初始化新成就。

13. **PlayerAdvancements 与 ServerPlayer 的关联时机**：`setServerPlayer()` 必须在 `ServerPlayer` 构造后尽早调用，因为 `_grantRewards()` 依赖 `m_player` 发放经验值、解锁配方和生成战利品表物品。

14. **ServerPlayerData::advancements 是废弃字段**：该字段始终为 nullptr，成就系统通过 `ServerPlayer::getAdvancements()` 获取 `PlayerAdvancements`。命令系统和实体选择器已改用 `ServerPlayerEntityManager → Player::asServerPlayer() → ServerPlayer::getAdvancements()` 路径。

15. **VillagerTradeEvent 事件链**：玩家与村民/流浪商人交易时，`AbstractVillagerEntity::notifyTrade()` 通过 `IWorld::onVillagerTrade()` 发布 `VillagerTradeEvent`，`AdvancementEventHandler::_onVillagerTrade()` 订阅该事件并执行：
    - 更新统计 `minecraft:traded_with_villager`（通过 `StatisticsManager::incrementCustom()`）
    - 触发 `VillagerTradeTrigger`（检查交易物品和村民实体谓词条件）

    事件触发路径：`MerchantResultSlot::onTake()` → `IMerchant::notifyTrade()` → `AbstractVillagerEntity::notifyTrade()` → `IWorld::onVillagerTrade()` → `ServerWorld::onVillagerTrade()` → `ServerEventBus::publish(VillagerTradeEvent)` → `AdvancementEventHandler::_onVillagerTrade()`

16. **可见性级联效应**：单个成就的状态变化可能级联影响子成就的可见性。`_ensureVisibility()` 使用 `AdvancementVisibilityEvaluator::evaluateVisibilityFromNode()` 从变更成就的根节点重新评估整棵成就树，而非仅更新单个成就。这确保了父成就完成时子成就正确可见、撤销父成就条件时子成就正确隐藏。

17. **m_manager 缓存**：`PlayerAdvancements` 在 `flushAdvancements()` 和 `onAdvancementsReloaded()` 中缓存 `AdvancementManager` 指针到 `m_manager`，用于 `_ensureVisibility()` 和 `_shouldShow()` 查找父成就。如果 `m_manager` 为空，`_ensureVisibility()` 回退到 `_shouldShow()` 的简化逻辑，`_shouldShow()` 在无 manager 时默认可见（非隐藏且有 display）。

18. **战利品表奖励的物品溢出**：`_grantRewards()` 将战利品表生成的物品添加到玩家物品栏，如果物品栏已满，剩余物品通过 `ItemDropHelper::spawnItemEntity()` 掉落在玩家位置，使用玩家的 UUID 作为掉落者标识以设置拾取延迟。

19. **TameAnimalTrigger 事件链**：玩家成功驯服动物时触发。驯服路径包括：
    - 鹦鹉驯服：`ParrotEntity::interactMob()` → `setTamed(true)` + `setOwnerId()` → `IWorld::onTameAnimal()` → `ServerWorld::onTameAnimal()` → `ServerEventBus::publish(TameAnimalEvent)` → `AdvancementEventHandler::_onTameAnimal()`
    - 马驯服：`RunAroundLikeCrazyGoal` → `AbstractHorseEntity::setTamedBy()` → `IWorld::onTameAnimal()` → 同上
    - **未实现**：狼（WolfEntity）和猫（CatEntity）尚未实现 `interactMob()` 驯服交互，驯服触发点待实现后接入

20. **SummonedEntityTrigger 事件链**：玩家召唤实体时触发。当前触发路径：
    - `/summon` 命令：`SummonCommand` → `world->spawnEntity()` → `IWorld::onSummonedEntity()` → `ServerWorld::onSummonedEntity()` → `ServerEventBus::publish(SummonedEntityEvent)` → `AdvancementEventHandler::_onSummonedEntity()`
    - 末影龙重生：`EndDragonFight::setRespawnStage(END)` → `_createNewDragon()` 创建新龙 → 遍历 `m_dragonBossBar->getPlayers()` → 对每个可见玩家调用 `IWorld::onSummonedEntity(playerId, newDragon)` → `ServerWorld::onSummonedEntity()` → 同上事件链（对应 MC Java `CriteriaTriggers.SUMMONED_ENTITY.trigger(serverplayer, enderdragon)`）
    - **未实现**：铁傀儡/雪傀儡建造（`CarvedPumpkinBlock`）、凋灵建造（`WitherSkullBlock` 未实现）。这些场景需要重构以获取附近玩家信息后触发

21. **PlayerInteractedWithEntityTrigger 触发路径**：不通过 IWorld 回调，而是在 `ServerPlayRouter` 的 UseEntity/Interact 分支中直接调用 `AbstractCriterionTrigger<PlayerInteractedWithEntityTriggerInstance>::trigger()`。当交互结果为 `Success` 或 `Consume` 时，获取玩家手持物品和目标实体进行谓词匹配。（旧 `PacketHandler::handleUseEntity` 已删除，逻辑迁入 `ServerPlayRouter`；TODO(Phase6) 完成接线。）
