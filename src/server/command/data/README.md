# Data Accessor 模块

本目录包含 `/data` 命令的数据访问器实现，用于统一访问不同来源的 NBT 数据。

## 目录结构

```
data/
├── DataAccessor.hpp    # 数据访问器接口（IDataAccessor）和三个实现类
└── DataAccessor.cpp    # 数据访问器实现
```

## 内部模块关系

```
┌─────────────────────┐
│   IDataAccessor     │  ← 抽象接口
│  (抽象接口)          │
└──────────┬──────────┘
           │ 继承
     ┌─────┼─────────────┐
     │     │             │
     ▼     ▼             ▼
┌────────┐ ┌────────┐ ┌────────┐
│ Block  │ │ Entity │ │Storage │
│DataAc- │ │DataAc- │ │DataAc- │
│ cessor │ │ cessor │ │ cessor │
└────────┘ └────────┘ └────────┘
     │          │          │
     └──────────┼──────────┘
                │
                ▼
        ┌───────────────┐
        │CommandStorage │  ← 独立的存储管理类
        └───────────────┘
```

- `IDataAccessor`：数据访问器的抽象接口，定义 getData/mergeData 等方法
- `BlockDataAccessor`：方块实体数据访问，通过 `IWorld::getBlockEntity()` 获取 BlockEntity，使用 `saveToNBT()`/`loadFromNBT()` 进行原生 NBT 序列化/反序列化
- `EntityDataAccessor`：实体数据访问，直接持有 Entity 指针，使用 `writeToNBT()`/`readFromNBT()` 进行原生 NBT 序列化/反序列化
- `StorageDataAccessor`：命令存储数据访问，通过 `CommandStorage` 管理持久化数据
- `CommandStorage`：独立的存储管理类，被 `StorageDataAccessor` 使用

## 上下游外部依赖关系

### 上游依赖（本模块依赖的外部模块）

- `common/command/arguments/NbtPath.hpp` - NBT 路径解析
- `common/command/exceptions/CommandExceptions.hpp` - 命令异常类型
- `common/resource/ResourceLocation.hpp` - 资源位置标识符
- `common/util/nbt/Nbt.hpp` - NBT 标签系统
- `common/world/block/BlockPos.hpp` - 方块位置
- `common/world/IWorld.hpp` - 世界接口（用于获取 BlockEntity 和通知方块更新）
- `common/world/blockentity/BlockEntity.hpp` - 方块实体基类（saveToNBT/loadFromNBT）
- `common/entity/core/Entity.hpp` - 实体基类（writeToNBT/readFromNBT）
- `common/entity/entities/player/Player.hpp` - 玩家实体（用于判断是否为玩家）

### 下游依赖（依赖本模块的外部模块）

- `src/server/command/commands/DataCommand.hpp` - `/data` 命令实现，使用本模块的访问器

## 容易踩的坑

1. **玩家数据保护**：`EntityDataAccessor::mergeData()` 会检查是否为玩家实体，玩家数据不允许直接修改，会抛出 `CommandException`。这是 MC 原版的行为。

2. **方块实体检测**：`BlockDataAccessor::getData()` 和 `mergeData()` 会检测 `m_blockEntity` 是否为空，不存在则抛出异常。调用前应使用 `isValid()` 检查。

3. **UUID 保护**：`EntityDataAccessor::mergeData()` 在加载 NBT 数据后会恢复实体的 UUID，防止 `/data merge` 命令意外修改实体 UUID。这与 MC Java 的 `EntityDataAccessor.setData()` 行为一致。

4. **合并语义**：所有访问器的 `mergeData()` 实现「获取当前数据 → 合并传入数据 → 重新加载」的三步语义。传入数据的每个键值对会覆盖当前数据中的对应键，未出现在传入数据中的键保持不变。这与 MC Java 的 `DataCommands.mergeData()` 行为一致。

5. **存储持久化**：`CommandStorage` 的数据需要显式调用 `save()` 和 `load()` 进行持久化，不会自动保存。服务器关闭前必须调用 `save()`。

6. **存储深拷贝**：`CommandStorage::get()` 返回的是深拷贝，修改返回值不会影响存储中的数据，必须通过 `set()` 或 `mergeData()` 写回。

7. **统一存储实例**：`CommandStorage` 由 `IServer::commandStorage()` 管理，通过 `server->commandStorage()` 获取。不要使用局部 `static CommandStorage`，否则不同 storage 子命令的数据互不共享。

8. **BlockEntity NBT 完整性**：`BlockDataAccessor` 使用 `saveToNBT()`/`loadFromNBT()` 原生 NBT 序列化，可获取/设置方块实体的全部数据。如果某个 BlockEntity 子类未实现 `saveToNBT()`/`loadFromNBT()`，则只有基类的 id/x/y/z 字段会被处理。确保需要通过 `/data` 命令访问的 BlockEntity 子类已正确实现这两个方法。

9. **BlockEntity 方块更新**：`BlockDataAccessor::mergeData()` 在数据合并后会调用 `IWorld::notifyBlockUpdate()` 通知客户端方块更新，确保客户端与服务端状态同步。

## 参考

MC 1.21.11: `net.minecraft.server.commands.data.DataAccessor` 及其实现类
