# Container 模块

提供GUI容器（Container/Menu）的实现，用于客户端-服务端同步玩家与方块实体的交互。

当前这一层里，`ChestContainer`、`FurnaceContainer`、`EnchantmentContainer`、`BrewingStandContainer` 和 `AnvilContainer` 已迁移到 `AbstractContainerMenu` 菜单基类；`Container` 仍保留给旧式槽位容器和 `HopperContainer` 这类轻量实现使用。

## 目录结构

```
container/
├── ChestContainer.hpp/cpp        # 箱子菜单（单箱27格/双箱54格，基于 AbstractContainerMenu）
├── FurnaceContainer.hpp/cpp      # 熔炉菜单（输入/燃料/输出槽，基于 AbstractContainerMenu）
├── EnchantmentContainer.hpp/cpp  # 附魔台菜单（物品槽/青金石槽，附魔选项生成）
├── BrewingStandContainer.hpp/cpp # 酿造台菜单（3药水槽/材料槽/燃料槽）
├── AnvilContainer.hpp/cpp        # 铁砧菜单（2输入槽/输出槽，修复/重命名/附魔合并）
├── HopperContainer.hpp/cpp       # 漏斗容器（5格）
└── README.md
```

## 文件详解

### ChestContainer.hpp/cpp

**职责**：箱子GUI菜单，处理玩家与箱子之间的物品交换，并与客户端/服务端菜单同步层对齐。

**主要功能**：
- 单箱模式：27格存储
- 双箱模式：54格存储（合并两个箱子）
- 玩家物品栏同步
- 打开/关闭计数管理

**槽位布局**：
```
单箱 (27格):
+----------------------------------+
| 0  1  2  3  4  5  6  7  8        |
| 9  10 11 12 13 14 15 16 17      |
| 18 19 20 21 22 23 24 25 26      |
+----------------------------------+

双箱 (54格):
+----------------------------------+
| 上半部分（LEFT箱子）              |
| 0-26                             |
+----------------------------------+
| 下半部分（RIGHT箱子）             |
| 27-53                            |
+----------------------------------+
```

### FurnaceContainer.hpp/cpp

**职责**：熔炉GUI菜单，处理玩家与熔炉之间的物品交换，并与客户端/服务端菜单同步层对齐。

**主要功能**：
- 3槽熔炉背包（输入/燃料/输出）
- 熔炼进度显示
- 燃烧时间显示
- 快速移动支持

**槽位布局**：
```
熔炉容器:
+------------+
| 输入 (0)   |
| 燃料 (1)   |
| 输出 (2)   |
+------------+
```

### EnchantmentContainer.hpp/cpp

**职责**：附魔台GUI菜单，处理附魔选项生成和青金石消耗。

**主要功能**：
- 物品槽：放置待附魔物品
- 青金石槽：消耗青金石作为附魔材料
- 附魔选项生成：基于书架力量和随机种子
- 附魔等级计算：MC 1.16.5 公式

**槽位布局**：
```
附魔台容器:
+------------+
| 物品 (0)   |
| 青金石 (1) |
+------------+
```

**书架力量计算**：
- 检测附魔台周围2格范围内的书架
- 书架与附魔台之间必须有空气
- 最大书架力量：15

### BrewingStandContainer.hpp/cpp

**职责**：酿造台GUI菜单，处理药水酿造。

**主要功能**：
- 3个药水槽：放置药水瓶
- 材料槽：放置酿造材料
- 燃料槽：放置烈焰粉（每次酿造消耗1点，共20点）
- 酿造状态同步

**槽位布局**：
```
酿造台容器:
+------------------+
| 材料 (3)  燃料(4)|
| 药水(0)          |
| 药水(1)          |
| 药水(2)          |
+------------------+
```

### AnvilContainer.hpp/cpp

**职责**：铁砧GUI菜单，处理物品修复、重命名和附魔合并。

