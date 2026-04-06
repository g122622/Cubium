#include "NetworkClient.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/GameStateChangePacket.hpp"
#include "common/network/packet/PlayerAbilitiesPacket.hpp"
#include "common/network/packet/BlockBreakAnimPacket.hpp"
#include "common/sound/network/SoundPackets.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/ChunkPos.hpp"
#include <chrono>
#include <spdlog/spdlog.h>
#include "common/perfetto/TraceEvents.hpp"

namespace mc::client {

// ============================================================================
// 常量
// ============================================================================

namespace {
    constexpr size_t RECEIVE_BUFFER_SIZE = 64 * 1024;  // 64KB
    constexpr size_t MAX_PACKET_SIZE = 2 * 1024 * 1024; // 2MB
}

// ============================================================================
// NetworkClient 实现
// ============================================================================

NetworkClient::NetworkClient()
    : m_receiveBuffer(RECEIVE_BUFFER_SIZE)
    , m_packetBuffer()
{
}

NetworkClient::~NetworkClient() {
    disconnect("Destructor");
}

Result<void> NetworkClient::connect(const NetworkClientConfig& config) {
    if (m_state != ClientState::Disconnected) {
        return Error(ErrorCode::InvalidState, "Already connected or connecting");
    }

    if (m_localEndpoint) {
        return Error(ErrorCode::InvalidState, "Cannot use TCP connect with local connection");
    }

    m_config = config;
    m_username = config.username;
    m_socket = std::make_unique<asio::ip::tcp::socket>(m_ioContext);

    setState(ClientState::Connecting);

    try {
        // 解析服务器地址
        asio::ip::tcp::resolver resolver(m_ioContext);
        auto endpoints = resolver.resolve(
            config.serverAddress,
            std::to_string(config.serverPort)
        );

        // 同步连接 (在主线程)
        asio::connect(*m_socket, endpoints);

        // 设置 TCP 选项
        m_socket->set_option(asio::ip::tcp::no_delay(true));
        m_socket->set_option(asio::socket_base::keep_alive(true));

        setState(ClientState::LoggingIn);

        // 启动接收线程
        m_running = true;
        m_ioThread = std::make_unique<std::thread>([this]() {
            receiveLoop();
        });

        // 发送登录请求
        sendLoginRequest();

        spdlog::info("Connected to {}:{}", config.serverAddress, config.serverPort);
        return Result<void>::ok();

    } catch (const std::exception& e) {
        setState(ClientState::Disconnected);
        spdlog::error("Failed to connect: {}", e.what());
        return Error(ErrorCode::ConnectionFailed, e.what());
    }
}

Result<void> NetworkClient::connectLocal(network::LocalEndpoint* endpoint,
                                         const NetworkClientConfig& config) {
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

    setState(ClientState::LoggingIn);

    // 本地连接模式：无需 IO 线程，直接发送登录请求
    sendLoginRequest();

    spdlog::info("Connected to integrated server (local)");
    return Result<void>::ok();
}

void NetworkClient::disconnect(const String& reason) {
    if (m_state == ClientState::Disconnected) {
        return;
    }

    setState(ClientState::Disconnecting);
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

    setState(ClientState::Disconnected);

    if (m_callbacks.onDisconnected) {
        m_callbacks.onDisconnected(reason);
    }

    spdlog::info("Disconnected: {}", reason);
}

bool NetworkClient::isConnected() const {
    if (m_localEndpoint) {
        return m_localEndpoint->isConnected() && m_state != ClientState::Disconnected;
    }
    return m_socket && m_socket->is_open() && m_state != ClientState::Disconnected;
}

ClientState NetworkClient::state() const {
    return m_state;
}

void NetworkClient::setCallbacks(const NetworkClientCallbacks& callbacks) {
    m_callbacks = callbacks;
}

void NetworkClient::sendLoginRequest() {
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

    sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendPlayerMove(const network::PlayerPosition& pos, network::PlayerMovePacket::MoveType type) {
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

    sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendBlockInteraction(network::BlockInteractionAction action,
                                         i32 x, i32 y, i32 z, Direction face) {
    network::BlockInteractionPacket packet(action, x, y, z, face);

    // spdlog::debug("[Mining] Queue block interaction action={} pos=({}, {}, {}) face={}",
    //              static_cast<i32>(action), x, y, z, static_cast<i32>(face));

    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::BlockInteraction));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendBlockPlacement(i32 x, i32 y, i32 z, Direction face,
                                        f32 hitX, f32 hitY, f32 hitZ, u8 hand) {
    network::PlayerTryUseItemOnBlockPacket packet(x, y, z, face, hitX, hitY, hitZ, hand);

    spdlog::info("[Place] Send block placement pos=({}, {}, {}) face={} hit=({:.2f}, {:.2f}, {:.2f})",
                 x, y, z, static_cast<i32>(face), hitX, hitY, hitZ);

    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::PlayerTryUseItemOnBlock));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendHotbarSelect(i32 slot) {
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

    sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendTeleportConfirm(u32 teleportId) {
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

    sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendKeepAlive(u64 id) {
    network::KeepAlivePacket packet;
    packet.setTimestamp(id);

    auto result = packet.serialize();
    if (result.success()) {
        sendRawData(result.value().data(), result.value().size());
    }

    m_lastKeepAliveSent = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void NetworkClient::sendChatMessage(const String& message) {
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

    sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendContainerClick(const ContainerClickPacket& packet) {
    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::ContainerClick));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendCloseContainer(ContainerId containerId) {
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

    sendRawData(fullPacket.data(), fullPacket.size());
}

void NetworkClient::sendKeepAliveIfNeeded() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    if (now - m_lastKeepAliveSent >= m_config.keepAliveIntervalMs) {
        sendKeepAlive(static_cast<u64>(now));
    }
}

void NetworkClient::poll() {
    if (!m_running) return;

    if (m_localEndpoint) {
        // 本地连接模式：直接从队列读取
        std::vector<u8> data;
        while (m_localEndpoint->receive(data)) {
            processPacket(data.data(), data.size());
            m_packetsReceived++;
        }
        sendKeepAliveIfNeeded();
        return;
    }

    // TCP 模式原有逻辑
    // 处理接收到的数据包
    processIncomingData();

    // 发送心跳
    sendKeepAliveIfNeeded();
}

void NetworkClient::receiveLoop() {
    while (m_running) {
        try {
            size_t bytesRead = m_socket->read_some(
                asio::buffer(m_receiveBuffer)
            );

            if (bytesRead > 0) {
                m_bytesReceived += bytesRead;

                // 将数据添加到处理缓冲区
                std::lock_guard<std::mutex> lock(m_receiveMutex);
                m_packetBuffer.insert(
                    m_packetBuffer.end(),
                    m_receiveBuffer.begin(),
                    m_receiveBuffer.begin() + bytesRead
                );
            }
        } catch (const asio::system_error& e) {
            if (m_running) {
                spdlog::error("Receive error: {}", e.what());
                disconnect("Connection error: " + String(e.what()));
            }
            break;
        }
    }
}

void NetworkClient::processIncomingData() {
    std::vector<u8> dataToProcess;
    {
        std::lock_guard<std::mutex> lock(m_receiveMutex);
        dataToProcess = std::move(m_packetBuffer);
        m_packetBuffer.clear();
    }

    size_t offset = 0;
    while (offset + network::PACKET_HEADER_SIZE <= dataToProcess.size()) {
        // 读取包大小
        u32 packetSize =
            (static_cast<u32>(dataToProcess[offset]) << 24) |
            (static_cast<u32>(dataToProcess[offset + 1]) << 16) |
            (static_cast<u32>(dataToProcess[offset + 2]) << 8) |
            static_cast<u32>(dataToProcess[offset + 3]);

        if (packetSize > MAX_PACKET_SIZE) {
            spdlog::error("Packet too large: {} bytes", packetSize);
            disconnect("Invalid packet size");
            return;
        }

        if (offset + packetSize > dataToProcess.size()) {
            // 数据不完整，等待更多数据
            break;
        }

        // 处理完整数据包
        processPacket(dataToProcess.data() + offset, packetSize);
        m_packetsReceived++;

        offset += packetSize;
    }

    // 保留未处理的数据
    if (offset < dataToProcess.size()) {
        std::lock_guard<std::mutex> lock(m_receiveMutex);
        m_packetBuffer.insert(
            m_packetBuffer.begin(),
            dataToProcess.begin() + offset,
            dataToProcess.end()
        );
    }
}

void NetworkClient::processPacket(const u8* data, size_t size) {
    if (size < network::PACKET_HEADER_SIZE) return;

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
                handleKeepAlive(packet.timestamp());
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
            handleLoginResponse(bodyDeser);
            break;
        }

        case network::PacketType::Teleport: {
            handleTeleport(bodyDeser);
            break;
        }

        case network::PacketType::ChunkData: {
            handleChunkData(bodyDeser);
            break;
        }

        case network::PacketType::UnloadChunk: {
            handleUnloadChunk(bodyDeser);
            break;
        }

        case network::PacketType::PlayerSpawn: {
            handlePlayerSpawn(bodyDeser);
            break;
        }

        case network::PacketType::PlayerDespawn: {
            handlePlayerDespawn(bodyDeser);
            break;
        }

        case network::PacketType::BlockUpdate: {
            handleBlockUpdate(bodyDeser);
            break;
        }

        case network::PacketType::ChatBroadcast: {
            handleChatMessage(bodyDeser);
            break;
        }

        case network::PacketType::TimeUpdate: {
            handleTimeUpdate(bodyDeser);
            break;
        }

        case network::PacketType::PlayerInventory: {
            handlePlayerInventory(bodyDeser);
            break;
        }

        case network::PacketType::OpenContainer: {
            handleOpenContainer(bodyDeser);
            break;
        }

        case network::PacketType::ContainerContent: {
            handleContainerContent(bodyDeser);
            break;
        }

        case network::PacketType::ContainerSlot: {
            handleContainerSlot(bodyDeser);
            break;
        }

        case network::PacketType::CloseContainer: {
            handleCloseContainer(bodyDeser);
            break;
        }

        // ========== 实体包 ==========

        case network::PacketType::SpawnEntity: {
            handleSpawnEntity(bodyDeser);
            break;
        }

        case network::PacketType::SpawnMob: {
            handleSpawnMob(bodyDeser);
            break;
        }

        case network::PacketType::EntityDestroy: {
            handleEntityDestroy(bodyDeser);
            break;
        }

        case network::PacketType::EntityMove: {
            handleEntityMove(bodyDeser);
            break;
        }

        case network::PacketType::EntityTeleport: {
            handleEntityTeleport(bodyDeser);
            break;
        }

        case network::PacketType::EntityVelocity: {
            handleEntityVelocity(bodyDeser);
            break;
        }

        case network::PacketType::EntityMetadata: {
            handleEntityMetadata(bodyDeser);
            break;
        }

        case network::PacketType::EntityAnimation: {
            handleEntityAnimation(bodyDeser);
            break;
        }

        case network::PacketType::EntityHeadLook: {
            handleEntityHeadLook(bodyDeser);
            break;
        }

        case network::PacketType::EntityStatus: {
            handleEntityStatus(bodyDeser);
            break;
        }

        case network::PacketType::GameStateChange: {
            handleGameStateChange(bodyDeser);
            break;
        }

        case network::PacketType::PlayerAbilities: {
            handlePlayerAbilities(bodyDeser);
            break;
        }

        case network::PacketType::LightUpdate: {
            handleLightUpdate(bodyDeser);
            break;
        }

        case network::PacketType::BlockBreakAnim: {
            handleBlockBreakAnim(bodyDeser);
            break;
        }

        case network::PacketType::PlaySound: {
            handlePlaySound(bodyDeser);
            break;
        }

        case network::PacketType::StopSound: {
            handleStopSound(bodyDeser);
            break;
        }

        case network::PacketType::PlaySoundEffect: {
            handlePlaySoundEffect(bodyDeser);
            break;
        }

        case network::PacketType::SetExperience: {
            handleSetExperience(bodyDeser);
            break;
        }

        case network::PacketType::SpawnExperienceOrb: {
            handleSpawnExperienceOrb(bodyDeser);
            break;
        }

        default:
            spdlog::debug("Unhandled packet type: {}", static_cast<int>(packetType));
            break;
    }
}

void NetworkClient::sendRawData(const u8* data, size_t size) {
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
    } catch (const std::exception& e) {
        spdlog::error("Send error: {}", e.what());
    }
}

