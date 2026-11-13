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
#include "common/entity/entities/player/Player.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/text/ComponentFactory.hpp"
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

namespace {

/** @brief 用两个实体名组件构造翻译消息。 */
[[nodiscard]] std::unique_ptr<::mc::text::ITextComponent> _twoNamesMessage(
    const char* key, const Entity& first, const Entity& second)
{
    std::vector<std::unique_ptr<::mc::text::ITextComponent>> params;
    params.push_back(first.getDisplayName());
    params.push_back(second.getDisplayName());
    return ::mc::text::translatable(key, std::move(params));
}

} // namespace

i32 RideCommand::_mount(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    Entity* passenger = support::EntityResolver::resolveSingle(source, context.getArgument<EntitySelector>("target"));
    Entity* vehicle = support::EntityResolver::resolveSingle(source, context.getArgument<EntitySelector>("vehicle"));
    if (passenger == nullptr || vehicle == nullptr) {
        source.sendFailure(::mc::text::translatable("argument.entity.notfound.entity"));
        return 0;
    }

    if (passenger->isRiding()) {
        Entity* currentVehicle = passenger->world()->getEntity(passenger->getVehicle());
        MC_ASSERT_RELEASE(currentVehicle != nullptr);
        source.sendFailure(_twoNamesMessage("commands.ride.already_riding", *passenger, *currentVehicle));
        return 0;
    }
    if (dynamic_cast<Player*>(vehicle) != nullptr) {
        source.sendFailure(::mc::text::translatable("commands.ride.mount.failure.cant_ride_players"));
        return 0;
    }
    if (passenger == vehicle || passenger->isRidingOrBeingRiddenBy(*vehicle)) {
        source.sendFailure(::mc::text::translatable("commands.ride.mount.failure.loop"));
        return 0;
    }
    if (passenger->world() != vehicle->world()) {
        source.sendFailure(::mc::text::translatable("commands.ride.mount.failure.wrong_dimension"));
        return 0;
    }
    if (!passenger->startRiding(*vehicle)) {
        source.sendFailure(_twoNamesMessage("commands.ride.mount.failure.generic", *passenger, *vehicle));
        return 0;
    }

    source.sendSuccess(_twoNamesMessage("commands.ride.mount.success", *passenger, *vehicle), true);
    return 1;
}

i32 RideCommand::_dismount(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    Entity* passenger = support::EntityResolver::resolveSingle(source, context.getArgument<EntitySelector>("target"));
    if (passenger == nullptr) {
        source.sendFailure(::mc::text::translatable("argument.entity.notfound.entity"));
        return 0;
    }

    if (!passenger->isRiding()) {
        source.sendFailure(::mc::text::translatable("commands.ride.not_riding", passenger->getDisplayName()));
        return 0;
    }

    Entity* vehicle = passenger->world()->getEntity(passenger->getVehicle());
    MC_ASSERT_RELEASE(vehicle != nullptr);
    passenger->stopRiding();

    source.sendSuccess(_twoNamesMessage("commands.ride.dismount.success", *passenger, *vehicle), true);
    return 1;
}

} // namespace mc::command
