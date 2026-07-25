# Server Interaction 模块

服务端交互模块，处理玩家与世界之间的各种交互行为。

## 目录结构

```
src/server/interaction/
├── BlockInteractionManager.hpp   # 方块交互管理器（放置、破坏、使用）
├── BlockInteractionManager.cpp
├── ContainerManager.hpp          # 容器管理器（打开、关闭、点击）
├── ContainerManager.cpp
├── InventoryManager.hpp          # 物品栏管理器（槽位、同步）
├── InventoryManager.cpp
├── MiningManager.hpp             # 挖掘管理器（进度、速度、动画）
├── MiningManager.cpp
├── SignCommandHelper.hpp         # 告示牌命令执行辅助类
└── SignCommandHelper.cpp
```

## 内部模块关系

```
                    ┌─────────────────────┐
                    │   MinecraftServer   │
                    └──────────┬──────────┘
                               │
           ┌───────────────────┼───────────────────┐
           │                   │                   │
           ▼                   ▼                   ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│ InventoryManager │◄─┤BlockInteraction  │  │ MiningManager    │
│                  │  │   Manager        │  │                  │
└────────┬─────────┘  └────────┬─────────┘  └────────┬─────────┘
         │                     │                     │
         │    ┌────────────────┴─────────────────────┘
         │    │
         ▼    ▼
┌──────────────────┐
│ ContainerManager │
└──────────────────┘
```

- **BlockInteractionManager**：处理方块放置、破坏、使用，依赖 InventoryManager 获取手持物品
- **MiningManager**：追踪挖掘进度，挖掘完成后触发 BlockInteractionManager 的破坏逻辑；通过 `setOnBreakAnimBroadcast` 回调广播破坏动画（由 MinecraftServer 注入），通过 `setEntityIdResolver` 回调获取正确的 EntityId（PlayerId ≠ EntityId）
- **ContainerManager**：管理容器菜单，独立于其他管理器
- **InventoryManager**：物品栏状态管理，被 BlockInteractionManager 依赖
- **SignCommandHelper**：服务端告示牌命令执行，被 BlockInteractionManager 调用

## 外部依赖关系

### 上游（谁依赖了这个模块）

- `MinecraftServer`：主服务器类，创建并协调所有管理器

### 下游（这个模块依赖了谁）

**内部依赖**：
- `server/world/ServerWorld.hpp` - 世界操作
- `server/core/PlayerManager.hpp` - 玩家数据
- `server/core/ConnectionManager.hpp` - 网络广播
- `server/core/ServerPlayerData.hpp` - 玩家实体数据
- `server/world/drop/BlockDropHandler.hpp` - 掉落物生成

**公共依赖**：
- `common/core/Types.hpp`, `common/core/Result.hpp`
- `common/world/block/BlockPos.hpp`, `common/world/block/Block.hpp`
- `common/entity/inventory/PlayerInventory.hpp`, `common/entity/inventory/AbstractContainerMenu.hpp`
- `common/item/ItemStack.hpp`, `common/item/BlockItemRegistry.hpp`
- `common/entity/inventory/ContainerTypeUtils.hpp` - 容器点击类型转换工具（`toClickType` 等，从旧 `ContainerTypes::` 迁出）
- `common/loot/LootTableManager.hpp`

## 容易踩的坑

### 1. 挖掘进度同步

挖掘进度只在服务端计算，需确保 `broadcastBreakAnim` 回调正确发送给所有可见玩家。动画阶段 (0-9) 变化时才广播，避免过度发送。

**回调接入**：`setOnBreakAnimBroadcast` 必须在 `MinecraftServer::initializeInteractionManagers()` 中设置，通过 `ServerWorld::destroyBlockProgress()` → `MinecraftServer::broadcastBlockBreakProgressInRange()` 将破坏动画广播给同维度范围内其他玩家（破坏者自身排除）。

**EntityId 解析**：`setEntityIdResolver` 必须在 `setOnBreakAnimBroadcast` 之前设置，将 PlayerId 转换为正确的 EntityId。PlayerId 和 EntityId 是完全不同的标识符，不能直接 static_cast，必须通过 `ServerPlayerEntityManager::getPlayerEntityId()` 转换。

