#Inventory 模块

本模块实现了 Minecraft 的物品存储和容器交互系统。

## 目录结构

```
inventory/
├── IInventory.hpp                 # 背包接口（抽象基类，含 addListener/removeListener）
├── IInventory.cpp
├── InventoryRef.hpp               # 背包引用类型（管理可能拥有的背包指针，解决 ISidedInventoryProvider 生命周期）
├── InventoryRef.cpp
├── ContainerListener.hpp          # 容器变更监听器接口（containerChanged 回调）
├── ISidedInventory.hpp            # 侧面背包接口，用于漏斗等定向传输
├── ISidedInventoryProvider.hpp    # 侧面背包提供者接口
├── IRecipeHolder.hpp              # 配方持有者接口
├── IRecipeHolder.cpp
├── IRecipeHelperPopulator.hpp     # 配方辅助填充器接口
├── INamedContainerProvider.hpp    # 命名容器提供者接口
├── ContainerTypes.hpp             # 容器相关类型定义（ContainerId、ContainerType、ClickAction 等）
├── Slot.hpp                       # 槽位类及特殊槽位实现（ArmorSlot、ResultSlot、FurnaceFuelSlot、FurnaceResultSlot）
├── Slot.cpp
├── PlayerInventory.hpp            # 玩家背包（41 槽位：快捷栏、主背包、护甲、副手）
├── PlayerInventory.cpp
├── PlayerEnderChestInventory.hpp  # 末影箱物品栏（27 槽位，数据存储在玩家 NBT 中）
├── PlayerEnderChestInventory.cpp
├── CraftingInventory.hpp          # 合成网格背包和合成结果背包
├── CraftingInventory.cpp
├── CreativeInventory.hpp          # 创造模式物品库辅助
├── CreativeInventory.cpp
├── AbstractContainerMenu.hpp      # 容器菜单基类，管理槽位集合和物品交互逻辑
├── AbstractContainerMenu.cpp
└── container/                     # 具体容器实现
    ├── AnvilContainer.hpp/cpp     # 铁砧容器（修复成本计算、附魔合并）
    ├── BrewingStandContainer.hpp/cpp # 酿造台容器
    ├── CartographyContainer.hpp/cpp  # 制图台容器
    ├── ChestContainer.hpp/cpp        # 箱子容器
    ├── EnchantmentContainer.hpp/cpp  # 附魔台容器（书架力量、附魔等级公式）
    ├── FurnaceContainer.hpp/cpp      # 熔炉容器
    ├── HopperContainer.hpp/cpp       # 漏斗容器
    ├── LoomContainer.hpp/cpp         # 织布机容器
    └── CrafterContainer.hpp/cpp      # 自动合成器容器（3x3网格+预览结果，配方匹配预览）
```

## 内部模块关系

```
                  IInventory (接口)
                       ▲
                       │
      ┌────────────────┼────────────────────────┐
      │                │                        │
 PlayerInventory  CraftingInventory  PlayerEnderChestInventory
      │                │                        │ (27格末影箱物品栏)
      │                │                (实现 IRecipeHolder)  │
      └────────┬───────┘                        │
               │                                │
         CreativeInventory                      │
               │                                │
             Slot ──────────────────────────────┘ (引用 IInventory)
               │
               │
      AbstractContainerMenu
               │
       ┌───────┴───────┐
       │               │
  CraftingMenu    container/ 下的各种容器
```

## 上下游外部依赖关系

### 本模块依赖

- `core/Types.hpp` - 基础类型定义
- `core/Result.hpp` - 错误处理
- `item/ItemStack.hpp` - 物品堆类型
- `item/Item.hpp` - 物品基类
- `network/codec/PacketSerializer.hpp` - 网络序列化（PacketSerializer/Deserializer 已迁至 codec/）
- `entity/inventory/ContainerTypeUtils.hpp` - 容器类型工具（ClickAction↔ClickType 映射等，从旧 `ContainerPacketHandler` 迁出）
- `entity/Player.hpp` - 玩家实体