**主要功能**：
- 输入槽1：待修复/重命名的物品
- 输入槽2：修复材料或附魔书
- 输出槽：修复/合并后的结果
- 修复成本计算（最大40级）
- 附魔合并逻辑

**槽位布局**：
```
铁砧容器:
+------------------+
| 输入1(0) 输入2(1)|
|    输出 (2)      |
+------------------+
```

**修复成本规则**：
- 基础成本：输入物品修复成本之和
- 重命名：+1级
- 附魔合并：根据附魔稀有度计算
- 修复耐久：+2级
- 最大成本：40级（超过则"太贵"）

### HopperContainer.hpp/cpp

**职责**：漏斗GUI容器，处理玩家与漏斗之间的物品交换。

**主要功能**：
- 5格漏斗背包
- 快速移动支持

**槽位布局**：
```
漏斗容器 (5格):
+---------------------+
| 0  1  2  3  4       |
+---------------------+
```

## 模块关系

```mermaid
graph TB
    IInventory[IInventory 背包接口]
    AbstractContainerMenu[AbstractContainerMenu 菜单基类]
    Container[Container 旧式容器基类]
    ChestContainer[ChestContainer]
    FurnaceContainer[FurnaceContainer]
    EnchantmentContainer[EnchantmentContainer]
    BrewingStandContainer[BrewingStandContainer]
    AnvilContainer[AnvilContainer]
    HopperContainer[HopperContainer]
    ChestEntity[ChestEntity]
    FurnaceEntity[AbstractFurnaceEntity]
    EnchantingTableEntity[EnchantingTableEntity]
    BrewingStandEntity[BrewingStandEntity]
    PlayerInventory[PlayerInventory]

    AbstractContainerMenu --> ChestContainer
    AbstractContainerMenu --> FurnaceContainer
    AbstractContainerMenu --> EnchantmentContainer
    AbstractContainerMenu --> BrewingStandContainer
    AbstractContainerMenu --> AnvilContainer
    Container --> HopperContainer
    ChestContainer -.关联.-> ChestEntity
    FurnaceContainer -.关联.-> FurnaceEntity
    EnchantmentContainer -.关联.-> EnchantingTableEntity
    BrewingStandContainer -.关联.-> BrewingStandEntity
    ChestContainer -.同步.-> PlayerInventory
    FurnaceContainer -.同步.-> PlayerInventory
    EnchantmentContainer -.同步.-> PlayerInventory
    BrewingStandContainer -.同步.-> PlayerInventory
    AnvilContainer -.同步.-> PlayerInventory
    HopperContainer -.同步.-> PlayerInventory
```

## 依赖项

### 内部依赖
- `entity/inventory/IInventory.hpp` - 背包接口
- `entity/inventory/PlayerInventory.hpp` - 玩家背包
- `entity/inventory/Slot.hpp` - 槽位类
- `world/blockentity/storage/ChestEntity.hpp` - 箱子实体
- `world/blockentity/processing/AbstractFurnaceEntity.hpp` - 熔炉实体
- `world/blockentity/processing/BrewingStandEntity.hpp` - 酿造台实体
- `world/blockentity/transport/HopperEntity.hpp` - 漏斗实体
- `item/enchantment/EnchantmentHelper.hpp` - 附魔工具类
- `item/potion/PotionBrewing.hpp` - 酿造配方管理

### 外部依赖
- `<memory>` - 智能指针
- `<vector>` - 动态数组
- `<array>` - 固定数组

## 使用方法

### 创建箱子容器

```cpp
// 单箱
mc::PlayerInventory playerInventory(nullptr);
auto chestContainer = std::make_unique<ChestContainer>(
    containerId,
    &playerInventory,
    chestEntity->getInventory()
);

// 双箱
auto doubleContainer = std::make_unique<ChestContainer>(
    containerId,
    &playerInventory,
    chestA->getInventory(),
    chestB->getInventory()
);
```

