/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "server/network/play/ServerPlayHandler.hpp"

#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "server/application/MinecraftServer.hpp"
#include <type_traits>
#include <utility>
#include <variant>
#include <spdlog/spdlog.h>

namespace mc::server::net {

namespace {

// 分支标签从包类型推导，新增或重排变体后仍指向同一类型。
// 类型缺失或重复时编译失败，避免默认为下标 0 后静默错派发。
template <typename Packet>
consteval std::size_t _playPacketIndex()
{
    using PlayPacket = mc::network::ir::PlayPacket;
    return []<std::size_t... Indices>(std::index_sequence<Indices...>) {
        constexpr auto matches =
            (std::size_t{std::is_same_v<Packet, std::variant_alternative_t<Indices, PlayPacket>>} + ...);
        static_assert(matches == 1, "Play packet type must appear exactly once");
        return ((std::is_same_v<Packet, std::variant_alternative_t<Indices, PlayPacket>> ? Indices : 0) + ...);
    }(std::make_index_sequence<std::variant_size_v<PlayPacket>>{});
}

/// 该 C→S 包是否属于「玩家主动动作」——对齐 vanilla ServerGamePacketListenerImpl 中调用
/// `player.resetLastActionTime()` 的处理器集合（player_input / player_action / use_item_on /
/// use_item / set_carried_item / chat / chat_command / player_command / interact /
/// container_click / place_recipe / sign_update）。自动回包（keep_alive / pong / ping_request）
/// 不在此列——否则挂机客户端会被 KeepAlive 续命、空闲踢出永不触发。
/// 移动包（MovePlayer*）亦不在此列：vanilla 经 handlePlayerKnownMovement 仅在位移显著时推进，
/// 由 MovementHandler 单独处理。
bool _isPlayerActionPacket(const mc::network::ir::PlayPacket& play)
{
    namespace irplay = mc::network::ir::play;
    return std::holds_alternative<irplay::PlayerInput>(play) || std::holds_alternative<irplay::PlayerAction>(play) ||
        std::holds_alternative<irplay::UseItemOn>(play) || std::holds_alternative<irplay::UseItem>(play) ||
        std::holds_alternative<irplay::SetCarriedItem>(play) || std::holds_alternative<irplay::Chat>(play) ||
        std::holds_alternative<irplay::ChatCommand>(play) || std::holds_alternative<irplay::PlayerCommand>(play) ||
        std::holds_alternative<irplay::Interact>(play) || std::holds_alternative<irplay::ContainerClick>(play) ||
        std::holds_alternative<irplay::PlaceRecipe>(play) || std::holds_alternative<irplay::SignUpdate>(play) ||
        std::holds_alternative<irplay::Swing>(play) || std::holds_alternative<irplay::ContainerButtonClick>(play);
}

} // namespace

ServerPlayHandler::ServerPlayHandler(MinecraftServer& server)
    : PlayHandlerBase(server)
    , m_movement(server)
    , m_blockAction(server)
    , m_entityAction(server)
    , m_containerAction(server)
    , m_chat(server)
    , m_playerState(server)
    , m_sessionSignal(server)
{}

void ServerPlayHandler::route(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 新网络层：按 ir::PlayPacket 变体分发到 handle*Packet。
    // 3 个纯虚（handleHotbarSelect/handleContainerClick/handleCloseContainer）经
    // m_server 虚分发到子类 override；其余为本门面方法直调。
    MC_ASSERT_RELEASE(packet.phase == mc::network::protocol::ConnectionProtocol::Play);
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    namespace irplay = mc::network::ir::play;

    // 空闲踢出的「最后动作时间」推进：对齐 vanilla 各 C→S 处理器里的 player.resetLastActionTime()。
    // 集中在此判定，避免散落到 6 个处理器；集合定义见 isPlayerActionPacket。
    if (_isPlayerActionPacket(play)) {
        m_server.recordPlayerAction(playerId);
    }

    switch (play.index()) {
        case _playPacketIndex<irplay::MovePlayerPos>():
        case _playPacketIndex<irplay::MovePlayerPosRot>():
        case _playPacketIndex<irplay::MovePlayerRot>():
        case _playPacketIndex<irplay::MovePlayerStatusOnly>():
            m_movement.handlePlayerMovePacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::AcceptTeleportation>():
            m_movement.handleTeleportConfirmPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::KeepAlive>():
            m_sessionSignal.handleKeepAlivePacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::Chat>():
            m_chat.handleChatMessagePacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ChatCommand>():
            m_chat.handleChatCommandPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::PlayerAction>():
            m_blockAction.handleBlockInteractionPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::UseItemOn>():
            m_blockAction.handleBlockPlacementPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::SetCarriedItem>():
            m_server.handleHotbarSelectPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ContainerClick>():
            m_server.handleContainerClickPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ContainerClose>():
            m_server.handleCloseContainerPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::PlayerCommand>():
            // PlayerCommand 全 action 分发（疾跑/潜行/起床/骑乘跳跃/开背包/滑翔）
            m_playerState.handlePlayerCommandPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::SignUpdate>():
            m_blockAction.handleUpdateSignPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::PlayerInput>():
            m_movement.handlePlayerInputPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ServerboundMoveVehicle>():
            m_movement.handleMoveVehiclePacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::PaddleBoat>():
            m_movement.handlePaddleBoatPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::Interact>():
            m_entityAction.handleInteractPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::Swing>():
            m_entityAction.handleSwingPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::UseItem>():
            m_blockAction.handleUseItemPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ServerboundPingRequest>():
            m_sessionSignal.handlePingRequestPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ServerboundPong>():
            m_sessionSignal.handlePongPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ServerboundChangeDifficulty>():
            m_playerState.handleChangeDifficultyPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::LockDifficulty>():
            m_playerState.handleLockDifficultyPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::TeleportToEntity>():
            m_playerState.handleTeleportToEntityPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ClientCommand>():
            m_playerState.handleClientCommandPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::SelectTrade>():
            m_containerAction.handleSelectTradePacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ContainerButtonClick>():
            m_containerAction.handleContainerButtonClickPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::RenameItem>():
            m_containerAction.handleRenameItemPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ClientInformation>():
            m_playerState.handleClientInformationPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ChangeGameMode>():
            m_playerState.handleChangeGameModePacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ServerboundPlayerAbilities>():
            m_playerState.handlePlayerAbilitiesPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ConfigurationAcknowledged>():
            m_sessionSignal.handleConfigurationAcknowledgedPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::SeenAdvancements>():
            m_playerState.handleSeenAdvancementsPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::PlaceRecipe>():
            m_playerState.handlePlaceRecipePacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ChunkBatchReceived>():
            m_sessionSignal.handleChunkBatchReceivedPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::ClientTickEnd>():
            m_movement.handleClientTickEndPacket(playerId, packet);
            break;
        case _playPacketIndex<irplay::PlayerLoaded>():
            m_sessionSignal.handlePlayerLoadedPacket(playerId);
            break;
        case _playPacketIndex<irplay::SetCreativeModeSlot>():
            m_server.handleSetCreativeModeSlotPacket(playerId, packet);
            break;
        default:

            // 未覆盖的 C→S 变体（创造模式/命令相关包）
            spdlog::warn("route: unhandled C->S play variant {}", play.index());
            break;
    }
}

void ServerPlayHandler::updateEntityTrackingForPlayer(PlayerId playerId, f64 x, f64 y, f64 z)
{
    m_movement.updateEntityTrackingForPlayer(playerId, x, y, z);
}

} // namespace mc::server::net
