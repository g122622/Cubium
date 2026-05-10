# Inventory 模块

本模块实现了 Minecraft 的物品存储和容器交互系统，参考 MC Java 1.16.5 的背包系统架构。

## MC 1.16.5 对齐状态

本模块已完成与 MC 1.16.5 的核心对齐：

- ✅ **IRecipeHolder/IRecipeHelperPopulator** - 配方追踪和配方书支持
- ✅ **IntReferenceHolder** - 整型数据同步器（熔炉进度、酿造时间等）
- ✅ **AnvilContainer** - 完整的修复成本计算和附魔合并算法
- ✅ **EnchantmentContainer** - 书架力量计算、附魔等级公式、多附魔生成
- ✅ **AbstractContainerMenu** - 拖拽分发状态机、变化检测优化、物品丢弃回调
- ✅ **网络协议** - OpenContainerPacket、ContainerContentPacket 字段类型
- ✅ **ISidedInventory/ISidedInventoryProvider** - 侧面背包接口，用于漏斗等定向传输
- ✅ **Slot回调** - onTake、onSwapCraft、onCrafting 回调（对齐 MC 1.16.5）
- ✅ **FurnaceFuelSlot/FurnaceResultSlot** - 特殊槽位实现
- ✅ **stillValid距离检查** - isWithinDistance() 方法用于容器访问距离验证
- ✅ **容器槽位坐标** - 所有容器槽位坐标已与 MC 1.16.5 对齐
- ✅ **创造模式特殊权限** - 铁砧和附魔台的创造模式玩家特殊权限已实现
- ✅ **PlayerInventory::isUsableByPlayer** - 使用 Entity::isAlive() 检查玩家存活状态

## 目录结构

```
inventory/
├── IInventory.hpp              # 背包接口（抽象基类）
├── ISidedInventory.hpp         # 侧面背包接口
├── ISidedInventoryProvider.hpp # 侧面背包提供者接口
├── IRecipeHolder.hpp           # 配方持有者接口
├── IRecipeHolder.cpp           # 配方持有者实现
├── IRecipeHelperPopulator.hpp  # 配方辅助填充器接口
├── ContainerTypes.hpp          # 容器相关类型定义
├── Slot.hpp                    # 槽位类
├── Slot.cpp
├── PlayerInventory.hpp         # 玩家背包
├── PlayerInventory.cpp
├── CraftingInventory.hpp       # 合成网格背包（实现 IRecipeHelperPopulator）
├── CraftingInventory.cpp
├── CreativeInventory.hpp       # 创造模式物品库辅助
├── CreativeInventory.cpp
├── AbstractContainerMenu.hpp   # 容器菜单基类
└── AbstractContainerMenu.cpp
```

## 文件详细介绍

### 1. IInventory.hpp / IInventory.cpp

**职责**: 定义背包接口，所有背包容器的抽象基类。

**主要内容**:
- 纯虚函数定义背包基本操作
- 容量查询：`getContainerSize()`, `isEmpty()`, `getMaxStackSize()`
- 物品操作：`getItem()`, `setItem()`, `removeItem()`, `removeItemNoUpdate()`
- 容器操作：`clear()`, `setChanged()`
- 物品查找：`getFirstEmptySlot()`, `countItem()`, `hasItem()`, `findSlot()`, `canPlaceItem()`
- 序列化支持
- **新增方法**（MC 1.16.5 对齐）：
  - `isUsableByPlayer()`: 检查玩家是否可以访问此背包（默认返回 true）
  - `openInventory()`: 打开背包时的回调（默认空实现）
  - `closeInventory()`: 关闭背包时的回调（默认空实现）
  - `hasAny()`: 检查是否包含指定物品集合中的任意物品

**依赖项**:
- `core/Types.hpp` - 基础类型
- `item/ItemStack.hpp` - 物品堆类型

### 2. IRecipeHolder.hpp

**职责**: 配方持有者接口，用于追踪当前使用的配方。

**主要内容**:
- `setRecipeUsed()` - 设置当前使用的配方
- `getRecipeUsed()` - 获取当前使用的配方

**实现类**: `CraftResultInventory`

### 3. IRecipeHelperPopulator.hpp

**职责**: 配方辅助填充器接口，用于配方书查找可用配方。

**主要内容**:
- `fillStackedContents()` - 将背包内容填充到物品计数器

**实现类**: `CraftingInventory`

### 4. ContainerTypes.hpp

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
  - 回调方法：`onTake()`, `onCrafting()`, `onSwapCraft()`, `onSlotChange()`
- `ArmorSlot` 类：护甲槽位，只接受对应类型护甲
  - `mayPlace()`: 检查护甲类型是否匹配槽位
  - `mayPickup()`: 检查护甲是否可取下（MC 1.16.5 对齐）
    - 空槽位总是可取
    - 创造模式玩家可取下任何护甲
    - 绑定诅咒附魔的护甲在非创造模式下无法取下
    - 普通护甲可正常取下
