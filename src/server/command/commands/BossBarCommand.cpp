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

#include "BossBarCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "server/application/IServer.hpp"
#include "server/bossbar/BossInfo.hpp"
#include "server/bossbar/CustomServerBossInfo.hpp"
#include "server/bossbar/CustomServerBossInfoManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace command {

void BossBarCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto bossbarNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("bossbar");
    bossbarNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(bossbarNode,
        support::makeMetadata(
            "Creates and modifies boss bars.", "/bossbar <add|remove|list|set|get> ...", 2, {}, true));

    // /bossbar add <id> <name>
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto idArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("id", StringArgumentType::string());
    auto nameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "name", StringArgumentType::greedyString());
    nameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _addBossBar(ctx); });
    idArg->addChild(nameArg);
    addNode->addChild(idArg);
    bossbarNode->addChild(addNode);

    // /bossbar remove <id>
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeIdArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("id", StringArgumentType::string());
    removeIdArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _removeBossBar(ctx); });
    removeNode->addChild(removeIdArg);
    bossbarNode->addChild(removeNode);

    // /bossbar list
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _listBossBars(ctx); });
    bossbarNode->addChild(listNode);

    // /bossbar set <id> <property> <value>
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setIdArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("id", StringArgumentType::string());

    // set name
    auto nameNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("name");
    auto nameValueArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "name", StringArgumentType::greedyString());
    nameValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setName(ctx); });
    nameNode->addChild(nameValueArg);
    setIdArg->addChild(nameNode);

    // set color
    auto colorNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("color");
    for (const auto& colorName : {"pink", "blue", "red", "green", "yellow", "purple", "white"}) {
        auto colorValueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(colorName);
        colorValueNode->setCommand(
            [colorName](CommandContext<ServerCommandSource>& ctx) { return _setColor(ctx, std::string(colorName)); });
        colorNode->addChild(colorValueNode);
    }
    setIdArg->addChild(colorNode);

    // set style (overlay)
    auto styleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("style");
    for (const auto& styleName : {"progress", "notched_6", "notched_10", "notched_12", "notched_20"}) {
        auto styleValueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(styleName);
        styleValueNode->setCommand(
            [styleName](CommandContext<ServerCommandSource>& ctx) { return _setStyle(ctx, std::string(styleName)); });
        styleNode->addChild(styleValueNode);
    }
    setIdArg->addChild(styleNode);

    // set value
    auto valueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("value");
    auto valueArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("value", IntegerArgumentType::integer(0));
    valueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setValue(ctx); });
    valueNode->addChild(valueArg);
    setIdArg->addChild(valueNode);

    // set max
    auto maxNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("max");
    auto maxArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("max", IntegerArgumentType::integer(1));
    maxArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setMax(ctx); });
    maxNode->addChild(maxArg);
    setIdArg->addChild(maxNode);

    // set visible
    auto visibleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("visible");
    auto visibleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("visible", BoolArgumentType::boolArg());
    visibleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setVisible(ctx); });
    visibleNode->addChild(visibleArg);
    setIdArg->addChild(visibleNode);

    // set players
    auto playersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("players");
    auto playersArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::players());
    playersArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setPlayers(ctx); });
    playersNode->addChild(playersArg);
    setIdArg->addChild(playersNode);

    setNode->addChild(setIdArg);
    bossbarNode->addChild(setNode);

    // /bossbar get <id> <property>
    auto getNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");
    auto getIdArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("id", StringArgumentType::string());

    auto getValueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("value");
    getValueNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getValue(ctx); });
    getIdArg->addChild(getValueNode);

    auto getMaxNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("max");
    getMaxNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getMax(ctx); });
    getIdArg->addChild(getMaxNode);

    auto getVisibleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("visible");
    getVisibleNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getVisible(ctx); });
    getIdArg->addChild(getVisibleNode);

    auto getPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("players");
    getPlayersNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getPlayers(ctx); });
    getIdArg->addChild(getPlayersNode);

    getNode->addChild(getIdArg);
    bossbarNode->addChild(getNode);

    dispatcher.registerCommand(bossbarNode);
}

i32 BossBarCommand::_addBossBar(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<std::string>("id");
    const std::string nameStr = context.getArgument<std::string>("name");

    ResourceLocation id(idStr);
    auto name = std::make_unique<text::StringTextComponent>(nameStr);

    auto& manager = source.server()->bossBarManager();

    // 检查是否已存在
    if (manager.get(id) != nullptr) {
        source.sendError("Boss bar with ID '" + idStr + "' already exists");
        return 0;
    }

    // 创建新的 Boss 栏
    auto bossInfo = manager.create(id, std::move(name));
    if (bossInfo == nullptr) {
        source.sendError("Failed to create boss bar '" + idStr + "'");
        return 0;
    }

    manager.add(std::move(bossInfo));

    std::ostringstream ss;
    ss << "Created custom bossbar [" << idStr << "]";
    source.sendMessage(ss.str());

    return static_cast<i32>(manager.size());
}

