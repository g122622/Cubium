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

#include "NetworkClient.hpp"
#include "common/core/Constants.hpp"
#include "common/network/packet/BlockBreakAnimPacket.hpp"
#include "common/network/packet/BlockEventPacket.hpp"
#include "common/network/packet/CommandTreePacket.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/GameStateChangePacket.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/packet/PlayerAbilitiesPacket.hpp"
#include "common/network/packet/ServerDifficultyPacket.hpp"
#include "common/network/packet/SetCameraPacket.hpp"
#include "common/network/packet/SetPassengersPacket.hpp"
#include "common/network/packet/SleepPacket.hpp"
#include "common/network/packet/SpawnPositionPacket.hpp"
#include "common/network/packet/TitlePacket.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/sound/network/SoundPackets.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include <chrono>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::client {

// ============================================================================
// 常量
// ============================================================================

namespace {
constexpr size_t RECEIVE_BUFFER_SIZE = 64 * 1024; // 64KB 初始缓冲区
// 使用 Constants.hpp 中定义的 mc::network::MAX_PACKET_SIZE (2MB)
} // namespace

// ============================================================================
// NetworkClient 实现
// ============================================================================

NetworkClient::NetworkClient()
    : m_receiveBuffer(RECEIVE_BUFFER_SIZE)
    , m_packetBuffer()
{}

NetworkClient::~NetworkClient()
{
    disconnect("Destructor");
}

Result<void> NetworkClient::connect(const NetworkClientConfig& config)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network,
        "NetworkClient::connect",
        "address",
        config.serverAddress,
        "port",
        config.serverPort);

    if (m_state != ClientState::Disconnected) {
        return Error(ErrorCode::InvalidState, "Already connected or connecting");
    }

    if (m_localEndpoint) {
        return Error(ErrorCode::InvalidState, "Cannot use TCP connect with local connection");
    }

    m_config = config;
    m_username = config.username;
    m_socket = std::make_unique<asio::ip::tcp::socket>(m_ioContext);

    _setState(ClientState::Connecting);

    try {
        // 解析服务器地址
        asio::ip::tcp::resolver resolver(m_ioContext);
        auto endpoints = resolver.resolve(config.serverAddress, std::to_string(config.serverPort));

        // 同步连接 (在主线程)
        asio::connect(*m_socket, endpoints);

        // 设置 TCP 选项
        m_socket->set_option(asio::ip::tcp::no_delay(true));
        m_socket->set_option(asio::socket_base::keep_alive(true));

        _setState(ClientState::LoggingIn);

        // 启动接收线程
        m_running = true;
        m_ioThread = std::make_unique<std::thread>([this]() { _receiveLoop(); });

        // 发送登录请求
        sendLoginRequest();

        spdlog::info("Connected to {}:{}", config.serverAddress, config.serverPort);
        return Result<void>::ok();
    }
    catch (const std::exception& e) {
        _setState(ClientState::Disconnected);
        spdlog::error("Failed to connect: {}", e.what());
        return Error(ErrorCode::ConnectionFailed, e.what());
    }
}

Result<void> NetworkClient::connectLocal(network::LocalEndpoint* endpoint, const NetworkClientConfig& config)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::connectLocal");

    if (m_state != ClientState::Disconnected) {
        return Error(ErrorCode::InvalidState, "Already connected or connecting");
    }

    if (!endpoint) {
        return Error(ErrorCode::InvalidArgument, "Local endpoint is null");
    }

    m_config = config;
    m_localEndpoint = endpoint;
    m_username = m_config.username;
    m_running = true;

    _setState(ClientState::LoggingIn);

    // 本地连接模式：无需 IO 线程，直接发送登录请求
    sendLoginRequest();

    spdlog::info("Connected to integrated server (local)");
    return Result<void>::ok();
}

void NetworkClient::disconnect(const std::string& reason)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::disconnect", "reason", reason);

    if (m_state == ClientState::Disconnected) {
        return;
    }

    _setState(ClientState::Disconnecting);
    m_running = false;

    if (m_localEndpoint) {
        // 本地连接模式：无需关闭 socket
        m_localEndpoint = nullptr;
    } else if (m_socket && m_socket->is_open()) {
        // TCP 模式
        asio::error_code ec;
        m_socket->shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        m_socket->close(ec);
    }

    // 等待 IO 线程（仅 TCP 模式）
    if (m_ioThread && m_ioThread->joinable()) {
        m_ioThread->join();
    }
    m_ioThread.reset();
    m_socket.reset();

    _setState(ClientState::Disconnected);

    if (m_callbacks.onDisconnected) {
        m_callbacks.onDisconnected(reason);
    }

    spdlog::info("Disconnected: {}", reason);
}

bool NetworkClient::isConnected() const
{
    if (m_localEndpoint) {
        return m_localEndpoint->isConnected() && m_state != ClientState::Disconnected;
    }
    return m_socket && m_socket->is_open() && m_state != ClientState::Disconnected;
}

ClientState NetworkClient::state() const
{
    return m_state;
}

void NetworkClient::setCallbacks(const NetworkClientCallbacks& callbacks)
{
    m_callbacks = callbacks;
}

void NetworkClient::sendLoginRequest()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Network.Packet, "NetworkClient::sendLoginRequest", "username", m_username);

    network::LoginRequestPacket packet(m_username, network::protocol::VERSION);

    network::PacketSerializer ser;
    packet.serialize(ser);

    // 封装数据包
    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::LoginRequest));
    fullPacket.writeU16(0); // flags
    fullPacket.writeU16(0); // reserved
    fullPacket.writeU16(0); // padding
    fullPacket.writeBytes(ser.buffer());

    _sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendPlayerMove(const network::PlayerPosition& pos, network::PlayerMovePacket::MoveType type)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Network.Packet, "NetworkClient::sendPlayerMove", "type", static_cast<i32>(type));

    network::PlayerMovePacket packet(pos, type);

    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::PlayerMove));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    _sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendBlockInteraction(network::BlockInteractionAction action, i32 x, i32 y, i32 z, Direction face)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Network.Packet,
        "NetworkClient::sendBlockInteraction",
        "action",
        static_cast<i32>(action),
        "pos",
        fmt::format("({}, {}, {})", x, y, z));

    network::BlockInteractionPacket packet(action, x, y, z, face);

    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::BlockInteraction));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    _sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendBlockPlacement(i32 x, i32 y, i32 z, Direction face, f32 hitX, f32 hitY, f32 hitZ, u8 hand)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Network.Packet,
        "NetworkClient::sendBlockPlacement",
        "pos",
        fmt::format("({}, {}, {})", x, y, z),
        "face",
        static_cast<i32>(face));

    network::PlayerTryUseItemOnBlockPacket packet(x, y, z, face, hitX, hitY, hitZ, hand);

    spdlog::info("[Place] Send block placement pos=({}, {}, {}) face={} hit=({:.2f}, {:.2f}, {:.2f})",
        x,
        y,
        z,
        static_cast<i32>(face),
        hitX,
        hitY,
        hitZ);

    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::PlayerTryUseItemOnBlock));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    _sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendHotbarSelect(i32 slot)
{
    HotbarSelectPacket packet(slot);

    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::HotbarSelect));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    _sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendTeleportConfirm(u32 teleportId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Network.Packet, "NetworkClient::sendTeleportConfirm", "teleportId", teleportId);

    network::TeleportConfirmPacket packet(teleportId);

    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::TeleportConfirm));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    _sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendConfirmDimensionChange(DimensionId dimension)
{
    network::ConfirmDimensionChangePacket packet;
    packet.setDimension(dimension);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize ConfirmDimensionChange packet: {}", result.error().message());
        return;
    }

    _sendRawData(result.value().data(), result.value().size());
}

