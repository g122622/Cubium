#pragma once

#include "MinecraftServer.hpp"
#include "server/settings/ServerSettings.hpp"
#include "server/network/TcpServer.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <thread>
#include <atomic>
#include <filesystem>
#include <unordered_map>

namespace mc::server {

/**
 * @brief 独立服务器启动参数
 *
 * 用于命令行覆盖设置文件中的配置。
 */
struct StandaloneServerParams {
    std::optional<u16> port;
    std::optional<String> bindAddress;
    std::optional<u32> maxPlayers;
    std::optional<String> worldName;
    std::optional<i64> seed;
    std::optional<String> settingsPath;
};

/**
 * @brief 独立服务器（多人模式）
 *
 * 继承 MinecraftServer，提供：
 * - TCP 网络通信
 * - 多玩家支持
 * - 完整的数据包处理
 */
class StandaloneServer : public MinecraftServer {
public:
    StandaloneServer();
    ~StandaloneServer() override;

    // 禁止拷贝
    StandaloneServer(const StandaloneServer&) = delete;
    StandaloneServer& operator=(const StandaloneServer&) = delete;

    // ========== IServer 接口实现 ==========

    [[nodiscard]] Result<void> initialize() override;
    void shutdown() override;

protected:
    void pollNetwork() override;
    void broadcastPacket(const u8* data, size_t size) override;

    [[nodiscard]] PlayerId getPlayerIdForSession(u32 sessionId) const override {
        return m_playerManager->getPlayerIdBySession(sessionId);
    }

    void sendPacketToPlayer(PlayerId playerId, const u8* data, size_t size) override {
        auto* player = m_playerManager->getPlayer(playerId);
        if (player && player->hasConnection()) {
            player->send(data, size);
        }
    }

    // ========== 数据包处理（特有逻辑） ==========

    void handleLoginRequestPacket(u32 sessionId, const u8* data, size_t size) override;
    void handleBlockPlacementPacket(PlayerId playerId, const u8* data, size_t size) override;
    void handleHotbarSelectPacket(PlayerId playerId, const u8* data, size_t size) override;
    void handleCreativeInventoryActionPacket(PlayerId playerId, const u8* data, size_t size) override;
    void handleContainerClickPacket(PlayerId playerId, const u8* data, size_t size) override;
    void handleCloseContainerPacket(PlayerId playerId, const u8* data, size_t size) override;

protected:
    void broadcastLightUpdate(ChunkCoord x, ChunkCoord z, i32 sectionY,
                              const std::vector<u8>& skyLight,
                              const std::vector<u8>& blockLight,
                              bool trustEdges) override;

public:

    // ========== StandaloneServer 特有接口 ==========

    /**
     * @brief 使用参数初始化
     */
    [[nodiscard]] Result<void> initialize(const StandaloneServerParams& params);

    /**
     * @brief 运行主循环
     */
    [[nodiscard]] Result<void> run();

    /**
     * @brief 停止服务器
     */
    void stop();

    /**
     * @brief 获取设置
     */
    [[nodiscard]] ServerSettings& settings() noexcept { return m_settings; }
    [[nodiscard]] const ServerSettings& settings() const noexcept { return m_settings; }

    // ========== 玩家实体管理 ==========

    [[nodiscard]] ServerPlayerEntityManager& playerEntityManager() override { return m_playerEntityManager; }
    [[nodiscard]] const ServerPlayerEntityManager& playerEntityManager() const override { return m_playerEntityManager; }

private:
    void mainLoop();

    // 加载设置
    [[nodiscard]] Result<void> loadSettings(const String& path);
    void applySettings();

    // 网络事件处理
    void onClientConnect(TcpSession* session);
    void onClientDisconnect(TcpSession* session, const String& reason);

    // 回调设置
    void setupChunkSendCallback();

    // 数据包发送
    void sendLoginResponse(TcpSession* session, bool success, PlayerId playerId, EntityId entityId,
                          const String& username, const String& message);

    ServerSettings m_settings;
    // 当前会话生效的设置文件路径（加载/自动保存/退出保存统一使用）
    std::filesystem::path m_settingsPath;
    std::unique_ptr<TcpServer> m_tcpServer;

    // 玩家实体管理器
    ServerPlayerEntityManager m_playerEntityManager;

    // PlayerId -> EntityId 映射（用于快速查找）
    std::unordered_map<PlayerId, EntityId> m_playerEntityIds;
};

} // namespace mc::server
