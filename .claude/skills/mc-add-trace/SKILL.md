---
name: mc-add-trace
description: 为用户指定的代码范围增加性能追踪宏（Perfetto + Tracy 双轨）
---

## 任务介绍

为用户指定的代码范围增加性能追踪宏插桩。项目采用 Perfetto + Tracy 双轨：`MC_TRACE_*` 宏会同时向两套后端发事件（除非编译时关闭其一），对外 API 不区分后端。下面是一些最佳实践：

```cpp
std::string ResourcePackList::normalizePath(const std::filesystem::path& path)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourcePackList::normalizePath");

    std::string result = path.string();
    // 统一使用正斜杠
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

void NetworkClient::handleBlockUpdate(network::PacketDeserializer& deser)
{
    auto result = network::BlockUpdatePacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("[NetworkClient::handleBlockUpdate] Failed to deserialize block update packet");
        return;
    }

    auto& packet = result.value();

    MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Lighting,
        "ReceiveBlockUpdate",
        "pos",
        fmt::format("({}, {}, {})", packet.x(), packet.y(), packet.z()),
        "stateId",
        packet.blockStateId(),
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(packet.x(), packet.y(), packet.z()).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    if (m_callbacks.onBlockUpdate) {
        m_callbacks.onBlockUpdate(packet.x(), packet.y(), packet.z(), packet.blockStateId());
    }
}

void ClientWorld::onLightUpdate(i32 chunkX,
    i32 chunkZ,
    i32 sectionY,
    const std::vector<u8>& skyLight,
    const std::vector<u8>& blockLight,
    bool /*trustEdges*/
)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Lighting,
        "ClientWorld::onLightUpdate",
        "Section",
        fmt::format("({}, {}, {})", chunkX, sectionY, chunkZ),
        "SkyLightSize",
        skyLight.size(),
        "BlockLightSize",
        blockLight.size(),
        [flow = ::perfetto::Flow::ProcessScoped(SectionPos(chunkX, sectionY, chunkZ).toLong())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    const ChunkId id(chunkX, chunkZ);
    ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return;
    }

    ChunkSection* section = chunk->data->getSection(sectionY);
    if (!section) {
        return;
    }

    if (!skyLight.empty() && skyLight.size() == NibbleArray::BYTE_SIZE) {
        section->skyLightNibble() = NibbleArray(skyLight);
    }

    if (!blockLight.empty() && blockLight.size() == NibbleArray::BYTE_SIZE) {
        section->blockLightNibble() = NibbleArray(blockLight);
    }

    requestChunkMeshRebuild(id);
}

void MinecraftServer::initializeRegistries(bool registerEntities)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries");

    // 注意，下面加trace的方式不推荐，建议进入相关函数，然后在函数内部顶部加trace，这样能避免外层函数全是trace，影响可读性。除非相关逻辑没有被封装为函数，时才可以在外层函数加trace。

    // 初始化方块注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Blocks");
        VanillaBlocks::initialize();
    }
    spdlog::info("Vanilla blocks initialized");

    // 初始化物品注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Items");
        Items::initialize();
    }
    spdlog::info("Vanilla items initialized");

    // 初始化附魔注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Enchantments");
        item::enchant::EnchantmentRegistry::initialize();
    }
    spdlog::info("Enchantments initialized");

    // 初始化方块物品注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::BlockItems");
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
    spdlog::info("Block items initialized");

    // 初始化物品标签（必须在所有物品注册后）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::ItemTags");
        item::tag::ItemTags::initialize();
    }
    spdlog::info("Item tags initialized");

    // 初始化发射器行为注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::DispenseBehaviors");
        blocks::DispenseItemBehaviorRegistry::instance().initDefaultBehaviors();
    }
    spdlog::info("Dispense item behaviors initialized");
}

```

## 要点