void NetworkClient::sendKeepAlive(u64 id)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Network.Packet, "NetworkClient::sendKeepAlive", "id", id);

    network::KeepAlivePacket packet;
    packet.setTimestamp(id);

    auto result = packet.serialize();
    if (result.success()) {
        _sendRawData(result.value().data(), result.value().size());
    }

    m_lastKeepAliveSent =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();
}

void NetworkClient::sendChatMessage(const std::string& message)
{
    network::ChatMessagePacket packet(message, m_playerId);

    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::ChatMessage));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    _sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendCreativeInventoryAction(const CreativeInventoryActionPacket& packet)
{
    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::CreativeInventoryAction));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    _sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendContainerClick(const ContainerClickPacket& packet)
{
    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::ContainerClick));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    _sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendCloseContainer(ContainerId containerId)
{
    CloseContainerPacket packet(containerId);

    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::CloseContainer));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    _sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendUpdateSign(
    const BlockPos& pos, const std::array<std::string, network::UpdateSignPacket::LINE_COUNT>& lines, bool isFrontSide)
{
    network::UpdateSignPacket packet(pos, lines, isFrontSide);

    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::UpdateSign));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    _sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendPlayerInput(f32 strafeSpeed, f32 forwardSpeed, bool jumping, bool sneaking)
{
    network::PlayerInputPacket packet;
    packet.setStrafeSpeed(strafeSpeed);
    packet.setForwardSpeed(forwardSpeed);
    packet.setJumping(jumping);
    packet.setSneaking(sneaking);

    auto result = packet.serialize();
    if (result.success()) {
        _sendRawData(result.value().data(), result.value().size());
    }
}

void NetworkClient::sendMoveVehicle(f64 x, f64 y, f64 z, f32 yaw, f32 pitch)
{
    network::MoveVehiclePacket packet;
    packet.setPosition(x, y, z);
    packet.setRotation(yaw, pitch);

    auto result = packet.serialize();
    if (result.success()) {
        _sendRawData(result.value().data(), result.value().size());
    }
}

void NetworkClient::sendEntityAction(network::EntityActionType action, i32 auxData)
{
    network::EntityActionPacket packet;
    packet.setEntityId(static_cast<u32>(m_playerId));
    packet.setAction(action);
    packet.setAuxData(auxData);

    auto result = packet.serialize();
    if (result.success()) {
        _sendRawData(result.value().data(), result.value().size());
    }
}

void NetworkClient::sendSteerBoat(bool leftPaddle, bool rightPaddle)
{
    network::SteerBoatPacket packet;
    packet.setPaddleState(leftPaddle, rightPaddle);

    auto result = packet.serialize();
    if (result.success()) {
        _sendRawData(result.value().data(), result.value().size());
    }
}

void NetworkClient::poll()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::poll");

    if (!m_running) return;

    if (m_localEndpoint) {
        // 本地连接模式：直接从队列读取
        std::vector<u8> data;
        while (m_localEndpoint->receive(data)) {
            _processPacket(data.data(), data.size());
            m_packetsReceived++;
        }

        if (!m_localEndpoint->isConnected()) {
            disconnect("Server disconnected");
            return;
        }

        return;
    }

    // TCP 模式原有逻辑
    // 处理接收到的数据包
    _processIncomingData();
}

void NetworkClient::_receiveLoop()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::_receiveLoop");

    while (m_running) {
        try {
            size_t bytesRead = m_socket->read_some(asio::buffer(m_receiveBuffer));

            if (bytesRead > 0) {
                m_bytesReceived += bytesRead;

                // 将数据添加到处理缓冲区
                std::lock_guard<std::mutex> lock(m_receiveMutex);
                m_packetBuffer.insert(
                    m_packetBuffer.end(), m_receiveBuffer.begin(), m_receiveBuffer.begin() + bytesRead);
            }
        }
        catch (const asio::system_error& e) {
            if (m_running) {
                spdlog::error("Receive error: {}", e.what());
                disconnect("Connection error: " + std::string(e.what()));
            }
            break;
        }
    }
}

void NetworkClient::_processIncomingData()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::_processIncomingData");

    std::vector<u8> dataToProcess;
    {
        std::lock_guard<std::mutex> lock(m_receiveMutex);
        dataToProcess = std::move(m_packetBuffer);
        m_packetBuffer.clear();
    }

    size_t offset = 0;
    while (offset + network::PACKET_HEADER_SIZE <= dataToProcess.size()) {
        // 读取包大小
        u32 packetSize = (static_cast<u32>(dataToProcess[offset]) << 24) |
            (static_cast<u32>(dataToProcess[offset + 1]) << 16) | (static_cast<u32>(dataToProcess[offset + 2]) << 8) |
            static_cast<u32>(dataToProcess[offset + 3]);

        if (packetSize > mc::network::MAX_PACKET_SIZE) {
            spdlog::error("Packet too large: {} bytes (max: {})", packetSize, mc::network::MAX_PACKET_SIZE);
            disconnect("Invalid packet size");
            return;
        }

        if (offset + packetSize > dataToProcess.size()) {
            // 数据不完整，等待更多数据
            break;
        }

        // 处理完整数据包
        _processPacket(dataToProcess.data() + offset, packetSize);
        m_packetsReceived++;

        offset += packetSize;
    }

    // 保留未处理的数据
    if (offset < dataToProcess.size()) {
        std::lock_guard<std::mutex> lock(m_receiveMutex);
        m_packetBuffer.insert(m_packetBuffer.begin(), dataToProcess.begin() + offset, dataToProcess.end());
    }
}

