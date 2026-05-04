# Inventory 模块

本模块实现了 Minecraft 的物品存储和容器交互系统，参考 MC Java 1.16.5 的背包系统架构。

## 目录结构

```
inventory/
├── IInventory.hpp              # 背包接口（抽象基类）
├── ContainerTypes.hpp          # 容器相关类型定义
├── Container.hpp               # 容器类（槽位管理）
├── Container.cpp
├── Slot.hpp                    # 槽位类
├── Slot.cpp
├── PlayerInventory.hpp         # 玩家背包
├── PlayerInventory.cpp
├── CraftingInventory.hpp       # 合成网格背包
├── CraftingInventory.cpp
├── CreativeInventory.hpp       # 创造模式物品库辅助
├── CreativeInventory.cpp
├── AbstractContainerMenu.hpp   # 容器菜单基类
└── AbstractContainerMenu.cpp
```

## 文件详细介绍

### 1. IInventory.hpp

**职责**: 定义背包接口，所有背包容器的抽象基类。

**主要内容**:
- 纯虚函数定义背包基本操作
- 容量查询：`getContainerSize()`, `isEmpty()`, `getMaxStackSize()`
- 物品操作：`getItem()`, `setItem()`, `removeItem()`, `removeItemNoUpdate()`
- 容器操作：`clear()`, `setChanged()`
- 物品查找：`getFirstEmptySlot()`, `countItem()`, `hasItem()`, `findSlot()`, `canPlaceItem()`
- 序列化支持

**依赖项**:
- `core/Types.hpp` - 基础类型
- `item/ItemStack.hpp` - 物品堆类型

### 2. ContainerTypes.hpp

**职责**: 定义容器系统相关类型常量。

**主要内容**:
- `ContainerId` - 容器ID类型别名（i32）
- `ContainerType` - 容器类型枚举（对应 MC 1.16.5 Registry.MENU 注册顺序）：
  - Generic9x1~Generic9x6 - 箱子（1-6行）
  - Generic3x3 - 发射器/投掷器
  - Anvil - 铁砧
  - Beacon - 信标
  - BlastFurnace - 高炉
  - BrewingStand - 酿造台
  - Crafting - 工作台
  - Enchantment - 附魔台
  - Furnace - 熔炉
  - Grindstone - 砂轮
  - Hopper - 漏斗
  - Lectern - 讲台
  - Loom - 织布机
  - Merchant - 村民交易
  - ShulkerBox - 潜影盒
  - Smithing - 锻造台
  - Smoker - 烟熏炉
  - Cartography - 制图台
  - Stonecutter - 切石机
  - Player - 玩家背包（特殊值 255）
- `ClickAction` - 点击操作类型枚举（网络协议用，对应 MC 1.16.5 协议）
- `ContainerAction` - 容器动作类型枚举
- `ClickType` - 点击类型枚举（Pick, Place, QuickMove, Drag 等）

**依赖项**: 无外部依赖

### 3. Slot.hpp / Slot.cpp

**职责**: 表示背包中的单个槽位，用于容器UI显示和交互。

**主要内容**:
- 槽位索引常量定义（`InventorySlots` 命名空间）：
  - 快捷栏：0-8
  - 主背包：9-35
  - 护甲：36-39（头盔、胸甲、护腿、靴子）
  - 副手：40
- `Slot` 类：基础槽位实现
  - 显示坐标（x, y）
  - 物品操作：`getItem()`, `set()`, `remove()`, `isEmpty()`
  - 可放置性检查：`mayPlace()`, `isValid()`, `getMaxStackSize()`
- `ArmorSlot` 类：护甲槽位，只接受对应类型护甲
- `ResultSlot` 类：合成结果槽位，只能取出不能放入

**依赖项**:
- `IInventory.hpp` - 背包接口
- `item/ItemStack.hpp` - 物品堆

### 4. Container.hpp / Container.cpp

**职责**: 管理客户端-服务端的背包同步，持有多个槽位引用。

**主要内容**:
- `SlotRange` 结构：槽位范围表示
- `Container` 类：
  - 槽位管理：`addSlot()`, `getSlot()`, `addInventorySlots()`
  - 物品操作：`clicked()`, `quickMoveStack()`, `mergeItem()`
  - 槽位范围设置：`setPlayerInventoryRange()`, `setContainerInventoryRange()`
  - 变更检测：`hasChanged()`, `setChanged()`, `getChangeCount()`
  - 同步支持：`getAllSlots()`, `setAllSlots()`, `serialize()`, `deserialize()`
  - 点击处理：`handlePickClick()`, `handleQuickMoveClick()`, `handleThrowClick()`, `handleDragClick()`, `handleSwapClick()`, `handleCloneClick()`
