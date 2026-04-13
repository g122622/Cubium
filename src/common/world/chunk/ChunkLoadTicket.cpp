#include "common/world/chunk/ChunkLoadTicket.hpp"

namespace mc::world {

// 预定义票据类型实例
const ChunkLoadTicketType<ChunkPos> TicketTypes::PLAYER =
    ChunkLoadTicketType<ChunkPos>::create("player");

const ChunkLoadTicketType<ChunkPos> TicketTypes::FORCED =
    ChunkLoadTicketType<ChunkPos>::create("forced");

const ChunkLoadTicketType<ChunkPos> TicketTypes::PORTAL =
    ChunkLoadTicketType<ChunkPos>::create("portal", 300);

const ChunkLoadTicketType<u32> TicketTypes::POST_TELEPORT =
    ChunkLoadTicketType<u32>::create("post_teleport", 5);

const ChunkLoadTicketType<ChunkPos> TicketTypes::UNKNOWN =
    ChunkLoadTicketType<ChunkPos>::create("unknown");

const ChunkLoadTicketType<Unit> TicketTypes::START =
    ChunkLoadTicketType<Unit>::create("start");

const ChunkLoadTicketType<Unit> TicketTypes::DRAGON =
    ChunkLoadTicketType<Unit>::create("dragon");

const ChunkLoadTicketType<ChunkPos> TicketTypes::LIGHT =
    ChunkLoadTicketType<ChunkPos>::create("light");

void TicketTypes::initializeTicketTypes() {
    // 票据类型已在静态初始化时创建，此函数保留用于未来扩展
}

// ChunkTicketSet 实现
void ChunkTicketSet::addTicket(ChunkLoadTicket ticket) {
    for (const auto& t : m_tickets) {
        if (t == ticket) {
            return;
        }
    }
    m_tickets.push_back(std::move(ticket));
}

bool ChunkTicketSet::removeTicket(const ChunkLoadTicket& ticket) {
    for (auto it = m_tickets.begin(); it != m_tickets.end(); ++it) {
        if (*it == ticket) {
            m_tickets.erase(it);
            return true;
        }
    }
    return false;
}

i32 ChunkTicketSet::getMinLevel() const {
    if (m_tickets.empty()) {
        return static_cast<i32>(ChunkLoadLevel::MaxLevel);
    }

    i32 minLevel = static_cast<i32>(ChunkLoadLevel::MaxLevel);
    for (const auto& ticket : m_tickets) {
        if (ticket.level() < minLevel) {
            minLevel = ticket.level();
        }
    }
    return minLevel;
}

void ChunkTicketSet::removeExpired(u64 currentTime) {
    auto it = m_tickets.begin();
    while (it != m_tickets.end()) {
        if (it->isExpired(currentTime)) {
            it = m_tickets.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace mc::world
