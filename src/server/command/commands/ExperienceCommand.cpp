#include "ExperienceCommand.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/player/ServerPlayer.hpp"
#include "common/entity/entities/player/Player.hpp"
#include <sstream>
#include <cmath>

namespace mc {
namespace command {

void ExperienceCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mc::command;

    // 主命令 /experience
    auto experienceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("experience");
    experienceNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });

    // 别名 /xp
    auto xpNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("xp");
    xpNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    xpNode->setRedirect(experienceNode);

    // ========== add 子命令 ==========
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");

    // 玩家参数
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
    );

    // 数量参数
    auto amountArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "amount",
        IntegerArgumentType::integer(0)
    );

    // points 子命令
    auto pointsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("points");
    pointsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return addPoints(ctx);
    });

    // levels 子命令
    auto levelsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("levels");
    levelsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return addLevels(ctx);
    });

    // 默认为 points（没有指定单位时）
    amountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return addPoints(ctx);
    });

    amountArg->addChild(pointsNode);
    amountArg->addChild(levelsNode);
    playerArg->addChild(amountArg);
    addNode->addChild(playerArg);

    // ========== set 子命令 ==========
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");

    auto setPlayerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
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

    // ========== query 子命令 ==========
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

    // 构建命令树
    experienceNode->addChild(addNode);
    experienceNode->addChild(setNode);
    experienceNode->addChild(queryNode);

    dispatcher.registerCommand(experienceNode);
    dispatcher.registerCommand(xpNode);
}

i32 ExperienceCommand::addPoints(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    EntitySelector selector = context.getArgument<EntitySelector>("player");
    i32 amount = context.getArgument<i32>("amount");

    // TODO: 解析选择器获取玩家
    // ServerPlayer* player = resolvePlayerSelector(selector, source);
    ServerPlayer* player = source.player();

    if (!player) {
        source.sendMessage("No player found");
        return 0;
    }

    player->addExperience(amount);

    std::ostringstream ss;
    ss << "Gave " << amount << " experience points to " << player->username();
    source.sendMessage(ss.str());

    return amount;
}

i32 ExperienceCommand::addLevels(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    EntitySelector selector = context.getArgument<EntitySelector>("player");
    i32 amount = context.getArgument<i32>("amount");

    ServerPlayer* player = source.player();

    if (!player) {
        source.sendMessage("No player found");
        return 0;
    }

    player->addExperienceLevels(amount);

    std::ostringstream ss;
    ss << "Gave " << amount << " levels to " << player->username();
    source.sendMessage(ss.str());

    return amount;
}

i32 ExperienceCommand::setPoints(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    EntitySelector selector = context.getArgument<EntitySelector>("player");
    i32 amount = context.getArgument<i32>("amount");

    ServerPlayer* player = source.player();

    if (!player) {
        source.sendMessage("No player found");
        return 0;
    }

    // 设置总经验
    i32 currentLevel = player->experienceLevel();
    player->setExperience(0, 0.0f, 0);
    player->addExperience(amount);

    std::ostringstream ss;
    ss << "Set " << player->username() << "'s experience to " << amount << " points";
    source.sendMessage(ss.str());

    return amount;
}

i32 ExperienceCommand::setLevels(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    EntitySelector selector = context.getArgument<EntitySelector>("player");
    i32 amount = context.getArgument<i32>("amount");

    ServerPlayer* player = source.player();

    if (!player) {
        source.sendMessage("No player found");
        return 0;
    }

    player->setExperienceLevel(amount);

    std::ostringstream ss;
    ss << "Set " << player->username() << "'s level to " << amount;
    source.sendMessage(ss.str());

    return amount;
}

i32 ExperienceCommand::queryPoints(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    EntitySelector selector = context.getArgument<EntitySelector>("player");

    ServerPlayer* player = source.player();

    if (!player) {
        source.sendMessage("No player found");
        return 0;
    }

    i32 totalXp = player->totalExperience();

    std::ostringstream ss;
    ss << player->username() << " has " << totalXp << " experience points";
    source.sendMessage(ss.str());

    return totalXp;
}

i32 ExperienceCommand::queryLevels(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    EntitySelector selector = context.getArgument<EntitySelector>("player");

    ServerPlayer* player = source.player();

    if (!player) {
        source.sendMessage("No player found");
        return 0;
    }

    i32 level = player->experienceLevel();

    std::ostringstream ss;
    ss << player->username() << " has " << level << " experience levels";
    source.sendMessage(ss.str());

    return level;
}

} // namespace command
} // namespace mc
