---
name: add-trace
description: 为用户指定的代码范围增加perfetto追踪宏
---

## 任务介绍

为用户指定的代码范围增加perfetto追踪宏插桩。下面是一些最佳实践：

```cpp
std::string ResourcePackList::normalizePath(const std::filesystem::path& path)
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::normalizePath");

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

    MC_TRACE_INSTANT("client.lighting",
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
    MC_TRACE_EVENT("client.lighting",
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
    MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries");

    // 注意，下面加trace的方式不推荐，建议进入相关函数，然后在函数内部顶部加trace，这样能避免外层函数全是trace，影响可读性。除非相关逻辑没有被封装为函数，时才可以在外层函数加trace。

    // 初始化方块注册表
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::Blocks");
        VanillaBlocks::initialize();
    }
    spdlog::info("Vanilla blocks initialized");

    // 初始化物品注册表
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::Items");
        Items::initialize();
    }
    spdlog::info("Vanilla items initialized");

    // 初始化附魔注册表
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::Enchantments");
        item::enchant::EnchantmentRegistry::initialize();
    }
    spdlog::info("Enchantments initialized");

    // 初始化方块物品注册表
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::BlockItems");
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
    spdlog::info("Block items initialized");

    // 初始化物品标签（必须在所有物品注册后）
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::ItemTags");
        item::tag::ItemTags::initialize();
    }
    spdlog::info("Item tags initialized");

    // 初始化发射器行为注册表
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::DispenseBehaviors");
        blocks::DispenseItemBehaviorRegistry::instance().initDefaultBehaviors();
    }
    spdlog::info("Dispense item behaviors initialized");
}

```

## 要点

1. 【必须】追踪宏的第一个参数是追踪类别，类别必须已经在`\src\common\perfetto\TraceCategories.hpp`中定义。尽可能复用这个文件中的定义，非必要不要增加定义。使用超出这个文件定义的类别会导致编译失败。
2. 【必须】追踪宏的第二个参数是事件名称，建议使用`类名::函数名`的格式。
3. 【可选】追踪宏的后续参数是任意数量的键值对，
4. 【可选】追踪宏的最后一个参数是一个lambda表达式，lambda表达式接受一个`perfetto::EventContext`参数，用户可以在lambda中使用这个参数来触发Flow事件。Flow事件可以用来关联不同函数中的事件，方便在perfetto UI中分析调用关系。如果你追踪的函数参数中有可以用来唯一标识调用实例的信息（比如实体ID、区块位置、方块Pos等），建议使用这些信息来构造Flow ID，这样可以在perfetto UI中清晰地看到同一个实体或区块的事件是如何流转的。
5. 追踪宏采用RAII机制，尽量放在当前作用域顶部，且离开当前作用域时会自动结束事件，因此不需要手动结束事件。
6. 尽量使用MC_TRACE_EVENT而不是MC_TRACE_INSTANT，因为前者可以记录一个时间区间，而后者只能记录一瞬间，导致信息量减少，除非你确实只关心一个瞬间事件（比如用户按下键盘、收到网络包），否则建议使用MC_TRACE_EVENT。
7. 引入trace之前，必须检查当前文件是否包含了 `#include "common/perfetto/TraceEvents.hpp"` 如果没有包含，则需要增加这个包含语句。
8. 建议进入相关函数，然后在函数内部顶部加trace，这样能避免外层函数全是trace，影响可读性。除非相关逻辑没有被封装为函数，时才可以在外层函数加trace。

## 任务开始前

开始修改代码之前，请先列出自己的修改计划，说明你打算在哪些文件的哪些函数中添加追踪宏。

## 任务结束后

代码编写结束之后，必须使用clang-format对你修改的文件进行格式化：

```
clang-format -i src\common\xxx\Foo.cpp
clang-format -i src\common\xxx\Foo.hpp
```

然后进行编译。
