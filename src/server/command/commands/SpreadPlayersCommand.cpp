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
#include "common/scoreboard/core/ScorePlayerTeam.hpp"
#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/command/support/SpreadAlgorithm.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/world/ServerWorld.hpp"

#include <iomanip>
#include <set>
#include <sstream>
#include <unordered_map>

namespace mc {
namespace command {

namespace {

/// 计算需要分散的位置数量（尊重队伍时，按队伍数计算；否则按实体数计算）
/// TODO: MC Java 版中非玩家实体会被统一归入 null 队伍，当前实现仅支持玩家，
///       当支持 EntityArgumentType::entities() 后需要区分玩家和非玩家的队伍归属。
i32 getNumberOfTeams(server::IServer& server, const std::vector<std::string>& playerNames)
{
    // 收集不同的队伍（nullptr 算作一支独立的"无队伍"）
    std::set<scoreboard::ScorePlayerTeam*> teams;
    auto& scoreboard = server.scoreboard();

    for (const auto& name : playerNames) {
        scoreboard::ScorePlayerTeam* team = scoreboard.getPlayersTeam(name);
        teams.insert(team); // nullptr 也会被插入，但只算一个
    }

    return static_cast<i32>(teams.size());
}

/// 将分散后的位置应用到玩家/实体
/// 返回所有玩家到最近分散点的最小距离的平均值
/// TODO: MC Java 版传送时保留实体的 Y 旋转和 X 旋转，当前仅传送位置
/// TODO: MC Java 版使用 Vec2ArgumentType 解析中心坐标（仅 x, z），当前使用 Vec3ArgumentType
///       多解析了一个无用的 y 分量，需要创建 Vec2ArgumentType 或适配解析
/// TODO: MC Java 版支持 under <maxHeight> 子命令变体，允许指定最大高度，
///       当前使用硬编码的 world::MAX_BUILD_HEIGHT，需要添加该变体
/// TODO: MC Java 版使用 world.getMaxY() + 1 获取动态最大高度，而非硬编码常量，
///       且在提供 maxHeight 时验证其不小于 world.getMinY()，当前缺少此验证
f64 setPlayerPositions(server::IServer& server,
    server::ServerWorld& world,
    const std::vector<PlayerId>& playerIds,
    const std::vector<std::string>& playerNames,
    std::vector<support::SpreadPosition>& positions,
    i32 maxHeight,
    bool respectTeams)
{
    f64 totalMinDist = 0.0;
    i32 positionIndex = 0;

    // 队伍 -> 分散位置的映射（当 respectTeams=true 时使用）
    std::unordered_map<scoreboard::ScorePlayerTeam*, support::SpreadPosition*> teamPositionMap;
    auto& scoreboard = server.scoreboard();

    for (size_t i = 0; i < playerIds.size(); ++i) {
        support::SpreadPosition* targetPos = nullptr;

        if (respectTeams) {
            scoreboard::ScorePlayerTeam* team = scoreboard.getPlayersTeam(playerNames[i]);
            auto it = teamPositionMap.find(team);
            if (it == teamPositionMap.end()) {
                // 此队伍首次出现，分配下一个分散位置
                MC_ASSERT_RELEASE(positionIndex < static_cast<i32>(positions.size()));
                teamPositionMap[team] = &positions[static_cast<size_t>(positionIndex)];
                ++positionIndex;
            }
            targetPos = teamPositionMap[team];
        } else {
            MC_ASSERT_RELEASE(positionIndex < static_cast<i32>(positions.size()));
            targetPos = &positions[static_cast<size_t>(positionIndex)];
            ++positionIndex;
        }

        // 计算生成 Y 坐标
        i32 spawnY = targetPos->getSpawnY(world, maxHeight);

        // 传送到分散位置（中心对齐到方块）
        f64 targetX = std::floor(targetPos->x) + 0.5;
        f64 targetZ = std::floor(targetPos->z) + 0.5;

        server.teleportManager().requestTeleport(playerIds[i], targetX, static_cast<f64>(spawnY), targetZ);

        // 计算此玩家到其他分散位置的最小距离
        f64 closestDist = std::numeric_limits<f64>::max();
        for (auto& pos : positions) {
            if (targetPos != &pos) {
                f64 d = targetPos->dist(pos);
                closestDist = std::min(closestDist, d);
            }
        }

        totalMinDist += closestDist;
    }

    return playerIds.size() < 2 ? 0.0 : totalMinDist / static_cast<f64>(playerIds.size());
}

} // namespace

// ============================================================================
// 命令注册
// ============================================================================

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

// ============================================================================
// 命令执行
// ============================================================================

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

    // 解析目标玩家
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched the selector");
        return 0;
    }

    // 获取世界
    auto* world = source.world();
    if (world == nullptr) {
        source.sendError("World not found");
        return 0;
    }

    // 解析玩家名称（用于队伍查询）
    std::vector<std::string> playerNames;
    playerNames.reserve(playerIds.size());
    for (const PlayerId playerId : playerIds) {
        playerNames.push_back(support::resolvePlayerName(source, playerId));
    }

    // 计算最大高度
    // TODO: 应使用 world->getMaxY() + 1 而非硬编码常量，以支持不同维度的最大高度
    const i32 maxHeight = world::MAX_BUILD_HEIGHT;

    // 计算需要分散的位置数量
    const i32 positionCount =
        respectTeams ? getNumberOfTeams(*server, playerNames) : static_cast<i32>(playerIds.size());

    if (positionCount == 0) {
        source.sendError("No positions to spread");
        return 0;
    }

    // 计算分散区域边界
    const f64 minX = center.x - static_cast<f64>(maxRange);
    const f64 minZ = center.z - static_cast<f64>(maxRange);
    const f64 maxX = center.x + static_cast<f64>(maxRange);
    const f64 maxZ = center.z + static_cast<f64>(maxRange);

    // 创建随机数生成器
    math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));

    // 创建初始随机位置
    auto positions = support::createInitialPositions(rng, positionCount, minX, minZ, maxX, maxZ);

    // 执行迭代分散算法
    bool success = support::spreadPositions(
        static_cast<f64>(spreadDistance), *world, rng, minX, minZ, maxX, maxZ, maxHeight, positions);

    if (!success) {
        // 分散失败：无法在给定参数下满足最小距离要求
        std::ostringstream ss;
        ss << "Could not spread " << (respectTeams ? "teams" : "entities") << " around (" << static_cast<i32>(center.x)
           << ", " << static_cast<i32>(center.z) << ") - positions are too crowded for spread distance "
           << spreadDistance;
        source.sendError(ss.str());
        return 0;
    }

    // 将分散位置应用到玩家
    f64 avgMinDist = setPlayerPositions(*server, *world, playerIds, playerNames, positions, maxHeight, respectTeams);

    // 构建成功反馈消息
    std::ostringstream ss;
    ss << "Spread " << positionCount << " " << (respectTeams ? "team" : "player") << (positionCount != 1 ? "s" : "")
       << " around (" << static_cast<i32>(center.x) << ", " << static_cast<i32>(center.z) << ")"
       << " with average distance " << std::fixed << std::setprecision(2) << avgMinDist;
    source.sendMessage(ss.str());

    return positionCount;
}

} // namespace command
} // namespace mc
