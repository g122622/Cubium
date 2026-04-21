# Server Interaction 模块

服务端交互模块，处理玩家与世界之间的各种交互行为。

## 目录结构

```
src/server/interaction/
├── BlockInteractionManager.hpp   # 方块交互管理器头文件
├── BlockInteractionManager.cpp   # 方块交互管理器实现
├── ContainerManager.hpp          # 容器管理器头文件
├── ContainerManager.cpp          # 容器管理器实现
├── InventoryManager.hpp          # 物品栏管理器头文件
├── InventoryManager.cpp          # 物品栏管理器实现
├── MiningManager.hpp             # 挖掘管理器头文件
└── MiningManager.cpp             # 挖掘管理器实现
```

## 文件详细介绍

### BlockInteractionManager.hpp / .cpp

**职责**：处理玩家与方块的交互，包括方块破坏、方块放置、方块使用等。

**主要数据结构**：

```cpp
// 方块交互结果
struct BlockInteractionResult {
    bool success = false;
    String message;
};

// 方块破坏结果
struct BlockBreakResult {
    bool blockBroken = false;
    u32 newBlockStateId = 0;
    String message;
};

// 方块放置结果
struct BlockPlacementResult {
    bool success = false;
    bool blockPlaced = false;
    bool itemConsumed = false;
    BlockPos position;
    u32 newBlockStateId = 0;
    String message;
};
```

**主要方法**：

| 方法 | 说明 |
|------|------|
| `handleBlockInteraction()` | 处理方块交互数据包（开始/中止/停止破坏） |
| `handleBlockPlacement()` | 处理方块放置（位置验证、碰撞检测、物品消耗） |
| `handleBlockBreak()` | 处理方块破坏（验证、掉落物生成、设置为空气） |
| `handleBlockUse()` | 处理右键激活（调用方块 `onBlockActivated`） |
| `canInteract()` | 验证玩家是否可以与方块交互（距离检查，最大6格） |
| `canBreakBlock()` | 验证玩家是否可以破坏方块（硬度检查） |
| `generateBlockDrops()` | 生成方块掉落物 |
| `setOnBlockBreak()` | 设置方块破坏回调 |
| `setOnBlockPlace()` | 设置方块放置回调 |

**交互距离验证**：
- 玩家眼睛位置到方块中心距离平方 <= 36.0（6格）

**游戏模式处理**：
- 创造模式：放置方块不消耗物品
- 旁观模式：禁止放置方块

---

### MiningManager.hpp / .cpp

**职责**：追踪玩家的挖掘进度，计算挖掘速度，广播破坏动画。

**主要数据结构**：

```cpp
// 挖掘状态
struct MiningState {
    BlockPos position;       // 挖掘位置
    f32 progress = 0.0f;     // 挖掘进度 (0.0 - 1.0)
    u8 lastStage = 255;      // 上次广播的阶段 (0-9)
    bool active = false;     // 是否正在挖掘
    u64 startTick = 0;       // 开始tick
    EntityId breakerId = 0;  // 破坏者实体ID（用于广播动画）
};
```

**主要方法**：

| 方法 | 说明 |
|------|------|
| `startMining()` | 开始挖掘，初始化挖掘状态 |
| `abortMining()` | 中止挖掘，清除状态 |
| `handleBlockInteraction()` | 处理方块交互数据包（分发到开始/中止/停止） |
| `tick()` | 每 tick 更新挖掘进度 |
| `getMiningProgress()` | 获取挖掘进度 (0.0 - 1.0) |
| `isMining()` | 检查是否正在挖掘 |
| `getMiningPosition()` | 获取当前挖掘位置 |
| `calculateMiningSpeed()` | 计算挖掘速度（基于方块硬度、工具、游戏模式） |
| `broadcastBreakAnim()` | 广播破坏动画 |

**挖掘速度计算**：
- 创造模式：瞬间破坏（速度 = 1.0）
- 硬度为0的方块：瞬间破坏
- 不可破坏方块（硬度 < 0）：速度 = 0
- 常规模式：基础速度 = 1.0 / (hardness * 30.0)

**动画阶段**：
- 阶段范围：0-9
- 计算公式：`stage = min(progress * 10.0, 9.0)`

---

### ContainerManager.hpp / .cpp

**职责**：管理玩家的容器交互，包括打开/关闭容器菜单、处理容器点击、合成系统。

**主要数据结构**：

```cpp
// 容器点击结果
struct ContainerClickResult {
    bool success = false;
    ItemStack cursorItem;
    String message;
};

// 打开的容器
struct OpenContainer {
    std::unique_ptr<AbstractContainerMenu> menu;
    ContainerType type = ContainerType::Player;
    BlockPos position;
};
```

**主要方法**：

