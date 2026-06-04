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

/**
 * @brief TCP服务器配置
 */
struct TcpServerConfig {
    u16 port = 25565;         ///< 监听端口
    u32 maxConnections = 100; ///< 最大连接数
    u32 backlog = 10;         ///< 连接队列长度
    bool noDelay = true;      ///< TCP_NODELAY选项
};

/**
 * @brief TCP服务器
 *
 * 负责接受客户端连接、管理会话、处理数据收发。
 * 使用非阻塞I/O实现单线程事件循环模式。
 */
class TcpServer {
public:
    TcpServer();
    ~TcpServer();

    // 禁止拷贝
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    // 禁止移动（因为包含平台相关的socket句柄）
    TcpServer(TcpServer&&) = delete;
    TcpServer& operator=(TcpServer&&) = delete;

    /**
     * @brief 启动服务器
     * @param config 服务器配置
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> start(const TcpServerConfig& config);

    /**
     * @brief 停止服务器
     */
    void stop();

    /**
     * @brief 检查服务器是否正在运行
     */
    bool isRunning() const noexcept { return m_running; }

    /**
     * @brief 获取指定会话
     * @param id 会话ID
     * @return 会话指针，不存在则返回nullptr
     */
    std::shared_ptr<TcpSession> getSession(SessionId id);

    /**
     * @brief 获取当前会话数量
     */
    size_t getSessionCount() const;

    /**
     * @brief 向所有会话广播数据
     * @param data 数据指针
     * @param size 数据大小
     */
    void broadcast(const u8* data, size_t size);

    /**
     * @brief 向所有会话广播数据包
     * @param packet 数据包
     */
    void broadcastPacket(const network::Packet& packet);

    /**
     * @brief 向除指定会话外的所有会话广播数据
     * @param excludeId 排除的会话ID
     * @param data 数据指针
     * @param size 数据大小
     */
    void broadcastExcept(SessionId excludeId, const u8* data, size_t size);

    /**
     * @brief 设置连接回调
     * @param callback 连接回调函数
     */
    void setOnConnect(ConnectCallback callback) { m_onConnect = std::move(callback); }

    /**
     * @brief 设置断开连接回调
     * @param callback 断开连接回调函数
     */
    void setOnDisconnect(DisconnectCallback callback) { m_onDisconnect = std::move(callback); }

    /**
     * @brief 设置数据包回调
     * @param callback 数据包回调函数
     */
    void setOnPacket(PacketCallback callback) { m_onPacket = std::move(callback); }

    /**
     * @brief 获取监听端口
     */
    u16 port() const noexcept { return m_config.port; }

    /**
     * @brief 处理待处理的连接和事件
     * @note 需要在主循环中周期性调用
     */
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
    bool _createListenSocket();
    void _closeListenSocket();
    void _acceptNewConnection();
    void _removeSession(SessionId id);
    void _handleSessionData(TcpSession* session);
    void _sendSessionData(TcpSession* session);
};

} // namespace mc::server
