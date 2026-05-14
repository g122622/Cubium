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
#include "common/network/packet/Packet.hpp"
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace mc::server {

class TcpServer;

#ifdef _WIN32
using NativeSocket = uintptr_t;
#else
using NativeSocket = int;
#endif

// 客户端会话ID
using SessionId = u64;

// 会话状态
enum class SessionState {
    Connecting,     // 连接中
    Connected,      // 已连接
    Authenticating, // 认证中
    Playing,        // 游戏中
    Disconnecting,  // 断开中
    Disconnected    // 已断开
};

// 会话统计
struct SessionStats {
    u64 bytesReceived = 0;
    u64 bytesSent = 0;
    u64 packetsReceived = 0;
    u64 packetsSent = 0;
    u64 lastActivityTime = 0;
};

// 前向声明
class TcpSession;

// 回调类型
using PacketCallback = std::function<void(TcpSession*, const u8*, size_t)>;
using ConnectCallback = std::function<void(TcpSession*)>;
using DisconnectCallback = std::function<void(TcpSession*, const std::string&)>;

// TCP会话类
class TcpSession : public std::enable_shared_from_this<TcpSession> {
public:
    TcpSession(SessionId id, TcpServer* server);
    ~TcpSession();

    SessionId id() const { return m_id; }
    SessionState state() const { return m_state; }
    const SessionStats& stats() const { return m_stats; }
    const std::string& address() const { return m_address; }
    u16 port() const { return m_port; }

    // 发送数据
    void send(const u8* data, size_t size);
    void sendPacket(const network::Packet& packet);

    // 断开连接
    void disconnect(const std::string& reason = "");

    // 内部使用 (由TcpServer调用)
    void setAddress(const std::string& address, u16 port)
    {
        m_address = address;
        m_port = port;
    }
    void setState(SessionState state) { m_state = state; }
    void setOnPacketCallback(PacketCallback callback) { m_onPacket = std::move(callback); }
    void setOnDisconnectCallback(DisconnectCallback callback) { m_onDisconnect = std::move(callback); }
    void setSocket(NativeSocket socket) { m_socket = socket; }
    [[nodiscard]] NativeSocket socket() const { return m_socket; }

    // 处理接收到的数据
    void handleReceivedData(const u8* data, size_t size);

    // 发送队列提取
    [[nodiscard]] std::vector<u8> takeNextSendBuffer();

private:
    SessionId m_id;
    SessionState m_state = SessionState::Connecting;
    TcpServer* m_server;
    std::string m_address;
    u16 m_port = 0;
    NativeSocket m_socket = static_cast<NativeSocket>(-1);

    SessionStats m_stats;

    // 接收缓冲区
    std::vector<u8> m_receiveBuffer;
    size_t m_expectedSize = 0; // 期望的完整包大小

    // 发送队列
    std::deque<std::vector<u8>> m_sendQueue;
    std::mutex m_sendMutex;

    // 回调
    PacketCallback m_onPacket;
    DisconnectCallback m_onDisconnect;

    // 处理完整数据包
    void processPacket(const u8* data, size_t size);
};

} // namespace mc::server
