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

#include "VersionCommand.hpp"

#include "common/command/CommandNode.hpp"
#include "server/command/support/CommandMetadata.hpp"

#include <memory>

namespace mc::command {

void VersionCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto versionNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("version");
    versionNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(versionNode, support::makeMetadata("Gets the version of the server.", "/version", 2));
    versionNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _showVersion(ctx);
    });

    dispatcher.registerCommand(versionNode);
}

i32 VersionCommand::_showVersion(CommandContext<ServerCommandSource>& context)
{
    // 原版输出 "This server is running ... version 1.21.11"；Cubium 对齐同一目标版本。
    context.getSource().sendMessage("This server is running Cubium on Minecraft 1.21.11");
    return 1;
}

} // namespace mc::command
