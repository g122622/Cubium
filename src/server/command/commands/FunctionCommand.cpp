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

    // TODO: 当前使用 StringArgumentType 解析函数名，原版使用 FunctionArgument
    // （提供 Tab 补全、函数标签 # 前缀支持、不存在的函数在输入时即报错等功能）。
    // 完整实现需要自定义 FunctionArgumentType 类，在建议阶段查询 FunctionManager 获取可用函数列表。
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

    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("Function command requires a server instance");
        return 0;
    }

    auto& functionManager = server->functionManager();

    // 检查是否为标签引用（# 前缀）
    if (!name.empty() && name[0] == '#') {
        // 标签引用: #namespace:path
        std::string tagRef = name.substr(1);
        ResourceLocation tagId = ResourceLocation::parse(tagRef);

        if (!functionManager.hasTag(tagId)) {
            std::ostringstream ss;
            ss << "Unknown function tag '" << tagId.toString() << "'";
            source.sendError(ss.str());
            return 0;
        }

        // 执行标签中的所有函数
        const auto& functionIds = functionManager.getTag(tagId);
        i32 totalSuccess = 0;
        i32 totalFailure = 0;
        Size executedCount = 0;

        for (const auto& funcId : functionIds) {
            auto result = functionManager.execute(funcId, source);
            totalSuccess += result.successCount;
            totalFailure += result.failureCount;
            ++executedCount;
        }

        std::ostringstream ss;
        ss << "Executed " << executedCount << " functions from tag '" << tagId.toString() << "' (" << totalSuccess
           << " commands succeeded";
        if (totalFailure > 0) {
            ss << ", " << totalFailure << " failed";
        }
        ss << ")";
        source.sendMessage(ss.str());

        return totalSuccess;
    }

    // 普通函数引用
    ResourceLocation functionId = ResourceLocation::parse(name);

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
