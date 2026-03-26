# Cache 模块

LRU 缓存实现模块，提供高性能的坐标到值映射缓存。

## 目录结构

```
cache/
├── Long2IntLRUCache.hpp      # 基于链表+哈希表的 LRU 缓存头文件
├── Long2IntLRUCache.cpp      # 基于链表+哈希表的 LRU 缓存实现
├── OpenAddressingLRUCache.hpp # 基于开放寻址的 LRU 缓存头文件
├── OpenAddressingLRUCache.cpp # 基于开放寻址的 LRU 缓存实现
└── README.md                  # 本文档
```

## 文件详细介绍

### Long2IntLRUCache.hpp / Long2IntLRUCache.cpp

**职责**：基于链表+哈希表实现的 LRU 缓存，参考 MC 1.16.5 的 `Long2IntLinkedOpenHashMap` 设计思想。

**主要内容**：
- 使用 `std::unordered_map` + `std::list` 组合实现 O(1) 时间复杂度的 LRU 缓存
- 双向链表维护访问顺序（前端为最新，后端为最旧）
- 哈希表存储键到链表迭代器的映射
- 线程安全（内部互斥锁）

**核心方法**：
| 方法 | 说明 |
|------|------|
| `get(key, value)` | 获取缓存值，命中时更新访问顺序 |
| `put(key, value)` | 设置缓存值，已存在则更新，超出容量则淘汰最旧条目 |
| `packCoords(x, z)` | 将坐标打包为 64 位键（高位 x，低位 z，使用 OR） |
| `getLocked(key, value)` | 获取缓存值（调用方已持有锁） |
| `putLocked(key, value)` | 设置缓存值（调用方已持有锁） |
| `size()` | 获取当前缓存大小 |
| `clear()` | 清除缓存 |
| `getMutex()` | 获取互斥锁引用 |

**数据结构**：
```cpp
// 双向链表节点：存储 (key, value)
using ListNode = std::pair<i64, i32>;
std::list<ListNode> m_list;  // 前面是最新，后面是最旧

// 哈希表：key -> 链表迭代器
std::unordered_map<i64, std::list<ListNode>::iterator> m_cache;
```

**淘汰策略**：当缓存满时，移除链表尾部（最旧）的一个条目。

### OpenAddressingLRUCache.hpp / OpenAddressingLRUCache.cpp

**职责**：基于开放寻址哈希表实现的高性能 LRU 缓存，相比 `Long2IntLRUCache` 具有更好的缓存局部性。

**主要内容**：
- 使用开放寻址线性探测法实现哈希表
- 使用 FIB 哈希（黄金比例乘数）获得更均匀的分布
- 连续内存布局，无指针追踪开销
- 批量淘汰策略减少频繁操作开销
- 性能统计（命中次数、未命中次数）

**核心方法**：
| 方法 | 说明 |
|------|------|
| `get(key, value)` | 获取缓存值，命中时更新时间戳 |
| `put(key, value)` | 设置缓存值，超出容量则批量淘汰 |
| `packCoords(x, z)` | 将坐标打包为 64 位键（高位 x XOR 低位 z） |
| `getLocked(key, value)` | 获取缓存值（调用方已持有锁） |
| `putLocked(key, value)` | 设置缓存值（调用方已持有锁） |
| `hitCount()` | 获取命中次数 |
| `missCount()` | 获取未命中次数 |
| `resetStats()` | 重置统计计数器 |

**数据结构**：
```cpp
struct Entry {
    i64 key = 0;          // 坐标键
    i32 value = 0;        // 缓存值
    u32 timestamp = 0;    // 访问时间戳（用于 LRU）
    bool occupied = false; // 是否被占用
};

std::vector<Entry> m_table;  // 连续内存布局的哈希表
```

**关键设计**：
- **FIB 哈希**：使用黄金比例乘数 `11400714819323198485ULL`，比取模分布更均匀
- **容量计算**：自动计算不小于 `maxSize * 1.5` 的最小 2 的幂次，保证负载因子不超过 0.75
- **批量淘汰**：每次淘汰 `capacity / 16` 个最旧条目，使用 `std::nth_element` 选择算法