### 被依赖

- `entity/player/` - 玩家实体持有 PlayerInventory 和 PlayerEnderChestInventory
- `world/level/block/entity/` - 方块实体（箱子、熔炉等）实现 IInventory
- `client/gui/` - GUI 系统使用 AbstractContainerMenu
- `server/` - 服务端处理容器交互：`IntegratedServer::handleContainerClickPacket` / `StandaloneServer::handleContainerClickPacket` 内联处理 IR `ir::play::ContainerClick`，委托 `ContainerManager::handleClick()`

## 容易踩的坑

### 槽位索引混淆

玩家背包有 41 个槽位，索引分布容易混淆：快捷栏 0-8、主背包 9-35、护甲 36-39、副手 40。使用 `InventorySlots` 命名空间中的常量，避免硬编码数字。

### ItemStack 可能为空

`ItemStack::EMPTY` 是特殊的空物品堆，检查时应使用 `isEmpty()` 方法，而非检查 `getItem() == nullptr`。

### 物品堆叠和合并

不同物品有不同堆叠上限（通常 64，有些是 1 或 16）。物品合并需使用 `canMergeWith()` 检查是否完全相同（包括 NBT），而非仅比较 `getItem()`。

### 网络同步时机

背包状态变更后需要手动触发同步：`inventory.setChanged()` 标记变更，`menu.broadcastChanges()` 广播到客户端。

### CraftingInventory 坐标转换

使用行优先存储，坐标转换：`posToSlot(x, y) = y * width + x`，`slotToPos(slot) = (slot % width, slot / width)`。

### CreativeInventory 初始化顺序

依赖运行时注册表完整可用，必须按顺序初始化：`VanillaBlocks::initialize()` → `Items::initialize()` → `BlockItemRegistry::instance().initializeVanillaBlockItems()`。顺序错误会导致创造物品库退化成空列表。

### 容器菜单槽位管理

`AbstractContainerMenu` 内部管理玩家背包槽位范围，Shift+点击快速移动会自动处理。子类只需调用 `addPlayerInventorySlots()` 和 `addPlayerHotbarSlots()`。

### QuickCraft（拖拽分发）协议

拖拽分发使用 `-999` 槽位发送 START/END 事件，ADD_SLOT 事件发送到实际槽位。协议流程：`START(-999)` → `ADD_SLOT(slot) × N` → `END(-999)`。

- **拖拽模式**（按钮高 2 位）：`MODE_EVEN`（左键均匀分发）、`MODE_SINGLE`（右键逐个分发）、`MODE_FILL`（中键全部分发，仅创造模式）
- **事件状态**（按钮低 2 位）：`EVENT_START`(0)、`EVENT_ADD_SLOT`(1)、`EVENT_END`(2)
- **单槽降级**：当 END 事件触发且 `m_dragSlots.size()==1` 时（对应 MC 1.21.11 `AbstractContainerMenu#doClick` 中 `quickcraftSlots.size()==1` 的降级路径），重置拖拽状态后递归调用 `clicked(slot, dragMode, Pick, player)`，让单槽拖拽降级为普通 PICKUP 点击，从而触发 `_tryItemClickBehaviourOverride` 槽位覆写协议（收纳袋的 `overrideStackedOnOther`/`overrideOtherStackedOnMe`）。`MODE_EVEN`/`MODE_SINGLE` 分别对应 PICKUP 左/右键；`MODE_FILL` 不降级（创造模式专属，单槽无意义）。
- **多槽分发**：`m_dragSlots.size()>=2` 时走 `_distributeToDragSlot` 分发路径，不触发槽位覆写协议。
- **拖拽槽位过滤**：`_canDragIntoSlot` 检查槽位是否可拖入（空槽位或同物品可合并），不同物品的槽位不会被加入拖拽列表，因此单槽降级也不会触发——这与 MC Java `canItemQuickReplace` 行为一致。收纳袋的覆写协议通过直接 PICKUP 点击触发，而非拖拽降级。

