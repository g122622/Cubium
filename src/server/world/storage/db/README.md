# RocksDB 存储层

## 目录结构

```
src/server/world/storage/db/
├── RocksDBDatabase.hpp/cpp    # 数据库封装（打开/读写/有界范围读取/批量/范围删除/迭代/快照/备份）
├── RocksDBConfig.hpp          # RocksDB 配置（MemTable/LSM/压缩/WAL/后台线程/Bloom），并由此推导写选项
├── ColumnFamilies.hpp         # 列族名常量与 16 个列族的创建/命名规范
├── ConsistencyMode.hpp        # 一致性模式枚举，唯一决定"默认写入是否等 WAL fsync"
├── SectionKey.hpp             # Section 键结构（维度 + 区块坐标 + 段 Y，13 字节编码）
├── SectionCodec.hpp/cpp       # 紧凑数组直接写入批次与恢复
└── README.md                  # 本文档
```

## 内部模块关系

```mermaid
flowchart LR
    A[RocksDBConfig] -->|createWriteOptions / createColumnFamilyOptions| B[RocksDBDatabase]
    C[ConsistencyMode] -->|默认 sync 级别| A
    D[ColumnFamilies] -->|列族名| B
    E[SectionKey] --> F[SectionCodec]
    G[SectionManager] -->|put / writeBatch / deleteRange| B
    H[EntityStorageManager] -->|put / deleteRange / writeBatch| B
```

`RocksDBDatabase` 是全存储层唯一的数据库句柄持有者（内部持 `std::unique_ptr<rocksdb::DB>`）；其余模块只通过它访问 RocksDB，不直接持有 `rocksdb::DB*`。备份由常驻的 `BackupManager` 持有引擎、经 `RocksDBDatabase::createBackupWith` 完成，数据库不对外暴露裸句柄。

## 上下游外部依赖关系

### 上游（谁依赖了这个模块）

- `section/SectionManager` - Section 读写与批量刷盘
- `entity/EntityStorageManager`、`blockentity/BlockEntityStorageManager` - 实体/方块实体持久化
- `player/PlayerDataManager`、`scoreboard/` - 玩家与计分板数据
- `snapshot/BackupManager` - 备份/恢复
- `SingleLevelStorageManager` - 打开/关闭数据库、装配 `RocksDBConfig`

### 下游（这个模块依赖了谁）

- `rocksdb` - 键值存储引擎
- `common/core/Result.hpp`、`common/core/Types.hpp`
- `common/profiler/TraceEvents.hpp` - `TraceEvents.Storage.Db` 追踪
- `spdlog` - 日志

## 容易踩的坑

1. **`options.sync` 只能由 `ConsistencyMode` 决定**：`consistencyModeSyncsEveryWrite()`（`ConsistencyMode.hpp`）是唯一的判据，`enableWAL` 只管 WAL 写不写、不管 fsync。历史上这里还有一个独立的 `walSync` 布尔开关且默认 `true`，把服务端实际配置的 `Eventual` 完全架空——每次 `put`/`deleteRange` 都要等一次 fsync，关服时 2048 个 section + 1089 次范围删除因此多花约 10 秒。新增写入开关前先确认它不会重新引入"配置项失效"。
2. **`sync=false` 不等于不写 WAL**：`disableWAL=false` 时写入仍进 WAL，进程崩溃可由 WAL 回放恢复；`sync` 只决定是否等待 fsync（即是否抵御操作系统崩溃/断电）。
3. **单条写入永远比一次批量提交贵**：`put`/`del`/`deleteRange` 各自是一次完整的 Write 调用，`sync=true` 时各付一次 fsync。需要写多条数据时必须用 `writeBatch`，把 N 次 fsync 压成 1 次。
4. **`writeBatch` 拒绝空批次**：空批次同样要付一次完整 WAL 写入，调用方必须在提交前判空（这也是断言而非静默跳过）。
5. **`getCF` 返回的 `ColumnFamilyHandle*` 由 `RocksDBDatabase` 拥有**：调用方不负责释放，但必须保证句柄生命周期内完成写入——`close()` 会先 flush 再销毁句柄，任何跨越 `close()` 的批次都会用到悬垂句柄。
6. **`WriteBatch` 内按插入顺序生效**：批内各条目获得递增序列号，因此"先 `DeleteRange` 整段删除、再 `Put` 新行"会得到"旧行消失、新行存活"的结果；顺序写反则新行会被自己的删除指令抹掉。
7. **`close()` 会等待 flush 完成**：本层写入默认不逐条 fsync，`close()` 若不等待就把 memtable 交给析构，这次会话的数据就只剩 WAL 一条退路。
8. **按区块读取使用 `readRange`**：边界必须交给底层迭代器；Seek 返回后再检查结束键，无法限制寻找下一条可见记录时越过大量已删除键的开销。方法内部管理边界与迭代器的生命周期，并检查最终读取状态，返回值拥有自己的字节存储。

9. **原生段只接受当前紧凑布局**：调色板顺序、位数组、完整反向哈希槽位与段计数原样保存；不展开方块、不进行业务层压缩或内容哈希。
10. **读取句柄必须及时释放**：`multiGetPinned` 的空 optional 表示不存在，存在但零长度的值仍需报损坏；句柄只能在数据库存活期间使用，恢复到最终区块数组后即释放。
11. **数组不能强制转换为对象指针**：读取地址可能未对齐；头部与数值数组经 memcpy 恢复，布局固定为小端。

12. **关闭顺序**：停止提交请求后先排空 I/O，再排空 Compute 恢复任务；所有固定读取句柄释放之后才能销毁数据库。
