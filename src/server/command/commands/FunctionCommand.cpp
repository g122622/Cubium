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

#include "FunctionCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <sstream>

namespace mc {
namespace command {

void FunctionCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto functionNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("function");
    functionNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(
        functionNode, support::makeMetadata("Runs a function from a data pack.", "/function <name>", 2, {}, true));

    auto nameArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("name", StringArgumentType::string());
    nameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return runFunction(ctx); });
    functionNode->addChild(nameArg);

    dispatcher.registerCommand(functionNode);
}

i32 FunctionCommand::runFunction(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("name");

    // 解析函数命名空间
    size_t colonPos = name.find(':');
    std::string namespaceName = colonPos != std::string::npos ? name.substr(0, colonPos) : "minecraft";
    std::string functionName = colonPos != std::string::npos ? name.substr(colonPos + 1) : name;

    std::ostringstream ss;
    ss << "Running function " << namespaceName << ":" << functionName;
    source.sendMessage(ss.str());

    // TODO: 实现数据包函数系统
    // 1. 从数据包加载函数文件
    // 2. 解析函数中的命令
    // 3. 按顺序执行命令
    // 4. 处理递归调用限制

    return 1;
}

} // namespace command
} // namespace mc
