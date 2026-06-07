# Cache 模块

LRU 缓存实现模块，提供高性能的坐标到值映射缓存。

## 目录结构

```
cache/
├── Long2IntLRUCache.hpp      # 链表+哈希表实现的 LRU 缓存（保留用于性能对比）
├── Long2IntLRUCache.cpp      # 对应实现文件
├── OpenAddressingLRUCache.hpp # 开放寻址 LRU 缓存（推荐用于生产环境）
├── OpenAddressingLRUCache.cpp # 对应实现文件
└── README.md                  # 本文档
```

## 内部模块关系

```
┌─────────────────────┐     ┌─────────────────────────┐
│  Long2IntLRUCache   │     │  OpenAddressingLRUCache │
│  (链表+哈希表实现)   │     │  (开放寻址实现)          │
│                     │     │                         │
│  - 标准 STL 实现    │     │  - 自定义数据结构       │
│  - 单条淘汰         │     │  - 批量淘汰             │
│  - 无统计功能       │     │  - 性能统计             │
└─────────────────────┘     └─────────────────────────┘
            │                         │
            └───────────┬─────────────┘
                        ▼
            ┌─────────────────────┐
            │  common/core/Types  │
            │  (基础类型定义)      │
            └─────────────────────┘
```

两个实现互相独立，接口一致，可根据性能需求选择。

| 特性 | Long2IntLRUCache | OpenAddressingLRUCache |
|------|-----------------|----------------------|
| 内存布局 | 非连续 | 连续（更好的缓存局部性） |
| 淘汰策略 | 单条淘汰 | 批量淘汰 |
| 性能统计 | 无 | 有 |
| 推荐场景 | 性能对比测试 | 生产环境 |

## 上下游外部依赖关系

**上游依赖**：
- `common/core/Types.hpp` - 基础类型定义 (i8, i16, i32, i64, u32, u64, f32)
- 标准库：`<unordered_map>`, `<list>`, `<vector>`, `<mutex>`, `<atomic>`

**下游使用方**：
- 当前无外部使用方，为独立工具模块

## 容易踩的坑

### 1. packCoords 方法不同

两个缓存类使用不同的坐标打包方法：

```cpp
// Long2IntLRUCache（OR 打包）
i64 key = (i64(x) << 32) | (i64(z) & 0xFFFFFFFFLL);

// OpenAddressingLRUCache（XOR 打包）
i64 key = (i64(x) << 32) ^ i64(z);
```

两种方法都能正确区分不同坐标，但生成的键值不同。**不要混用两个缓存的键**。

### 2. 淘汰行为差异

- `Long2IntLRUCache`：每次插入超容量时淘汰 **一个** 最旧条目
- `OpenAddressingLRUCache`：容量满时批量淘汰 **capacity/16** 个最旧条目

`OpenAddressingLRUCache` 在淘汰后可能暂时远低于最大容量。

### 3. 批量操作应使用 Locked 方法

两个类都是线程安全的（内部互斥锁），但批量操作时应使用 `Locked` 方法避免重复加锁：

```cpp
// 错误：每次操作都加锁，性能差
for (int i = 0; i < 1000; ++i) {
    cache.put(key, value);
}

// 正确：批量操作时只加一次锁
{
    std::lock_guard<std::mutex> lock(cache.getMutex());
    for (int i = 0; i < 1000; ++i) {
        cache.putLocked(key, value);
    }
}
```

### 4. 容量计算差异

- `Long2IntLRUCache`：容量就是 `maxSize`
- `OpenAddressingLRUCache`：实际哈希表容量为不小于 `maxSize * 1.5` 的最小 2 的幂次

```cpp
OpenAddressingLRUCache cache(100);
// 实际容量 = 256（因为 128 < 150 < 256）
```

### 5. 统计功能仅 OpenAddressingLRUCache 支持

`Long2IntLRUCache` 没有 `hitCount()`, `missCount()`, `resetStats()` 方法。