1. 【必须】追踪宏的第一个参数是追踪类别，必须取自 `mc::trace::TraceEvents` 枚举树（定义在 `src\common\profiler\TraceCategories.hpp`）。该枚举树按子系统组织成嵌套结构体，叶子节点形如 `TraceEvents.Server.Tick`、`TraceEvents.Rendering.Frame`、`TraceEvents.World.ChunkGen`。尽可能复用已有的叶子节点，非必要不要新增；新增时须同时在枚举树和 `PERFETTO_DEFINE_CATEGORIES` 中登记（仅 Perfetto 后端需要注册，Tracy 不消费 category）。使用枚举树之外的类别会导致编译失败（仅 Perfetto 启用时）。
2. 【必须】追踪宏的第二个参数是事件名称，建议使用`类名::函数名`的格式。
3. 【可选】追踪宏的后续参数是任意数量的键值对，
4. 【可选】追踪宏的最后一个参数是一个lambda表达式，lambda表达式接受一个`perfetto::EventContext`参数，用户可以在lambda中使用这个参数来触发Flow事件。Flow事件可以用来关联不同函数中的事件，方便在perfetto UI中分析调用关系。如果你追踪的函数参数中有可以用来唯一标识调用实例的信息（比如实体ID、区块位置、方块Pos等），建议使用这些信息来构造Flow ID，这样可以在perfetto UI中清晰地看到同一个实体或区块的事件是如何流转的。
5. 追踪宏采用RAII机制，尽量放在当前作用域顶部，且离开当前作用域时会自动结束事件，因此不需要手动结束事件。
6. 尽量使用 `MC_TRACE_SCOPED_EVENT` 而不是 `MC_TRACE_INSTANT_EVENT`，因为前者可以记录一个时间区间，而后者只能记录一瞬间，导致信息量减少，除非你确实只关心一个瞬间事件（比如用户按下键盘、收到网络包），否则建议使用 `MC_TRACE_SCOPED_EVENT`。
7. 引入trace之前，必须检查当前文件是否包含了 `#include "common/profiler/TraceEvents.hpp"` 如果没有包含，则需要增加这个包含语句。
8. 建议进入相关函数，然后在函数内部顶部加trace，这样能避免外层函数全是trace，影响可读性。除非相关逻辑没有被封装为函数，时才可以在外层函数加trace。
9. 【命名空间】`.cpp` 文件通常在 include 区之后加一行 `using namespace mc::trace;`，即可直接写 `TraceEvents.Server.Tick`；`.hpp` 文件用全限定 `::mc::trace::TraceEvents.X.Y`，不要在头文件加 `using namespace`。
10. 【内存追踪】分配级追踪由独立 CMake 开关 `MC_ENABLE_MEMORY` 控制（默认 OFF，须同时 `MC_ENABLE_TRACY=ON`）。**不要**直接用 `MC_TRACE_MEM_ALLOC/FREE` 宏去标 `std::vector::data()` 或 `shared_ptr::get()` 这类不稳定指针——会触发 Tracy 硬失败终止会话（详见下方「内存追踪」章节）。追踪 vector 用 `TracyTrackingAlloc`，追踪对象用 `TracyObjectTracker`，两者都在 `common/profiler/MemoryTracking.hpp`。

## 可用宏速查

- `MC_TRACE_SCOPED_EVENT(category, name, ...)` — 作用域事件（最常用，RAII 自动结束）
- `MC_TRACE_INSTANT_EVENT(category, name, ...)` — 瞬时事件（零持续时间）
- `MC_TRACE_COUNTER(category, name, value)` — 计数器（value 须为 int64_t）
- `MC_TRACE_EVENT_BEGIN(category, name, ...)` / `MC_TRACE_EVENT_END(category)` — 手动跨函数配对
- `MC_TRACE_SET_THREAD_NAME(name)` — 线程命名
- `MC_TRACE_MEM_ALLOC(name, ptr, size)` — 内存分配事件（分配级追踪，按 name 分组）
- `MC_TRACE_MEM_FREE(name, ptr)` — 内存释放事件

## 内存追踪

除上面的 CPU zone 事件外，项目还提供**分配级内存追踪**，由独立的 CMake 开关 `MC_ENABLE_MEMORY` 控制（默认 OFF，与 Tracy 正交）。它记录每次分配/释放的 (指针, 字节数, 调用栈)，在 Tracy UI 的内存视图中按 `name` 分组显示池的增长曲线与热点。仅 Tracy 后端提供实现，故须同时 `MC_ENABLE_TRACY=ON`。这与 `MemoryTraceThread`（后台线程 100Hz 采样进程整体 RSS）互补：前者是分配级精细追踪，后者是粗粒度整体采样。

### Tracy 硬不变量（违反即终止会话，无 flag 可关闭）

`TracyAllocN(ptr, size, name)` 要求 `(name, ptr)` 当前**不在**活跃集中；`TracyFreeN(ptr, name)` 要求 `(name, ptr)` 当前**在**活跃集中。

- 违反前者 → `Failure::MemAllocTwice`（"already tracked and not freed"）→ **终止整个 profiling 会话**
- 违反后者 → `Failure::MemFree`（"free without matching allocation"）→ **终止整个 profiling 会话**

**两者都是硬失败，`TRACY_IGNORE_MEMORY_FAULTS` 只能压 `MemFree`、压不了 `MemAllocTwice`，没有任何 flag 可关闭。** 源码见 `third_party/tracy/server/TracyWorker.cpp` 的 `ProcessMemAllocImpl` / `ProcessMemFreeImpl`。

因此 Tracy 的约束是：**对每个 `(name, ptr)` 对，alloc 与 free 必须严格一对一，且 `ptr` 必须是一次真实堆分配的返回值、在其被真实释放前不再被 alloc。**