void NetworkClient::_processPacket(const u8* data, size_t size)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::_processPacket", "size", size);

    MC_ASSERT_RELEASE(size >= network::PACKET_HEADER_SIZE);

    network::PacketDeserializer headerDeser(data, size);

    auto sizeResult = headerDeser.readU32();
    if (sizeResult.failed()) return;

    auto typeResult = headerDeser.readU16();
    if (typeResult.failed()) return;

    // auto flagsResult = headerDeser.readU16();
    // auto reservedResult = headerDeser.readU16();
    // auto paddingResult = headerDeser.readU16();

    network::PacketType packetType = static_cast<network::PacketType>(typeResult.value());

    // 创建包体反序列化器（跳过头部）
    network::PacketDeserializer bodyDeser(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);

    switch (packetType) {
        case network::PacketType::KeepAlive: {
            network::KeepAlivePacket packet;
            auto result = packet.deserialize(data, size);
            if (result.success()) {
                _handleKeepAlive(packet.timestamp());
            }
            break;
        }

        case network::PacketType::Disconnect: {
            network::DisconnectPacket packet;
            auto result = packet.deserialize(data, size);
            if (result.success()) {
                disconnect(packet.reason());
            }
            break;
        }

        case network::PacketType::LoginResponse: {
            _handleLoginResponse(bodyDeser);
            break;
        }

        case network::PacketType::CommandTree: {
            _handleCommandTree(bodyDeser.data(), bodyDeser.size());
            break;
        }

        case network::PacketType::Teleport: {
            _handleTeleport(bodyDeser);
            break;
        }

        case network::PacketType::ChunkData: {
            _handleChunkData(bodyDeser);
            break;
        }

        case network::PacketType::UnloadChunk: {
            _handleUnloadChunk(bodyDeser);
            break;
        }

        case network::PacketType::PlayerSpawn: {
            _handlePlayerSpawn(bodyDeser);
            break;
        }

        case network::PacketType::PlayerDespawn: {
            _handlePlayerDespawn(bodyDeser);
            break;
        }

        case network::PacketType::BlockUpdate: {
            _handleBlockUpdate(bodyDeser);
            break;
        }

        case network::PacketType::ChatBroadcast: {
            _handleChatMessage(bodyDeser);
            break;
        }

        case network::PacketType::TimeUpdate: {
            _handleTimeUpdate(bodyDeser);
            break;
        }

        case network::PacketType::PlayerInventory: {
            _handlePlayerInventory(bodyDeser);
            break;
        }

        case network::PacketType::OpenContainer: {
            _handleOpenContainer(bodyDeser);
            break;
        }

        case network::PacketType::ContainerContent: {
            _handleContainerContent(bodyDeser);
            break;
        }

        case network::PacketType::ContainerSlot: {
            _handleContainerSlot(bodyDeser);
            break;
        }

        case network::PacketType::CloseContainer: {
            _handleCloseContainer(bodyDeser);
            break;
        }

        case network::PacketType::OpenSignEditor: {
            _handleSignEditorOpen(bodyDeser);
            break;
        }

        case network::PacketType::BlockEntityData: {
            _handleBlockEntityData(bodyDeser);
            break;
        }

            // ========== 实体包 ==========

        case network::PacketType::SpawnEntity: {
            _handleSpawnEntity(bodyDeser);
            break;
        }

        case network::PacketType::SpawnMob: {
            _handleSpawnMob(bodyDeser);
            break;
        }

        case network::PacketType::EntityDestroy: {
            _handleEntityDestroy(bodyDeser);
            break;
        }

        case network::PacketType::EntityMove: {
            _handleEntityMove(bodyDeser);
            break;
        }

        case network::PacketType::EntityTeleport: {
            _handleEntityTeleport(bodyDeser);
            break;
        }

        case network::PacketType::EntityVelocity: {
            _handleEntityVelocity(bodyDeser);
            break;
        }

        case network::PacketType::EntityMetadata: {
            _handleEntityMetadata(bodyDeser);
            break;
        }

        case network::PacketType::EntityAnimation: {
            _handleEntityAnimation(bodyDeser);
            break;
        }

        case network::PacketType::EntityHeadLook: {
            _handleEntityHeadLook(bodyDeser);
            break;
        }

        case network::PacketType::EntityStatus: {
            _handleEntityStatus(bodyDeser);
            break;
        }

        case network::PacketType::CollectItem: {
            _handleCollectItem(bodyDeser);
            break;
        }

        case network::PacketType::GameStateChange: {
            _handleGameStateChange(bodyDeser);
            break;
        }

        case network::PacketType::PlayerAbilities: {
            _handlePlayerAbilities(bodyDeser);
            break;
        }

        case network::PacketType::ServerDifficulty: {
            _handleServerDifficulty(bodyDeser);
            break;
        }

        case network::PacketType::LightUpdate: {
            _handleLightUpdate(bodyDeser);
            break;
        }

        case network::PacketType::BlockBreakAnim: {
            _handleBlockBreakAnim(bodyDeser);
            break;
        }

        case network::PacketType::BlockEvent: {
            _handleBlockEvent(bodyDeser);
            break;
        }

        case network::PacketType::PlaySound: {
            _handlePlaySound(bodyDeser);
            break;
        }

        case network::PacketType::StopSound: {
            _handleStopSound(bodyDeser);
            break;
        }

        case network::PacketType::PlaySoundEffect: {
            _handlePlaySoundEffect(bodyDeser);
            break;
        }

        case network::PacketType::SetExperience: {
            _handleSetExperience(bodyDeser);
            break;
        }

        case network::PacketType::SpawnExperienceOrb: {
            _handleSpawnExperienceOrb(bodyDeser);
            break;
        }

        case network::PacketType::PlayerListItem: {
            _handlePlayerListItem(bodyDeser);
            break;
        }

        case network::PacketType::Particle: {
            _handleParticle(bodyDeser);
            break;
        }

        case network::PacketType::MovingSound: {
            _handleMovingSound(bodyDeser);
            break;
        }

        case network::PacketType::WorldEvent: {
            _handleWorldEvent(bodyDeser);
            break;
        }

        case network::PacketType::SetPassengers: {
            _handleSetPassengers(bodyDeser);
            break;
        }

        case network::PacketType::SetCamera: {
            _handleSetCamera(bodyDeser);
            break;
        }

        case network::PacketType::Respawn: {
            _handleRespawn(bodyDeser);
            break;
        }

        case network::PacketType::DimensionInfo: {
            _handleDimensionInfo(bodyDeser);
            break;
        }

        case network::PacketType::SpawnPosition: {
            _handleSpawnPosition(bodyDeser);
            break;
        }

        case network::PacketType::VehicleMove: {
            _handleVehicleMove(bodyDeser);
            break;
        }

        case network::PacketType::Sleep: {
            _handleSleep(bodyDeser);
            break;
        }

        case network::PacketType::HotbarSet: {
            _handleHotbarSet(bodyDeser);
            break;
        }

        case network::PacketType::Title: {
            _handleTitle(bodyDeser);
            break;
        }

        case network::PacketType::MapData: {
            _handleMapData(bodyDeser);
            break;
        }

        case network::PacketType::Explosion: {
            _handleExplosion(bodyDeser);
            break;
        }

        default:
            spdlog::error("Unhandled packet type: {}", static_cast<i32>(packetType));
            break;
    }
}