- `PlayerContainer` 类：玩家背包容器，添加快捷栏、主背包、护甲、副手槽位

**依赖项**:
- `IInventory.hpp`, `Slot.hpp`, `ContainerTypes.hpp`
- `PlayerInventory.hpp`
- `item/ItemStack.hpp`

### 5. PlayerInventory.hpp / PlayerInventory.cpp

**职责**: 实现玩家的完整背包系统。

**主要内容**:
- 背包布局（41个槽位）：
  - 快捷栏（9个）：0-8
  - 主背包（27个）：9-35
  - 护甲（4个）：36-39
  - 副手（1个）：40
- 快捷栏操作：`getSelectedSlot()`, `setSelectedSlot()`, `getSelectedStack()`, `getBestHotbarSlot()`
- 物品添加：`add()`, `addInRange()`, `addItemCopy()`
- 物品查找：`getFirstEmptySlot()`, `findSlot()`, `findSlotMatching()`, `findSlotMatchingInRange()`
- 护甲操作：`getHelmet()`, `getChestplate()`, `getLeggings()`, `getBoots()`, `getOffhandItem()`
- 槽位操作：`swapSlots()`, `placeItem()`
- 统计：`countItem()`, `hasItem()`
- 序列化：`serialize()`, `deserialize()`

**依赖项**:
- `IInventory.hpp`, `Slot.hpp`
- `item/ItemStack.hpp`
- `entity/Player.hpp`（前向声明）

### 6. CraftingInventory.hpp / CraftingInventory.cpp

**职责**: 实现合成网格背包和合成结果背包。

**主要内容**:
- `CraftingInventory` 类：合成网格背包
  - 支持 2x2（玩家背包）和 3x3（工作台）网格
  - 网格坐标访问：`getItemAt()`, `setItemAt()`, `removeItemAt()`
  - 坐标转换：`posToSlot()`, `slotToPos()`
  - 边界计算：`getContentBounds()`
  - 内容变更回调
- `CraftResultInventory` 类：合成结果背包
  - 单槽位背包，存储合成结果
  - `setResultItem()`, `getResultItem()`, `hasResult()`

**依赖项**:
- `IInventory.hpp`
- `network/packet/PacketSerializer.hpp`

### 7. CreativeInventory.hpp / CreativeInventory.cpp

**职责**: 生成创造模式物品库条目，并为创造模式玩家填充初始背包。

**主要内容**:
- `CreativeInventoryEntry`：创造物品库条目，包含物品堆和搜索 key
- `buildCreativePaletteEntries()`：遍历运行时注册表，生成可搜索的创造模式条目列表
- `fillCreativeModeInventory(PlayerInventory&)`：把创造模式常用方块物品写入玩家背包

**依赖项**:
- `item/ItemRegistry.hpp` - 遍历已注册物品
- `item/items/block/BlockItemRegistry.hpp` - 判断方块物品并排序
- `entity/inventory/PlayerInventory.hpp` - 填充创造初始背包

### 8. AbstractContainerMenu.hpp / AbstractContainerMenu.cpp

**职责**: 容器菜单基类，管理槽位集合和物品交互逻辑。

**主要内容**:
- 槽位管理：`addSlot()`, `getSlot()`, `getSlotCount()`
- 玩家槽位快捷添加：`addPlayerInventorySlots()`, `addPlayerHotbarSlots()`, `addPlayerArmorSlots()`, `addPlayerOffhandSlot()`
- 点击处理：`clicked()`, `quickMoveStack()`
- 拖拽分发状态机（MC 1.16.5 对齐）：
  - `DragConstants` 命名空间定义事件和模式常量
  - 三种拖拽模式：均匀分发(左键)、逐个分发(右键)、全部分发(中键-仅创造模式)
  - `handleQuickCraft()` 实现完整状态机
- 物品移动：`moveItemToRange()`
- 持有物品管理：`getCarriedItem()`, `setCarriedItem()`
- 事件通知：`addListener()`, `removeListener()`, `broadcastChanges()`, `slotsChanged()`
- 生命周期：`stillValid()`, `removed()`
- 事务ID：`incrementTransactionId()`, `getTransactionId()`, `setTransactionId()` 用于网络防重放

**依赖项**:
- `core/Types.hpp`
- `item/ItemStack.hpp`
- `ContainerTypes.hpp`
- `Slot.hpp`, `PlayerInventory.hpp`

## 模块关系图

