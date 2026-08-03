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

#include "SaveOffCommand.hpp"

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

void SaveOffCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto saveOffNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("save-off");
    saveOffNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(4); });
    support::applyMetadata(
        saveOffNode, support::makeMetadata("Disables server automatic saving.", "/save-off", 4, {}, false));

    saveOffNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _disableAutoSave(ctx); });

    dispatcher.registerCommand(saveOffNode);
}

i32 SaveOffCommand::_disableAutoSave(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    auto* server = source.server();
    auto* storage = server ? server->sharedStorage() : nullptr;
    if (!storage) {
        source.sendMessage("Error: Storage manager not available");
        return 0;
    }

    if (server->isSharedStorageReadonlyForeignWorld()) {
        source.sendMessage("Automatic saving is already unavailable: current world is a readonly foreign save");
        return 0;
    }

    storage->stopAutoSave();
    source.sendMessage("Automatic saving is now disabled");
    return 1;
}

} // namespace command
} // namespace mc