void NetworkClient::setState(ClientState state) {
    m_state = state;
}

void NetworkClient::handleKeepAlive(u64 id) {
    m_lastKeepAliveReceived = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    // 计算延迟
    if (m_lastKeepAliveSent > 0) {
        m_ping = static_cast<u32>(m_lastKeepAliveReceived - m_lastKeepAliveSent);
    }

    // 回复心跳
    sendKeepAlive(id);
}

void NetworkClient::handleLoginResponse(network::PacketDeserializer& deser) {
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
        setState(ClientState::Playing);

        spdlog::info("Login successful: playerId={}", m_playerId);

        if (m_callbacks.onLoginSuccess) {
            m_callbacks.onLoginSuccess(m_playerId, response.username());
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

void NetworkClient::handleTeleport(network::PacketDeserializer& deser) {
    auto result = network::TeleportPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize teleport packet");
        return;
    }

    auto& packet = result.value();

    // 发送确认
    sendTeleportConfirm(packet.teleportId());

    // 回调通知
    if (m_callbacks.onTeleport) {
        m_callbacks.onTeleport(
            packet.x(), packet.y(), packet.z(),
            packet.yaw(), packet.pitch(),
            packet.teleportId()
        );
    }
}

void NetworkClient::handleChunkData(network::PacketDeserializer& deser) {
    auto result = network::ChunkDataPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize chunk data packet: {}", result.error().message());
        return;
    }

    auto& packet = result.value();
    spdlog::debug("Received chunk data: ({}, {}) dim={} size: {} bytes", packet.x(), packet.z(), packet.dimension(), packet.data().size());

    if (m_callbacks.onChunkData) {
        m_callbacks.onChunkData(packet.x(), packet.z(), packet.dimension(), packet.data());
    }
}

