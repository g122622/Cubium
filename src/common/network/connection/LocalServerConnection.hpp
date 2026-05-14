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
    [[nodiscard]] std::string getAddress() const override;

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
