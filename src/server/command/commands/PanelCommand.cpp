/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "PanelCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "server/application/IServer.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/application/console/ServerConsole.hpp"
#include "server/application/console/render/PanelStats.hpp"
#include "server/application/console/render/PanelStatsCollector.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mc::command {

namespace {

using server::console::PanelDetailStore;
namespace panel = server::console;
using server::MinecraftServer;
using server::ServerWorld;

/// `/panel places` 允许一次抽取的最大数量（防止一次打印刷屏）。
constexpr i32 MAX_PLACES = 32;

/// 方块实体 NBT 在单行里展示的最大字符数。
constexpr usize MAX_NBT_LENGTH = 200;

/// 没有交互式终端时给用户的提示：面板只存在于 `--interactive` 模式。
constexpr const char* NO_PANEL_HINT = "No interactive console is attached; start the server with --interactive";

/**
 * @brief 取出控制台面板的自定义行存储
 *
 * 面板由 `ServerConsole` 持有；未启用 `--interactive` 时返回 nullptr（命令仍可执行，
 * 只是结果以普通回执的形式出现在日志里）。
 */
PanelDetailStore* _panelStore(ServerCommandSource& source)
{
    auto* server = dynamic_cast<server::MinecraftServer*>(source.server());
    if (server == nullptr) {
        return nullptr;
    }
    auto* console = server->serverConsole();
    if (console == nullptr) {
        return nullptr;
    }
    return &console->panelDetails();
}

/**
 * @brief 读取指定坐标的区块；区块未加载时返回 nullptr
 */
ChunkData* _requireChunk(ServerWorld& world, const Vector3i& position)
{
    auto* chunkManager = world.chunkManager();
    if (chunkManager == nullptr) {
        return nullptr;
    }
    return chunkManager->tryToGetChunkInMem(
        static_cast<ChunkCoord>(position.x >> 4), static_cast<ChunkCoord>(position.z >> 4));
}

/// 运行时群系查询（不依赖区块是否加载，对齐 vanilla Level.getBiome）
[[nodiscard]] std::string _runtimeBiomeName(ServerWorld& world, i32 x, i32 y, i32 z)
{
    auto* worldChunkManager = world.chunkManager();
    if (worldChunkManager == nullptr || worldChunkManager->generator() == nullptr) {
        return "unknown";
    }

    // 噪声生成器暴露 Voronoi 抖动缩放后的群系查询；其它生成器回落到生成器自身的
    // getBiome（flat 等生成器的实现即直接返回其固定群系）。
    BiomeId id = world::biome::Biomes::Plains;
    if (const auto* noiseGenerator = dynamic_cast<const NoiseChunkGenerator*>(worldChunkManager->generator());
        noiseGenerator != nullptr) {
        id = noiseGenerator->getBiomeAtWorldPosition(x, y, z);
    } else {
        id = worldChunkManager->generator()->getBiome(x, y, z);
    }
    return panel::stripNamespace(world::biome::BiomeRegistry::instance().get(id).name());
}

/// 随机数种子：命令是交互驱动的，用时钟即可（与 /spreadplayers 等保持一致）。
[[nodiscard]] u64 _randomSeed()
{
    return static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count());
}

} // namespace

void PanelCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto root = std::make_shared<LiteralCommandNode<ServerCommandSource>>("panel");
    root->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(root,
        support::makeMetadata("Refresh the interactive terminal's status panel and inspect world data.",
            "/panel [refresh|block|places|biome]",
            2));
    root->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _refresh(ctx);
    });

    auto refresh = std::make_shared<LiteralCommandNode<ServerCommandSource>>("refresh");
    refresh->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _refresh(ctx);
    });
    root->addChild(refresh);

    {
        auto block = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
        auto pos = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
            "pos", BlockPosArgumentType::blockPos());
        pos->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return _describeBlock(ctx);
        });
        block->addChild(pos);
        root->addChild(block);
    }

    {
        auto places = std::make_shared<LiteralCommandNode<ServerCommandSource>>("places");
        places->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return _describePlaces(ctx);
        });
        auto count = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
            "count", IntegerArgumentType::integer(1, MAX_PLACES));
        count->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return _describePlaces(ctx);
        });
        places->addChild(count);
        root->addChild(places);
    }

    {
        auto biome = std::make_shared<LiteralCommandNode<ServerCommandSource>>("biome");
        auto pos = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
            "pos", BlockPosArgumentType::blockPos());
        pos->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return _describeBiome(ctx);
        });
        biome->addChild(pos);
        root->addChild(biome);
    }

    dispatcher.registerCommand(root);
}

i32 PanelCommand::_refresh(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    // 直接用 IServer：PanelStatsCollector 只依赖 IServer::dimensionManager()，改成具体类型
    // 会让 dynamic_cast 要求 MinecraftServer 的 typeinfo，而 mc_benchmark 并未编入
    // MinecraftServer.cpp（它只为基准测试登记最小符号闭包），Release 链接期必然失败。
    server::IServer* server = source.server();
    if (server == nullptr) {
        source.sendError("Server not available");
        return 0;
    }

    PanelDetailStore* store = _panelStore(source);
    if (store == nullptr) {
        source.sendError(NO_PANEL_HINT);
        return 0;
    }

    panel::PanelStatsCollector collector(*server, panel::PanelStatsOptions{});
    std::vector<std::string> lines = collector.collect();
    const usize lineCount = lines.size();
    store->update(std::move(lines));

    source.sendMessage(fmt::format("Panel refreshed ({} lines)", lineCount));
    return static_cast<i32>(lineCount);
}