void NetworkClient::_sendRawData(const u8* data, size_t size)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::_sendRawData", "size", size);

    if (m_localEndpoint) {
        // 本地连接模式
        if (m_localEndpoint->isConnected()) {
            m_localEndpoint->send(data, size);
            m_bytesSent += size;
            m_packetsSent++;
        }
        return;
    }

    // TCP 模式
    if (!m_socket || !m_socket->is_open()) {
        return;
    }

    try {
        asio::write(*m_socket, asio::buffer(data, size));
        m_bytesSent += size;
        m_packetsSent++;
    }
    catch (const std::exception& e) {
        spdlog::error("Send error: {}", e.what());
    }
}

void NetworkClient::_setState(ClientState state)
{
    m_state = state;
}

void NetworkClient::_handleKeepAlive(u64 id)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::_handleKeepAlive", "id", id);

    m_lastKeepAliveReceived =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();

    if (m_lastKeepAliveSent > 0) {
        m_ping = static_cast<u32>(m_lastKeepAliveReceived - m_lastKeepAliveSent);
    }

    sendKeepAlive(id);
}

void NetworkClient::_handleLoginResponse(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::_handleLoginResponse");

    auto result = network::LoginResponsePacket::deserialize(deser);
    if (result.failed()) {
        if (m_callbacks.onLoginFailed) {
            m_callbacks.onLoginFailed("Invalid login response");
        }
        disconnect("Login failed");
        return;
    }

    auto& response = result.value();
    if (response.success()) {
        m_playerId = response.playerId();
        EntityId entityId = response.entityId();
        _setState(ClientState::Playing);

        spdlog::info(
            "[NetworkClient::_handleLoginResponse] Login successful: playerId={}, entityId={}", m_playerId, entityId);

        if (m_callbacks.onLoginSuccess) {
            m_callbacks.onLoginSuccess(m_playerId, entityId, response.username());
        }
        if (m_callbacks.onConnected) {
            m_callbacks.onConnected();
        }
    } else {
        if (m_callbacks.onLoginFailed) {
            m_callbacks.onLoginFailed(response.message());
        }
        disconnect(response.message());
    }
}

void NetworkClient::_handleCommandTree(const u8* data, size_t size)
{
    network::CommandTreePacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("[NetworkClient::_handleCommandTree] Failed to deserialize command tree packet: {}",
            result.error().message());
        return;
    }

    if (m_callbacks.onCommandTree) {
        m_callbacks.onCommandTree(packet.treeJson());
    }
}

void NetworkClient::_handleTeleport(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::_handleTeleport");

    auto result = network::TeleportPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("[NetworkClient::_handleTeleport] Failed to deserialize teleport packet");
        return;
    }

    auto& packet = result.value();

    // 发送确认
    sendTeleportConfirm(packet.teleportId());

    // 回调通知
    if (m_callbacks.onTeleport) {
        m_callbacks.onTeleport(packet.x(), packet.y(), packet.z(), packet.yaw(), packet.pitch(), packet.teleportId());
    }
}

void NetworkClient::_handleChunkData(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::_handleChunkData");

    auto result = network::ChunkDataPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error(
            "[NetworkClient::_handleChunkData] Failed to deserialize chunk data packet: {}", result.error().message());
        return;
    }

    auto& packet = result.value();

    if (m_callbacks.onChunkData) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network,
            "NetworkClient::_handleChunkDataCallback",
            "pos",
            fmt::format("({}, {})", packet.x(), packet.z()));
        m_callbacks.onChunkData(packet.x(), packet.z(), packet.dimension(), packet.data());
    }
}

void NetworkClient::_handleTimeUpdate(network::PacketDeserializer& deser)
{
    auto result = network::TimeUpdatePacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("[NetworkClient::_handleTimeUpdate] Failed to deserialize time update packet: {}",
            result.error().message());
        return;
    }

    const auto& packet = result.value();

    if (m_callbacks.onTimeUpdate) {
        m_callbacks.onTimeUpdate(packet.gameTime(), packet.dayTime(), packet.daylightCycleEnabled());
    }
}

void NetworkClient::_handleUnloadChunk(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::_handleUnloadChunk");

    auto result = network::UnloadChunkPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("[NetworkClient::_handleUnloadChunk] Failed to deserialize unload chunk packet");
        return;
    }

    auto& packet = result.value();

    if (m_callbacks.onChunkUnload) {
        m_callbacks.onChunkUnload(packet.x(), packet.z(), packet.dimension());
    }
}

void NetworkClient::_handlePlayerSpawn(network::PacketDeserializer& deser)
{
    auto result = network::PlayerSpawnPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("[NetworkClient::_handlePlayerSpawn] Failed to deserialize player spawn packet");
        return;
    }

    auto& packet = result.value();

    if (m_callbacks.onPlayerSpawn) {
        m_callbacks.onPlayerSpawn(
            packet.playerId(), packet.username(), packet.position().x, packet.position().y, packet.position().z);
    }
}

void NetworkClient::_handlePlayerDespawn(network::PacketDeserializer& deser)
{
    auto result = network::PlayerDespawnPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("[NetworkClient::_handlePlayerDespawn] Failed to deserialize player despawn packet");
        return;
    }

    auto& packet = result.value();

    if (m_callbacks.onPlayerDespawn) {
        m_callbacks.onPlayerDespawn(packet.playerId());
    }
}

void NetworkClient::_handleBlockUpdate(network::PacketDeserializer& deser)
{
    auto result = network::BlockUpdatePacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("[NetworkClient::_handleBlockUpdate] Failed to deserialize block update packet");
        return;
    }

    auto& packet = result.value();

    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Lighting,
        "ReceiveBlockUpdate",
        "pos",
        fmt::format("({}, {}, {})", packet.x(), packet.y(), packet.z()),
        "stateId",
        packet.blockStateId(),
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(packet.x(), packet.y(), packet.z()).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    if (m_callbacks.onBlockUpdate) {
        m_callbacks.onBlockUpdate(packet.x(), packet.y(), packet.z(), packet.blockStateId());
    }
}

void NetworkClient::_handleChatMessage(network::PacketDeserializer& deser)
{
    auto result = network::ChatMessagePacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("[NetworkClient::_handleChatMessage] Failed to deserialize chat message packet");
        return;
    }

    auto& packet = result.value();

    if (m_callbacks.onChatMessage) {
        m_callbacks.onChatMessage(packet.message(), packet.senderId());
    }
}

void NetworkClient::_handlePlayerInventory(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::_handlePlayerInventory");

    auto result = PlayerInventoryPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize player inventory packet");
        return;
    }

    const auto& packet = result.value();
    if (m_callbacks.onPlayerInventory) {
        m_callbacks.onPlayerInventory(packet.selectedSlot(), packet.items());
    }
}

