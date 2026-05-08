#include "ForceLoadCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "common/world/chunk/ChunkLoadTicketManager.hpp"
#include "common/world/chunk/ChunkPos.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/WorldConstants.hpp"
#include <sstream>
#include <algorithm>

namespace mc {
namespace command {

// 引入 ChunkPos 类型
using mc::ChunkPos;

namespace {
// MC 1.16.5 最大强制加载区块数量限制
constexpr i32 MAX_FORCE_LOAD_CHUNKS = 256;

// 世界边界常量
constexpr i32 WORLD_BORDER_MIN = -30000000;
constexpr i32 WORLD_BORDER_MAX = 30000000;
} // namespace

void ForceLoadCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto forceloadNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("forceload");
    forceloadNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        forceloadNode,
        support::makeMetadata(
            "Forces chunks to stay loaded.",
            "/forceload <add|remove|query> <pos> [to]",
            2,
            {},
            true));

    // /forceload add <from> [to]
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto posArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "pos",
        Vec3ArgumentType::vec3());
    auto toArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "to",
        Vec3ArgumentType::vec3());
    toArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return addForceLoad(ctx);
    });
    posArg->addChild(toArg);
    posArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return addForceLoad(ctx);
    });
    addNode->addChild(posArg);
    forceloadNode->addChild(addNode);

    // /forceload remove <pos> [to]
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removePosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "pos",
        Vec3ArgumentType::vec3());
    auto removeToArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "to",
        Vec3ArgumentType::vec3());
    removeToArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return removeForceLoad(ctx);
    });
    removePosArg->addChild(removeToArg);
    removePosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return removeForceLoad(ctx);
    });
    removeNode->addChild(removePosArg);

    // /forceload remove all
    auto allNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("all");
    allNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return removeAllForceLoad(ctx);
    });
    removeNode->addChild(allNode);

    forceloadNode->addChild(removeNode);

    // /forceload query [<pos>]
    auto queryNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("query");
    auto queryPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "pos",
        Vec3ArgumentType::vec3());
    queryPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return queryForceLoad(ctx);
    });
    queryNode->addChild(queryPosArg);
    // /forceload query (without position - list all)
    queryNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return listAllForceLoad(ctx);
    });
    forceloadNode->addChild(queryNode);

    dispatcher.registerCommand(forceloadNode);
}

i32 ForceLoadCommand::addForceLoad(CommandContext<ServerCommandSource>& context)
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

    const Vector3d& pos = context.getArgument<Vector3d>("pos");

    // 转换为区块坐标
    BlockPos blockPos(
        static_cast<BlockCoord>(pos.x),
        static_cast<BlockCoord>(pos.y),
        static_cast<BlockCoord>(pos.z)
    );

    ChunkCoord chunkX = blockPos.x >> 4;
    ChunkCoord chunkZ = blockPos.z >> 4;

    // 计算范围
    ChunkCoord minChunkX = chunkX;
    ChunkCoord minChunkZ = chunkZ;
    ChunkCoord maxChunkX = chunkX;
    ChunkCoord maxChunkZ = chunkZ;

    // 检查是否有范围参数
    if (context.hasArgument("to")) {
        const Vector3d& to = context.getArgument<Vector3d>("to");
        BlockPos toPos(
            static_cast<BlockCoord>(to.x),
            static_cast<BlockCoord>(to.y),
            static_cast<BlockCoord>(to.z)
        );
        ChunkCoord toChunkX = toPos.x >> 4;
        ChunkCoord toChunkZ = toPos.z >> 4;

        minChunkX = std::min(chunkX, toChunkX);
        minChunkZ = std::min(chunkZ, toChunkZ);
        maxChunkX = std::max(chunkX, toChunkX);
        maxChunkZ = std::max(chunkZ, toChunkZ);
    }

    // 世界边界检查
    i64 minBlockX = static_cast<i64>(minChunkX) * 16;
    i64 minBlockZ = static_cast<i64>(minChunkZ) * 16;
    i64 maxBlockX = static_cast<i64>(maxChunkX) * 16 + 15;
    i64 maxBlockZ = static_cast<i64>(maxChunkZ) * 16 + 15;

    if (minBlockX < WORLD_BORDER_MIN || minBlockZ < WORLD_BORDER_MIN ||
        maxBlockX >= WORLD_BORDER_MAX || maxBlockZ >= WORLD_BORDER_MAX) {
        source.sendError("commands.forceload.failed.outOfWorld");
        return 0;
    }

    // 计算区块数量并检查限制
    i64 totalChunks = (static_cast<i64>(maxChunkX) - minChunkX + 1) *
                      (static_cast<i64>(maxChunkZ) - minChunkZ + 1);

    if (totalChunks > MAX_FORCE_LOAD_CHUNKS) {
        std::ostringstream ss;
        ss << "commands.forceload.failed.tooManyChunks:" << MAX_FORCE_LOAD_CHUNKS << ":" << totalChunks;
        source.sendError(ss.str());
        return 0;
    }

    // 获取维度信息
    auto dimensionId = world->dimension();
    std::string dimensionName = "minecraft:overworld";
    if (dimensionId == -1) {
        dimensionName = "minecraft:the_nether";
    } else if (dimensionId == 1) {
        dimensionName = "minecraft:the_end";
    }

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
        ss << "commands.forceload.added.multiple:" << successCount << "," << dimensionName
           << ",[" << minChunkX << ", " << minChunkZ << "],[" << maxChunkX << ", " << maxChunkZ << "]";
        source.sendMessage(ss.str());
    }

    return successCount;
}

