# 实体存储模块

## 目录结构

```
src/common/world/storage/entity/
├── EntityKey.hpp              # 实体存储键格式定义（键格式：`{chunkX}:{chunkZ}:{uuid}`）
├── EntityStorageManager.hpp   # 实体存储管理器（使用 RocksDB 持久化，gzip 压缩的 NBT 数据）
├── EntityStorageManager.cpp   # 实体存储管理器实现
└── README.md                  # 本文档
```

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
5. **保存语义**：当前实体持久化只支持“区块卸载保存”和“显式全量保存”，不要误以为存在独立 dirty 刷盘通道