void NetworkClient::_handleOpenContainer(network::PacketDeserializer& deser)
{
    auto result = OpenContainerPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize open container packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onOpenContainer) {
        m_callbacks.onOpenContainer(result.value());
    }
}

void NetworkClient::_handleSignEditorOpen(network::PacketDeserializer& deser)
{
    auto result = network::OpenSignEditorPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize open sign editor packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onSignEditorOpen) {
        m_callbacks.onSignEditorOpen(result.value());
    }
}

void NetworkClient::_handleBlockEntityData(network::PacketDeserializer& deser)
{
    auto result = network::BlockEntityDataPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize block entity data packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onBlockEntityData) {
        m_callbacks.onBlockEntityData(result.value());
    }
}

void NetworkClient::_handleContainerContent(network::PacketDeserializer& deser)
{
    auto result = ContainerContentPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize container content packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onContainerContent) {
        m_callbacks.onContainerContent(result.value());
    }
}

void NetworkClient::_handleContainerSlot(network::PacketDeserializer& deser)
{
    auto result = ContainerSlotPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize container slot packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onContainerSlot) {
        m_callbacks.onContainerSlot(result.value());
    }
}

void NetworkClient::_handleCloseContainer(network::PacketDeserializer& deser)
{
    auto result = CloseContainerPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize close container packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onCloseContainer) {
        m_callbacks.onCloseContainer(result.value().containerId());
    }
}

// ============================================================================
// 实体包处理
// ============================================================================

void NetworkClient::_handleSpawnEntity(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Entity, "NetworkClient::_handleSpawnEntity");

    // 获取原始数据指针
    const u8* data = deser.data();
    size_t size = deser.size();

    network::SpawnEntityPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize SpawnEntity packet: {}", result.error().message());
        return;
    }

    spdlog::info("Received SpawnEntity: id={}, type={}, pos=({:.1f}, {:.1f}, {:.1f}){}",
        packet.entityId(),
        packet.entityTypeId().c_str(),
        packet.x(),
        packet.y(),
        packet.z(),
        packet.hasItemStack() ? " (with ItemStack)" : "");

    if (m_callbacks.onSpawnEntity) {
        m_callbacks.onSpawnEntity(packet.entityId(),
            packet.entityTypeId(),
            packet.x(),
            packet.y(),
            packet.z(),
            packet.yaw(),
            packet.pitch(),
            static_cast<f32>(packet.velocityX()) / 8000.0f,
            static_cast<f32>(packet.velocityY()) / 8000.0f,
            static_cast<f32>(packet.velocityZ()) / 8000.0f,
            packet.itemStack());
    }
}

void NetworkClient::_handleSpawnMob(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Entity, "NetworkClient::_handleSpawnMob");

    const u8* data = deser.data();
    size_t size = deser.size();

    network::SpawnMobPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize SpawnMob packet: {}", result.error().message());
        return;
    }

    // spdlog::info("Received SpawnMob: id={}, type={}, pos=({:.1f}, {:.1f}, {:.1f}), metadata size: {}",
    //             packet.entityId(), packet.entityTypeId().c_str(),
    //             packet.x(), packet.y(), packet.z(), packet.metadata().size());

    if (m_callbacks.onSpawnMob) {
        m_callbacks.onSpawnMob(packet.entityId(),
            packet.entityTypeId(),
            packet.x(),
            packet.y(),
            packet.z(),
            packet.yaw(),
            packet.pitch(),
            packet.headYaw());
    }

    if (m_callbacks.onEntityMetadata) {
        m_callbacks.onEntityMetadata(packet.entityId(), packet.metadata());
    }
}

void NetworkClient::_handleEntityDestroy(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Entity, "NetworkClient::_handleEntityDestroy");

    const u8* data = deser.data();
    size_t size = deser.size();

    network::EntityDestroyPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize EntityDestroy packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onEntityDestroy) {
        m_callbacks.onEntityDestroy(packet.entityIds());
    }
}

void NetworkClient::_handleEntityMove(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Entity, "NetworkClient::_handleEntityMove");

    const u8* data = deser.data();
    size_t size = deser.size();

    network::EntityMovePacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize EntityMove packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onEntityMove) {
        // 相对移动转换为绝对位置需要客户端缓存
        // TODO 这里暂时简化处理，发送相对移动信息
        m_callbacks.onEntityMove(packet.entityId(),
            packet.deltaX() / 32.0f, // 转换为方块单位
            packet.deltaY() / 32.0f,
            packet.deltaZ() / 32.0f);
    }
}

void NetworkClient::_handleEntityTeleport(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Entity, "NetworkClient::_handleEntityTeleport");

    const u8* data = deser.data();
    size_t size = deser.size();

    network::EntityTeleportPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize EntityTeleport packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onEntityTeleport) {
        m_callbacks.onEntityTeleport(
            packet.entityId(), packet.x(), packet.y(), packet.z(), packet.yaw(), packet.pitch());
    }
}

void NetworkClient::_handleEntityVelocity(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Entity, "NetworkClient::_handleEntityVelocity");

    const u8* data = deser.data();
    size_t size = deser.size();

    network::EntityVelocityPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize EntityVelocity packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onEntityVelocity) {
        m_callbacks.onEntityVelocity(packet.entityId(), packet.velocityX(), packet.velocityY(), packet.velocityZ());
    }
}

void NetworkClient::_handleExplosion(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Entity, "NetworkClient::_handleExplosion");

    // ExplosionPacket::deserialize 接收原始 data/size（内部自建 PacketDeserializer）
    const u8* data = deser.data();
    size_t size = deser.size();

    network::ExplosionPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize Explosion packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onExplosion) {
        m_callbacks.onExplosion(packet);
    }
}

void NetworkClient::_handleEntityMetadata(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Entity, "NetworkClient::_handleEntityMetadata");

    const u8* data = deser.data();
    size_t size = deser.size();

    network::EntityMetadataPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize EntityMetadata packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onEntityMetadata) {
        m_callbacks.onEntityMetadata(packet.entityId(), packet.metadata());
    }
}

void NetworkClient::_handleEntityAnimation(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Entity, "NetworkClient::_handleEntityAnimation");

    const u8* data = deser.data();
    size_t size = deser.size();

    network::EntityAnimationPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize EntityAnimation packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onEntityAnimation) {
        m_callbacks.onEntityAnimation(packet.entityId(), static_cast<u8>(packet.animation()));
    }
}

void NetworkClient::_handleEntityHeadLook(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    network::EntityHeadLookPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize EntityHeadLook packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onEntityHeadLook) {
        m_callbacks.onEntityHeadLook(packet.entityId(), packet.headYaw());
    }
}