**中止挖掘**：`abortMining()` 在挖掘已开始（lastStage != 255）时会发送 stage=-1 的移除动画广播，对应 MC Java `ServerPlayerGameMode.stopDestroyBlock()` 中的行为。

### 2. 物品消耗时序

方块放置成功但物品未正确消耗：在 `handleBlockPlacement` 中检查 `tryPlace` 返回值后再消耗物品。创造模式设置 `itemConsumed = true` 但不实际减少数量。

### 3. 容器ID冲突

使用 `m_nextContainerIds` 为每个玩家维护独立的容器ID计数器，每次打开容器时递增。

### 4. 挖掘中断处理

新挖掘开始时检查并清除现有状态；玩家断开连接时清理挖掘状态。

### 5. 距离验证精度

使用玩家眼睛位置（y + 1.62）计算距离，使用双精度浮点数计算距离平方，避免开方。

### 6. 容器菜单未清理

在 PlayerManager 的玩家移除回调中调用 `cleanupInventory`，确保所有管理器在玩家离开时清理状态。

### 7. 创造模式瞬间破坏

在 `calculateMiningSpeed` 中直接返回 1.0，无需考虑硬度。

### 8. 物品栏槽位范围

- `setSelectedSlot` 限制范围 0-8
- `setItem` 限制范围 0-35（主背包+快捷栏）
- 添加边界检查和日志警告

### 9. 不要固定绑定主世界

`BlockInteractionManager` 必须按玩家维度解析 `ServerWorld`，交互时通过 `IServer::getPlayerWorld(playerId)` 获取当前世界。告示牌命令、掉落物、方块状态查询都基于该玩家当前维度执行。

### 10. 眼睛位置检测（水下挖掘惩罚）

MC 1.16.5 的水下挖掘惩罚检测玩家**眼睛位置**是否在水中，不是身体位置。需精确计算眼睛位置和流体表面高度进行比较。

### 11. spawnAfterBreak 调用时序

方块破坏流程中 `spawnAfterBreak` 的调用时序必须与 MC Java 一致：

1. `_generateBlockDrops` — 生成掉落物
2. `_setBlockToAir` — 将方块设置为空气
3. `block.spawnAfterBreak(world, pos, state, tool, true)` — 触发方块被破坏后的额外行为

**关键**：`spawnAfterBreak` 必须在 `_setBlockToAir` **之后**调用，因为 MC Java 中 `BlockBehaviour.spawnAfterBreak` 在方块已被移除后执行。对于 InfestedBlock，这意味着蠹虫在方块变为空气后才生成。

`handleBlockInteraction` 的 StopDestroyBlock 路径和 `handleBlockBreak` 路径都遵循此顺序。

### 12. 冒险模式 CanPlaceOn/CanDestroy 检查

`BlockInteractionManager` 在冒险模式下增加了方块交互权限检查：

- **放置检查**（`handleBlockPlacement`）：冒险模式下，检查手持物品的 CanPlaceOn 标签。只有持有带 CanPlaceOn 标签且目标方块匹配的物品时才允许放置。无 CanPlaceOn 标签的物品在冒险模式下不能放置方块。
- **破坏检查**（`_canBreakBlock`）：冒险模式下，检查手持物品的 CanDestroy 标签。只有持有带 CanDestroy 标签且目标方块匹配的物品时才允许破坏。无 CanDestroy 标签的物品在冒险模式下不能破坏方块。
- **mayInteract 检查**：`Player::mayInteract()` 在冒险模式下会检查主手和副手物品的 CanPlaceOn 标签，用于投掷物等间接交互场景。
- **注意**：CanDestroy 检查仅检查主手物品（与 MC Java `Player.blockActionRestricted()` 行为一致），而 CanPlaceOn 检查取决于调用方式——`handleBlockPlacement` 接收 `heldItem` 参数（由上层根据交互手传入），`Player::mayInteract()` 则检查双手。