```
                    IInventory (接口)
                         ▲
                         │
        ┌────────────────┼────────────────┐
        │                │                │
 PlayerInventory   CraftingInventory  CraftResultInventory
        │                │
        │                │
        └────────┬───────┘
                 │
            CreativeInventory
                │
                │
              Slot (引用 IInventory)
                 │
                 │
             Container
                 │
                 │
      AbstractContainerMenu
                 │
         ┌───────┴───────┐
         │               │
   PlayerContainer   其他容器菜单
```

## 整体职责

1. **背包接口抽象** (`IInventory`): 定义统一的背包操作接口，支持不同类型的背包容器

2. **槽位管理** (`Slot`, `Container`): 提供槽位的抽象表示，支持UI显示和物品交互

3. **玩家背包** (`PlayerInventory`): 实现玩家专属背包，包含快捷栏、主背包、护甲、副手

4. **创造模式物品库** (`CreativeInventory`): 生成创造模式条目并填充初始背包

5. **合成系统支持** (`CraftingInventory`, `CraftResultInventory`): 提供合成网格和结果槽位

6. **容器菜单** (`AbstractContainerMenu`): 管理客户端-服务端的背包同步和交互逻辑

## 输入和输出

### 输入
- 玩家点击操作（鼠标按钮、点击类型）
- 物品堆（`ItemStack`）
- 网络同步数据（序列化/反序列化）

### 输出
- 更新后的背包状态
- 物品变更事件通知
- 序列化数据（用于网络同步）

## 依赖项

### 内部依赖
- `core/Types.hpp` - 基础类型定义
- `core/Result.hpp` - 错误处理
- `item/ItemStack.hpp` - 物品堆类型
- `item/Item.hpp` - 物品基类
- `network/packet/PacketSerializer.hpp` - 网络序列化

### 外部依赖
- C++20 标准库（`<array>`, `<vector>`, `<functional>`, `<memory>`）

## 使用方法

### 创建玩家背包

```cpp
#include "entity/inventory/PlayerInventory.hpp"

// 创建空背包
mc::PlayerInventory inventory(nullptr);

// 添加物品
mc::ItemStack diamonds(*diamondItem, 32);
inventory.add(diamonds);

// 获取当前选中物品
mc::ItemStack selected = inventory.getSelectedStack();

// 获取护甲
mc::ItemStack helmet = inventory.getHelmet();
```

### 创建合成网格

```cpp
#include "entity/inventory/CraftingInventory.hpp"

// 创建 3x3 工作台网格
mc::CraftingInventory grid(3, 3);

// 设置物品
grid.setItemAt(0, 0, ItemStack(*planksItem, 1));
grid.setItemAt(1, 0, ItemStack(*planksItem, 1));

// 检测合成结果
// ...

// 清空网格
grid.clear();
```

### 创建创造模式背包

```cpp
#include "entity/inventory/CreativeInventory.hpp"

mc::PlayerInventory inventory;
mc::fillCreativeModeInventory(inventory);

auto paletteEntries = mc::buildCreativePaletteEntries();
```

### 创建容器

```cpp
#include "entity/inventory/Container.hpp"

// 创建容器
mc::Container container(mc::ContainerType::Chest, 0);

// 添加槽位
container.addSlot(std::make_unique<mc::Slot>(&inventory, 0, 10, 10));

// 处理点击
mc::ItemStack cursorItem = ...;
cursorItem = container.clicked(slotIndex, button, mc::ClickType::Pick, cursorItem);
```

### 使用容器菜单

```cpp
#include "entity/inventory/AbstractContainerMenu.hpp"

class MyContainerMenu : public mc::AbstractContainerMenu {
public:
    MyContainerMenu(mc::ContainerId id, mc::PlayerInventory* playerInventory)
        : mc::AbstractContainerMenu(id, playerInventory) {
        // 添加自定义槽位
        addSlot(std::make_unique<mc::Slot>(&myInventory, 0, 10, 10));
        
        // 添加玩家背包槽位
        addPlayerInventorySlots(8, 84);
        addPlayerHotbarSlots(8, 142);
    }
    
    bool stillValid(const mc::Player& player) const override {
        return true;
    }
};
```

## 容易踩的坑

### 1. 槽位索引混淆

玩家背包有41个槽位，索引分布容易混淆：
- 快捷栏：0-8（不是9个连续槽位从1开始）
- 主背包：9-35（27个槽位）
- 护甲：36-39（头、胸、腿、脚）
- 副手：40

**建议**: 使用 `InventorySlots` 命名空间中的常量，避免硬编码数字。

