# Tick 模块

Tick模块是Minecraft世界中的计划刻(Scheduled Tick)调度系统，负责管理和执行方块、流体等游戏元素的延迟更新。

## 目录结构

```
tick/
├── base/                           # 基础类型
│   ├── TickPriority.hpp           # Tick执行优先级枚举
│   └── ScheduledTick.hpp          # 调度条目结构体模板
├── list/                           # Tick列表实现
│   ├── ITickList.hpp              # Tick列表接口
│   ├── EmptyTickList.hpp          # 空实现（客户端用）
│   └── ServerTickList.hpp         # 服务端实现
└── manager/                        # 管理器
    ├── TickManager.hpp            # Tick管理器头文件
    └── TickManager.cpp            # Tick管理器实现
```

## 内部模块关系

```
ITickList<T> (接口)
    ↑
    ├── EmptyTickList<T> (客户端空实现)
    │
    └── ServerTickList<T> (服务端实现)
            ↑
            │
       TickManager (外观类)
            ├── ServerTickList<Block>
            └── ServerTickList<Fluid>
```

- **base/**: 定义基础数据类型，被 list/ 依赖
- **list/**: 定义tick列表接口和实现，被 manager/ 依赖
- **manager/**: 对外提供统一的tick管理API，封装方块和流体tick列表

## 上下游外部依赖

### 本模块依赖
- `common/core/Types.hpp`: 基础类型定义
- `common/world/block/BlockPos.hpp`: 方块位置
- `common/world/block/Block.hpp`, `BlockRegistry.hpp`: 方块基类和注册表
- `common/world/fluid/Fluid.hpp`, `FluidRegistry.hpp`: 流体基类和注册表
- `common/world/IWorld.hpp`: 世界接口
- `common/resource/ResourceLocation.hpp`: 资源位置

### 被依赖
- `ServerWorld`: 使用 TickManager 管理方块和流体tick
- 区块序列化系统: 调用 `getPendingBlockTicks/getPendingFluidTicks` 保存tick数据

## 容易踩的坑

1. **区块未加载时tick不执行**: `canTick()` 检查区块是否加载，未加载的区块中的tick会被重新调度(delay=0)

2. **tick回调中的状态变化**: tick回调可能修改世界状态（如方块变化），需要注意迭代器的有效性

3. **同一位置重复调度**: 调度已存在的(位置,目标)组合会被忽略（不会替换）

4. **延迟为负数时**: 会自动设置为0（立即执行）

5. **每tick最大执行数量**: 默认最多执行65536个tick，超过的留到下一tick

6. **线程安全**: 所有操作应在主线程执行，非线程安全

7. **指针有效性**: `ScheduledTick` 存储目标对象的原始指针，需要确保目标对象生命周期

8. **双向容器同步**: `m_pendingTicksTree` 和 `m_pendingTicksSet` 需要保持同步，代码中有自动修复机制

## 与 Minecraft Java 1.16.5 对应关系

| 本项目类 | MC Java 1.16.5类 |
|---------|-----------------|
| `TickPriority` | `net.minecraft.world.TickPriority` |
| `ScheduledTick` | `net.minecraft.world.NextTickListEntry` |
| `ITickList` | `net.minecraft.world.ITickList` |
| `EmptyTickList` | `net.minecraft.world.EmptyTickList` |
| `ServerTickList` | `net.minecraft.world.server.ServerTickList` |
| `TickManager` | `World.pendingBlockTicks` + `World.pendingFluidTicks` |
