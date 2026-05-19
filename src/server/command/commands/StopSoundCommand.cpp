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

#include "StopSoundCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/core/Types.hpp"
#include "common/sound/network/SoundPackets.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"

namespace mc {
namespace command {

namespace {
/**
 * @brief 从字符串解析声源类别
 */
sound::SoundCategory parseSoundCategory(const std::string& name)
{
    if (name == "master") return sound::SoundCategory::Master;
    if (name == "music") return sound::SoundCategory::Music;
    if (name == "record") return sound::SoundCategory::Records;
    if (name == "weather") return sound::SoundCategory::Weather;
    if (name == "block" || name == "blocks") return sound::SoundCategory::Blocks;
    if (name == "hostile") return sound::SoundCategory::Hostile;
    if (name == "neutral") return sound::SoundCategory::Neutral;
    if (name == "player" || name == "players") return sound::SoundCategory::Players;
    if (name == "ambient") return sound::SoundCategory::Ambient;
    if (name == "voice") return sound::SoundCategory::Voice;
    return sound::SoundCategory::Master;
}

/**
 * @brief 发送 StopSoundPacket 给指定玩家
 */
void sendStopSoundPacket(server::core::ConnectionManager& connMgr,
    PlayerId playerId,
    const std::optional<ResourceLocation>& soundId,
    const std::optional<sound::SoundCategory>& category)
{
    sound::StopSoundPacket packet(soundId, category);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize StopSoundPacket: {}", result.error().message());
        return;
    }

    connMgr.sendPacketToPlayer(playerId, network::PacketType::StopSound, result.value());
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
    playerNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        auto& source = ctx.getSource();
        const auto& selector = ctx.getArgument<EntitySelector>("player");
        auto playerIds = support::resolvePlayerIds(source, selector);

        if (playerIds.empty()) {
            source.sendError("No matching players were found");
            return 0;
        }

        auto* server = source.server();
        auto& connMgr = server->connectionManager();

        // 发送 StopSoundPacket 停止所有声音（不指定声音ID和类别）
        for (PlayerId playerId : playerIds) {
            sendStopSoundPacket(connMgr, playerId, std::nullopt, std::nullopt);
        }

        return static_cast<i32>(playerIds.size());
    });

    // 声源子节点
    auto createSourceNode = [](const char* name) {
        return std::make_shared<LiteralCommandNode<ServerCommandSource>>(name);
    };

    auto masterNode = createSourceNode("master");
    auto musicNode = createSourceNode("music");
    auto recordNode = createSourceNode("record");
    auto weatherNode = createSourceNode("weather");
    auto blockNode = createSourceNode("block");
    auto hostileNode = createSourceNode("hostile");
    auto neutralNode = createSourceNode("neutral");
    auto playerSoundNode = createSourceNode("player");
    auto ambientNode = createSourceNode("ambient");
    auto voiceNode = createSourceNode("voice");

    // 为每个声源添加命令
    for (auto& sourceNode : {masterNode,
             musicNode,
             recordNode,
             weatherNode,
             blockNode,
             hostileNode,
             neutralNode,
             playerSoundNode,
             ambientNode,
             voiceNode}) {
        // /stopsound <player> <source> - 停止该类别所有声音
        sourceNode->setCommand([sourceName = sourceNode->getName()](CommandContext<ServerCommandSource>& ctx) {
            auto& source = ctx.getSource();
            const auto& selector = ctx.getArgument<EntitySelector>("player");
            auto playerIds = support::resolvePlayerIds(source, selector);

            if (playerIds.empty()) {
                source.sendError("No matching players were found");
                return 0;
            }

            auto* server = source.server();
            auto& connMgr = server->connectionManager();
            auto category = parseSoundCategory(sourceName);

            // 发送 StopSoundPacket 停止指定类别的声音
            for (PlayerId playerId : playerIds) {
                sendStopSoundPacket(connMgr, playerId, std::nullopt, category);
            }

            return static_cast<i32>(playerIds.size());
        });

        // /stopsound <player> <source> <sound> - 停止特定声音
        auto soundNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
            "sound", ResourceLocationArgumentType::resourceLocation());
        soundNode->setCommand([sourceName = sourceNode->getName()](CommandContext<ServerCommandSource>& ctx) {
            auto& source = ctx.getSource();
            const auto& selector = ctx.getArgument<EntitySelector>("player");
            const ResourceLocation& soundId = ctx.getArgument<ResourceLocation>("sound");
            auto playerIds = support::resolvePlayerIds(source, selector);

            if (playerIds.empty()) {
                source.sendError("No matching players were found");
                return 0;
            }

            auto* server = source.server();
            auto& connMgr = server->connectionManager();
            auto category = parseSoundCategory(sourceName);

            // 发送 StopSoundPacket 停止指定声音
            for (PlayerId playerId : playerIds) {
                sendStopSoundPacket(connMgr, playerId, soundId, category);
            }

            return static_cast<i32>(playerIds.size());
        });

        sourceNode->addChild(soundNode);
        playerNode->addChild(sourceNode);
    }

    stopsoundNode->addChild(playerNode);
    dispatcher.registerCommand(stopsoundNode);
}

i32 StopSoundCommand::stopAllSounds(CommandContext<ServerCommandSource>& context)
{
    // 已在 registerTo 中内联实现，此方法不再使用
    return 0;
}

i32 StopSoundCommand::stopSourceSounds(CommandContext<ServerCommandSource>& context)
{
    // 已在 registerTo 中内联实现，此方法不再使用
    return 0;
}

i32 StopSoundCommand::stopSpecificSound(CommandContext<ServerCommandSource>& context)
{
    // 已在 registerTo 中内联实现，此方法不再使用
    return 0;
}

} // namespace command
} // namespace mc
