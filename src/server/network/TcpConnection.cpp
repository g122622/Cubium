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
