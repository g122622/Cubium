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

## 文件详细说明

### base/TickPriority.hpp

**职责**: 定义Tick执行优先级枚举

**主要内容**:
- `TickPriority` 枚举类，定义7个优先级级别：
  - `ExtremelyHigh` (-3): 极高优先级，如活塞、红石
  - `VeryHigh` (-2): 很高优先级
  - `High` (-1): 高优先级
  - `Normal` (0): 普通优先级（默认）
  - `Low` (1): 低优先级
  - `VeryLow` (2): 很低优先级
  - `ExtremelyLow` (3): 极低优先级
- `fromInt(i32)`: 从整数值获取优先级（带边界保护）
- `toInt(TickPriority)`: 获取优先级的整数值

**参考**: `net.minecraft.world.TickPriority`

```cpp
// 使用示例
scheduleBlockTick(pos, block, 10, TickPriority::High);
```

---

### base/ScheduledTick.hpp

**职责**: 定义调度条目的数据结构

**主要内容**:
- `ScheduledTick<T>` 模板结构体，存储待执行的tick信息：
  - `position`: 方块位置 (BlockPos)
  - `target`: 目标对象指针 (T*)
  - `scheduledTick`: 调度执行的游戏刻 (u64)
  - `priority`: 执行优先级 (TickPriority)
  - `tickEntryId`: 唯一ID（用于排序）

**关键特性**:
- 排序比较器 `operator<`: 按 scheduledTick -> priority -> tickEntryId 排序
- 相等比较器 `operator==`: 基于位置和目标（用于去重）
- `hashCode()`: 哈希值（用于HashSet）

**参考**: `net.minecraft.world.NextTickListEntry`

```cpp
// 创建调度条目
ScheduledTick<Block> tick(pos, &block, 100, TickPriority::Normal, id);
```

---

### list/ITickList.hpp

**职责**: 定义Tick调度列表的抽象接口

**主要内容**:
- `ITickList<T>` 模板接口类：
  - `isTickScheduled(pos, target)`: 检查是否已调度
  - `isTickPending(pos, target)`: 检查是否在本tick待执行
  - `scheduleTick(pos, target, delay)`: 调度tick（普通优先级）
  - `scheduleTick(pos, target, delay, priority)`: 调度tick（指定优先级）
  - `cancelTick(pos, target)`: 取消tick
  - `pendingCount()`: 获取待处理数量

**参考**: `net.minecraft.world.ITickList`

---

### list/EmptyTickList.hpp

**职责**: 提供空实现的Tick列表（客户端用）

**主要内容**:
- `EmptyTickList<T>` 单例模板类，继承自 `ITickList<T>`
- 所有方法返回空操作或false
- 用于客户端或不需要tick处理的场景

**特点**:
- 单例模式：`EmptyTickList<Block>::get()`
- 零开销：所有操作都是空操作

**参考**: `net.minecraft.world.EmptyTickList`

```cpp
// 客户端使用空列表
ITickList<Block>& ticks = EmptyTickList<Block>::get();
ticks.scheduleTick(pos, block, 10);  // 无操作
```

---

### list/ServerTickList.hpp

**职责**: 服务端Tick调度列表的核心实现

**主要内容**:
- `ServerTickList<T>` 模板类，继承自 `ITickList<T>`

**数据结构**:
- `m_pendingTicksTree`: `std::set<ScheduledTick<T>>` - 按时间/优先级排序
- `m_pendingTicksSet`: `std::unordered_set<ScheduledTick<T>>` - 快速存在检查
- `m_ticksThisTick`: `std::queue<ScheduledTick<T>>` - 本tick待处理队列
- `m_executedThisTick`: `std::vector<ScheduledTick<T>>` - 本tick已执行列表

**核心方法**:
- `scheduleTick()`: 调度新的tick（会替换已存在的同位置同目标tick）
- `tick(currentTick, maxTicks)`: 执行当前游戏刻的所有待处理tick
- `getPendingTicks()`: 获取区块范围内的tick（用于序列化）
- `copyTicks()`: 复制tick到新位置（用于结构放置）
- `cancelTick()`: 取消指定tick

**执行流程**:
1. 设置当前tick用于调度计算
2. 从TreeSet取出 scheduledTick <= currentTick 的条目（最多65536个）
3. 检查区块是否加载
4. 如果加载，执行tick回调
5. 如果未加载，保持原样（下次再试）

