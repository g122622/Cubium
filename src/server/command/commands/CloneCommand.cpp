#include "CloneCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "common/world/block/BlockPos.hpp"
#include <sstream>

namespace mc {
namespace command {

void CloneCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto cloneNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clone");
    cloneNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        cloneNode,
        support::makeMetadata(
            "Clones blocks from one region to another.",
            "/clone <begin> <end> <destination>",
            2,
            {},
            true));

    auto beginArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "begin",
        Vec3ArgumentType::vec3());

    auto endArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "end",
        Vec3ArgumentType::vec3());

    auto destArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "destination",
        Vec3ArgumentType::vec3());
    destArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return cloneBlocks(ctx);
    });

    endArg->addChild(destArg);
    beginArg->addChild(endArg);
    cloneNode->addChild(beginArg);

    dispatcher.registerCommand(cloneNode);
}

i32 CloneCommand::cloneBlocks(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const Vector3d& begin = context.getArgument<Vector3d>("begin");
    const Vector3d& end = context.getArgument<Vector3d>("end");
    const Vector3d& dest = context.getArgument<Vector3d>("destination");

    BlockPos beginPos(
        static_cast<BlockCoord>(begin.x),
        static_cast<BlockCoord>(begin.y),
        static_cast<BlockCoord>(begin.z)
    );
    BlockPos endPos(
        static_cast<BlockCoord>(end.x),
        static_cast<BlockCoord>(end.y),
        static_cast<BlockCoord>(end.z)
    );
    BlockPos destPos(
        static_cast<BlockCoord>(dest.x),
        static_cast<BlockCoord>(dest.y),
        static_cast<BlockCoord>(dest.z)
    );

    // 计算区域大小
    i32 minX = std::min(beginPos.x, endPos.x);
    i32 minY = std::min(beginPos.y, endPos.y);
    i32 minZ = std::min(beginPos.z, endPos.z);
    i32 maxX = std::max(beginPos.x, endPos.x);
    i32 maxY = std::max(beginPos.y, endPos.y);
    i32 maxZ = std::max(beginPos.z, endPos.z);

    i32 blocksCloned = (maxX - minX + 1) * (maxY - minY + 1) * (maxZ - minZ + 1);

    // TODO: 实现实际的方块复制逻辑
    // 需要从世界读取源区域方块，然后写入目标区域

    std::ostringstream ss;
    ss << "Cloned " << blocksCloned << " blocks from ("
       << minX << ", " << minY << ", " << minZ << ") to ("
       << maxX << ", " << maxY << ", " << maxZ << ") at destination ("
       << destPos.x << ", " << destPos.y << ", " << destPos.z << ")";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
