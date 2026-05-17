# BlockEntity 模块

方块实体系统，用于存储方块状态无法表示的复杂数据。

## 目录结构

```
blockentity/
├── BlockEntity.hpp              # 方块实体基类
├── BlockEntity.cpp              # 基类实现
├── BlockEntityType.hpp          # 方块实体类型枚举
├── BlockEntityType.cpp          # 类型转换函数
├── ContainerBlockEntity.hpp     # 容器方块实体基类
├── ContainerBlockEntity.cpp     # 容器方块实体实现
├── CraftingTableEntity.hpp      # 工作台方块实体
├── CraftingTableEntity.cpp      # 工作台实现
├── BLOCK_ENTITY_PLAN.md         # 方块实体补全计划
├── core/                        # 核心基础设施
│   ├── BlockEntityRegistry.hpp/cpp  # 方块实体注册表
│   ├── LockableBlockEntity.hpp/cpp  # 可锁定容器基类
│   ├── SimpleInventory.hpp/cpp      # 简单背包实现
│   └── README.md
├── storage/                     # 存储类方块实体
│   ├── ChestEntity.hpp/cpp          # 箱子实体
│   ├── TrappedChestEntity.hpp/cpp   # 陷阱箱实体
│   ├── DoubleSidedInventory.hpp/cpp # 双箱合并容器
│   ├── EnderChestEntity.hpp/cpp     # 末影箱实体
│   ├── ShulkerBoxEntity.hpp/cpp     # 潜影盒实体
│   ├── BarrelEntity.hpp/cpp         # 木桶实体
│   └── README.md
├── transport/                   # 传输类方块实体
│   ├── IHopper.hpp/cpp              # 漏斗接口
│   ├── HopperEntity.hpp/cpp         # 漏斗实体
│   └── README.md
├── processing/                  # 加工类方块实体
│   ├── AbstractFurnaceEntity.hpp/cpp # 熔炉基类
│   ├── FurnaceEntity.hpp/cpp         # 普通熔炉
│   ├── BlastFurnaceEntity.hpp/cpp    # 高炉
│   ├── SmokerEntity.hpp/cpp          # 烟熏炉
│   ├── FurnaceInventory.hpp/cpp      # 熔炉背包
│   ├── BrewingStandEntity.hpp/cpp    # 酿造台
│   ├── BeaconEntity.hpp/cpp          # 信标
│   └── README.md
├── redstone/                    # 红石类方块实体
│   ├── CommandBlockEntity.hpp/cpp   # 命令方块实体
│   ├── ComparatorEntity.hpp/cpp     # 比较器实体
│   ├── DaylightDetectorEntity.hpp/cpp # 日光探测器实体
│   └── README.md
└── interactive/                 # 交互类方块实体
    ├── EnchantingTableEntity.hpp/cpp # 附魔台
    ├── PistonBlockEntity.hpp/cpp     # 活塞实体
    ├── DispenserBlockEntity.hpp/cpp  # 发射器/投掷器实体
    ├── SignEntity.hpp/cpp            # 告示牌
    ├── BannerEntity.hpp/cpp          # 旗帜
    ├── JukeboxEntity.hpp/cpp         # 唱片机
    ├── LecternEntity.hpp/cpp         # 讲台
    └── README.md
```

## 文件详解

### BlockEntityType.hpp / BlockEntityType.cpp

**职责**：定义所有已知的方块实体类型枚举，提供类型与资源位置ID之间的双向转换。

**主要内容**：
- `BlockEntityType` 枚举：定义了 38 种方块实体类型
  - 存储类：Chest, TrappedChest, EnderChest, ShulkerBox, Barrel
  - 工作类：CraftingTable, Furnace, BlastFurnace, Smoker, BrewingStand, Anvil, Grindstone, Stonecutter, SmithingTable, Loom, CartographyTable
  - 红石类：Dispenser, Dropper, Hopper, Piston, Observer, Comparator, DaylightDetector
  - 标识类：Sign, Banner, StructureBlock, JigsawBlock
  - 其他：Beacon, Bed, Bell, CommandBlock, EnchantingTable, EndGateway, EndPortal, MobSpawner, Skull, Beehive, Campfire, Conduit, Lectern
- `blockEntityTypeToId()`：枚举值转资源位置ID（如 `BlockEntityType::Chest` → `minecraft:chest`）
- `blockEntityTypeFromId()`：资源位置ID转枚举值，支持简写形式（如 `chest` 等价于 `minecraft:chest`）

### BlockEntity.hpp / BlockEntity.cpp

**职责**：方块实体基类，定义所有方块实体的通用接口和行为。

