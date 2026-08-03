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

#include "SaveOnCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/core/Types.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"
#include <memory>

namespace mc {
namespace command {

void SaveOnCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto saveOnNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("save-on");
    saveOnNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(4); });
    support::applyMetadata(
        saveOnNode, support::makeMetadata("Enables server automatic saving.", "/save-on", 4, {}, false));

    saveOnNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _enableAutoSave(ctx); });

    dispatcher.registerCommand(saveOnNode);
}

i32 SaveOnCommand::_enableAutoSave(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    auto* server = source.server();
    auto* storage = server ? server->sharedStorage() : nullptr;
    if (!storage) {
        source.sendMessage("Error: Storage manager not available");
        return 0;
    }

    if (server->isSharedStorageReadonlyForeignWorld()) {
        source.sendMessage("Automatic saving remains disabled: current world is a readonly foreign save");
        return 0;
    }

    storage->startAutoSave();
    source.sendMessage("Automatic saving is now enabled");
    return 1;
}

} // namespace command
} // namespace mc