| 方法 | 说明 |
|------|------|
| `openContainer()` | 打开容器（返回容器ID） |
| `closeContainer()` | 关闭容器 |
| `handleClick()` | 处理容器点击（槽位、按钮、模式） |
| `getOpenMenu()` | 获取打开的菜单 |
| `getOpenContainerType()` | 获取打开的容器类型 |
| `hasOpenContainer()` | 检查是否有打开的容器 |
| `setOnContainerOpen()` | 设置容器打开回调 |
| `setOnContainerClose()` | 设置容器关闭回调 |
| `setOnContainerUpdate()` | 设置容器内容更新回调 |

**支持的容器类型**：
- `ContainerType::Player`：玩家背包
- `ContainerType::CraftingTable`：工作台

**当前实现状态**：
- `CraftingTable` 打开时会创建真实 `CraftingMenu`，并可处理 `handleClick()` 点击逻辑
- 容器更新通过 `setOnContainerUpdate()` 回调推送到网络层
- `handleClick()` 现在通过 `ContainerTypes::toClickType()` 统一翻译点击动作，避免客户端和服务端各自维护一套映射

---

### InventoryManager.hpp / .cpp

**职责**：管理玩家的物品栏，包括物品栏同步、手持物品槽位、物品栏操作。

**主要方法**：

| 方法 | 说明 |
|------|------|
| `getInventory()` | 获取玩家物品栏 |
| `setSelectedSlot()` | 设置选中槽位 (0-8) |
| `getSelectedSlot()` | 获取选中槽位 |
| `getHeldItem()` | 获取手持物品 |
| `setItem()` | 设置槽位物品 |
| `syncToClient()` | 同步物品栏到客户端 |
| `syncAllToClient()` | 同步所有物品栏到客户端 |
| `initializeInventory()` | 初始化玩家物品栏 |
| `cleanupInventory()` | 清理玩家物品栏数据 |
| `setOnInventoryUpdate()` | 设置物品栏更新回调 |

**物品栏槽位范围**：
- 快捷栏：0-8
- 主背包：0-35
- 总大小：41（快捷栏9 + 主背包27 + 护甲4 + 副手1）

---

## 模块关系

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
         │                     │                     │
         │    ┌────────────────┴─────────────────────┘
         │    │
         ▼    ▼
┌──────────────────┐
│ ContainerManager │
└──────────────────┘
```

**依赖关系**：

| 模块 | 依赖 |
|------|------|
| BlockInteractionManager | ServerWorld, PlayerManager, LootTableManager, InventoryManager, BlockDropHandler, BlockItemRegistry |
| MiningManager | PlayerManager, ConnectionManager, ServerWorld |
| ContainerManager | PlayerManager |
| InventoryManager | PlayerManager |

**交互流程**：

1. **方块放置流程**：
   ```
   客户端发送 PlayerTryUseItemOnBlockPacket
       ↓
   MinecraftServer 分发给 BlockInteractionManager
       ↓
   BlockInteractionManager.handleBlockPlacement()
       ↓
   验证距离、游戏模式、物品类型
       ↓
   BlockItem.tryPlace() → BlockItemUseContext
       ↓
   ServerWorld.setBlock()
       ↓
   InventoryManager 消耗物品（非创造模式）
       ↓
   回调通知 MinecraftServer 广播
   ```

2. **方块破坏流程**：
   ```
   客户端发送 BlockInteractionPacket (StartDestroyBlock)
       ↓
   MiningManager.startMining()
       ↓
   每 tick 更新进度
       ↓
   进度达到 1.0 时调用回调
       ↓
   BlockInteractionManager.handleBlockBreak()
       ↓
   生成掉落物、设置为空气
       ↓
   回调通知 MinecraftServer 广播
   ```

3. **容器打开流程**：
   ```
   客户端发送 OpenContainerPacket
       ↓
   MinecraftServer 分发给 ContainerManager
       ↓
   ContainerManager.openContainer()
       ↓
   创建容器菜单、分配容器ID
       ↓
   回调通知 MinecraftServer 发送 OpenContainerPacket
   ```

---

## 模块整体说明

### 整体职责

`interaction` 模块负责处理服务端所有玩家与世界交互的逻辑：

1. **方块交互**：破坏、放置、使用方块
2. **挖掘进度**：追踪挖掘进度、计算速度、广播动画
3. **容器管理**：打开/关闭容器、处理点击操作
4. **物品栏管理**：物品栏状态、同步、操作

### 输入

- 玩家数据（PlayerManager）
- 世界状态（ServerWorld）
- 网络数据包（通过 MinecraftServer 分发）
- 物品/方块注册表

### 输出

- 方块变化（通过 ServerWorld）
- 掉落物实体（通过 ServerWorld.entityManager()）
- 网络数据包（通过回调发送给客户端）
- 容器状态更新

### 依赖项

**内部依赖**：
- `server/world/ServerWorld.hpp`
- `server/core/PlayerManager.hpp`
- `server/core/ConnectionManager.hpp`
- `server/core/ServerPlayerData.hpp`
- `server/world/drop/BlockDropHandler.hpp`

**公共依赖**：
- `common/core/Types.hpp`
- `common/core/Result.hpp`
- `common/world/block/BlockPos.hpp`
- `common/world/block/Block.hpp`
- `common/entity/inventory/PlayerInventory.hpp`
- `common/entity/inventory/AbstractContainerMenu.hpp`
- `common/item/ItemStack.hpp`
- `common/item/BlockItemRegistry.hpp`
- `common/item/BlockItemUseContext.hpp`
- `common/network/packet/ProtocolPackets.hpp`
- `common/loot/LootTableManager.hpp`

### 使用方法

**初始化**（在 MinecraftServer 中）：

```cpp
// 创建管理器
m_inventoryManager = std::make_unique<interaction::InventoryManager>(*m_playerManager);
m_miningManager = std::make_unique<interaction::MiningManager>(*m_playerManager, *m_connectionManager);
m_containerManager = std::make_unique<interaction::ContainerManager>(*m_playerManager);
m_blockInteractionManager = std::make_unique<interaction::BlockInteractionManager>(
    *m_world, *m_playerManager, *m_lootTableManager);

