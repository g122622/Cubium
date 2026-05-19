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

#include "LocalServerConnection.hpp"
#include <spdlog/spdlog.h>

namespace mc::network {

LocalServerConnection::LocalServerConnection(LocalEndpoint* endpoint)
    : m_endpoint(endpoint)
    , m_id(++s_nextId)
{}

void LocalServerConnection::send(const u8* data, size_t size)
{
    if (m_endpoint && m_endpoint->isConnected()) {
        m_endpoint->send(data, size);
    }
}

void LocalServerConnection::disconnect(const std::string& reason)
{
    if (m_endpoint) {
        m_endpoint->disconnect();
        if (!reason.empty()) {
            spdlog::debug("LocalServerConnection {} disconnected: {}", m_id, reason);
        }
    }
}

bool LocalServerConnection::isConnected() const
{
    return m_endpoint && m_endpoint->isConnected();
}

std::string LocalServerConnection::identifier() const
{
    return "Local:" + std::to_string(m_id);
}

ConnectionType LocalServerConnection::type() const
{
    return ConnectionType::Local;
}

std::string LocalServerConnection::getAddress() const
{
    // 本地连接没有 IP 地址
    return "";
}

} // namespace mc::network
