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

#include "TcpSession.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include <memory>

namespace mc::server {

/**
 * @brief TCP 连接适配器
 *
 * 将 TcpSession 适配到 IServerConnection 接口。
 * 用于远程客户端连接。
 *
 * 使用示例：
 * @code
 * auto conn = std::make_shared<TcpConnection>(session);
 * world.addPlayer(playerId, username, conn);
 * @endcode
 */
class TcpConnection : public network::IServerConnection {
public:
    /**
     * @brief 构造 TCP 连接适配器
     * @param session TCP 会话共享指针
     */
    explicit TcpConnection(std::shared_ptr<TcpSession> session);

    ~TcpConnection() override = default;

    // 禁止拷贝
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    // ========== IServerConnection 接口实现 ==========

    void send(const u8* data, size_t size) override;
    void disconnect(const std::string& reason = "") override;
    [[nodiscard]] bool isConnected() const override;
    [[nodiscard]] std::string identifier() const override;
    [[nodiscard]] network::ConnectionType type() const override;
    [[nodiscard]] std::string getAddress() const override;

    // ========== TCP 特有方法 ==========

    /**
     * @brief 获取底层 TCP 会话
     * @return TCP 会话指针
     */
    [[nodiscard]] std::shared_ptr<TcpSession> session() const { return m_session; }

    /**
     * @brief 获取会话 ID
     * @return 会话 ID
     */
    [[nodiscard]] SessionId sessionId() const;

private:
    std::shared_ptr<TcpSession> m_session;
};

} // namespace mc::server
