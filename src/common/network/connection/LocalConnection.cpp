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

#include "LocalConnection.hpp"
#include <cstring>

namespace mc::network {

void LocalEndpoint::send(const u8* data, size_t size)
{
    if (!m_remote || !m_connected) {
        return;
    }

    std::vector<u8> packet(data, data + size);

    {
        std::lock_guard<std::mutex> lock(m_remote->m_mutex);
        m_remote->m_queue.push(std::move(packet));
    }
    m_remote->m_cv.notify_one();
}

bool LocalEndpoint::receive(std::vector<u8>& outData)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty()) {
        return false;
    }

    outData = std::move(m_queue.front());
    m_queue.pop();
    return true;
}

bool LocalEndpoint::receiveWait(std::vector<u8>& outData, u32 timeoutMs)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (timeoutMs == 0) {
        m_cv.wait(lock, [this] { return !m_queue.empty() || !m_connected; });
    } else {
        if (!m_cv.wait_for(
                lock, std::chrono::milliseconds(timeoutMs), [this] { return !m_queue.empty() || !m_connected; })) {
            return false;
        }
    }

    if (m_queue.empty()) {
        return false;
    }

    outData = std::move(m_queue.front());
    m_queue.pop();
    return true;
}

bool LocalEndpoint::hasData() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_queue.empty();
}

size_t LocalEndpoint::pendingCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

void LocalEndpoint::connectTo(LocalEndpoint* remote)
{
    m_remote = remote;
    m_connected = true;
}

void LocalEndpoint::disconnect()
{
    m_connected = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_queue.empty()) {
            m_queue.pop();
        }
    }
    m_cv.notify_all();
}

bool LocalEndpoint::isConnected() const
{
    return m_connected;
}

void LocalConnectionPair::connect()
{
    m_clientEndpoint.connectTo(&m_serverEndpoint);
    m_serverEndpoint.connectTo(&m_clientEndpoint);
}

void LocalConnectionPair::disconnect()
{
    m_clientEndpoint.disconnect();
    m_serverEndpoint.disconnect();
}

} // namespace mc::network