i32 BossBarCommand::_removeBossBar(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<std::string>("id");

    ResourceLocation id(idStr);
    auto& manager = source.server()->bossBarManager();

    auto* bossInfo = manager.get(id);
    if (bossInfo == nullptr) {
        source.sendError("Unknown boss bar: " + idStr);
        return 0;
    }

    manager.remove(*bossInfo);

    source.sendMessage("Removed boss bar '" + idStr + "'");
    return static_cast<i32>(manager.size());
}

i32 BossBarCommand::_listBossBars(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto& manager = source.server()->bossBarManager();

    auto ids = manager.getIds();
    if (ids.empty()) {
        source.sendMessage("There are no custom bossbars");
        return 0;
    }

    std::ostringstream ss;
    ss << "There are " << ids.size() << " custom bossbar" << (ids.size() == 1 ? "" : "s") << ": ";

    bool first = true;
    for (const auto& id : ids) {
        if (!first) {
            ss << ", ";
        }
        auto* bossInfo = manager.get(id);
        if (bossInfo != nullptr) {
            ss << bossInfo->formattedName()->getFormattedText();
        } else {
            ss << "[" << id.toString() << "]";
        }
        first = false;
    }

    source.sendMessage(ss.str());
    return static_cast<i32>(ids.size());
}

i32 BossBarCommand::_setName(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<std::string>("id");
    const std::string nameStr = context.getArgument<std::string>("name");

    ResourceLocation id(idStr);
    auto& manager = source.server()->bossBarManager();

    auto* bossInfo = manager.get(id);
    if (bossInfo == nullptr) {
        source.sendError("Unknown boss bar: " + idStr);
        return 0;
    }

    auto newName = std::make_unique<text::StringTextComponent>(nameStr);
    bossInfo->setName(std::move(newName));

    source.sendMessage("Changed name of boss bar '" + idStr + "'");
    return 0;
}

i32 BossBarCommand::_setColor(CommandContext<ServerCommandSource>& context, const std::string& colorStr)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<std::string>("id");

    ResourceLocation id(idStr);
    auto& manager = source.server()->bossBarManager();

    auto* bossInfo = manager.get(id);
    if (bossInfo == nullptr) {
        source.sendError("Unknown boss bar: " + idStr);
        return 0;
    }

    server::BossInfoColor color = server::bossInfoColorFromName(colorStr);
    bossInfo->setColor(color);

    source.sendMessage("Changed color of boss bar '" + idStr + "'");
    return 0;
}

i32 BossBarCommand::_setStyle(CommandContext<ServerCommandSource>& context, const std::string& styleStr)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<std::string>("id");

    ResourceLocation id(idStr);
    auto& manager = source.server()->bossBarManager();

    auto* bossInfo = manager.get(id);
    if (bossInfo == nullptr) {
        source.sendError("Unknown boss bar: " + idStr);
        return 0;
    }

    server::BossInfoOverlay overlay = server::bossInfoOverlayFromName(styleStr);
    bossInfo->setOverlay(overlay);

    source.sendMessage("Changed style of boss bar '" + idStr + "'");
    return 0;
}

i32 BossBarCommand::_setValue(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<std::string>("id");
    const i32 value = context.getArgument<i32>("value");

    ResourceLocation id(idStr);
    auto& manager = source.server()->bossBarManager();

    auto* bossInfo = manager.get(id);
    if (bossInfo == nullptr) {
        source.sendError("Unknown boss bar: " + idStr);
        return 0;
    }

    bossInfo->setValue(value);

    std::ostringstream ss;
    ss << "Set value of boss bar '" << idStr << "' to " << value;
    source.sendMessage(ss.str());
    return value;
}

i32 BossBarCommand::_setMax(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<std::string>("id");
    const i32 max = context.getArgument<i32>("max");

    ResourceLocation id(idStr);
    auto& manager = source.server()->bossBarManager();

    auto* bossInfo = manager.get(id);
    if (bossInfo == nullptr) {
        source.sendError("Unknown boss bar: " + idStr);
        return 0;
    }

    bossInfo->setMax(max);

    std::ostringstream ss;
    ss << "Set max of boss bar '" << idStr << "' to " << max;
    source.sendMessage(ss.str());
    return max;
}

