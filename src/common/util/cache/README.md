# Cache 模块

LRU 缓存实现模块，提供高性能的坐标到值映射缓存。

## 目录结构

```
cache/
├── Long2FloatLRUCache.hpp  # Long→Float LRU 缓存（Biome 温度缓存用）
├── Long2IntLRUCache.hpp    # 链表+哈希表实现的 LRU 缓存（保留用于性能对比）
├── Long2IntLRUCache.cpp    # 对应实现文件
├── OpenAddressingLRUCache.hpp # 开放寻址 LRU 缓存（推荐用于生产环境）
├── OpenAddressingLRUCache.cpp # 对应实现文件
└── README.md                # 本文档
```

## 内部模块关系

```
┌─────────────────────────┐     ┌─────────────────────────┐     ┌─────────────────────────┐
│  Long2FloatLRUCache     │     │  Long2IntLRUCache       │     │  OpenAddressingLRUCache │
│  (头文件 Only)          │     │  (链表+哈希表实现)      │     │  (开放寻址实现)          │
│                         │     │                         │     │                         │
│  - FIFO 淘汰（与MC一致）│     │  - 标准 STL 实现       │     │  - 自定义数据结构       │
│  - NaN 默认返回值       │     │  - 单条淘汰             │     │  - 批量淘汰             │
│  - BlockPos 键打包      │     │  - 无统计功能           │     │  - 性能统计             │
│  - 用于 Biome 温度缓存  │     │                         │     │                         │
└─────────────────────────┘     └─────────────────────────┘     └─────────────────────────┘
            │                         │                         │
            └───────────┬─────────────┘                         │
                        ▼                                       ▼
            ┌─────────────────────┐                 ┌─────────────────────┐
            │  common/core/Types  │                 │  common/core/Types  │
            │  (基础类型定义)      │                 │  (基础类型定义)      │
            └─────────────────────┘                 └─────────────────────┘
```

## Long2FloatLRUCache

参考 MC 1.21.11 的 `Biome.TEMPERATURE_CACHE` 使用 `Long2FloatLinkedOpenHashMap`：
- 容量 1024，不进行 rehash
- 默认返回 NaN 表示缓存未命中
- FIFO 淘汰（与 MC 一致，先入先出）
- 提供 `packBlockPos(x, y, z)` 将 BlockPos 打包为 i64 键

| 特性 | Long2FloatLRUCache | Long2IntLRUCache | OpenAddressingLRUCache |
|------|-------------------|-----------------|----------------------|
| 值类型 | f32 | i32 | i32 |
| 淘汰策略 | FIFO | LRU | 批量 LRU |
| 默认返回 | NaN | 无 | 无 |
| 线程安全 | 否（需外部 thread_local） | 是（mutex） | 是（mutex） |
| BlockPos 键 | packBlockPos(x,y,z) | packCoords(x,z) | packCoords(x,z) |
| 实现方式 | 纯头文件 | .hpp + .cpp | .hpp + .cpp |
| 推荐场景 | Biome 温度缓存 | 性能对比测试 | 生产环境 |

## 容易踩的坑

### 1. Long2FloatLRUCache 线程安全

Long2FloatLRUCache 本身不线程安全，但通过 `thread_local` 使用（如 Biome::getTemperatureCache()），
每个线程拥有独立的缓存实例，无需加锁。

### 2. packBlockPos 与 packCoords 不同

Long2FloatLRUCache 使用 `packBlockPos(x, y, z)` 打包三维坐标，
而 Long2IntLRUCache 和 OpenAddressingLRUCache 使用 `packCoords(x, z)` 打包二维坐标。
**不要混用不同缓存的键**。

### 3. FIFO vs LRU

Long2FloatLRUCache 使用 FIFO 淘汰（与 MC 一致），先插入的条目先被淘汰，
即使它刚刚被访问过。这与 LRU（最近最少使用）不同。
