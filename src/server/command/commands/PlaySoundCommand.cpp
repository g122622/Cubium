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
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/text/ComponentFactory.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/CommandSuggestionProviders.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/network/outbound/PacketSender.hpp"
#include "server/player/lifecycle/PlayerManager.hpp"
#include "server/player/runtime/ServerPlayerData.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <glm/ext/vector_float3.hpp>

namespace mc {
namespace command {

namespace {
/**
 * @brief 从命令参数解析声源类别
 */
sound::SoundCategory parseSoundCategory(const std::string& name)
{
    if (name == "master") {
        return sound::SoundCategory::Master;
    }
    if (name == "music") {
        return sound::SoundCategory::Music;
    }
    if (name == "record") {
        return sound::SoundCategory::Records;
    }
    if (name == "weather") {
        return sound::SoundCategory::Weather;
    }
    if (name == "block" || name == "blocks") {
        return sound::SoundCategory::Blocks;
    }
    if (name == "hostile") {
        return sound::SoundCategory::Hostile;
    }
    if (name == "neutral") {
        return sound::SoundCategory::Neutral;
    }
    if (name == "player" || name == "players") {
        return sound::SoundCategory::Players;
    }
    if (name == "ambient") {
        return sound::SoundCategory::Ambient;
    }
    if (name == "voice") {
        return sound::SoundCategory::Voice;
    }
    if (name == "ui") {
        return sound::SoundCategory::UI;
    }
    return sound::SoundCategory::Master;
}

/**
 * @brief 发送 PlaySound IR 包给指定玩家
 */
void sendPlaySoundPacket(server::PacketSender& connMgr,
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
    playsoundNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(playsoundNode,
        support::makeMetadata("Plays a sound effect.",
            "/playsound <sound> <source> <targets> [<pos>] [<volume>] [<pitch>] [<minVolume>]",
            2,
            {},
            true));

    // /playsound <sound>
    auto soundNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "sound", ResourceLocationArgumentType::resourceLocation());
    // 原版挂的是 minecraft:available_sounds：音效事件由服务端按已加载资源包计算，走 ask_server。
    soundNode->setCustomSuggestions(
        std::make_shared<RegisteredSuggestionProvider<ServerCommandSource>>("minecraft:available_sounds"));
    // 只有音效名是最短完整命令：`/playsound <sound>` 按 master 声源、全部玩家播放。
    soundNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _playSoundImpl(ctx, sound::SoundCategory::Master);
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
        // 原版声源字面量本身也是完整命令（目标缺省为全部玩家）。不捕获节点本身以免形成引用环。
        sourceNode->setCommand(
            [audioCategory = parseSoundCategory(sourceNode->getName())](CommandContext<ServerCommandSource>& ctx) {
                return _playSoundImpl(ctx, audioCategory);
            });

        // 原版参数名是 targets（EntityArgument.players()），不是 player。
        auto targetNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
            "targets", EntityArgumentType::players());
        targetNode->setCommand([sourceName = sourceNode->getName()](CommandContext<ServerCommandSource>& ctx) {
            return _playSoundImpl(ctx, parseSoundCategory(sourceName));
        });

        auto posNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
            "pos", Vec3ArgumentType::vec3());
        posNode->setCommand([sourceName = sourceNode->getName()](CommandContext<ServerCommandSource>& ctx) {
            return _playSoundImpl(ctx, parseSoundCategory(sourceName));
        });

