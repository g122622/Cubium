# 实体存储模块

## 目录结构

```
src/common/world/storage/entity/
├── EntityKey.hpp              # 实体存储键格式定义
├── EntityStorageManager.hpp  # 实体存储管理器
├── EntityStorageManager.cpp  # 实体存储管理器实现
└── README.md                 # 本文档
```

## 文件详解

### EntityKey.hpp

实体存储键结构体，定义了 RocksDB 中实体数据的键格式。

**键格式**: `{chunkX}:{chunkZ}:{uuid}`

- `chunkX` - 区块 X 坐标（i32）
- `chunkZ` - 区块 Z 坐标（i32）
- `uuid` - 实体 UUID（32位十六进制字符串）

**设计理由**：
- 前缀扫描：同一区块的实体键具有相同前缀，可用 `Seek` + 范围扫描高效加载
- 唯一性：UUID 保证每个实体键全局唯一
- 字典序：RocksDB 按字典序排列，同区块实体自然聚集

### EntityStorageManager.hpp/cpp

实体存储管理器，负责实体的持久化存储。

**主要方法**：

| 方法 | 说明 |
|------|------|
| `saveEntity()` | 保存单个实体 |
| `loadEntity()` | 加载单个实体 |
| `deleteEntity()` | 删除单个实体 |
| `loadEntitiesInChunk()` | 加载区块内所有实体 |
| `saveEntitiesInChunk()` | 批量保存区块内实体 |
| `deleteEntitiesInChunk()` | 删除区块内所有实体 |
| `markDirty()` | 标记实体为脏 |
| `flushDirty()` | 刷盘脏实体 |

**序列化格式**：
- 键：字符串格式 `{chunkX}:{chunkZ}:{uuid}`
- 值：gzip 压缩的 NBT 二进制数据（Java 版格式）

**列族映射**：
| 列族 | 维度 |
|------|------|
| `entities_overworld` | 主世界 (0) |
| `entities_nether` | 下界 (-1) |
| `entities_the_end` | 末地 (1) |

## 内部模块关系

```
EntityStorageManager
    │
    ├── EntityDeserializer  (反序列化 NBT → Entity)
    │       └── EntityRegistry (查找 EntityType)
    │       └── Entity::readFromNBT() (填充数据)
    │
    ├── Entity::writeToNBT()  (序列化 Entity → NBT)
    │
    ├── RocksDBDatabase  (底层键值存储)
    │       └── ColumnFamilies (entities_overworld/nether/the_end)
    │
    └── EntityKey  (键格式工具)
```

## 外部依赖关系

```
SingleLevelStorageManager
    └── EntityStorageManager  (通过 entityStorage() 访问)

ServerWorld
    └── EntityStorageManager  (区块加载/卸载时调用)

Entity (序列化)
    ├── EntityNbtKeys  (NBT 键名常量)
    ├── NbtHelper      (NBT 读写工具)
    └── EntityDeserializer (反序列化工厂)
```

## 容易踩的坑

1. **区块坐标计算**：实体位置转区块坐标时使用 `floor(pos / 16.0)`，负坐标要向下取整
2. **范围扫描边界**：区块前缀扫描时，结束键需要加 `0xFF` 确保覆盖所有 UUID
3. **维度隔离**：不同维度使用不同列族，避免数据混淆
4. **乘客递归**：实体序列化时乘客嵌套在车辆 NBT 中，加载时需递归处理
5. **脏数据追踪**：当前 flushDirty() 尚未完全实现，通过区块卸载保存替代
