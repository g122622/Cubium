#include "ForceLoadCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "common/world/block/BlockPos.hpp"
#include <sstream>

namespace mc {
namespace command {

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

    // /forceload add <pos> [to]
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
    forceloadNode->addChild(removeNode);

    // /forceload query <pos>
    auto queryNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("query");
    auto queryPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "pos",
        Vec3ArgumentType::vec3());
    queryPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return queryForceLoad(ctx);
    });
    queryNode->addChild(queryPosArg);
    forceloadNode->addChild(queryNode);

    // /forceload remove all
    auto removeAllNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto allNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("all");
    allNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return removeAllForceLoad(ctx);
    });
    removeNode->addChild(allNode);

    dispatcher.registerCommand(forceloadNode);
}

i32 ForceLoadCommand::addForceLoad(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const Vector3d& pos = context.getArgument<Vector3d>("pos");

    BlockPos blockPos(
        static_cast<BlockCoord>(pos.x),
        static_cast<BlockCoord>(pos.y),
        static_cast<BlockCoord>(pos.z)
    );

    ChunkCoord chunkX = blockPos.x >> 4;
    ChunkCoord chunkZ = blockPos.z >> 4;

    i32 count = 1;
    if (context.hasArgument("to")) {
        const Vector3d& to = context.getArgument<Vector3d>("to");
        BlockPos toPos(
            static_cast<BlockCoord>(to.x),
            static_cast<BlockCoord>(to.y),
            static_cast<BlockCoord>(to.z)
        );
        ChunkCoord toChunkX = toPos.x >> 4;
        ChunkCoord toChunkZ = toPos.z >> 4;

        i32 minX = std::min(chunkX, toChunkX);
        i32 minZ = std::min(chunkZ, toChunkZ);
        i32 maxX = std::max(chunkX, toChunkX);
        i32 maxZ = std::max(chunkZ, toChunkZ);

        count = (maxX - minX + 1) * (maxZ - minZ + 1);

        std::ostringstream ss;
        ss << "Added " << count << " chunk(s) to force load from (" << minX << ", " << minZ
           << ") to (" << maxX << ", " << maxZ << ")";
        source.sendMessage(ss.str());
    } else {
        std::ostringstream ss;
        ss << "Added chunk at (" << chunkX << ", " << chunkZ << ") to force load";
        source.sendMessage(ss.str());
    }

    // TODO: 实现强制加载区块系统
    // 1. 添加区块到强制加载列表
    // 2. 持久化到世界数据

    return count;
}

i32 ForceLoadCommand::removeForceLoad(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const Vector3d& pos = context.getArgument<Vector3d>("pos");

    BlockPos blockPos(
        static_cast<BlockCoord>(pos.x),
        static_cast<BlockCoord>(pos.y),
        static_cast<BlockCoord>(pos.z)
    );

    ChunkCoord chunkX = blockPos.x >> 4;
    ChunkCoord chunkZ = blockPos.z >> 4;

    std::ostringstream ss;
    ss << "Removed chunk at (" << chunkX << ", " << chunkZ << ") from force load";
    source.sendMessage(ss.str());

    // TODO: 实现强制加载区块系统

    return 1;
}

i32 ForceLoadCommand::queryForceLoad(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const Vector3d& pos = context.getArgument<Vector3d>("pos");

    BlockPos blockPos(
        static_cast<BlockCoord>(pos.x),
        static_cast<BlockCoord>(pos.y),
        static_cast<BlockCoord>(pos.z)
    );

    ChunkCoord chunkX = blockPos.x >> 4;
    ChunkCoord chunkZ = blockPos.z >> 4;

    // TODO: 实现强制加载区块系统，查询区块是否被强制加载
    std::ostringstream ss;
    ss << "Chunk at (" << chunkX << ", " << chunkZ << ") is not force loaded";
    source.sendMessage(ss.str());

    return 1;
}

i32 ForceLoadCommand::removeAllForceLoad(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    source.sendMessage("Removed all force loaded chunks in this dimension");

    // TODO: 实现强制加载区块系统

    return 1;
}

} // namespace command
} // namespace mc
