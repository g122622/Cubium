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
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/core/Types.hpp"
#include "common/resource/LanguageManager.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "server/application/IServer.hpp"
#include "server/bossbar/BossInfo.hpp"
#include "server/bossbar/CustomServerBossInfo.hpp"
#include "server/bossbar/CustomServerBossInfoManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/CommandSuggestionProviders.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace command {

namespace {

/// 语言文件加载器（反馈文本复用原版 commands.bossbar.* 键）
const LanguageManager& _lang()
{
    return LanguageManager::instance();
}

/**
 * @brief 查找 Boss 栏；不存在时向命令源发送未知 ID 错误
 */
server::CustomServerBossInfo* _findBossBar(ServerCommandSource& source, const std::string& idStr)
{
    ResourceLocation id(idStr);
    auto* bossInfo = source.server()->bossBarManager().get(id);
    if (bossInfo == nullptr) {
        source.sendError(_lang().get("commands.bossbar.unknown", {idStr}));
    }
    return bossInfo;
}

/**
 * @brief Boss 栏的显示名（带方括号格式，与原版 CustomBossEvent.getDisplayName 一致）
 */
std::string _displayName(const server::CustomServerBossInfo& bossInfo)
{
    return "[" + bossInfo.formattedName()->getFormattedText() + "]";
}

/**
 * @brief 解析玩家名列表文本，形如 "a, b, c"
 */
std::string _joinPlayerNames(const std::vector<std::string>& names)
{
    std::string joined;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i != 0) {
            joined += ", ";
        }
        joined += names[i];
    }
    return joined;
}

} // namespace

void BossBarCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto bossbarNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("bossbar");
    bossbarNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(bossbarNode,
        support::makeMetadata(
            "Creates and modifies boss bars.", "/bossbar <add|remove|list|set|get> ...", 2, {}, true));

    // /bossbar add <id> <name>
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "id", IdentifierArgumentType::identifier());
    // 原版 /bossbar add 的 id 没有 .suggests（只有 remove/set 有），此处不标记。
    auto addNameArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("name", StringArgumentType::string());
    addNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _addBossBar(ctx);
    });
    addIdArg->addChild(addNameArg);
    addNode->addChild(addIdArg);
    bossbarNode->addChild(addNode);

    // /bossbar remove <id>
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "id", IdentifierArgumentType::identifier());
    removeIdArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    removeIdArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _removeBossBar(ctx);
    });
    removeNode->addChild(removeIdArg);
    bossbarNode->addChild(removeNode);

    // /bossbar list
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _listBossBars(ctx);
    });
    bossbarNode->addChild(listNode);

    // /bossbar set <id> <property> ...
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "id", IdentifierArgumentType::identifier());
    setIdArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());

    // /bossbar set <id> name <name>
    auto setNameNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("name");
    auto setNameArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("name", StringArgumentType::string());
    setNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setName(ctx);
    });
    setNameNode->addChild(setNameArg);
    setIdArg->addChild(setNameNode);

    // /bossbar set <id> color <color>
    auto setColorNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("color");
    auto setColorArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("color", StringArgumentType::string());
    setColorArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setColor(ctx, ctx.getArgument<std::string>("color"));
    });
    setColorNode->addChild(setColorArg);
    setIdArg->addChild(setColorNode);

    // /bossbar set <id> style <style>
    auto setStyleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("style");
    auto setStyleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("style", StringArgumentType::string());
    setStyleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setStyle(ctx, ctx.getArgument<std::string>("style"));
    });
    setStyleNode->addChild(setStyleArg);
    setIdArg->addChild(setStyleNode);

    // /bossbar set <id> value <value>
    auto setValueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("value");
    auto setValueArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("value", IntegerArgumentType::integer(0));
    setValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setValue(ctx);
    });
    setValueNode->addChild(setValueArg);
    setIdArg->addChild(setValueNode);

    // /bossbar set <id> max <max>
    auto setMaxNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("max");
    auto setMaxArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("max", IntegerArgumentType::integer(1));
    setMaxArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setMax(ctx);
    });
    setMaxNode->addChild(setMaxArg);
    setIdArg->addChild(setMaxNode);

    // /bossbar set <id> visible <visible>
    auto setVisibleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("visible");
    auto setVisibleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("visible", BoolArgumentType::boolArg());
    setVisibleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setVisible(ctx);
    });
    setVisibleNode->addChild(setVisibleArg);
    setIdArg->addChild(setVisibleNode);

    // /bossbar set <id> players <targets>
    auto setPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("players");
    auto setPlayersArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::players());
    setPlayersArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setPlayers(ctx);
    });
    setPlayersNode->addChild(setPlayersArg);
    setIdArg->addChild(setPlayersNode);

    setNode->addChild(setIdArg);
    bossbarNode->addChild(setNode);

    // /bossbar get <id> <property>
    auto getNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");
    auto getIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "id", IdentifierArgumentType::identifier());
    getIdArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());

    auto getValueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("value");
    getValueNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getValue(ctx);
    });
    getIdArg->addChild(getValueNode);

    auto getMaxNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("max");
    getMaxNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getMax(ctx);
    });
    getIdArg->addChild(getMaxNode);

    auto getVisibleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("visible");
    getVisibleNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getVisible(ctx);
    });
    getIdArg->addChild(getVisibleNode);

    auto getPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("players");
    getPlayersNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getPlayers(ctx);
    });
    getIdArg->addChild(getPlayersNode);

    getNode->addChild(getIdArg);
    bossbarNode->addChild(getNode);

    dispatcher.registerCommand(bossbarNode);
}

