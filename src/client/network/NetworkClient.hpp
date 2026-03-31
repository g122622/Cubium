#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "../../common/entity/entities/player/Player.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include <asio.hpp>
#include <functional>
#include <memory>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace mc::client {

// ============================================================================
// 客户端网络状态
// ============================================================================

enum class ClientState : u8 {
    Disconnected = 0,
    Connecting = 1,
    LoggingIn = 2,
    Playing = 3,
    Disconnecting = 4
};

// ============================================================================
// 客户端配置
// ============================================================================

struct NetworkClientConfig {
    String serverAddress = "127.0.0.1";
    u16 serverPort = 25565;
    String username = "Player";
    u32 connectTimeoutMs = 5000;
    u32 keepAliveIntervalMs = 15000;  // 心跳间隔
    u32 reconnectDelayMs = 1000;       // 重连延迟
    bool autoReconnect = true;
};

// ============================================================================
// 客户端网络事件回调
// ============================================================================

struct NetworkClientCallbacks {
    // 连接事件
    std::function<void()> onConnected;
    std::function<void(const String& reason)> onDisconnected;
    std::function<void(const String& error)> onError;

    // 登录事件
    std::function<void(PlayerId playerId, const String& username)> onLoginSuccess;
    std::function<void(const String& reason)> onLoginFailed;

    // 游戏事件
    std::function<void(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId)> onTeleport;
    std::function<void(ChunkCoord x, ChunkCoord z, const std::vector<u8>& data)> onChunkData;
    std::function<void(ChunkCoord x, ChunkCoord z)> onChunkUnload;
    std::function<void(PlayerId playerId, const String& username, f64 x, f64 y, f64 z)> onPlayerSpawn;
    std::function<void(PlayerId playerId)> onPlayerDespawn;
    std::function<void(i32 x, i32 y, i32 z, u32 blockStateId)> onBlockUpdate;
    std::function<void(const String& message, PlayerId senderId)> onChatMessage;
    std::function<void(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch)> onPlayerMove;
    std::function<void(i64 gameTime, i64 dayTime, bool daylightCycleEnabled)> onTimeUpdate;
    std::function<void(i32 selectedSlot, const std::vector<ItemStack>& items)> onPlayerInventory;
    std::function<void(const OpenContainerPacket& packet)> onOpenContainer;
    std::function<void(const ContainerContentPacket& packet)> onContainerContent;
    std::function<void(const ContainerSlotPacket& packet)> onContainerSlot;
    std::function<void(ContainerId containerId)> onCloseContainer;

    // 实体事件
    std::function<void(u32 entityId, const String& typeId, f32 x, f32 y, f32 z, f32 yaw, f32 pitch, f32 headYaw)> onSpawnMob;
    std::function<void(u32 entityId, const String& typeId, f32 x, f32 y, f32 z, f32 yaw, f32 pitch, const ItemStack* itemStack)> onSpawnEntity;
    std::function<void(u32 entityId, f32 deltaX, f32 deltaY, f32 deltaZ)> onEntityMove;
    std::function<void(u32 entityId, i16 vx, i16 vy, i16 vz)> onEntityVelocity;
    std::function<void(u32 entityId, f32 x, f32 y, f32 z, f32 yaw, f32 pitch)> onEntityTeleport;
    std::function<void(const std::vector<u32>& entityIds)> onEntityDestroy;
    std::function<void(u32 entityId, u8 animation)> onEntityAnimation;
    std::function<void(u32 entityId, f32 headYaw)> onEntityHeadLook;
    std::function<void(u32 entityId, u8 status)> onEntityStatus;

    // 天气事件
    std::function<void(f32 rainStrength)> onRainStrengthChange;
    std::function<void(f32 thunderStrength)> onThunderStrengthChange;
    std::function<void()> onBeginRaining;
    std::function<void()> onEndRaining;

    // 游戏模式事件
    std::function<void(GameMode mode)> onGameModeChange;

    // 玩家能力事件
    std::function<void(bool invulnerable, bool flying, bool canFly, bool creativeMode, f32 flySpeed, f32 walkSpeed)> onPlayerAbilities;

    // 光照更新事件
    std::function<void(i32 chunkX, i32 chunkZ, i32 sectionY,
                       const std::vector<u8>& skyLight,
                       const std::vector<u8>& blockLight,
                       bool trustEdges)> onLightUpdate;

    // 方块破坏动画事件
    std::function<void(EntityId breakerEntityId, i32 x, i32 y, i32 z, i8 stage)> onBlockBreakAnim;

    // 声音事件
    std::function<void(const ResourceLocation& soundEventId,
                       mc::sound::SoundCategory category,
                       f32 x, f32 y, f32 z,
                       f32 volume,
                       f32 pitch)> onPlaySound;
    std::function<void(const Optional<ResourceLocation>& soundEventId,
                       const Optional<mc::sound::SoundCategory>& category)> onStopSound;

    // 实体元数据事件
    std::function<void(u32 entityId, const std::vector<u8>& metadata)> onEntityMetadata;
};