**主要内容**：
- 类型与位置管理：`getType()`, `getPos()`
- 数据持久化：`load()`, `save()` - 使用 JSON 格式
- 生命周期：`tick()`, `needsTick()` - 用于需要定期更新的方块实体（如熔炉）
- 修改追踪：`setChanged()`, `isChanged()`, `clearChanged()` - 触发区块保存
- 自定义名称：`getCustomName()`, `setCustomName()` - 支持重命名
- 克隆：`clone()` - 纯虚函数，子类必须实现

**线程安全说明**：
- `tick()` 方法可能在服务器线程调用
- `load()`/`save()` 可能在世界保存线程调用
- 需要子类自行处理线程同步

### ContainerBlockEntity.hpp / ContainerBlockEntity.cpp

**职责**：为拥有背包的方块实体提供通用功能，继承自 `BlockEntity`。

**主要内容**：
- 背包管理：`getInventory()`, `getContainerSize()`, `isEmpty()`, `clearContainer()`
- 打开计数：`openContainer()`, `closeContainer()`, `getOpenCount()` - 用于音效和红石信号
  - MC 1.16.5 对齐：观察者模式玩家不计入打开数
  - 负数保护：防止计数数据损坏
- 数据持久化：重写 `load()`/`save()`，增加物品和自定义名称的保存

**子类**：箱子、漏斗、工作台、熔炉等所有有背包的方块实体

### CraftingTableEntity.hpp / CraftingTableEntity.cpp

**职责**：工作台方块实体，存储玩家在工作台中放置的物品和当前匹配的配方。

**主要内容**：
- 合成网格：3x3 的 `CraftingInventory`
- 结果槽位：`CraftResultInventory`
- 配方匹配：`updateCraftingResult()` - 查找匹配配方并更新结果
- 执行合成：`craft()` - 消耗原料并返回结果
- 清空：`clear()` - 清除网格和结果槽位

**注意**：与原版 MC 不同，此实现保留物品是为了支持未来的连续合成功能。原版 MC 中工作台关闭后物品会弹出。

## 子模块概览

### core/ - 核心基础设施

提供方块实体的基础功能组件：

- **BlockEntityRegistry** - 方块实体注册表，工厂方法创建方块实体
- **LockableBlockEntity** - 可锁定容器基类，支持自定义名称和钥匙锁定
- **SimpleInventory** - 通用背包实现，用于箱子、漏斗等容器

详见 [core/README.md](core/README.md)

### storage/ - 存储类方块实体

提供箱子、陷阱箱等存储类方块实体：

- **ChestEntity** - 箱子实体，27格存储，双箱合并，战利品表填充
- **TrappedChestEntity** - 陷阱箱，输出红石信号
- **DoubleSidedInventory** - 双箱合并容器，54格

详见 [storage/README.md](storage/README.md)

### transport/ - 传输类方块实体

提供漏斗等传输类方块实体：

- **IHopper** - 漏斗接口，统一处理漏斗方块和漏斗矿车
- **HopperEntity** - 漏斗实体，物品传输机制，支持 ISidedInventoryProvider

详见 [transport/README.md](transport/README.md)

### processing/ - 加工类方块实体

提供熔炉、高炉、烟熏炉等加工类方块实体：

- **AbstractFurnaceEntity** - 熔炉基类，燃烧和熔炼逻辑
- **FurnaceEntity** - 普通熔炉，200tick熔炼
- **BlastFurnaceEntity** - 高炉，100tick熔炼矿石
- **SmokerEntity** - 烟熏炉，100tick烹饪食物
- **FurnaceInventory** - 熔炉专用3槽背包

详见 [processing/README.md](processing/README.md)

### interactive/ - 交互类方块实体

提供附魔台、活塞等交互类方块实体：

- **EnchantingTableEntity** - 附魔台，书本动画
- **PistonBlockEntity** - 活塞，方块移动动画
- **DispenserBlockEntity** - 发射器方块实体基类，物品发射，战利品表填充
- **DropperBlockEntity** - 投掷器方块实体，继承自DispenserBlockEntity

详见 [interactive/README.md](interactive/README.md)

### redstone/ - 红石类方块实体

提供比较器、日光探测器、命令方块等红石相关方块实体：

- **CommandBlockEntity** - 命令方块，存储命令并执行，支持三种模式（脉冲、循环、连锁）
- **ComparatorEntity** - 比较器，存储输出信号强度
- **DaylightDetectorEntity** - 日光探测器，定期更新信号

详见 [redstone/README.md](redstone/README.md)

## 类继承关系

