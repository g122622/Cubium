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
 * LIABILITY, IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
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
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/world/ServerWorld.hpp"

#include <cmath>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <unordered_map>

namespace mc {
namespace command {

// ============================================================================
// SpreadPosition - 2D 位置辅助结构
// ============================================================================
//
// 对应 MC 源码中 SpreadPlayersCommand.Position 的内部类。
// 表示一个 2D (x, z) 位置，用于迭代分散算法中的位置计算。
//
struct SpreadPosition {
    f64 x = 0.0;
    f64 z = 0.0;

    /// 计算与另一个位置的距离
    [[nodiscard]] f64 dist(const SpreadPosition& other) const
    {
        f64 dx = x - other.x;
        f64 dz = z - other.z;
        return std::sqrt(dx * dx + dz * dz);
    }

    /// 获取向量长度
    [[nodiscard]] f64 getLength() const { return std::sqrt(x * x + z * z); }

    /// 归一化向量
    void normalize()
    {
        f64 len = getLength();
        if (len > 0.0) {
            x /= len;
            z /= len;
        }
    }

    /// 沿指定方向的反方向移动（远离）
    void moveAway(const SpreadPosition& direction)
    {
        x -= direction.x;
        z -= direction.z;
    }

    /// 将位置钳制到指定矩形范围内，返回是否发生了钳制
    bool clamp(f64 minX, f64 minZ, f64 maxX, f64 maxZ)
    {
        bool clamped = false;
        if (x < minX) {
            x = minX;
            clamped = true;
        } else if (x > maxX) {
            x = maxX;
            clamped = true;
        }
        if (z < minZ) {
            z = minZ;
            clamped = true;
        } else if (z > maxZ) {
            z = maxZ;
            clamped = true;
        }
        return clamped;
    }

    /// 计算此位置的生成 Y 坐标（从上往下搜索第一个安全的站立位置）
    [[nodiscard]] i32 getSpawnY(server::ServerWorld& world, i32 maxHeight) const
    {
        // 从 maxHeight + 1 开始向下搜索，找到第一个"上方两格都是空气、脚下不是空气"的位置
        i32 blockX = static_cast<i32>(std::floor(x));
        i32 blockZ = static_cast<i32>(std::floor(z));

        // 从 maxHeight + 1 开始搜索
        i32 y = maxHeight + 1;
        const BlockState* state = world.getBlockState(blockX, y, blockZ);
        bool above = (state != nullptr) && !state->isAir();
        bool current = false;

        // 逐格向下搜索
        while (y > world::MIN_BUILD_HEIGHT) {
            --y;
            state = world.getBlockState(blockX, y, blockZ);
            current = (state != nullptr) && !state->isAir();

            // 找到脚下是固体、上方两格是空气的位置
            if (!current && above) {
                // 检查再上一格是否也是空气
                if (y + 2 <= maxHeight + 1) {
                    const BlockState* aboveState = world.getBlockState(blockX, y + 2, blockZ);
                    bool aboveTwo = (aboveState == nullptr) || aboveState->isAir();
                    if (aboveTwo) {
                        return y + 1;
                    }
                }
                // 如果无法检查上方两格，仍然返回当前位置
                return y + 1;
            }
            above = current;
        }

        // 如果找不到合适的位置，返回 maxHeight + 1
        return maxHeight + 1;
    }

    /// 检查此位置是否安全（不是液体、不是火焰）
    [[nodiscard]] bool isSafe(server::ServerWorld& world, i32 maxHeight) const
    {
        i32 blockX = static_cast<i32>(std::floor(x));
        i32 blockZ = static_cast<i32>(std::floor(z));
        i32 spawnY = getSpawnY(world, maxHeight);

        // 脚下方块
        const BlockState* belowState = world.getBlockState(blockX, spawnY - 1, blockZ);
        if (belowState == nullptr) {
            return false;
        }

        // 检查是否是液体
        if (belowState->isLiquid()) {
            return false;
        }

        // 检查是否是火焰方块
        if (BlockTags::FIRE().contains(*belowState)) {
            return false;
        }

        return spawnY < maxHeight;
    }

