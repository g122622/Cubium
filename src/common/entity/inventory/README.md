# Inventory 模块

本模块实现了 Minecraft 的物品存储和容器交互系统。

## 目录结构

```
inventory/
├── IInventory.hpp                 # 背包接口（抽象基类）
├── IInventory.cpp
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
    └── LoomContainer.hpp/cpp         # 织布机容器
```

## 内部模块关系

```
                  IInventory (接口)
                       ▲
                       │
      ┌────────────────┼────────────────┐
      │                │                │
 PlayerInventory  CraftingInventory  CraftResultInventory
      │                │                │ (实现 IRecipeHolder)
      │                │                │
      └────────┬───────┘                │
               │                        │
         CreativeInventory              │
               │                        │
             Slot ──────────────────────┘ (引用 IInventory)
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
- `network/packet/PacketSerializer.hpp` - 网络序列化
- `entity/Player.hpp` - 玩家实体

### 被依赖

- `entity/player/` - 玩家实体持有 PlayerInventory
- `world/level/block/entity/` - 方块实体（箱子、熔炉等）实现 IInventory
- `client/gui/` - GUI 系统使用 AbstractContainerMenu
- `server/` - 服务端处理容器交互和数据包

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

### 物品丢弃回调

上层（ServerWorld/IntegratedServer）需注入物品丢弃回调到 `AbstractContainerMenu`，用于 Q 键丢弃、点击屏幕外部丢弃、容器关闭时丢弃无法放入背包的物品。

### 护甲槽位限制和绑定诅咒

`ArmorSlot` 通过 `mayPlace()` 检查护甲类型匹配，`mayPickup()` 检查绑定诅咒——绑定诅咒的护甲在非创造模式下无法取下。