i32 BossBarCommand::_addBossBar(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<ResourceLocation>("id").toString();
    const std::string nameStr = context.getArgument<std::string>("name");

    ResourceLocation id(idStr);
    auto& manager = source.server()->bossBarManager();

    // vanilla：ID 已存在时抛 create.failed（不覆盖既有 Boss 栏）。
    if (manager.get(id) != nullptr) {
        source.sendError(_lang().get("commands.bossbar.create.failed", {idStr}));
        return 0;
    }

    auto name = std::make_unique<text::StringTextComponent>(nameStr);
    auto bossInfo = manager.create(id, std::move(name));
    if (bossInfo == nullptr) {
        source.sendError(_lang().get("commands.bossbar.create.failed", {idStr}));
        return 0;
    }

    manager.add(std::move(bossInfo));
    source.sendMessage(_lang().get("commands.bossbar.create.success", {_displayName(*manager.get(id))}));
    return static_cast<i32>(manager.size());
}

i32 BossBarCommand::_removeBossBar(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<ResourceLocation>("id").toString();

    auto* bossInfo = _findBossBar(source, idStr);
    if (bossInfo == nullptr) {
        return 0;
    }

    const std::string displayName = _displayName(*bossInfo);
    auto& manager = source.server()->bossBarManager();
    manager.remove(*bossInfo);

    source.sendMessage(_lang().get("commands.bossbar.remove.success", {displayName}));
    return static_cast<i32>(manager.size());
}

i32 BossBarCommand::_listBossBars(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto& manager = source.server()->bossBarManager();

    const auto ids = manager.getIds();
    if (ids.empty()) {
        source.sendMessage(_lang().get("commands.bossbar.list.bars.none"));
        return 0;
    }

    std::vector<std::string> names;
    names.reserve(ids.size());
    for (const auto& id : ids) {
        const auto* bossInfo = manager.get(id);
        if (bossInfo != nullptr) {
            names.push_back(_displayName(*bossInfo));
        }
    }

    source.sendMessage(
        _lang().get("commands.bossbar.list.bars.some", {std::to_string(ids.size()), _joinPlayerNames(names)}));
    return static_cast<i32>(ids.size());
}

