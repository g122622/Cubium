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

#include "ForceLoadCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/load/ChunkLoadTicketManager.hpp"
#include "common/world/dimension/MapDimensionId.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string_view>
#include <vector>

namespace mc {
namespace command {

namespace {
// 单次操作最大强制加载区块数量限制
constexpr i32 MAX_FORCE_LOAD_CHUNKS = 256;
} // namespace

void ForceLoadCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto forceloadNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("forceload");
    forceloadNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(forceloadNode,
        support::makeMetadata(
            "Forces chunks to stay loaded.", "/forceload <add|remove|query> <pos> [to]", 2, {}, true));

    // /forceload add <from> [to]
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto posArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>("pos", Vec3ArgumentType::vec3());
    auto toArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>("to", Vec3ArgumentType::vec3());
    toArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _addForceLoad(ctx); });
    posArg->addChild(toArg);
    posArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _addForceLoad(ctx); });
    addNode->addChild(posArg);
    forceloadNode->addChild(addNode);

    // /forceload remove <pos> [to]
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removePosArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>("pos", Vec3ArgumentType::vec3());
    auto removeToArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>("to", Vec3ArgumentType::vec3());
    removeToArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _removeForceLoad(ctx); });
    removePosArg->addChild(removeToArg);
    removePosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _removeForceLoad(ctx); });
    removeNode->addChild(removePosArg);

    // /forceload remove all
    auto allNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("all");
    allNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _removeAllForceLoad(ctx); });
    removeNode->addChild(allNode);

    forceloadNode->addChild(removeNode);

    // /forceload query [<pos>]
    auto queryNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("query");
    auto queryPosArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>("pos", Vec3ArgumentType::vec3());
    queryPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _queryForceLoad(ctx); });
    queryNode->addChild(queryPosArg);
    // /forceload query (without position - list all)
    queryNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _listAllForceLoad(ctx); });
    forceloadNode->addChild(queryNode);

    dispatcher.registerCommand(forceloadNode);
}

std::string_view ForceLoadCommand::_getDimensionName(DimensionId dimensionId)
{
    return dimensionIdToString(dimensionId);
}

i32 ForceLoadCommand::_addForceLoad(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.forceload.failed.noWorld");
        return 0;
    }

    // 获取区块管理器
    server::ServerChunkManager* chunkManager = world->chunkManager();
    if (chunkManager == nullptr) {
        source.sendError("commands.forceload.failed.noChunkManager");
        return 0;
    }

    const Vector3d pos = Vec3ArgumentType::getVec3(context, "pos", source);

    // 转换为区块坐标（使用 BlockPos 的 chunkX/chunkZ 方法）
    BlockPos blockPos(static_cast<BlockCoord>(pos.x), static_cast<BlockCoord>(pos.y), static_cast<BlockCoord>(pos.z));

    ChunkCoord chunkX = blockPos.chunkX();
    ChunkCoord chunkZ = blockPos.chunkZ();

    // 计算范围
    ChunkCoord minChunkX = chunkX;
    ChunkCoord minChunkZ = chunkZ;
    ChunkCoord maxChunkX = chunkX;
    ChunkCoord maxChunkZ = chunkZ;

    // 检查是否有范围参数
    if (context.hasArgument("to")) {
        const Vector3d to = Vec3ArgumentType::getVec3(context, "to", source);
        BlockPos toPos(static_cast<BlockCoord>(to.x), static_cast<BlockCoord>(to.y), static_cast<BlockCoord>(to.z));
        ChunkCoord toChunkX = toPos.chunkX();
        ChunkCoord toChunkZ = toPos.chunkZ();

        minChunkX = std::min(chunkX, toChunkX);
        minChunkZ = std::min(chunkZ, toChunkZ);
        maxChunkX = std::max(chunkX, toChunkX);
        maxChunkZ = std::max(chunkZ, toChunkZ);
    }

    // 世界边界检查（使用 world 命名空间的工具函数）
    if (!world::isValidChunkCoord(minChunkX, minChunkZ) || !world::isValidChunkCoord(maxChunkX, maxChunkZ)) {
        source.sendError("commands.forceload.failed.outOfWorld");
        return 0;
    }

    // 计算区块数量并检查限制
    i64 totalChunks = (static_cast<i64>(maxChunkX) - minChunkX + 1) * (static_cast<i64>(maxChunkZ) - minChunkZ + 1);

    if (totalChunks > MAX_FORCE_LOAD_CHUNKS) {
        std::ostringstream ss;
        ss << "commands.forceload.failed.tooManyChunks:" << MAX_FORCE_LOAD_CHUNKS << ":" << totalChunks;
        source.sendError(ss.str());
        return 0;
    }

    // 获取维度信息
    auto dimensionId = world->dimension();
    std::string_view dimensionName = _getDimensionName(dimensionId);

    // 添加强制加载
    auto& ticketManager = chunkManager->ticketManager();
    i32 successCount = 0;
    ChunkPos firstChunk(minChunkX, minChunkZ);

    for (ChunkCoord cx = minChunkX; cx <= maxChunkX; ++cx) {
        for (ChunkCoord cz = minChunkZ; cz <= maxChunkZ; ++cz) {
            ticketManager.forceChunk(cx, cz, true);
            ++successCount;
            if (successCount == 1) {
                firstChunk = ChunkPos(cx, cz);
            }
        }
    }

    // 处理票据更新
    ticketManager.processUpdates();

    // 发送反馈消息
    if (successCount == 1) {
        std::ostringstream ss;
        ss << "commands.forceload.added.single:[" << firstChunk.x << ", " << firstChunk.z << "]," << dimensionName;
        source.sendMessage(ss.str());
    } else {
        std::ostringstream ss;
        ss << "commands.forceload.added.multiple:" << successCount << "," << dimensionName << ",[" << minChunkX << ", "
           << minChunkZ << "],[" << maxChunkX << ", " << maxChunkZ << "]";
        source.sendMessage(ss.str());
    }

    return successCount;
}

