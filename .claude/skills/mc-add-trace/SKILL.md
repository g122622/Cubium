---
name: mc-add-trace
description: 为用户指定的代码范围增加perfetto追踪宏
---

## 任务介绍

为用户指定的代码范围增加perfetto追踪宏插桩。下面是一些最佳实践：

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

1. 【必须】追踪宏的第一个参数是追踪类别，必须取自 `mc::trace::TraceEvents` 枚举树（定义在 `\src\common\perfetto\TraceCategories.hpp`）。该枚举树按子系统组织成嵌套结构体，叶子节点形如 `TraceEvents.Server.Tick`、`TraceEvents.Rendering.Frame`、`TraceEvents.World.ChunkGen`。尽可能复用已有的叶子节点，非必要不要新增；新增时须同时在枚举树和 `PERFETTO_DEFINE_CATEGORIES` 中登记。使用枚举树之外的类别会导致编译失败。
2. 【必须】追踪宏的第二个参数是事件名称，建议使用`类名::函数名`的格式。
3. 【可选】追踪宏的后续参数是任意数量的键值对，
4. 【可选】追踪宏的最后一个参数是一个lambda表达式，lambda表达式接受一个`perfetto::EventContext`参数，用户可以在lambda中使用这个参数来触发Flow事件。Flow事件可以用来关联不同函数中的事件，方便在perfetto UI中分析调用关系。如果你追踪的函数参数中有可以用来唯一标识调用实例的信息（比如实体ID、区块位置、方块Pos等），建议使用这些信息来构造Flow ID，这样可以在perfetto UI中清晰地看到同一个实体或区块的事件是如何流转的。
5. 追踪宏采用RAII机制，尽量放在当前作用域顶部，且离开当前作用域时会自动结束事件，因此不需要手动结束事件。
6. 尽量使用 `MC_TRACE_SCOPED_EVENT` 而不是 `MC_TRACE_INSTANT_EVENT`，因为前者可以记录一个时间区间，而后者只能记录一瞬间，导致信息量减少，除非你确实只关心一个瞬间事件（比如用户按下键盘、收到网络包），否则建议使用 `MC_TRACE_SCOPED_EVENT`。
7. 引入trace之前，必须检查当前文件是否包含了 `#include "common/perfetto/TraceEvents.hpp"` 如果没有包含，则需要增加这个包含语句。
8. 建议进入相关函数，然后在函数内部顶部加trace，这样能避免外层函数全是trace，影响可读性。除非相关逻辑没有被封装为函数，时才可以在外层函数加trace。
9. 【命名空间】`.cpp` 文件通常在 include 区之后加一行 `using namespace mc::trace;`，即可直接写 `TraceEvents.Server.Tick`；`.hpp` 文件用全限定 `::mc::trace::TraceEvents.X.Y`，不要在头文件加 `using namespace`。

## 可用宏速查

- `MC_TRACE_SCOPED_EVENT(category, name, ...)` — 作用域事件（最常用，RAII 自动结束）
- `MC_TRACE_INSTANT_EVENT(category, name, ...)` — 瞬时事件（零持续时间）
- `MC_TRACE_COUNTER(category, name, value)` — 计数器（value 须为 int64_t）
- `MC_TRACE_EVENT_BEGIN(category, name, ...)` / `MC_TRACE_EVENT_END(category)` — 手动跨函数配对
- `MC_TRACE_SET_THREAD_NAME(name)` — 线程命名

## 任务开始前

开始修改代码之前，请先列出自己的修改计划，说明你打算在哪些文件的哪些函数中添加追踪宏，以及每个事件用哪个枚举树叶节点。

## 任务结束后

代码编写结束之后，必须使用clang-format对你修改的文件进行格式化：

```
clang-format -i src\common\xxx\Foo.cpp
clang-format -i src\common\xxx\Foo.hpp
```

然后进行编译。
