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

#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/network/packet/BlockEntityDataPacket.hpp"
#include "common/network/packet/DimensionPackets.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/ExperiencePackets.hpp"
#include "common/network/packet/ExplosionPacket.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/packet/MapDataPacket.hpp"
#include "common/network/packet/ParticlePacket.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/network/packet/SignPackets.hpp"
#include "common/network/packet/SleepPacket.hpp"
#include "common/network/packet/SpawnPositionPacket.hpp"
#include "common/network/packet/TitlePacket.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/network/SkinPackets.hpp"
#include "common/sound/SoundCategory.hpp"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <asio.hpp>

namespace mc::particle {
enum class ParticleTypeId : u16;
}

namespace mc::client {

// ============================================================================
// 客户端网络状态
// ============================================================================

enum class ClientState : u8 { Disconnected = 0, Connecting = 1, LoggingIn = 2, Playing = 3, Disconnecting = 4 };

// ============================================================================
// 客户端配置
// ============================================================================

struct NetworkClientConfig {
    std::string serverAddress = "127.0.0.1";
    u16 serverPort = 25565;
    std::string username = "Player";
    u32 connectTimeoutMs = 5000;
    u32 keepAliveIntervalMs = 15000; // 心跳间隔
    u32 reconnectDelayMs = 1000;     // 重连延迟
    bool autoReconnect = true;
};

// ============================================================================
// 客户端网络事件回调
// ============================================================================

struct NetworkClientCallbacks {
    // 连接事件
    std::function<void()> onConnected;
    std::function<void(const std::string& reason)> onDisconnected;
    std::function<void(const std::string& error)> onError;

    // 登录事件
    std::function<void(PlayerId playerId, EntityId entityId, const std::string& username)> onLoginSuccess;
    std::function<void(const std::string& reason)> onLoginFailed;
    std::function<void(const std::string& treeJson)> onCommandTree;

    // 游戏事件
    std::function<void(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId)> onTeleport;
    std::function<void(ChunkCoord x, ChunkCoord z, DimensionId dimension, const std::vector<u8>& data)> onChunkData;
    std::function<void(ChunkCoord x, ChunkCoord z, DimensionId dimension)> onChunkUnload;
    std::function<void(PlayerId playerId, const std::string& username, f64 x, f64 y, f64 z)> onPlayerSpawn;
    std::function<void(PlayerId playerId)> onPlayerDespawn;
    std::function<void(i32 x, i32 y, i32 z, u32 blockStateId)> onBlockUpdate;
    std::function<void(const std::string& message, PlayerId senderId)> onChatMessage;
    std::function<void(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch)> onPlayerMove;
    std::function<void(i64 gameTime, i64 dayTime, bool daylightCycleEnabled)> onTimeUpdate;
    std::function<void(i32 selectedSlot, const std::vector<ItemStack>& items)> onPlayerInventory;
    std::function<void(const OpenContainerPacket& packet)> onOpenContainer;
    std::function<void(const network::OpenSignEditorPacket& packet)> onSignEditorOpen;
    std::function<void(const network::BlockEntityDataPacket& packet)> onBlockEntityData;
    std::function<void(const ContainerContentPacket& packet)> onContainerContent;
    std::function<void(const ContainerSlotPacket& packet)> onContainerSlot;
    std::function<void(ContainerId containerId)> onCloseContainer;

    // 实体事件
    std::function<void(u32 entityId, const std::string& typeId, f32 x, f32 y, f32 z, f32 yaw, f32 pitch, f32 headYaw)>
        onSpawnMob;
    std::function<void(u32 entityId,
        const std::string& typeId,
        f32 x,
        f32 y,
        f32 z,
        f32 yaw,
        f32 pitch,
        f32 vx,
        f32 vy,
        f32 vz,
        const ItemStack* itemStack)>
        onSpawnEntity;
    std::function<void(u32 entityId, f32 deltaX, f32 deltaY, f32 deltaZ)> onEntityMove;
    std::function<void(u32 entityId, i16 vx, i16 vy, i16 vz)> onEntityVelocity;
    std::function<void(u32 entityId, f32 x, f32 y, f32 z, f32 yaw, f32 pitch)> onEntityTeleport;
    std::function<void(const std::vector<u32>& entityIds)> onEntityDestroy;
    std::function<void(u32 entityId, u8 animation)> onEntityAnimation;
    std::function<void(u32 entityId, f32 headYaw)> onEntityHeadLook;
    std::function<void(u32 entityId, u8 status)> onEntityStatus;