**淘汰策略**：当缓存满时，批量淘汰时间戳最小的 `capacity / 16` 个条目。

## 文件之间的关系

```
┌─────────────────────────────────────────────────────────────┐
│                      使用者代码                              │
└─────────────────────┬───────────────────────────────────────┘
                      │
          ┌───────────┴───────────┐
          ▼                       ▼
┌─────────────────────┐ ┌─────────────────────────┐
│  Long2IntLRUCache   │ │  OpenAddressingLRUCache │
│  (链表+哈希表实现)   │ │  (开放寻址实现)          │
│                     │ │                         │
│  - 标准 STL 实现    │ │  - 自定义数据结构       │
│  - 单条淘汰         │ │  - 批量淘汰             │
│  - 无统计功能       │ │  - 性能统计             │
└─────────────────────┘ └─────────────────────────┘
          │                       │
          └───────────┬───────────┘
                      ▼
          ┌─────────────────────┐
          │  common/core/Types  │
          │  (基础类型定义)      │
          └─────────────────────┘
```

**对比选择**：

| 特性 | Long2IntLRUCache | OpenAddressingLRUCache |
|------|-----------------|----------------------|
| 实现方式 | std::list + std::unordered_map | 开放寻址哈希表 |
| 内存布局 | 非连续（链表节点分散） | 连续（std::vector） |
| 缓存局部性 | 一般 | 优秀 |
| 淘汰策略 | 单条淘汰 | 批量淘汰 |
| 性能统计 | 无 | 有 |
| 推荐场景 | 性能对比测试 | 生产环境 |

## 模块整体说明

### 整体职责

提供高性能的 LRU 缓存实现，主要用于：
- 区块坐标到值的映射缓存
- 频繁访问数据的快速查找
- 内存受限场景下的自动淘汰

### 输入和输出

**输入**：
- 构造时：最大缓存容量 `maxSize`
- 运行时：键值对 `(i64 key, i32 value)`
- 坐标打包：`packCoords(i32 x, i32 z)` → `i64 key`

**输出**：
- 缓存查询结果 `get(key, value)` → `bool`（是否命中）
- 统计数据：`hitCount()`, `missCount()`, `size()`

### 依赖项

```cpp
// 外部依赖
#include <unordered_map>  // Long2IntLRUCache
#include <list>           // Long2IntLRUCache
#include <vector>         // OpenAddressingLRUCache
#include <mutex>          // 线程安全
#include <atomic>         // 原子操作（统计计数）

// 内部依赖
#include "../../core/Types.hpp"  // 基础类型 (i8, i16, i32, i64, u32, u64, f32)
```

### 使用方法

#### 基本使用

```cpp
#include "util/cache/OpenAddressingLRUCache.hpp"

// 创建缓存（容量 1024）
mc::OpenAddressingLRUCache cache(1024);

// 存储坐标值
cache.put(cache.packCoords(10, 20), 42);
cache.put(cache.packCoords(-5, 30), 100);

// 获取值
i32 value;
if (cache.get(cache.packCoords(10, 20), value)) {
    // 命中，value = 42
} else {
    // 未命中
}
```

#### 批量操作（避免重复加锁）

```cpp
mc::OpenAddressingLRUCache cache(1024);

// 批量操作时持有锁
std::lock_guard<std::mutex> lock(cache.getMutex());

for (int i = 0; i < 100; ++i) {
    i32 value;
    if (!cache.getLocked(cache.packCoords(i, i), value)) {
        cache.putLocked(cache.packCoords(i, i), i * 2);
    }
}
// 锁在作用域结束时释放
```

#### 性能统计

```cpp
mc::OpenAddressingLRUCache cache(1024);

// ... 使用缓存 ...

// 获取统计
std::cout << "Hits: " << cache.hitCount() << std::endl;
std::cout << "Misses: " << cache.missCount() << std::endl;

// 重置统计
cache.resetStats();
```

### 容易踩的坑

#### 1. packCoords 方法不同

