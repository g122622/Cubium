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
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

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

std::optional<PointOfInterest> PointOfInterest::deserialize(const nbt::tags::compound_tag& tag)
{
    const auto* xTag = tag.findAs<nbt::tags::int_tag>("X");
    const auto* yTag = tag.findAs<nbt::tags::int_tag>("Y");
    const auto* zTag = tag.findAs<nbt::tags::int_tag>("Z");
    const auto* typeTag = tag.findAs<nbt::tags::int_tag>("Type");
    if (xTag == nullptr || yTag == nullptr || zTag == nullptr || typeTag == nullptr) {
        spdlog::warn("PointOfInterest::deserialize: missing position or type field, POI skipped");
        return std::nullopt;
    }

    const i32 rawType = typeTag->value;
    if (rawType < 0 || rawType > static_cast<i32>(PointOfInterestType::LightningRod)) {
        spdlog::warn("PointOfInterest::deserialize: out-of-range POI type {}, POI skipped", rawType);
        return std::nullopt;
    }

    PointOfInterest poi(BlockPos{xTag->value, yTag->value, zTag->value}, static_cast<PointOfInterestType>(rawType));

    // MaxTickets 缺失时沿用构造函数按类型推导出的上限
    if (const auto* maxTicketsTag = tag.findAs<nbt::tags::int_tag>("MaxTickets"); maxTicketsTag != nullptr) {
        poi.m_maxTickets = maxTicketsTag->value;
    }

    // 反序列化票据
    auto it = tag.value.find("Tickets");
    if (it != tag.value.end()) {
        auto* ticketsList = dynamic_cast<const nbt::tags::compound_list_tag*>(it->second.get());
        if (ticketsList) {
            for (const auto& ticketTag : ticketsList->value) {
                const auto* ownerTag = ticketTag.findAs<nbt::tags::long_tag>("OwnerId");
                const auto* createdTag = ticketTag.findAs<nbt::tags::long_tag>("CreatedAt");
                if (ownerTag == nullptr || createdTag == nullptr) {
                    spdlog::warn("PointOfInterest::deserialize: malformed ticket entry, entry skipped");
                    continue;
                }

                POITicket ticket;
                ticket.ownerId = static_cast<u64>(ownerTag->value);
                ticket.createdAt = createdTag->value;

                const auto* releasedTag = ticketTag.findAs<nbt::tags::byte_tag>("Released");
                ticket.released = releasedTag != nullptr && releasedTag->value != 0;

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
