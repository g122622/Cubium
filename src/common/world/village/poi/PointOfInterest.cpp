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

#include "PointOfInterest.hpp"
#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace village {
namespace poi {

PointOfInterest::PointOfInterest(BlockPos pos, PointOfInterestType type)
    : m_position(pos)
    , m_type(type)
    , m_maxTickets(POITypeHelper::getMaxTickets(type))
{}

bool PointOfInterest::canAcquire(u64 ownerId) const
{
    // 检查是否已经占用
    if (isOccupied()) {
        return false;
    }
    // 检查是否已经被同一个实体占用
    return !isOwnedBy(ownerId);
}

bool PointOfInterest::acquire(u64 ownerId, i64 gameTime)
{
    if (!canAcquire(ownerId)) {
        return false;
    }

    POITicket ticket;
    ticket.ownerId = ownerId;
    ticket.createdAt = gameTime;
    ticket.released = false;
    m_tickets.push_back(ticket);
    return true;
}

bool PointOfInterest::release(u64 ownerId)
{
    auto it = std::find_if(m_tickets.begin(), m_tickets.end(), [ownerId](const POITicket& ticket) {
        return ticket.ownerId == ownerId && !ticket.released;
    });

    if (it != m_tickets.end()) {
        it->released = true;
        m_tickets.erase(it);
        return true;
    }
    return false;
}

bool PointOfInterest::isOwnedBy(u64 ownerId) const
{
    return std::any_of(m_tickets.begin(), m_tickets.end(), [ownerId](const POITicket& ticket) {
        return ticket.ownerId == ownerId && !ticket.released;
    });
}

std::vector<u64> PointOfInterest::getOwners() const
{
    std::vector<u64> owners;
    owners.reserve(m_tickets.size());
    for (const auto& ticket : m_tickets) {
        if (!ticket.released) {
            owners.push_back(ticket.ownerId);
        }
    }
    return owners;
}

void PointOfInterest::serialize(nbt::tags::compound_tag& tag) const
{
    tag.put("X", static_cast<std::int32_t>(m_position.x));
    tag.put("Y", static_cast<std::int32_t>(m_position.y));
    tag.put("Z", static_cast<std::int32_t>(m_position.z));
    tag.put("Type", static_cast<std::int32_t>(m_type));
    tag.put("MaxTickets", static_cast<std::int32_t>(m_maxTickets));

    // 序列化票据
    auto ticketsList = std::make_unique<nbt::tags::compound_list_tag>();
    for (const auto& ticket : m_tickets) {
        nbt::tags::compound_tag ticketTag;
        ticketTag.put("OwnerId", static_cast<std::int64_t>(ticket.ownerId));
        ticketTag.put("CreatedAt", static_cast<std::int64_t>(ticket.createdAt));
        ticketTag.put("Released", ticket.released ? static_cast<std::int8_t>(1) : static_cast<std::int8_t>(0));
        ticketsList->value.push_back(std::move(ticketTag));
    }
    tag.value["Tickets"] = std::move(ticketsList);
}

PointOfInterest PointOfInterest::deserialize(const nbt::tags::compound_tag& tag)
{
    BlockPos pos;
    pos.x = tag.get<nbt::tags::int_tag>("X");
    pos.y = tag.get<nbt::tags::int_tag>("Y");
    pos.z = tag.get<nbt::tags::int_tag>("Z");

    PointOfInterest poi(pos, static_cast<PointOfInterestType>(tag.get<nbt::tags::int_tag>("Type")));
    poi.m_maxTickets = tag.get<nbt::tags::int_tag>("MaxTickets");

    // 反序列化票据
    auto it = tag.value.find("Tickets");
    if (it != tag.value.end()) {
        auto* ticketsList = dynamic_cast<const nbt::tags::compound_list_tag*>(it->second.get());
        if (ticketsList) {
            for (const auto& ticketTag : ticketsList->value) {
                POITicket ticket;
                ticket.ownerId = static_cast<u64>(ticketTag.get<nbt::tags::long_tag>("OwnerId"));
                ticket.createdAt = ticketTag.get<nbt::tags::long_tag>("CreatedAt");
                ticket.released = ticketTag.get<nbt::tags::byte_tag>("Released") != 0;
                if (!ticket.released) {
                    poi.m_tickets.push_back(ticket);
                }
            }
        }
    }

    return poi;
}

} // namespace poi
} // namespace village
} // namespace world
} // namespace mc
