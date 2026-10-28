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

#include "RotateCommand.hpp"

#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EntityResolver.hpp"

#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace mc::command {
namespace {

/// Yaw/Pitch（度），与原版 Rotation（yaw 绕 Y 轴、pitch 抬头为正）同义。
Vector2f _lookAt(const Vector3d& from, const Vector3d& to)
{
    const f64 dx = to.x - from.x;
    const f64 dy = to.y - from.y;
    const f64 dz = to.z - from.z;
    const f64 horizontal = std::sqrt(dx * dx + dz * dz);
    const f32 yaw = static_cast<f32>(std::atan2(dz, dx) * 180.0 / 3.14159265358979323846 - 90.0);
    const f32 pitch = static_cast<f32>(-std::atan2(dy, horizontal) * 180.0 / 3.14159265358979323846);
    return Vector2f(yaw, pitch);
}

i32 _applyRotation(ServerCommandSource& source, const std::vector<Entity*>& entities, const Vector2f& rotation)
{
    for (Entity* entity : entities) {
        if (entity != nullptr) {
            entity->setRotation(rotation.x, rotation.y);
        }
    }
    if (entities.empty()) {
        source.sendError("No entity was found");
        return 0;
    }
    return static_cast<i32>(entities.size());
}

} // namespace

void RotateCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto rotateNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("rotate");
    rotateNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        rotateNode, support::makeMetadata("Rotates entities.", "/rotate <target> <rotation>", 2, {}, true));

    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entity());
    rotateNode->addChild(targetArg);

    // /rotate <target> <rotation>
    auto rotationArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "rotation", RotationArgumentType::rotation());
    rotationArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _rotateByCoordinates(ctx);
    });
    targetArg->addChild(rotationArg);

    // /rotate <target> facing <location> | facing entity <entity> [<anchor>]
    auto facingNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("facing");
    auto locationArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "facingLocation", Vec3ArgumentType::vec3());
    locationArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _rotateFacingLocation(ctx);
    });
    facingNode->addChild(locationArg);

    auto entityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    // 原版参数名：facingEntity / facingLocation / facingAnchor。
    auto facingTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "facingEntity", EntityArgumentType::entity());
    facingTargetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _rotateFacingEntity(ctx);
    });
    // 锚点可以单独给出（默认眼睛高度不写锚点时 facingEntity 自身就是完整命令）。
    auto facingAnchorArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "facingAnchor", EntityAnchorArgumentType::entityAnchor());
    facingAnchorArg->setCommand(facingTargetArg->getCommand());
    facingTargetArg->addChild(facingAnchorArg);
    entityNode->addChild(facingTargetArg);
    facingNode->addChild(entityNode);

    targetArg->addChild(facingNode);

    dispatcher.registerCommand(rotateNode);
}

i32 RotateCommand::_rotateByCoordinates(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const Vector2f rotation = RotationArgumentType::getRotation(context, "rotation", source);
    return _applyRotation(
        source, support::EntityResolver::resolve(source, context.getArgument<EntitySelector>("target")), rotation);
}

i32 RotateCommand::_rotateFacingLocation(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const Vector3d location = Vec3ArgumentType::getVec3(context, "facingLocation", source);
    Vector3d from = source.position();
    if (auto* entity = source.entity(); entity != nullptr) {
        from = Vector3d(entity->x(), entity->y() + static_cast<f64>(entity->eyeHeight()), entity->z());
    }
    return _applyRotation(source,
        support::EntityResolver::resolve(source, context.getArgument<EntitySelector>("target")),
        _lookAt(from, location));
}

i32 RotateCommand::_rotateFacingEntity(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    Entity* target =
        support::EntityResolver::resolveSingle(source, context.getArgument<EntitySelector>("facingEntity"));
    if (target == nullptr) {
        source.sendError("No entity was found");
        return 0;
    }
    // 锚点缺省为眼睛高度（原版朝实体看时默认瞄眼睛）。
    const std::string anchor =
        context.hasArgument("facingAnchor") ? context.getArgument<std::string>("facingAnchor") : std::string("eyes");
    const Vector3d from = source.position();
    const f64 eyeOffset = anchor == "feet" ? 0.0 : static_cast<f64>(target->eyeHeight());
    const Vector3d to(target->x(), target->y() + eyeOffset, target->z());
    return _applyRotation(source,
        support::EntityResolver::resolve(source, context.getArgument<EntitySelector>("target")),
        _lookAt(from, to));
}

} // namespace mc::command