void NetworkClient::handleTimeUpdate(network::PacketDeserializer& deser) {
    auto result = network::TimeUpdatePacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize time update packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();

    if (m_callbacks.onTimeUpdate) {
        m_callbacks.onTimeUpdate(packet.gameTime(), packet.dayTime(), packet.daylightCycleEnabled());
    }
}

void NetworkClient::handleUnloadChunk(network::PacketDeserializer& deser) {
    auto result = network::UnloadChunkPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize unload chunk packet");
        return;
    }

    auto& packet = result.value();
    spdlog::debug("Received unload chunk: ({}, {}) dim={}", packet.x(), packet.z(), packet.dimension());

    if (m_callbacks.onChunkUnload) {
        m_callbacks.onChunkUnload(packet.x(), packet.z(), packet.dimension());
    }
}

void NetworkClient::handlePlayerSpawn(network::PacketDeserializer& deser) {
    auto result = network::PlayerSpawnPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize player spawn packet");
        return;
    }

    auto& packet = result.value();

    if (m_callbacks.onPlayerSpawn) {
        m_callbacks.onPlayerSpawn(
            packet.playerId(), packet.username(),
            packet.position().x, packet.position().y, packet.position().z
        );
    }
}

void NetworkClient::handlePlayerDespawn(network::PacketDeserializer& deser) {
    auto result = network::PlayerDespawnPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize player despawn packet");
        return;
    }

    auto& packet = result.value();

    if (m_callbacks.onPlayerDespawn) {
        m_callbacks.onPlayerDespawn(packet.playerId());
    }
}