i32 BossBarCommand::_setName(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<ResourceLocation>("id").toString();
    const std::string nameStr = context.getArgument<std::string>("name");

    auto* bossInfo = _findBossBar(source, idStr);
    if (bossInfo == nullptr) {
        return 0;
    }

    // vanilla：名称未变化时抛 set.name.unchanged。
    auto newName = std::make_unique<text::StringTextComponent>(nameStr);
    if (bossInfo->name().getFormattedText() == newName->getFormattedText()) {
        source.sendError(_lang().get("commands.bossbar.set.name.unchanged"));
        return 0;
    }

    const std::string displayName = _displayName(*bossInfo);
    bossInfo->setName(std::move(newName));
    source.sendMessage(_lang().get("commands.bossbar.set.name.success", {displayName}));
    return 0;
}

i32 BossBarCommand::_setColor(CommandContext<ServerCommandSource>& context, const std::string& colorStr)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<ResourceLocation>("id").toString();

    auto* bossInfo = _findBossBar(source, idStr);
    if (bossInfo == nullptr) {
        return 0;
    }

    // 颜色名在命令树层已由 StringArgumentType 解析；未知名称回退为 White，
    // 若与当前颜色相同则按原版语义报告 unchanged。
    const server::BossInfoColor color = server::bossInfoColorFromName(colorStr);
    if (bossInfo->color() == color) {
        source.sendError(_lang().get("commands.bossbar.set.color.unchanged"));
        return 0;
    }

    const std::string displayName = _displayName(*bossInfo);
    bossInfo->setColor(color);
    source.sendMessage(_lang().get("commands.bossbar.set.color.success", {displayName}));
    return 0;
}

i32 BossBarCommand::_setStyle(CommandContext<ServerCommandSource>& context, const std::string& styleStr)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<ResourceLocation>("id").toString();

    auto* bossInfo = _findBossBar(source, idStr);
    if (bossInfo == nullptr) {
        return 0;
    }

    const server::BossInfoOverlay overlay = server::bossInfoOverlayFromName(styleStr);
    if (bossInfo->overlay() == overlay) {
        source.sendError(_lang().get("commands.bossbar.set.style.unchanged"));
        return 0;
    }

    const std::string displayName = _displayName(*bossInfo);
    bossInfo->setOverlay(overlay);
    source.sendMessage(_lang().get("commands.bossbar.set.style.success", {displayName}));
    return 0;
}

i32 BossBarCommand::_setValue(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<ResourceLocation>("id").toString();
    const i32 value = context.getArgument<i32>("value");

    auto* bossInfo = _findBossBar(source, idStr);
    if (bossInfo == nullptr) {
        return 0;
    }

    if (bossInfo->value() == value) {
        source.sendError(_lang().get("commands.bossbar.set.value.unchanged"));
        return 0;
    }

    const std::string displayName = _displayName(*bossInfo);
    bossInfo->setValue(value);
    source.sendMessage(_lang().get("commands.bossbar.set.value.success", {displayName, std::to_string(value)}));
    return value;
}

i32 BossBarCommand::_setMax(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<ResourceLocation>("id").toString();
    const i32 max = context.getArgument<i32>("max");

    auto* bossInfo = _findBossBar(source, idStr);
    if (bossInfo == nullptr) {
        return 0;
    }

    if (bossInfo->max() == max) {
        source.sendError(_lang().get("commands.bossbar.set.max.unchanged"));
        return 0;
    }

    const std::string displayName = _displayName(*bossInfo);
    bossInfo->setMax(max);
    source.sendMessage(_lang().get("commands.bossbar.set.max.success", {displayName, std::to_string(max)}));
    return max;
}

i32 BossBarCommand::_setVisible(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<ResourceLocation>("id").toString();
    const bool visible = context.getArgument<bool>("visible");

    auto* bossInfo = _findBossBar(source, idStr);
    if (bossInfo == nullptr) {
        return 0;
    }

    if (bossInfo->visible() == visible) {
        source.sendError(_lang().get(visible ? "commands.bossbar.set.visibility.unchanged.visible"
                                             : "commands.bossbar.set.visibility.unchanged.hidden"));
        return 0;
    }

    const std::string displayName = _displayName(*bossInfo);
    bossInfo->setVisible(visible);
    source.sendMessage(_lang().get(
        visible ? "commands.bossbar.set.visible.success.visible" : "commands.bossbar.set.visible.success.hidden",
        {displayName}));
    return 0;
}

