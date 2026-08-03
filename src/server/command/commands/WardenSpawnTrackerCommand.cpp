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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT THAT THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR IN THE EVENT THAT THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "WardenSpawnTrackerCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/player/ServerPlayer.hpp"

#include <memory>
#include <sstream>

namespace mc {
namespace command {

void WardenSpawnTrackerCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto rootNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("warden_spawn_tracker");
    rootNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(rootNode,
        support::makeMetadata(
            "Manages the warden spawn tracker.", "/warden_spawn_tracker (clear|set <warning_level>)", 2, {}, true));

    // /warden_spawn_tracker clear
    auto clearNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");
    clearNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _clear(ctx); });
    rootNode->addChild(clearNode);

    // /warden_spawn_tracker set <warning_level>
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");

    auto levelNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "warning_level", IntegerArgumentType::integer(0, 4));
    levelNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setWarningLevel(ctx); });
    setNode->addChild(levelNode);
    rootNode->addChild(setNode);

    dispatcher.registerCommand(rootNode);
}

i32 WardenSpawnTrackerCommand::_clear(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 命令必须由玩家执行
    if (!source.isPlayer()) {
        source.sendError("Only players can execute this command");
        return 0;
    }

    ServerPlayer& player = source.assertPlayer();
    player.wardenWarningEffect().reset();

    source.sendMessage("Warden spawn tracker cleared");
    return 1;
}

i32 WardenSpawnTrackerCommand::_setWarningLevel(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 命令必须由玩家执行
    if (!source.isPlayer()) {
        source.sendError("Only players can execute this command");
        return 0;
    }

    i32 level = context.getArgument<i32>("warning_level");
    ServerPlayer& player = source.assertPlayer();
    player.wardenWarningEffect().setWarningLevel(level);

    std::ostringstream ss;
    ss << "Warden spawn tracker set to " << level;
    source.sendMessage(ss.str());
    return 1;
}

} // namespace command
} // namespace mc