```
BlockEntity (基类)
│
├── ContainerBlockEntity (容器基类)
│   │
│   ├── LockableBlockEntity (可锁定容器基类)
│   │   │
│   │   ├── LootableContainerBlockEntity (可填充战利品表的容器基类)
│   │   │   │
│   │   │   ├── ChestEntity (箱子)
│   │   │   │   └── TrappedChestEntity (陷阱箱)
│   │   │   │
│   │   │   ├── BarrelEntity (木桶)
│   │   │   │
│   │   │   ├── ShulkerBoxEntity (潜影盒)
│   │   │   │
│   │   │   └── DispenserBlockEntity (发射器)
│   │   │       └── DropperBlockEntity (投掷器)
│   │   │
│   │   ├── HopperEntity (漏斗)
│   │   │
│   │   └── AbstractFurnaceEntity (熔炉基类)
│   │       ├── FurnaceEntity (普通熔炉)
│   │       ├── BlastFurnaceEntity (高炉)
│   │       └── SmokerEntity (烟熏炉)
│   │
│   └── CraftingTableEntity (工作台)
│
├── EnchantingTableEntity (附魔台)
│
├── PistonBlockEntity (活塞)
│
├── CommandBlockEntity (命令方块) [实现 ICommandSource]
│
├── ComparatorEntity (比较器)
│
└── DaylightDetectorEntity (日光探测器)
```

## 文件关系图

```
BlockEntityType (枚举)
       ↑
       │ 使用
       │
  BlockEntity (基类)
       ↑
       │ 继承
       │
ContainerBlockEntity (容器基类)
       ↑
       │ 继承
       │
 CraftingTableEntity (工作台)
       │
       ├─依赖→ CraftingInventory (合成网格)
       ├─依赖→ CraftResultInventory (结果槽)
       └─依赖→ RecipeManager (配方管理)
```

## 模块职责

### 整体职责

1. **数据存储**：存储方块状态无法表示的复杂数据（背包内容、工作状态、自定义文本等）
2. **生命周期管理**：管理方块实体的创建、更新和销毁
3. **数据持久化**：提供 JSON 格式的序列化/反序列化
4. **容器功能**：为有背包的方块实体提供统一的容器接口

### 输入

- 区块数据（JSON 格式）→ `load()`
- 玩家交互 → 子类处理
- 配方匹配请求 → `updateCraftingResult()`

### 输出

- 方块实体数据（JSON 格式）← `save()`
- 合成结果 ← `craft()`
- 容器状态查询 ← `getInventory()`, `isEmpty()`, `getOpenCount()`

## 依赖项

### 外部依赖
- `nlohmann/json` - JSON 序列化
- `<memory>` - 智能指针

### 内部依赖
- `world/block/BlockPos.hpp` - 方块位置
- `resource/ResourceLocation.hpp` - 资源位置
- `entity/inventory/IInventory.hpp` - 背包接口
- `entity/inventory/CraftingInventory.hpp` - 合成背包
- `item/crafting/RecipeManager.hpp` - 配方管理

## 使用方法

### 创建自定义方块实体

```cpp
#include "world/blockentity/BlockEntity.hpp"

class MyBlockEntity : public BlockEntity {
public:
    explicit MyBlockEntity(const BlockPos& pos)
        : BlockEntity(BlockEntityType::MyType, pos)
        , m_data(0) {}

    // 实现数据持久化
    bool load(const nlohmann::json& data) override {
        if (!BlockEntity::load(data)) return false;
        m_data = data.value("my_data", 0);
        return true;
    }

    void save(nlohmann::json& data) const override {
        BlockEntity::save(data);
        data["my_data"] = m_data;
    }

    // 如果需要每 tick 更新
    bool needsTick() const override { return true; }
    void tick(World& world) override {
        // 处理逻辑
    }

    std::unique_ptr<BlockEntity> clone() const override {
        return std::make_unique<MyBlockEntity>(m_pos);
    }

private:
    i32 m_data;
};
```

### 创建容器方块实体

```cpp
#include "world/blockentity/ContainerBlockEntity.hpp"

class MyContainerEntity : public ContainerBlockEntity {
public:
    explicit MyContainerEntity(const BlockPos& pos)
        : ContainerBlockEntity(BlockEntityType::MyType, pos)
        , m_inventory(27) {} // 27 格背包

    IInventory* getInventory() override { return &m_inventory; }
    const IInventory* getInventory() const override { return &m_inventory; }
    i32 getContainerSize() const override { return 27; }

    std::unique_ptr<BlockEntity> clone() const override {
        return std::make_unique<MyContainerEntity>(m_pos);
    }

private:
    SimpleInventory m_inventory;
};
```

