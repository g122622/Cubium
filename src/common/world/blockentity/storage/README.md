# 存储类方块实体模块

提供箱子、陷阱箱等存储类方块实体的实现。

## 目录结构

```
storage/
├── ChestEntity.hpp/cpp          # 箱子实体（27格存储、双箱合并、盖子动画）
├── TrappedChestEntity.hpp/cpp   # 陷阱箱实体（继承ChestEntity、红石信号输出）
├── DoubleSidedInventory.hpp/cpp # 双箱合并容器（54格、委托模式）
├── EnderChestEntity.hpp/cpp     # 末影箱实体（玩家独立存储、打开动画）
├── ShulkerBoxEntity.hpp/cpp     # 潜影盒实体（保留物品、防递归嵌套、ISidedInventory）
├── BarrelEntity.hpp/cpp         # 木桶实体（27格、无上方方块限制、状态切换）
└── README.md
```

## 内部模块关系

```
BlockEntity (父模块基类)
       ↑
       │
ContainerBlockEntity (父模块容器基类)
       ↑
       │
LockableBlockEntity (core/ 可锁定容器基类，mc::blockentity 命名空间)
       ↑
       │
LootableContainerBlockEntity (core/ 可填充战利品表的容器基类)
       ↑
       ├──────────────────┬──────────────────┐
       │                  │                  │
   ChestEntity       BarrelEntity     ShulkerBoxEntity
       ↑              (木桶)          (多重继承 ISidedInventory)
       │
TrappedChestEntity
   (陷阱箱)

EnderChestEntity (末影箱，独立继承 BlockEntity，无容器功能)
DoubleSidedInventory (双箱容器，非 BlockEntity，用于合并两个箱子)
```

## 上下游外部依赖关系

### 上游依赖（谁使用了这个模块）

- `world/block/blocks/storage/` - 箱子方块、陷阱箱方块、木桶方块、潜影盒方块等
- `world/chunk/` - 区块加载时反序列化方块实体
- `entity/inventory/container/` - 背包 GUI 容器类
- `client/renderer/` - 箱子盖子动画渲染

### 下游依赖（这个模块依赖了谁）

- `world/blockentity/BlockEntity.hpp` - 方块实体基类
- `world/blockentity/ContainerBlockEntity.hpp` - 容器方块实体基类
- `world/blockentity/core/LockableBlockEntity.hpp` - 可锁定基类
- `world/blockentity/core/LootableContainerBlockEntity.hpp` - 战利品表容器基类
- `world/blockentity/core/SimpleInventory.hpp` - 简单背包实现
- `entity/inventory/IInventory.hpp` - 背包接口
- `entity/inventory/ISidedInventory.hpp` - 分面背包接口（潜影盒）
- `entity/loot/LootTableManager.hpp` - 战利品表管理器

## 容易踩的坑

### 1. ChestEntity 双箱合并

两个箱子相邻放置时会自动合并为 54 格容器。使用 `getDoubleInventory()` 获取合并容器。注意：`DoubleSidedInventory` 是委托模式，操作会转发到底层两个箱子。

### 2. TrappedChestEntity 红石信号

陷阱箱的红石信号输出 = 打开的玩家数（最大 15）。`getRedstoneSignal(IWorld&)` 方法会查询双箱连接，聚合两侧的打开玩家数后 clamp 到 15。每次 `openContainer()`/`closeContainer()` 都会通知邻居方块更新红石信号（通过 `RedstoneSystem::updateNeighbors` 和 `updateComparators`），双箱时还会同步通知连接箱子侧的邻居。注意：`openContainer`/`closeContainer` 的双箱计数同步由 `ChestBlock::onBlockActivated()` 和服务器端的容器关闭处理负责，而非 `TrappedChestEntity` 自身。

### 3. EnderChestEntity 不存储物品

末影箱不存储实际物品，物品存储在玩家数据中。`EnderChestEntity` 只负责打开/关闭动画和音效。

### 4. ShulkerBoxEntity 防递归嵌套

`canInsertItem()` 会检查物品是否为潜影盒，防止潜影盒放入另一个潜影盒。这是通过检查物品类型实现的。

### 5. ShulkerBoxEntity 动画状态

潜影盒有 4 种动画状态：Closed, Opening, Opened, Closing。`tick()` 方法更新动画进度，`getProgress()` 返回插值后的进度（0.0-1.0）。

### 6. BarrelEntity 无双箱功能

木桶不能像箱子一样合并为双箱，每个木桶都是独立的 27 格容器。

### 7. 战利品表填充时机

继承自 `LootableContainerBlockEntity`，`isEmpty()` 和 `openContainer()` 会自动触发填充。只有 ServerWorld 的 `lootTableManager()` 返回有效指针。

### 8. DoubleSidedInventory 非拥有指针

`DoubleSidedInventory` 持有两个 `ChestEntity*` 原始指针，不拥有它们的生命周期。确保两个箱子在 `DoubleSidedInventory` 使用期间有效。
