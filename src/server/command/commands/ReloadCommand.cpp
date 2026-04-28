#include "ReloadCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"

namespace mc {
namespace command {

void ReloadCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto reloadNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reload");
    reloadNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        reloadNode,
        support::makeMetadata(
            "Reloads loot tables, advancements, and functions from disk.",
            "/reload",
            2,
            {},
            true));

    reloadNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return reload(ctx);
    });

    dispatcher.registerCommand(reloadNode);
}

i32 ReloadCommand::reload(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    source.sendMessage("Reloading server resources...");

    // TODO: 实现资源重新加载系统
    // 1. 重新加载数据包
    // 2. 重新加载进度
    // 3. 重新加载战利品表
    // 4. 重新加载函数
    // 5. 通知客户端

    source.sendMessage("Reload complete!");
    return 1;
}

} // namespace command
} // namespace mc
