#include "ExperienceCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/application/IServer.hpp"

#include <cmath>
#include <sstream>

namespace mc {
namespace command {

void ExperienceCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mc::command;

    auto experienceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("experience");
    experienceNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        experienceNode,
        support::makeMetadata(
            "Add, set, or query player experience.",
            "/experience <add|set|query> <player> ...",
            2,
            {"xp"},
            false));

    // /xp 是 /experience 的别名
    auto xpNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("xp");
    xpNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    xpNode->setRedirect(experienceNode);

    // /experience add <player> <amount> [points|levels]
    // 注意：amount 参数允许负值（原版 MC 支持）
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::players()
    );
    auto amountArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "amount",
        IntegerArgumentType::integer()  // 移除最小值限制，允许负值
    );
    auto pointsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("points");
    pointsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return addPoints(ctx);
    });
    auto levelsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("levels");
    levelsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return addLevels(ctx);
    });
    amountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return addPoints(ctx);
    });
    amountArg->addChild(pointsNode);
    amountArg->addChild(levelsNode);
    playerArg->addChild(amountArg);
    addNode->addChild(playerArg);

    // /experience set <player> <amount> [points|levels]
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setPlayerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::players()
    );
    auto setAmountArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "amount",
        IntegerArgumentType::integer(0)
    );
    auto setPointsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("points");
    setPointsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setPoints(ctx);
    });
    auto setLevelsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("levels");
    setLevelsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setLevels(ctx);
    });
    setAmountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setPoints(ctx);
    });
    setAmountArg->addChild(setPointsNode);
    setAmountArg->addChild(setLevelsNode);
    setPlayerArg->addChild(setAmountArg);
    setNode->addChild(setPlayerArg);

    // /experience query <player> [points|levels]
    auto queryNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("query");
    auto queryPlayerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
    );
    auto queryPointsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("points");
    queryPointsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return queryPoints(ctx);
    });
    auto queryLevelsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("levels");
    queryLevelsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return queryLevels(ctx);
    });
    queryPlayerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return queryLevels(ctx);
    });
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
 * @brief 获取玩家的 ServerPlayer 实例
 *
 * 当前项目架构中，ServerPlayerData 存储玩家数据，
 * 但 ServerPlayer 实例可能不在 PlayerManager 中直接可用。
 *
 * 当前实现：通过 source.player() 获取执行者的 ServerPlayer。
 * TODO: 当 PlayerManager 支持获取 ServerPlayer 实例时，应改用目标玩家。
 */
ServerPlayer* getTargetPlayer(ServerCommandSource& source, PlayerId playerId)
{
    // 当前简化实现：只支持对命令执行者操作
    // 完整实现需要 PlayerManager::getServerPlayer(PlayerId)
    return source.player();
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
 * @brief 获取玩家名称用于命令反馈
 */
std::string getPlayerName(ServerCommandSource& source, PlayerId playerId, ServerPlayer* player)
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

i32 ExperienceCommand::addPoints(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");
    i32 amount = context.getArgument<i32>("amount");

    // 解析目标玩家
    PlayerId playerId = resolveFirstPlayer(source, selector);
    if (playerId == 0) {
        source.sendError("commands.experience.add.failed.noPlayer");
        return 0;
    }

    // 获取 ServerPlayer 实例
    ServerPlayer* player = getTargetPlayer(source, playerId);
    if (player == nullptr) {
        source.sendError("commands.experience.add.failed.noPlayer");
        return 0;
    }

    // 添加经验
    player->addExperience(amount);

    std::ostringstream ss;
    ss << "Gave " << amount << " experience points to " << getPlayerName(source, playerId, player);
    source.sendMessage(ss.str());

    return amount;
}

i32 ExperienceCommand::addLevels(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");
    i32 amount = context.getArgument<i32>("amount");

    // 解析目标玩家
    PlayerId playerId = resolveFirstPlayer(source, selector);
    if (playerId == 0) {
        source.sendError("commands.experience.add.failed.noPlayer");
        return 0;
    }

    // 获取 ServerPlayer 实例
    ServerPlayer* player = getTargetPlayer(source, playerId);
    if (player == nullptr) {
        source.sendError("commands.experience.add.failed.noPlayer");
        return 0;
    }

    // 添加等级
    player->addExperienceLevels(amount);

    std::ostringstream ss;
    ss << "Gave " << amount << " levels to " << getPlayerName(source, playerId, player);
    source.sendMessage(ss.str());

    return amount;
}

i32 ExperienceCommand::setPoints(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");
    i32 amount = context.getArgument<i32>("amount");

    // 解析目标玩家
    PlayerId playerId = resolveFirstPlayer(source, selector);
    if (playerId == 0) {
        source.sendError("commands.experience.set.failed.noPlayer");
        return 0;
    }

    // 获取 ServerPlayer 实例
    ServerPlayer* player = getTargetPlayer(source, playerId);
    if (player == nullptr) {
        source.sendError("commands.experience.set.failed.noPlayer");
        return 0;
    }

    // 设置经验点数（重置后添加）
    player->setExperience(0, 0.0f, 0);
    player->addExperience(amount);

    std::ostringstream ss;
    ss << "Set " << getPlayerName(source, playerId, player) << "'s experience to " << amount << " points";
    source.sendMessage(ss.str());

    return amount;
}

i32 ExperienceCommand::setLevels(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");
    i32 amount = context.getArgument<i32>("amount");

    // 解析目标玩家
    PlayerId playerId = resolveFirstPlayer(source, selector);
    if (playerId == 0) {
        source.sendError("commands.experience.set.failed.noPlayer");
        return 0;
    }

    // 获取 ServerPlayer 实例
    ServerPlayer* player = getTargetPlayer(source, playerId);
    if (player == nullptr) {
        source.sendError("commands.experience.set.failed.noPlayer");
        return 0;
    }

    // 设置等级
    player->setExperienceLevel(amount);

    std::ostringstream ss;
    ss << "Set " << getPlayerName(source, playerId, player) << "'s level to " << amount;
    source.sendMessage(ss.str());

    return amount;
}

i32 ExperienceCommand::queryPoints(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 解析目标玩家
    PlayerId playerId = resolveFirstPlayer(source, selector);
    if (playerId == 0) {
        source.sendError("commands.experience.query.failed.noPlayer");
        return 0;
    }

    // 获取 ServerPlayer 实例
    ServerPlayer* player = getTargetPlayer(source, playerId);
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

i32 ExperienceCommand::queryLevels(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 解析目标玩家
    PlayerId playerId = resolveFirstPlayer(source, selector);
    if (playerId == 0) {
        source.sendError("commands.experience.query.failed.noPlayer");
        return 0;
    }

    // 获取 ServerPlayer 实例
    ServerPlayer* player = getTargetPlayer(source, playerId);
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
