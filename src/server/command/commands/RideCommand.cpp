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

#include "RideCommand.hpp"

#include "common/command/CommandNode.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/entity/core/Entity.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EntityResolver.hpp"
#include "server/world/ServerWorld.hpp"

#include <memory>
#include <vector>

namespace mc::command {

void RideCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto rideNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("ride");
    rideNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        rideNode, support::makeMetadata("Rides an entity.", "/ride <target> mount <vehicle>", 2, {}, true));

    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entity());

    auto mountNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("mount");
    auto vehicleArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "vehicle", EntityArgumentType::entity());
    vehicleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _mount(ctx);
    });
    mountNode->addChild(vehicleArg);
    targetArg->addChild(mountNode);

    auto dismountNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("dismount");
    dismountNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _dismount(ctx);
    });
    targetArg->addChild(dismountNode);

    rideNode->addChild(targetArg);
    dispatcher.registerCommand(rideNode);
}

i32 RideCommand::_mount(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    Entity* passenger = support::EntityResolver::resolveSingle(source, context.getArgument<EntitySelector>("target"));
    Entity* vehicle = support::EntityResolver::resolveSingle(source, context.getArgument<EntitySelector>("vehicle"));
    if (passenger == nullptr || vehicle == nullptr) {
        source.sendError("No entity was found");
        return 0;
    }
    if (passenger == vehicle) {
        source.sendError("An entity cannot ride itself");
        return 0;
    }
    if (!passenger->startRiding(*vehicle)) {
        source.sendError("That entity can't ride this one");
        return 0;
    }
    return 1;
}

i32 RideCommand::_dismount(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    Entity* passenger = support::EntityResolver::resolveSingle(source, context.getArgument<EntitySelector>("target"));
    if (passenger == nullptr) {
        source.sendError("No entity was found");
        return 0;
    }
    if (!passenger->isRiding()) {
        source.sendError("That entity is not riding anything");
        return 0;
    }
    passenger->stopRiding();
    return 1;
}

} // namespace mc::command
