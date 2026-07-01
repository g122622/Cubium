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
- `entity/vehicle/MinecartEntity.cpp` - 漏斗矿车（通过 IHopper 接口）
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

传输冷却必须在成功传输后设置，否则会连续传输。正常冷却为 8 tick，漏斗链优化时目标漏斗的冷却时间为 7 tick。`isOnCustomCooldown()` 返回 `cooldownTime > 8`，用于判断漏斗是否被设置了超过正常值的自定义冷却。

### 2. 漏斗自循环

`_transferItemsOut()` 和 `pullItems()` 中检查目标容器是否是漏斗自身，避免物品无限循环传输。

### 3. ISidedInventory 槽位访问

漏斗使用 `ISidedInventory.getSlotsForFace()` 获取可访问槽位，使用 `canInsertItem()` 和 `canExtractItem()` 检查是否可操作。非方向性容器回退到全槽位访问。

### 4. 物品实体捕获

MC Java 中漏斗捕获物品不检查 `pickupDelay`（只有玩家拾取才检查），`getCaptureItems()` 仅过滤 `isAlive()` 的物品实体。`captureItem()` 在部分物品被捕获时也返回 `true`（触发传输冷却），与 MC Java 的 `addItem()` 行为一致。

### 5. 客户端/服务端

`HopperEntity::tick()` 必须在开头检查 `world.isClientSide()`，漏斗传输逻辑仅在服务端执行。MC Java 通过 `HopperBlock.getTicker()` 返回 `null` 实现。

### 6. 上方方块阻挡物品吸取

当漏斗上方没有容器时，MC Java 会检查上方方块的碰撞形状：如果漏斗对齐网格（`isGridAligned()=true`）且上方方块向下碰撞面为完整方块，则不吸取物品实体。但如果上方方块在 `DOES_NOT_BLOCK_HOPPERS` 标签中（如蜂巢/蜂箱），即使碰撞形状为完整方块，漏斗仍可吸取物品——这允许漏斗与蜂巢/蜂箱交互（吸取蜂蜜瓶/空瓶）。方块漏斗返回 `isGridAligned()=true`，漏斗矿车返回 `false`。

### 7. 收集区域计算

`IHopper::getCollectionArea()` 返回 MC Java 的 `SUCK_AABB`（由 `Block.column(16.0, 11.0, 32.0)` 定义），从漏斗碗口顶部（Y=11/16）向上延伸到两格高度，水平覆盖完整方块区域。

### 8. ISidedInventoryProvider 内存泄漏

`getInventoryAtPosition()` 中通过 `ISidedInventoryProvider::createInventory()` 创建的 `unique_ptr` 被 `.release()` 转为原始指针返回，调用方无法释放。目前仅 ComposterBlock 使用此路径，泄漏量较小。

### 9. IHopper 获取背包方式

`IHopper` 实现者（HopperEntity、HopperMinecartEntity）不继承 `IInventory`，而是通过组合方式持有 `SimpleInventory`。因此，`pullItems()` 和 `_pullItemFromSlot()` 中不能使用 `dynamic_cast<IInventory*>(&hopper)` 获取漏斗背包（会返回 nullptr）。必须通过 `IHopper::getHopperInventory()` 虚方法获取。此方法在 IHopper 接口中定义，默认返回 nullptr，由子类覆写返回实际背包指针。
