#include "SeedCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"

#include <sstream>

namespace mc {
namespace command {

void SeedCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto seedNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("seed");
    seedNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(seedNode, support::makeMetadata("Show the world seed.", "/seed", 2));
    seedNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return showSeed(ctx); });

    dispatcher.registerCommand(seedNode);
}

i32 SeedCommand::showSeed(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    i64 seed = 0;

    if (server) {
        seed = static_cast<i64>(server->seed());
    } else {
        source.sendMessage("Server not available");
        return 0;
    }

    std::ostringstream ss;
    ss << "Seed: [" << seed << "]";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc