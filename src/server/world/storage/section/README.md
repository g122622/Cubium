# Section 存储子模块

## 目录结构树

```text
section/
├── SectionManager.hpp/cpp    # 单维度紧凑数组批次捕获、固定缓冲区批量读取与提交
└── README.md                # 模块依赖与数据生命周期约束
```

## 内部模块关系

`appendChunk` 借用稳定区块的紧凑数组，通过 `SectionCodec` 直接追加到拥有数据的 WriteBatch。
`saveBatch` 只提交批次；`loadSectionsSync` 通过 RocksDBDatabase 返回按输入排序的读取句柄。
本模块不持有常驻段缓存。

## 上下游外部依赖关系

- 上游仅为 SingleLevelStorageManager，世界层经该门面提交区块列表。
- 下游依赖 SectionCodec、SectionKey、RocksDBDatabase、公共区块模型与追踪宏。

## 容易踩的坑

- 多个脏区块必须聚合成一个 WriteBatch，不能退化为逐段或逐区块提交。
- 构建批次时区块必须稳定；append 返回后批次已拥有数据，后台任务不能持有数组借用。
- 空 optional 只代表段不存在；损坏、截断、空值必须传播错误。
- 批量读取顺序必须与 key 顺序一致，同一管理器只接受本维度的 key。
- 未分配的空气段也写入仅含生物群系的记录，恢复时不创建方块段。
- 读取句柄在 Compute 阶段恢复完成后释放，不能长期固定 RocksDB 缓存块或跨越数据库关闭。
- 显式 sync 要求等待 fsync，其余写入遵循一致性模式；压缩只由 RocksDB 执行。
