#include "WhitelistCommand.hpp"

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

void WhitelistCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto whitelistNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("whitelist");
    whitelistNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(3);
    });
    support::applyMetadata(
        whitelistNode,
        support::makeMetadata(
            "Manages the server whitelist.",
            "/whitelist <on|off|list|add|remove|reload>",
            3,
            {},
            false));

    // /whitelist on
    auto onNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("on");
    onNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return whitelistOn(ctx);
    });

    // /whitelist off
    auto offNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("off");
    offNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return whitelistOff(ctx);
    });

    // /whitelist list
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return whitelistList(ctx);
    });

    // /whitelist add <player>
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addPlayerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
    );
    addPlayerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return whitelistAdd(ctx);
    });
    addNode->addChild(addPlayerArg);

    // /whitelist remove <player>
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removePlayerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
    );
    removePlayerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return whitelistRemove(ctx);
    });
    removeNode->addChild(removePlayerArg);

    // /whitelist reload
    auto reloadNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reload");
    reloadNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return whitelistReload(ctx);
    });

    whitelistNode->addChild(onNode);
    whitelistNode->addChild(offNode);
    whitelistNode->addChild(listNode);
    whitelistNode->addChild(addNode);
    whitelistNode->addChild(removeNode);
    whitelistNode->addChild(reloadNode);

    dispatcher.registerCommand(whitelistNode);
}

i32 WhitelistCommand::whitelistOn(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // TODO: 实现白名单系统
    // 需要：
    // 1. WhitelistManager 类
    // 2. 保存到 whitelist.json
    // 3. 踢出不在白名单的玩家

    source.sendMessage("Whitelist is now turned on");
    return 1;
}

i32 WhitelistCommand::whitelistOff(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // TODO: 实现白名单系统
    source.sendMessage("Whitelist is now turned off");
    return 1;
}

i32 WhitelistCommand::whitelistList(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // TODO: 实现白名单列表
    source.sendMessage("There are 0 whitelisted players: ");
    return 1;
}

i32 WhitelistCommand::whitelistAdd(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 获取玩家名
    std::string playerName;
    if (selector.hasUsername()) {
        playerName = selector.username();
    } else {
        std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
        if (!playerIds.empty()) {
            auto* server = source.server();
            if (server != nullptr) {
                auto* playerData = server->playerManager().getPlayer(playerIds.front());
                if (playerData != nullptr) {
                    playerName = playerData->username;
                }
            }
        }
    }

    if (playerName.empty()) {
        source.sendError("commands.whitelist.add.failed");
        return 0;
    }

    // TODO: 实现白名单系统
    std::ostringstream ss;
    ss << "Added " << playerName << " to the whitelist";
    source.sendMessage(ss.str());

    return 1;
}

i32 WhitelistCommand::whitelistRemove(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 获取玩家名
    std::string playerName;
    if (selector.hasUsername()) {
        playerName = selector.username();
    } else {
        std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
        if (!playerIds.empty()) {
            auto* server = source.server();
            if (server != nullptr) {
                auto* playerData = server->playerManager().getPlayer(playerIds.front());
                if (playerData != nullptr) {
                    playerName = playerData->username;
                }
            }
        }
    }

    if (playerName.empty()) {
        source.sendError("commands.whitelist.remove.failed");
        return 0;
    }

    // TODO: 实现白名单系统
    std::ostringstream ss;
    ss << "Removed " << playerName << " from the whitelist";
    source.sendMessage(ss.str());

    return 1;
}

i32 WhitelistCommand::whitelistReload(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // TODO: 实现白名单系统
    source.sendMessage("Whitelist reloaded");
    return 1;
}

} // namespace command
} // namespace mc
