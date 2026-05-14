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

#include "TcpConnection.hpp"
#include <spdlog/spdlog.h>

namespace mc::server {

TcpConnection::TcpConnection(std::shared_ptr<TcpSession> session)
    : m_session(std::move(session))
{}

void TcpConnection::send(const u8* data, size_t size)
{
    if (m_session) {
        m_session->send(data, size);
    }
}

void TcpConnection::disconnect(const std::string& reason)
{
    if (m_session) {
        m_session->disconnect(reason);
    }
}

bool TcpConnection::isConnected() const
{
    return m_session && m_session->state() == SessionState::Playing;
}

std::string TcpConnection::identifier() const
{
    if (m_session) {
        return "TCP:" + m_session->address() + ":" + std::to_string(m_session->port());
    }
    return "TCP:disconnected";
}

network::ConnectionType TcpConnection::type() const
{
    return network::ConnectionType::Tcp;
}

std::string TcpConnection::getAddress() const
{
    return m_session ? m_session->address() : "";
}

SessionId TcpConnection::sessionId() const
{
    return m_session ? m_session->id() : 0;
}

} // namespace mc::server
