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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SpreadPlayersCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/scoreboard/core/ScorePlayerTeam.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EntityResolver.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/command/support/SpreadAlgorithm.hpp"
#include "server/network/sync/player/TeleportManager.hpp"
#include "server/player/lifecycle/PlayerManager.hpp"
#include "server/player/runtime/ServerPlayerData.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

namespace {

/// 一个"待分散目标"：玩家与非玩家实体统一到同一抽象，
/// 使分散算法与传送逻辑不必再按类型分叉两次。
struct SpreadTarget {
    /// 实体指针（玩家时指向 Player，非玩家时指向该实体）。始终非空。
    Entity* entity = nullptr;
    /// 玩家的 PlayerId；非玩家实体为 0 且 isPlayer 为 false。
    PlayerId playerId = 0;
    /// 用于查队伍归属的名字：玩家用玩家名（与原版 `entity.getScoreboardName()` 对玩家返回名字一致），
    /// 非玩家实体用 UUID 字符串（与原版对非玩家实体返回 UUID 一致）。
    std::string scoreboardName;
    /// 是否为玩家（决定传送走玩家通道还是实体通道）。
    bool isPlayer = false;
};

/// 计算需要分散的位置数量（尊重队伍时，按队伍数计算；否则按目标数计算）
i32 _getNumberOfTeams(server::IServer& server, const std::vector<SpreadTarget>& targets)
{
    // 收集不同的队伍（nullptr 算作一支独立的"无队伍"）
    std::set<scoreboard::ScorePlayerTeam*> teams;
    auto& scoreboard = server.scoreboard();

    for (const auto& target : targets) {
        // 非玩家实体查不到队伍时 getPlayersTeam 返回 nullptr，
        // 与原版"无队伍实体统一归入 null 队伍"语义一致。
        scoreboard::ScorePlayerTeam* team = scoreboard.getPlayersTeam(target.scoreboardName);
        teams.insert(team); // nullptr 也会被插入，但只算一个
    }

    return static_cast<i32>(teams.size());
}

/// 将分散后的位置应用到玩家/实体
/// 返回所有目标到最近分散点的最小距离的平均值
f64 _setTargetPositions(server::IServer& server,
    server::ServerWorld& world,
    const std::vector<SpreadTarget>& targets,
    std::vector<support::SpreadPosition>& positions,
    i32 maxHeight,
    bool respectTeams)
{
    f64 totalMinDist = 0.0;
    i32 positionIndex = 0;

    // 队伍 -> 分散位置的映射（当 respectTeams=true 时使用）
    std::unordered_map<scoreboard::ScorePlayerTeam*, support::SpreadPosition*> teamPositionMap;
    auto& scoreboard = server.scoreboard();

    for (const SpreadTarget& target : targets) {
        support::SpreadPosition* targetPos = nullptr;

        if (respectTeams) {
            scoreboard::ScorePlayerTeam* team = scoreboard.getPlayersTeam(target.scoreboardName);
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

        // 保留目标当前的旋转角度（原版 setPlayerPositions 用 entity.getYRot()/getXRot()，
        // 即实体自身朝向）。
        f32 targetYaw = 0.0f;
        f32 targetPitch = 0.0f;

        if (target.isPlayer) {
            // 真实玩家优先读 ServerPlayerData（网络层权威朝向），SimulatedPlayer 不进
            // PlayerManager（getPlayer 返 nullptr），回退经 ServerPlayerEntityManager 取实体层
            // yaw/pitch。原先仅读 ServerPlayerData，SimulatedPlayer 朝向归零（与原版偏差）。
            auto* playerData = server.playerManager().getPlayer(target.playerId);
            if (playerData != nullptr) {
                targetYaw = playerData->yaw;
                targetPitch = playerData->pitch;
            } else if (auto* playerEntity = server.playerEntityManager().getPlayerEntity(target.playerId, world)) {
                targetYaw = playerEntity->yaw();
                targetPitch = playerEntity->pitch();
            }

            // 传送：真实玩家经 TeleportManager（改 ServerPlayerData + 发传送包，客户端回移动包后
            // 实体收敛）；requestTeleport 对 SimulatedPlayer 返 0，回退经 ServerPlayerEntityManager
            // 取实体直接 setPosition+setRotation 立即移动。同款回退范式见
            // TeleportCommand::teleportPlayers。
            if (server.teleportManager().requestTeleport(
                    target.playerId, targetX, static_cast<f64>(spawnY), targetZ, targetYaw, targetPitch) == 0) {
                mc::Player* playerEntity = server.playerEntityManager().getPlayerEntity(target.playerId, world);
                if (playerEntity != nullptr) {
                    playerEntity->setPosition(
                        static_cast<f32>(targetX), static_cast<f32>(spawnY), static_cast<f32>(targetZ));
                    playerEntity->setRotation(targetYaw, targetPitch);
                }
            }
        } else {
            // 非玩家实体：没有连接、没有传送包，直接改实体坐标与朝向，
            // 等价于原版 entity.teleportTo 立即移动实体的语义。
            MC_ASSERT_RELEASE(target.entity != nullptr);
            targetYaw = target.entity->yaw();
            targetPitch = target.entity->pitch();
            target.entity->setPosition(static_cast<f32>(targetX), static_cast<f32>(spawnY), static_cast<f32>(targetZ));
            target.entity->setRotation(targetYaw, targetPitch);
        }

        // 计算此目标到其他分散位置的最小距离
        f64 closestDist = std::numeric_limits<f64>::max();
        for (auto& pos : positions) {
            if (targetPos != &pos) {
                f64 d = targetPos->dist(pos);
                closestDist = std::min(closestDist, d);
            }
        }

        totalMinDist += closestDist;
    }

    return targets.size() < 2 ? 0.0 : totalMinDist / static_cast<f64>(targets.size());
}

/// 无 under 子命令的执行入口：使用世界默认最大高度
i32 spreadPlayersImpl(CommandContext<ServerCommandSource>& context, i32 maxHeight)
{
    auto& source = context.getSource();
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    // 解析命令参数（center 为水平面 x, z 坐标）
    const Vector2d center = Vec2ArgumentType::getVec2(context, "center", source);
    const f32 spreadDistance = context.getArgument<f32>("spreadDistance");
    const f32 maxRange = context.getArgument<f32>("maxRange");
    const bool respectTeams = context.getArgument<bool>("respectTeams");
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    // 解析目标：选择器用 entities() 语义，玩家与非玩家实体都要处理。
    // 原版 /spreadplayers 的 targets 就是实体集合，非玩家实体（盔甲架、动物等）同样会被分散。
    auto matchedEntities = support::EntityResolver::resolve(source, selector);
    if (matchedEntities.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    std::vector<SpreadTarget> targets;
    targets.reserve(matchedEntities.size());
    for (Entity* matched : matchedEntities) {
        SpreadTarget target;
        target.entity = matched;
        if (auto* playerEntity = dynamic_cast<mc::Player*>(matched)) {
            target.isPlayer = true;
            target.playerId = playerEntity->playerId();
            // 玩家的队伍归属按玩家名查（与原版 entity.getScoreboardName() 对玩家返回名字一致）。
            target.scoreboardName = support::resolvePlayerName(source, target.playerId);
        } else {
            // 非玩家实体的队伍归属按 UUID 字符串查（与原版对非玩家实体返回 UUID 一致）。
            target.scoreboardName = matched->uuid();
        }
        targets.push_back(std::move(target));
    }

    // 获取世界
    auto* world = source.world();
    if (world == nullptr) {
        source.sendError("World not found");
        return 0;
    }

    // 验证 maxHeight 不小于世界最低建筑高度
    const i32 minY = world->getMinBuildHeight();
    if (maxHeight < minY) {
        std::ostringstream ss;
        ss << "Invalid height " << maxHeight << ": must be at least " << minY;
        source.sendError(ss.str());
        return 0;
    }

    // 计算需要分散的位置数量
    const i32 positionCount = respectTeams ? _getNumberOfTeams(*server, targets) : static_cast<i32>(targets.size());

    if (positionCount == 0) {
        source.sendError("No positions to spread");
        return 0;
    }

    // 计算分散区域边界（center.y 语义为 z 坐标，与 MC 原版 Vec2 一致）
    const f64 minX = center.x - static_cast<f64>(maxRange);
    const f64 minZ = center.y - static_cast<f64>(maxRange);
    const f64 maxX = center.x + static_cast<f64>(maxRange);
    const f64 maxZ = center.y + static_cast<f64>(maxRange);

    // 创建随机数生成器
    math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));

    // 创建初始随机位置
    auto positions = support::createInitialPositions(rng, positionCount, minX, minZ, maxX, maxZ);

    // 执行迭代分散算法
    f64 algorithmMinDist = 0.0;
    bool spreadSuccess = support::spreadPositions(
        static_cast<f64>(spreadDistance), *world, rng, minX, minZ, maxX, maxZ, maxHeight, positions, algorithmMinDist);

    if (!spreadSuccess) {
        // 分散失败：无法在给定参数下满足最小距离要求
        std::ostringstream ss;
        ss << "Could not spread " << positionCount << " " << (respectTeams ? "teams" : "entities") << " around "
           << static_cast<i32>(center.x) << ", " << static_cast<i32>(center.y) << " - too many "
           << (respectTeams ? "teams" : "entities") << " for spread distance " << std::fixed << std::setprecision(2)
           << spreadDistance << " (max distance is " << std::fixed << std::setprecision(2) << algorithmMinDist << ")";
        source.sendError(ss.str());
        return 0;
    }

    // 将分散位置应用到目标（玩家与非玩家实体）
    f64 avgMinDist = _setTargetPositions(*server, *world, targets, positions, maxHeight, respectTeams);

    // 构建成功反馈消息
    std::ostringstream ss;
    ss << "Spread " << positionCount << " " << (respectTeams ? "team" : "entity") << (positionCount != 1 ? "s" : "")
       << " around (" << static_cast<i32>(center.x) << ", " << static_cast<i32>(center.y) << ")"
       << " with average distance " << std::fixed << std::setprecision(2) << avgMinDist;
    source.sendMessage(ss.str());

    return positionCount;
}

} // namespace

