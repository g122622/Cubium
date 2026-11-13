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

#include "WorldBorderCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/TimeArgument.hpp"
#include "common/core/Types.hpp"
#include "common/util/text/ComponentFactory.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"
#include <algorithm>
#include <memory>
#include <sstream>
#include <fmt/format.h>

namespace mc {
namespace command {

void WorldBorderCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto borderNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("worldborder");
    borderNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(borderNode,
        support::makeMetadata(
            "Controls the world border.", "/worldborder <set|center|damage|warning|get|add>", 2, {}, true));

    // /worldborder set <distance> [time]
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setSizeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f64>>(
        "distance", DoubleArgumentType::doubleArg(-59999968.0, 59999968.0));
    setSizeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setBorder(ctx);
    });
    auto setTimeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("time", TimeArgumentType::time());
    setTimeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setBorder(ctx);
    });
    setSizeArg->addChild(setTimeArg);
    setNode->addChild(setSizeArg);
    borderNode->addChild(setNode);

    // /worldborder center <pos>：原版是一个 Vec2Argument（水平面 x/z），参数名 "pos"。
    auto centerNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("center");
    auto centerArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>("pos", Vec2ArgumentType::vec2());
    centerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setCenter(ctx);
    });
    centerNode->addChild(centerArg);
    borderNode->addChild(centerNode);

    // /worldborder damage amount <damagePerBlock>
    auto damageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("damage");
    auto amountNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("amount");
    auto amountArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
        "damagePerBlock", FloatArgumentType::floatArg(0.0f));
    amountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setDamageAmount(ctx);
    });
    amountNode->addChild(amountArg);
    damageNode->addChild(amountNode);

    // /worldborder damage buffer <distance>
    auto bufferNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("buffer");
    auto bufferArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("distance", FloatArgumentType::floatArg(0.0f));
    bufferArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setDamageBuffer(ctx);
    });
    bufferNode->addChild(bufferArg);
    damageNode->addChild(bufferNode);
    borderNode->addChild(damageNode);

    // /worldborder warning time <seconds>
    auto warningNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("warning");
    auto timeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("time");
    // 原版 warning time 的参数名是 time（命令树里是 time 参数类型）。
    auto warnTimeArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("time", TimeArgumentType::time());
    warnTimeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setWarningTime(ctx);
    });
    timeNode->addChild(warnTimeArg);
    warningNode->addChild(timeNode);

    // /worldborder warning distance <blocks>
    auto distanceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("distance");
    // 原版 warning distance 的参数名是 distance。
    auto warnDistArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("distance", IntegerArgumentType::integer(0));
    warnDistArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setWarningDistance(ctx);
    });
    distanceNode->addChild(warnDistArg);
    warningNode->addChild(distanceNode);
    borderNode->addChild(warningNode);

    // /worldborder get
    auto getNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");
    getNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getBorder(ctx);
    });
    borderNode->addChild(getNode);

    // /worldborder add <distance> [time]
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addDistArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f64>>(
        "distance", DoubleArgumentType::doubleArg(-59999968.0, 59999968.0));
    addDistArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _addBorder(ctx);
    });
    auto addTimeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("time", TimeArgumentType::time());
    addTimeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _addBorder(ctx);
    });
    addDistArg->addChild(addTimeArg);
    addNode->addChild(addDistArg);
    borderNode->addChild(addNode);

    dispatcher.registerCommand(borderNode);
}

i32 WorldBorderCommand::_setBorder(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendFailure(::mc::text::literal("No world available."));
        return 0;
    }

    auto& border = world->worldBorder();
    const f32 size = static_cast<f32>(context.getArgument<f64>("distance"));

    // 检查是否有时间参数（渐变）
    if (context.hasArgument("time")) {
        const i32 timeSeconds = context.getArgument<i32>("time");
        const u64 timeMs = static_cast<u64>(timeSeconds) * 1000;
        border.setSizeLerp(border.getSize(), static_cast<f64>(size), timeMs);

        source.sendSuccess(::mc::text::translatable("commands.worldborder.set.grow",
                               {fmt::format("{:.1f}", size), fmt::format("{:.2f}", static_cast<f64>(timeSeconds))}),
            true);
    } else {
        border.setSize(static_cast<f64>(size));

        source.sendSuccess(
            ::mc::text::translatable("commands.worldborder.set.immediate", fmt::format("{:.1f}", size)), true);
    }

    return 1;
}

