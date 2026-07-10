#pragma once

#include "common/item/loot/LootPredicateManager.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/data/DataAccessor.hpp"
#include "server/core/BannedIpList.hpp"
#include "server/core/BannedPlayerList.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/GameModeManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/PacketHandler.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/core/WhitelistManager.hpp"
#include "server/function/FunctionManager.hpp"
#include "server/function/TimerQueue.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"

#include <stdexcept>
#include <vector>

namespace mc::test {

class FakeServerConnection : public network::IServerConnection {
public:
    void send(const u8* data, size_t size) override;
    void disconnect(const std::string& reason = "") override;
    [[nodiscard]] bool isConnected() const override { return m_connected; }
    [[nodiscard]] std::string identifier() const override { return "FakeConnection"; }
    [[nodiscard]] network::ConnectionType type() const override { return network::ConnectionType::Local; }
    [[nodiscard]] std::string getAddress() const override { return ""; }
    [[nodiscard]] size_t sentBytes() const noexcept { return m_sentData.size(); }
    [[nodiscard]] const std::string& disconnectReason() const noexcept { return m_disconnectReason; }

private:
    bool m_connected = true;
    std::string m_disconnectReason;
    std::vector<u8> m_sentData;
};

class BaseTestServer : public server::IServer {
public:
    BaseTestServer();
    ~BaseTestServer() override = default;

    [[nodiscard]] Result<void> initialize() override { return Result<void>::ok(); }
    void shutdown() override { m_running = false; }
    void tick() override {}
    [[nodiscard]] bool isRunning() const override { return m_running; }

    [[nodiscard]] bool isIntegrated() const noexcept override { return false; }
    [[nodiscard]] bool isDedicated() const noexcept override { return true; }

    [[nodiscard]] server::core::PlayerManager& playerManager() override { return m_playerManager; }
    [[nodiscard]] const server::core::PlayerManager& playerManager() const override { return m_playerManager; }
    [[nodiscard]] server::core::ConnectionManager& connectionManager() override { return m_connectionManager; }
    [[nodiscard]] const server::core::ConnectionManager& connectionManager() const override
    {
        return m_connectionManager;
    }
    [[nodiscard]] server::core::TimeManager& timeManager() override { return m_timeManager; }
    [[nodiscard]] const server::core::TimeManager& timeManager() const override { return m_timeManager; }
    [[nodiscard]] server::core::TeleportManager& teleportManager() override { return m_teleportManager; }
    [[nodiscard]] const server::core::TeleportManager& teleportManager() const override { return m_teleportManager; }
    [[nodiscard]] server::core::KeepAliveManager& keepAliveManager() override { return m_keepAliveManager; }
    [[nodiscard]] const server::core::KeepAliveManager& keepAliveManager() const override { return m_keepAliveManager; }
    [[nodiscard]] server::core::PositionTracker& positionTracker() override { return m_positionTracker; }
    [[nodiscard]] const server::core::PositionTracker& positionTracker() const override { return m_positionTracker; }
    [[nodiscard]] server::core::PacketHandler& packetHandler() override { return m_packetHandler; }
    [[nodiscard]] const server::core::PacketHandler& packetHandler() const override { return m_packetHandler; }
    [[nodiscard]] server::core::GameModeManager& gameModeManager() override { return m_gameModeManager; }
    [[nodiscard]] const server::core::GameModeManager& gameModeManager() const override { return m_gameModeManager; }
    [[nodiscard]] server::core::WhitelistManager& whitelistManager() override { return m_whitelistManager; }
    [[nodiscard]] const server::core::WhitelistManager& whitelistManager() const override { return m_whitelistManager; }
    [[nodiscard]] server::core::BannedPlayerList& bannedPlayerList() override { return m_bannedPlayerList; }
    [[nodiscard]] const server::core::BannedPlayerList& bannedPlayerList() const override { return m_bannedPlayerList; }
    [[nodiscard]] server::core::BannedIpList& bannedIpList() override { return m_bannedIpList; }
    [[nodiscard]] const server::core::BannedIpList& bannedIpList() const override { return m_bannedIpList; }
    [[nodiscard]] server::core::OpListManager& opListManager() override { return m_opListManager; }
    [[nodiscard]] const server::core::OpListManager& opListManager() const override { return m_opListManager; }

