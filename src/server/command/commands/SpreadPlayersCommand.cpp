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
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <chrono>
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
    targetsArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return spreadPlayers(ctx); });

    respectTeamsArg->addChild(targetsArg);
    maxRangeArg->addChild(respectTeamsArg);
    spreadDistanceArg->addChild(maxRangeArg);
    centerArg->addChild(spreadDistanceArg);
    spreadNode->addChild(centerArg);

    dispatcher.registerCommand(spreadNode);
}

i32 SpreadPlayersCommand::spreadPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const Vector3d& center = context.getArgument<Vector3d>("center");
    const f32 spreadDistance = context.getArgument<f32>("spreadDistance");
    const f32 maxRange = context.getArgument<f32>("maxRange");
    const bool respectTeams = context.getArgument<bool>("respectTeams");
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

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

    // 计算分散区域
    f32 minX = static_cast<f32>(center.x) - maxRange;
    f32 maxX = static_cast<f32>(center.x) + maxRange;
    f32 minZ = static_cast<f32>(center.z) - maxRange;
    f32 maxZ = static_cast<f32>(center.z) + maxRange;

    // 随机分散玩家
    math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));

    i32 successCount = 0;
    auto server = source.server();
    for (PlayerId playerId : playerIds) {
        auto player = server->playerManager().getPlayer(playerId);
        if (!player) continue;

        // 随机选择位置
        f32 x = rng.nextFloat(minX, maxX);
        f32 z = rng.nextFloat(minZ, maxZ);

        // 使用世界高度查找获取地面高度
        i32 y = world->getHeight(static_cast<BlockCoord>(x), static_cast<BlockCoord>(z));

        // 传送玩家
        player->x = x;
        player->y = static_cast<f32>(y);
        player->z = z;
        successCount++;
    }

    std::ostringstream ss;
    ss << "Spread " << successCount << " player(s) around (" << static_cast<i32>(center.x) << ", "
       << static_cast<i32>(center.z) << ")"
       << " with spread distance " << spreadDistance << " and max range " << maxRange;
    if (respectTeams) {
        ss << " (respecting teams)";
    }
    source.sendMessage(ss.str());

    return successCount;
}

} // namespace command
} // namespace mc
