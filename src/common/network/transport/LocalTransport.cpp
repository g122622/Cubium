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

#include "common/network/transport/LocalTransport.hpp"

namespace mc::network::transport {

Result<void> LocalTransport::send(ir::IrPacket packet)
{
    if (!isConnected()) {
        return Error(ErrorCode::InvalidState, "LocalTransport disconnected", "LocalTransport::send");
    }
    if (m_peer == nullptr) {
        return Error(ErrorCode::InvalidState, "LocalTransport has no paired peer", "LocalTransport::send");
    }
    // 投递到对端的 inbox，等对端 pump 时回调
    {
        std::lock_guard<std::mutex> lock(m_peer->m_inboxMutex);
        m_peer->m_inbox.push(std::move(packet));
    }
    return Result<void>::ok();
}

void LocalTransport::onPacket(PacketCallback callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_packetCallback = std::move(callback);
}

void LocalTransport::onDisconnect(DisconnectCallback callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_disconnectCallback = std::move(callback);
}

bool LocalTransport::isConnected() const noexcept
{
    return m_connected;
}

void LocalTransport::close()
{
    if (!m_connected.exchange(false)) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_disconnectCallback) {
        m_disconnectCallback();
        m_disconnectCallback = nullptr;
    }
}

void LocalTransport::pump()
{
    PacketCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        callback = m_packetCallback;
    }
    if (!callback) {
        return;
    }

    // 取出当前积压包，释放锁后再回调（避免回调中再 send 导致死锁）
    std::queue<ir::IrPacket> drained;
    {
        std::lock_guard<std::mutex> lock(m_inboxMutex);
        drained.swap(m_inbox);
    }
    while (!drained.empty()) {
        callback(std::move(drained.front()));
        drained.pop();
    }
}

LocalTransportPair LocalTransportPair::create()
{
    LocalTransportPair pair;
    pair.client = std::make_unique<LocalTransport>();
    pair.server = std::make_unique<LocalTransport>();
    pair.client->setPeer(pair.server.get());
    pair.server->setPeer(pair.client.get());
    return pair;
}

} // namespace mc::network::transport