    [[nodiscard]] ServerDimensionManager& dimensionManager() override;
    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override;
    [[nodiscard]] server::ServerWorld* getPlayerWorld(PlayerId playerId) override;
    [[nodiscard]] server::ServerPlayerEntityManager& playerEntityManager() override;
    [[nodiscard]] const server::ServerPlayerEntityManager& playerEntityManager() const override;
    [[nodiscard]] server::interaction::BlockInteractionManager& blockInteractionManager() override;
    [[nodiscard]] const server::interaction::BlockInteractionManager& blockInteractionManager() const override;
    [[nodiscard]] server::interaction::MiningManager& miningManager() override;
    [[nodiscard]] const server::interaction::MiningManager& miningManager() const override;
    [[nodiscard]] server::interaction::ContainerManager& containerManager() override;
    [[nodiscard]] const server::interaction::ContainerManager& containerManager() const override;
    [[nodiscard]] server::interaction::InventoryManager& inventoryManager() override { return m_inventoryManager; }
    [[nodiscard]] const server::interaction::InventoryManager& inventoryManager() const override
    {
        return m_inventoryManager;
    }
    [[nodiscard]] PlayerInventory* playerInventory(PlayerId playerId) override
    {
        return m_inventoryManager.getInventory(playerId);
    }
    [[nodiscard]] const PlayerInventory* playerInventory(PlayerId playerId) const override
    {
        return m_inventoryManager.getInventory(playerId);
    }
    [[nodiscard]] command::CommandRegistry& commandRegistry() override { return m_commandRegistry; }
    [[nodiscard]] const command::CommandRegistry& commandRegistry() const override { return m_commandRegistry; }
    [[nodiscard]] command::CommandStorage& commandStorage() override { return m_commandStorage; }
    [[nodiscard]] const command::CommandStorage& commandStorage() const override { return m_commandStorage; }
    [[nodiscard]] resource::DataPackRepository& dataPackList() override;
    [[nodiscard]] const resource::DataPackRepository& dataPackList() const override;
    [[nodiscard]] loot::LootTableManager& lootTableManager() override;
    [[nodiscard]] const loot::LootTableManager& lootTableManager() const override;
    [[nodiscard]] loot::LootPredicateManager& predicateManager() override { return m_predicateManager; }
    [[nodiscard]] const loot::LootPredicateManager& predicateManager() const override { return m_predicateManager; }
    [[nodiscard]] function::FunctionManager& functionManager() override { return m_functionManager; }
    [[nodiscard]] const function::FunctionManager& functionManager() const override { return m_functionManager; }
    [[nodiscard]] function::TimerQueue& functionTimerQueue() override { return m_functionTimerQueue; }
    [[nodiscard]] const function::TimerQueue& functionTimerQueue() const override { return m_functionTimerQueue; }
    [[nodiscard]] world::storage::SingleLevelStorageManager* sharedStorage() override { return nullptr; }
    [[nodiscard]] const world::storage::SingleLevelStorageManager* sharedStorage() const override { return nullptr; }
    [[nodiscard]] bool isSharedStorageReadonlyForeignWorld() const override { return false; }
    [[nodiscard]] server::ServerScoreboard& scoreboard() override { return m_scoreboard; }
    [[nodiscard]] const server::ServerScoreboard& scoreboard() const override { return m_scoreboard; }
    [[nodiscard]] server::CustomServerBossInfoManager& bossBarManager() override;
    [[nodiscard]] const server::CustomServerBossInfoManager& bossBarManager() const override;

