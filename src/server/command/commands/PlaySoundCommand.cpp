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

#include "PlaySoundCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/math/Vector3.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <glm/ext/vector_float3.hpp>

namespace mc {
namespace command {

namespace {
/**
 * @brief 从命令参数解析声源类别
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
    if (name == "ui") return sound::SoundCategory::UI;
    return sound::SoundCategory::Master;
}

/**
 * @brief 发送 PlaySound IR 包给指定玩家
 */
void sendPlaySoundPacket(server::core::ConnectionManager& connMgr,
    PlayerId playerId,
    const ResourceLocation& soundId,
    sound::SoundCategory category,
    const glm::vec3& position,
    f32 volume,
    f32 pitch)
{
    // 1.21.11 ClientboundSoundPacket：Holder<SoundEvent> + source + 坐标(×8 整数) +
    //   volume + pitch + seed。坐标按 ×8 截断（Java writeInt）。
    // soundHolder 用内联 SoundEvent（direct=true，identifier=soundId），对齐 vanilla wire。
    //   seed 暂用固定值 0，后续应接入确定性种子源。
    mc::network::ir::play::PlaySound pkt;
    pkt.soundHolder.direct = true;
    pkt.soundHolder.identifier = soundId.toString();
    pkt.soundHolder.hasFixedRange = false;
    pkt.source = static_cast<i32>(category);
    pkt.x = static_cast<i32>(position.x * 8.0f);
    pkt.y = static_cast<i32>(position.y * 8.0f);
    pkt.z = static_cast<i32>(position.z * 8.0f);
    pkt.volume = volume;
    pkt.pitch = pitch;
    pkt.seed = 0;

    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
    connMgr.sendToPlayer(playerId, packet);
}
} // namespace

void PlaySoundCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto playsoundNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("playsound");
    playsoundNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(playsoundNode,
        support::makeMetadata("Plays a sound effect.",
            "/playsound <sound> <source> <player> [<pos>] [<volume>] [<pitch>] [<minimumVolume>]",
            2,
            {},
            true));

    // /playsound <sound>
    auto soundNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "sound", ResourceLocationArgumentType::resourceLocation());

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
    auto uiNode = createSourceNode("ui");

    // 为每个声源添加目标节点
    for (auto& sourceNode : {masterNode,
             musicNode,
             recordNode,
             weatherNode,
             blockNode,
             hostileNode,
             neutralNode,
             playerSoundNode,
             ambientNode,
             voiceNode,
             uiNode}) {
        auto targetNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
            "player", EntityArgumentType::player());
        targetNode->setCommand([sourceName = sourceNode->getName()](CommandContext<ServerCommandSource>& ctx) {
            return _playSoundDefault(ctx, parseSoundCategory(sourceName));
        });

        auto posNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
            "pos", Vec3ArgumentType::vec3());
        posNode->setCommand([sourceName = sourceNode->getName()](CommandContext<ServerCommandSource>& ctx) {
            return _playSoundAtPosition(ctx, parseSoundCategory(sourceName));
        });

        // volume 节点
        auto volumeNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
            "volume", FloatArgumentType::floatArg(0.0f, 1000.0f));
        volumeNode->setCommand([sourceName = sourceNode->getName()](CommandContext<ServerCommandSource>& ctx) {
            return _playSoundWithVolume(ctx, parseSoundCategory(sourceName));
        });

        // pitch 节点
        auto pitchNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
            "pitch", FloatArgumentType::floatArg(0.0f, 2.0f));
        pitchNode->setCommand([sourceName = sourceNode->getName()](CommandContext<ServerCommandSource>& ctx) {
            return _playSoundWithPitch(ctx, parseSoundCategory(sourceName));
        });

        // minimumVolume 节点
        auto minVolumeNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
            "minimumVolume", FloatArgumentType::floatArg(0.0f, 1.0f));
        minVolumeNode->setCommand([sourceName = sourceNode->getName()](CommandContext<ServerCommandSource>& ctx) {
            return _playSoundWithMinVolume(ctx, parseSoundCategory(sourceName));
        });

        // 构建参数链
        pitchNode->addChild(minVolumeNode);
        volumeNode->addChild(pitchNode);
        posNode->addChild(volumeNode);
        targetNode->addChild(posNode);
        sourceNode->addChild(targetNode);
        soundNode->addChild(sourceNode);
    }

    playsoundNode->addChild(soundNode);
    dispatcher.registerCommand(playsoundNode);
}

