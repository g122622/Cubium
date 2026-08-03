/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "DeOpCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/network/PacketBuilders.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <memory>
#include <sstream>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

void DeOpCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto deopNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("deop");
    deopNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(3); });
    support::applyMetadata(
        deopNode, support::makeMetadata("Revokes operator status from a player.", "/deop <player>", 3, {}, false));

    // /deop <player>
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::player());
    playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _deopPlayer(ctx); });

    deopNode->addChild(playerArg);
    dispatcher.registerCommand(deopNode);
}

i32 DeOpCommand::_deopPlayer(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 解析目标玩家
    std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("commands.deop.failed.noPlayer");
        return 0;
    }

    // 只操作一个玩家
    PlayerId targetId = playerIds.front();

    // 获取服务器实例
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.deop.failed.noServer");
        return 0;
    }

    // 获取玩家数据
    auto* playerData = server->playerManager().getPlayer(targetId);
    if (playerData == nullptr) {
        source.sendError("commands.deop.failed.playerNotFound");
        return 0;
    }

    // 获取 OP 列表管理器
    auto& opList = server->opListManager();

    // 检查玩家是否是 OP
    if (!opList.isOp(playerData->uuid)) {
        std::ostringstream ss;
        ss << "commands.deop.failed";
        source.sendError(ss.str());
        return 0;
    }

    // 从 OP 列表移除
    if (!opList.removeEntry(playerData->uuid)) {
        source.sendError("commands.deop.failed");
        return 0;
    }

    // 保存 OP 列表
    auto saveResult = opList.save();
    if (saveResult.failed()) {
        spdlog::error("Failed to save ops.json: {}", saveResult.error().message());
    }

    // 更新在线玩家实体的权限等级（已移除 OP，降为普通玩家）
    if (auto* world = server->getPlayerWorld(targetId)) {
        if (Player* player = server->playerEntityManager().getPlayerEntity(targetId, *world)) {
            player->setPermissionLevel(0);

            // 通知客户端权限等级变更并同步命令树（批5b：经 builder + connectionManager
            // 投递，原 IServer::sendPermissionLevelChange 纯虚已删。对齐原实现：先发
            // EntityEvent(24+0)，再发命令树刷新可用命令列表）。
            auto& conn = server->connectionManager();
            conn.sendToPlayer(
                targetId, mc::server::net::buildPermissionLevelChangeIr(static_cast<i32>(player->id()), 0));
            if (auto commands = mc::server::net::buildCommandsIr(server->commandRegistry())) {
                conn.sendToPlayer(targetId, *commands);
            }
        }
    }

    // 发送成功消息
    std::ostringstream ss;
    ss << "Made " << playerData->username << " no longer a server operator";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