// 设置依赖
m_blockInteractionManager->setInventoryManager(m_inventoryManager.get());

// 设置回调
m_miningManager->setOnMiningComplete([this](PlayerId playerId, const BlockPos& pos) {
    m_blockInteractionManager->handleBlockBreak(playerId, pos);
});

m_blockInteractionManager->setOnBlockBreak([this](PlayerId playerId, const BlockPos& pos, const BlockState& state) {
    broadcastBlockBreak(playerId, pos, state);
});

m_inventoryManager->setOnInventoryUpdate([this](PlayerId playerId, const PlayerInventory& inventory) {
    sendInventoryUpdate(playerId, inventory);
});
```

**处理数据包**：

```cpp
void MinecraftServer::handleBlockInteractionPacket(PlayerId playerId, const BlockInteractionPacket& packet) {
    m_miningManager->handleBlockInteraction(playerId, packet.pos, packet.action);
}

void MinecraftServer::handleBlockPlacementPacket(PlayerId playerId, const PlayerTryUseItemOnBlockPacket& packet) {
    auto heldItem = m_inventoryManager->getHeldItem(playerId);
    auto result = m_blockInteractionManager->handleBlockPlacement(
        playerId, packet.pos, packet.hitPos, packet.face, heldItem);
    // 处理结果...
}
```

**每帧更新**：

```cpp
void MinecraftServer::tick() {
    m_miningManager->tick(*m_world);
}
```

---

## 容易踩的坑

### 1. 挖掘进度同步问题

**问题**：挖掘进度只在服务端计算，客户端可能会出现视觉延迟。

**解决方案**：
- 确保 `broadcastBreakAnim` 回调正确发送给所有可见玩家
- 动画阶段 (0-9) 变化时才广播，避免过度发送

### 2. 物品消耗时序

**问题**：方块放置成功但物品未正确消耗。

**解决方案**：
- 在 `handleBlockPlacement` 中检查 `tryPlace` 返回值后再消耗物品
- 创造模式下设置 `itemConsumed = true` 但不实际减少数量

### 3. 容器ID冲突

**问题**：多个容器可能使用相同的容器ID。

**解决方案**：
- 使用 `m_nextContainerIds` 为每个玩家维护独立的容器ID计数器
- 每次打开容器时递增ID

### 4. 挖掘中断处理

**问题**：玩家切换目标方块时，之前的挖掘状态未清除。

**解决方案**：
- 新挖掘开始时检查并清除现有状态
- 玩家断开连接时清理挖掘状态

### 5. 距离验证精度

**问题**：使用整数坐标计算距离可能导致精度问题。

**解决方案**：
- 使用玩家眼睛位置（y + 1.62）计算
- 使用双精度浮点数计算距离平方，避免开方

### 6. 容器菜单未正确清理

**问题**：玩家断开连接时容器状态未清理。

**解决方案**：
- 在 PlayerManager 的玩家移除回调中调用 `cleanupInventory`
- 确保所有管理器在玩家离开时清理状态

### 7. 创造模式瞬间破坏

**问题**：创造模式下挖掘进度需要完整计算。

**解决方案**：
- 在 `calculateMiningSpeed` 中直接返回 1.0，无需考虑硬度

### 8. 物品栏槽位范围

**问题**：槽位索引超出范围导致越界访问。

**解决方案**：
- `setSelectedSlot` 限制范围为 0-8
- `setItem` 限制范围为 0-35（主背包+快捷栏）
- 添加边界检查和日志警告

---

## 相关测试用例

### 物品栏测试 (tests/common/test_inventory.cpp)

测试 PlayerInventory 和 Slot 的基础功能：

| 测试类 | 测试用例 |
|--------|----------|
| InventorySlotsTest | ConstantsAreCorrect - 槽位常量验证 |
| PlayerInventoryTest | InitialState - 初始状态 |
| PlayerInventoryTest | SetAndGetItem - 设置和获取物品 |
| PlayerInventoryTest | HotbarOperations - 快捷栏操作 |
| PlayerInventoryTest | RemoveItem - 移除物品 |
| PlayerInventoryTest | ClearInventory - 清空物品栏 |
| PlayerInventoryTest | AddItem - 添加物品 |
| PlayerInventoryTest | AddItemMerging - 物品合并 |
| PlayerInventoryTest | AddMultipleItems - 添加多种物品 |
| PlayerInventoryTest | FindSlot - 查找槽位 |
| PlayerInventoryTest | CountItem - 统计物品数量 |
| PlayerInventoryTest | HasItem - 检查物品存在 |
| PlayerInventoryTest | GetFirstEmptySlot - 获取第一个空槽位 |
| PlayerInventoryTest | SwapSlots - 交换槽位 |
| PlayerInventoryTest | PlaceItem - 放置物品 |
| PlayerInventoryTest | ArmorSlots - 护甲槽位 |
| PlayerInventoryTest | OffhandSlot - 副手槽位 |
| PlayerInventoryTest | SerializationEmpty - 空物品栏序列化 |
| PlayerInventoryTest | SerializationWithItems - 带物品序列化 |
| PlayerInventoryTest | DamageableItemStacking - 有耐久物品堆叠 |
| PlayerInventoryTest | IsHotbar - 快捷栏检查 |
| PlayerInventoryTest | GetBestHotbarSlot - 获取最佳快捷栏槽位 |
| SlotTest | BasicOperations - 基础操作 |
| SlotTest | MaxStackSize - 最大堆叠数 |
| SlotTest | MayPlace - 是否可放置 |

### 容器测试 (tests/common/test_container.cpp)

测试 Container、PlayerContainer 和容器数据包：

| 测试类 | 测试用例 |
|--------|----------|
| ContainerTest | Creation - 创建容器 |
| ContainerTest | AddSlot - 添加槽位 |
| ContainerTest | AddInventorySlots - 添加物品栏槽位 |
| ContainerTest | GetSlot - 获取槽位 |
| ContainerTest | GetSlotItem - 获取槽位物品 |
| ContainerTest | MergeItem - 合并物品 |
| ContainerTest | MergeItemToEmptySlot - 合并到空槽位 |
| ContainerTest | ClickPickLeft - 左键拾取 |
| ContainerTest | ClickPickRight - 右键拾取一半 |
| ContainerTest | ClickPlaceLeft - 左键放置 |
| ContainerTest | ClickPlaceRight - 右键放置一个 |
| ContainerTest | ClickSwap - 交换物品 |
| ContainerTest | SlotRange - 槽位范围 |
| ContainerTest | GetAllSlots - 获取所有槽位 |
| ContainerTest | SetAllSlots - 设置所有槽位 |
| ContainerTest | SerializeDeserialize - 序列化反序列化 |
| PlayerContainerTest | Creation - 创建玩家容器 |
| PlayerContainerTest | HotbarSlots - 快捷栏槽位 |
| PlayerContainerTest | PlayerInventoryRange - 玩家物品栏范围 |
| ContainerPacketTest | ContainerContentPacket - 容器内容包 |
| ContainerPacketTest | ContainerSlotPacket - 容器槽位包 |
| ContainerPacketTest | ContainerClickPacket - 容器点击包 |
| ContainerPacketTest | CloseContainerPacket - 关闭容器包 |
| ContainerPacketTest | OpenContainerPacket - 打开容器包 |
| ContainerPacketTest | HotbarSelectPacket - 快捷栏选择包 |
| ContainerPacketTest | HotbarSetPacket - 快捷栏设置包 |
| ContainerPacketTest | PlayerInventoryPacket - 玩家物品栏包 |

---

## 注意事项

1. **网络同步**：所有管理器通过网络回调与客户端同步，不直接操作网络层
2. **线程安全**：目前为单线程模型，如果需要多线程需要添加同步机制
3. **性能考虑**：挖掘进度每 tick 更新，避免在 `calculateMiningSpeed` 中进行复杂计算
4. **状态清理**：玩家断开连接时必须清理所有管理器中的相关状态
