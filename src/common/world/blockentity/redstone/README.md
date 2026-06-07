# 红石方块实体 (Redstone Block Entities)

红石系统相关的方块实体实现。

## 目录结构

```
redstone/
├── CommandBlockEntity.hpp/cpp     # 命令方块实体（脉冲/循环/连锁三种模式）
├── ComparatorEntity.hpp/cpp       # 比较器方块实体（存储输出信号强度）
├── DaylightDetectorEntity.hpp/cpp # 日光探测器方块实体（定期更新信号）
└── README.md
```

## 内部模块关系

```
BlockEntity (父模块基类)
       ↑
       ├──────────────────┬──────────────────────┐
       │                  │                      │
CommandBlockEntity  ComparatorEntity  DaylightDetectorEntity
       │
       └──实现──→ ICommandSource (命令源接口)
```

三个类相互独立，没有继承关系。

## 上下游外部依赖关系

### 上游依赖（谁使用了这个模块）

- `world/block/blocks/redstone/` - 命令方块、比较器、日光探测器方块创建和访问方块实体
- `world/chunk/` - 区块加载时反序列化方块实体
- `server/` - 服务器执行命令时访问命令方块实体

### 下游依赖（这个模块依赖了谁）

- `world/blockentity/BlockEntity.hpp` - 方块实体基类
- `world/blockentity/BlockEntityType.hpp` - 方块实体类型枚举
- `world/blockentity/core/BlockEntityRegistry.hpp` - 注册表
- `command/ICommandSource.hpp` - 命令源接口（CommandBlockEntity）

## 容易踩的坑

### 1. 忘记注册方块实体

必须在 `BlockEntityRegistry::registerBuiltinTypes()` 中注册：
```cpp
registerType(BlockEntityType::CommandBlock, [](const BlockPos& pos) {
    return std::make_unique<CommandBlockEntity>(pos);
});
```

### 2. 成功计数范围

命令方块的成功计数范围是 0-15，用于比较器输出信号强度。`setSuccessCount()` 会使用 `std::clamp(count, 0, 15)` 限制范围。

### 3. 同一 tick 防止重复执行

`CommandBlockEntity::trigger()` 方法会检查 `m_lastExecution` 防止同一 tick 内重复执行。

### 4. 条件执行检查

条件模式下需要检查背后命令方块的成功计数：
- 获取背后命令方块：`m_pos.offset(Directions::opposite(facing))`
- 检查成功计数是否 > 0

### 5. 方块创建时创建实体

方块放置时需要自动创建 BlockEntity：
```cpp
void CommandBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    if (!world.getBlockEntity(pos)) {
        world.setBlockEntity(pos, createBlockEntity(pos).release());
    }
}
```

### 6. 方块移除时清理实体

方块移除时需要清理 BlockEntity：
```cpp
void CommandBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) {
    world.removeBlockEntity(pos);
}
```

### 7. 命令方块三种模式

- `Redstone`（脉冲）：红石信号上升沿触发
- `Auto`（循环）：每 tick 自动执行
- `Sequence`（连锁）：被前一个命令方块触发

### 8. 比较器信号保持

`ComparatorEntity` 存储输出信号强度，实现"前端信号保持"特性。当比较器从激活变为未激活时，输出信号需要保持一段时间。

### 9. 日光探测器更新间隔

日光探测器每 20 tick 更新一次信号，而非每 tick 更新。这是性能优化。