### 使用工作台方块实体

```cpp
// 创建工作台
CraftingTableEntity table(BlockPos(0, 0, 0));

// 放置物品到网格
table.getCraftingGrid().setItem(0, ItemStack(Items::DIAMOND, 1));
table.getCraftingGrid().setItem(1, ItemStack(Items::STICK, 1));

// 更新合成结果
if (table.updateCraftingResult()) {
    // 找到匹配的配方
    auto recipe = table.getCurrentRecipe();
    ItemStack result = table.getResultInventory().getItem(0);
}

// 执行合成
ItemStack crafted = table.craft();
```

## 容易踩的坑

### 1. 忘记调用 setChanged()

修改方块实体数据后必须调用 `setChanged()` 触发区块保存，否则数据可能丢失。

```cpp
// 错误示例
table.getCraftingGrid().setItem(0, item);
// 忘记 setChanged()，数据不会保存

// 正确示例
table.getCraftingGrid().setItem(0, item);
table.setChanged();
```

### 2. getBlockState() 尚未实现

当前 `BlockEntity::getBlockState()` 返回 `nullptr`，需要 World 类支持后才能实现。使用前需要检查返回值。

### 3. 线程安全问题

`tick()` 和 `load()`/`save()` 可能在不同线程调用，需要注意：
- 熔炉等需要 tick 的方块实体应该使用互斥锁保护数据
- 静态方块实体（如箱子）可以返回 `needsTick() == false` 避免不必要的开销

### 4. 打开计数下溢

`closeContainer()` 不会让计数变为负数，但不匹配的 open/close 调用会导致计数错误。确保每次 open 都有对应的 close。

### 5. 工作台物品持久化

当前 `CraftingTableEntity::save()` 不保存网格内容（与原版行为一致），如需持久化需要在子类中重写。

### 6. 类型转换失败处理

`blockEntityTypeFromId()` 对未知类型返回 `BlockEntityType::Unknown`，需要处理这种情况：

```cpp
BlockEntityType type = blockEntityTypeFromId(id);
if (type == BlockEntityType::Unknown) {
    // 处理未知类型
}
```

## 测试用例

测试文件位于 `tests/common/world/blockentity/BlockEntityTest.cpp`，包含以下测试：

### BlockEntityType 测试
- `BlockEntityType_ToId_KnownTypes` - 已知类型转 ID
- `BlockEntityType_ToId_UnknownReturnsUnknown` - 未知类型返回 "minecraft:unknown"
- `BlockEntityType_FromId_KnownIds` - 已知 ID 转类型
- `BlockEntityType_FromId_ShortForm` - 简写形式识别
- `BlockEntityType_FromId_UnknownReturnsUnknown` - 未知 ID 返回 Unknown

### BlockEntity 基础测试
- `Create_GetTypeAndPos` - 创建并获取类型和位置
- `ChangedFlag_InitiallyFalse` - 修改标记初始为 false
- `SetChanged_SetsFlag` - setChanged 设置标记
- `ClearChanged_ClearsFlag` - clearChanged 清除标记
- `NeedsTick_DefaultFalse` - 默认不需要 tick
- `GetCustomName_DefaultEmpty` - 默认无自定义名称
- `Save_ContainsBasicInfo` - 保存包含基本信息
- `Clone_CreatesCopy` - 克隆创建副本

### ContainerBlockEntity 测试
- `Container_OpenCount_InitiallyZero` - 打开计数初始为 0
- `Container_OpenContainer_IncrementsCount` - open 增加计数
- `Container_CloseContainer_DecrementsCount` - close 减少计数
- `Container_CloseContainer_NotBelowZero` - close 不会使计数为负
- `Container_GetInventory_ReturnsNullByDefault` - 默认返回 nullptr
- `Container_IsEmpty_ReturnsTrueByDefault` - 默认为空

## 扩展计划

当前实现为框架基础，未来需要添加：

1. **更多方块实体类型**
   - ChestEntity - 箱子 ✅
   - FurnaceEntity - 熔炉（需要 tick 处理燃烧逻辑）✅
   - HopperEntity - 漏斗（需要 tick 处理物品传输）✅
   - SignEntity - 告示牌（存储文本）✅
   - BeaconEntity - 信标（需要 tick 处理效果）✅ 红石比较器信号已实现

2. **方块实体注册表** ✅
   - 工厂方法注册
   - 从 JSON 创建方块实体
   - 支持模组自定义方块实体

3. **客户端同步**
   - 方块实体数据包
   - 客户端方块实体渲染器

4. **World 集成**
   - 实现 `getBlockState()`
   - 区块中的方块实体存储和管理
