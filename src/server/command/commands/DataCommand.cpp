#include "DataCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <sstream>

namespace mc {
namespace command {

void DataCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto dataNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("data");
    dataNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(dataNode,
        support::makeMetadata("Gets, merges, modifies, or removes block entity and entity NBT data.",
            "/data <get|set|merge|remove> <target> [<path>]",
            2,
            {},
            true));

    // /data get
    auto getNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");
    auto targetArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("target", StringArgumentType::string());
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return getData(ctx); });
    getNode->addChild(targetArg);
    dataNode->addChild(getNode);

    // /data set
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setTargetArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("target", StringArgumentType::string());
    auto setValueArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "value", StringArgumentType::greedyString());
    setValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return setData(ctx); });
    setTargetArg->addChild(setValueArg);
    setNode->addChild(setTargetArg);
    dataNode->addChild(setNode);

    // /data merge
    auto mergeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("merge");
    auto mergeTargetArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("target", StringArgumentType::string());
    auto mergeValueArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "value", StringArgumentType::greedyString());
    mergeValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return mergeData(ctx); });
    mergeTargetArg->addChild(mergeValueArg);
    mergeNode->addChild(mergeTargetArg);
    dataNode->addChild(mergeNode);

    // /data remove
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeTargetArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("target", StringArgumentType::string());
    removeTargetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return removeData(ctx); });
    removeNode->addChild(removeTargetArg);
    dataNode->addChild(removeNode);

    dispatcher.registerCommand(dataNode);
}

i32 DataCommand::getData(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");

    // TODO: 实现 NBT 数据获取
    source.sendMessage("Data get: " + target + " (NBT system not yet implemented)");
    return 1;
}

i32 DataCommand::setData(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");
    const std::string value = context.getArgument<std::string>("value");

    // TODO: 实现 NBT 数据设置
    source.sendMessage("Data set: " + target + " = " + value + " (NBT system not yet implemented)");
    return 1;
}

i32 DataCommand::mergeData(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");
    const std::string value = context.getArgument<std::string>("value");

    // TODO: 实现 NBT 数据合并
    source.sendMessage("Data merge: " + target + " (NBT system not yet implemented)");
    return 1;
}

i32 DataCommand::removeData(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("target");

    // TODO: 实现 NBT 数据删除
    source.sendMessage("Data remove: " + target + " (NBT system not yet implemented)");
    return 1;
}

} // namespace command
} // namespace mc
