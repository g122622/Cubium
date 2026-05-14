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
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mc::server {

// TCP服务器配置
struct TcpServerConfig {
    u16 port = 25565;         // 监听端口
    u32 maxConnections = 100; // 最大连接数
    u32 backlog = 10;         // 连接队列长度
    bool noDelay = true;      // TCP_NODELAY选项
};

// TCP服务器
class TcpServer {
public:
    TcpServer();
    ~TcpServer();

    // 禁止拷贝
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    // 生命周期
    [[nodiscard]] Result<void> start(const TcpServerConfig& config);
    void stop();
    bool isRunning() const { return m_running; }

    // 会话管理
    std::shared_ptr<TcpSession> getSession(SessionId id);
    size_t getSessionCount() const;
    void broadcast(const u8* data, size_t size);
    void broadcastPacket(const network::Packet& packet);
    void broadcastExcept(SessionId excludeId, const u8* data, size_t size);

    // 回调设置
    void setOnConnect(ConnectCallback callback) { m_onConnect = std::move(callback); }
    void setOnDisconnect(DisconnectCallback callback) { m_onDisconnect = std::move(callback); }
    void setOnPacket(PacketCallback callback) { m_onPacket = std::move(callback); }

    // 服务器信息
    u16 port() const { return m_config.port; }

    // 处理待处理的连接和事件 (需要在主循环中调用)
    void poll();

private:
    TcpServerConfig m_config;
    bool m_running = false;
    SessionId m_nextSessionId = 1;

    // 会话映射
    std::unordered_map<SessionId, std::shared_ptr<TcpSession>> m_sessions;
    mutable std::mutex m_sessionsMutex;

    // 回调
    ConnectCallback m_onConnect;
    DisconnectCallback m_onDisconnect;
    PacketCallback m_onPacket;

    // 平台相关数据
#ifdef _WIN32
    uintptr_t m_listenSocket = ~0ull; // INVALID_SOCKET
#else
    int m_listenSocket = -1;
#endif

    // 内部方法
    bool createListenSocket();
    void closeListenSocket();
    void acceptNewConnection();
    void removeSession(SessionId id);
    void handleSessionData(TcpSession* session);
    void sendSessionData(TcpSession* session);
};

} // namespace mc::server