i32 ForceLoadCommand::_removeForceLoad(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.forceload.failed.noWorld");
        return 0;
    }

    // 获取区块管理器
    server::ServerChunkManager* chunkManager = world->chunkManager();
    if (chunkManager == nullptr) {
        source.sendError("commands.forceload.failed.noChunkManager");
        return 0;
    }

    const Vector3d pos = Vec3ArgumentType::getVec3(context, "pos", source);

    // 转换为区块坐标
    BlockPos blockPos(static_cast<BlockCoord>(pos.x), static_cast<BlockCoord>(pos.y), static_cast<BlockCoord>(pos.z));

    ChunkCoord chunkX = blockPos.chunkX();
    ChunkCoord chunkZ = blockPos.chunkZ();

    // 计算范围
    ChunkCoord minChunkX = chunkX;
    ChunkCoord minChunkZ = chunkZ;
    ChunkCoord maxChunkX = chunkX;
    ChunkCoord maxChunkZ = chunkZ;

    // 检查是否有范围参数
    if (context.hasArgument("to")) {
        const Vector3d to = Vec3ArgumentType::getVec3(context, "to", source);
        BlockPos toPos(static_cast<BlockCoord>(to.x), static_cast<BlockCoord>(to.y), static_cast<BlockCoord>(to.z));
        ChunkCoord toChunkX = toPos.chunkX();
        ChunkCoord toChunkZ = toPos.chunkZ();

        minChunkX = std::min(chunkX, toChunkX);
        minChunkZ = std::min(chunkZ, toChunkZ);
        maxChunkX = std::max(chunkX, toChunkX);
        maxChunkZ = std::max(chunkZ, toChunkZ);
    }

    // 获取维度信息
    auto dimensionId = world->dimension();
    std::string_view dimensionName = _getDimensionName(dimensionId);

    // 移除强制加载
    auto& ticketManager = chunkManager->ticketManager();
    i32 removedCount = 0;

    for (ChunkCoord cx = minChunkX; cx <= maxChunkX; ++cx) {
        for (ChunkCoord cz = minChunkZ; cz <= maxChunkZ; ++cz) {
            // 检查是否原本就是强制加载的
            if (ticketManager.isForcedChunk(cx, cz)) {
                ticketManager.forceChunk(cx, cz, false);
                ++removedCount;
            }
        }
    }

    // 处理票据更新
    ticketManager.processUpdates();

    if (removedCount == 0) {
        source.sendError("commands.forceload.remove.failed");
        return 0;
    }

    // 发送反馈消息
    if (removedCount == 1) {
        std::ostringstream ss;
        ss << "commands.forceload.removed.single:[" << minChunkX << ", " << minChunkZ << "]," << dimensionName;
        source.sendMessage(ss.str());
    } else {
        std::ostringstream ss;
        ss << "commands.forceload.removed.multiple:" << removedCount << "," << dimensionName << ",[" << minChunkX
           << ", " << minChunkZ << "],[" << maxChunkX << ", " << maxChunkZ << "]";
        source.sendMessage(ss.str());
    }

    return removedCount;
}

