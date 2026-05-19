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

#include "KillCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <sstream>

namespace mc {
namespace command {

void KillCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto killNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("kill");
    killNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(
        killNode, support::makeMetadata("Kill entities (players, mobs, etc.).", "/kill [<target>]", 2, {}, false));

    // /kill - 杀死自己
    killNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return killSelf(ctx); });

    // /kill <target> - 杀死目标实体
    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entities());
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return killEntities(ctx); });
    killNode->addChild(targetArg);

    dispatcher.registerCommand(killNode);
}

i32 KillCommand::killSelf(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 检查命令源是否是实体（玩家）
    if (!source.isPlayer()) {
        source.sendError("commands.kill.failed.notEntity");
        return 0;
    }

    ServerPlayer* player = source.player();
    if (player == nullptr) {
        source.sendError("commands.kill.failed.notEntity");
        return 0;
    }

    // MC 1.16.5: 调用 entity.onKillCommand()
    player->onKillCommand();

    std::ostringstream ss;
    ss << "Killed " << player->username();
    source.sendMessage(ss.str());

    return 1;
}

i32 KillCommand::killEntities(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("target");

    // 获取目标玩家 ID 列表
    std::vector<PlayerId> targetPlayerIds = support::resolvePlayerIds(source, selector);

    if (targetPlayerIds.empty()) {
        source.sendError("commands.kill.failed.noEntity");
        return 0;
    }

    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.kill.failed.noEntity");
        return 0;
    }

    auto* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.kill.failed.noEntity");
        return 0;
    }

    i32 killedCount = 0;

    for (PlayerId playerId : targetPlayerIds) {
        if (playerId == 0) {
            continue;
        }

        // 通过 ServerPlayerEntityManager 获取玩家实体
        Player* player = server->playerEntityManager().getPlayerEntity(playerId, *world);
        if (player == nullptr) {
            continue;
        }

        // MC 1.16.5: 调用 entity.onKillCommand()
        player->onKillCommand();
        killedCount++;
    }

    // 发送反馈消息
    if (killedCount == 1) {
        // 获取第一个被杀死的玩家名称
        auto* playerData = server->playerManager().getPlayer(targetPlayerIds.front());
        if (playerData != nullptr) {
            std::ostringstream ss;
            ss << "Killed " << playerData->username;
            source.sendMessage(ss.str());
        } else {
            std::ostringstream ss;
            ss << "Killed " << killedCount << " entities";
            source.sendMessage(ss.str());
        }
    } else {
        std::ostringstream ss;
        ss << "Killed " << killedCount << " entities";
        source.sendMessage(ss.str());
    }

    return killedCount;
}

} // namespace command
} // namespace mc