void NetworkClient::handleBlockUpdate(network::PacketDeserializer& deser) {
    auto result = network::BlockUpdatePacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize block update packet");
        return;
    }

    auto& packet = result.value();

    // spdlog::debug("[Mining] Received block update pos=({}, {}, {}) stateId={}",
    //              packet.x(), packet.y(), packet.z(), packet.blockStateId());

    MC_TRACE_INSTANT("client.lighting",
        "ReceiveBlockUpdate",
        "pos", fmt::format("({}, {}, {})", packet.x(), packet.y(), packet.z()),
        "stateId", packet.blockStateId(),
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(packet.x(), packet.y(), packet.z()).toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    if (m_callbacks.onBlockUpdate) {
        m_callbacks.onBlockUpdate(
            packet.x(), packet.y(), packet.z(),
            packet.blockStateId()
        );
    }
}

void NetworkClient::handleChatMessage(network::PacketDeserializer& deser) {
    auto result = network::ChatMessagePacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize chat message packet");
        return;
    }

    auto& packet = result.value();

    if (m_callbacks.onChatMessage) {
        m_callbacks.onChatMessage(packet.message(), packet.senderId());
    }
}

void NetworkClient::handlePlayerInventory(network::PacketDeserializer& deser) {
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

void NetworkClient::handleOpenContainer(network::PacketDeserializer& deser) {
    auto result = OpenContainerPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize open container packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onOpenContainer) {
        m_callbacks.onOpenContainer(result.value());
    }
}

