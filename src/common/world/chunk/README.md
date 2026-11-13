# chunk 模块

## 目录结构树

```text
chunk/
├── base/  # 本领域共享的基础类型与定义
├── data/  # 该领域的子模块
└── IChunkManager.hpp  # 区块管理器接口
```

## 内部模块关系

base 提供坐标和标识，data 提供共享区块模型，IChunkManager 为公共游戏逻辑提供访问契约。服务端的票据、阶段与生命周期位于 server/world/chunk。

## 上下游外部依赖关系

公共层提供领域类型与接口；服务端通过应用装配连接玩家、网络和世界实现。源文件的构建归属与领域目录一致。

## 容易踩的坑

- 目录按领域职责组织；跨领域协作通过明确入口和基础类型。
- 新增或迁移源文件时同步更新所有消费目标的 CMake 清单。

1. **`ChunkDemandController::tick()` 必须每 tick 调用一次** — 带生命周期票据（PORTAL 300t、POST_TELEPORT 5t、UNLOAD_COOLDOWN 100t 等）的剩余 tick 只在这里递减；漏调会让它们永不失效，其钉住的票级也不会释放（历史上 `tick()` 无调用方，导致票据过期机制整体失效）。显式 ticket、玩家 source 和追踪系统的更新仍是批处理的，需要在添加/移除后调用 `processUpdates()`
2. **线程安全** — `ChunkHolder` 使用互斥锁；`ChunkData` 和 `ChunkPrimer` 不是线程安全的
3. **区块段懒创建** — 设置空气方块不会创建区块段；写入非空气方块才会创建
4. **光照初始化是惰性的** — 天空光照默认 15（全亮），方块光照默认 0（无光），两者都**不预分配**底层 2048 字节缓冲：**数组为空即代表默认值**（`getSkyLight` 对空数组返回 15、`getBlockLight` 返回 0），只有真正写入非默认值时才按需分配（`NibbleArray::set` 与可变 `data()` 都会 `ensureAllocated`）。空数组与「全 15」在格式上等价，原生存档不保存光照，加载后由光照任务重算。改动此处语义时必须同时核对这两处。**光照的权威副本只有 `ChunkData` 的 `SWMRNibbleArray` 一处**（`ChunkSection` 不持有光照）
5. **level 语义** — 级别越小优先级越高；级别 ≤ 34（`ChunkLoadLevel::Border`）的区块应该被加载，≤ 32（BlockTicking）的区块才推进游戏逻辑
6. **ChunkStatus 比较** — 使用 `isAtLeast()` 和 `isBefore()`，不要直接比较 ordinal
7. **高度图内部存储** — `Heightmap` 内部存储 `y + 1`，不是实际方块 Y。`getTopBlockY` 返回方块本身 Y（内部值 -1），但**空列回退为 `MIN_BUILD_HEIGHT`**（与"minY 处有方块"无法区分）；需精确识别空列的调用方（如 `HeightmapPlacement`）改用 `getHeightmapFirstAvailable` 拿原始值（`y+1` 或 `NO_BLOCK_SENTINEL`），对齐 MC `Heightmap.getFirstAvailable`
8. **高度图底层是位压缩存储，`getData()` 按值返回** — `Heightmap` 用 9 bit/列的紧凑位存储（256 列 37 个 u64 = 296 B，7 张合计 2072 B/区块），位宽与布局对齐原版 `SimpleBitStorage`。因此 `Heightmap::getData()` 与 `ChunkData::getHeightmapData()` 都**按值返回** `std::array<BlockCoord, 256>`，**不要用 `const auto&` 绑定**——临时数组会立即析构，后续读到栈上垃圾。位存储内部每列存 `raw - NO_BLOCK_SENTINEL`（哨兵编码为 0，故零初始化天然等价于"全部无方块"），越界输入夹到 `[NO_BLOCK_SENTINEL, MAX_BUILD_HEIGHT]` 而非回绕成另一个合法高度
9. **WorldGenRegion 阶段校验** — 生成期区块访问按当前 `ChunkStep::directDependencies()` 校验距离对应的允许状态；调用点需要传递实际请求的 `ChunkStatus`
10. **ChunkStatus 根状态自引用 parent** — `ChunkStatus` 构造函数将 `nullptr` parent 转为 `this`，因此 `EMPTY.parent() == &EMPTY`（非 nullptr）。遍历 parent 链时必须用 `*status != EMPTY` 而非 `status != nullptr` 作为终止条件（见 `ChunkStep::buildRequiredStatusByRadius`）
11. **getRadiusOf 返回最后覆盖半径** — `ChunkDependencies::getRadiusOf(status)` 返回覆盖该 status ordinal 的**最大**半径 j（前向循环覆盖），不是最小半径。例如 FULL 的累积依赖中 `getRadiusOf(STRUCTURE_STARTS) = 11`（外圈），而非 4（首次出现）。`byRadius[]` 构建依赖此语义：高状态先填小半径，低状态填剩余空隙，最终 `byRadius[r] == accumulatedDependencies.get(r)`
12. **byRadius[] 查找表** — `ChunkStep::getRequiredStatusAtRadius(radius)` 返回生成 `targetStatus` 时距离中心 `radius` 的邻居必须达到的状态。`byRadius[0] = accumulatedDependencies.get(0)`（中心区块前一步状态），表在 `ChunkPyramid` 构建时由 `buildRequiredStatusByRadius` 填充。与 Moonrise `ChunkStepMixin.moonrise$getRequiredStatusAtRadius` 对齐
13. **非空气方块计数（`ChunkSection::m_blockCount`）是"数据完整性"字段，不是性能计数器** — 它被 `isEmpty()` 直接消费，原生存档始终保留实际紧凑数组，其他序列化路径仍可能依赖 `isEmpty()` 跳过数据：计数为 0 而调色板里实际有方块时，整段地形会在"已保存"的假象下被清空、读回全是空气且毫无报错。因此：
    - 写入方块状态会自动维护该计数（`_updateCounters`，判据 `BlockState::isAir()`，**注意状态 1/2 是洞穴空气/虚空空气，按 `stateId != 0` 判断会把它们误计为非空气**）；
    - `setBlockCount()` 是"绕开调色板直接覆盖"的后门，**必须在写完方块状态之后调用**，否则会被 `setBlockStateIdFast` 的自动累加再叠加一次，得到两倍值（历史上 `VanillaChunkWire` 客户端收包路径就是先赋计数后写状态）；
    - 从外部数据（存档字节、网络包）导入段时**不要原样采信外部计数**：`ChunkSection::deserialize` 已改为按实际方块数据重算并对外部值不符的情况告警，原生存档直接恢复段计数和经过边界验证的紧凑数组。

14. **`ChunkData` 的移动构造/移动赋值必须逐个成员搬运** — 两者都是手写的初始化列表/赋值序列，**新增成员时极易漏搬**：漏掉的成员在移动后静默变回默认值，源对象也不会被清空。`m_loadedEntityNbt` 就曾因此一直为空（`JavaColumnReader` 明明填了实体 NBT，`ChunkData` 一 move 就丢），而方块实体 `m_blockEntities` 因为被搬了所以毫无异常，问题极难定位。新增成员后务必同时检查 move ctor、move assignment 与 `operator=`