- `ResultSlot` 类：合成结果槽位，只能取出不能放入
  - `mayPlace()`: 始终返回 false
  - `onCrafting()`: 触发配方解锁通知（通过 `IRecipeHolder::onCrafting`）
  - `onTake()`: 触发合成完成事件
  - **MC 1.16.5 配方解锁机制**:
    - 当 `m_inventory` 实现 `IRecipeHolder` 接口时，自动调用 `onCrafting(player)`
    - 用于解锁配方到配方书（需配方书系统完善后完全生效）
  - **材料消耗说明**: 由 `CraftingMenu.handleResultSlotClick()` 和 `quickMoveStack()` 处理
    - 调用 `consumeIngredients()` 消耗材料
    - 通过 `recipe->getRemainingItems()` 处理剩余物品（如水桶->空桶）
- `FurnaceFuelSlot` 类：熔炉燃料槽位，只接受燃料物品
  - `mayPlace()`: 检查物品是否为有效燃料或桶类物品
  - `isFuel()`: 静态方法检查物品燃料属性（委托给 `AbstractFurnaceEntity::isFuel()`）
  - `isBucket()`: 检查是否为桶类物品（空桶、水桶、岩浆桶等所有桶类型）
  - `getMaxStackSize()`: 桶类物品返回 1，其他返回默认值
- `FurnaceResultSlot` 类：熔炉输出槽位
  - `mayPlace()`: 始终返回 false，不能放入物品
  - `remove()`: 重写以支持经验值累积追踪
  - `onTake()`: 触发熔炼成就和经验奖励
  - `onCrafting()`: 从熔炉实体提取累积经验并发放给玩家
  - 构造函数参数：玩家指针、背包指针、槽位索引、显示坐标、熔炉实体指针
  - `setFurnaceEntity()` / `getFurnaceEntity()`: 动态设置/获取熔炉实体
  - **MC 1.16.5 经验发放机制**:
    - 熔炉在熔炼过程中累积经验值（存储在 `AbstractFurnaceEntity::m_storedExperience`）
    - 玩家从输出槽取出物品时，`FurnaceResultSlot` 自动发放累积经验
    - 支持两种取出方式：普通点击取出（通过 `remove()` 追踪数量）和 Shift+快速移动（通过 `onTake()` 自动设置数量）
    - 经验值向下取整后发放（使用 `std::floor`）

**依赖项**:
- `IInventory.hpp` - 背包接口
- `item/ItemStack.hpp` - 物品堆

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
- **新增方法**（MC 1.16.5 对齐）：
  - `tick()`: 背包 tick 处理，调用所有物品的 `inventoryTick()` 和护甲的 `onArmorTick()`
  - `dropAllItems()`: 丢弃所有物品（使用 `Player::dropItem()` 生成物品实体）
  - `deleteStack()`: 删除指定物品堆
  - `placeItemBackInInventory()`: 将物品放回背包
  - `damageArmor(DamageSource&, float)`: 护甲损伤处理，火焰伤害不损坏可燃烧护甲
  - `getDestroySpeed(const BlockState&)`: 获取当前手持物品的挖掘速度
  - `containsAny()`: 检查是否包含指定物品集合中的任意物品
  - `isUsableByPlayer()`: 检查玩家是否可用（覆盖 IInventory 接口）

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
- **Slot 回调触发**（MC 1.16.5 对齐）：
  - `handleClickPick()`: 在拾取和交换场景中调用 `slot->onTake()` 回调
  - `handleQuickMove()`: 在快速移动完成后调用 `slot->onTake()` 回调
  - 这确保了 `FurnaceResultSlot` 等特殊槽位的回调（如经验发放）能正确触发

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
        AbstractContainerMenu
                 │
         ┌───────┴───────┐
         │               │
   CraftingMenu    其他容器菜单
```

## 整体职责

1. **背包接口抽象** (`IInventory`): 定义统一的背包操作接口，支持不同类型的背包容器

2. **槽位管理** (`Slot`, `AbstractContainerMenu`): 提供槽位的抽象表示，支持UI显示和物品交互

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

// 处理点击
mc::ItemStack cursorItem = ...;
cursorItem = menu.clicked(slotIndex, button, mc::ClickType::Pick, player);
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

### 5. 容器菜单槽位管理

`AbstractContainerMenu` 内部管理玩家背包槽位范围，Shift+点击快速移动会自动处理：

```cpp
// AbstractContainerMenu 内部已正确设置 m_playerInvStart/m_playerInvEnd
// 子类只需调用 addPlayerInventorySlots() 和 addPlayerHotbarSlots()
```

### 6. 网络同步

背包状态变更后需要手动触发同步：

```cpp
inventory.setChanged();  // 标记变更
menu.broadcastChanges();  // 广播到客户端
```

### 7. 护甲槽位限制

`ArmorSlot` 通过 `mayPlace()` 方法检查护甲类型，头盔/胸甲/护腿/靴子只能放入对应槽位。

### 8. 物品丢弃

`AbstractContainerMenu` 中的物品丢弃功能已通过回调实现。

**使用方法**：
```cpp
// 上层（ServerWorld/IntegratedServer）注入物品丢弃回调
menu->setItemDropCallback([this, player](const ItemStack& stack, Player& p, bool retainOwnership) {
    // 在世界中生成物品实体
    spawnItemEntity(world, p.blockPos(), stack, retainOwnership);
});

