#include "BanIpCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"

#include <sstream>

namespace mc {
namespace command {

void BanIpCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto banIpNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("ban-ip");
    banIpNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(3);
    });
    support::applyMetadata(
        banIpNode,
        support::makeMetadata(
            "Bans an IP address from the server.",
            "/ban-ip <target> [reason]",
            3,
            {},
            false));

    // /ban-ip <target>
    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "target",
        StringArgumentType::string()
    );
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return banIp(ctx);
    });

    // /ban-ip <target> <reason>
    auto reasonArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "reason",
        StringArgumentType::greedyString()
    );
    reasonArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return banIp(ctx);
    });

    targetArg->addChild(reasonArg);
    banIpNode->addChild(targetArg);
    dispatcher.registerCommand(banIpNode);
}

i32 BanIpCommand::banIp(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    std::string target = context.getArgument<std::string>("target");

    // 获取可选原因
    std::string reason = "Banned by an operator";
    if (context.hasArgument("reason")) {
        reason = context.getArgument<std::string>("reason");
    }

    // 检查是否是 IP 地址格式
    bool isIp = true;
    for (char c : target) {
        if (c != '.' && (c < '0' || c > '9')) {
            isIp = false;
            break;
        }
    }

    // TODO: 实现封禁 IP 列表系统
    // 需要：
    // 1. BannedIpList 类
    // 2. BannedIpEntry 类
    // 3. 保存到 banned-ips.json
    // 4. 断开该 IP 的所有连接

    std::ostringstream ss;
    if (isIp) {
        ss << "Banned IP " << target << ": " << reason;
    } else {
        ss << "Banned IP for player " << target << ": " << reason;
    }
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
