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

#include "ScoreboardCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/core/Types.hpp"
#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/scoreboard/criteria/TriggerCriteria.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace mc {
namespace command {

// 使用 mc::server 命名空间中的 ServerScoreboard
using ::mc::server::ServerScoreboard;

// 辅助函数：获取服务端记分板
static ServerScoreboard* getScoreboard(ServerCommandSource& source)
{
    auto* server = source.server();
    if (!server) {
        return nullptr;
    }
    return &server->scoreboard();
}

void ScoreboardCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto scoreboardNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("scoreboard");
    scoreboardNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(scoreboardNode,
        support::makeMetadata(
            "Manages scoreboard objectives and players.", "/scoreboard <objectives|players|teams> ...", 2, {}, true));

    // /scoreboard objectives ...
    auto objectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("objectives");

    // /scoreboard objectives add <name> <criteria> [displayName]
    auto addObjectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto objNameArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("name", StringArgumentType::string());
    auto criteriaArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "criteria", StringArgumentType::string());
    criteriaArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _addObjective(ctx); });
    objNameArg->addChild(criteriaArg);
    addObjectivesNode->addChild(objNameArg);

    // /scoreboard objectives remove <name>
    auto removeObjectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeNameArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("name", StringArgumentType::string());
    removeNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _removeObjective(ctx); });
    removeObjectivesNode->addChild(removeNameArg);

    // /scoreboard objectives list
    auto listObjectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listObjectivesNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _listObjectives(ctx); });

    objectivesNode->addChild(addObjectivesNode);
    objectivesNode->addChild(removeObjectivesNode);
    objectivesNode->addChild(listObjectivesNode);
    scoreboardNode->addChild(objectivesNode);

    // /scoreboard players ...
    auto playersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("players");

    // /scoreboard players set <target> <objective> <score>
    auto setPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setTargetArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("target", StringArgumentType::string());
    auto setObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", StringArgumentType::string());
    auto scoreArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("score", IntegerArgumentType::integer());
    scoreArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setScore(ctx); });
    setObjectiveArg->addChild(scoreArg);
    setTargetArg->addChild(setObjectiveArg);
    setPlayersNode->addChild(setTargetArg);

    // /scoreboard players add <target> <objective> <score>
    auto addPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addTargetArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("target", StringArgumentType::string());
    auto addObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", StringArgumentType::string());
    auto addScoreArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("score", IntegerArgumentType::integer());
    addScoreArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _addScore(ctx); });
    addObjectiveArg->addChild(addScoreArg);
    addTargetArg->addChild(addObjectiveArg);
    addPlayersNode->addChild(addTargetArg);

    // /scoreboard players remove <target> <objective> <score>
    auto removePlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeTargetArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("target", StringArgumentType::string());
    auto removeObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", StringArgumentType::string());
    auto removeScoreArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("score", IntegerArgumentType::integer());
    removeScoreArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _removeScore(ctx); });
    removeObjectiveArg->addChild(removeScoreArg);
    removeTargetArg->addChild(removeObjectiveArg);
    removePlayersNode->addChild(removeTargetArg);

    // /scoreboard players reset <target> [objective]
    auto resetPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reset");
    auto resetTargetArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("target", StringArgumentType::string());
    resetTargetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _resetScore(ctx); });
    resetPlayersNode->addChild(resetTargetArg);

    // /scoreboard players get <target> <objective>
    auto getPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");
    auto getTargetArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("target", StringArgumentType::string());
    auto getObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", StringArgumentType::string());
    getObjectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getScore(ctx); });
    getTargetArg->addChild(getObjectiveArg);
    getPlayersNode->addChild(getTargetArg);

    // /scoreboard players enable <target> <objective>
    // 用于启用 trigger 类型目标，让玩家可以再次使用 /trigger 命令
    auto enablePlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("enable");
    auto enableTargetArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("target", StringArgumentType::string());
    auto enableObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", StringArgumentType::string());
    enableObjectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _enableTrigger(ctx); });
    enableTargetArg->addChild(enableObjectiveArg);
    enablePlayersNode->addChild(enableTargetArg);

    // /scoreboard players list [target]
    auto listPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    auto listTargetArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("target", StringArgumentType::string());
    listTargetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _listPlayers(ctx); });
    listPlayersNode->addChild(listTargetArg);

    playersNode->addChild(setPlayersNode);
    playersNode->addChild(addPlayersNode);
    playersNode->addChild(removePlayersNode);
    playersNode->addChild(resetPlayersNode);
    playersNode->addChild(getPlayersNode);
    playersNode->addChild(enablePlayersNode);
    playersNode->addChild(listPlayersNode);
    scoreboardNode->addChild(playersNode);

    dispatcher.registerCommand(scoreboardNode);
}

