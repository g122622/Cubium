# I/O 系统 (IO)

提供异步 I/O 能力和文件操作工具。

## 文件说明

| 文件 | 职责 |
|------|------|
| `IOWorker.hpp/cpp` | 异步 I/O 工作线程，处理区块读写请求 |
| `FileUtil.hpp/cpp` | 文件工具函数，原子写入、目录操作等 |
| `CompressionUtil.hpp/cpp` | GZIP/Zlib 压缩和解压缩工具 |

## 异步 I/O 架构

```
┌─────────────┐     ┌─────────────────┐     ┌──────────────┐
│ 主线程      │────▶│ 任务队列        │────▶│ IOWorker线程 │
│             │     │                 │     │              │
│ loadChunk() │     │ LoadRequest     │     │ RegionFile   │
│ saveChunk() │     │ SaveRequest     │     │ NBT序列化    │
│ sync()      │     │ SyncRequest     │     │ 压缩/解压    │
└─────────────┘     └─────────────────┘     └──────────────┘
```

## IOWorker 接口

```cpp
// 异步加载区块
std::future<Result<std::optional<nbt::CompoundTag>>>
loadChunk(ChunkCoord x, ChunkCoord z);

// 异步保存区块
std::future<Result<void>>
saveChunk(ChunkCoord x, ChunkCoord z, std::unique_ptr<nbt::CompoundTag> nbt);

// 同步所有写入
std::future<Result<void>> sync();
```

## 原子写入

为确保数据完整性，写入文件使用原子操作：

1. 写入临时文件（`.tmp` 后缀）
2. 调用 `fsync` 确保数据落盘
3. 原子重命名到目标文件

```cpp
Result<void> FileUtil::atomicWrite(const Path& path, const void* data, size_t size) {
    Path tempPath = path.string() + ".tmp";
    // 1. 写入临时文件
    writeFile(tempPath, data, size);
    // 2. 同步到磁盘
    fsync(tempPath);
    // 3. 原子重命名
    rename(tempPath, path);
}
```

## 压缩类型

| 类型 | 用途 | 压缩率 | 速度 |
|------|------|--------|------|
| GZIP | level.dat, player.dat | 高 | 慢 |
| Zlib | 区块数据（最常用） | 高 | 中 |
| Uncompressed | 小数据 | 无 | 快 |

## 容易踩的坑

1. **线程安全**：RegionFile 的读写需要互斥锁保护
2. **错误传播**：异步操作的错误需要正确传递到 future
3. **资源限制**：限制同时打开的文件数量
4. **中断处理**：程序退出时需要等待所有写入完成
