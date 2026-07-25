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
 * IMPLIED, INCLUDING ANY OF FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "StopSoundCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"

#include <spdlog/spdlog.h>

namespace mc {
namespace command {

namespace {
/**
 * @brief 发送 StopSound IR 包给指定玩家
 */
void sendStopSoundPacket(server::core::ConnectionManager& connMgr,
    PlayerId playerId,
    const std::optional<ResourceLocation>& soundId,
    const std::optional<sound::SoundCategory>& category)
{
    // 1.21.11 StopSound：flags bit0=HAS_SOURCE bit1=HAS_SOUND
    mc::network::ir::play::StopSound pkt;
    pkt.flags = 0;
    if (category.has_value()) {
        pkt.flags |= 0x01;
        pkt.source = static_cast<i32>(*category);
    }
    if (soundId.has_value()) {
        pkt.flags |= 0x02;
        pkt.name = soundId->toString();
    }

    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
    connMgr.sendToPlayer(playerId, packet);
}

/**
 * @brief 构建停止声音的反馈消息
 *
 * 根据 category 和 soundId 是否为空，返回不同格式的反馈文本。
 */
std::string buildStopSoundFeedback(
    const std::optional<sound::SoundCategory>& category, const std::optional<ResourceLocation>& soundId)
{
    if (category.has_value()) {
        std::string categoryName(sound::getSoundCategoryName(category.value()));
        if (soundId.has_value()) {
            return fmt::format("Stopped sound '{}' in category '{}'", soundId.value().toString(), categoryName);
        }
        return fmt::format("Stopped all sounds in category '{}'", categoryName);
    }

    if (soundId.has_value()) {
        return fmt::format("Stopped sound '{}' across all categories", soundId.value().toString());
    }

    return "Stopped all sounds";
}

} // namespace

void StopSoundCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto stopsoundNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("stopsound");
    stopsoundNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(stopsoundNode,
        support::makeMetadata(
            "Stops playing a sound effect.", "/stopsound <player> [<source>] [<sound>]", 2, {}, true));

    // /stopsound <player> - 停止所有声音
    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::players());
    playerNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _stopAllSounds(ctx); });

    // /stopsound <player> * [<sound>] - 通配符：停止所有类别的声音
    auto wildcardNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("*");
    wildcardNode->setCommand(
        [](CommandContext<ServerCommandSource>& ctx) { return _stopSounds(ctx, std::nullopt, std::nullopt); });
    auto wildcardSoundNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "sound", ResourceLocationArgumentType::resourceLocation());
    wildcardSoundNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        const ResourceLocation& soundId = ctx.getArgument<ResourceLocation>("sound");
        return _stopSounds(ctx, std::nullopt, soundId);
    });
    wildcardNode->addChild(wildcardSoundNode);
    playerNode->addChild(wildcardNode);

    // 为每个声源类别添加命令节点
    for (u8 i = 0; i < static_cast<u8>(sound::SoundCategory::Count); ++i) {
        auto category = static_cast<sound::SoundCategory>(i);
        std::string categoryName(sound::getSoundCategoryName(category));

        auto sourceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(categoryName);

        // /stopsound <player> <source> - 停止该类别所有声音
        sourceNode->setCommand(
            [category](CommandContext<ServerCommandSource>& ctx) { return _stopSounds(ctx, category, std::nullopt); });

        // /stopsound <player> <source> <sound> - 停止特定声音
        auto soundNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
            "sound", ResourceLocationArgumentType::resourceLocation());
        soundNode->setCommand([category](CommandContext<ServerCommandSource>& ctx) {
            const ResourceLocation& soundId = ctx.getArgument<ResourceLocation>("sound");
            return _stopSounds(ctx, category, soundId);
        });

        sourceNode->addChild(soundNode);
        playerNode->addChild(sourceNode);
    }

    stopsoundNode->addChild(playerNode);
    dispatcher.registerCommand(stopsoundNode);
}

i32 StopSoundCommand::_stopAllSounds(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    auto* server = source.server();
    auto& connMgr = server->connectionManager();

    for (PlayerId playerId : playerIds) {
        sendStopSoundPacket(connMgr, playerId, std::nullopt, std::nullopt);
    }

    source.sendMessage(buildStopSoundFeedback(std::nullopt, std::nullopt));
    return static_cast<i32>(playerIds.size());
}

i32 StopSoundCommand::_stopSounds(CommandContext<ServerCommandSource>& context,
    std::optional<sound::SoundCategory> category,
    std::optional<ResourceLocation> soundId)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    auto* server = source.server();
    auto& connMgr = server->connectionManager();

    for (PlayerId playerId : playerIds) {
        sendStopSoundPacket(connMgr, playerId, soundId, category);
    }

    source.sendMessage(buildStopSoundFeedback(category, soundId));
    return static_cast<i32>(playerIds.size());
}

} // namespace command
} // namespace mc