i32 ScoreboardCommand::_addObjective(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("name");
    const std::string criteria = context.getArgument<std::string>("criteria");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }

    // 验证目标名称长度
    if (name.length() > scoreboard::ScoreObjective::MAX_NAME_LENGTH) {
        std::ostringstream ss;
        ss << "Objective name '" << name << "' is too long (max " << scoreboard::ScoreObjective::MAX_NAME_LENGTH
           << " characters)";
        source.sendMessage(ss.str());
        return 0;
    }

    // 检查目标是否已存在
    if (scoreboard->hasObjective(name)) {
        std::ostringstream ss;
        ss << "An objective with the name '" << name << "' already exists";
        source.sendMessage(ss.str());
        return 0;
    }

    // 获取判据
    auto& registry = scoreboard::ScoreCriteriaRegistry::instance();
    auto* criteriaObj = registry.getCriteria(criteria);
    if (!criteriaObj) {
        std::ostringstream ss;
        ss << "Unknown scoreboard criteria '" << criteria << "'";
        source.sendMessage(ss.str());
        return 0;
    }

    // 创建目标
    auto displayName = std::make_unique<text::StringTextComponent>(name);
    auto* objective = scoreboard->addObjective(name, *criteriaObj, std::move(displayName));
    if (!objective) {
        source.sendMessage("Failed to create objective");
        return 0;
    }

    std::ostringstream ss;
    ss << "Created new objective '" << name << "' with criteria '" << criteria << "'";
    source.sendMessage(ss.str());

    return 1;
}

i32 ScoreboardCommand::_removeObjective(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("name");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }

    // 获取目标
    auto* objective = scoreboard->getObjective(name);
    if (!objective) {
        std::ostringstream ss;
        ss << "Unknown objective '" << name << "'";
        source.sendMessage(ss.str());
        return 0;
    }

    // 移除目标
    scoreboard->removeObjective(*objective);

    std::ostringstream ss;
    ss << "Removed objective '" << name << "'";
    source.sendMessage(ss.str());

    return 1;
}

i32 ScoreboardCommand::_listObjectives(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }

    auto objectives = scoreboard->getObjectives();
    if (objectives.empty()) {
        source.sendMessage("There are no objectives");
        return 1;
    }

    std::ostringstream ss;
    ss << "There are " << objectives.size() << " objective(s): ";
    for (size_t i = 0; i < objectives.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << objectives[i]->getName();
    }
    source.sendMessage(ss.str());

    return 1;
}

i32 ScoreboardCommand::_setScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");
    const std::string objectiveName = context.getArgument<std::string>("objective");
    const i32 score = context.getArgument<i32>("score");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }

    // 获取目标
    auto* objective = scoreboard->getObjective(objectiveName);
    if (!objective) {
        std::ostringstream ss;
        ss << "Unknown objective '" << objectiveName << "'";
        source.sendMessage(ss.str());
        return 0;
    }

    // 检查判据是否只读
    if (objective->getCriteria().isReadOnly()) {
        std::ostringstream ss;
        ss << "Cannot set score for read-only criteria '" << objective->getCriteria().getName() << "'";
        source.sendMessage(ss.str());
        return 0;
    }

    // 设置分数
    auto* scoreObj = scoreboard->getOrCreateScore(target, *objective);
    if (!scoreObj) {
        source.sendMessage("Failed to create score");
        return 0;
    }
    scoreObj->setScorePoints(score);

    std::ostringstream ss;
    ss << "Set " << target << "'s score in '" << objectiveName << "' to " << score;
    source.sendMessage(ss.str());

    return 1;
}

i32 ScoreboardCommand::_addScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");
    const std::string objectiveName = context.getArgument<std::string>("objective");
    const i32 score = context.getArgument<i32>("score");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }

    // 获取目标
    auto* objective = scoreboard->getObjective(objectiveName);
    if (!objective) {
        std::ostringstream ss;
        ss << "Unknown objective '" << objectiveName << "'";
        source.sendMessage(ss.str());
        return 0;
    }

    // 检查判据是否只读
    if (objective->getCriteria().isReadOnly()) {
        std::ostringstream ss;
        ss << "Cannot modify score for read-only criteria '" << objective->getCriteria().getName() << "'";
        source.sendMessage(ss.str());
        return 0;
    }

    // 获取分数
    auto* scoreObj = scoreboard->getOrCreateScore(target, *objective);
    if (!scoreObj) {
        source.sendMessage("Failed to create score");
        return 0;
    }
    scoreObj->addScore(score);

    std::ostringstream ss;
    ss << "Added " << score << " to " << target << "'s score in '" << objectiveName << "' (now "
       << scoreObj->getScorePoints() << ")";
    source.sendMessage(ss.str());

    return 1;
}