void NetworkClient::handleContainerContent(network::PacketDeserializer& deser) {
    auto result = ContainerContentPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize container content packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onContainerContent) {
        m_callbacks.onContainerContent(result.value());
    }
}

void NetworkClient::handleContainerSlot(network::PacketDeserializer& deser) {
    auto result = ContainerSlotPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize container slot packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onContainerSlot) {
        m_callbacks.onContainerSlot(result.value());
    }
}

void NetworkClient::handleCloseContainer(network::PacketDeserializer& deser) {
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

void NetworkClient::handleSpawnEntity(network::PacketDeserializer& deser) {
    // 获取原始数据指针
    const u8* data = deser.data();
    size_t size = deser.size();

    network::SpawnEntityPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize SpawnEntity packet: {}", result.error().message());
        return;
    }

    spdlog::debug("Received SpawnEntity: id={}, type={}, pos=({:.1f}, {:.1f}, {:.1f}){}",
                  packet.entityId(), packet.entityTypeId().c_str(),
                  packet.x(), packet.y(), packet.z(),
                  packet.hasItemStack() ? " (with ItemStack)" : "");

    if (m_callbacks.onSpawnEntity) {
        m_callbacks.onSpawnEntity(
            packet.entityId(),
            packet.entityTypeId(),
            packet.x(), packet.y(), packet.z(),
            packet.yaw(), packet.pitch(),
            packet.itemStack()
        );
    }
}

void NetworkClient::handleSpawnMob(network::PacketDeserializer& deser) {
    const u8* data = deser.data();
    size_t size = deser.size();

    network::SpawnMobPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize SpawnMob packet: {}", result.error().message());
        return;
    }

    spdlog::debug("Received SpawnMob: id={}, type={}, pos=({:.1f}, {:.1f}, {:.1f})",
                  packet.entityId(), packet.entityTypeId().c_str(),
                  packet.x(), packet.y(), packet.z());

    if (m_callbacks.onSpawnMob) {
        m_callbacks.onSpawnMob(
            packet.entityId(),
            packet.entityTypeId(),
            packet.x(), packet.y(), packet.z(),
            packet.yaw(), packet.pitch(), packet.headYaw()
        );
    }
}

void NetworkClient::handleEntityDestroy(network::PacketDeserializer& deser) {
    const u8* data = deser.data();
    size_t size = deser.size();

    network::EntityDestroyPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize EntityDestroy packet: {}", result.error().message());
        return;
    }

    spdlog::debug("Received EntityDestroy: {} entities", packet.entityIds().size());

    if (m_callbacks.onEntityDestroy) {
        m_callbacks.onEntityDestroy(packet.entityIds());
    }
}

