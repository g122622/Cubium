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
#include "common/world/border/WorldBorder.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"
#include <algorithm>
#include <memory>
#include <sstream>

namespace mc {
namespace command {

void WorldBorderCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto borderNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("worldborder");
    borderNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(borderNode,
        support::makeMetadata(
            "Controls the world border.", "/worldborder <set|center|damage|warning|get|add>", 2, {}, true));

    // /worldborder set <size> [time]
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setSizeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
        "size", FloatArgumentType::floatArg(1.0f, static_cast<f32>(world::border::WorldBorder::MAX_SIZE)));
    auto setTimeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("time", TimeArgumentType::time());
    setTimeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setBorder(ctx); });
    setSizeArg->addChild(setTimeArg);
    // 默认时间为 0（立即）
    setSizeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setBorder(ctx); });
    setNode->addChild(setSizeArg);
    borderNode->addChild(setNode);

    // /worldborder center <x> <z>
    auto centerNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("center");
    auto xArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("x", FloatArgumentType::floatArg());
    auto zArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("z", FloatArgumentType::floatArg());
    zArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setCenter(ctx); });
    xArg->addChild(zArg);
    centerNode->addChild(xArg);
    borderNode->addChild(centerNode);

    // /worldborder damage <amount|buffer> <value>
    auto damageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("damage");
    auto amountNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("amount");
    auto amountArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
        "damagePerBlock", FloatArgumentType::floatArg(0.0f));
    amountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setDamageAmount(ctx); });
    amountNode->addChild(amountArg);
    damageNode->addChild(amountNode);

    auto bufferNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("buffer");
    auto bufferArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("distance", FloatArgumentType::floatArg(0.0f));
    bufferArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setDamageBuffer(ctx); });
    bufferNode->addChild(bufferArg);
    damageNode->addChild(bufferNode);
    borderNode->addChild(damageNode);

    // /worldborder warning <time|distance> <value>
    auto warningNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("warning");
    auto timeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("time");
    auto warnTimeArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("seconds", TimeArgumentType::time());
    warnTimeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setWarningTime(ctx); });
    timeNode->addChild(warnTimeArg);
    warningNode->addChild(timeNode);

    auto distanceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("distance");
    auto warnDistArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("blocks", IntegerArgumentType::integer(0));
    warnDistArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setWarningDistance(ctx); });
    distanceNode->addChild(warnDistArg);
    warningNode->addChild(distanceNode);
    borderNode->addChild(warningNode);

    // /worldborder get
    auto getNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");
    getNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getBorder(ctx); });
    borderNode->addChild(getNode);

    // /worldborder add <distance> [time]
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addDistArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("distance", FloatArgumentType::floatArg());
    auto addTimeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("time", TimeArgumentType::time());
    addTimeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _addBorder(ctx); });
    addDistArg->addChild(addTimeArg);
    // 默认时间为 0（立即）
    addDistArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _addBorder(ctx); });
    addNode->addChild(addDistArg);
    borderNode->addChild(addNode);

    dispatcher.registerCommand(borderNode);
}

i32 WorldBorderCommand::_setBorder(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendError("No world available.");
        return 0;
    }

    auto& border = world->worldBorder();
    const f32 size = context.getArgument<f32>("size");

    // 检查是否有时间参数（渐变）
    if (context.hasArgument("time")) {
        const i32 timeSeconds = context.getArgument<i32>("time");
        const u64 timeMs = static_cast<u64>(timeSeconds) * 1000;
        border.setSizeLerp(border.getSize(), static_cast<double>(size), timeMs);

        std::ostringstream ss;
        ss << "World border size will change to " << static_cast<i64>(size) << " blocks over " << timeSeconds
           << " seconds";
        source.sendMessage(ss.str());
    } else {
        border.setSize(static_cast<double>(size));

        std::ostringstream ss;
        ss << "World border size set to " << static_cast<i64>(size) << " blocks";
        source.sendMessage(ss.str());
    }

    return 1;
}

i32 WorldBorderCommand::_getBorder(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendError("No world available.");
        return 0;
    }

    auto& border = world->worldBorder();
    double currentSize = border.getSize();

    std::ostringstream ss;
    ss << "World border is currently " << static_cast<i64>(currentSize) << " blocks wide";
    source.sendMessage(ss.str());

    return 1;
}

i32 WorldBorderCommand::_setCenter(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendError("No world available.");
        return 0;
    }

    auto& border = world->worldBorder();
    const f32 x = context.getArgument<f32>("x");
    const f32 z = context.getArgument<f32>("z");

    border.setCenter(static_cast<double>(x), static_cast<double>(z));

    std::ostringstream ss;
    ss << "World border center set to " << x << ", " << z;
    source.sendMessage(ss.str());

    return 1;
}

i32 WorldBorderCommand::_setDamageAmount(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendError("No world available.");
        return 0;
    }

    auto& border = world->worldBorder();
    const f32 damage = context.getArgument<f32>("damagePerBlock");

    border.setDamagePerBlock(static_cast<double>(damage));

    std::ostringstream ss;
    ss << "World border damage per block set to " << damage;
    source.sendMessage(ss.str());

    return 1;
}

i32 WorldBorderCommand::_setDamageBuffer(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendError("No world available.");
        return 0;
    }

    auto& border = world->worldBorder();
    const f32 distance = context.getArgument<f32>("distance");

    border.setDamageBuffer(static_cast<double>(distance));

    std::ostringstream ss;
    ss << "World border damage buffer set to " << distance << " blocks";
    source.sendMessage(ss.str());

    return 1;
}

i32 WorldBorderCommand::_setWarningTime(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendError("No world available.");
        return 0;
    }

    auto& border = world->worldBorder();
    const i32 seconds = context.getArgument<i32>("seconds");

    border.setWarningTime(seconds);

    std::ostringstream ss;
    ss << "World border warning time set to " << seconds << " seconds";
    source.sendMessage(ss.str());

    return 1;
}

i32 WorldBorderCommand::_setWarningDistance(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendError("No world available.");
        return 0;
    }

    auto& border = world->worldBorder();
    const i32 blocks = context.getArgument<i32>("blocks");

    border.setWarningDistance(blocks);

    std::ostringstream ss;
    ss << "World border warning distance set to " << blocks << " blocks";
    source.sendMessage(ss.str());

    return 1;
}

i32 WorldBorderCommand::_addBorder(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* world = source.world();

    if (world == nullptr) {
        source.sendError("No world available.");
        return 0;
    }

    auto& border = world->worldBorder();
    const f32 distance = context.getArgument<f32>("distance");

    double newSize = border.getSize() + static_cast<double>(distance);
    // 限制最大值
    newSize = std::min(newSize, world::border::WorldBorder::MAX_SIZE);
    newSize = std::max(newSize, 1.0);

    // 检查是否有时间参数（渐变）
    if (context.hasArgument("time")) {
        const i32 timeSeconds = context.getArgument<i32>("time");
        const u64 timeMs = static_cast<u64>(timeSeconds) * 1000;
        border.setSizeLerp(border.getSize(), newSize, timeMs);

        std::ostringstream ss;
        ss << "World border size will change to " << static_cast<i64>(newSize) << " blocks over " << timeSeconds
           << " seconds";
        source.sendMessage(ss.str());
    } else {
        border.setSize(newSize);

        std::ostringstream ss;
        ss << "World border size set to " << static_cast<i64>(newSize) << " blocks";
        source.sendMessage(ss.str());
    }

    return 1;
}

} // namespace command
} // namespace mc