i32 ScoreboardCommand::_removeScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");
    const std::string objectiveName = context.getArgument<std::string>("objective");
    const i32 score = context.getArgument<i32>("score");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }

    // 获取目标
    auto* objective = scoreboard->getObjective(objectiveName);
    if (!objective) {
        std::ostringstream ss;
        ss << "Unknown objective '" << objectiveName << "'";
        source.sendMessage(ss.str());
        return 0;
    }

    // 检查判据是否只读
    if (objective->getCriteria().isReadOnly()) {
        std::ostringstream ss;
        ss << "Cannot modify score for read-only criteria '" << objective->getCriteria().getName() << "'";
        source.sendMessage(ss.str());
        return 0;
    }

    // 获取分数
    auto* scoreObj = scoreboard->getOrCreateScore(target, *objective);
    if (!scoreObj) {
        source.sendMessage("Failed to create score");
        return 0;
    }
    scoreObj->subtractScore(score);

    std::ostringstream ss;
    ss << "Subtracted " << score << " from " << target << "'s score in '" << objectiveName << "' (now "
       << scoreObj->getScorePoints() << ")";
    source.sendMessage(ss.str());

    return 1;
}

i32 ScoreboardCommand::_resetScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }

    // 重置玩家的所有分数
    scoreboard->removeScore(target);

    std::ostringstream ss;
    ss << "Reset all scores for " << target;
    source.sendMessage(ss.str());

    return 1;
}

i32 ScoreboardCommand::_getScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");
    const std::string objectiveName = context.getArgument<std::string>("objective");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }

    // 获取目标
    auto* objective = scoreboard->getObjective(objectiveName);
    if (!objective) {
        std::ostringstream ss;
        ss << "Unknown objective '" << objectiveName << "'";
        source.sendMessage(ss.str());
        return 0;
    }

    // 获取分数
    auto* scoreObj = scoreboard->getScore(target, *objective);
    if (!scoreObj) {
        std::ostringstream ss;
        ss << target << " has no score in '" << objectiveName << "'";
        source.sendMessage(ss.str());
        return 0;
    }

    std::ostringstream ss;
    ss << target << " has " << scoreObj->getScorePoints() << " in '" << objectiveName << "'";
    source.sendMessage(ss.str());

    return 1;
}

i32 ScoreboardCommand::_enableTrigger(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");
    const std::string objectiveName = context.getArgument<std::string>("objective");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }

    // 获取目标
    auto* objective = scoreboard->getObjective(objectiveName);
    if (!objective) {
        std::ostringstream ss;
        ss << "Unknown objective '" << objectiveName << "'";
        source.sendMessage(ss.str());
        return 0;
    }

    // 检查判据是否为 trigger 类型
    auto& criteria = objective->getCriteria();
    if (criteria.getName() != scoreboard::TriggerCriteria::NAME) {
        std::ostringstream ss;
        ss << "Objective '" << objectiveName << "' is not a trigger objective";
        source.sendMessage(ss.str());
        return 0;
    }

    // 获取或创建分数（这会为玩家"准备"触发器）
    auto* score = scoreboard->getOrCreateScore(target, *objective);
    if (!score) {
        source.sendMessage("Failed to create score");
        return 0;
    }

    // 解锁触发器
    score->setLocked(false);

    std::ostringstream ss;
    ss << "Enabled trigger '" << objectiveName << "' for " << target;
    source.sendMessage(ss.str());

    return 1;
}

i32 ScoreboardCommand::_listPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }

    // 获取玩家的所有目标名称
    auto objectiveNames = scoreboard->getPlayerObjectives(target);
    if (objectiveNames.empty()) {
        std::ostringstream ss;
        ss << target << " has no scores recorded";
        source.sendMessage(ss.str());
        return 1;
    }

    std::ostringstream ss;
    ss << target << " has " << objectiveNames.size() << " score(s): ";
    for (size_t i = 0; i < objectiveNames.size(); ++i) {
        if (i > 0) ss << ", ";
        auto* objective = scoreboard->getObjective(objectiveNames[i]);
        auto* score = objective ? scoreboard->getScore(target, *objective) : nullptr;
        if (score) {
            ss << objectiveNames[i] << "=" << score->getScorePoints();
        } else {
            ss << objectiveNames[i];
        }
    }
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
