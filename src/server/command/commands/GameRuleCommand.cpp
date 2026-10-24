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

    // vanilla 规则名为字面量（GameRules.getAvailableRules() 进规则名补全），此处保持字符串参数；
    // 其下按规则值类名分派两个字面量子节点，各自再带一个字面量 "value" 子节点：
    //
    //   /gamerule <rule> boolean [value]   — 布尔规则：无 value=查询，有 value=设置
    //   /gamerule <rule> integer [value]   — 整型规则：无 value=查询，有 value=设置
    //
    // 类名与规则类型不匹配时，vanilla 在查询路径上报「no game rule named <rule>」，
    // 在设置路径上报「is of wrong type」，二者由下方 execute 区分。
    auto ruleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("rule", StringArgumentType::string());
    ruleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _executeQuery(ctx);
    });

    for (const std::string_view classType :
        {world::gamerule::GameRuleClassNames::BOOLEAN, world::gamerule::GameRuleClassNames::INTEGER}) {
        auto classArg = std::make_shared<LiteralCommandNode<ServerCommandSource>>(std::string(classType));
        classArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return _executeQuery(ctx);
        });

        auto valueArg = std::make_shared<LiteralCommandNode<ServerCommandSource>>("value");
        valueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return _executeSet(ctx);
        });

        classArg->addChild(valueArg);
        ruleArg->addChild(classArg);
    }

    queryNode->addChild(ruleArg);

    dispatcher.registerCommand(queryNode);
}

std::string GameRuleCommand::_requireValueClassName(const CommandContext<ServerCommandSource>& context)
{
    // 字面量节点不写入参数表，只能从当前节点（"boolean"/"integer"）取类名
    const auto node = context.getCurrentNode();
    if (!node) {
        throw CommandException(CommandErrorType::DispatcherUnknownArgument, "Missing game rule value type");
    }
    return node->getName();
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

    // 查询路径要求 类名 与 规则值的真实类名 一致，否则视为「不存在该规则」——
    // 例如 /gamerule randomTickSpeed boolean 查询一个整型规则。
    const std::string className = _requireValueClassName(context);
    const auto ruleClass = world::gamerule::GameRules::getRuleClass(ruleName);
    if (!ruleClass || *ruleClass != className) {
        source.sendError("Can't find a game rule named " + ruleName);
        return 0;
    }

    // 按规则名字符串查询当前值（GameRuleKey 通过名称判等，分类不参与取值）
    std::string valueStr;
    if (className == world::gamerule::GameRuleClassNames::BOOLEAN) {
        const world::gamerule::BooleanGameRuleKey key(ruleName, world::gamerule::GameRuleCategory::Misc);
        valueStr = gameRules.getBoolean(key) ? "true" : "false";
    } else {
        const world::gamerule::IntegerGameRuleKey key(ruleName, world::gamerule::GameRuleCategory::Misc);
        valueStr = std::to_string(gameRules.getInt(key));
    }

    // vanilla getValueMessage 仅在值非默认时追加 "(default: ...)"
    const std::string defaultStr = gameRules.getDefaultValueAsString(ruleName);
    if (valueStr != defaultStr) {
        valueStr += fmt::format(" (default: {})", defaultStr);
    }

    source.sendMessage("Game rule " + ruleName + " is currently set to: " + valueStr);
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

    // 设置路径要求 类名 与 规则值的真实类名 一致。
    // vanilla 在规则存在但类名不符时报「is of wrong type」，规则不存在则落入通用错误。
    const std::string className = _requireValueClassName(context);
    const auto ruleClass = world::gamerule::GameRules::getRuleClass(ruleName);
    if (!ruleClass) {
        source.sendError("Can't find a game rule named " + ruleName);
        return 0;
    }
    if (*ruleClass != className) {
        source.sendError("The game rule " + ruleName + " is of wrong type");
        return 0;
    }

    // 尝试设置规则值。ServerCommandSource 只持有 IServer*，而 GameRuleValue::set 的变更监听器
    // 形参是 MinecraftServer*；当前没有任何规则注册变更监听器，故此处不传 server。
    // TODO: 待 ServerCommandSource 暴露 MinecraftServer 入口（或 IServer 增加规则变更通知）后，
    // 在此传入 server，让依赖变更通知的规则（如同步给客户端）在 /gamerule 修改后立即生效。
    if (!gameRules.setTypedFromString(ruleName, className, valueStr, nullptr)) {
        source.sendError("Invalid value for game rule " + ruleName + ": " + valueStr);
        return 0;
    }

    // 回读实际生效值作为反馈，避免把解析失败的原始输入回显给玩家。
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