### 创建熔炉容器

```cpp
auto furnaceContainer = std::make_unique<FurnaceContainer>(
    containerId,
    &playerInventory,
    furnaceEntity->getFurnaceInventory()
);
```

### 创建附魔台容器

```cpp
auto enchantmentContainer = std::make_unique<EnchantmentContainer>(
    containerId,
    &playerInventory,
    enchantingTablePos,
    &world
);
```

### 创建酿造台容器

```cpp
auto brewingContainer = std::make_unique<BrewingStandContainer>(
    containerId,
    &playerInventory,
    brewingStandEntity->getInventory(),
    brewingStandEntity
);
```

### 创建铁砧容器

```cpp
auto anvilContainer = std::make_unique<AnvilContainer>(
    containerId,
    &playerInventory,
    anvilPos,
    &world
);
```

### 创建漏斗容器

```cpp
auto hopperContainer = std::make_unique<HopperContainer>(
    containerId,
    playerInventory,
    hopperEntity->getInventory()
);
```

## 容易踩的坑

### 1. 双箱槽位映射

双箱容器需要正确映射槽位到两个箱子：

```cpp
// 错误：直接访问槽位
ItemStack item = chestA->getItem(slot);

// 正确：根据槽位选择箱子
if (slot < 27) {
    return chestA->getItem(slot);
} else {
    return chestB->getItem(slot - 27);
}
```

### 2. 容器ID管理

每个容器需要唯一的ID用于网络同步：

```cpp
// 服务端分配ID
u32 containerId = nextContainerId++;

// 客户端接收时验证
if (containerId != expectedId) {
    // ID不匹配，忽略
}
```

### 3. 玩家背包同步

打开容器时需要同步玩家背包状态：

```cpp
void onContainerOpen(Player& player) {
    // 添加玩家背包槽位到容器
    for (int i = 0; i < 36; ++i) {
        addSlot(new PlayerInventorySlot(player.getInventory(), i));
    }
}
```

### 4. 容器关闭处理

关闭容器时需要正确处理物品返回：

```cpp
void onContainerClose(Player& player) {
    // 返回容器中的物品
    for (auto& slot : m_slots) {
        if (!slot.getItem().isEmpty()) {
            player.getInventory().addItem(slot.getItem());
        }
    }
}
```

### 5. 附魔台书架力量计算

书架必须在附魔台周围2格范围内，且中间必须有空气：

```cpp
// 检查书架与附魔台之间的空气
BlockPos airPos1 = tablePos.offset(dx > 0 ? 1 : (dx < 0 ? -1 : 0), 0, ...);
BlockPos airPos2 = tablePos.offset(..., 1, ...);
if (world->isAirBlock(airPos1) && world->isAirBlock(airPos2)) {
    power++;
}
```

### 6. 铁砧修复成本限制

修复成本超过40级时操作不可用：

```cpp
if (repairCost >= 40) {
    // 显示"太贵"提示
    return;
}
```

## 测试用例

测试文件位于 `tests/common/entity/inventory/container/`：

- `ChestContainerTest.cpp` - 箱子菜单测试，覆盖槽位布局和快速移动
- `FurnaceContainerTest.cpp` - 熔炉菜单测试，覆盖槽位布局和快速移动
- `HopperContainerTest.cpp` - 漏斗容器测试
- `EnchantmentContainerTest.cpp` - 附魔台菜单测试（待创建）
- `BrewingStandContainerTest.cpp` - 酿造台菜单测试（待创建）
- `AnvilContainerTest.cpp` - 铁砧菜单测试（待创建）

### 测试覆盖

- 槽位访问和修改
- 双箱槽位映射
- 快速移动（Shift+点击）
- 物品交换和堆叠
- 容器打开/关闭
- 网络同步
- 附魔选项生成
- 书架力量计算
- 酿造台燃料消耗
- 铁砧修复成本计算
- 附魔合并逻辑
