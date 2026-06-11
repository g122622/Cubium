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
- `BlockDataAccessor`：方块实体数据访问，通过 `IWorld::getBlockEntity()` 获取 BlockEntity
- `EntityDataAccessor`：实体数据访问，直接持有 Entity 指针
- `StorageDataAccessor`：命令存储数据访问，通过 `CommandStorage` 管理持久化数据
- `CommandStorage`：独立的存储管理类，被 `StorageDataAccessor` 使用

## 上下游外部依赖关系

### 上游依赖（本模块依赖的外部模块）

- `common/command/arguments/NbtPath.hpp` - NBT 路径解析
- `common/command/exceptions/CommandExceptions.hpp` - 命令异常类型
- `common/resource/ResourceLocation.hpp` - 资源位置标识符
- `common/util/nbt/Nbt.hpp` - NBT 标签系统
- `common/world/block/BlockPos.hpp` - 方块位置
- `common/world/IWorld.hpp` - 世界接口（用于获取 BlockEntity）
- `common/world/blockentity/BlockEntity.hpp` - 方块实体基类
- `common/entity/core/Entity.hpp` - 实体基类
- `common/entity/core/LivingEntity.hpp` - 生物实体（用于生命值、吸收值等数据）
- `common/entity/entities/player/Player.hpp` - 玩家实体（用于判断是否为玩家）

### 下游依赖（依赖本模块的外部模块）

- `src/server/command/commands/DataCommand.hpp` - `/data` 命令实现，使用本模块的访问器

## 容易踩的坑

1. **玩家数据保护**：`EntityDataAccessor::mergeData()` 会检查是否为玩家实体，玩家数据不允许直接修改，会抛出 `CommandException`。这是 MC 1.16.5 的行为。

2. **方块实体检测**：`BlockDataAccessor::getData()` 和 `mergeData()` 会检测 `m_blockEntity` 是否为空，不存在则抛出异常。调用前应使用 `isValid()` 检查。

3. **JSON 转换精度损失**：`BlockDataAccessor` 使用 JSON 作为中间格式（BlockEntity 的 save/load 接口），JSON 与 NBT 之间的转换可能存在精度损失，特别是浮点数。

4. **存储持久化**：`CommandStorage` 的数据需要显式调用 `save()` 和 `load()` 进行持久化，不会自动保存。服务器关闭前必须调用 `save()`。

5. **存储深拷贝**：`CommandStorage::get()` 返回的是深拷贝，修改返回值不会影响存储中的数据，必须通过 `set()` 或 `mergeData()` 写回。

6. **统一存储实例**：`CommandStorage` 由 `IServer::commandStorage()` 管理，通过 `server->commandStorage()` 获取。不要使用局部 `static CommandStorage`，否则不同 storage 子命令的数据互不共享。

7. **LivingEntity 吸收值**：`EntityDataAccessor` 通过 `LivingEntity::absorptionAmount()` 和 `setAbsorptionAmount()` 读写 `"AbsorptionAmount"` NBT 键。`setAbsorptionAmount` 会将值限制在 `[0, maxAbsorption]` 范围内，因此合并 NBT 数据时不需要额外的值域检查。

## 参考

MC 1.16.5: `net.minecraft.command.data.DataAccessor` 及其实现类
