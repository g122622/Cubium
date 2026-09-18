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
#include <utility>
#include <variant>
#include <spdlog/spdlog.h>

namespace mc::server::net {

ServerPlayHandler::ServerPlayHandler(MinecraftServer& server)
    : PlayHandlerBase(server)
    , m_movement(server)
    , m_blockAction(server)
    , m_entityAction(server)
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

    if (std::holds_alternative<irplay::MovePlayerPos>(play) || std::holds_alternative<irplay::MovePlayerPosRot>(play) ||
        std::holds_alternative<irplay::MovePlayerRot>(play) ||
        std::holds_alternative<irplay::MovePlayerStatusOnly>(play)) {
        m_movement.handlePlayerMovePacket(playerId, packet);
    } else if (std::holds_alternative<irplay::AcceptTeleportation>(play)) {
        m_movement.handleTeleportConfirmPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::KeepAlive>(play)) {
        m_sessionSignal.handleKeepAlivePacket(playerId, packet);
    } else if (std::holds_alternative<irplay::Chat>(play)) {
        m_chat.handleChatMessagePacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ChatCommand>(play)) {
        m_chat.handleChatCommandPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::PlayerAction>(play)) {
        m_blockAction.handleBlockInteractionPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::UseItemOn>(play)) {
        m_blockAction.handleBlockPlacementPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::SetCarriedItem>(play)) {
        m_server.handleHotbarSelectPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ContainerClick>(play)) {
        m_server.handleContainerClickPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ContainerClose>(play)) {
        m_server.handleCloseContainerPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::PlayerCommand>(play)) {
        // PlayerCommand 全 action 分发（疾跑/潜行/起床/骑乘跳跃/开背包/滑翔）
        m_playerState.handlePlayerCommandPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::SignUpdate>(play)) {
        m_blockAction.handleUpdateSignPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::PlayerInput>(play)) {
        m_movement.handlePlayerInputPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ServerboundMoveVehicle>(play)) {
        m_movement.handleMoveVehiclePacket(playerId, packet);
    } else if (std::holds_alternative<irplay::PaddleBoat>(play)) {
        m_movement.handlePaddleBoatPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::Interact>(play)) {
        m_entityAction.handleInteractPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::UseItem>(play)) {
        m_blockAction.handleUseItemPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ServerboundPingRequest>(play)) {
        m_sessionSignal.handlePingRequestPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ServerboundPong>(play)) {
        m_sessionSignal.handlePongPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ServerboundChangeDifficulty>(play)) {
        m_playerState.handleChangeDifficultyPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::LockDifficulty>(play)) {
        m_playerState.handleLockDifficultyPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ConfigurationAcknowledged>(play)) {
        m_sessionSignal.handleConfigurationAcknowledgedPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::SeenAdvancements>(play)) {
        m_playerState.handleSeenAdvancementsPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::PlaceRecipe>(play)) {
        m_playerState.handlePlaceRecipePacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ChunkBatchReceived>(play)) {
        m_sessionSignal.handleChunkBatchReceivedPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::SetCreativeModeSlot>(play)) {
        m_server.handleSetCreativeModeSlotPacket(playerId, packet);
    } else {
        // 未覆盖的 C→S 变体（创造模式/命令相关包）
        spdlog::info("route: unhandled C->S play variant");
    }
}

void ServerPlayHandler::updateEntityTrackingForPlayer(PlayerId playerId, f64 x, f64 y, f64 z)
{
    m_movement.updateEntityTrackingForPlayer(playerId, x, y, z);
}

} // namespace mc::server::net
