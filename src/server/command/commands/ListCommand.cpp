#include "ListCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/core/PlayerManager.hpp"

#include <sstream>

namespace mc {
namespace command {

void ListCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(0); });
    support::applyMetadata(listNode, support::makeMetadata("List online players.", "/list", 0));
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return listPlayers(ctx); });

    dispatcher.registerCommand(listNode);
}

i32 ListCommand::listPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    size_t playerCount = 0;

    if (auto* server = source.server()) {
        server->playerManager().forEachPlayer([&playerCount](auto&) {
            ++playerCount;
            return true;
        });
    }

    std::ostringstream ss;
    ss << "There are " << playerCount << " player(s) online";
    source.sendMessage(ss.str());

    return static_cast<i32>(playerCount);
}

} // namespace command
} // namespace mc