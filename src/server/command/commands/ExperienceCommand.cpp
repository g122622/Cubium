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

#include "ExperienceCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace mc {
namespace command {

void ExperienceCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto experienceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("experience");
    experienceNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(experienceNode,
        support::makeMetadata(
            "Add, set, or query player experience.", "/experience <add|set|query> <player> ...", 2, {"xp"}, false));

    // /xp 是 /experience 的别名
    auto xpNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("xp");
    xpNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    xpNode->setRedirect(experienceNode);

    // /experience add <player> <amount> [points|levels]
    // 注意：amount 参数允许负值（原版 MC 支持）
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::players());
    auto amountArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("amount",
        IntegerArgumentType::integer() // 移除最小值限制，允许负值
    );
    auto pointsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("points");
    pointsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return addPoints(ctx); });
    auto levelsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("levels");
    levelsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return addLevels(ctx); });
    amountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return addPoints(ctx); });
    amountArg->addChild(pointsNode);
    amountArg->addChild(levelsNode);
    playerArg->addChild(amountArg);
    addNode->addChild(playerArg);

    // /experience set <player> <amount> [points|levels]
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setPlayerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::players());
    auto setAmountArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("amount", IntegerArgumentType::integer(0));
    auto setPointsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("points");
    setPointsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return setPoints(ctx); });
    auto setLevelsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("levels");
    setLevelsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return setLevels(ctx); });
    setAmountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return setPoints(ctx); });
    setAmountArg->addChild(setPointsNode);
    setAmountArg->addChild(setLevelsNode);
    setPlayerArg->addChild(setAmountArg);
    setNode->addChild(setPlayerArg);

    // /experience query <player> [points|levels]
    auto queryNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("query");
    auto queryPlayerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::player());
    auto queryPointsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("points");
    queryPointsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return queryPoints(ctx); });
    auto queryLevelsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("levels");
    queryLevelsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return queryLevels(ctx); });
    queryPlayerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return queryLevels(ctx); });
    queryPlayerArg->addChild(queryPointsNode);
    queryPlayerArg->addChild(queryLevelsNode);
    queryNode->addChild(queryPlayerArg);

    experienceNode->addChild(addNode);
    experienceNode->addChild(setNode);
    experienceNode->addChild(queryNode);

    dispatcher.registerCommand(experienceNode);
    dispatcher.registerCommand(xpNode);
}

namespace {

/**
 * @brief 获取玩家的 Player 实体
 *
 * 通过 ServerPlayerEntityManager 获取目标玩家的实体对象。
 */
Player* getTargetPlayer(ServerCommandSource& source, PlayerId playerId)
{
    auto* server = source.server();
    if (server == nullptr) {
        return nullptr;
    }

    auto* world = source.world();
    if (world == nullptr) {
        return nullptr;
    }

    return server->playerEntityManager().getPlayerEntity(playerId, *world);
}

/**
 * @brief 解析目标玩家并返回第一个玩家ID
 */
PlayerId resolveFirstPlayer(ServerCommandSource& source, const EntitySelector& selector)
{
    std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        return 0;
    }
    return playerIds.front();
}

/**
 * @brief 解析目标玩家集合，返回完整 PlayerId 列表（多目标）。
 *
 * add/set 子命令的 player 参数是 players()（多目标），须遍历全部选中玩家执行操作，
 * 而非仅取第一个。原先 add/set 复用 resolveFirstPlayer 致 /xp add @a 5 levels 只给第一个
 * 选中玩家加等级（对齐缺陷）。
 */
std::vector<PlayerId> resolveTargetPlayerIds(ServerCommandSource& source, const EntitySelector& selector)
{
    return support::resolvePlayerIds(source, selector);
}

/**
 * @brief 获取玩家名称用于命令反馈
 */
std::string getPlayerName(ServerCommandSource& source, PlayerId playerId, Player* player)
{
    if (source.server() != nullptr) {
        mc::server::core::PlayerManager& pm = source.server()->playerManager();
        mc::server::ServerPlayerData* playerData = pm.getPlayer(playerId);
        if (playerData != nullptr) {
            return playerData->username;
        }
    }
    if (player != nullptr) {
        return player->username();
    }
    return "unknown";
}

} // namespace

i32 ExperienceCommand::addPoints(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");
    i32 amount = context.getArgument<i32>("amount");

    // 解析目标玩家集合（多目标，players() 参数）。
    const std::vector<PlayerId> playerIds = resolveTargetPlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("commands.experience.add.failed.noPlayer");
        return 0;
    }

    i32 successCount = 0;
    for (const PlayerId playerId : playerIds) {
        if (playerId == 0) {
            continue;
        }
        Player* player = getTargetPlayer(source, playerId);
        if (player == nullptr) {
            continue;
        }
        // 添加经验
        player->addExperience(amount);
        ++successCount;

        std::ostringstream ss;
        ss << "Gave " << amount << " experience points to " << getPlayerName(source, playerId, player);
        source.sendMessage(ss.str());
    }

    if (successCount == 0) {
        source.sendError("commands.experience.add.failed.noPlayer");
        return 0;
    }

    return amount;
}