i32 BossBarCommand::_setPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<ResourceLocation>("id").toString();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    auto* bossInfo = _findBossBar(source, idStr);
    if (bossInfo == nullptr) {
        return 0;
    }

    // 解析目标选择器到在线玩家实体（旁观者不参与 Boss 栏可见列表，与 vanilla 一致）。
    std::vector<ServerPlayer*> players;
    std::vector<std::string> names;
    for (PlayerId playerId : support::resolvePlayerIds(source, selector)) {
        Player* player = source.server()->playerEntityManager().getPlayerEntity(playerId, *source.world());
        if (player == nullptr) {
            continue;
        }
        ServerPlayer* serverPlayer = player->asServerPlayer();
        if (serverPlayer == nullptr || serverPlayer->isSpectator()) {
            continue;
        }
        players.push_back(serverPlayer);
        const server::ServerPlayerData* data = source.server()->playerManager().getPlayer(playerId);
        names.push_back(data != nullptr ? data->username : std::string());
    }

    const std::string displayName = _displayName(*bossInfo);
    if (!bossInfo->setPlayers(players)) {
        source.sendError(_lang().get("commands.bossbar.set.players.unchanged"));
        return 0;
    }

    if (players.empty()) {
        source.sendMessage(_lang().get("commands.bossbar.set.players.success.none", {displayName}));
    } else {
        source.sendMessage(_lang().get("commands.bossbar.set.players.success.some",
            {displayName, std::to_string(players.size()), _joinPlayerNames(names)}));
    }
    return static_cast<i32>(players.size());
}

i32 BossBarCommand::_getValue(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<ResourceLocation>("id").toString();

    auto* bossInfo = _findBossBar(source, idStr);
    if (bossInfo == nullptr) {
        return 0;
    }

    source.sendMessage(
        _lang().get("commands.bossbar.get.value", {_displayName(*bossInfo), std::to_string(bossInfo->value())}));
    return bossInfo->value();
}

i32 BossBarCommand::_getMax(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<ResourceLocation>("id").toString();

    auto* bossInfo = _findBossBar(source, idStr);
    if (bossInfo == nullptr) {
        return 0;
    }

    source.sendMessage(
        _lang().get("commands.bossbar.get.max", {_displayName(*bossInfo), std::to_string(bossInfo->max())}));
    return bossInfo->max();
}

i32 BossBarCommand::_getVisible(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<ResourceLocation>("id").toString();

    auto* bossInfo = _findBossBar(source, idStr);
    if (bossInfo == nullptr) {
        return 0;
    }

    const bool visible = bossInfo->visible();
    source.sendMessage(
        _lang().get(visible ? "commands.bossbar.get.visible.visible" : "commands.bossbar.get.visible.hidden",
            {_displayName(*bossInfo)}));
    return visible ? 1 : 0;
}

i32 BossBarCommand::_getPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string idStr = context.getArgument<ResourceLocation>("id").toString();

    auto* bossInfo = _findBossBar(source, idStr);
    if (bossInfo == nullptr) {
        return 0;
    }

    const std::string displayName = _displayName(*bossInfo);
    const auto& playerIds = bossInfo->players();
    if (playerIds.empty()) {
        source.sendMessage(_lang().get("commands.bossbar.get.players.none", {displayName}));
        return 0;
    }

    std::vector<std::string> names;
    names.reserve(playerIds.size());
    for (PlayerId playerId : playerIds) {
        const server::ServerPlayerData* data = source.server()->playerManager().getPlayer(playerId);
        if (data != nullptr) {
            names.push_back(data->username);
        }
    }

    source.sendMessage(_lang().get(
        "commands.bossbar.get.players.some", {displayName, std::to_string(playerIds.size()), _joinPlayerNames(names)}));
    return static_cast<i32>(playerIds.size());
}

} // namespace command
} // namespace mc
