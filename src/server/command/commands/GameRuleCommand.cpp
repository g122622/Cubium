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
#include <string_view>
#include <vector>
#include <fmt/format.h>

namespace mc::command {

void GameRuleCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    // /gamerule <rule> - 查询规则值
    auto queryNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("gamerule");
    queryNode->setRequirement([](const ServerCommandSource& source) {
        // 需要 OP 权限等级 2 或更高
        return source.hasPermission(2);
    });

    // 原版把每条规则注册成独立的字面量节点（`/gamerule keepInventory`），其下：
    //   boolean 规则 → value:bool      （查询）
    //   integer 规则 → value:integer   （查询）
    //   value(value)                   （设置：字面量 value + 值参数）
    // 规则名清单与类型来自 GameRules 注册表。
    // 每条规则注册两份：短名（`keep_inventory`）与带命名空间的完整名
    // （`minecraft:keep_inventory`）——原版的注册表键两种写法都能解析出来。
    const auto addRuleNode = [&queryNode](const std::string& literal, const std::string& ruleName, bool isBoolean) {
        auto ruleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(literal);
        ruleNode->setCommand([ruleName](CommandContext<ServerCommandSource>& ctx) {
            return _executeQueryRule(ctx, ruleName);
        });

        // 原版结构（GameRuleCommand.buildRuleArguments）：规则字面量节点自身执行查询，
        // 其下只有一个 value 参数节点执行设置。此前这里在 value 之下又挂了一个同名 value
        // 参数节点，结果 `/gamerule <rule> false` 的布尔值被外层 value 先吃掉并按查询执行，
        // 设置分支永远不触发。
        if (isBoolean) {
            auto valueArg =
                std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("value", BoolArgumentType::boolArg());
            valueArg->setCommand([ruleName](CommandContext<ServerCommandSource>& ctx) {
                return _executeSetRule(ctx, ruleName);
            });
            ruleNode->addChild(valueArg);
        } else {
            // 整数规则在原版里都带 [min, max] 约束，命令树据此注册带边界的 integer 参数
            // （客户端用它做补全与校验，网络负载里也会写出边界字节）。此前统一用无界
            // integer()，命令树与客户端拿到的序列化参数都与原版不一致。
            const auto [minValue, maxValue] = world::gamerule::GameRules::getIntRange(ruleName);
            auto valueArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
                "value", IntegerArgumentType::integer(minValue, maxValue));
            valueArg->setCommand([ruleName](CommandContext<ServerCommandSource>& ctx) {
                return _executeSetRule(ctx, ruleName);
            });
            ruleNode->addChild(valueArg);
        }
        queryNode->addChild(ruleNode);
    };

    for (const std::string& ruleName : world::gamerule::GameRules::getRuleNames()) {
        const auto ruleClass = world::gamerule::GameRules::getRuleClass(ruleName);
        if (!ruleClass) {
            continue;
        }
        const bool isBoolean = *ruleClass == world::gamerule::GameRuleClassNames::BOOLEAN;
        // max_minecart_speed 在原版里受 FeatureFlag 门控，默认世界（DEDICATED、不开实验性
        // 数据包）的命令树里没有它——规则值照常保留，只是不注册命令节点。
        if (ruleName == "max_minecart_speed") {
            continue;
        }
        addRuleNode(ruleName, ruleName, isBoolean);
        addRuleNode("minecraft:" + ruleName, ruleName, isBoolean);
    }

    dispatcher.registerCommand(queryNode);
}

i32 GameRuleCommand::_executeQueryRule(CommandContext<ServerCommandSource>& context, const std::string& ruleName)
{
    auto& source = context.getSource();
    server::ServerWorld* world = source.world();
    if (!world) {
        source.sendError("No world available");
        return 0;
    }

    const auto& gameRules = world->getGameRules();
    const std::string valueStr = gameRules.getValueAsString(ruleName);

    // vanilla getValueMessage 仅在值非默认时追加 "(default: ...)"
    const std::string defaultStr = gameRules.getDefaultValueAsString(ruleName);
    std::string message = valueStr;
    if (valueStr != defaultStr) {
        message += fmt::format(" (default: {})", defaultStr);
    }

    source.sendMessage("Game rule " + ruleName + " is currently set to: " + message);
    return 1;
}

i32 GameRuleCommand::_executeSetRule(CommandContext<ServerCommandSource>& context, const std::string& ruleName)
{
    auto& source = context.getSource();
    server::ServerWorld* world = source.world();
    if (!world) {
        source.sendError("No world available");
        return 0;
    }

    auto& gameRules = world->getGameRules();
    const auto ruleClass = world::gamerule::GameRules::getRuleClass(ruleName);
    if (!ruleClass) {
        source.sendError("Can't find a game rule named " + ruleName);
        return 0;
    }

    // 值参数的类型与规则类别一致（注册时已按类别区分），这里按类型取值再走统一的字符串写入。
    std::string valueStr;
    if (*ruleClass == world::gamerule::GameRuleClassNames::BOOLEAN) {
        valueStr = context.getArgument<bool>("value") ? "true" : "false";
    } else {
        valueStr = std::to_string(context.getArgument<i32>("value"));
    }

    if (!gameRules.setTypedFromString(ruleName, std::string(*ruleClass), valueStr, nullptr)) {
        source.sendError("Invalid value for game rule " + ruleName + ": " + valueStr);
        return 0;
    }

    const std::string displayValue = gameRules.getValueAsString(ruleName);
    source.sendMessage("Game rule " + ruleName + " has been updated to " + displayValue);
    spdlog::info("Game rule '{}' changed to '{}' by {}", ruleName, displayValue, source.name());
    return 1;
}

std::vector<std::string> GameRuleCommand::_getAllRuleNames()
{
    return world::gamerule::GameRules::getRuleNames();
}

} // namespace mc::command
