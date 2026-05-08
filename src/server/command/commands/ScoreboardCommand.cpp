#include "ScoreboardCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <sstream>

namespace mc {
namespace command {

void ScoreboardCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto scoreboardNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("scoreboard");
    scoreboardNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        scoreboardNode,
        support::makeMetadata(
            "Manages scoreboard objectives and players.",
            "/scoreboard <objectives|players|teams> ...",
            2,
            {},
            true));

    // /scoreboard objectives ...
    auto objectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("objectives");

    // /scoreboard objectives add <name> <criteria> [displayName]
    auto addObjectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto objNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "name",
        StringArgumentType::string());
    auto criteriaArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "criteria",
        StringArgumentType::string());
    criteriaArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return addObjective(ctx);
    });
    objNameArg->addChild(criteriaArg);
    addObjectivesNode->addChild(objNameArg);

    // /scoreboard objectives remove <name>
    auto removeObjectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "name",
        StringArgumentType::string());
    removeNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return removeObjective(ctx);
    });
    removeObjectivesNode->addChild(removeNameArg);

    // /scoreboard objectives list
    auto listObjectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listObjectivesNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return listObjectives(ctx);
    });

    objectivesNode->addChild(addObjectivesNode);
    objectivesNode->addChild(removeObjectivesNode);
    objectivesNode->addChild(listObjectivesNode);
    scoreboardNode->addChild(objectivesNode);

    // /scoreboard players ...
    auto playersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("players");

    // /scoreboard players set <target> <objective> <score>
    auto setPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "target",
        StringArgumentType::string());
    auto setObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective",
        StringArgumentType::string());
    auto scoreArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "score",
        IntegerArgumentType::integer());
    scoreArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setScore(ctx);
    });
    setObjectiveArg->addChild(scoreArg);
    setTargetArg->addChild(setObjectiveArg);
    setPlayersNode->addChild(setTargetArg);

    // /scoreboard players add <target> <objective> <score>
    auto addPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "target",
        StringArgumentType::string());
    auto addObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective",
        StringArgumentType::string());
    auto addScoreArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "score",
        IntegerArgumentType::integer());
    addScoreArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return addScore(ctx);
    });
    addObjectiveArg->addChild(addScoreArg);
    addTargetArg->addChild(addObjectiveArg);
    addPlayersNode->addChild(addTargetArg);

    // /scoreboard players remove <target> <objective> <score>
    auto removePlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "target",
        StringArgumentType::string());
    auto removeObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective",
        StringArgumentType::string());
    auto removeScoreArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "score",
        IntegerArgumentType::integer());
    removeScoreArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return removeScore(ctx);
    });
    removeObjectiveArg->addChild(removeScoreArg);
    removeTargetArg->addChild(removeObjectiveArg);
    removePlayersNode->addChild(removeTargetArg);

    // /scoreboard players reset <target> [objective]
    auto resetPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reset");
    auto resetTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "target",
        StringArgumentType::string());
    resetTargetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return resetScore(ctx);
    });
    resetPlayersNode->addChild(resetTargetArg);

    // /scoreboard players get <target> <objective>
    auto getPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");
    auto getTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "target",
        StringArgumentType::string());
    auto getObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective",
        StringArgumentType::string());
    getObjectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return getScore(ctx);
    });
    getTargetArg->addChild(getObjectiveArg);
    getPlayersNode->addChild(getTargetArg);

    playersNode->addChild(setPlayersNode);
    playersNode->addChild(addPlayersNode);
    playersNode->addChild(removePlayersNode);
    playersNode->addChild(resetPlayersNode);
    playersNode->addChild(getPlayersNode);
    scoreboardNode->addChild(playersNode);

    dispatcher.registerCommand(scoreboardNode);
}

i32 ScoreboardCommand::addObjective(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("name");
    const std::string criteria = context.getArgument<std::string>("criteria");

    std::ostringstream ss;
    ss << "Created new scoreboard objective '" << name << "' with criteria '" << criteria << "'";
    source.sendMessage(ss.str());

    // TODO: 实现记分板系统
    // 1. 创建新的记分板目标
    // 2. 验证准则类型
    // 3. 注册到记分板管理器

    return 1;
}

i32 ScoreboardCommand::removeObjective(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("name");

    std::ostringstream ss;
    ss << "Removed scoreboard objective '" << name << "'";
    source.sendMessage(ss.str());

    // TODO: 实现记分板系统

    return 1;
}

i32 ScoreboardCommand::listObjectives(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // TODO: 实现记分板系统，列出所有目标
    source.sendMessage("Scoreboard objectives: (none)");

    return 1;
}

i32 ScoreboardCommand::setScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");
    const std::string objective = context.getArgument<std::string>("objective");
    const i32 score = context.getArgument<i32>("score");

    std::ostringstream ss;
    ss << "Set " << target << "'s score in '" << objective << "' to " << score;
    source.sendMessage(ss.str());

    // TODO: 实现记分板系统

    return 1;
}

i32 ScoreboardCommand::addScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");
    const std::string objective = context.getArgument<std::string>("objective");
    const i32 score = context.getArgument<i32>("score");

    std::ostringstream ss;
    ss << "Added " << score << " to " << target << "'s score in '" << objective << "'";
    source.sendMessage(ss.str());

    // TODO: 实现记分板系统

    return 1;
}

i32 ScoreboardCommand::removeScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");
    const std::string objective = context.getArgument<std::string>("objective");
    const i32 score = context.getArgument<i32>("score");

    std::ostringstream ss;
    ss << "Removed " << score << " from " << target << "'s score in '" << objective << "'";
    source.sendMessage(ss.str());

    // TODO: 实现记分板系统

    return 1;
}

i32 ScoreboardCommand::resetScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");

    std::ostringstream ss;
    ss << "Reset all scores for " << target;
    source.sendMessage(ss.str());

    // TODO: 实现记分板系统

    return 1;
}

i32 ScoreboardCommand::getScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");
    const std::string objective = context.getArgument<std::string>("objective");

    // TODO: 实现记分板系统，获取分数
    std::ostringstream ss;
    ss << target << " has 0 in '" << objective << "'";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