**参考**: `net.minecraft.world.server.ServerTickList`

```cpp
// 创建服务端tick列表
auto blockTicks = std::make_unique<ServerTickList<Block>>(
    world,
    [](Block& b) { return false; },                    // 过滤器
    [](Block& b) { return b.blockLocation(); },        // 序列化
    [](const ResourceLocation& id) { ... },            // 反序列化
    [](IWorld& w, const BlockPos& pos, Block& b) { ... } // tick回调
);

// 调度tick
blockTicks->scheduleTick(pos, block, 10, TickPriority::Normal);

// 每游戏刻调用
blockTicks->tick(currentTick);
```

---

### manager/TickManager.hpp & TickManager.cpp

**职责**: 统一管理方块和流体的计划刻调度（外观类）

**主要内容**:
- `TickManager` 类，封装 `ServerTickList<Block>` 和 `ServerTickList<Fluid>`

**方块tick调度**:
- `scheduleBlockTick(pos, block, delay)`
- `scheduleBlockTick(pos, block, delay, priority)`
- `isBlockTickScheduled(pos, block)`
- `isBlockTickPending(pos, block)`
- `cancelBlockTick(pos, block)`

**流体tick调度**:
- `scheduleFluidTick(pos, fluid, delay)`
- `scheduleFluidTick(pos, fluid, delay, priority)`
- `isFluidTickScheduled(pos, fluid)`
- `isFluidTickPending(pos, fluid)`
- `cancelFluidTick(pos, fluid)`

**执行tick**:
- `tick(currentTick)`: 执行当前游戏刻的所有待处理tick

**区块序列化**:
- `getPendingBlockTicks(chunkX, chunkZ, remove)`
- `getPendingFluidTicks(chunkX, chunkZ, remove)`

**统计**:
- `pendingBlockTickCount()`: 待处理方块tick数量
- `pendingFluidTickCount()`: 待处理流体tick数量
- `executedBlockTickCount()`: 本tick已执行的方块tick数量
- `executedFluidTickCount()`: 本tick已执行的流体tick数量

**参考**: MC 1.16.5中World类持有的 `pendingBlockTicks` 和 `pendingFluidTicks`

```cpp
// 使用示例
TickManager tickManager(world);

// 调度方块tick（10tick后执行）
tickManager.scheduleBlockTick(pos, block, 10);

// 调度高优先级流体tick
tickManager.scheduleFluidTick(pos, fluid, 5, TickPriority::High);

// 每游戏刻调用
tickManager.tick(currentTick);
```

## 模块整体分析

### 整体职责

Tick模块负责管理Minecraft世界中的"计划刻"(Scheduled Tick)系统，这是游戏机制的核心部分：

1. **延迟更新**: 方块和流体可以在未来的某个游戏刻执行更新
2. **优先级排序**: 多个tick在同一时刻执行时，按优先级排序
3. **区块感知**: 未加载区块的tick会被重新调度
4. **序列化支持**: 区块卸载时可以保存待处理的tick

### 输入和输出

**输入**:
- `scheduleTick()`: 调度新的tick（位置、目标、延迟、优先级）
- `cancelTick()`: 取消已调度的tick
- `tick()`: 触发当前游戏刻的tick执行

**输出**:
- 执行tick回调（调用目标的tick方法）
- `getPendingTicks()`: 返回区块范围内的tick列表（用于保存）

### 依赖项

**内部依赖**:
- `common/core/Types.hpp`: 基础类型定义
- `common/world/block/BlockPos.hpp`: 方块位置
- `common/world/block/Block.hpp`: 方块基类
- `common/world/block/BlockRegistry.hpp`: 方块注册表
- `common/world/fluid/Fluid.hpp`: 流体基类
- `common/world/fluid/FluidRegistry.hpp`: 流体注册表
- `common/world/IWorld.hpp`: 世界接口
- `common/resource/ResourceLocation.hpp`: 资源位置

**外部依赖**:
- C++标准库: `<set>`, `<unordered_set>`, `<queue>`, `<vector>`, `<functional>`

### 使用方法

