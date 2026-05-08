#include "LootCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>

namespace mc {
namespace command {

void LootCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto lootNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("loot");
    lootNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        lootNode,
        support::makeMetadata(
            "Drops items from a loot table.",
            "/loot <give|insert|replace|spawn> <target> <loot_table>",
            2,
            {},
            true));

    // /loot give <targets> <loot_table>
    auto giveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("give");
    auto giveTargetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets",
        EntityArgumentType::players());
    auto giveLootArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "loot_table",
        StringArgumentType::string());
    giveLootArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return giveLoot(ctx);
    });
    giveTargetsArg->addChild(giveLootArg);
    giveNode->addChild(giveTargetsArg);
    lootNode->addChild(giveNode);

    // /loot spawn <pos> <loot_table>
    auto spawnNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("spawn");
    auto spawnPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "pos",
        Vec3ArgumentType::vec3());
    auto spawnLootArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "loot_table",
        StringArgumentType::string());
    spawnLootArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return spawnLoot(ctx);
    });
    spawnPosArg->addChild(spawnLootArg);
    spawnNode->addChild(spawnPosArg);
    lootNode->addChild(spawnNode);

    // /loot insert <pos> <loot_table>
    auto insertNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("insert");
    auto insertPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "pos",
        Vec3ArgumentType::vec3());
    auto insertLootArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "loot_table",
        StringArgumentType::string());
    insertLootArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return insertLoot(ctx);
    });
    insertPosArg->addChild(insertLootArg);
    insertNode->addChild(insertPosArg);
    lootNode->addChild(insertNode);

    // /loot replace <entity|block> <target> <slot> <loot_table>
    auto replaceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replace");

    auto entityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    auto entityTargetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets",
        EntityArgumentType::entities());
    auto entitySlotArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "slot",
        StringArgumentType::string());
    auto entityLootArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "loot_table",
        StringArgumentType::string());
    entityLootArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return replaceLoot(ctx);
    });
    entitySlotArg->addChild(entityLootArg);
    entityTargetsArg->addChild(entitySlotArg);
    entityNode->addChild(entityTargetsArg);
    replaceNode->addChild(entityNode);

    auto blockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto blockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "pos",
        Vec3ArgumentType::vec3());
    auto blockSlotArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "slot",
        StringArgumentType::string());
    auto blockLootArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "loot_table",
        StringArgumentType::string());
    blockLootArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return replaceLoot(ctx);
    });
    blockSlotArg->addChild(blockLootArg);
    blockPosArg->addChild(blockSlotArg);
    blockNode->addChild(blockPosArg);
    replaceNode->addChild(blockNode);

    lootNode->addChild(replaceNode);

    dispatcher.registerCommand(lootNode);
}

i32 LootCommand::giveLoot(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");
    const std::string lootTable = context.getArgument<std::string>("loot_table");

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched the selector");
        return 0;
    }

    std::ostringstream ss;
    ss << "Gave loot from '" << lootTable << "' to " << playerIds.size() << " player(s)";
    source.sendMessage(ss.str());

    // TODO: 实现战利品表系统
    // 1. 解析战利品表
    // 2. 生成物品
    // 3. 给予玩家

    return static_cast<i32>(playerIds.size());
}

i32 LootCommand::insertLoot(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const Vector3d& pos = context.getArgument<Vector3d>("pos");
    const std::string lootTable = context.getArgument<std::string>("loot_table");

    std::ostringstream ss;
    ss << "Inserted loot from '" << lootTable << "' at (" << pos.x << ", " << pos.y << ", " << pos.z << ")";
    source.sendMessage(ss.str());

    // TODO: 实现战利品表系统和容器交互

    return 1;
}

i32 LootCommand::replaceLoot(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string lootTable = context.getArgument<std::string>("loot_table");

    std::ostringstream ss;
    ss << "Replaced slot with loot from '" << lootTable << "'";
    source.sendMessage(ss.str());

    // TODO: 实现战利品表系统

    return 1;
}

i32 LootCommand::spawnLoot(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const Vector3d& pos = context.getArgument<Vector3d>("pos");
    const std::string lootTable = context.getArgument<std::string>("loot_table");

    std::ostringstream ss;
    ss << "Spawned loot from '" << lootTable << "' at (" << pos.x << ", " << pos.y << ", " << pos.z << ")";
    source.sendMessage(ss.str());

    // TODO: 实现战利品表系统
    // 1. 解析战利品表
    // 2. 生成物品
    // 3. 生成物品实体

    return 1;
}

} // namespace command
} // namespace mc