// AbstractContainerMenu 内部会自动调用
// - Q键丢弃槽位物品：handleThrow()
// - 点击屏幕外部丢弃鼠标物品：clicked() 中 ClickType::Throw
// - 容器关闭时丢弃无法放入背包的物品：removed()
```

**回调参数**：
- `stack`: 要丢弃的物品堆
- `player`: 触发丢弃的玩家
- `retainOwnership`: 是否保留所有权（如创造模式删除物品）

### 9. 坐标转换

`CraftingInventory` 使用行优先存储，坐标转换时注意：
- `posToSlot(x, y)` = `y * width + x`
- `slotToPos(slot)` = `(slot % width, slot / width)`

### 10. 创造库存初始化顺序

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
- `PlayerInventoryNewMethodsTest`: 玩家背包新方法测试
  - `containsAny()`: 检查是否包含指定物品集合
  - `isUsableByPlayer()`: 玩家可用性检查
- `SlotTest`: 槽位基础测试
  - `BasicOperations`: 槽位基本操作
  - `MaxStackSize`: 最大堆叠数
  - `MayPlace`: 放置检查
  - `ArmorSlotOnlyAcceptsMatchingArmorType`: 护甲槽位类型检查
  - `ArmorSlotMayPickupReturnsTrueForEmptySlot`: 空槽位可拾取
  - `ArmorSlotMayPickupReturnsTrueForCreativePlayer`: 创造模式可取下任何护甲
  - `ArmorSlotMayPickupReturnsTrueForNormalArmor`: 普通护甲可取下
  - `ArmorSlotMayPickupReturnsFalseForBindingCurseArmor`: 绑定诅咒护甲生存模式不可取下
  - `ArmorSlotMayPickupWithMultipleEnchantments`: 多附魔护甲绑定诅咒检查
- `IInventoryInterfaceTest`: IInventory 接口测试
  - `hasAny()`: 接口方法测试

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

### tests/common/entity/inventory/container/FurnaceContainerTest.cpp

- `FurnaceContainerTest`: 熔炉容器测试
  - 槽位数量验证
  - 容器类型验证
  - 槽位索引常量验证
- `FurnaceFuelSlotTest`: 熔炉燃料槽位测试
  - 创建槽位
  - `mayPlace()`: 只接受燃料物品和桶类物品
  - `getMaxStackSize()`: 燃料物品堆叠上限，桶类物品堆叠上限为 1
  - `isFuel()`: 静态方法测试（委托给 `AbstractFurnaceEntity::isFuel()`）
  - `isBucket()`: 检查所有桶类型（空桶、水桶、岩浆桶、鱼桶、牛奶桶）
  - 各种燃料类型检测（煤炭、木炭、烈焰棒、岩浆桶、木棍等）
- `FurnaceResultSlotTest`: 熔炉输出槽位测试
  - 创建槽位
  - `mayPlace()`: 始终返回 false
  - `remove()`: 物品移除和经验累积追踪
- `FurnaceResultSlotExperienceTest`: 熔炉输出槽位经验发放测试（MC 1.16.5）
  - `OnTake_WithFurnaceEntity_GrantsExperience`: 从输出槽取出物品时发放经验
  - `OnTake_NoFurnaceEntity_NoExperienceGranted`: 无熔炉实体时不发放经验
  - `OnTake_NoPlayer_NoExperienceGranted`: 无玩家时不发放经验
  - `OnTake_NoStoredExperience_NoExperienceGranted`: 无累积经验时不发放
  - `Remove_TracksRemoveCount`: `remove()` 正确追踪取出数量
  - `SetFurnaceEntity_UpdatesEntityReference`: 动态设置熔炉实体引用
  - `OnCrafting_CalledFromOnTake`: `onTake()` 内部正确调用 `onCrafting()`
  - `MultipleRemoves_ThenOnTake`: 多次 `remove()` 后一次性 `onTake()` 的场景
  - `ExperienceRoundedDown`: 经验值向下取整（`floor(10.7) = 10`）
  - `ZeroStoredExperience_NoEffect`: 累积经验为 0 时无效果
  - `OnTakeWithEmptySlot_NoEffect`: 从空槽位取出无效果

### tests/common/test_container.cpp

- `AbstractContainerMenuTest`: 容器菜单测试
  - Shift+点击快速移动
  - 数字键交换
  - 拖拽分发
  - 双击拾取全部
  - 创造模式复制
- `ContainerPacketTest`: 容器包测试
  - ContainerContentPacket 序列化/反序列化
  - ContainerSlotPacket 序列化/反序列化
  - ContainerClickPacket 序列化/反序列化
  - 点击类型映射
  - 热栏选择包
  - 玩家背包包
  - 创造模式背包操作包
- `CreativeInventoryTest`: 创造模式物品库测试
  - 创造条目生成
  - 创造模式初始背包填充

## 参考

- Minecraft Java 1.16.5 源码: `net.minecraft.inventory.IInventory`, `net.minecraft.inventory.container.AbstractContainerMenu`, `net.minecraft.entity.player.PlayerInventory`