i32 PlaySoundCommand::_playSoundDefault(CommandContext<ServerCommandSource>& context, sound::SoundCategory category)
{
    auto& source = context.getSource();

    const ResourceLocation& soundId = context.getArgument<ResourceLocation>("sound");
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    auto* server = source.server();
    auto& playerManager = server->playerManager();
    auto& connMgr = server->connectionManager();

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        auto* playerData = playerManager.getPlayer(playerId);
        if (!playerData) {
            continue;
        }

        // 在玩家位置播放声音
        glm::vec3 pos(playerData->x, playerData->y, playerData->z);
        sendPlaySoundPacket(connMgr, playerId, soundId, category, pos, 1.0f, 1.0f);
        successCount++;
    }

    return successCount;
}

i32 PlaySoundCommand::_playSoundAtPosition(CommandContext<ServerCommandSource>& context, sound::SoundCategory category)
{
    auto& source = context.getSource();

    const ResourceLocation& soundId = context.getArgument<ResourceLocation>("sound");
    const auto& selector = context.getArgument<EntitySelector>("player");
    const Vector3d pos = Vec3ArgumentType::getVec3(context, "pos", source);
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    auto* server = source.server();
    auto& connMgr = server->connectionManager();
    glm::vec3 soundPos(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        sendPlaySoundPacket(connMgr, playerId, soundId, category, soundPos, 1.0f, 1.0f);
        successCount++;
    }

    return successCount;
}

i32 PlaySoundCommand::_playSoundWithVolume(CommandContext<ServerCommandSource>& context, sound::SoundCategory category)
{
    auto& source = context.getSource();

    const ResourceLocation& soundId = context.getArgument<ResourceLocation>("sound");
    const auto& selector = context.getArgument<EntitySelector>("player");
    const Vector3d pos = Vec3ArgumentType::getVec3(context, "pos", source);
    f32 volume = context.getArgument<f32>("volume");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    auto* server = source.server();
    auto& connMgr = server->connectionManager();
    glm::vec3 soundPos(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        sendPlaySoundPacket(connMgr, playerId, soundId, category, soundPos, volume, 1.0f);
        successCount++;
    }

    return successCount;
}

i32 PlaySoundCommand::_playSoundWithPitch(CommandContext<ServerCommandSource>& context, sound::SoundCategory category)
{
    auto& source = context.getSource();

    const ResourceLocation& soundId = context.getArgument<ResourceLocation>("sound");
    const auto& selector = context.getArgument<EntitySelector>("player");
    const Vector3d pos = Vec3ArgumentType::getVec3(context, "pos", source);
    f32 volume = context.getArgument<f32>("volume");
    f32 pitch = context.getArgument<f32>("pitch");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    auto* server = source.server();
    auto& connMgr = server->connectionManager();
    glm::vec3 soundPos(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        sendPlaySoundPacket(connMgr, playerId, soundId, category, soundPos, volume, pitch);
        successCount++;
    }

    return successCount;
}

i32 PlaySoundCommand::_playSoundWithMinVolume(
    CommandContext<ServerCommandSource>& context, sound::SoundCategory category)
{
    auto& source = context.getSource();

    const ResourceLocation& soundId = context.getArgument<ResourceLocation>("sound");
    const auto& selector = context.getArgument<EntitySelector>("player");
    const Vector3d pos = Vec3ArgumentType::getVec3(context, "pos", source);
    f32 volume = context.getArgument<f32>("volume");
    f32 pitch = context.getArgument<f32>("pitch");
    f32 minVolume = context.getArgument<f32>("minimumVolume");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    auto* server = source.server();
    auto& playerManager = server->playerManager();
    auto& connMgr = server->connectionManager();
    glm::vec3 soundPos(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));

    // 计算声音可听距离（MC 原版公式：volume * 16）
    f32 audibleRange = std::max(volume, 1.0f) * 16.0f;
    f32 audibleRangeSq = audibleRange * audibleRange;

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        auto* playerData = playerManager.getPlayer(playerId);
        if (!playerData) {
            continue;
        }

        // 计算玩家到声音位置的距离
        f32 dx = static_cast<f32>(playerData->x - pos.x);
        f32 dy = static_cast<f32>(playerData->y - pos.y);
        f32 dz = static_cast<f32>(playerData->z - pos.z);
        f32 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq <= audibleRangeSq) {
            // 玩家在可听范围内，正常播放
            sendPlaySoundPacket(connMgr, playerId, soundId, category, soundPos, volume, pitch);
        } else if (minVolume > 0.0f) {
            // 玩家超出可听范围，但使用最小音量
            // 在玩家位置以最小音量播放
            glm::vec3 playerPos(playerData->x, playerData->y, playerData->z);
            sendPlaySoundPacket(connMgr, playerId, soundId, category, playerPos, minVolume, pitch);
        }

        successCount++;
    }

    return successCount;
}

} // namespace command
} // namespace mc
