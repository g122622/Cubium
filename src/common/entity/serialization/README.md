# Entity Serialization

实体序列化模块，负责实体与 NBT 数据之间的转换。

## 目录结构

```
src/common/entity/serialization/
├── EntityDeserializer.hpp/cpp   # 实体反序列化器
├── EntityNbtKeys.hpp            # NBT 键名常量
├── NbtHelper.hpp/cpp            # NBT 辅助工具函数
└── README.md
```

## 内部模块关系

```
┌─────────────────────┐
│  EntityDeserializer │
└──────────┬──────────┘
           │
     ┌─────┴─────┐
     │           │
     ▼           ▼
┌─────────┐  ┌────────────┐
│  Nbt    │  │EntityNbtKeys│
│ Helper  │  │  (常量)     │
└────┬────┘  └────────────┘
     │
     ▼
┌─────────────────┐
│ nbt::tags 命名空间│
│ (compound_tag等) │
└─────────────────┘
```

**依赖关系**：
- `EntityDeserializer` 依赖 `EntityRegistry` 获取实体类型
- `EntityDeserializer` 依赖 `Entity::readFromNBT()` 和 `Entity::writeToNBT()`
- `EntityDeserializer` 依赖 `NbtHelper` 读取 NBT 数据
- `EntityDeserializer` 依赖 `EntityNbtKeys` 获取键名常量

## 外部依赖关系

### 被依赖方

本模块被以下模块使用：

1. **存档系统** (`src/common/world/storage/`)
   - 实体数据持久化
   - 区块实体加载/保存

2. **网络同步** (`src/common/network/`)
   - 实体数据包序列化
   - 客户端实体状态同步

3. **命令系统** (`src/common/command/`)
   - `/summon` 命令实体生成
   - NBT 数据解析

### 依赖方

本模块依赖以下模块：

1. **实体核心** (`src/common/entity/core/`)
   - `Entity` 基类
   - `EntityType` 类型定义
   - `EntityRegistry` 注册表

2. **NBT 系统** (`src/common/util/nbt/`)
   - `compound_tag` 复合标签
   - `list_tag` 列表标签
   - NBT 上下文（Java 版格式）

3. **核心类型** (`src/common/core/`)
   - `Result<T>` 结果类型
   - `Error` 错误类型
   - `Types.hpp` 基本类型定义

4. **世界接口** (`src/common/world/`)
   - `IWorld` 世界访问接口

## 容易踩的坑

### 1. 实体类型未注册

```cpp
// 错误：实体类型未注册
auto result = EntityDeserializer::deserialize(tag, world);
// 如果 "id" 对应的类型未注册，返回 InvalidEntity 错误

// 正确：确保实体类型已注册
// 在游戏初始化时调用 VanillaEntities::registerAll()
```

### 2. 乘客实体的所有权

```cpp
// 注意：deserialize() 中加载的乘客实体
// 仅建立了骑乘关系，不会自动添加到世界
// 调用方需要将乘客实体添加到世界中

auto result = EntityDeserializer::deserialize(tag, world);
if (result.success()) {
    auto& entity = result.value();
    // 主实体需要添加到世界
    world.addEntity(std::move(entity));

    // 乘客实体已在 deserialize 中建立骑乘关系
    // 但调用方需要处理乘客的添加（如果需要）
}
```

### 3. 压缩格式

```cpp
// EntityDeserializer 使用 gzip 压缩格式
// 与 PlayerSaveData 保持一致
// 解压时先尝试解压，失败则尝试直接解析

// zlib 压缩级别：Z_BEST_COMPRESSION (9)
// 提供最佳压缩率，但压缩速度较慢
```

### 4. NBT 键名大小写

```cpp
// MC 1.16.5 Java 版 NBT 键名区分大小写
// 使用 EntityNbtKeys.hpp 中的常量，避免硬编码

// 正确
tag.put(nbt_keys::ID, "minecraft:pig");

// 错误：大小写错误
tag.put("ID", "minecraft:pig");  // 应该是 "id"
tag.put("uuid", "...");          // 应该是 "UUID"
```

### 5. 递归乘客加载

```cpp
// deserialize() 会递归处理 Passengers 列表
// 但只建立 Entity 之间的骑乘关系
// 乘客实体的世界引用由传入的 world 参数设置

// 如果乘客反序列化失败，会记录警告日志
// 但不会中断主实体的返回
```

### 6. 空指针处理

```cpp
// deserialize() 的 world 参数可以为 nullptr
// 某些实体（如物品实体）可能不需要世界引用
// 但大多数实体需要世界引用才能正常工作

// EntityType::create() 返回 std::unique_ptr<Entity>
// 需要检查返回值是否为 nullptr
```

### 7. 二进制格式兼容性

```cpp
// deserializeFromBinary() 使用 Java 版 NBT 格式
// nbt::contexts::java

// 如果需要支持基岩版格式，需要：
// 1. 检测输入数据格式
// 2. 使用对应的上下文（bedrock_disk 或 bedrock_net）
// 3. 处理键名差异（基岩版使用不同的键名）
```

## 参考

- MC 1.16.5 `net.minecraft.entity.EntityType.loadEntityAndExecute()`
- MC 1.16.5 `net.minecraft.entity.Entity.writeWithoutTypeId()`
- MC 1.16.5 `net.minecraft.entity.Entity.read(CompoundNBT)`
- MC 1.16.5 `net.minecraft.nbt.CompoundNBT`