i32 WorldBorderCommand::_getBorder(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendFailure(::mc::text::literal("No world available."));
        return 0;
    }

    auto& border = world->worldBorder();
    f64 currentSize = border.getSize();

    std::ostringstream ss;
    source.sendSuccess(::mc::text::translatable("commands.worldborder.get", fmt::format("{:.0f}", currentSize)), false);

    return 1;
}

i32 WorldBorderCommand::_setCenter(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendFailure(::mc::text::literal("No world available."));
        return 0;
    }

    auto& border = world->worldBorder();
    const Vector2d center = Vec2ArgumentType::getVec2(context, "pos", source);
    const f64 x = center.x;
    const f64 z = center.y;

    border.setCenter(x, z);

    std::ostringstream ss;
    source.sendSuccess(::mc::text::translatable(
                           "commands.worldborder.center.success", {fmt::format("{:.2f}", x), fmt::format("{:.2f}", z)}),
        true);

    return 1;
}

i32 WorldBorderCommand::_setDamageAmount(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendFailure(::mc::text::literal("No world available."));
        return 0;
    }

    auto& border = world->worldBorder();
    const f32 damage = context.getArgument<f32>("damagePerBlock");

    border.setDamagePerBlock(static_cast<f64>(damage));

    std::ostringstream ss;
    source.sendSuccess(
        ::mc::text::translatable("commands.worldborder.damage.amount.success", fmt::format("{:.2f}", damage)), true);

    return 1;
}

i32 WorldBorderCommand::_setDamageBuffer(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendFailure(::mc::text::literal("No world available."));
        return 0;
    }

    auto& border = world->worldBorder();
    const f32 distance = context.getArgument<f32>("distance");

    border.setDamageBuffer(static_cast<f64>(distance));

    std::ostringstream ss;
    source.sendSuccess(
        ::mc::text::translatable("commands.worldborder.damage.buffer.success", fmt::format("{:.2f}", distance)), true);

    return 1;
}

i32 WorldBorderCommand::_setWarningTime(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendFailure(::mc::text::literal("No world available."));
        return 0;
    }

    auto& border = world->worldBorder();
    const i32 seconds = context.getArgument<i32>("time");

    border.setWarningTime(seconds);

    std::ostringstream ss;
    source.sendSuccess(::mc::text::translatable("commands.worldborder.warning.time.success",
                           fmt::format("{:.2f}", static_cast<f64>(seconds))),
        true);

    return 1;
}

i32 WorldBorderCommand::_setWarningDistance(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendFailure(::mc::text::literal("No world available."));
        return 0;
    }

    auto& border = world->worldBorder();
    const i32 blocks = context.getArgument<i32>("distance");

    border.setWarningDistance(blocks);

    std::ostringstream ss;
    source.sendSuccess(
        ::mc::text::translatable("commands.worldborder.warning.distance.success", fmt::format("{}", blocks)), true);

    return 1;
}

i32 WorldBorderCommand::_addBorder(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendFailure(::mc::text::literal("No world available."));
        return 0;
    }

    auto& border = world->worldBorder();
    const f64 distance = context.getArgument<f64>("distance");

    f64 newSize = border.getSize() + distance;
    // 限制最大值
    newSize = std::min(newSize, world::border::WorldBorder::MAX_SIZE);
    newSize = std::max(newSize, 1.0);

    // 检查是否有时间参数（渐变）
    if (context.hasArgument("time")) {
        const i32 timeSeconds = context.getArgument<i32>("time");
        const u64 timeMs = static_cast<u64>(timeSeconds) * 1000;
        border.setSizeLerp(border.getSize(), newSize, timeMs);

        source.sendSuccess(::mc::text::translatable("commands.worldborder.set.grow",
                               {fmt::format("{:.1f}", newSize), fmt::format("{:.2f}", static_cast<f64>(timeSeconds))}),
            true);
    } else {
        border.setSize(newSize);

        source.sendSuccess(
            ::mc::text::translatable("commands.worldborder.set.immediate", fmt::format("{:.1f}", newSize)), true);
    }

    return 1;
}

} // namespace command
} // namespace mc