i32 BossBarCommand::_setVisible(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<std::string>("id");
    const bool visible = context.getArgument<bool>("visible");

    ResourceLocation id(idStr);
    auto& manager = source.server()->bossBarManager();

    auto* bossInfo = manager.get(id);
    if (bossInfo == nullptr) {
        source.sendError("Unknown boss bar: " + idStr);
        return 0;
    }

    bossInfo->setVisible(visible);

    std::ostringstream ss;
    ss << "Set visibility of boss bar '" << idStr << "' to " << (visible ? "visible" : "hidden");
    source.sendMessage(ss.str());
    return 0;
}

i32 BossBarCommand::_setPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<std::string>("id");
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    ResourceLocation id(idStr);
    auto& manager = source.server()->bossBarManager();

    auto* bossInfo = manager.get(id);
    if (bossInfo == nullptr) {
        source.sendError("Unknown boss bar: " + idStr);
        return 0;
    }

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        // 清空玩家列表
        bossInfo->removeAllPlayers();
        source.sendMessage("Removed all players from boss bar '" + idStr + "'");
        return 0;
    }

    // 获取玩家实体列表
    std::vector<ServerPlayer*> players;
    for (PlayerId playerId : playerIds) {
        Player* player = source.server()->playerEntityManager().getPlayerEntity(playerId, *source.world());
        if (player != nullptr) {
            ServerPlayer* serverPlayer = player->asServerPlayer();
            if (serverPlayer != nullptr) {
                players.push_back(serverPlayer);
            }
        }
    }

    bool changed = bossInfo->setPlayers(players);

    if (!changed) {
        source.sendError("Players unchanged for boss bar '" + idStr + "'");
        return 0;
    }

    std::ostringstream ss;
    if (players.empty()) {
        ss << "Removed all players from boss bar '" << idStr << "'";
    } else {
        ss << "Set players of boss bar '" << idStr << "' to " << players.size() << " player"
           << (players.size() == 1 ? "" : "s");
    }
    source.sendMessage(ss.str());
    return static_cast<i32>(players.size());
}

i32 BossBarCommand::_getValue(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<std::string>("id");

    ResourceLocation id(idStr);
    auto& manager = source.server()->bossBarManager();

    auto* bossInfo = manager.get(id);
    if (bossInfo == nullptr) {
        source.sendError("Unknown boss bar: " + idStr);
        return 0;
    }

    std::ostringstream ss;
    ss << bossInfo->formattedName()->getFormattedText() << " has value " << bossInfo->value();
    source.sendMessage(ss.str());
    return bossInfo->value();
}

i32 BossBarCommand::_getMax(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<std::string>("id");

    ResourceLocation id(idStr);
    auto& manager = source.server()->bossBarManager();

    auto* bossInfo = manager.get(id);
    if (bossInfo == nullptr) {
        source.sendError("Unknown boss bar: " + idStr);
        return 0;
    }

    std::ostringstream ss;
    ss << bossInfo->formattedName()->getFormattedText() << " has max " << bossInfo->max();
    source.sendMessage(ss.str());
    return bossInfo->max();
}

i32 BossBarCommand::_getVisible(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<std::string>("id");

    ResourceLocation id(idStr);
    auto& manager = source.server()->bossBarManager();

    auto* bossInfo = manager.get(id);
    if (bossInfo == nullptr) {
        source.sendError("Unknown boss bar: " + idStr);
        return 0;
    }

    std::ostringstream ss;
    if (bossInfo->visible()) {
        ss << bossInfo->formattedName()->getFormattedText() << " is currently visible";
    } else {
        ss << bossInfo->formattedName()->getFormattedText() << " is currently hidden";
    }
    source.sendMessage(ss.str());
    return bossInfo->visible() ? 1 : 0;
}

i32 BossBarCommand::_getPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<std::string>("id");

    ResourceLocation id(idStr);
    auto& manager = source.server()->bossBarManager();

    auto* bossInfo = manager.get(id);
    if (bossInfo == nullptr) {
        source.sendError("Unknown boss bar: " + idStr);
        return 0;
    }

    auto& players = bossInfo->players();
    if (players.empty()) {
        std::ostringstream ss;
        ss << "No players are currently tracking boss bar '" << idStr << "'";
        source.sendMessage(ss.str());
        return 0;
    }

    std::ostringstream ss;
    ss << bossInfo->formattedName()->getFormattedText() << " has " << players.size() << " player"
       << (players.size() == 1 ? "" : "s") << ": ";

    bool first = true;
    for (PlayerId playerId : players) {
        if (!first) {
            ss << ", ";
        }
        auto* playerData = source.server()->playerManager().getPlayer(playerId);
        if (playerData != nullptr) {
            ss << playerData->username;
        }
        first = false;
    }

    source.sendMessage(ss.str());
    return static_cast<i32>(players.size());
}

} // namespace command
} // namespace mc