### 槽位覆写协议（Slot Override Protocol）

`_tryItemClickBehaviourOverride` 在 `_handleClickPick` 中于常规拾取/放置逻辑之前调用，给光标物品和槽位物品各一次机会自定义交互行为：

1. 光标物品非空 → 调用 `Item::overrideStackedOnOther(cursor, slot, action, player)`，返回 true 则跳过默认逻辑
2. 槽位物品非空 → 调用 `Item::overrideOtherStackedOnMe(slotStack, cursor, slot, action, player)`，返回 true 则跳过默认逻辑

`overrideOtherStackedOnMe` 接收 `slotStack`（槽位物品的拷贝）和 `cursor`（光标引用，可被修改）。修改后的 `slotStack` 会被写回槽位（`slot.set(slotStack)`）。该协议是 MC 1.20+ 引入的，主要用于收纳袋（`BundleItem`）。

### 物品丢弃回调

上层（ServerWorld/IntegratedServer）需注入物品丢弃回调到 `AbstractContainerMenu`，用于 Q 键丢弃、点击屏幕外部丢弃、容器关闭时丢弃无法放入背包的物品。

### 护甲槽位限制和绑定诅咒

`ArmorSlot` 通过 `mayPlace()` 检查护甲类型匹配，`mayPickup()` 检查绑定诅咒——绑定诅咒的护甲在非创造模式下无法取下。

### NBT 序列化格式（MC 1.21.11）

`PlayerInventory::toNbt()` 和 `PlayerInventory::fromNbt()` 遵循 MC 1.21.11 新格式：

- **Inventory 列表**：仅包含快捷栏（Slot 0-8）和主背包（Slot 9-35），不再包含护甲和副手
- **equipment 复合标签**：由 `LivingEntity::addAdditionalSaveData()` 写入，使用 `EquipmentSlot` 枚举名作为键（`"head"`, `"chest"`, `"legs"`, `"feet"`, `"offhand"`, `"mainhand"`），空槽位省略
- **SelectedItemSlot**：当前选中的快捷栏槽位

`fromNbt()` 同时支持旧格式兼容读取：当 `equipment` 标签不存在时，从 Inventory 列表中读取护甲（Slot 100-103）和副手（Slot -106）。

### 末影箱物品栏变更通知

`PlayerEnderChestInventory` 支持两种变更通知机制：

1. **ContainerListener 模式**（推荐）：通过 `addListener(ContainerListener*)` / `removeListener(ContainerListener*)` 注册/移除监听器，支持多个监听者。容器内容变更时，所有监听器的 `containerChanged(IInventory&)` 被调用。参考 MC Java 的 `SimpleContainer.addListener()` / `removeListener()`。

2. **setOnChanged 回调**（兼容旧接口）：`setOnChanged(std::function<void()>)` 设置单一回调函数，在 `setChanged()` 中与 ContainerListener 一起被触发。适用于简单场景。

`ServerPlayer::setupInventoryCallback()` 中通过 `setOnChanged()` 注册了回调，通过 `PlayerDataManager::markDirty()` 标记玩家数据为脏以触发自动保存。当网络同步系统就绪后，可通过 `addListener()` 注册额外的监听器实现客户端同步。

### 末影箱容器打开/关闭流程

1. `EnderChestBlock::onBlockActivated()` → `setActiveChest()` → `openContainer()` → `startOpen()`（开盖动画和音效）
2. 容器菜单创建：`ContainerManager` 菜单工厂检测 `BlockEntityType::EnderChest`，创建 `ChestContainer` 以 `PlayerEnderChestInventory` 为背板
3. 容器关闭：`ChestContainer::removed()` → `closeInventory()` → `stopOpen()`（关盖动画、音效、清除 `activeChest` 引用）
4. **不要在 `StandaloneServer` 的容器关闭回调中再次调用 `stopOpen()`**，否则会导致 `EnderChestEntity::closeContainer()` 被调用两次，打开计数错误