        // volume 节点
        auto volumeNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
            "volume", FloatArgumentType::floatArg(0.0f));
        volumeNode->setCommand([sourceName = sourceNode->getName()](CommandContext<ServerCommandSource>& ctx) {
            return _playSoundImpl(ctx, parseSoundCategory(sourceName));
        });

        // pitch 节点
        auto pitchNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
            "pitch", FloatArgumentType::floatArg(0.0f, 2.0f));
        pitchNode->setCommand([sourceName = sourceNode->getName()](CommandContext<ServerCommandSource>& ctx) {
            return _playSoundImpl(ctx, parseSoundCategory(sourceName));
        });

        // minimumVolume 节点
        auto minVolumeNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
            "minVolume", FloatArgumentType::floatArg(0.0f, 1.0f));
        minVolumeNode->setCommand([sourceName = sourceNode->getName()](CommandContext<ServerCommandSource>& ctx) {
            return _playSoundImpl(ctx, parseSoundCategory(sourceName));
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

i32 PlaySoundCommand::_playSoundImpl(CommandContext<ServerCommandSource>& context, sound::SoundCategory category)
{
    auto& source = context.getSource();
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    const ResourceLocation& soundId = context.getArgument<ResourceLocation>("sound");

    // 未显式给出 targets 时，原版对执行命令的玩家播放（控制台执行则没有目标）。
    std::vector<PlayerId> playerIds;
    if (context.hasArgument("targets")) {
        playerIds = support::resolvePlayerIds(source, context.getArgument<EntitySelector>("targets"));
    } else if (source.isPlayer()) {
        playerIds.push_back(source.playerId());
    }

    const Vector3d pos =
        context.hasArgument("pos") ? Vec3ArgumentType::getVec3(context, "pos", source) : source.position();
    const f32 volume = context.hasArgument("volume") ? context.getArgument<f32>("volume") : 1.0f;
    const f32 pitch = context.hasArgument("pitch") ? context.getArgument<f32>("pitch") : 1.0f;
    const f32 minVolume = context.hasArgument("minVolume") ? context.getArgument<f32>("minVolume") : 0.0f;

    // 可听半径：原版 SoundEvent.getRange(volume) = max(volume, 1) × 16。
    const f64 audibleRange = static_cast<f64>(std::max(volume, 1.0f)) * 16.0;
    const f64 audibleRangeSq = audibleRange * audibleRange;

    auto& playerManager = server->playerManager();
    auto& connMgr = server->packetSender();

    std::vector<PlayerId> heardPlayers;
    for (PlayerId playerId : playerIds) {
        const server::ServerPlayerData* playerData = playerManager.getPlayer(playerId);
        if (playerData == nullptr || !playerData->loggedIn) {
            continue;
        }
        if (server->getPlayerWorld(playerId) != source.world()) {
            continue;
        }

        const f64 dx = pos.x - static_cast<f64>(playerData->x);
        const f64 dy = pos.y - static_cast<f64>(playerData->y);
        const f64 dz = pos.z - static_cast<f64>(playerData->z);
        const f64 distSq = dx * dx + dy * dy + dz * dz;

        glm::vec3 soundPos(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));
        f32 playedVolume = volume;
        if (distSq > audibleRangeSq) {
            // 超出可听半径：没有最小音量就完全听不到；否则把声源挪到玩家外侧 2 格、按最小音量播放。
            if (minVolume <= 0.0f) {
                continue;
            }
            const f64 distance = std::sqrt(distSq);
            soundPos = glm::vec3(static_cast<f32>(static_cast<f64>(playerData->x) + dx / distance * 2.0),
                static_cast<f32>(static_cast<f64>(playerData->y) + dy / distance * 2.0),
                static_cast<f32>(static_cast<f64>(playerData->z) + dz / distance * 2.0));
            playedVolume = minVolume;
        }

        sendPlaySoundPacket(connMgr, playerId, soundId, category, soundPos, playedVolume, pitch);
        heardPlayers.push_back(playerId);
    }

    if (heardPlayers.empty()) {
        source.sendFailure(::mc::text::translatable("commands.playsound.failed"));
        return 0;
    }

    if (heardPlayers.size() == 1) {
        std::vector<std::unique_ptr<::mc::text::ITextComponent>> params;
        params.push_back(::mc::text::literal(soundId.toString()));
        params.push_back(::mc::text::literal(support::resolvePlayerName(source, heardPlayers.front())));
        source.sendSuccess(::mc::text::translatable("commands.playsound.success.single", std::move(params)), true);
    } else {
        std::vector<std::unique_ptr<::mc::text::ITextComponent>> params;
        params.push_back(::mc::text::literal(soundId.toString()));
        params.push_back(::mc::text::literal(std::to_string(heardPlayers.size())));
        source.sendSuccess(::mc::text::translatable("commands.playsound.success.multiple", std::move(params)), true);
    }

    return static_cast<i32>(heardPlayers.size());
}

} // namespace command
} // namespace mc
