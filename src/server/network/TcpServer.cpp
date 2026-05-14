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

#include "TcpServer.hpp"
#include <array>
#include <spdlog/spdlog.h>
#ifndef _WIN32
#include <cerrno>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSE_SOCKET closesocket
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#define SOCKET_ERROR_VALUE SOCKET_ERROR
#define SOCKET_TYPE SOCKET
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#define CLOSE_SOCKET close
#define INVALID_SOCKET_VALUE -1
#define SOCKET_ERROR_VALUE -1
#define SOCKET_TYPE int
#endif

namespace mc::server {

// Winsock初始化辅助类
class WinsockInitializer {
public:
    WinsockInitializer()
    {
#ifdef _WIN32
        WSADATA wsaData;
        m_initialized = (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
        if (!m_initialized) {
            spdlog::error("Failed to initialize Winsock");
        }
#endif
    }

    ~WinsockInitializer()
    {
#ifdef _WIN32
        if (m_initialized) {
            WSACleanup();
        }
#endif
    }

    bool isInitialized() const { return m_initialized; }

private:
    bool m_initialized = false;
};

// 全局Winsock初始化
static WinsockInitializer s_winsock;
#ifdef _WIN32
static bool isWouldBlockError()
{
    const int err = WSAGetLastError();
    return err == WSAEWOULDBLOCK;
}
#else
static bool isWouldBlockError()
{
    return errno == EAGAIN || errno == EWOULDBLOCK;
}
#endif

TcpServer::TcpServer()
{
#ifdef _WIN32
    m_listenSocket = INVALID_SOCKET;
#endif
}

TcpServer::~TcpServer()
{
    stop();
}

Result<void> TcpServer::start(const TcpServerConfig& config)
{
    if (m_running) {
        return Error(ErrorCode::AlreadyExists, "Server already running");
    }

#ifdef _WIN32
    if (!s_winsock.isInitialized()) {
        return Error(ErrorCode::Unknown, "Winsock not initialized");
    }
#endif

    m_config = config;

    // 创建监听socket
    if (!createListenSocket()) {
        return Error(ErrorCode::Unknown, "Failed to create listen socket");
    }

    m_running = true;
    spdlog::info("TCP server started on port {}", m_config.port);
    return Result<void>::ok();
}

void TcpServer::stop()
{
    if (!m_running) {
        return;
    }

    spdlog::info("Stopping TCP server...");
    m_running = false;

    // 断开所有会话
    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        for (auto& [id, session] : m_sessions) {
            session->disconnect("Server shutdown");
        }
        m_sessions.clear();
    }

    closeListenSocket();
    spdlog::info("TCP server stopped");
}

bool TcpServer::createListenSocket()
{
    // 创建socket
    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET_VALUE) {
        spdlog::error("Failed to create socket");
        return false;
    }

    // 设置SO_REUSEADDR选项
    int optval = 1;
    if (setsockopt(static_cast<int>(m_listenSocket),
            SOL_SOCKET,
            SO_REUSEADDR,
            reinterpret_cast<const char*>(&optval),
            sizeof(optval)) == SOCKET_ERROR_VALUE) {
        spdlog::warn("Failed to set SO_REUSEADDR");
    }

    // 设置TCP_NODELAY选项
    if (m_config.noDelay) {
        optval = 1;
        if (setsockopt(static_cast<int>(m_listenSocket),
                IPPROTO_TCP,
                TCP_NODELAY,
                reinterpret_cast<const char*>(&optval),
                sizeof(optval)) == SOCKET_ERROR_VALUE) {
            spdlog::warn("Failed to set TCP_NODELAY");
        }
    }

    // 绑定地址
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(m_config.port);

    if (bind(static_cast<int>(m_listenSocket), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) ==
        SOCKET_ERROR_VALUE) {
        spdlog::error("Failed to bind socket to port {}", m_config.port);
        closeListenSocket();
        return false;
    }

    // 开始监听
    if (listen(static_cast<int>(m_listenSocket), static_cast<int>(m_config.backlog)) == SOCKET_ERROR_VALUE) {
        spdlog::error("Failed to listen on socket");
        closeListenSocket();
        return false;
    }

    // 设置非阻塞模式
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(m_listenSocket, FIONBIO, &mode);
#else
    int flags = fcntl(static_cast<int>(m_listenSocket), F_GETFL, 0);
    fcntl(static_cast<int>(m_listenSocket), F_SETFL, flags | O_NONBLOCK);
#endif

    return true;
}

void TcpServer::closeListenSocket()
{
    if (m_listenSocket != INVALID_SOCKET_VALUE) {
        CLOSE_SOCKET(static_cast<int>(m_listenSocket));
        m_listenSocket = INVALID_SOCKET_VALUE;
    }
}

void TcpServer::poll()
{
    if (!m_running) {
        return;
    }

    // 接受新连接
    acceptNewConnection();

    // 处理现有会话的数据
    std::vector<std::shared_ptr<TcpSession>> sessionsCopy;
    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        for (const auto& [id, session] : m_sessions) {
            sessionsCopy.push_back(session);
        }
    }

    for (auto& session : sessionsCopy) {
        if (session->state() != SessionState::Disconnected) {
            handleSessionData(session.get());
            sendSessionData(session.get());
        }
    }

    // 清理已断开的会话
    std::vector<SessionId> toRemove;
    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        for (const auto& [id, session] : m_sessions) {
            if (session->state() == SessionState::Disconnected) {
                toRemove.push_back(id);
            }
        }
    }
    for (SessionId id : toRemove) {
        removeSession(id);
    }
}