```cpp
// 1. 在ServerWorld中创建TickManager
class ServerWorld : public IWorld {
    std::unique_ptr<world::tick::TickManager> m_tickManager;
};

// 2. 初始化
m_tickManager = std::make_unique<world::tick::TickManager>(*this);

// 3. 调度tick
// 方块想要在20tick后执行更新
m_tickManager->scheduleBlockTick(pos, block, 20);

// 流体需要高优先级更新
m_tickManager->scheduleFluidTick(pos, fluid, 5, TickPriority::High);

// 4. 每游戏刻执行
void ServerWorld::tick() {
    m_tickManager->tick(m_currentTick);
}

// 5. 区块卸载时保存tick
auto blockTicks = m_tickManager->getPendingBlockTicks(chunkX, chunkZ, true);
// 序列化到NBT...
```

### 容易踩的坑

1. **区块未加载时tick不执行**: `canTick()` 检查区块是否加载，未加载的区块中的tick会被跳过

2. **tick回调中的状态变化**: tick回调可能修改世界状态（如方块变化），需要注意迭代器的有效性

3. **同一位置重复调度**: 调度已存在的(位置,目标)组合会替换旧的tick

4. **延迟为负数时**: 会自动设置为0（立即执行）

5. **每tick最大执行数量**: 默认最多执行65536个tick，超过的留到下一tick

6. **线程安全**: 所有操作应在主线程执行，非线程安全

7. **指针有效性**: `ScheduledTick` 存储目标对象的原始指针，需要确保目标对象生命周期

8. **双向容器同步**: `m_pendingTicksTree` 和 `m_pendingTicksSet` 需要保持同步，代码中有自动修复机制

### 涉及的测试用例

测试文件位置:
- `tests/common/world/tick/ServerTickListTest.cpp`
- `tests/common/test_tick_manager.cpp`

**测试覆盖**:

1. **TickPriority测试** (`TickPriorityTest`):
   - `FromIntReturnsCorrectPriority`: 整数到优先级转换
   - `FromIntClampsOutOfRange`: 边界值处理
   - `ToIntReturnsCorrectValue`: 优先级到整数转换

2. **ScheduledTick测试** (`ScheduledTickTest`):
   - `Construction`: 构造和成员初始化
   - `ComparisonOrdersByScheduledTick`: 按时间排序
   - `ComparisonOrdersByPriorityWhenSameTick`: 同时间按优先级排序
   - `ComparisonOrdersByIdWhenSameTickAndPriority`: 同时间同优先级按ID排序
   - `EqualityBasedOnPositionAndTarget`: 相等比较基于位置和目标
   - `HashCodeConsistency`: 相同位置目标的哈希一致性

3. **EmptyTickList测试** (`EmptyTickListTest`):
   - `AllOperationsReturnFalse`: 所有查询返回false
   - `ScheduleDoesNothing`: 调度操作无效果
   - `SingletonPattern`: 单例模式验证

4. **ServerTickList测试**:
   - 基本构造测试（完整测试需要Mock ServerWorld）

## 设计模式

1. **外观模式 (Facade Pattern)**:
   - `TickManager` 封装了 `ServerTickList<Block>` 和 `ServerTickList<Fluid>`，提供简化的API

2. **策略模式 (Strategy Pattern)**:
   - `ITickList` 接口允许不同的实现（服务端用 `ServerTickList`，客户端用 `EmptyTickList`）

3. **单例模式 (Singleton Pattern)**:
   - `EmptyTickList<T>::get()` 返回全局唯一实例

4. **模板方法模式 (Template Method Pattern)**:
   - `ITickList` 定义接口，子类实现具体行为

## 性能考虑

1. **双容器设计**:
   - `std::set` 保证排序
   - `std::unordered_set` 保证O(1)查找
   - 空间换时间

2. **tick执行限制**:
   - 每tick最多执行65536个，防止单tick执行时间过长

3. **区块感知**:
   - 只执行已加载区块的tick，避免加载不必要的区块

## 与Minecraft Java 1.16.5的对应关系

| 本项目类 | MC Java 1.16.5类 |
|---------|-----------------|
| `TickPriority` | `net.minecraft.world.TickPriority` |
| `ScheduledTick` | `net.minecraft.world.NextTickListEntry` |
| `ITickList` | `net.minecraft.world.ITickList` |
| `EmptyTickList` | `net.minecraft.world.EmptyTickList` |
| `ServerTickList` | `net.minecraft.world.server.ServerTickList` |
| `TickManager` | `World.pendingBlockTicks` + `World.pendingFluidTicks` |
