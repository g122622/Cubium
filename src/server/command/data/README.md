# Data Accessor 模块

本目录包含 `/data` 命令的数据访问器实现，用于统一访问不同来源的 NBT 数据。

## 目录结构

```
data/
├── DataAccessor.hpp    # 数据访问器接口和实现
└── DataAccessor.cpp    # 数据访问器实现
```

## 核心类

### IDataAccessor

数据访问器的抽象接口，定义了所有数据访问器必须实现的方法：

```cpp
class IDataAccessor {
public:
    virtual ~IDataAccessor() = default;

    // 获取完整 NBT 数据
    virtual std::unique_ptr<nbt::tags::compound_tag> getData() const = 0;

    // 合并 NBT 数据
    virtual void mergeData(const nbt::tags::compound_tag& data) = 0;

    // 获取显示名称
    virtual std::string getDisplayName() const = 0;

    // 获取修改消息
    virtual std::string getModifiedMessage() const = 0;

    // 获取查询消息
    virtual std::string getQueryMessage(const nbt::tags::tag& nbt) const = 0;

    // 获取获取消息
    virtual std::string getGetMessage(const NbtPath& path, double scale, i32 value) const = 0;
};
```

### BlockDataAccessor

方块实体数据访问器，用于访问容器、告示牌等方块实体的 NBT 数据。

**功能：**
- 从 `BlockEntity` 读取 NBT 数据
- 将 NBT 数据合并到 `BlockEntity`
- 处理 JSON 到 NBT 的转换

**使用示例：**
```cpp
BlockPos pos(10, 64, 20);
BlockDataAccessor accessor(world, pos);
auto data = accessor.getData();
```

### EntityDataAccessor

实体数据访问器，用于访问实体的 NBT 数据。

**功能：**
- 从 `Entity` 读取 NBT 数据（位置、UUID、标签、生命值等）
- 合并 NBT 数据到实体（玩家除外）
- 处理 `LivingEntity` 特有数据

**使用示例：**
```cpp
EntityDataAccessor accessor(entity);
auto data = accessor.getData();
accessor.mergeData(nbtData);
```

### StorageDataAccessor

命令存储数据访问器，用于访问持久化命令存储。

**功能：**
- 读取/写入命令存储数据
- 支持命名空间存储 ID

**使用示例：**
```cpp
ResourceLocation storageId("minecraft", "my_storage");
StorageDataAccessor accessor(&commandStorage, storageId);
auto data = accessor.getData();
accessor.mergeData(nbtData);
```

### CommandStorage

命令存储管理器，管理所有 `/data` 命令的持久化存储。

**功能：**
- `get(id)` - 获取存储数据
- `set(id, data)` - 设置存储数据
- `exists(id)` - 检查存储是否存在
- `listAll()` - 列出所有存储
- `clear(id)` - 清除存储
- `save(json)` / `load(json)` - 序列化/反序列化

## 与 DataCommand 的集成

这些数据访问器被 `DataCommand` 使用，支持以下命令：

- `/data get block <pos> [<path>] [<scale>]`
- `/data get entity <target> [<path>] [<scale>]`
- `/data get storage <id> [<path>] [<scale>]`
- `/data set block <pos> <path> <value>`
- `/data set entity <target> <path> <value>`
- `/data set storage <id> <path> <value>`
- `/data merge block <pos> <nbt>`
- `/data merge entity <target> <nbt>`
- `/data merge storage <id> <nbt>`
- `/data remove block <pos> <path>`
- `/data remove entity <target> <path>`
- `/data remove storage <id> <path>`

## 数据流

```
┌─────────────────┐    getData()    ┌─────────────────┐
│  Data Source    │ ──────────────> │  IDataAccessor  │ ──> NBT Compound
│ (Block/Entity/  │                 │                 │
│    Storage)     │ <────────────── │                 │ <── NBT Compound
└─────────────────┘   mergeData()   └─────────────────┘
```

## 注意事项

1. **玩家数据保护**：`EntityDataAccessor` 不允许直接修改玩家数据，会抛出异常
2. **方块实体检测**：`BlockDataAccessor` 会检测方块实体是否存在，不存在则抛出异常
3. **存储持久化**：`CommandStorage` 数据需要显式调用 `save()` 和 `load()` 进行持久化
4. **JSON 转换**：`BlockDataAccessor` 使用 JSON 作为中间格式，可能存在精度损失

## 依赖关系

- `NbtPath` - NBT 路径解析和操作
- `BlockEntity` - 方块实体基类
- `Entity` / `LivingEntity` - 实体基类
- `ResourceLocation` - 资源位置标识符
- `IWorld` - 世界接口

## 参考

MC 1.16.5: `net.minecraft.command.data.DataAccessor` 及其实现类
