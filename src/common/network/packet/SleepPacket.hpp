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

#include "../../world/block/BlockPos.hpp"
#include "Packet.hpp"
#include "PacketSerializer.hpp"
#include <optional>

namespace mc::network {

/**
 * @brief 睡眠状态同步包 (S->C)
 *
 * 服务器发送给客户端，同步玩家的睡眠状态。
 * 当玩家进入睡眠时，发送带有床位置的包。
 * 当玩家离开睡眠时，发送不带床位置的包。
 *
 * 参考 MC 1.16.5 SUseBedPacket
 */
class SleepPacket : public Packet {
public:
    SleepPacket()
        : Packet(PacketType::Sleep)
    {}

    /**
     * @brief 构造进入睡眠的包
     * @param entityId 实体ID
     * @param bedPos 床头位置
     */
    SleepPacket(u32 entityId, const BlockPos& bedPos);

    /**
     * @brief 构造离开睡眠的包
     * @param entityId 实体ID
     */
    static SleepPacket createWakeUp(u32 entityId);

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    // Getters
    [[nodiscard]] u32 entityId() const { return m_entityId; }
    [[nodiscard]] const std::optional<BlockPos>& bedPosition() const { return m_bedPos; }
    [[nodiscard]] bool isSleeping() const { return m_bedPos.has_value(); }

    // Setters
    void setEntityId(u32 id) { m_entityId = id; }
    void setBedPosition(const BlockPos& pos) { m_bedPos = pos; }
    void clearBedPosition() { m_bedPos = std::nullopt; }

private:
    u32 m_entityId = 0;
    std::optional<BlockPos> m_bedPos;
};

} // namespace mc::network
