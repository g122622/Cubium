# Section 存储子模块

## 概述

`section/` 负责单维度 Section 数据的缓存、批量读取、写回和卸载协调，是 `SingleLevelStorageManager` 与 RocksDB 之间的直接桥梁。

本子模块的目标有三个：

1. 统一 Section 级缓存语义，避免运行时重复反序列化
2. 对同一维度下的多个 Section 读请求做聚合，优先命中缓存，未命中时走一次批量 DB 读取
3. 维护脏标记与批量刷盘流程，供自动保存和关闭流程复用

## 目录结构树

```text
section/
├── README.md                      # 本文档
├── SectionCache.hpp/cpp           # 线程安全 LRU 缓存，保存 SectionData、脏标记、访问统计
├── SectionManager.hpp/cpp         # 单维度 Section 管理器，负责加载/保存/缓存管理
```

## 内部模块关系

```mermaid
flowchart LR
    A[SingleLevelStorageManager] --> B[SectionManager]
    B --> C[SectionCache]
    B --> D[RocksDBDatabase]
    B --> E[SectionCodec]

    style A fill:#8ecae6,stroke:#1d4ed8,color:#111
    style B fill:#90be6d,stroke:#2f6f3e,color:#111
    style C fill:#ffd166,stroke:#b7791f,color:#111
    style D fill:#f4a261,stroke:#b45309,color:#111
    style E fill:#cdb4db,stroke:#6d28d9,color:#111
```

## 上下游外部依赖关系

### 上游（谁依赖了这个模块）

- `SingleLevelStorageManager` - 通过 `SectionManager` 进行区块数据的读写

### 下游（这个模块依赖了谁）

- `db/RocksDBDatabase` - RocksDB 数据库封装
- `db/SectionCodec` - Section 序列化/反序列化
- `db/SectionKey` - Section 键结构
- `task/StorageTaskManager` - 异步任务调度
- `rocksdb` - RocksDB 库
- `spdlog` - 日志
- `perfetto` - 性能追踪

## 容易踩的坑

1. **批量读取只适用于同一个 `SectionManager`**：同一个 `SectionManager` 只服务一个 `DimensionId`，因此批量读取天然只覆盖同一维度下的同一列族。

2. **空指针只表示”Section 不存在”**：`loadSectionsSync()` 返回 `Result<std::vector<std::shared_ptr<const SectionData>>>`。外层失败表示系统级错误；外层成功但某个位置是空指针，才表示该 Section 不存在。

3. **缓存命中和批量读取必须分开处理**：不要把所有 key 都无脑送进 `MultiGet`。正确做法是先查缓存，只把 miss 交给数据库。

4. **批量读取返回顺序必须稳定**：上层 `loadChunk()` 依赖返回顺序与 `sectionY` 顺序一致，不能在实现里打乱顺序。

5. **脏数据写回仍然走 `WriteBatch`**：`MultiGet` 只优化读取；写路径的批量能力仍然是 `rocksdb::WriteBatch`。
