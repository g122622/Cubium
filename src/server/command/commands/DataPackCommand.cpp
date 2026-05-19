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

#include "DataPackCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <sstream>

namespace mc {
namespace command {

void DataPackCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto datapackNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("datapack");
    datapackNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(datapackNode,
        support::makeMetadata("Controls data packs.", "/datapack <enable|disable|list> [name]", 2, {}, true));

    // /datapack enable <name>
    auto enableNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("enable");
    auto enableNameArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("name", StringArgumentType::string());
    enableNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return enableDataPack(ctx); });
    enableNode->addChild(enableNameArg);
    datapackNode->addChild(enableNode);

    // /datapack disable <name>
    auto disableNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("disable");
    auto disableNameArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("name", StringArgumentType::string());
    disableNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return disableDataPack(ctx); });
    disableNode->addChild(disableNameArg);
    datapackNode->addChild(disableNode);

    // /datapack list
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return listDataPacks(ctx); });
    datapackNode->addChild(listNode);

    dispatcher.registerCommand(datapackNode);
}

i32 DataPackCommand::enableDataPack(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("name");

    std::ostringstream ss;
    ss << "Enabled data pack '" << name << "'";
    source.sendMessage(ss.str());

    // TODO: 实现数据包系统
    // 1. 查找数据包
    // 2. 验证依赖
    // 3. 加载数据包
    // 4. 通知客户端

    return 1;
}

i32 DataPackCommand::disableDataPack(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("name");

    std::ostringstream ss;
    ss << "Disabled data pack '" << name << "'";
    source.sendMessage(ss.str());

    // TODO: 实现数据包系统

    return 1;
}

i32 DataPackCommand::listDataPacks(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // TODO: 实现数据包系统
    source.sendMessage("There are no data packs enabled");

    return 1;
}

} // namespace command
} // namespace mc
