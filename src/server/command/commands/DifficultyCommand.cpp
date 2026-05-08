#include "DifficultyCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include <sstream>

namespace mc::command {

void DifficultyCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto difficultyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("difficulty");
    difficultyNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        difficultyNode,
        support::makeMetadata(
            "Query or set the world difficulty.",
            "/difficulty [peaceful|easy|normal|hard]",
            2));
    difficultyNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return queryDifficulty(ctx);
    });

    auto peacefulNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("peaceful");
    peacefulNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("difficulty", Difficulty::Peaceful);
        return setDifficulty(ctx);
    });

    auto easyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("easy");
    easyNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("difficulty", Difficulty::Easy);
        return setDifficulty(ctx);
    });

    auto normalNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("normal");
    normalNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("difficulty", Difficulty::Normal);
        return setDifficulty(ctx);
    });

    auto hardNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("hard");
    hardNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("difficulty", Difficulty::Hard);
        return setDifficulty(ctx);
    });

    difficultyNode->addChild(peacefulNode);
    difficultyNode->addChild(easyNode);
    difficultyNode->addChild(normalNode);
    difficultyNode->addChild(hardNode);

    dispatcher.registerCommand(difficultyNode);
}

i32 DifficultyCommand::queryDifficulty(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    std::ostringstream ss;
    ss << "The difficulty is " << support::getDifficultyCommandName(server->difficulty());
    source.sendMessage(ss.str());
    return static_cast<i32>(server->difficulty());
}

i32 DifficultyCommand::setDifficulty(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    const Difficulty difficulty = context.getArgument<Difficulty>("difficulty");
    if (server->difficulty() == difficulty) {
        source.sendMessage("Difficulty is already set to " + std::string(support::getDifficultyCommandName(difficulty)));
        return 0;
    }

    server->setDifficulty(difficulty);
    source.sendMessage("Set difficulty to " + std::string(support::getDifficultyCommandName(difficulty)));
    return 1;
}

} // namespace mc::command