// ============================================================================
// 网络客户端
// ============================================================================

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    // 禁止拷贝
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    // 连接管理
    [[nodiscard]] Result<void> connect(const NetworkClientConfig& config);
    [[nodiscard]] Result<void> connectLocal(network::LocalEndpoint* endpoint,
                                            const NetworkClientConfig& config = {});
    void disconnect(const String& reason = "Client disconnect");
    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] ClientState state() const;
    [[nodiscard]] bool isLocalConnection() const { return m_localEndpoint != nullptr; }

    // 配置
    void setCallbacks(const NetworkClientCallbacks& callbacks);
    [[nodiscard]] const NetworkClientConfig& config() const { return m_config; }

    // 发送数据包
    void sendLoginRequest();
    void sendPlayerMove(const network::PlayerPosition& pos, network::PlayerMovePacket::MoveType type);
    void sendBlockInteraction(network::BlockInteractionAction action,
                              i32 x, i32 y, i32 z, Direction face);
    void sendBlockPlacement(i32 x, i32 y, i32 z, Direction face,
                            f32 hitX, f32 hitY, f32 hitZ, u8 hand = 0);
    void sendHotbarSelect(i32 slot);
    void sendTeleportConfirm(u32 teleportId);
    void sendKeepAlive(u64 id);
    void sendChatMessage(const String& message);
    void sendContainerClick(const ContainerClickPacket& packet);
    void sendCloseContainer(ContainerId containerId);

    // 心跳
    void sendKeepAliveIfNeeded();

    // 主循环更新
    void poll();

    // 统计
    [[nodiscard]] u64 bytesReceived() const { return m_bytesReceived; }
    [[nodiscard]] u64 bytesSent() const { return m_bytesSent; }
    [[nodiscard]] u64 packetsReceived() const { return m_packetsReceived; }
    [[nodiscard]] u64 packetsSent() const { return m_packetsSent; }
    [[nodiscard]] u32 ping() const { return m_ping; }

    // 玩家信息
    [[nodiscard]] PlayerId playerId() const { return m_playerId; }
    [[nodiscard]] const String& username() const { return m_username; }
    [[nodiscard]] bool isLoggedIn() const { return m_state == ClientState::Playing; }

private:
    // 内部方法
    void receiveLoop();
    void processIncomingData();
    void processPacket(const u8* data, size_t size);
    void sendRawData(const u8* data, size_t size);
    void sendPacket(const std::vector<u8>& packetData);
    void setState(ClientState state);
    void handleKeepAlive(u64 id);
    void handleLoginResponse(network::PacketDeserializer& deser);
    void handleTeleport(network::PacketDeserializer& deser);
    void handleChunkData(network::PacketDeserializer& deser);
    void handleUnloadChunk(network::PacketDeserializer& deser);
    void handlePlayerSpawn(network::PacketDeserializer& deser);
    void handlePlayerDespawn(network::PacketDeserializer& deser);
    void handleBlockUpdate(network::PacketDeserializer& deser);
    void handleChatMessage(network::PacketDeserializer& deser);
    void handleTimeUpdate(network::PacketDeserializer& deser);
    void handlePlayerInventory(network::PacketDeserializer& deser);
    void handleOpenContainer(network::PacketDeserializer& deser);
    void handleContainerContent(network::PacketDeserializer& deser);
    void handleContainerSlot(network::PacketDeserializer& deser);
    void handleCloseContainer(network::PacketDeserializer& deser);
    void handleDisconnect(network::PacketDeserializer& deser);

    // 实体包处理
    void handleSpawnEntity(network::PacketDeserializer& deser);
    void handleSpawnMob(network::PacketDeserializer& deser);
    void handleEntityDestroy(network::PacketDeserializer& deser);
    void handleEntityMove(network::PacketDeserializer& deser);
    void handleEntityTeleport(network::PacketDeserializer& deser);
    void handleEntityVelocity(network::PacketDeserializer& deser);
    void handleEntityMetadata(network::PacketDeserializer& deser);
    void handleEntityAnimation(network::PacketDeserializer& deser);
    void handleEntityHeadLook(network::PacketDeserializer& deser);
    void handleEntityStatus(network::PacketDeserializer& deser);

    // 天气包处理
    void handleGameStateChange(network::PacketDeserializer& deser);

    // 玩家能力包处理
    void handlePlayerAbilities(network::PacketDeserializer& deser);

    // 光照更新包处理
    void handleLightUpdate(network::PacketDeserializer& deser);

    // 方块破坏动画包处理
    void handleBlockBreakAnim(network::PacketDeserializer& deser);

    // 声音包处理
    void handlePlaySound(network::PacketDeserializer& deser);
    void handleStopSound(network::PacketDeserializer& deser);
    void handlePlaySoundEffect(network::PacketDeserializer& deser);

    // ASIO 网络
    asio::io_context m_ioContext;
    std::unique_ptr<asio::ip::tcp::socket> m_socket;
    std::unique_ptr<std::thread> m_ioThread;
    std::atomic<bool> m_running{false};

    // 本地连接模式
    network::LocalEndpoint* m_localEndpoint = nullptr;

    // 接收缓冲区
    std::vector<u8> m_receiveBuffer;
    std::vector<u8> m_packetBuffer;
    std::mutex m_receiveMutex;

    // 发送队列
    std::queue<std::vector<u8>> m_sendQueue;
    std::mutex m_sendMutex;

    // 状态
    std::atomic<ClientState> m_state{ClientState::Disconnected};
    NetworkClientConfig m_config;
    NetworkClientCallbacks m_callbacks;

    // 玩家信息
    PlayerId m_playerId = 0;
    String m_username;

    // 心跳
    u64 m_lastKeepAliveSent = 0;
    u64 m_lastKeepAliveReceived = 0;
    u32 m_ping = 0;

    // 统计
    std::atomic<u64> m_bytesReceived{0};
    std::atomic<u64> m_bytesSent{0};
    std::atomic<u64> m_packetsReceived{0};
    std::atomic<u64> m_packetsSent{0};
};

} // namespace mc::client
