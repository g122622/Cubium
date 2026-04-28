#include "WorldBorderCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <sstream>

namespace mc {
namespace command {

void WorldBorderCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto borderNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("worldborder");
    borderNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        borderNode,
        support::makeMetadata(
            "Controls the world border.",
            "/worldborder <set|center|damage|warning|get|add>",
            2,
            {},
            true));

    // /worldborder set <size> [time]
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setSizeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
        "size",
        FloatArgumentType::floatArg(1.0f));
    auto setTimeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "time",
        IntegerArgumentType::integer(0));
    setTimeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setBorder(ctx);
    });
    setSizeArg->addChild(setTimeArg);
    // 默认时间为 0（立即）
    setSizeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setBorder(ctx);
    });
    setNode->addChild(setSizeArg);
    borderNode->addChild(setNode);

    // /worldborder center <x> <z>
    auto centerNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("center");
    auto xArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
        "x",
        FloatArgumentType::floatArg());
    auto zArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
        "z",
        FloatArgumentType::floatArg());
    zArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setCenter(ctx);
    });
    xArg->addChild(zArg);
    centerNode->addChild(xArg);
    borderNode->addChild(centerNode);

    // /worldborder damage <amount|buffer> <value>
    auto damageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("damage");
    auto amountNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("amount");
    auto amountArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
        "damagePerBlock",
        FloatArgumentType::floatArg(0.0f));
    amountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setDamage(ctx);
    });
    amountNode->addChild(amountArg);
    damageNode->addChild(amountNode);

    auto bufferNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("buffer");
    auto bufferArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
        "distance",
        FloatArgumentType::floatArg(0.0f));
    bufferArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setDamage(ctx);
    });
    bufferNode->addChild(bufferArg);
    damageNode->addChild(bufferNode);
    borderNode->addChild(damageNode);

    // /worldborder warning <time|distance> <value>
    auto warningNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("warning");
    auto timeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("time");
    auto warnTimeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "seconds",
        IntegerArgumentType::integer(0));
    warnTimeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setWarning(ctx);
    });
    timeNode->addChild(warnTimeArg);
    warningNode->addChild(timeNode);

    auto distanceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("distance");
    auto warnDistArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "blocks",
        IntegerArgumentType::integer(0));
    warnDistArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setWarning(ctx);
    });
    distanceNode->addChild(warnDistArg);
    warningNode->addChild(distanceNode);
    borderNode->addChild(warningNode);

    // /worldborder get
    auto getNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");
    getNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return getBorder(ctx);
    });
    borderNode->addChild(getNode);

    // /worldborder add <distance> [time]
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addDistArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
        "distance",
        FloatArgumentType::floatArg());
    auto addTimeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "time",
        IntegerArgumentType::integer(0));
    addTimeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setBorder(ctx);
    });
    addDistArg->addChild(addTimeArg);
    // 默认时间为 0（立即）
    addDistArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setBorder(ctx);
    });
    addNode->addChild(addDistArg);
    borderNode->addChild(addNode);

    dispatcher.registerCommand(borderNode);
}

i32 WorldBorderCommand::setBorder(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const f32 size = context.getArgument<f32>("size");

    std::ostringstream ss;
    ss << "World border size set to " << static_cast<i32>(size) << " blocks";
    source.sendMessage(ss.str());

    // TODO: 实现世界边界系统
    // 1. 存储边界大小
    // 2. 如果有时间参数，渐变到目标大小
    // 3. 通知所有客户端边界变更

    return 1;
}

i32 WorldBorderCommand::getBorder(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // TODO: 从世界边界管理器获取当前大小
    f32 currentSize = 60000000.0f; // 默认值

    std::ostringstream ss;
    ss << "World border is currently " << static_cast<i64>(currentSize) << " blocks wide";
    source.sendMessage(ss.str());

    return 1;
}

i32 WorldBorderCommand::setCenter(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const f32 x = context.getArgument<f32>("x");
    const f32 z = context.getArgument<f32>("z");

    std::ostringstream ss;
    ss << "World border center set to " << x << ", " << z;
    source.sendMessage(ss.str());

    // TODO: 实现世界边界中心设置

    return 1;
}

i32 WorldBorderCommand::setDamage(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 判断是 amount 还是 buffer
    if (context.hasArgument("damagePerBlock")) {
        const f32 damage = context.getArgument<f32>("damagePerBlock");
        std::ostringstream ss;
        ss << "World border damage per block set to " << damage;
        source.sendMessage(ss.str());
    } else if (context.hasArgument("distance")) {
        const f32 distance = context.getArgument<f32>("distance");
        std::ostringstream ss;
        ss << "World border damage buffer set to " << distance << " blocks";
        source.sendMessage(ss.str());
    }

    // TODO: 实现世界边界伤害设置

    return 1;
}

i32 WorldBorderCommand::setWarning(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 判断是时间还是距离
    if (context.hasArgument("seconds")) {
        const i32 seconds = context.getArgument<i32>("seconds");
        std::ostringstream ss;
        ss << "World border warning time set to " << seconds << " seconds";
        source.sendMessage(ss.str());
    } else if (context.hasArgument("blocks")) {
        const i32 blocks = context.getArgument<i32>("blocks");
        std::ostringstream ss;
        ss << "World border warning distance set to " << blocks << " blocks";
        source.sendMessage(ss.str());
    }

    // TODO: 实现世界边界警告设置

    return 1;
}

} // namespace command
} // namespace mc
