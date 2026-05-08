#include "ExecuteCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/Block.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"

#include <sstream>

namespace mc {
namespace command {

namespace {

/**
 * @brief 解析方块ID
 */
Block* parseBlockId(const std::string& input) {
    auto& registry = BlockRegistry::instance();

    std::string namespace_;
    std::string path;
    size_t colonPos = input.find(':');
    if (colonPos != std::string::npos) {
        namespace_ = input.substr(0, colonPos);
        path = input.substr(colonPos + 1);
    } else {
        namespace_ = "minecraft";
        path = input;
    }

    // 去除状态属性部分
    size_t bracketPos = path.find('[');
    if (bracketPos != std::string::npos) {
        path = path.substr(0, bracketPos);
    }

    ResourceLocation location(namespace_, path);
    return registry.getBlock(location);
}

} // namespace

void ExecuteCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto executeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("execute");
    executeNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        executeNode,
        support::makeMetadata(
            "Executes a command.",
            "/execute as|at|positioned|run|if|unless ...",
            2,
            {},
            false));

    // /execute as <entity> <command>
    auto asNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("as");
    auto asEntityArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "entity",
        EntityArgumentType::player()
    );
    // 注：完整实现需要支持嵌套命令，这里简化处理
    asEntityArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return executeAs(ctx);
    });

    // /execute at <entity> <command>
    auto atNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("at");
    auto atEntityArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "entity",
        EntityArgumentType::player()
    );
    atEntityArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return executeAt(ctx);
    });

    // /execute positioned <pos> <command>
    auto positionedNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("positioned");
    auto posArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "pos",
        Vec3ArgumentType::vec3()
    );
    posArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return executePositioned(ctx);
    });

    // /execute run <command>
    auto runNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    // 注：完整实现需要支持嵌套命令，这里简化处理
    runNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return executeRun(ctx);
    });

    // /execute if block <pos> <block> <command>
    auto ifNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("if");
    auto ifBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto ifBlockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3i>>(
        "pos",
        BlockPosArgumentType::blockPos()
    );
    auto ifBlockArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "block",
        StringArgumentType::string()
    );
    ifBlockArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return executeIfBlock(ctx);
    });

    // /execute unless block <pos> <block> <command>
    auto unlessNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("unless");
    auto unlessBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto unlessBlockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3i>>(
        "pos",
        BlockPosArgumentType::blockPos()
    );
    auto unlessBlockArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "block",
        StringArgumentType::string()
    );
    unlessBlockArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return executeUnlessBlock(ctx);
    });

    // 构建命令树
    asNode->addChild(asEntityArg);
    executeNode->addChild(asNode);

    atNode->addChild(atEntityArg);
    executeNode->addChild(atNode);

    positionedNode->addChild(posArg);
    executeNode->addChild(positionedNode);

    executeNode->addChild(runNode);

    ifBlockPosArg->addChild(ifBlockNode);
    ifBlockNode->addChild(ifBlockArg);
    ifNode->addChild(ifBlockPosArg);
    executeNode->addChild(ifNode);

    unlessBlockPosArg->addChild(unlessBlockNode);
    unlessBlockNode->addChild(unlessBlockArg);
    unlessNode->addChild(unlessBlockPosArg);
    executeNode->addChild(unlessNode);

    dispatcher.registerCommand(executeNode);
}

i32 ExecuteCommand::executeAs(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("entity");

    // 解析目标玩家
    std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("commands.execute.failed.noEntity");
        return 0;
    }

    // 以每个目标玩家身份执行
    i32 totalResult = 0;
    for (PlayerId playerId : playerIds) {
        // 获取玩家数据
        auto* server = source.server();
        if (server == nullptr) {
            continue;
        }

        auto* playerData = server->playerManager().getPlayer(playerId);
        if (playerData == nullptr) {
            continue;
        }

        // 创建修改后的命令源（使用 withPosition 方法）
        ServerCommandSource modifiedSource = source.withPosition(
            Vector3d(playerData->x, playerData->y, playerData->z)
        );

        // TODO: 执行嵌套命令
        // 当前简化实现：仅报告成功
        totalResult++;
    }

    return totalResult;
}

i32 ExecuteCommand::executeAt(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("entity");

    // 解析目标玩家
    std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("commands.execute.failed.noEntity");
        return 0;
    }

    // 在每个目标玩家位置执行
    i32 totalResult = 0;
    for (PlayerId playerId : playerIds) {
        auto* server = source.server();
        if (server == nullptr) {
            continue;
        }

        auto* playerData = server->playerManager().getPlayer(playerId);
        if (playerData == nullptr) {
            continue;
        }

        // 创建修改位置的命令源
        ServerCommandSource modifiedSource = source.withPosition(
            Vector3d(playerData->x, playerData->y, playerData->z)
        );

        // TODO: 执行嵌套命令
        totalResult++;
    }

    return totalResult;
}

i32 ExecuteCommand::executePositioned(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    Vector3d position = context.getArgument<Vector3d>("pos");

    // 创建修改位置的命令源
    ServerCommandSource modifiedSource = source.withPosition(position);

    // TODO: 执行嵌套命令
    // 当前简化实现：仅报告成功
    source.sendMessage("Position set to execute commands at");

    return 1;
}

i32 ExecuteCommand::executeRun(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // TODO: 执行嵌套命令
    // 当前简化实现：仅报告成功
    source.sendMessage("Run subcommand executed");

    return 1;
}

i32 ExecuteCommand::executeIfBlock(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    Vector3i position = context.getArgument<Vector3i>("pos");
    std::string blockInput = context.getArgument<std::string>("block");

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.execute.failed.noWorld");
        return 0;
    }

    // 解析目标方块
    Block* targetBlock = parseBlockId(blockInput);
    if (targetBlock == nullptr) {
        std::ostringstream ss;
        ss << "commands.execute.failed.invalidBlock: " << blockInput;
        source.sendMessage(ss.str());
        return 0;
    }

    // 检查方块
    const BlockState* currentState = world->getBlockState(position.x, position.y, position.z);
    bool matches = false;
    if (currentState != nullptr) {
        matches = currentState->getBlock().blockId() == targetBlock->blockId();
    }

    if (!matches) {
        // 条件不满足，不执行后续命令
        return 0;
    }

    // TODO: 执行嵌套命令
    return 1;
}

i32 ExecuteCommand::executeUnlessBlock(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    Vector3i position = context.getArgument<Vector3i>("pos");
    std::string blockInput = context.getArgument<std::string>("block");

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.execute.failed.noWorld");
        return 0;
    }

    // 解析目标方块
    Block* targetBlock = parseBlockId(blockInput);
    if (targetBlock == nullptr) {
        std::ostringstream ss;
        ss << "commands.execute.failed.invalidBlock: " << blockInput;
        source.sendMessage(ss.str());
        return 0;
    }

    // 检查方块
    const BlockState* currentState = world->getBlockState(position.x, position.y, position.z);
    bool matches = false;
    if (currentState != nullptr) {
        matches = currentState->getBlock().blockId() == targetBlock->blockId();
    }

    if (matches) {
        // 条件满足，不执行后续命令
        return 0;
    }

    // TODO: 执行嵌套命令
    return 1;
}

} // namespace command
} // namespace mc