void NetworkClient::_handleEntityStatus(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    network::EntityStatusPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize EntityStatus packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onEntityStatus) {
        m_callbacks.onEntityStatus(packet.entityId(), static_cast<u8>(packet.status()));
    }
}

void NetworkClient::_handleCollectItem(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    network::CollectItemPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize CollectItem packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onEntityDestroy) {
        m_callbacks.onEntityDestroy({packet.collectedEntityId()});
    }
}

void NetworkClient::_handleGameStateChange(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::_handleGameStateChange");

    // GameStateChangePacket 使用原始数据 deserialize，需要获取底层数据
    const u8* data = deser.data();
    size_t size = deser.size();

    network::GameStateChangePacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize GameStateChange packet: {}", result.error().message());
        return;
    }

    const auto reason = packet.reason();
    const f32 value = packet.value();

    switch (reason) {
        case network::GameStateChangeReason::EndRaining:
            if (m_callbacks.onEndRaining) {
                m_callbacks.onEndRaining();
            }
            break;

        case network::GameStateChangeReason::BeginRaining:
            if (m_callbacks.onBeginRaining) {
                m_callbacks.onBeginRaining();
            }
            break;

        case network::GameStateChangeReason::RainStrengthChange:
            if (m_callbacks.onRainStrengthChange) {
                m_callbacks.onRainStrengthChange(value);
            }
            break;

        case network::GameStateChangeReason::ThunderStrengthChange:
            if (m_callbacks.onThunderStrengthChange) {
                m_callbacks.onThunderStrengthChange(value);
            }
            break;

        case network::GameStateChangeReason::ChangeGameMode:
            spdlog::info("Game mode changed to {}", static_cast<i32>(value));
            if (m_callbacks.onGameModeChange) {
                m_callbacks.onGameModeChange(static_cast<GameMode>(static_cast<i32>(value)));
            }
            break;

        default:
            spdlog::warn("GameStateChange: unhandled reason={}, value={}", static_cast<u8>(reason), value);
            break;
    }
}

void NetworkClient::_handlePlayerAbilities(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::_handlePlayerAbilities");

    const u8* data = deser.data();
    size_t size = deser.size();

    network::PlayerAbilitiesPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize PlayerAbilities packet: {}", result.error().message());
        return;
    }

    spdlog::info("[NetworkClient::_handlePlayerAbilities] PlayerAbilities: invulnerable={}, flying={}, canFly={}, "
                 "creativeMode={}, flySpeed={}, walkSpeed={}",
        packet.invulnerable(),
        packet.flying(),
        packet.canFly(),
        packet.creativeMode(),
        packet.flySpeed(),
        packet.walkSpeed());

    if (m_callbacks.onPlayerAbilities) {
        m_callbacks.onPlayerAbilities(packet.invulnerable(),
            packet.flying(),
            packet.canFly(),
            packet.creativeMode(),
            packet.flySpeed(),
            packet.walkSpeed());
    }
}

void NetworkClient::_handleServerDifficulty(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    network::ServerDifficultyPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize ServerDifficulty packet: {}", result.error().message());
        return;
    }

    spdlog::info("[NetworkClient::_handleServerDifficulty] Difficulty: {}, locked: {}",
        static_cast<i32>(packet.difficulty()),
        packet.locked());

    if (m_callbacks.onDifficultyChange) {
        m_callbacks.onDifficultyChange(packet.difficulty(), packet.locked());
    }
}

void NetworkClient::_handleLightUpdate(network::PacketDeserializer& deser)
{
    auto result = network::LightUpdatePacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize LightUpdate packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Lighting,
        "NetworkClient::_handleLightUpdate",
        "Section",
        fmt::format("({}, {}, {})", packet.chunkX(), packet.sectionY(), packet.chunkZ()),
        "SkyLightSize",
        packet.skyLight().size(),
        "BlockLightSize",
        packet.blockLight().size(),
        [flow = ::perfetto::Flow::ProcessScoped(
             SectionPos(packet.chunkX(), packet.sectionY(), packet.chunkZ()).toLong())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    if (m_callbacks.onLightUpdate) {
        m_callbacks.onLightUpdate(packet.chunkX(),
            packet.chunkZ(),
            packet.sectionY(),
            packet.skyLight(),
            packet.blockLight(),
            packet.trustEdges());
    }
}

void NetworkClient::_handleBlockBreakAnim(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    network::BlockBreakAnimPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize BlockBreakAnim packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onBlockBreakAnim) {
        m_callbacks.onBlockBreakAnim(
            packet.breakerEntityId(), packet.position().x, packet.position().y, packet.position().z, packet.stage());
    }
}

void NetworkClient::_handleBlockEvent(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    network::BlockEventPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize BlockEvent packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onBlockEvent) {
        m_callbacks.onBlockEvent(packet.position().x,
            packet.position().y,
            packet.position().z,
            packet.paramA(),
            packet.paramB(),
            packet.blockStateId());
    }
}

void NetworkClient::_handlePlaySound(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Sound, "NetworkClient::_handlePlaySound");

    const u8* data = deser.data();
    size_t size = deser.size();

    sound::PlaySoundPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize PlaySound packet: {}", result.error().message());
        return;
    }

    const glm::vec3 pos = packet.getPosition();

    if (m_callbacks.onPlaySound) {
        m_callbacks.onPlaySound(
            packet.getSoundEventId(), packet.getCategory(), pos.x, pos.y, pos.z, packet.getVolume(), packet.getPitch());
    }
}

void NetworkClient::_handleStopSound(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Sound, "NetworkClient::_handleStopSound");

    const u8* data = deser.data();
    size_t size = deser.size();

    sound::StopSoundPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize StopSound packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onStopSound) {
        m_callbacks.onStopSound(packet.getSoundEventId(), packet.getCategory());
    }
}

void NetworkClient::_handlePlaySoundEffect(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Sound, "NetworkClient::_handlePlaySoundEffect");

    const u8* data = deser.data();
    size_t size = deser.size();

    sound::PlaySoundEffectPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize PlaySoundEffect packet: {}", result.error().message());
        return;
    }

    // 当前客户端没有区分 sound 与 sound effect，统一通过 onPlaySound 回调播放
    if (m_callbacks.onPlaySound) {
        const glm::vec3 pos = packet.getPosition();
        m_callbacks.onPlaySound(
            packet.getSoundEventId(), packet.getCategory(), pos.x, pos.y, pos.z, packet.getVolume(), packet.getPitch());
    }
}

void NetworkClient::_handleSetExperience(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    network::SetExperiencePacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize SetExperience packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onSetExperience) {
        m_callbacks.onSetExperience(packet.progress(), packet.totalXp(), packet.level());
    }
}