void TcpServer::acceptNewConnection()
{
    sockaddr_in clientAddr{};
    socklen_t clientAddrLen = static_cast<socklen_t>(sizeof(clientAddr));

    SOCKET_TYPE clientSocket =
        accept(static_cast<SOCKET_TYPE>(m_listenSocket), reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);

    if (clientSocket == INVALID_SOCKET_VALUE) {
        // 非阻塞模式下没有新连接是正常的
        return;
    }

    // 检查连接数限制
    if (getSessionCount() >= m_config.maxConnections) {
        spdlog::warn("Connection rejected: max connections reached");
        CLOSE_SOCKET(clientSocket);
        return;
    }

    // 设置非阻塞模式
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);
#else
    int flags = fcntl(clientSocket, F_GETFL, 0);
    fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
#endif

    // 获取客户端地址
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    u16 clientPort = ntohs(clientAddr.sin_port);

    // 创建会话
    auto session = std::make_shared<TcpSession>(m_nextSessionId++, this);
    session->setAddress(std::string(ipStr), clientPort);
    session->setState(SessionState::Connected);

    // 设置回调
    session->setOnPacketCallback(m_onPacket);
    session->setOnDisconnectCallback(m_onDisconnect);

    session->setSocket(clientSocket);

    // 添加到会话映射
    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        m_sessions[session->id()] = session;
    }

    spdlog::info("New connection from {}:{}", ipStr, clientPort);

    if (m_onConnect) {
        m_onConnect(session.get());
    }
}

/**
 * @brief 从 socket 拉取可读数据并交给会话进行分包重组。
 * @param session 会话对象
 * @note 非阻塞模式下，WSAEWOULDBLOCK/EAGAIN 视为正常状态。
 */
void TcpServer::handleSessionData(TcpSession* session)
{
    if (session == nullptr || session->state() == SessionState::Disconnected) {
        return;
    }

    const auto socket = session->socket();
    if (socket == static_cast<decltype(socket)>(-1)) {
        return;
    }

    std::array<u8, 8192> buffer{};
    while (true) {
#ifdef _WIN32
        const int received = recv(
            static_cast<SOCKET>(socket), reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0);
#else
        const int received = recv(socket, buffer.data(), buffer.size(), 0);
#endif
        if (received > 0) {
            session->handleReceivedData(buffer.data(), static_cast<size_t>(received));
            continue;
        }

        if (received == 0) {
            session->disconnect("Remote closed");
            return;
        }

        if (isWouldBlockError()) {
            return;
        }

        spdlog::warn("Session {} recv failed, disconnecting", session->id());
        session->disconnect("Receive error");
        return;
    }
}

/**
 * @brief 将会话发送队列中的数据写入 socket。
 * @param session 会话对象
 * @note 当前实现按“整包发送”处理，若遇到部分发送会将剩余部分回退到队首。
 */
void TcpServer::sendSessionData(TcpSession* session)
{
    if (session == nullptr || session->state() == SessionState::Disconnected) {
        return;
    }

    const auto socket = session->socket();
    if (socket == static_cast<decltype(socket)>(-1)) {
        return;
    }

    while (true) {
        auto payload = session->takeNextSendBuffer();
        if (payload.empty()) {
            return;
        }

        size_t sentTotal = 0;
        while (sentTotal < payload.size()) {
#ifdef _WIN32
            const int sent = send(static_cast<SOCKET>(socket),
                reinterpret_cast<const char*>(payload.data() + sentTotal),
                static_cast<int>(payload.size() - sentTotal),
                0);
#else
            const int sent = send(socket, payload.data() + sentTotal, payload.size() - sentTotal, 0);
#endif
            if (sent > 0) {
                sentTotal += static_cast<size_t>(sent);
                continue;
            }

            if (isWouldBlockError()) {
                std::vector<u8> remain(payload.begin() + static_cast<std::ptrdiff_t>(sentTotal), payload.end());
                session->send(remain.data(), remain.size());
                return;
            }

            spdlog::warn("Session {} send failed, disconnecting", session->id());
            session->disconnect("Send error");
            return;
        }
    }
}

std::shared_ptr<TcpSession> TcpServer::getSession(SessionId id)
{
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    auto it = m_sessions.find(id);
    if (it != m_sessions.end()) {
        return it->second;
    }
    return nullptr;
}

size_t TcpServer::getSessionCount() const
{
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    return m_sessions.size();
}

void TcpServer::removeSession(SessionId id)
{
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    m_sessions.erase(id);
}

void TcpServer::broadcast(const u8* data, size_t size)
{
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    for (auto& [id, session] : m_sessions) {
        if (session->state() == SessionState::Playing || session->state() == SessionState::Connected) {
            session->send(data, size);
        }
    }
}

void TcpServer::broadcastPacket(const network::Packet& packet)
{
    auto result = packet.serialize();
    if (result.success()) {
        broadcast(result.value().data(), result.value().size());
    }
}

void TcpServer::broadcastExcept(SessionId excludeId, const u8* data, size_t size)
{
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    for (auto& [id, session] : m_sessions) {
        if (id != excludeId &&
            (session->state() == SessionState::Playing || session->state() == SessionState::Connected)) {
            session->send(data, size);
        }
    }
}

} // namespace mc::server
