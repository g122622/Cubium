#include "PardonCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/BannedPlayerList.hpp"

#include <sstream>

namespace mc {
namespace command {

void PardonCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto pardonNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("pardon");
    pardonNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(3);
    });
    support::applyMetadata(
        pardonNode,
        support::makeMetadata(
            "Removes a player from the ban list.",
            "/pardon <player>",
            3,
            {},
            false));

    // /pardon <player>
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
    );
    playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return pardonPlayer(ctx);
    });

    pardonNode->addChild(playerArg);
    dispatcher.registerCommand(pardonNode);
}

i32 PardonCommand::pardonPlayer(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 获取玩家名
    std::string playerName;

    // 首先尝试从选择器获取玩家名
    if (selector.hasUsername()) {
        playerName = selector.username();
    } else {
        // 尝试从在线玩家获取
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
        source.sendError("commands.pardon.failed.noPlayer");
        return 0;
    }

    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.pardon.failed.noServer");
        return 0;
    }

    auto& banList = server->bannedPlayerList();

    // 检查玩家是否被封禁
    if (!banList.isNameBanned(playerName)) {
        std::ostringstream ss;
        ss << "Could not unban " << playerName << " (not banned)";
        source.sendError(ss.str());
        return 0;
    }

    // 移除封禁
    if (!banList.removeEntryByName(playerName)) {
        std::ostringstream ss;
        ss << "Failed to remove " << playerName << " from ban list";
        source.sendError(ss.str());
        return 0;
    }

    // 保存封禁列表
    auto saveResult = banList.save();
    if (saveResult.failed()) {
        spdlog::error("Failed to save banned players list: {}", saveResult.error().message());
    }

    // 发送成功消息
    std::ostringstream ss;
    ss << "Unbanned " << playerName;
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