void NetworkClient::_handleSpawnExperienceOrb(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    network::SpawnExperienceOrbPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize SpawnExperienceOrb packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onSpawnExperienceOrb) {
        m_callbacks.onSpawnExperienceOrb(
            static_cast<u32>(packet.entityId()), packet.x(), packet.y(), packet.z(), packet.xpValue());
    }
}

void NetworkClient::_handlePlayerListItem(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    skin::PlayerListItemPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize PlayerListItem packet: {}", result.error().message());
        return;
    }

    switch (packet.action()) {
        case skin::PlayerListAction::AddPlayer: {
            if (m_callbacks.onPlayerListAdd) {
                m_callbacks.onPlayerListAdd(packet.entries());
            }
            break;
        }

        case skin::PlayerListAction::RemovePlayer: {
            if (m_callbacks.onPlayerListRemove) {
                std::vector<std::array<u8, 16>> uuids;
                uuids.reserve(packet.entries().size());
                for (const auto& entry : packet.entries()) {
                    uuids.push_back(entry.uuid);
                }
                m_callbacks.onPlayerListRemove(uuids);
            }
            break;
        }

        case skin::PlayerListAction::UpdateGameMode: {
            if (m_callbacks.onPlayerListUpdateGameMode) {
                for (const auto& entry : packet.entries()) {
                    m_callbacks.onPlayerListUpdateGameMode(entry);
                }
            }
            break;
        }

        case skin::PlayerListAction::UpdateLatency: {
            if (m_callbacks.onPlayerListUpdateLatency) {
                for (const auto& entry : packet.entries()) {
                    m_callbacks.onPlayerListUpdateLatency(entry.uuid, entry.ping);
                }
            }
            break;
        }

        case skin::PlayerListAction::UpdateDisplayName: {
            if (m_callbacks.onPlayerListUpdateDisplayName) {
                for (const auto& entry : packet.entries()) {
                    m_callbacks.onPlayerListUpdateDisplayName(entry.uuid, entry.displayName);
                }
            }
            break;
        }
    }
}

void NetworkClient::_handleParticle(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    network::ParticlePacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize ParticlePacket: {}", result.error().message());
        return;
    }

    if (m_callbacks.onParticle) {
        m_callbacks.onParticle(packet.particleType(),
            packet.x(),
            packet.y(),
            packet.z(),
            packet.velocityX(),
            packet.velocityY(),
            packet.velocityZ(),
            packet.offsetX(),
            packet.offsetY(),
            packet.offsetZ(),
            packet.count());
    }

    // 振动粒子特殊处理：解码目标位置来源和到达时间
    if (packet.isVibrationParticle() && m_callbacks.onVibrationParticle) {
        auto target = packet.decodeVibrationTarget();
        auto arrivalInTicks = packet.decodeVibrationArrivalInTicks();
        if (target.has_value() && arrivalInTicks.has_value()) {
            // 方块来源：目标位置为方块中心 (x+0.5, y+0.5, z+0.5)，对应 MC Java Vec3.atCenterOf
            // 实体来源：目标位置暂设为粒子起始位置，由 ClientApplicationNetwork 解析实体位置
            f64 targetX = packet.x();
            f64 targetY = packet.y();
            f64 targetZ = packet.z();
            EntityId targetEntityId = INVALID_ENTITY_ID;
            f32 yOffset = 0.0f;
            const u8 targetKind = static_cast<u8>(target->kind);
            if (target->kind == network::ParticlePacket::VibrationTarget::Kind::Block) {
                targetX = static_cast<f64>(target->blockPos.x) + 0.5;
                targetY = static_cast<f64>(target->blockPos.y) + 0.5;
                targetZ = static_cast<f64>(target->blockPos.z) + 0.5;
            } else {
                targetEntityId = target->entityId;
                yOffset = target->yOffset;
            }
            m_callbacks.onVibrationParticle(packet.x(),
                packet.y(),
                packet.z(),
                targetKind,
                targetX,
                targetY,
                targetZ,
                targetEntityId,
                yOffset,
                arrivalInTicks.value());
        }
    }

    // 轨迹粒子特殊处理：解码目标位置、颜色和持续时间
    if (packet.isTrailParticle() && m_callbacks.onTrailParticle) {
        auto target = packet.decodeTrailTarget();
        auto color = packet.decodeTrailColor();
        auto duration = packet.decodeTrailDuration();
        if (target.has_value() && color.has_value() && duration.has_value()) {
            m_callbacks.onTrailParticle(
                packet.x(), packet.y(), packet.z(), target->x, target->y, target->z, color.value(), duration.value());
        }
    }

    // 灰尘粒子特殊处理：解码 ARGB 颜色和缩放
    if (packet.isDustParticle() && m_callbacks.onDustParticle) {
        auto color = packet.decodeDustColor();
        auto scale = packet.decodeDustScale();
        if (color.has_value() && scale.has_value()) {
            m_callbacks.onDustParticle(packet.particleType(),
                packet.x(),
                packet.y(),
                packet.z(),
                packet.velocityX(),
                packet.velocityY(),
                packet.velocityZ(),
                packet.offsetX(),
                packet.offsetY(),
                packet.offsetZ(),
                packet.count(),
                color.value(),
                scale.value());
        }
    }

    // 颜色过渡灰尘粒子特殊处理：解码起始颜色、目标颜色和缩放
    if (packet.isDustColorTransitionParticle() && m_callbacks.onDustColorTransitionParticle) {
        auto fromColor = packet.decodeDustColorTransitionFromColor();
        auto toColor = packet.decodeDustColorTransitionToColor();
        auto scale = packet.decodeDustColorTransitionScale();
        if (fromColor.has_value() && toColor.has_value() && scale.has_value()) {
            m_callbacks.onDustColorTransitionParticle(packet.x(),
                packet.y(),
                packet.z(),
                packet.velocityX(),
                packet.velocityY(),
                packet.velocityZ(),
                packet.offsetX(),
                packet.offsetY(),
                packet.offsetZ(),
                packet.count(),
                fromColor.value(),
                toColor.value(),
                scale.value());
        }
    }

    // 实体效果粒子特殊处理：解码 ARGB 颜色
    if (packet.isEntityEffectParticle() && m_callbacks.onEntityEffectParticle) {
        auto color = packet.decodeEntityEffectColor();
        if (color.has_value()) {
            m_callbacks.onEntityEffectParticle(packet.x(),
                packet.y(),
                packet.z(),
                packet.velocityX(),
                packet.velocityY(),
                packet.velocityZ(),
                packet.offsetX(),
                packet.offsetY(),
                packet.offsetZ(),
                packet.count(),
                color.value());
        }
    }

    // 方块粒子特殊处理：解码方块状态 ID
    if (packet.isBlockParticle() && m_callbacks.onBlockParticle) {
        auto stateId = packet.decodeBlockStateId();
        if (stateId.has_value()) {
            m_callbacks.onBlockParticle(packet.particleType(),
                packet.x(),
                packet.y(),
                packet.z(),
                packet.velocityX(),
                packet.velocityY(),
                packet.velocityZ(),
                packet.offsetX(),
                packet.offsetY(),
                packet.offsetZ(),
                packet.count(),
                stateId.value());
        }
    }

    // 物品粒子特殊处理：解码物品堆
    if (packet.isItemParticle() && m_callbacks.onItemParticle) {
        auto itemStack = packet.decodeItemStack();
        if (itemStack.has_value()) {
            m_callbacks.onItemParticle(packet.particleType(),
                packet.x(),
                packet.y(),
                packet.z(),
                packet.velocityX(),
                packet.velocityY(),
                packet.velocityZ(),
                packet.offsetX(),
                packet.offsetY(),
                packet.offsetZ(),
                packet.count(),
                itemStack.value());
        }
    }
}

