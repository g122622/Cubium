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
#include "common/resource/ResourceLocation.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/function/FunctionManager.hpp"
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
    nameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _runFunction(ctx); });
    functionNode->addChild(nameArg);

    dispatcher.registerCommand(functionNode);
}

i32 FunctionCommand::_runFunction(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("name");

    // 解析函数命名空间（格式：namespace:path 或 path）
    ResourceLocation functionId = ResourceLocation::parse(name);

    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("Function command requires a server instance");
        return 0;
    }

    auto& functionManager = server->functionManager();

    // 检查函数是否存在
    if (!functionManager.hasFunction(functionId)) {
        std::ostringstream ss;
        ss << "Unknown function '" << functionId.toString() << "'";
        source.sendError(ss.str());
        return 0;
    }

    // 执行函数
    auto result = functionManager.execute(functionId, source);

    // 反馈执行结果
    std::ostringstream ss;
    ss << "Executed function '" << functionId.toString() << "' (" << result.successCount << " commands succeeded";
    if (result.failureCount > 0) {
        ss << ", " << result.failureCount << " failed";
    }
    ss << ")";
    source.sendMessage(ss.str());

    return result.successCount;
}

} // namespace command
} // namespace mc
