#include "SetWorldSpawnCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/application/IServer.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include <sstream>

namespace mc {
namespace command {

void SetWorldSpawnCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto setWorldSpawnNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("setworldspawn");
    setWorldSpawnNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        setWorldSpawnNode,
        support::makeMetadata(
            "Sets the world spawn point.",
            "/setworldspawn [<pos>]",
            2,
            {},
            true));

    // /setworldspawn - 设置当前位置为世界出生点
    setWorldSpawnNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setCurrentPosition(ctx);
    });

    // /setworldspawn <pos>
    auto posNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "pos",
        Vec3ArgumentType::vec3()
    );
    posNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setPosition(ctx);
    });

    setWorldSpawnNode->addChild(posNode);
    dispatcher.registerCommand(setWorldSpawnNode);
}

i32 SetWorldSpawnCommand::setCurrentPosition(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    if (!source.isPlayer()) {
        source.sendError("You must be a player to use this command");
        return 0;
    }

    auto* server = source.server();
    DimensionId dimensionId = DimensionId(0); // 默认主世界
    auto* dimension = server->dimensionManager().getDimension(dimensionId);

    if (!dimension) {
        source.sendError("Dimension not found");
        return 0;
    }

    const Vector3d& pos = source.position();
    dimension->setSpawnPoint(pos);

    std::ostringstream ss;
    ss << "Set world spawn point to "
       << static_cast<BlockCoord>(pos.x) << ", "
       << static_cast<BlockCoord>(pos.y) << ", "
       << static_cast<BlockCoord>(pos.z);
    source.sendMessage(ss.str());

    return 1;
}

i32 SetWorldSpawnCommand::setPosition(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto* server = source.server();

    auto& pos = context.getArgument<Vector3d>("pos");

    DimensionId dimensionId = DimensionId(0); // 默认主世界
    auto* dimension = server->dimensionManager().getDimension(dimensionId);

    if (!dimension) {
        source.sendError("Dimension not found");
        return 0;
    }

    dimension->setSpawnPoint(pos);

    std::ostringstream ss;
    ss << "Set world spawn point to "
       << static_cast<BlockCoord>(pos.x) << ", "
       << static_cast<BlockCoord>(pos.y) << ", "
       << static_cast<BlockCoord>(pos.z);
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
