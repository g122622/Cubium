#pragma once

#include "IServerConnection.hpp"
#include "LocalConnection.hpp"

namespace mc::network {

/**
 * @brief 本地服务端连接适配器
 *
 * 将 LocalEndpoint 适配到 IServerConnection 接口。
 * 用于 IntegratedServer 的进程内通信。
 *
 * 使用示例：
 * @code
 * auto conn = std::make_shared<LocalServerConnection>(serverEndpoint);
 * world.addPlayer(playerId, username, conn);
 * @endcode
 */
class LocalServerConnection : public IServerConnection {
public:
    /**
     * @brief 构造本地连接适配器
     * @param endpoint 服务端本地端点指针（不获取所有权）
     */
    explicit LocalServerConnection(LocalEndpoint* endpoint);

    ~LocalServerConnection() override = default;

    // 禁止拷贝
    LocalServerConnection(const LocalServerConnection&) = delete;
    LocalServerConnection& operator=(const LocalServerConnection&) = delete;

    // ========== IServerConnection 接口实现 ==========

    void send(const u8* data, size_t size) override;
    void disconnect(const std::string& reason = "") override;
    [[nodiscard]] bool isConnected() const override;
    [[nodiscard]] std::string identifier() const override;
    [[nodiscard]] ConnectionType type() const override;

    // ========== 本地连接特有方法 ==========

    /**
     * @brief 获取底层本地端点
     * @return 本地端点指针
     */
    [[nodiscard]] LocalEndpoint* endpoint() const { return m_endpoint; }

private:
    LocalEndpoint* m_endpoint;
    static inline u64 s_nextId = 0;
    u64 m_id;
};

} // namespace mc::network