    // 爆炸事件
    // 对应 MC Java ClientboundExplodePacket / ClientPacketListener.handleExplosion。
    // 客户端通过 addVelocity 累加击退向量到本地玩家速度（不是 setVelocity 覆盖）。
    // affectedBlocks 用于客户端方块破坏视觉；position/strength 用于粒子与音效。
    std::function<void(const network::ExplosionPacket& packet)> onExplosion;

    // 乘客事件
    std::function<void(u32 entityId, const std::vector<u32>& passengerIds)> onSetPassengers;

    // 旁观者摄像机事件
    std::function<void(u32 cameraEntityId)> onSetCamera;

    // 天气事件
    std::function<void(f32 rainStrength)> onRainStrengthChange;
    std::function<void(f32 thunderStrength)> onThunderStrengthChange;
    std::function<void()> onBeginRaining;
    std::function<void()> onEndRaining;

    // 游戏模式事件
    std::function<void(GameMode mode)> onGameModeChange;

    // 难度事件
    std::function<void(Difficulty difficulty, bool locked)> onDifficultyChange;

    // 玩家能力事件
    std::function<void(bool invulnerable, bool flying, bool canFly, bool creativeMode, f32 flySpeed, f32 walkSpeed)>
        onPlayerAbilities;

    // 光照更新事件
    std::function<void(i32 chunkX,
        i32 chunkZ,
        i32 sectionY,
        const std::vector<u8>& skyLight,
        const std::vector<u8>& blockLight,
        bool trustEdges)>
        onLightUpdate;

    // 方块破坏动画事件
    std::function<void(EntityId breakerEntityId, i32 x, i32 y, i32 z, i8 stage)> onBlockBreakAnim;

    // 方块事件（箱子开合、活塞动画等）
    std::function<void(i32 x, i32 y, i32 z, u8 paramA, u8 paramB, u32 blockStateId)> onBlockEvent;

    // 声音事件
    std::function<void(const ResourceLocation& soundEventId,
        mc::sound::SoundCategory category,
        f32 x,
        f32 y,
        f32 z,
        f32 volume,
        f32 pitch)>
        onPlaySound;
    std::function<void(
        const std::optional<ResourceLocation>& soundEventId, const std::optional<mc::sound::SoundCategory>& category)>
        onStopSound;
    std::function<void(
        const ResourceLocation& soundEventId, mc::sound::SoundCategory category, i32 entityId, f32 volume, f32 pitch)>
        onMovingSound;

    // 实体元数据事件
    std::function<void(u32 entityId, const std::vector<u8>& metadata)> onEntityMetadata;

    // 经验事件
    std::function<void(f32 progress, i32 totalXp, i32 level)> onSetExperience;
    std::function<void(u32 entityId, f64 x, f64 y, f64 z, i16 xpValue)> onSpawnExperienceOrb;

    // 玩家列表事件
    std::function<void(const std::vector<skin::PlayerListEntry>& entries)> onPlayerListAdd;
    std::function<void(const std::vector<std::array<u8, 16>>& uuids)> onPlayerListRemove;
    std::function<void(const skin::PlayerListEntry& entry)> onPlayerListUpdateGameMode;
    std::function<void(const std::array<u8, 16>& uuid, i32 ping)> onPlayerListUpdateLatency;
    std::function<void(const std::array<u8, 16>& uuid, const std::optional<std::string>& displayName)>
        onPlayerListUpdateDisplayName;

    // 粒子事件
    std::function<void(::mc::particle::ParticleTypeId type,
        f64 x,
        f64 y,
        f64 z,
        f32 vx,
        f32 vy,
        f32 vz,
        f32 ox,
        f32 oy,
        f32 oz,
        u32 count)>
        onParticle;