void NetworkClient::handleEntityMove(network::PacketDeserializer& deser) {
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
        // 这里简化处理，发送相对移动信息
        m_callbacks.onEntityMove(
            packet.entityId(),
            packet.deltaX() / 32.0f,  // 转换为方块单位
            packet.deltaY() / 32.0f,
            packet.deltaZ() / 32.0f
        );
    }
}

void NetworkClient::handleEntityTeleport(network::PacketDeserializer& deser) {
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
            packet.entityId(),
            packet.x(), packet.y(), packet.z(),
            packet.yaw(), packet.pitch()
        );
    }
}

void NetworkClient::handleEntityVelocity(network::PacketDeserializer& deser) {
    const u8* data = deser.data();
    size_t size = deser.size();

    network::EntityVelocityPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize EntityVelocity packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onEntityVelocity) {
        m_callbacks.onEntityVelocity(
            packet.entityId(),
            packet.velocityX(),
            packet.velocityY(),
            packet.velocityZ()
        );
    }
}

void NetworkClient::handleEntityMetadata(network::PacketDeserializer& deser) {
    const u8* data = deser.data();
    size_t size = deser.size();

    network::EntityMetadataPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize EntityMetadata packet: {}", result.error().message());
        return;
    }

    spdlog::debug("Received EntityMetadata for entity {}, metadata size: {}",
                  packet.entityId(), packet.metadata().size());

    if (m_callbacks.onEntityMetadata) {
        m_callbacks.onEntityMetadata(packet.entityId(), packet.metadata());
    }
}

void NetworkClient::handleEntityAnimation(network::PacketDeserializer& deser) {
    const u8* data = deser.data();
    size_t size = deser.size();

    network::EntityAnimationPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize EntityAnimation packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onEntityAnimation) {
        m_callbacks.onEntityAnimation(
            packet.entityId(),
            static_cast<u8>(packet.animation())
        );
    }
}

void NetworkClient::handleEntityHeadLook(network::PacketDeserializer& deser) {
    const u8* data = deser.data();
    size_t size = deser.size();

    network::EntityHeadLookPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize EntityHeadLook packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onEntityHeadLook) {
        m_callbacks.onEntityHeadLook(
            packet.entityId(),
            packet.headYaw()
        );
    }
}

void NetworkClient::handleEntityStatus(network::PacketDeserializer& deser) {
    const u8* data = deser.data();
    size_t size = deser.size();

    network::EntityStatusPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize EntityStatus packet: {}", result.error().message());
        return;
    }

    if (m_callbacks.onEntityStatus) {
        m_callbacks.onEntityStatus(
            packet.entityId(),
            static_cast<u8>(packet.status())
        );
    }
}

void NetworkClient::handleGameStateChange(network::PacketDeserializer& deser) {
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
            spdlog::debug("Weather: EndRaining received");
            if (m_callbacks.onEndRaining) {
                m_callbacks.onEndRaining();
            }
            break;

        case network::GameStateChangeReason::BeginRaining:
            spdlog::debug("Weather: BeginRaining received");
            if (m_callbacks.onBeginRaining) {
                m_callbacks.onBeginRaining();
            }
            break;

        case network::GameStateChangeReason::RainStrengthChange:
            spdlog::debug("Weather: RainStrengthChange received, value={}", value);
            if (m_callbacks.onRainStrengthChange) {
                m_callbacks.onRainStrengthChange(value);
            }
            break;

        case network::GameStateChangeReason::ThunderStrengthChange:
            spdlog::debug("Weather: ThunderStrengthChange received, value={}", value);
            if (m_callbacks.onThunderStrengthChange) {
                m_callbacks.onThunderStrengthChange(value);
            }
            break;

        case network::GameStateChangeReason::ChangeGameMode:
            spdlog::info("Game mode changed to {}", static_cast<int>(value));
            if (m_callbacks.onGameModeChange) {
                m_callbacks.onGameModeChange(static_cast<GameMode>(static_cast<int>(value)));
            }
            break;

        default:
            spdlog::debug("GameStateChange: unhandled reason={}, value={}",
                          static_cast<u8>(reason), value);
            break;
    }
}

