# 存储类方块实体模块

提供箱子、陷阱箱等存储类方块实体的实现。

## 目录结构

```
storage/
├── ChestEntity.hpp/cpp          # 箱子实体
├── TrappedChestEntity.hpp/cpp   # 陷阱箱实体
├── DoubleSidedInventory.hpp/cpp # 双箱合并容器
├── EnderChestEntity.hpp/cpp     # 末影箱实体
├── ShulkerBoxEntity.hpp/cpp     # 潜影盒实体
├── BarrelEntity.hpp/cpp         # 木桶实体
└── README.md
```

## 文件详解

### ChestEntity.hpp/cpp

**职责**：箱子方块实体，存储27格物品。

**主要功能**：
- 27格物品存储
- 打开计数和盖子动画
- 双箱检测与合并
- 红石比较器信号
- 锁定功能（继承自LockableBlockEntity）

### TrappedChestEntity.hpp/cpp

**职责**：陷阱箱实体，输出红石信号。

**主要功能**：
- 继承自ChestEntity
- 红石信号输出 = 打开玩家数（最大15）
- 打开/关闭时通知邻居更新

### EnderChestEntity.hpp/cpp

**职责**：末影箱方块实体。

**主要功能**：
- 不存储实际物品（物品在玩家数据中）
- 打开动画与普通箱子相同
- 爆破抗性高（600）
- 每个玩家有独立的物品存储

### ShulkerBoxEntity.hpp/cpp

**职责**：潜影盒方块实体。

**主要功能**：
- 27格物品存储
- 被破坏时保留物品（不掉落）
- 可以被锁定（需要正确名称的物品打开）
- 打开时有动画效果

### BarrelEntity.hpp/cpp

**职责**：木桶方块实体。

**主要功能**：
- 27格物品存储（与箱子相同）
- 可以在上方有方块时打开（与箱子不同）
- 没有双箱合并功能
- 可以面向任意六个方向放置

### DoubleSidedInventory.hpp/cpp

**职责**：双箱合并容器，将两个27格箱子合并为54格。

**主要功能**：
- 委托模式，操作转发到底层两个箱子
- 54格容器（27+27）
- 槽位映射：前27格→上半部分，后27格→下半部分

## 类继承关系

```
BlockEntity (基类)
│
├── ContainerBlockEntity (容器基类)
│   │
│   ├── LockableBlockEntity (可锁定容器基类)
│   │   │
│   │   ├── ChestEntity (箱子)
│   │   │   └── TrappedChestEntity (陷阱箱)
│   │   │
│   │   ├── ShulkerBoxEntity (潜影盒)
│   │   │
│   │   └── BarrelEntity (木桶)
│   │
│   └── DoubleSidedInventory (双箱容器)
│
└── EnderChestEntity (末影箱)
```

## 依赖项

### 内部依赖
- `world/blockentity/core/LockableBlockEntity.hpp` - 可锁定基类
- `world/blockentity/core/SimpleInventory.hpp` - 简单背包
- `entity/inventory/IInventory.hpp` - 背包接口

### 外部依赖
- `<memory>` - 智能指针
- `<array>` - 静态数组
- `<functional>` - 回调函数

## 使用方法

### 创建箱子实体

```cpp
// 创建箱子
auto chest = std::make_unique<ChestEntity>(BlockPos(0, 0, 0));

// 设置物品
chest->getInventory()->setItem(0, ItemStack(Items::DIAMOND, 64));

// 打开箱子
chest->openContainer();

// 获取红石信号
i32 signal = chest->getComparatorSignal(world);
```

### 双箱合并

```cpp
// 检查双箱
if (chest.isDoubleChest(world)) {
    // 获取合并容器
    auto doubleInv = chest.getDoubleInventory(world);

    // 操作54格容器
    doubleInv->setItem(0, ItemStack(Items::DIAMOND, 32));
    doubleInv->setItem(27, ItemStack(Items::IRON, 64));
}
```

### 创建潜影盒

```cpp
// 创建潜影盒
auto shulker = std::make_unique<ShulkerBoxEntity>(BlockPos(0, 0, 0));

// 设置物品
shulker->getInventory()->setItem(0, ItemStack(Items::DIAMOND, 64));

// 检查动画状态
if (shulker->getAnimationStatus() == ShulkerBoxEntity::AnimationStatus::Opened) {
    // 潜影盒已打开
}
```

## 测试用例

测试文件位于 `tests/common/world/blockentity/`：

- `ChestEntityTest.cpp` - 箱子实体测试
- `DoubleSidedInventoryTest.cpp` - 双箱容器测试
- `TrappedChestTest.cpp` - 陷阱箱测试
- `ShulkerBoxEntityTest.cpp` - 潜影盒测试
- `BarrelEntityTest.cpp` - 木桶测试
