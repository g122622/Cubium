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

#include "TitleCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/TimeArgument.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/util/text/ComponentNbtSerialization.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include <sstream>

namespace mc {
namespace command {

namespace {

/// 把文本（JSON 字符串）转为 1.21.11 Component NBT wire 字节。
/// 对齐 vanilla ComponentSerialization.TRUSTED_STREAM_CODEC：解析 JSON 为 ITextComponent 后序列化为
/// NBT（可折叠纯文本→StringTag，复杂组件→CompoundTag），NBT 自定界无外层 VarInt 长度。
/// 解析失败时降级为纯文本 StringTag（把原 JSON 当纯文本承载）。
std::vector<u8> titleTextToBytes(const std::string& text)
{
    return text::parseJsonComponentToNbtBytes(text);
}

/// 构造一个 SetTitleText IR 包
mc::network::ir::IrPacket makeTitleTextPacket(const std::string& jsonText)
{
    namespace irplay = mc::network::ir::play;
    irplay::SetTitleText evt;
    evt.text = titleTextToBytes(jsonText);
    return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{irplay::SetTitleText{std::move(evt)}}};
}

/// 构造一个 SetSubtitleText IR 包
mc::network::ir::IrPacket makeSubtitleTextPacket(const std::string& jsonText)
{
    namespace irplay = mc::network::ir::play;
    irplay::SetSubtitleText evt;
    evt.text = titleTextToBytes(jsonText);
    return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{irplay::SetSubtitleText{std::move(evt)}}};
}

/// 构造一个 SetActionBarText IR 包
mc::network::ir::IrPacket makeActionBarTextPacket(const std::string& jsonText)
{
    namespace irplay = mc::network::ir::play;
    irplay::SetActionBarText evt;
    evt.text = titleTextToBytes(jsonText);
    return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{irplay::SetActionBarText{std::move(evt)}}};
}

/// 构造一个 SetTitlesAnimation IR 包
mc::network::ir::IrPacket makeTitlesAnimationPacket(i32 fadeIn, i32 stay, i32 fadeOut)
{
    namespace irplay = mc::network::ir::play;
    irplay::SetTitlesAnimation evt;
    evt.fadeIn = fadeIn;
    evt.stay = stay;
    evt.fadeOut = fadeOut;
    return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{irplay::SetTitlesAnimation{std::move(evt)}}};
}

/// 构造一个 ClearTitles IR 包
mc::network::ir::IrPacket makeClearTitlesPacket(bool resetTimes)
{
    namespace irplay = mc::network::ir::play;
    irplay::ClearTitles evt;
    evt.resetTimes = resetTimes;
    return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{irplay::ClearTitles{std::move(evt)}}};
}

/// 广播一个 IR 包给所有指定玩家，返回成功投递数。
i32 broadcastIrPacket(
    ServerCommandSource& source, const std::vector<PlayerId>& playerIds, const mc::network::ir::IrPacket& packet)
{
    auto* server = source.server();
    if (!server) {
        return 0;
    }

    auto& connMgr = server->connectionManager();
    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        if (connMgr.sendToPlayer(playerId, packet)) {
            successCount++;
        }
    }
    return successCount;
}
} // namespace

void TitleCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto titleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("title");
    titleNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(titleNode,
        support::makeMetadata("Controls screen title display.",
            "/title <player> (clear|reset|title|subtitle|actionbar|times) ...",
            2,
            {},
            true));

    // /title <player>
    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::players());

    // clear 子命令
    auto clearNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");
    clearNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _clearTitle(ctx); });

    // reset 子命令
    auto resetNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reset");
    resetNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _resetTitle(ctx); });

    // title <json> 子命令
    auto titleTextNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("title");
    auto titleJsonNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "json", StringArgumentType::greedyString());
    titleJsonNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setTitle(ctx); });
    titleTextNode->addChild(titleJsonNode);

    // subtitle <json> 子命令
    auto subtitleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("subtitle");
    auto subtitleJsonNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "json", StringArgumentType::greedyString());
    subtitleJsonNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setSubtitle(ctx); });
    subtitleNode->addChild(subtitleJsonNode);

    // actionbar <json> 子命令
    auto actionbarNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("actionbar");
    auto actionbarJsonNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "json", StringArgumentType::greedyString());
    actionbarJsonNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setActionbar(ctx); });
    actionbarNode->addChild(actionbarJsonNode);

    // times <fadeIn> <stay> <fadeOut> 子命令
    auto timesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("times");
    auto fadeInNode =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("fadeIn", TimeArgumentType::time());
    auto stayNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("stay", TimeArgumentType::time());
    auto fadeOutNode =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("fadeOut", TimeArgumentType::time());
    fadeOutNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setTimes(ctx); });

    fadeInNode->addChild(stayNode);
    stayNode->addChild(fadeOutNode);
    timesNode->addChild(fadeInNode);

    playerNode->addChild(clearNode);
    playerNode->addChild(resetNode);
    playerNode->addChild(titleTextNode);
    playerNode->addChild(subtitleNode);
    playerNode->addChild(actionbarNode);
    playerNode->addChild(timesNode);
    titleNode->addChild(playerNode);

    dispatcher.registerCommand(titleNode);
}

i32 TitleCommand::_clearTitle(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 创建并广播清除标题包
    auto packet = makeClearTitlesPacket(false);
    return broadcastIrPacket(source, playerIds, packet);
}

i32 TitleCommand::_resetTitle(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 创建并广播重置标题包
    auto packet = makeClearTitlesPacket(true);
    return broadcastIrPacket(source, playerIds, packet);
}

i32 TitleCommand::_setTitle(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 获取JSON文本参数
    const std::string& jsonText = context.getArgument<std::string>("json");

    // 创建并广播主标题包
    auto packet = makeTitleTextPacket(jsonText);
    return broadcastIrPacket(source, playerIds, packet);
}

i32 TitleCommand::_setSubtitle(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 获取JSON文本参数
    const std::string& jsonText = context.getArgument<std::string>("json");

    // 创建并广播副标题包
    auto packet = makeSubtitleTextPacket(jsonText);
    return broadcastIrPacket(source, playerIds, packet);
}

i32 TitleCommand::_setActionbar(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 获取JSON文本参数
    const std::string& jsonText = context.getArgument<std::string>("json");

    // 创建并广播动作栏包
    auto packet = makeActionBarTextPacket(jsonText);
    return broadcastIrPacket(source, playerIds, packet);
}

i32 TitleCommand::_setTimes(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 获取时间参数（单位：tick）
    i32 fadeIn = context.getArgument<i32>("fadeIn");
    i32 stay = context.getArgument<i32>("stay");
    i32 fadeOut = context.getArgument<i32>("fadeOut");

    // 创建并广播时间设置包
    auto packet = makeTitlesAnimationPacket(fadeIn, stay, fadeOut);
    return broadcastIrPacket(source, playerIds, packet);
}

} // namespace command
} // namespace mc
