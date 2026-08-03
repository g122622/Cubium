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

#include "TriggerCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/core/Types.hpp"
#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/scoreboard/criteria/TriggerCriteria.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include <memory>
#include <sstream>
#include <string>

namespace mc {
namespace command {

// ============================================================================
// 错误消息键
// ============================================================================

namespace {
// 错误消息键
constexpr const char* ERROR_NOT_PRIMED = "commands.trigger.failed.unprimed";
constexpr const char* ERROR_NOT_A_TRIGGER = "commands.trigger.failed.invalid";
} // namespace

// ============================================================================
// 辅助函数
// ============================================================================

namespace {

/**
 * @brief 检查目标是否为有效的触发器
 *
 * @param source 命令源
 * @param playerName 玩家名称
 * @param objective 目标
 * @return 分数对象指针，如果无效则返回 nullptr
 */
[[nodiscard]] scoreboard::Score* checkValidTrigger(
    ServerCommandSource& source, const std::string& playerName, scoreboard::ScoreObjective* objective)
{
    if (objective == nullptr) {
        return nullptr;
    }

    // 检查判据是否为 trigger 类型
    auto& criteria = objective->getCriteria();
    if (criteria.getName() != scoreboard::TriggerCriteria::NAME) {
        source.sendError("Objective '" + objective->getName() + "' is not a trigger objective");
        return nullptr;
    }

    // 获取记分板
    auto& server = *source.server();
    auto& scoreboard = server.scoreboard();

    // 检查玩家是否有该目标的分数
    if (!scoreboard.entityHasObjective(playerName, *objective)) {
        source.sendError("Player '" + playerName + "' has not been primed for trigger '" + objective->getName() + "'");
        return nullptr;
    }

    // 获取分数
    auto* score = scoreboard.getOrCreateScore(playerName, *objective);
    if (score == nullptr) {
        source.sendError("Failed to get or create score for player '" + playerName + "'");
        return nullptr;
    }

    // 检查分数是否被锁定
    if (score->isLocked()) {
        source.sendError("Trigger '" + objective->getName() + "' has already been used and is now locked");
        return nullptr;
    }

    return score;
}

/**
 * @brief 锁定触发器分数
 *
 * 触发器使用后需要锁定，管理员需要使用 /scoreboard players enable 重新启用。
 *
 * @param score 分数对象
 */
void lockTriggerScore(scoreboard::Score* score)
{
    if (score != nullptr) {
        score->setLocked(true);
    }
}

} // namespace

// ============================================================================
// 命令注册
// ============================================================================

void TriggerCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto triggerNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("trigger");
    // Trigger 命令权限为 0，所有玩家可用
    triggerNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(0); });
    support::applyMetadata(triggerNode,
        support::makeMetadata(
            "Sets a trigger to be activated.", "/trigger <objective> [add|set] [value]", 0, {}, true));

    // /trigger <objective>
    auto objectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", StringArgumentType::string());
    objectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _trigger(ctx); });

    // /trigger <objective> add <value>
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addValueArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("value", IntegerArgumentType::integer());
    addValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _triggerAdd(ctx); });
    addNode->addChild(addValueArg);
    objectiveArg->addChild(addNode);

    // /trigger <objective> set <value>
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setValueArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("value", IntegerArgumentType::integer());
    setValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _triggerSet(ctx); });
    setNode->addChild(setValueArg);
    objectiveArg->addChild(setNode);

    triggerNode->addChild(objectiveArg);
    dispatcher.registerCommand(triggerNode);
}

// ============================================================================
// 命令处理
// ============================================================================

i32 TriggerCommand::_triggerAdd(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string objectiveName = context.getArgument<std::string>("objective");
    const i32 value = context.getArgument<i32>("value");

    if (!source.isPlayer()) {
        source.sendError("Only players can use this command");
        return 0;
    }

    // 获取玩家名称
    const std::string playerName = source.name();

    // 获取服务器和记分板
    auto& server = *source.server();
    auto& scoreboard = server.scoreboard();

    // 获取目标
    auto* objective = scoreboard.getObjective(objectiveName);
    if (objective == nullptr) {
        source.sendError("Objective '" + objectiveName + "' does not exist");
        return 0;
    }

    // 检查是否为有效触发器
    auto* score = checkValidTrigger(source, playerName, objective);
    if (score == nullptr) {
        return 0;
    }

    // 增加分数
    score->addScore(value);

    // 锁定触发器
    lockTriggerScore(score);

    // 发送反馈
    std::ostringstream ss;
    ss << "Trigger '" << objectiveName << "' added " << value << " (now " << score->getScorePoints() << ")";
    source.sendMessage(ss.str());

    return score->getScorePoints();
}

i32 TriggerCommand::_triggerSet(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string objectiveName = context.getArgument<std::string>("objective");
    const i32 value = context.getArgument<i32>("value");

    if (!source.isPlayer()) {
        source.sendError("Only players can use this command");
        return 0;
    }

    // 获取玩家名称
    const std::string playerName = source.name();

    // 获取服务器和记分板
    auto& server = *source.server();
    auto& scoreboard = server.scoreboard();

    // 获取目标
    auto* objective = scoreboard.getObjective(objectiveName);
    if (objective == nullptr) {
        source.sendError("Objective '" + objectiveName + "' does not exist");
        return 0;
    }

    // 检查是否为有效触发器
    auto* score = checkValidTrigger(source, playerName, objective);
    if (score == nullptr) {
        return 0;
    }

    // 设置分数
    score->setScorePoints(value);

    // 锁定触发器
    lockTriggerScore(score);

    // 发送反馈
    std::ostringstream ss;
    ss << "Trigger '" << objectiveName << "' set to " << value;
    source.sendMessage(ss.str());

    return value;
}

i32 TriggerCommand::_trigger(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string objectiveName = context.getArgument<std::string>("objective");

    if (!source.isPlayer()) {
        source.sendError("Only players can use this command");
        return 0;
    }

    // 获取玩家名称
    const std::string playerName = source.name();

    // 获取服务器和记分板
    auto& server = *source.server();
    auto& scoreboard = server.scoreboard();

    // 获取目标
    auto* objective = scoreboard.getObjective(objectiveName);
    if (objective == nullptr) {
        source.sendError("Objective '" + objectiveName + "' does not exist");
        return 0;
    }

    // 检查是否为有效触发器
    auto* score = checkValidTrigger(source, playerName, objective);
    if (score == nullptr) {
        return 0;
    }

    // 增加 1 分
    score->incrementScore();

    // 锁定触发器
    lockTriggerScore(score);

    // 发送反馈
    std::ostringstream ss;
    ss << "Trigger '" << objectiveName << "' activated (now " << score->getScorePoints() << ")";
    source.sendMessage(ss.str());

    return score->getScorePoints();
}

} // namespace command
} // namespace mc