void NetworkClient::_handleMovingSound(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Sound, "NetworkClient::_handleMovingSound");

    const u8* data = deser.data();
    size_t size = deser.size();

    sound::MovingSoundPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize MovingSound packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onMovingSound) {
        m_callbacks.onMovingSound(packet.getSoundEventId(),
            packet.getCategory(),
            packet.getEntityId(),
            packet.getVolume(),
            packet.getPitch());
    }
}

void NetworkClient::_handleWorldEvent(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    sound::WorldEventPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize WorldEvent packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onWorldEvent) {
        m_callbacks.onWorldEvent(packet.getEventId(), packet.getX(), packet.getY(), packet.getZ(), packet.getData());
    }
}

void NetworkClient::_handleSetPassengers(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    network::SetPassengersPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize SetPassengers packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onSetPassengers) {
        m_callbacks.onSetPassengers(packet.entityId(), packet.passengerIds());
    }
}

void NetworkClient::_handleSetCamera(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    network::SetCameraPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize SetCamera packet: {}", result.error().message());
        return;
    }

    spdlog::info("SetCamera: camera entity id = {}", packet.cameraEntityId());

    if (m_callbacks.onSetCamera) {
        m_callbacks.onSetCamera(packet.cameraEntityId());
    }
}

void NetworkClient::_handleRespawn(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::_handleRespawn");

    const u8* data = deser.data();
    size_t size = deser.size();

    network::RespawnPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize Respawn packet: {}", result.error().message());
        return;
    }

    spdlog::info("[NetworkClient] Received Respawn: dimensionType={}, dimension={}, gameMode={}, keepData={}",
        packet.dimensionType(),
        static_cast<i32>(packet.dimension()),
        static_cast<i32>(packet.gameMode()),
        packet.keepData());

    if (m_callbacks.onRespawn) {
        m_callbacks.onRespawn(packet.dimensionType(),
            packet.dimension(),
            packet.hashedSeed(),
            packet.gameMode(),
            packet.previousGameMode(),
            packet.isDebug(),
            packet.isFlat(),
            packet.keepData(),
            packet.lastDeathLocation());
    }
}

void NetworkClient::_handleDimensionInfo(network::PacketDeserializer& deser)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "NetworkClient::_handleDimensionInfo");

    const u8* data = deser.data();
    size_t size = deser.size();

    network::DimensionInfoPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize DimensionInfo packet: {}", result.error().message());
        return;
    }

    spdlog::info("[NetworkClient] Received DimensionInfo: {} dimensions", packet.count());

    if (m_callbacks.onDimensionInfo) {
        // 转换为 tuple 格式
        std::vector<std::tuple<DimensionId, std::string, bool, bool, f32>> dimensions;
        dimensions.reserve(packet.dimensions().size());
        for (const auto& dim : packet.dimensions()) {
            dimensions.emplace_back(dim.id, dim.name, dim.hasSkyLight, dim.hasCeiling, dim.ambientLight);
        }
        m_callbacks.onDimensionInfo(dimensions);
    }
}

void NetworkClient::_handleSpawnPosition(network::PacketDeserializer& deser)
{
    network::SpawnPositionPacket packet;
    auto result = packet.deserialize(deser.data(), deser.size());
    if (result.failed()) {
        spdlog::error("Failed to deserialize SpawnPosition packet: {}", result.error().message());
        return;
    }

    const auto& pos = packet.position();

    spdlog::info("[NetworkClient] Received SpawnPosition: ({}, {}, {})", pos.x, pos.y, pos.z);

    if (m_callbacks.onSpawnPosition) {
        m_callbacks.onSpawnPosition(pos.x, pos.y, pos.z, packet.angle());
    }
}

void NetworkClient::_handleMapData(network::PacketDeserializer& deser)
{
    network::MapDataPacket packet;
    auto result = packet.deserialize(deser.data(), deser.size());
    if (result.failed()) {
        spdlog::error("Failed to deserialize MapData packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onMapData) {
        m_callbacks.onMapData(packet);
    }
}

void NetworkClient::_handleVehicleMove(network::PacketDeserializer& deser)
{
    network::VehicleMovePacket packet;
    auto result = packet.deserialize(deser.data(), deser.size());
    if (result.failed()) {
        spdlog::error("Failed to deserialize VehicleMove packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onVehicleMove) {
        m_callbacks.onVehicleMove(packet.x(), packet.y(), packet.z(), packet.yaw(), packet.pitch());
    }
}

void NetworkClient::_handleSleep(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    network::SleepPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize Sleep packet: {}", result.error().message());
        return;
    }

    if (packet.isSleeping()) {
        const auto& bedPos = packet.bedPosition();
        spdlog::info("[NetworkClient] Received Sleep: entity {} sleeping at ({}, {}, {})",
            packet.entityId(),
            bedPos->x,
            bedPos->y,
            bedPos->z);
    } else {
        spdlog::info("[NetworkClient] Received Sleep: entity {} woke up", packet.entityId());
    }

    if (m_callbacks.onSleep) {
        if (packet.isSleeping()) {
            const auto& bedPos = packet.bedPosition();
            m_callbacks.onSleep(packet.entityId(), true, bedPos->x, bedPos->y, bedPos->z);
        } else {
            m_callbacks.onSleep(packet.entityId(), false, 0, 0, 0);
        }
    }
}

void NetworkClient::_handleHotbarSet(network::PacketDeserializer& deser)
{
    auto result = HotbarSetPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize HotbarSet packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();

    if (m_callbacks.onHotbarSet) {
        m_callbacks.onHotbarSet(packet.slot());
    }
}

void NetworkClient::_handleTitle(network::PacketDeserializer& deser)
{
    const u8* data = deser.data();
    size_t size = deser.size();

    network::TitlePacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize Title packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onTitle) {
        m_callbacks.onTitle(packet.action(), packet.text(), packet.fadeIn(), packet.stay(), packet.fadeOut());
    }
}

} // namespace mc::client
