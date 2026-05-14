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
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <sstream>

namespace mc {
namespace command {

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
    objectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return trigger(ctx); });

    // /trigger <objective> add <value>
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addValueArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("value", IntegerArgumentType::integer());
    addValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return triggerAdd(ctx); });
    addNode->addChild(addValueArg);
    objectiveArg->addChild(addNode);

    // /trigger <objective> set <value>
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setValueArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("value", IntegerArgumentType::integer());
    setValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return triggerSet(ctx); });
    setNode->addChild(setValueArg);
    objectiveArg->addChild(setNode);

    triggerNode->addChild(objectiveArg);
    dispatcher.registerCommand(triggerNode);
}

i32 TriggerCommand::triggerAdd(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string objective = context.getArgument<std::string>("objective");
    const i32 value = context.getArgument<i32>("value");

    if (!source.isPlayer()) {
        source.sendError("Only players can use this command");
        return 0;
    }

    std::ostringstream ss;
    ss << "Trigger '" << objective << "' added " << value;
    source.sendMessage(ss.str());

    // TODO: 实现触发器记分板系统
    // 1. 检查目标是否为 trigger 类型
    // 2. 检查玩家是否已启用该触发器
    // 3. 增加玩家的分数
    // 4. 禁用触发器

    return 1;
}

i32 TriggerCommand::triggerSet(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string objective = context.getArgument<std::string>("objective");
    const i32 value = context.getArgument<i32>("value");

    if (!source.isPlayer()) {
        source.sendError("Only players can use this command");
        return 0;
    }

    std::ostringstream ss;
    ss << "Trigger '" << objective << "' set to " << value;
    source.sendMessage(ss.str());

    // TODO: 实现触发器记分板系统
    // 1. 检查目标是否为 trigger 类型
    // 2. 检查玩家是否已启用该触发器
    // 3. 设置玩家的分数
    // 4. 禁用触发器

    return 1;
}

i32 TriggerCommand::trigger(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string objective = context.getArgument<std::string>("objective");

    if (!source.isPlayer()) {
        source.sendError("Only players can use this command");
        return 0;
    }

    std::ostringstream ss;
    ss << "Trigger '" << objective << "' activated";
    source.sendMessage(ss.str());

    // TODO: 实现触发器记分板系统
    // 1. 检查目标是否为 trigger 类型
    // 2. 检查玩家是否已启用该触发器
    // 3. 增加 1 分
    // 4. 禁用触发器

    return 1;
}

} // namespace command
} // namespace mc