// ============================================================================
// 命令注册
// ============================================================================

void SpreadPlayersCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto spreadNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("spreadplayers");
    spreadNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(spreadNode,
        support::makeMetadata("Spreads players to random locations within an area.",
            "/spreadplayers <center> <spreadDistance> <maxRange> <respectTeams> <targets>",
            2,
            {},
            true));

    auto centerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "center", Vec2ArgumentType::vec2());

    auto spreadDistanceArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
        "spreadDistance", FloatArgumentType::floatArg(0.0f));

    auto maxRangeArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("maxRange", FloatArgumentType::floatArg(1.0f));

    // 路径 A: /spreadplayers <center> <spreadDistance> <maxRange> <respectTeams> <targets>
    //          使用世界默认最大高度 (getMaxBuildHeight())
    auto respectTeamsArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("respectTeams", BoolArgumentType::boolArg());

    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities());
    targetsArg->setCommand([](CommandContext<ServerCommandSource>& ctx) -> i32 {
        auto* world = ctx.getSource().world();
        i32 maxHeight = (world != nullptr) ? world->getMaxBuildHeight() : world::MAX_BUILD_HEIGHT;
        return spreadPlayersImpl(ctx, maxHeight);
    });

    respectTeamsArg->addChild(targetsArg);
    maxRangeArg->addChild(respectTeamsArg);

    // 路径 B: /spreadplayers <center> <spreadDistance> <maxRange> under <maxHeight> <respectTeams> <targets>
    //          用户指定最大高度
    auto underNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("under");

    auto maxHeightArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("maxHeight", IntegerArgumentType::integer());

    auto respectTeamsUnderArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("respectTeams", BoolArgumentType::boolArg());

    auto targetsUnderArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities());
    targetsUnderArg->setCommand([](CommandContext<ServerCommandSource>& ctx) -> i32 {
        i32 maxHeight = ctx.getArgument<i32>("maxHeight");
        return spreadPlayersImpl(ctx, maxHeight);
    });

    respectTeamsUnderArg->addChild(targetsUnderArg);
    maxHeightArg->addChild(respectTeamsUnderArg);
    underNode->addChild(maxHeightArg);
    maxRangeArg->addChild(underNode);

    spreadDistanceArg->addChild(maxRangeArg);
    centerArg->addChild(spreadDistanceArg);
    spreadNode->addChild(centerArg);

    dispatcher.registerCommand(spreadNode);
}

} // namespace command
} // namespace mc