### 反模式：把宏绑到 `std::vector::data()` / `shared_ptr::get()`

这会必然违反上面的不变量，因为这类指针不稳定、且其生命周期不对应一次成对的 malloc/free：

- `vector::reserve()` 不 realloc 时返回**同一个** `data()` → 再次 `TracyAllocN` 同指针 → `MemAllocTwice`
- `vector::clear()` **不释放、不改 `data()`** → `TracyFreeN` 标了一个仍存活的指针，后续复用即错配 → `MemFree` / `MemAllocTwice`
- `vector` realloc 时旧指针被静默释放，Tracy 不知情 → 旧指针泄漏在活跃集，堆复用该地址时 → `MemAllocTwice`
- `shared_ptr::get()` 析构时机不确定，只标 alloc 不标 free → 地址被堆回收复用后 → `MemAllocTwice`

> 历史教训：8f43905d5 曾用 `MC_TRACE_MEM_ALLOC/FREE` 直接标 `MeshData::vertices.data()`（在 `reserve`/`clear` 里）、`ItemTextureAtlas::m_pixels.data()`、`make_unique<ChunkData>` 调用点，三者全部触发上述失败，导致 `ChunkMeshWorker` 线程上一跑 Tracy 就被终止。已改用下方的安全工具修复。

### 正确做法：用 `MemoryTracking.hpp` 的两个安全工具

`common/profiler/MemoryTracking.hpp` 提供两个**自动维持不变量**的工具，优先使用它们；手动 `MC_TRACE_MEM_ALLOC/FREE` 宏仅留给无法套用这两种模式的极少数场景（且须极其小心配对）。

**方案 A — `TracyTrackingAlloc<T, kName>`（追踪 `std::vector` 的内部缓冲区）**

有状态分配器，截获 vector 每次 `allocate`/`deallocate`（含 realloc 的成对 free+alloc），从根上保证一对一。把 vector 的分配器换成它即可：

```cpp
#include "common/profiler/MemoryTracking.hpp"

// 在宿主类型所在命名空间起一个语义别名（name 进类型系统，不同 name 是不同类型）
template <typename T>
using ChunkMeshAlloc = ::mc::profiler::TracyTrackingAlloc<T, "ChunkMesh">;

struct MeshData {
    std::vector<Vertex, ChunkMeshAlloc<Vertex>> vertices;  // 改这里
    std::vector<u32,    ChunkMeshAlloc<u32>>    indices;
};
```

注意点：
1. 换了分配器后 vector 是**新类型**，与普通 `std::vector<T>` 不兼容——不能互相赋值。从普通 vector 拷贝要用 `assign(begin, end)`；swap 临时对象释放容量时临时对象也必须是同分配器类型：`std::vector<Vertex, ChunkMeshAlloc<Vertex>>().swap(v);`
2. `name` 是 C++20 fixed-string NTTP（字符串字面量直接作模板参数）。不同 `name` 是不同分配器类型，容器不可互换——这符合预期（不同内存池本不应混用）。
3. 仅 `MC_ENABLE_MEMORY && MC_ENABLE_TRACY` 时发事件，其余分支透传 `std::allocator` 零开销。
4. 适用：任何想追踪内部缓冲区生命周期的 `std::vector`（如 `MeshData::vertices/indices`、`ItemTextureAtlas::m_pixels`）。

**方案 B — `TracyObjectTracker<kName>`（追踪对象本身的驻留）**

RAII 成员守卫，绑定宿主对象地址，ctor 发 alloc、dtor 发 free。`this` 在对象生命周期内稳定，天然满足一对一。比在 `make_unique` 调用点插 alloc 更稳健——调用点无法捕获析构，且覆盖**所有**构造路径。

```cpp
#include "common/profiler/MemoryTracking.hpp"

class ChunkData {
private:
    ::mc::profiler::TracyObjectTracker<"ChunkCache"> m_memTrack;
public:
    ChunkData()        : m_memTrack(this) {}            // 普通构造：bind this
    ChunkData(ChunkData&& o) noexcept { /* 见下 */ }    // move：重绑定
    ~ChunkData() = default;                              // dtor 自动 free
};
```

**move 语义（关键，必须正确处理）**：宿主 move 时地址变了，旧地址的内存会被回收、可能被堆复用，若旧地址仍留在活跃集 → `MemAllocTwice`。故 move 必须「释放旧地址 + 分配新地址」。守卫**不可移动**（禁拷贝禁移动），由宿主显式重绑定：