i32 ForceLoadCommand::_queryForceLoad(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.forceload.failed.noWorld");
        return 0;
    }

    // 获取区块管理器
    server::ServerChunkManager* chunkManager = world->chunkManager();
    if (chunkManager == nullptr) {
        source.sendError("commands.forceload.failed.noChunkManager");
        return 0;
    }

    const Vector3d pos = Vec3ArgumentType::getVec3(context, "pos", source);

    // 转换为区块坐标
    BlockPos blockPos(static_cast<BlockCoord>(pos.x), static_cast<BlockCoord>(pos.y), static_cast<BlockCoord>(pos.z));

    ChunkCoord chunkX = blockPos.chunkX();
    ChunkCoord chunkZ = blockPos.chunkZ();

    // 获取维度信息
    auto dimensionId = world->dimension();
    std::string_view dimensionName = _getDimensionName(dimensionId);

    // 查询强制加载状态
    auto& ticketManager = chunkManager->ticketManager();
    bool isForced = ticketManager.isForcedChunk(chunkX, chunkZ);

    if (isForced) {
        std::ostringstream ss;
        ss << "commands.forceload.query.success:[" << chunkX << ", " << chunkZ << "]," << dimensionName;
        source.sendMessage(ss.str());
        return 1;
    } else {
        std::ostringstream ss;
        ss << "commands.forceload.query.failed:[" << chunkX << ", " << chunkZ << "]," << dimensionName;
        source.sendError(ss.str());
        return 0;
    }
}

i32 ForceLoadCommand::_listAllForceLoad(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.forceload.failed.noWorld");
        return 0;
    }

    // 获取区块管理器
    server::ServerChunkManager* chunkManager = world->chunkManager();
    if (chunkManager == nullptr) {
        source.sendError("commands.forceload.failed.noChunkManager");
        return 0;
    }

    // 获取维度信息
    auto dimensionId = world->dimension();
    std::string_view dimensionName = _getDimensionName(dimensionId);

    // 获取所有强制加载区块
    auto& ticketManager = chunkManager->ticketManager();
    std::vector<ChunkPos> forcedChunks = ticketManager.getForcedChunks();

    i32 count = static_cast<i32>(forcedChunks.size());

    if (count == 0) {
        std::ostringstream ss;
        ss << "commands.forceload.list.none:" << dimensionName;
        source.sendMessage(ss.str());
        return 0;
    }

    // 按坐标排序
    std::sort(forcedChunks.begin(), forcedChunks.end(), [](const ChunkPos& a, const ChunkPos& b) {
        if (a.x != b.x) return a.x < b.x;
        return a.z < b.z;
    });

    // 构建区块列表字符串
    std::ostringstream chunkList;
    for (size_t i = 0; i < forcedChunks.size(); ++i) {
        if (i > 0) {
            chunkList << ", ";
        }
        chunkList << "[" << forcedChunks[i].x << ", " << forcedChunks[i].z << "]";
    }

    if (count == 1) {
        std::ostringstream ss;
        ss << "commands.forceload.list.single:" << dimensionName << "," << chunkList.str();
        source.sendMessage(ss.str());
    } else {
        std::ostringstream ss;
        ss << "commands.forceload.list.multiple:" << count << "," << dimensionName << "," << chunkList.str();
        source.sendMessage(ss.str());
    }

    return count;
}

i32 ForceLoadCommand::_removeAllForceLoad(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.forceload.failed.noWorld");
        return 0;
    }

    // 获取区块管理器
    server::ServerChunkManager* chunkManager = world->chunkManager();
    if (chunkManager == nullptr) {
        source.sendError("commands.forceload.failed.noChunkManager");
        return 0;
    }

    // 获取维度信息
    auto dimensionId = world->dimension();
    std::string_view dimensionName = _getDimensionName(dimensionId);

    // 获取所有强制加载区块并移除
    auto& ticketManager = chunkManager->ticketManager();
    std::vector<ChunkPos> forcedChunks = ticketManager.getForcedChunks();

    i32 removedCount = 0;
    for (const auto& pos : forcedChunks) {
        ticketManager.forceChunk(pos.x, pos.z, false);
        ++removedCount;
    }

    // 处理票据更新
    ticketManager.processUpdates();

    // 发送反馈消息
    std::ostringstream ss;
    ss << "commands.forceload.removed.all:" << dimensionName;
    source.sendMessage(ss.str());

    return removedCount;
}

} // namespace command
} // namespace mc
