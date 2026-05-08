#include "BossBarCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>

namespace mc {
namespace command {

void BossBarCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto bossbarNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("bossbar");
    bossbarNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        bossbarNode,
        support::makeMetadata(
            "Creates and modifies boss bars.",
            "/bossbar <add|remove|list|set|get> ...",
            2,
            {},
            true));

    // /bossbar add <id> <name>
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto idArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "id",
        StringArgumentType::string());
    auto nameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "name",
        StringArgumentType::greedyString());
    nameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return addBossBar(ctx);
    });
    idArg->addChild(nameArg);
    addNode->addChild(idArg);
    bossbarNode->addChild(addNode);

    // /bossbar remove <id>
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "id",
        StringArgumentType::string());
    removeIdArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return removeBossBar(ctx);
    });
    removeNode->addChild(removeIdArg);
    bossbarNode->addChild(removeNode);

    // /bossbar list
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return listBossBars(ctx);
    });
    bossbarNode->addChild(listNode);

    // /bossbar set <id> <property> <value>
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "id",
        StringArgumentType::string());

    // set name
    auto nameNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("name");
    auto nameValueArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "name",
        StringArgumentType::greedyString());
    nameValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setBossBar(ctx);
    });
    nameNode->addChild(nameValueArg);
    setIdArg->addChild(nameNode);

    // set color
    auto colorNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("color");
    auto colorValueArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "color",
        StringArgumentType::string());
    colorValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setBossBar(ctx);
    });
    colorNode->addChild(colorValueArg);
    setIdArg->addChild(colorNode);

    // set value
    auto valueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("value");
    auto valueArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "value",
        IntegerArgumentType::integer(0, 100));
    valueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setBossBar(ctx);
    });
    valueNode->addChild(valueArg);
    setIdArg->addChild(valueNode);

    // set visible
    auto visibleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("visible");
    auto visibleArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>(
        "visible",
        BoolArgumentType::boolArg());
    visibleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setBossBar(ctx);
    });
    visibleNode->addChild(visibleArg);
    setIdArg->addChild(visibleNode);

    // set players
    auto playersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("players");
    auto playersArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets",
        EntityArgumentType::players());
    playersArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setBossBar(ctx);
    });
    playersNode->addChild(playersArg);
    setIdArg->addChild(playersNode);

    setNode->addChild(setIdArg);
    bossbarNode->addChild(setNode);

    // /bossbar get <id> <property>
    auto getNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");
    auto getIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "id",
        StringArgumentType::string());

    auto getValueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("value");
    getValueNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return getBossBar(ctx);
    });
    getIdArg->addChild(getValueNode);

    auto getMaxNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("max");
    getMaxNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return getBossBar(ctx);
    });
    getIdArg->addChild(getMaxNode);

    auto getVisibleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("visible");
    getVisibleNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return getBossBar(ctx);
    });
    getIdArg->addChild(getVisibleNode);

    auto getPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("players");
    getPlayersNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return getBossBar(ctx);
    });
    getIdArg->addChild(getPlayersNode);

    getNode->addChild(getIdArg);
    bossbarNode->addChild(getNode);

    dispatcher.registerCommand(bossbarNode);
}

i32 BossBarCommand::addBossBar(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string id = context.getArgument<std::string>("id");
    const std::string name = context.getArgument<std::string>("name");

    std::ostringstream ss;
    ss << "Created custom bossbar '" << id << "' with name '" << name << "'";
    source.sendMessage(ss.str());

    // TODO: 实现 Boss Bar 系统

    return 1;
}

i32 BossBarCommand::removeBossBar(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string id = context.getArgument<std::string>("id");

    std::ostringstream ss;
    ss << "Removed custom bossbar '" << id << "'";
    source.sendMessage(ss.str());

    // TODO: 实现 Boss Bar 系统

    return 1;
}

i32 BossBarCommand::listBossBars(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // TODO: 实现 Boss Bar 系统
    source.sendMessage("There are no custom bossbars");

    return 1;
}

i32 BossBarCommand::setBossBar(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string id = context.getArgument<std::string>("id");

    std::ostringstream ss;
    ss << "Modified bossbar '" << id << "'";
    source.sendMessage(ss.str());

    // TODO: 实现 Boss Bar 系统

    return 1;
}

i32 BossBarCommand::getBossBar(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string id = context.getArgument<std::string>("id");

    std::ostringstream ss;
    ss << "Bossbar '" << id << "' value: 0";
    source.sendMessage(ss.str());

    // TODO: 实现 Boss Bar 系统

    return 1;
}

} // namespace command
} // namespace mc