    /// 在指定范围内随机初始化位置
    void randomize(math::Random& rng, f64 minX, f64 minZ, f64 maxX, f64 maxZ)
    {
        x = rng.nextDouble(minX, maxX);
        z = rng.nextDouble(minZ, maxZ);
    }
};

// ============================================================================
// 最大迭代次数
// ============================================================================
static constexpr i32 MAX_ITERATION_COUNT = 10000;

// ============================================================================
// 辅助函数
// ============================================================================

/// 计算需要分散的位置数量（尊重队伍时，按队伍数计算；否则按实体数计算）
static i32 getNumberOfTeams(server::IServer& server, const std::vector<std::string>& playerNames)
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

/// 创建初始随机位置
static std::vector<SpreadPosition> createInitialPositions(
    math::Random& rng, i32 count, f64 minX, f64 minZ, f64 maxX, f64 maxZ)
{
    std::vector<SpreadPosition> positions(static_cast<size_t>(count));
    for (auto& pos : positions) {
        pos.randomize(rng, minX, minZ, maxX, maxZ);
    }
    return positions;
}

/// 迭代分散算法：将位置推开到满足最小距离要求
/// 对应 MC 源码 SpreadPlayersCommand.spreadPositions
static bool spreadPositions(f64 spreadDistance,
    server::ServerWorld& world,
    math::Random& rng,
    f64 minX,
    f64 minZ,
    f64 maxX,
    f64 maxZ,
    i32 maxHeight,
    std::vector<SpreadPosition>& positions)
{
    bool moved = true;
    f64 minDist = std::numeric_limits<f64>::max();

    i32 iteration = 0;
    for (; iteration < MAX_ITERATION_COUNT && moved; ++iteration) {
        moved = false;
        minDist = std::numeric_limits<f64>::max();

        for (size_t j = 0; j < positions.size(); ++j) {
            i32 closeCount = 0;
            SpreadPosition delta;

            for (size_t l = 0; l < positions.size(); ++l) {
                if (j == l) {
                    continue;
                }

                f64 d = positions[j].dist(positions[l]);
                minDist = std::min(minDist, d);

                if (d < spreadDistance) {
                    ++closeCount;
                    delta.x += (positions[l].x - positions[j].x);
                    delta.z += (positions[l].z - positions[j].z);
                }
            }

            if (closeCount > 0) {
                delta.x /= static_cast<f64>(closeCount);
                delta.z /= static_cast<f64>(closeCount);

                f64 len = delta.getLength();
                if (len > 0.0) {
                    delta.normalize();
                    positions[j].moveAway(delta);
                } else {
                    positions[j].randomize(rng, minX, minZ, maxX, maxZ);
                }

                moved = true;
            }

            if (positions[j].clamp(minX, minZ, maxX, maxZ)) {
                moved = true;
            }
        }

        // 如果所有位置都已满足距离要求，检查安全性
        if (!moved) {
            for (auto& pos : positions) {
                if (!pos.isSafe(world, maxHeight)) {
                    pos.randomize(rng, minX, minZ, maxX, maxZ);
                    moved = true;
                }
            }
        }
    }

    if (minDist == std::numeric_limits<f64>::max()) {
        minDist = 0.0;
    }

    // 如果超过最大迭代次数，分散失败
    if (iteration >= MAX_ITERATION_COUNT) {
        return false;
    }

    return true;
}

/// 将分散后的位置应用到玩家/实体
/// 对应 MC 源码 SpreadPlayersCommand.setPlayerPositions
/// 返回所有玩家到最近分散点的最小距离的平均值
static f64 setPlayerPositions(server::IServer& server,
    server::ServerWorld& world,
    const std::vector<PlayerId>& playerIds,
    const std::vector<std::string>& playerNames,
    std::vector<SpreadPosition>& positions,
    i32 maxHeight,
    bool respectTeams)
{
    f64 totalMinDist = 0.0;
    i32 positionIndex = 0;

    // 队伍 -> 分散位置的映射（当 respectTeams=true 时使用）
    std::unordered_map<scoreboard::ScorePlayerTeam*, SpreadPosition*> teamPositionMap;
    auto& scoreboard = server.scoreboard();

    for (size_t i = 0; i < playerIds.size(); ++i) {
        SpreadPosition* targetPos = nullptr;

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

    // 计算最大高度（对应 MC 的 getLevel().getMaxY() + 1）
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
    auto positions = createInitialPositions(rng, positionCount, minX, minZ, maxX, maxZ);

    // 执行迭代分散算法
    bool success =
        spreadPositions(static_cast<f64>(spreadDistance), *world, rng, minX, minZ, maxX, maxZ, maxHeight, positions);

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