```cpp
// 错误
inventory.getItem(36); // 什么是36？

// 正确
inventory.getItem(mc::InventorySlots::ARMOR_HEAD); // 清晰表示头盔槽
```

### 2. ItemStack 可能为空

`ItemStack::EMPTY` 是特殊的空物品堆，检查时应该使用 `isEmpty()` 方法：

```cpp
// 错误
if (stack.getItem() == nullptr) { ... }

// 正确
if (stack.isEmpty()) { ... }
```

### 3. 物品堆叠上限

不同物品有不同的堆叠上限（通常64，但有些是1或16）：

```cpp
i32 maxStack = stack.getMaxStackSize();
i32 space = maxStack - existing.getCount();
```

### 4. 物品合并检查

不是所有物品都可以合并，需要检查是否完全相同（包括NBT）：

```cpp
// 错误
if (stack1.getItem() == stack2.getItem()) { /* 可能不够 */ }

// 正确
if (stack1.canMergeWith(stack2)) { /* 考虑了所有因素 */ }
```

### 5. 容器槽位范围

在处理 Shift+点击快速移动时，需要正确设置槽位范围：

```cpp
container.setPlayerInventoryRange(0, 36);   // 快捷栏+主背包
container.setContainerInventoryRange(36, 41); // 护甲+副手
```

### 6. 网络同步

背包状态变更后需要手动触发同步：

```cpp
inventory.setChanged();  // 标记变更
container.setChanged();  // 通知容器
container.broadcastChanges();  // 广播到客户端
```

### 7. 护甲槽位限制

`ArmorSlot` 通过 `mayPlace()` 方法检查护甲类型，头盔/胸甲/护腿/靴子只能放入对应槽位。

### 8. 物品丢弃（Future Work）

`AbstractContainerMenu` 和 `Container` 中的 `handleThrow()` 方法当前仅更新背包状态，不生成物品实体。

**待实现功能**：
- `AbstractContainerMenu.cpp:131` - 点击屏幕外部丢弃鼠标物品
- `AbstractContainerMenu.cpp:328` - Q键丢弃槽位物品
- `Container.cpp:504,512,525` - 丢弃物品

**设计方案**：
```cpp
// 添加丢弃回调接口到 AbstractContainerMenu
using ItemDropCallback = std::function<void(const ItemStack& stack, const Player& player)>;
void setItemDropCallback(ItemDropCallback callback);

// 上层（ServerWorld/IntegratedServer）注入实现
menu->setItemDropCallback([this, player](const ItemStack& stack, const Player& p) {
    BlockDropHandler::spawnDrops(*entityManager, nullptr, player.blockPos(), {stack}, p.uuid());
});
```

此功能依赖 World/EntityManager 集成，标记为后续迭代任务。

### 9. 坐标转换

`CraftingInventory` 使用行优先存储，坐标转换时注意：
- `posToSlot(x, y)` = `y * width + x`
- `slotToPos(slot)` = `(slot % width, slot / width)`

### 9. 创造库存初始化顺序

`CreativeInventory` 依赖运行时注册表完整可用，测试或启动代码必须按下面顺序初始化：

```cpp
VanillaBlocks::initialize();
Items::initialize();
BlockItemRegistry::instance().initializeVanillaBlockItems();
```

如果少了第一步，`BlockItemRegistry` 可能只能看到空方块指针，创造物品库就会退化成空列表。

## 涉及的测试用例

### tests/common/test_inventory.cpp

- `InventorySlotsTest`: 槽位索引常量验证
- `PlayerInventoryTest`: 玩家背包完整测试
  - 初始状态测试
  - 物品设置和获取
  - 快捷栏操作
  - 物品移除
  - 背包清空
  - 物品添加和合并
  - 物品查找和统计
  - 槽位交换
  - 护甲和副手槽操作
  - 序列化/反序列化
- `SlotTest`: 槽位基础测试

### tests/common/entity/inventory/CraftingInventoryTest.cpp

- `CraftingInventoryTest`: 合成网格测试
  - 不同尺寸网格创建（1x1, 2x2, 3x3）
  - 坐标转换测试
  - 物品操作测试
  - 内容变更回调
  - 边界计算
- `CraftResultInventoryTest`: 合成结果背包测试
  - 单槽位操作
  - 结果设置和获取

### tests/common/test_container.cpp

- `CreativeInventoryTest`: 创造模式物品库测试
  - 创造条目生成
  - 创造模式初始背包填充

## 参考

- Minecraft Java 1.16.5 源码: `net.minecraft.inventory.IInventory`, `net.minecraft.inventory.container.Container`, `net.minecraft.entity.player.PlayerInventory`