    // 振动粒子回调（携带目标位置来源和到达时间）
    // targetKind: 0=方块位置（targetX/Y/Z 为方块中心），1=实体位置（targetEntityId + yOffset，需客户端解析实体位置）
    // 当 targetKind==1 且客户端无法解析实体位置时，targetX/Y/Z 回退为粒子起始位置
    std::function<void(f64 x,
        f64 y,
        f64 z,
        u8 targetKind,
        f64 targetX,
        f64 targetY,
        f64 targetZ,
        EntityId targetEntityId,
        f32 yOffset,
        i32 arrivalInTicks)>
        onVibrationParticle;

    // 轨迹粒子回调（携带目标位置、颜色和持续时间）
    std::function<void(f64 x, f64 y, f64 z, f64 targetX, f64 targetY, f64 targetZ, u32 color, i32 durationInTicks)>
        onTrailParticle;

    // 灰尘粒子回调（携带 ARGB 颜色和缩放）
    std::function<void(particle::ParticleTypeId type,
        f64 x,
        f64 y,
        f64 z,
        f32 vx,
        f32 vy,
        f32 vz,
        f32 ox,
        f32 oy,
        f32 oz,
        u32 count,
        u32 color,
        f32 scale)>
        onDustParticle;

    // 颜色过渡灰尘粒子回调（携带起始颜色、目标颜色和缩放）
    std::function<void(f64 x,
        f64 y,
        f64 z,
        f32 vx,
        f32 vy,
        f32 vz,
        f32 ox,
        f32 oy,
        f32 oz,
        u32 count,
        u32 fromColor,
        u32 toColor,
        f32 scale)>
        onDustColorTransitionParticle;

    // 实体效果粒子回调（携带 ARGB 颜色）
    std::function<void(f64 x, f64 y, f64 z, f32 vx, f32 vy, f32 vz, f32 ox, f32 oy, f32 oz, u32 count, u32 color)>
        onEntityEffectParticle;

    // 方块粒子回调（携带方块状态 ID）
    // 用于 Block/Breaking/FallingDust/BlockMarker/BlockCrumble/DustPillar 等粒子，
    // 客户端通过 BlockRegistry::instance().getBlockState(stateId) 解析回 BlockState。
    std::function<void(particle::ParticleTypeId type,
        f64 x,
        f64 y,
        f64 z,
        f32 vx,
        f32 vy,
        f32 vz,
        f32 ox,
        f32 oy,
        f32 oz,
        u32 count,
        u32 blockStateId)>
        onBlockParticle;

    // 物品粒子回调（携带物品堆）
    // 用于 Item/ItemSlime/ItemCobweb/ItemSnowball 等粒子，
    // 客户端通过 ItemStack 解析物品纹理（方块物品走 BlockModelCache，普通物品走 ItemTextureAtlas）。
    std::function<void(particle::ParticleTypeId type,
        f64 x,
        f64 y,
        f64 z,
        f32 vx,
        f32 vy,
        f32 vz,
        f32 ox,
        f32 oy,
        f32 oz,
        u32 count,
        const ItemStack& itemStack)>
        onItemParticle;

    // 世界事件（音效/粒子效果）
    std::function<void(i32 eventId, i32 x, i32 y, i32 z, i32 data)> onWorldEvent;

    // 重生/维度切换事件
    std::function<void(i32 dimensionType,
        DimensionId dimension,
        u64 hashedSeed,
        GameMode gameMode,
        GameMode previousGameMode,
        bool isDebug,
        bool isFlat,
        bool keepData,
        std::optional<GlobalPos> lastDeathLocation)>
        onRespawn;

    // 维度信息事件
    std::function<void(const std::vector<std::tuple<DimensionId, std::string, bool, bool, f32>>& dimensions)>
        onDimensionInfo;

    // 世界出生点事件（包含偏航角，用于指南针指向）
    std::function<void(i32 x, i32 y, i32 z, f32 angle)> onSpawnPosition;

    // 地图数据更新事件
    std::function<void(const network::MapDataPacket& packet)> onMapData;

    // 载具移动同步事件
    std::function<void(f64 x, f64 y, f64 z, f32 yaw, f32 pitch)> onVehicleMove;