i32 ExperienceCommand::addLevels(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");
    i32 amount = context.getArgument<i32>("amount");

    // 解析目标玩家集合（多目标，players() 参数）。
    const std::vector<PlayerId> playerIds = resolveTargetPlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("commands.experience.add.failed.noPlayer");
        return 0;
    }

    i32 successCount = 0;
    for (const PlayerId playerId : playerIds) {
        if (playerId == 0) {
            continue;
        }
        Player* player = getTargetPlayer(source, playerId);
        if (player == nullptr) {
            continue;
        }
        // 添加等级
        player->addExperienceLevels(amount);
        ++successCount;

        std::ostringstream ss;
        ss << "Gave " << amount << " levels to " << getPlayerName(source, playerId, player);
        source.sendMessage(ss.str());
    }

    if (successCount == 0) {
        source.sendError("commands.experience.add.failed.noPlayer");
        return 0;
    }

    return amount;
}

i32 ExperienceCommand::setPoints(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");
    i32 amount = context.getArgument<i32>("amount");

    // 解析目标玩家集合（多目标，players() 参数）。
    const std::vector<PlayerId> playerIds = resolveTargetPlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("commands.experience.set.failed.noPlayer");
        return 0;
    }

    i32 successCount = 0;
    for (const PlayerId playerId : playerIds) {
        if (playerId == 0) {
            continue;
        }
        Player* player = getTargetPlayer(source, playerId);
        if (player == nullptr) {
            continue;
        }
        // 设置经验点数（重置后添加）
        player->setExperience(0, 0.0f, 0);
        player->addExperience(amount);
        ++successCount;

        std::ostringstream ss;
        ss << "Set " << getPlayerName(source, playerId, player) << "'s experience to " << amount << " points";
        source.sendMessage(ss.str());
    }

    if (successCount == 0) {
        source.sendError("commands.experience.set.failed.noPlayer");
        return 0;
    }

    return amount;
}

i32 ExperienceCommand::setLevels(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");
    i32 amount = context.getArgument<i32>("amount");

    // 解析目标玩家集合（多目标，players() 参数）。
    const std::vector<PlayerId> playerIds = resolveTargetPlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("commands.experience.set.failed.noPlayer");
        return 0;
    }

    i32 successCount = 0;
    for (const PlayerId playerId : playerIds) {
        if (playerId == 0) {
            continue;
        }
        Player* player = getTargetPlayer(source, playerId);
        if (player == nullptr) {
            continue;
        }
        // 设置等级
        player->setExperienceLevel(amount);
        ++successCount;

        std::ostringstream ss;
        ss << "Set " << getPlayerName(source, playerId, player) << "'s level to " << amount;
        source.sendMessage(ss.str());
    }

    if (successCount == 0) {
        source.sendError("commands.experience.set.failed.noPlayer");
        return 0;
    }

    return amount;
}

i32 ExperienceCommand::queryPoints(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 解析目标玩家
    PlayerId playerId = resolveFirstPlayer(source, selector);
    if (playerId == 0) {
        source.sendError("commands.experience.query.failed.noPlayer");
        return 0;
    }

    // 获取 Player 实体
    Player* player = getTargetPlayer(source, playerId);
    if (player == nullptr) {
        source.sendError("commands.experience.query.failed.noPlayer");
        return 0;
    }

    // 查询经验点数
    i32 totalXp = player->totalExperience();

    std::ostringstream ss;
    ss << getPlayerName(source, playerId, player) << " has " << totalXp << " experience points";
    source.sendMessage(ss.str());

    return totalXp;
}

i32 ExperienceCommand::queryLevels(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 解析目标玩家
    PlayerId playerId = resolveFirstPlayer(source, selector);
    if (playerId == 0) {
        source.sendError("commands.experience.query.failed.noPlayer");
        return 0;
    }

    // 获取 Player 实体
    Player* player = getTargetPlayer(source, playerId);
    if (player == nullptr) {
        source.sendError("commands.experience.query.failed.noPlayer");
        return 0;
    }

    // 查询等级
    i32 level = player->experienceLevel();

    std::ostringstream ss;
    ss << getPlayerName(source, playerId, player) << " has " << level << " experience levels";
    source.sendMessage(ss.str());

    return level;
}

} // namespace command
} // namespace mc