i32 ForceLoadCommand::removeForceLoad(CommandContext<ServerCommandSource>& context)
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

    const Vector3d& pos = context.getArgument<Vector3d>("pos");

    // 转换为区块坐标
    BlockPos blockPos(
        static_cast<BlockCoord>(pos.x),
        static_cast<BlockCoord>(pos.y),
        static_cast<BlockCoord>(pos.z)
    );

    ChunkCoord chunkX = blockPos.x >> 4;
    ChunkCoord chunkZ = blockPos.z >> 4;

    // 计算范围
    ChunkCoord minChunkX = chunkX;
    ChunkCoord minChunkZ = chunkZ;
    ChunkCoord maxChunkX = chunkX;
    ChunkCoord maxChunkZ = chunkZ;

    // 检查是否有范围参数
    if (context.hasArgument("to")) {
        const Vector3d& to = context.getArgument<Vector3d>("to");
        BlockPos toPos(
            static_cast<BlockCoord>(to.x),
            static_cast<BlockCoord>(to.y),
            static_cast<BlockCoord>(to.z)
        );
        ChunkCoord toChunkX = toPos.x >> 4;
        ChunkCoord toChunkZ = toPos.z >> 4;

        minChunkX = std::min(chunkX, toChunkX);
        minChunkZ = std::min(chunkZ, toChunkZ);
        maxChunkX = std::max(chunkX, toChunkX);
        maxChunkZ = std::max(chunkZ, toChunkZ);
    }

    // 获取维度信息
    auto dimensionId = world->dimension();
    std::string dimensionName = "minecraft:overworld";
    if (dimensionId == -1) {
        dimensionName = "minecraft:the_nether";
    } else if (dimensionId == 1) {
        dimensionName = "minecraft:the_end";
    }

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
        ss << "commands.forceload.removed.multiple:" << removedCount << "," << dimensionName
           << ",[" << minChunkX << ", " << minChunkZ << "],[" << maxChunkX << ", " << maxChunkZ << "]";
        source.sendMessage(ss.str());
    }

    return removedCount;
}

i32 ForceLoadCommand::queryForceLoad(CommandContext<ServerCommandSource>& context)
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

    const Vector3d& pos = context.getArgument<Vector3d>("pos");

    // 转换为区块坐标
    BlockPos blockPos(
        static_cast<BlockCoord>(pos.x),
        static_cast<BlockCoord>(pos.y),
        static_cast<BlockCoord>(pos.z)
    );

    ChunkCoord chunkX = blockPos.x >> 4;
    ChunkCoord chunkZ = blockPos.z >> 4;

    // 获取维度信息
    auto dimensionId = world->dimension();
    std::string dimensionName = "minecraft:overworld";
    if (dimensionId == -1) {
        dimensionName = "minecraft:the_nether";
    } else if (dimensionId == 1) {
        dimensionName = "minecraft:the_end";
    }

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

i32 ForceLoadCommand::listAllForceLoad(CommandContext<ServerCommandSource>& context)
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
    std::string dimensionName = "minecraft:overworld";
    if (dimensionId == -1) {
        dimensionName = "minecraft:the_nether";
    } else if (dimensionId == 1) {
        dimensionName = "minecraft:the_end";
    }

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
    std::sort(forcedChunks.begin(), forcedChunks.end(),
        [](const ChunkPos& a, const ChunkPos& b) {
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

i32 ForceLoadCommand::removeAllForceLoad(CommandContext<ServerCommandSource>& context)
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
    std::string dimensionName = "minecraft:overworld";
    if (dimensionId == -1) {
        dimensionName = "minecraft:the_nether";
    } else if (dimensionId == 1) {
        dimensionName = "minecraft:the_end";
    }

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
