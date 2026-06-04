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

#include "TcpSession.hpp"
#include "TcpServer.hpp"
#include "common/core/Constants.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include <spdlog/spdlog.h>

namespace mc::server {

TcpSession::TcpSession(SessionId id, TcpServer* server)
    : m_id(id)
    , m_state(SessionState::Connecting)
    , m_server(server)
{
    m_receiveBuffer.reserve(4096);
}

TcpSession::~TcpSession()
{
    if (m_state != SessionState::Disconnected) {
        _closeLocally();
    }
}

/**
 * @brief 将原始字节数据加入发送队列。
 * @param data 数据指针（调用后会复制）
 * @param size 数据大小
 * @note 仅负责排队，真正发送由 TcpServer::sendSessionData 在网络线程执行。
 */
void TcpSession::send(const u8* data, size_t size)
{
    {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        m_sendQueue.emplace_back(data, data + size);
    }
    m_stats.bytesSent += size;
    m_stats.packetsSent++;
}

void TcpSession::sendPacket(const network::Packet& packet)
{
    auto result = packet.serialize();
    if (result.success()) {
        send(result.value().data(), result.value().size());
    } else {
        spdlog::error("Failed to serialize packet: {}", result.error().toString());
    }
}

void TcpSession::disconnect(const std::string& reason)
{
    if (m_state == SessionState::Disconnected) {
        return;
    }

    m_state = SessionState::Disconnecting;
    spdlog::info("Session {} disconnecting: {}", m_id, reason.empty() ? "No reason" : reason);

    // 发送断开连接包
    network::DisconnectPacket packet;
    packet.setReason(reason);
    sendPacket(packet);

    m_state = SessionState::Disconnected;

    if (m_onDisconnect) {
        m_onDisconnect(this, reason);
    }
}

void TcpSession::_closeLocally()
{
    m_state = SessionState::Disconnected;
    {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        m_sendQueue.clear();
    }
}

/**
 * @brief 处理接收缓冲并重组完整协议包。
 * @param data 新接收的数据块
 * @param size 数据块大小
 * @note 支持非阻塞 socket 的分包/粘包场景。
 */
void TcpSession::handleReceivedData(const u8* data, size_t size)
{
    // 追加到缓冲区
    m_receiveBuffer.insert(m_receiveBuffer.end(), data, data + size);
    m_stats.bytesReceived += size;

    // 尝试解析完整数据包
    while (true) {
        // 检查是否收到足够的数据来读取包头
        if (m_receiveBuffer.size() < network::PACKET_HEADER_SIZE) {
            break;
        }

        // 如果还不知道期望的包大小，从头部读取
        if (m_expectedSize == 0) {
            network::PacketDeserializer deserializer(m_receiveBuffer.data(), network::PACKET_HEADER_SIZE);
            auto sizeResult = deserializer.readU32();
            if (sizeResult.failed()) {
                spdlog::error("Failed to read packet size");
                disconnect("Invalid packet");
                return;
            }
            m_expectedSize = sizeResult.value();

            // 验证包大小（使用 Constants.hpp 中定义的 MAX_PACKET_SIZE = 2MB）
            if (m_expectedSize > mc::network::MAX_PACKET_SIZE) {
                spdlog::error("Packet too large: {} bytes (max: {})", m_expectedSize, mc::network::MAX_PACKET_SIZE);
                disconnect("Packet too large");
                return;
            }
        }

        // 检查是否收到完整的包
        if (m_receiveBuffer.size() < m_expectedSize) {
            break;
        }

        // 处理完整的数据包
        _processPacket(m_receiveBuffer.data(), m_expectedSize);

        // 移除已处理的数据
        m_receiveBuffer.erase(m_receiveBuffer.begin(), m_receiveBuffer.begin() + m_expectedSize);
        m_expectedSize = 0;
        m_stats.packetsReceived++;
    }
}

void TcpSession::_processPacket(const u8* data, size_t size)
{
    if (m_onPacket) {
        m_onPacket(this, data, size);
    }
}

/**
 * @brief 从发送队列取出下一包待发送数据。
 * @return 若队列为空返回空 vector，否则返回并移除队首元素。
 */
std::vector<u8> TcpSession::takeNextSendBuffer()
{
    std::lock_guard<std::mutex> lock(m_sendMutex);
    if (m_sendQueue.empty()) {
        return {};
    }

    auto next = std::move(m_sendQueue.front());
    m_sendQueue.pop_front();
    return next;
}

} // namespace mc::server