i32 PanelCommand::_describeBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("No world available for this command source");
        return 0;
    }

    const Vector3i position = BlockPosArgumentType::getBlockPos(context, "pos", source);
    ChunkData* chunk = _requireChunk(*world, position);
    if (chunk == nullptr) {
        source.sendError("That chunk is not loaded");
        return 0;
    }

    const BlockPos pos(position.x, position.y, position.z);
    const BlockState* state = chunk->getBlockState(position.x, position.y, position.z);
    if (state == nullptr) {
        source.sendError("No block data at this position");
        return 0;
    }

    source.sendMessage(
        fmt::format("Block at {}, {}, {}: {}", position.x, position.y, position.z, panel::describeBlockState(state)));

    // 方块实体是位置的附加信息：有就打印一份压缩后的 NBT，没有就明确说明。
    if (const BlockEntity* blockEntity = world->getBlockEntity(pos); blockEntity != nullptr) {
        const ::mc::nbt::CompoundTag tag = blockEntity->getUpdateTag();
        source.sendMessage("Block entity: " + panel::describeNbt(tag, MAX_NBT_LENGTH));
    } else {
        source.sendMessage("Block entity: none");
    }
    return 1;
}

i32 PanelCommand::_describePlaces(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    // 直接用 IServer：PanelStatsCollector 只依赖 IServer::dimensionManager()，改成具体类型
    // 会让 dynamic_cast 要求 MinecraftServer 的 typeinfo，而 mc_benchmark 并未编入
    // MinecraftServer.cpp（它只为基准测试登记最小符号闭包），Release 链接期必然失败。
    server::IServer* server = source.server();
    if (server == nullptr) {
        source.sendError("Server not available");
        return 0;
    }

    i32 count = 5;
    if (context.hasArgument("count")) {
        count = context.getArgument<i32>("count");
    }
    count = std::clamp(count, 1, MAX_PLACES);

    // 收集全部维度的非空气方块坐标（每个区块纵向抽样步长与面板统计一致）。
    struct PlaceSample {
        std::string dimension;
        i32 x = 0;
        i32 y = 0;
        i32 z = 0;
        std::string block;
    };

    std::vector<PlaceSample> places;

    server->dimensionManager().forEachDimension([&places](Dimension& dimension) {
        auto* serverDimension = static_cast<ServerDimension*>(&dimension);
        auto* world = serverDimension->world();
        if (world == nullptr) {
            return;
        }
        auto* chunkManager = world->chunkManager();
        if (chunkManager == nullptr) {
            return;
        }
        const std::string dimensionName = panel::stripNamespace(dimension.type().name());

        chunkManager->forEachLoadedChunk([&places, &dimensionName](const ChunkData& chunk) {
            const i32 baseX = chunk.x() * world::CHUNK_WIDTH;
            const i32 baseZ = chunk.z() * world::CHUNK_WIDTH;
            for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
                for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
                    for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; y += 8) {
                        const auto* state = chunk.getBlockState(baseX + x, y, baseZ + z);
                        if (state == nullptr || state->isAir()) {
                            continue;
                        }
                        places.push_back(
                            PlaceSample{dimensionName, baseX + x, y, baseZ + z, panel::describeBlockState(state)});
                        return true; // 每列只取最低的一处非空气方块，坐标天然不重复
                    }
                }
            }
            return true;
        });
    });

    if (places.empty()) {
        source.sendError("No loaded blocks to sample");
        return 0;
    }

    math::Random rng(_randomSeed());
    // 局部洗牌前 count 项即可（Fisher–Yates），避免整表打乱。
    const usize limit = std::min<usize>(static_cast<usize>(count), places.size());
    for (usize index = 0; index < limit; ++index) {
        const usize remaining = places.size() - index;
        const usize pick =
            index + static_cast<usize>(rng.nextInt(static_cast<i32>(std::min<usize>(remaining, 0x7FFFFFFF))));
        std::swap(places[index], places[pick]);
        const PlaceSample& sample = places[index];
        source.sendMessage(
            fmt::format(" {} {} {} {} ({})", sample.dimension, sample.x, sample.y, sample.z, sample.block));
    }
    return static_cast<i32>(limit);
}

i32 PanelCommand::_describeBiome(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("No world available for this command source");
        return 0;
    }

    const Vector3i position = BlockPosArgumentType::getBlockPos(context, "pos", source);
    source.sendMessage(fmt::format("Biome at {}, {}, {}: {}",
        position.x,
        position.y,
        position.z,
        _runtimeBiomeName(*world, position.x, position.y, position.z)));
    return 1;
}

} // namespace mc::command
