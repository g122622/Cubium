#include "PardonIpCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/application/IServer.hpp"

#include <sstream>

namespace mc {
namespace command {

void PardonIpCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto pardonIpNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("pardon-ip");
    pardonIpNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(3);
    });
    support::applyMetadata(
        pardonIpNode,
        support::makeMetadata(
            "Removes an IP address from the ban list.",
            "/pardon-ip <target>",
            3,
            {},
            false));

    // /pardon-ip <target>
    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "target",
        StringArgumentType::string()
    );
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return pardonIp(ctx);
    });

    pardonIpNode->addChild(targetArg);
    dispatcher.registerCommand(pardonIpNode);
}

i32 PardonIpCommand::pardonIp(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    String target = context.getArgument<String>("target");

    // TODO: 实现封禁 IP 列表系统
    // 需要检查 BannedIpList 是否包含该 IP

    std::ostringstream ss;
    ss << "Unbanned IP " << target;
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
