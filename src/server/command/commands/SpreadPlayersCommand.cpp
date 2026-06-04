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

#include "SpreadPlayersCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/world/ServerWorld.hpp"

#include <sstream>

namespace mc {
namespace command {

void SpreadPlayersCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto spreadNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("spreadplayers");
    spreadNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(spreadNode,
        support::makeMetadata("Spreads players to random locations within an area.",
            "/spreadplayers <center> <spreadDistance> <maxRange> <respectTeams> <targets>",
            2,
            {},
            true));

    auto centerArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>("center", Vec3ArgumentType::vec3());

    auto spreadDistanceArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
        "spreadDistance", FloatArgumentType::floatArg(0.0f));

    auto maxRangeArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("maxRange", FloatArgumentType::floatArg(1.0f));

    auto respectTeamsArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("respectTeams", BoolArgumentType::boolArg());

    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::players());
    targetsArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _spreadPlayers(ctx); });

    respectTeamsArg->addChild(targetsArg);
    maxRangeArg->addChild(respectTeamsArg);
    spreadDistanceArg->addChild(maxRangeArg);
    centerArg->addChild(spreadDistanceArg);
    spreadNode->addChild(centerArg);

    dispatcher.registerCommand(spreadNode);
}

i32 SpreadPlayersCommand::_spreadPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    // 解析命令参数
    const Vector3d& center = context.getArgument<Vector3d>("center");
    const f32 spreadDistance = context.getArgument<f32>("spreadDistance");
    const f32 maxRange = context.getArgument<f32>("maxRange");
    const bool respectTeams = context.getArgument<bool>("respectTeams");
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    // TODO: 实现 spreadDistance 参数 - 确保玩家之间的最小距离
    // TODO: 实现 respectTeams 参数 - 同队玩家应分散到相近位置
    MC_UNUSED(spreadDistance);
    MC_UNUSED(respectTeams);

    // 解析目标玩家
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched the selector");
        return 0;
    }

    // 获取世界
    auto world = source.world();
    if (!world) {
        source.sendError("World not found");
        return 0;
    }

    // 计算分散区域边界
    const f32 minX = static_cast<f32>(center.x) - maxRange;
    const f32 maxX = static_cast<f32>(center.x) + maxRange;
    const f32 minZ = static_cast<f32>(center.z) - maxRange;
    const f32 maxZ = static_cast<f32>(center.z) + maxRange;

    // 使用当前时间戳作为随机种子
    math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));

    i32 successCount = 0;
    for (const PlayerId playerId : playerIds) {
        auto* player = server->playerManager().getPlayer(playerId);
        if (player == nullptr) {
            continue;
        }

        // 随机选择目标位置
        const f32 x = rng.nextFloat(minX, maxX);
        const f32 z = rng.nextFloat(minZ, maxZ);

        // 使用世界高度查找获取地面高度
        const i32 y = world->getHeight(static_cast<BlockCoord>(x), static_cast<BlockCoord>(z));

        // 通过传送管理器请求传送（会通知客户端）
        if (server->teleportManager().requestTeleport(playerId, x, static_cast<f64>(y), z) != 0) {
            ++successCount;
        }
    }

    // 构建反馈消息
    std::ostringstream ss;
    ss << "Spread " << successCount << " player(s) around (" << static_cast<i32>(center.x) << ", "
       << static_cast<i32>(center.z) << ")"
       << " with max range " << maxRange;
    source.sendMessage(ss.str());

    return successCount;
}

} // namespace command
} // namespace mc
