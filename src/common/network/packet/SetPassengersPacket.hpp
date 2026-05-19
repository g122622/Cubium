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

#pragma once

#include "../../core/Types.hpp"
#include "Packet.hpp"
#include <vector>

namespace mc::network {

/**
 * @brief 设置乘客列表包
 *
 * 服务端向客户端发送实体的乘客列表。
 * 当实体骑乘/离开载具时发送。
 *
 * 参考 MC 1.16.5 SSetPassengersPacket
 */
class SetPassengersPacket : public Packet {
public:
    SetPassengersPacket();
    explicit SetPassengersPacket(u32 entityId, const std::vector<u32>& passengerIds);

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    [[nodiscard]] u32 entityId() const { return m_entityId; }
    [[nodiscard]] const std::vector<u32>& passengerIds() const { return m_passengerIds; }

    void setEntityId(u32 entityId) { m_entityId = entityId; }
    void setPassengerIds(const std::vector<u32>& ids) { m_passengerIds = ids; }
    void addPassengerId(u32 id) { m_passengerIds.push_back(id); }

private:
    u32 m_entityId = 0;
    std::vector<u32> m_passengerIds;
};

} // namespace mc::network
