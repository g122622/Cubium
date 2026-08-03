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

/**
 * @file GameRuleCommand.cpp
 * @brief /gamerule 命令实现
 */

#include "GameRuleCommand.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/core/Types.hpp"
#include "common/world/gamerule/GameRule.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/world/ServerWorld.hpp"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <cctype>
#include <memory>
#include <string>
#include <vector>

namespace mc::command {

void GameRuleCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    // /gamerule <rule> - 查询规则值
    auto queryNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("gamerule");
    queryNode->setRequirement([](const ServerCommandSource& source) {
        // 需要 OP 权限等级 2 或更高
        return source.hasPermission(2);
    });

    // /gamerule <rule> <value> - 设置规则值
    auto ruleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("rule", StringArgumentType::string());

    auto valueArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "value", StringArgumentType::greedyString());

    // 构建命令树
    // /gamerule <rule> -> 查询
    ruleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _executeQuery(ctx); });

    // /gamerule <rule> <value> -> 设置
    valueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _executeSet(ctx); });

    // 连接节点
    queryNode->addChild(ruleArg);
    ruleArg->addChild(valueArg);

    dispatcher.registerCommand(queryNode);
}

i32 GameRuleCommand::_executeQuery(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    std::string ruleName = context.getArgument<std::string>("rule");

    // 获取世界
    server::ServerWorld* world = source.world();
    if (!world) {
        source.sendError("No world available");
        return 0;
    }

    const auto& gameRules = world->getGameRules();

    // 检查规则是否存在
    if (!world::gamerule::GameRules::hasRule(ruleName)) {
        source.sendError("Unknown game rule: " + ruleName);
        return 0;
    }

    // 获取规则类型并返回值
    auto ruleType = world::gamerule::GameRules::getRuleType(ruleName);
    if (!ruleType) {
        source.sendError("Unknown game rule: " + ruleName);
        return 0;
    }

    std::string valueStr;
    if (*ruleType == world::gamerule::GameRuleValueType::Boolean) {
        // 创建临时键来获取值
        world::gamerule::BooleanGameRuleKey key(ruleName, world::gamerule::GameRuleCategory::Misc);
        bool value = gameRules.getBoolean(key);
        valueStr = value ? "true" : "false";
    } else {
        // 创建临时键来获取值
        world::gamerule::IntegerGameRuleKey key(ruleName, world::gamerule::GameRuleCategory::Misc);
        i32 value = gameRules.getInt(key);
        valueStr = std::to_string(value);
    }

    source.sendMessage("Game rule " + ruleName + " is currently: " + valueStr);
    return 1;
}

i32 GameRuleCommand::_executeSet(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    std::string ruleName = context.getArgument<std::string>("rule");
    std::string valueStr = context.getArgument<std::string>("value");

    // 获取世界
    server::ServerWorld* world = source.world();
    if (!world) {
        source.sendError("No world available");
        return 0;
    }

    auto& gameRules = world->getGameRules();

    // 检查规则是否存在
    if (!world::gamerule::GameRules::hasRule(ruleName)) {
        source.sendError("Unknown game rule: " + ruleName);
        return 0;
    }

    // 尝试设置规则值
    if (gameRules.setFromString(ruleName, valueStr, nullptr)) {
        // 获取设置后的值用于反馈
        auto ruleType = world::gamerule::GameRules::getRuleType(ruleName);
        std::string displayValue = valueStr;

        // 格式化显示值
        if (ruleType == world::gamerule::GameRuleValueType::Boolean) {
            // 转换为标准 true/false 格式
            std::transform(
                valueStr.begin(), valueStr.end(), valueStr.begin(), [](unsigned char c) { return std::tolower(c); });
            displayValue = (valueStr == "true" || valueStr == "1") ? "true" : "false";
        }

        source.sendMessage("Game rule " + ruleName + " has been updated to " + displayValue);
        spdlog::info("Game rule '{}' changed to '{}' by {}", ruleName, displayValue, source.name());
        return 1;
    } else {
        source.sendError("Invalid value for game rule " + ruleName + ": " + valueStr);
        return 0;
    }
}

std::vector<std::string> GameRuleCommand::_getAllRuleNames()
{
    return world::gamerule::GameRules::getRuleNames();
}

} // namespace mc::command
