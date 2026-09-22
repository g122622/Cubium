# 实体存储模块

## 目录结构

```
src/server/world/storage/entity/
├── EntityKey.hpp              # 实体存储键格式定义（键格式：`{chunkX}:{chunkZ}:{uuid}`）
├── EntityStorageManager.hpp   # 实体存储管理器（RocksDB 持久化 gzip 压缩的 NBT），含 ChunkEntityWrite 多区块批次
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
6. **多区块落盘必须用 `replaceEntitiesInChunks`**：它把一批区块的“整段删除 + 写存活实体”折进**一个** `WriteBatch`，一次提交只付一次 WAL fsync。逐区块各提交一次是区块数量次 fsync——关服时视野距离 16 有 1089 个区块，实测约 3.4 秒，而其中绝大多数区块根本没有实体、整段删除是空操作。单区块卸载也应走这个接口，把“1 次删除 + N 次写入”合并成 1 次写入。
7. **批内条目必须“先删后写”**：`replaceEntitiesInChunks` 对每个区块先 `DeleteRange` 再 `Put`。批内按插入顺序分配递增序列号，所以后写入的行不会被先插入的 RangeTombstone 抹掉；顺序写反会让刚写进去的实体被自己的删除指令清空，且没有任何报错。
8. **落键必须落在本条目自己的删除前缀之内**：键由调用方传入的区块坐标构造，**不**按实体当前位置推导。一旦按位置推导，实体跨区块漂移后写出的键就落在删除范围之外，旧行永远清不掉，会在同一 UUID 上不断堆积副本。该不变量在 `replaceEntitiesInChunks` 内有断言把关。
9. **同一批次内区块坐标不得重复**：重复会让后一条的 `DeleteRange` 把前一条刚写入的实体一并抹掉，是静默丢数据的来源；同样有断言把关。