    [[nodiscard]] i32 viewDistance() const override { return m_viewDistance; }
    [[nodiscard]] i32 maxPlayers() const override { return m_maxPlayers; }
    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] u64 currentTick() const override { return m_timeManager.currentTick(); }
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty difficulty) override { m_difficulty = difficulty; }
    [[nodiscard]] GameMode defaultGameMode() const override { return m_defaultGameMode; }
    void setDefaultGameMode(GameMode mode) override { m_defaultGameMode = mode; }
    [[nodiscard]] i32 playerIdleTimeoutMinutes() const override { return m_idleTimeoutMinutes; }
    void setPlayerIdleTimeoutMinutes(i32 timeoutMinutes) override { m_idleTimeoutMinutes = timeoutMinutes; }
    void broadcastServerMessage(std::string_view message) override { m_lastBroadcastMessage = std::string(message); }
    void requestStop() override
    {
        m_stopRequested = true;
        m_running = false;
    }
    [[nodiscard]] Result<void> publishToLan(i32 port, bool allowCheats) override
    {
        (void)port;
        (void)allowCheats;
        return Error(ErrorCode::Unsupported, "Test server does not support LAN publishing");
    }
    void broadcastParticleInRange(u32, f64, f64, f64, f32, f32, f32, f32, f32, f32, u32, f32) override {}
    void sendCommandTreePacket(PlayerId) override {}
    void sendPermissionLevelChange(PlayerId, i32) override {}
    void sendSoundToPlayer(PlayerId playerId,
        const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override;

    [[nodiscard]] server::ServerPlayerData* addTestPlayer(PlayerId playerId, const std::string& username);
    void setPlayerWorld(server::ServerWorld* world) { m_playerWorld = world; }
    [[nodiscard]] const std::string& lastBroadcastMessage() const noexcept { return m_lastBroadcastMessage; }
    [[nodiscard]] bool stopRequested() const noexcept { return m_stopRequested; }

protected:
    [[noreturn]] static void throwUnused();
    [[nodiscard]] std::shared_ptr<FakeServerConnection> lastConnection() const
    {
        return m_connections.empty() ? nullptr : m_connections.back();
    }

    i32 m_viewDistance = 10;
    i32 m_maxPlayers = 20;
    u64 m_seed = 0;
    bool m_running = true;
    Difficulty m_difficulty = Difficulty::Normal;
    GameMode m_defaultGameMode = GameMode::Survival;
    i32 m_idleTimeoutMinutes = 0;
    bool m_stopRequested = false;
    std::string m_lastBroadcastMessage;

    bool m_soundSent = false;
    PlayerId m_lastSoundPlayerId = 0;
    ResourceLocation m_lastSoundEvent{""};
    sound::SoundCategory m_lastSoundCategory = sound::SoundCategory::Master;
    Vector3 m_lastSoundPosition{0, 0, 0};
    f32 m_lastSoundVolume = 1.0f;
    f32 m_lastSoundPitch = 1.0f;

    server::ServerWorld* m_playerWorld = nullptr;
    server::core::PlayerManager m_playerManager;
    server::interaction::InventoryManager m_inventoryManager;
    server::core::ConnectionManager m_connectionManager;
    server::core::TimeManager m_timeManager;
    server::core::TeleportManager m_teleportManager;
    server::core::KeepAliveManager m_keepAliveManager;
    server::core::PositionTracker m_positionTracker;
    server::core::PacketHandler m_packetHandler;
    server::core::GameModeManager m_gameModeManager;
    server::core::WhitelistManager m_whitelistManager;
    server::core::BannedPlayerList m_bannedPlayerList;
    server::core::BannedIpList m_bannedIpList;
    server::core::OpListManager m_opListManager;
    command::CommandRegistry m_commandRegistry;
    command::CommandStorage m_commandStorage;
    server::ServerScoreboard m_scoreboard;
    loot::LootPredicateManager m_predicateManager;
    function::FunctionManager m_functionManager;
    function::TimerQueue m_functionTimerQueue;
    std::vector<std::shared_ptr<FakeServerConnection>> m_connections;
};

} // namespace mc::test