    // 睡眠状态事件
    std::function<void(u32 entityId, bool isSleeping, i32 bedX, i32 bedY, i32 bedZ)> onSleep;

    // 快捷栏设置事件
    std::function<void(i32 slot)> onHotbarSet;

    // 标题显示事件
    std::function<void(
        network::TitleAction action, const std::optional<std::string>& text, i32 fadeIn, i32 stay, i32 fadeOut)>
        onTitle;
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
    [[nodiscard]] Result<void> connectLocal(network::LocalEndpoint* endpoint, const NetworkClientConfig& config = {});
    void disconnect(const std::string& reason = "Client disconnect");
    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] ClientState state() const;
    [[nodiscard]] bool isLocalConnection() const { return m_localEndpoint != nullptr; }

    // 配置
    void setCallbacks(const NetworkClientCallbacks& callbacks);
    [[nodiscard]] const NetworkClientConfig& config() const { return m_config; }

    // 发送数据包
    void sendLoginRequest();
    void sendPlayerMove(const network::PlayerPosition& pos, network::PlayerMovePacket::MoveType type);
    void sendBlockInteraction(network::BlockInteractionAction action, i32 x, i32 y, i32 z, Direction face);
    void sendBlockPlacement(i32 x, i32 y, i32 z, Direction face, f32 hitX, f32 hitY, f32 hitZ, u8 hand = 0);
    void sendHotbarSelect(i32 slot);
    void sendTeleportConfirm(u32 teleportId);
    void sendConfirmDimensionChange(DimensionId dimension);
    void sendKeepAlive(u64 id);
    void sendChatMessage(const std::string& message);
    void sendCreativeInventoryAction(const CreativeInventoryActionPacket& packet);
    void sendContainerClick(const ContainerClickPacket& packet);
    void sendCloseContainer(ContainerId containerId);

    /**
     * @brief 发送告示牌文本更新包
     *
     * 客户端在告示牌编辑器关闭时调用，将编辑后的4行文本发送给服务端。
     *
     * @param pos 告示牌方块位置
     * @param lines 4行文本
     * @param isFrontSide 是否编辑正面
     */
    void sendUpdateSign(const BlockPos& pos,
        const std::array<std::string, network::UpdateSignPacket::LINE_COUNT>& lines,
        bool isFrontSide);

    // 骑乘相关数据包
    /**
     * @brief 发送玩家输入包（骑乘时使用）
     * @param strafeSpeed 左右移动速度（正值=左，负值=右）
     * @param forwardSpeed 前后移动速度（正值=前，负值=后）
     * @param jumping 是否跳跃
     * @param sneaking 是否潜行
     */
    void sendPlayerInput(f32 strafeSpeed, f32 forwardSpeed, bool jumping, bool sneaking);

    /**
     * @brief 发送载具移动包（骑乘时客户端同步载具位置）
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param yaw 偏航角
     * @param pitch 俯仰角
     */
    void sendMoveVehicle(f64 x, f64 y, f64 z, f32 yaw, f32 pitch);

    /**
     * @brief 发送实体动作包
     * @param action 动作类型
     * @param auxData 辅助数据（如马跳跃蓄力）
     */
    void sendEntityAction(network::EntityActionType action, i32 auxData = 0);

    /**
     * @brief 发送船划桨状态包
     * @param leftPaddle 左桨是否在划动
     * @param rightPaddle 右桨是否在划动
     */
    void sendSteerBoat(bool leftPaddle, bool rightPaddle);

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
    [[nodiscard]] const std::string& username() const { return m_username; }
    [[nodiscard]] bool isLoggedIn() const { return m_state == ClientState::Playing; }

private:
    // 内部方法
    void _receiveLoop();
    void _processIncomingData();
    void _processPacket(const u8* data, size_t size);
    void _sendRawData(const u8* data, size_t size);
    void _sendPacket(const std::vector<u8>& packetData);
    void _setState(ClientState state);
    void _handleKeepAlive(u64 id);
    void _handleLoginResponse(network::PacketDeserializer& deser);
    void _handleCommandTree(const u8* data, size_t size);
    void _handleTeleport(network::PacketDeserializer& deser);
    void _handleChunkData(network::PacketDeserializer& deser);
    void _handleUnloadChunk(network::PacketDeserializer& deser);
    void _handlePlayerSpawn(network::PacketDeserializer& deser);
    void _handlePlayerDespawn(network::PacketDeserializer& deser);
    void _handleBlockUpdate(network::PacketDeserializer& deser);
    void _handleChatMessage(network::PacketDeserializer& deser);
    void _handleTimeUpdate(network::PacketDeserializer& deser);
    void _handlePlayerInventory(network::PacketDeserializer& deser);
    void _handleOpenContainer(network::PacketDeserializer& deser);
    void _handleContainerContent(network::PacketDeserializer& deser);
    void _handleContainerSlot(network::PacketDeserializer& deser);
    void _handleCloseContainer(network::PacketDeserializer& deser);
    void _handleSignEditorOpen(network::PacketDeserializer& deser);
    void _handleBlockEntityData(network::PacketDeserializer& deser);
    void _handleDisconnect(network::PacketDeserializer& deser);