两个缓存类使用不同的坐标打包方法：

```cpp
// Long2IntLRUCache（OR 打包）
i64 key = (i64(x) << 32) | (i64(z) & 0xFFFFFFFFLL);

// OpenAddressingLRUCache（XOR 打包）
i64 key = (i64(x) << 32) ^ i64(z);
```

**注意**：两种方法都能正确区分不同坐标，但生成的键值不同。不要混用两个缓存的键。

#### 2. 淘汰行为差异

- `Long2IntLRUCache`：每次插入超容量时淘汰 **一个** 最旧条目
- `OpenAddressingLRUCache`：容量满时批量淘汰 **capacity/16** 个最旧条目

这意味着 `OpenAddressingLRUCache` 在淘汰后可能暂时远低于最大容量。

#### 3. 线程安全

两个类都是线程安全的（内部互斥锁），但批量操作时应使用 `Locked` 方法避免重复加锁：

```cpp
// 错误：每次操作都加锁，性能差
for (int i = 0; i < 1000; ++i) {
    cache.put(key, value);  // 每次都加锁/解锁
}

// 正确：批量操作时只加一次锁
{
    std::lock_guard<std::mutex> lock(cache.getMutex());
    for (int i = 0; i < 1000; ++i) {
        cache.putLocked(key, value);  // 不加锁
    }
}
```

#### 4. 容量计算

- `Long2IntLRUCache`：容量就是 `maxSize`
- `OpenAddressingLRUCache`：实际哈希表容量为不小于 `maxSize * 1.5` 的最小 2 的幂次

```cpp
OpenAddressingLRUCache cache(100);
// 实际容量 = 256（因为 128 < 150 < 256）
```

#### 5. 统计功能仅 OpenAddressingLRUCache 支持

`Long2IntLRUCache` 没有 `hitCount()`, `missCount()`, `resetStats()` 方法。

## 涉及的测试用例

测试文件位置：`tests/common/util/cache/CacheBenchmark.cpp`

### Long2IntLRUCache 测试

| 测试名称 | 说明 |
|---------|------|
| `BasicOperations` | 基本 put/get 操作 |
| `PackCoords` | 坐标打包正确性 |
| `LRUEviction` | LRU 淘汰策略验证 |
| `UpdateValue` | 更新已存在键的值 |

### OpenAddressingLRUCache 测试

| 测试名称 | 说明 |
|---------|------|
| `BasicOperations` | 基本 put/get 操作 |
| `PackCoords` | 坐标打包正确性 |
| `LRUEviction` | LRU 批量淘汰策略验证 |
| `UpdateValue` | 更新已存在键的值 |
| `LockedOperations` | 批量操作（持有锁） |
| `Statistics` | 命中/未命中统计功能 |

### 性能对比测试

| 测试名称 | 说明 |
|---------|------|
| `Long2IntLRUCache_WritePerformance` | 写入性能测试 |
| `OpenAddressingLRUCache_WritePerformance` | 写入性能测试 |
| `Long2IntLRUCache_ReadPerformance` | 读取性能测试 |
| `OpenAddressingLRUCache_ReadPerformance` | 读取性能测试 |
| `OpenAddressingLRUCache_BatchPerformance` | 批量操作性能测试 |

### 一致性测试

| 测试名称 | 说明 |
|---------|------|
| `SameCoordinates_SameValue` | 两个缓存相同坐标产生相同结果 |
| `ClearAndRefill` | 清空后重新填充的一致性 |

## 性能建议

1. **生产环境推荐 `OpenAddressingLRUCache`**：更好的缓存局部性和批量淘汰效率
2. **批量操作使用 `Locked` 方法**：避免重复加锁开销
3. **合理设置容量**：根据实际内存限制和使用模式设置 `maxSize`
4. **利用统计功能**：通过 `hitCount()` / `missCount()` 监控缓存效率

## 参考

- Minecraft Java 1.16.5 `Long2IntLinkedOpenHashMap` 设计思想
- FIB 哈希算法（黄金比例哈希）
- 开放寻址哈希表实现
