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
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/core/Types.hpp"
#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/scoreboard/criteria/TriggerCriteria.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <set>
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
    scoreboardNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(scoreboardNode,
        support::makeMetadata(
            "Manages scoreboard objectives and players.", "/scoreboard <objectives|players|teams> ...", 2, {}, true));

    // /scoreboard objectives ...
    auto objectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("objectives");

    // /scoreboard objectives add <name> <criteria> [displayName]
    auto addObjectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto objNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    auto criteriaArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "criteria", StringArgumentType::string());
    criteriaArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _addObjective(ctx);
    });
    objNameArg->addChild(criteriaArg);
    auto displayNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ComponentTextPtr>>(
        "displayName", ComponentArgumentType::component());
    displayNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _addObjective(ctx);
    });
    criteriaArg->addChild(displayNameArg);
    addObjectivesNode->addChild(objNameArg);

    // /scoreboard objectives remove <name>
    auto removeObjectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    removeNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _removeObjective(ctx);
    });
    removeObjectivesNode->addChild(removeNameArg);

    // /scoreboard objectives modify <objective> displayName <displayName>
    auto modifyObjectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("modify");
    auto modifyNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    auto displayNameNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("displayName");
    auto modifyDisplayNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ComponentTextPtr>>(
        "displayName", ComponentArgumentType::component());
    modifyDisplayNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _modifyObjectiveDisplayName(ctx);
    });
    displayNameNode->addChild(modifyDisplayNameArg);
    modifyNameArg->addChild(displayNameNode);
    modifyObjectivesNode->addChild(modifyNameArg);
    objectivesNode->addChild(modifyObjectivesNode);

    // /scoreboard objectives setdisplay <slot> [objective]
    auto setdisplayNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("setdisplay");
    auto setdisplaySlotArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "slot", DisplaySlotArgumentType::displaySlot());
    setdisplaySlotArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setDisplaySlot(ctx);
    });
    auto setdisplayObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    setdisplayObjectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setDisplaySlot(ctx);
    });
    setdisplaySlotArg->addChild(setdisplayObjectiveArg);
    setdisplayNode->addChild(setdisplaySlotArg);
    objectivesNode->addChild(setdisplayNode);

    // /scoreboard objectives list
    auto listObjectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listObjectivesNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _listObjectives(ctx);
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
        "targets", ScoreHolderArgumentType::scoreHolder());
    auto setObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    auto scoreArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("score", IntegerArgumentType::integer());
    scoreArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setScore(ctx);
    });
    setObjectiveArg->addChild(scoreArg);
    setTargetArg->addChild(setObjectiveArg);
    setPlayersNode->addChild(setTargetArg);

    // /scoreboard players add <target> <objective> <score>
    auto addPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targets", ScoreHolderArgumentType::scoreHolder());
    auto addObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    auto addScoreArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("score", IntegerArgumentType::integer());
    addScoreArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _addScore(ctx);
    });
    addObjectiveArg->addChild(addScoreArg);
    addTargetArg->addChild(addObjectiveArg);
    addPlayersNode->addChild(addTargetArg);

    // /scoreboard players remove <target> <objective> <score>
    auto removePlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targets", ScoreHolderArgumentType::scoreHolder());
    auto removeObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    auto removeScoreArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("score", IntegerArgumentType::integer());
    removeScoreArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _removeScore(ctx);
    });
    removeObjectiveArg->addChild(removeScoreArg);
    removeTargetArg->addChild(removeObjectiveArg);
    removePlayersNode->addChild(removeTargetArg);

    // /scoreboard players display <targets> <objective>
    auto displayPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("display");
    auto displayTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targets", ScoreHolderArgumentType::scoreHolder());
    auto displayObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    displayObjectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _displayPlayerScore(ctx);
    });
    displayTargetArg->addChild(displayObjectiveArg);
    displayPlayersNode->addChild(displayTargetArg);

    // /scoreboard players operation <targets> <objective> <op> <source> <sourceObjective>
    auto operationPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("operation");
    auto operationTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targets", ScoreHolderArgumentType::scoreHolder());
    auto operationObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    auto operationArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "operation", ScoreOperationArgumentType::operation());
    auto operationSourceArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "source", ScoreHolderArgumentType::scoreHolder());
    auto operationSourceObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "sourceObjective", ObjectiveNameArgumentType::objective());
    operationSourceObjectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _operateScore(ctx);
    });
    operationSourceArg->addChild(operationSourceObjectiveArg);
    operationArg->addChild(operationSourceArg);
    operationObjectiveArg->addChild(operationArg);
    operationTargetArg->addChild(operationObjectiveArg);
    operationPlayersNode->addChild(operationTargetArg);

    // /scoreboard players reset <target> [objective]
    auto resetPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reset");
    auto resetTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targets", ScoreHolderArgumentType::scoreHolder());
    resetTargetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _resetScore(ctx);
    });
    resetPlayersNode->addChild(resetTargetArg);

    // /scoreboard players get <target> <objective>
    auto getPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");
    auto getTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targets", ScoreHolderArgumentType::scoreHolder());
    auto getObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    getObjectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getScore(ctx);
    });
    getTargetArg->addChild(getObjectiveArg);
    getPlayersNode->addChild(getTargetArg);

    // /scoreboard players enable <target> <objective>
    // 用于启用 trigger 类型目标，让玩家可以再次使用 /trigger 命令
    auto enablePlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("enable");
    auto enableTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targets", ScoreHolderArgumentType::scoreHolder());
    auto enableObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    enableObjectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _enableTrigger(ctx);
    });
    enableTargetArg->addChild(enableObjectiveArg);
    enablePlayersNode->addChild(enableTargetArg);

    // /scoreboard players list [target]
    auto listPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    auto listTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targets", ScoreHolderArgumentType::scoreHolder());
    listTargetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _listPlayers(ctx);
    });
    listPlayersNode->addChild(listTargetArg);
    listPlayersNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _listAllPlayers(ctx);
    });

    playersNode->addChild(setPlayersNode);
    playersNode->addChild(addPlayersNode);
    playersNode->addChild(removePlayersNode);
    playersNode->addChild(resetPlayersNode);
    playersNode->addChild(getPlayersNode);
    playersNode->addChild(enablePlayersNode);
    playersNode->addChild(listPlayersNode);
    playersNode->addChild(displayPlayersNode);
    playersNode->addChild(operationPlayersNode);
    scoreboardNode->addChild(playersNode);

    dispatcher.registerCommand(scoreboardNode);
}