```cpp
ChunkData::ChunkData(ChunkData&& other) noexcept
    /* m_memTrack 不进初始化列表 → 默认构造为非活跃 */
{
    // ...逐成员 move...
    other.m_memTrack.unbind();   // 释放源旧地址
    m_memTrack.bind(this);       // 分配目标新地址
}

ChunkData& ChunkData::operator=(ChunkData&& other) noexcept {
    if (this != &other) {
        // ...逐成员 move...
        m_memTrack.unbind();         // 释放目标当前地址
        other.m_memTrack.unbind();   // 释放源旧地址
        m_memTrack.bind(this);       // 分配目标新地址
    }
    return *this;
}
```

注意点：
1. `alloc` 的 size **必须传 `sizeof(宿主类型)`**，不能传 0。Tracy 的内存曲线按「活跃分配 size 之和」绘制（`TracyWorker.cpp` 的 `ProcessMemAllocImpl` 里 `memdata.usage += size`），传 0 则曲线全程为 0——事件照发、但不可见，表现为「池全程是 0」。守卫的 ctor/bind 是模板方法，从传入的 `this`（`T*`）推导 `sizeof(T)`，宿主无需手算。`sizeof(T)` 只反映外层结构体、不含内部 vector 的堆分配，绝对值偏低但能正确反映对象驻留波动（加载增长、卸载回落）。
2. 守卫只存一个 `void*` + 一个 `bool`，不访问宿主其余成员，成员位置无强制要求。
3. 若宿主类**不可移动**（或 move 后无所谓地址），则无需重绑定，普通 ctor 里 `m_memTrack(this)` 即可，dtor 自动 free。
4. 适用：任何想追踪「对象驻留」的堆对象（如 `ChunkData`）。shared_ptr 多持有者的析构时机问题由「守卫随对象走」天然解决——对象只构造/析构一次，与 shared_ptr 计数无关。

### 追踪 CPU 暂存缓冲区要当心「全程为 0」

如果一个 `vector` 是**一次性 CPU 暂存缓冲区**——init 时分配、用完立刻 `clear()+shrink_to_fit()` 释放、之后长期为空（典型：纹理上传前的 `m_pixels`，GPU 上传后即释放 CPU 副本）——那么用 `TracyTrackingAlloc` 追踪它**只会看到 init 期一个尖峰**，稳态下曲线回到 0。这是正确行为（缓冲区确实已释放），不是 bug。

如果用户报告「某池全程是 0」，先排除两种可能：
- **size=0**（方案 B 守卫传 0，见上）——曲线恒 0，无尖峰。修法：传 `sizeof(host)`。
- **稳态确为 0**（暂存缓冲区已释放）——只有 init 尖峰，连到 GUI 时若已过且事件队列已环绕，可能连尖峰都看不到。这不是追踪的错，是被追踪对象本就是瞬态的。

想看「纹理驻留」这类**GPU 内存**，`TracyTrackingAlloc` 帮不上——它只截获 CPU 堆分配，GPU 的 `vkAllocateMemory`/`vkFreeMemory` 不经过 CPU 分配器。可用手动宏标**稳定的句柄成员地址**（如 `&m_imageMemory`，宿主对象存活期间稳定，且 vkAlloc/Free 严格一对一），这是手动宏少数安全场景之一：

```cpp
// _createImage 成功后：
MC_TRACE_MEM_ALLOC("TextureAtlas-GPU", &m_imageMemory, memoryRequirements.size);
// destroy 中 vkFreeMemory 之前：
MC_TRACE_MEM_FREE("TextureAtlas-GPU", &m_imageMemory);
```

注意：标的是 `&m_imageMemory`（存储句柄的成员地址，稳定），**不是** `m_imageMemory`（句柄值本身，是 GPU 侧不透明 ID，非堆指针）。

### 手动宏（`MC_TRACE_MEM_ALLOC/FREE`）——仅限极少数场景

```cpp
MC_TRACE_MEM_ALLOC("PoolName", ptr, size);  // 在 name 池中标记 ptr 处分配了 size 字节
MC_TRACE_MEM_FREE("PoolName", ptr);         // 在 name 池中标记 ptr 处释放
```

仅当追踪对象既不是 `std::vector`、也无法套用对象守卫（如纯 C 风式 malloc/free、第三方库返回的裸指针且你能精确捕获其 free）时才用。**绝对不要**用来标 `vector::data()`、`shared_ptr::get()`、`reserve`/`clear`/`resize` 调用点。同样需要 `#include "common/profiler/TraceEvents.hpp"`。

## 任务开始前

开始修改代码之前，请先列出自己的修改计划，说明你打算在哪些文件的哪些函数中添加追踪宏，以及每个事件用哪个枚举树叶节点。

## 任务结束后

代码编写结束之后，必须使用clang-format对你修改的文件进行格式化：

```
clang-format -i src\common\xxx\Foo.cpp
clang-format -i src\common\xxx\Foo.hpp
```

然后进行编译。
