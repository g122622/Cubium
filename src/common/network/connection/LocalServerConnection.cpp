#include "LocalServerConnection.hpp"
#include <spdlog/spdlog.h>

namespace mc::network {

LocalServerConnection::LocalServerConnection(LocalEndpoint* endpoint)
    : m_endpoint(endpoint)
    , m_id(++s_nextId)
{
}

void LocalServerConnection::send(const u8* data, size_t size) {
    if (m_endpoint && m_endpoint->isConnected()) {
        m_endpoint->send(data, size);
    }
}

void LocalServerConnection::disconnect(const std::string& reason) {
    if (m_endpoint) {
        m_endpoint->disconnect();
        if (!reason.empty()) {
            spdlog::debug("LocalServerConnection {} disconnected: {}", m_id, reason);
        }
    }
}

bool LocalServerConnection::isConnected() const {
    return m_endpoint && m_endpoint->isConnected();
}

std::string LocalServerConnection::identifier() const {
    return "Local:" + std::to_string(m_id);
}

ConnectionType LocalServerConnection::type() const {
    return ConnectionType::Local;
}

std::string LocalServerConnection::getAddress() const {
    // 本地连接没有 IP 地址
    return "";
}

} // namespace mc::network