i32 ScoreboardCommand::_addObjective(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    // 参数节点名与命令树里的登记名必须一致（objective/criteria），否则运行时取不到值。
    const std::string name = context.getArgument<std::string>("objective");
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

    // 创建目标（原版可带一个显示名；未给时用目标名）。
    std::string displayNameText = name;
    if (context.hasArgument("displayName")) {
        displayNameText = context.getArgument<ComponentTextPtr>("displayName")->text;
    }
    auto displayName = std::make_unique<text::StringTextComponent>(displayNameText);
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
    const std::string name = context.getArgument<std::string>("objective");

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
        if (i > 0) {
            ss << ", ";
        }
        ss << objectives[i]->getName();
    }
    source.sendMessage(ss.str());

    return 1;
}

i32 ScoreboardCommand::_setScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("targets");
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
    const std::string target = context.getArgument<std::string>("targets");
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
    const std::string target = context.getArgument<std::string>("targets");
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
    const std::string target = context.getArgument<std::string>("targets");

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
    const std::string target = context.getArgument<std::string>("targets");
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
    const std::string target = context.getArgument<std::string>("targets");
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
    const std::string target = context.getArgument<std::string>("targets");

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
        if (i > 0) {
            ss << ", ";
        }
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

i32 ScoreboardCommand::_modifyObjectiveDisplayName(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("objective");
    const std::string displayName = context.getArgument<ComponentTextPtr>("displayName")->text;

    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }
    auto* objective = scoreboard->getObjective(name);
    if (objective == nullptr) {
        source.sendMessage("Unknown objective '" + name + "'");
        return 0;
    }
    objective->setDisplayName(std::make_unique<text::StringTextComponent>(displayName));
    source.sendMessage("Set display name of objective '" + name + "' to '" + displayName + "'");
    return 1;
}