void NetworkClient::handlePlayerAbilities(network::PacketDeserializer& deser) {
    const u8* data = deser.data();
    size_t size = deser.size();

    network::PlayerAbilitiesPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize PlayerAbilities packet: {}", result.error().message());
        return;
    }

    spdlog::debug("PlayerAbilities: invulnerable={}, flying={}, canFly={}, creativeMode={}, flySpeed={}, walkSpeed={}",
                  packet.invulnerable(), packet.flying(), packet.canFly(),
                  packet.creativeMode(), packet.flySpeed(), packet.walkSpeed());

    if (m_callbacks.onPlayerAbilities) {
        m_callbacks.onPlayerAbilities(
            packet.invulnerable(),
            packet.flying(),
            packet.canFly(),
            packet.creativeMode(),
            packet.flySpeed(),
            packet.walkSpeed()
        );
    }
}

void NetworkClient::handleLightUpdate(network::PacketDeserializer& deser) {
    auto result = network::LightUpdatePacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to deserialize LightUpdate packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    SectionPos sectionPos(packet.chunkX(), packet.sectionY(), packet.chunkZ());
    MC_TRACE_INSTANT("client.lighting",
        "ReceiveLightUpdate",
        "Section", fmt::format("({}, {}, {})", packet.chunkX(), packet.sectionY(), packet.chunkZ()),
        "SkyLightSize", packet.skyLight().size(),
        "BlockLightSize", packet.blockLight().size(),
        [flow = ::perfetto::Flow::ProcessScoped(sectionPos.toLong())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    spdlog::debug("LightUpdate: chunk({}, {}, {}), skyLight={}B, blockLight={}B",
                  packet.chunkX(), packet.sectionY(), packet.chunkZ(),
                  packet.skyLight().size(), packet.blockLight().size());

    if (m_callbacks.onLightUpdate) {
        m_callbacks.onLightUpdate(
            packet.chunkX(),
            packet.chunkZ(),
            packet.sectionY(),
            packet.skyLight(),
            packet.blockLight(),
            packet.trustEdges()
        );
    }
}

void NetworkClient::handleBlockBreakAnim(network::PacketDeserializer& deser) {
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
            packet.breakerEntityId(),
            packet.position().x,
            packet.position().y,
            packet.position().z,
            packet.stage()
        );
    }
}

void NetworkClient::handlePlaySound(network::PacketDeserializer& deser) {
    const u8* data = deser.data();
    size_t size = deser.size();

    sound::PlaySoundPacket packet;
    auto result = packet.deserialize(data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize PlaySound packet: {}", result.error().message());
        return;
    }

    const glm::vec3 pos = packet.getPosition();
    spdlog::debug("[NetworkClient] Received PlaySound: {} at ({}, {}, {}) vol={} pitch={}",
                  packet.getSoundEventId().toString(), pos.x, pos.y, pos.z,
                  packet.getVolume(), packet.getPitch());

    if (m_callbacks.onPlaySound) {
        m_callbacks.onPlaySound(packet.getSoundEventId(),
                                packet.getCategory(),
                                pos.x,
                                pos.y,
                                pos.z,
                                packet.getVolume(),
                                packet.getPitch());
    }
}

void NetworkClient::handleStopSound(network::PacketDeserializer& deser) {
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

void NetworkClient::handlePlaySoundEffect(network::PacketDeserializer& deser) {
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
        m_callbacks.onPlaySound(packet.getSoundEventId(),
                                packet.getCategory(),
                                pos.x,
                                pos.y,
                                pos.z,
                                packet.getVolume(),
                                packet.getPitch());
    }
}

void NetworkClient::handleSetExperience(network::PacketDeserializer& deser) {
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

void NetworkClient::handleSpawnExperienceOrb(network::PacketDeserializer& deser) {
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
            static_cast<u32>(packet.entityId()),
            packet.x(),
            packet.y(),
            packet.z(),
            packet.xpValue()
        );
    }
}

} // namespace mc::client
