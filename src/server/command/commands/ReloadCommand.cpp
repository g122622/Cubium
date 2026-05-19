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

#include "ReloadCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"

namespace mc {
namespace command {

void ReloadCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto reloadNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reload");
    reloadNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(reloadNode,
        support::makeMetadata("Reloads loot tables, advancements, and functions from disk.", "/reload", 2, {}, true));

    reloadNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return reload(ctx); });

    dispatcher.registerCommand(reloadNode);
}

i32 ReloadCommand::reload(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    source.sendMessage("Reloading server resources...");

    // TODO: 实现资源重新加载系统
    // 1. 重新加载数据包
    // 2. 重新加载进度
    // 3. 重新加载战利品表
    // 4. 重新加载函数
    // 5. 通知客户端

    source.sendMessage("Reload complete!");
    return 1;
}

} // namespace command
} // namespace mc