i32 ScoreboardCommand::_setDisplaySlot(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string slotName = context.getArgument<std::string>("slot");
    // 原版槽位名：list / sidebar / below_name，以及 sidebar.team.<color>。
    const scoreboard::DisplaySlot slot = DisplaySlotArgumentType::toDisplaySlot(slotName);

    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }
    if (!context.hasArgument("objective")) {
        // /scoreboard objectives setdisplay <slot>：清空该槽位。
        scoreboard->setObjectiveInDisplaySlot(slot, nullptr);
        source.sendMessage("Cleared display slot '" + slotName + "'");
        return 1;
    }
    auto* objective = scoreboard->getObjective(context.getArgument<std::string>("objective"));
    if (objective == nullptr) {
        source.sendMessage("Unknown objective '" + context.getArgument<std::string>("objective") + "'");
        return 0;
    }
    scoreboard->setObjectiveInDisplaySlot(slot, objective);
    source.sendMessage("Set display slot '" + slotName + "' to objective '" + objective->getName() + "'");
    return 1;
}

i32 ScoreboardCommand::_listAllPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }

    std::set<std::string> names;
    for (const auto& name : scoreboard->getTrackedPlayerNames()) {
        names.insert(name);
    }
    if (names.empty()) {
        source.sendMessage("There are no tracked players");
        return 0;
    }
    std::ostringstream ss;
    ss << "There are " << names.size() << " tracked entities:";
    for (const auto& name : names) {
        ss << " " << name;
    }
    source.sendMessage(ss.str());
    return static_cast<i32>(names.size());
}

i32 ScoreboardCommand::_displayPlayerScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string targets = context.getArgument<std::string>("targets");
    const std::string objectiveName = context.getArgument<std::string>("objective");
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }
    auto* objective = scoreboard->getObjective(objectiveName);
    if (objective == nullptr) {
        source.sendMessage("Unknown objective '" + objectiveName + "'");
        return 0;
    }
    auto* score = scoreboard->getScore(targets, *objective);
    if (score == nullptr) {
        source.sendMessage("Could not get score for " + targets + " on " + objectiveName);
        return 0;
    }
    source.sendMessage(
        "Set display of " + targets + " on " + objectiveName + " to " + std::to_string(score->getScorePoints()));
    return score->getScorePoints();
}

i32 ScoreboardCommand::_operateScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string targets = context.getArgument<std::string>("targets");
    const std::string objectiveName = context.getArgument<std::string>("objective");
    const std::string operation = context.getArgument<std::string>("operation");
    const std::string sourceName = context.getArgument<std::string>("source");
    const std::string sourceObjectiveName = context.getArgument<std::string>("sourceObjective");

    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendMessage("Scoreboard is not available");
        return 0;
    }
    auto* objective = scoreboard->getObjective(objectiveName);
    auto* sourceObjective = scoreboard->getObjective(sourceObjectiveName);
    if (objective == nullptr || sourceObjective == nullptr) {
        source.sendMessage("Unknown objective");
        return 0;
    }
    auto* targetScore = scoreboard->getScore(targets, *objective);
    auto* sourceScore = scoreboard->getScore(sourceName, *sourceObjective);
    if (targetScore == nullptr || sourceScore == nullptr) {
        source.sendMessage("Could not get score");
        return 0;
    }

    i32 result = targetScore->getScorePoints();
    const i32 rhs = sourceScore->getScorePoints();
    if (operation == "=") {
        result = rhs;
    } else if (operation == "+=") {
        result += rhs;
    } else if (operation == "-=") {
        result -= rhs;
    } else if (operation == "*=") {
        result *= rhs;
    } else if (operation == "/=" || operation == "%=") {
        if (rhs == 0) {
            source.sendError("Cannot divide by zero");
            return 0;
        }
        result = operation == "/=" ? result / rhs : result % rhs;
    } else if (operation == "<" || operation == ">") {
        result = operation == "<" ? std::min(result, rhs) : std::max(result, rhs);
    } else if (operation == "><") {
        std::swap(targetScore, sourceScore);
        source.sendMessage("Swapped " + targets + " and " + sourceName + " on " + objectiveName);
        return 1;
    }
    targetScore->setScorePoints(result);
    source.sendMessage("Set " + targets + " on " + objectiveName + " to " + std::to_string(result));
    return result;
}

} // namespace command
} // namespace mc
