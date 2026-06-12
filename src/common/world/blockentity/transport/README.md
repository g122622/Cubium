# 传输类方块实体模块

提供漏斗等传输类方块实体的实现。

## 目录结构

```
transport/
├── IHopper.hpp/cpp       # 漏斗接口（统一处理漏斗方块和漏斗矿车，坐标返回 f64）
├── HopperEntity.hpp/cpp  # 漏斗实体（物品传输、ISidedInventory）
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
HopperEntity (漏斗实体)
       │
       └──实现──→ IHopper (漏斗接口)

HopperEntity ──组合──→ SimpleInventory (core/ 简单背包)
```

## 上下游外部依赖关系

### 上游依赖（谁使用了这个模块）

- `world/block/blocks/HopperBlock.cpp` - 漏斗方块创建和访问方块实体
- `entity/vehicle/MinecartHopper.cpp` - 漏斗矿车（通过 IHopper 接口）
- `world/chunk/` - 区块加载时反序列化方块实体

### 下游依赖（这个模块依赖了谁）

- `world/blockentity/BlockEntity.hpp` - 方块实体基类
- `world/blockentity/ContainerBlockEntity.hpp` - 容器方块实体基类
- `world/blockentity/core/LockableBlockEntity.hpp` - 可锁定基类
- `world/blockentity/core/SimpleInventory.hpp` - 简单背包实现
- `entity/inventory/IInventory.hpp` - 背包接口
- `entity/inventory/ISidedInventory.hpp` - 分面背包接口
- `entity/ItemEntity.hpp` - 物品实体（捕获物品）

## 容易踩的坑

### 1. 传输冷却管理

传输冷却必须在成功传输后设置，否则会连续传输。正常冷却为 8 tick，漏斗链优化时目标漏斗的冷却时间为 7 tick。

### 2. 漏斗自循环

检测目标容器是否是漏斗自己，避免自循环：
```cpp
if (target == this) {
    return false;  // 避免自循环
}
```

### 3. ISidedInventory 槽位访问

漏斗使用 `ISidedInventory.getSlotsForFace()` 获取可访问槽位，使用 `canInsertItem()` 和 `canExtractItem()` 检查是否可操作。非方向性容器回退到全槽位访问。

### 4. 物品实体捕获

捕获物品实体后需要正确处理剩余物品：
- 如果剩余为空：调用 `itemEntity->remove()`
- 如果有剩余：调用 `itemEntity->setItemStack(remaining)`

### 5. 红石状态同步

漏斗的红石状态由方块状态 `ENABLED` 属性控制，红石信号变化时需要更新方块状态。

### 6. 比较器信号计算

输出信号基于填充度：`signal = floor(fillRatio * 14) + (nonEmpty ? 1 : 0)`，最大 15。

### 7. 收集区域计算

`IHopper::getCollectionArea()` 返回碗状内部区域 + 上方方块区域，用于收集上方物品实体。