    // 实体包处理
    void _handleSpawnEntity(network::PacketDeserializer& deser);
    void _handleSpawnMob(network::PacketDeserializer& deser);
    void _handleEntityDestroy(network::PacketDeserializer& deser);
    void _handleEntityMove(network::PacketDeserializer& deser);
    void _handleEntityTeleport(network::PacketDeserializer& deser);
    void _handleEntityVelocity(network::PacketDeserializer& deser);
    void _handleEntityMetadata(network::PacketDeserializer& deser);
    void _handleEntityAnimation(network::PacketDeserializer& deser);
    void _handleEntityHeadLook(network::PacketDeserializer& deser);
    void _handleEntityStatus(network::PacketDeserializer& deser);
    void _handleCollectItem(network::PacketDeserializer& deser);

    // 爆炸包处理
    void _handleExplosion(network::PacketDeserializer& deser);

    // 天气包处理
    void _handleGameStateChange(network::PacketDeserializer& deser);

    // 玩家能力包处理
    void _handlePlayerAbilities(network::PacketDeserializer& deser);

    // 难度同步包处理
    void _handleServerDifficulty(network::PacketDeserializer& deser);

    // 光照更新包处理
    void _handleLightUpdate(network::PacketDeserializer& deser);

    // 方块破坏动画包处理
    void _handleBlockBreakAnim(network::PacketDeserializer& deser);

    // 方块事件包处理
    void _handleBlockEvent(network::PacketDeserializer& deser);

    // 声音包处理
    void _handlePlaySound(network::PacketDeserializer& deser);
    void _handleStopSound(network::PacketDeserializer& deser);
    void _handlePlaySoundEffect(network::PacketDeserializer& deser);
    void _handleMovingSound(network::PacketDeserializer& deser);

    // 世界事件处理
    void _handleWorldEvent(network::PacketDeserializer& deser);

    // 经验包处理
    void _handleSetExperience(network::PacketDeserializer& deser);
    void _handleSpawnExperienceOrb(network::PacketDeserializer& deser);

    // 玩家列表包处理
    void _handlePlayerListItem(network::PacketDeserializer& deser);

    // 粒子包处理
    void _handleParticle(network::PacketDeserializer& deser);

    // 乘客包处理
    void _handleSetPassengers(network::PacketDeserializer& deser);

    // 旁观者摄像机包处理
    void _handleSetCamera(network::PacketDeserializer& deser);

    // 重生/维度切换包处理
    void _handleRespawn(network::PacketDeserializer& deser);

    // 维度信息包处理
    void _handleDimensionInfo(network::PacketDeserializer& deser);

    // 世界出生点包处理
    void _handleSpawnPosition(network::PacketDeserializer& deser);

    // 地图数据包处理
    void _handleMapData(network::PacketDeserializer& deser);

    // 载具移动同步包处理
    void _handleVehicleMove(network::PacketDeserializer& deser);

    // 睡眠状态包处理
    void _handleSleep(network::PacketDeserializer& deser);

    // 快捷栏设置包处理
    void _handleHotbarSet(network::PacketDeserializer& deser);

    // 标题显示包处理
    void _handleTitle(network::PacketDeserializer& deser);

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
    std::string m_username;

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
