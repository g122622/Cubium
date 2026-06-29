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

#include "Packet.hpp"
#include "common/core/Types.hpp"

namespace mc::network {

/**
 * @brief 设置实体拴绳链接包
 *
 * 服务端向客户端同步实体拴绳绑定状态。
 * 当实体被拴绳拴住或解拴时发送此包。
 *
 * 协议格式：
 * - VarInt: 被拴实体ID（源实体）
 * - VarInt: 拴绳目标实体ID（0 表示解除拴绳）
 */
class SetEntityLinkPacket : public Packet {
public:
    SetEntityLinkPacket();
    SetEntityLinkPacket(u32 entityId, u32 linkedEntityId);

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    [[nodiscard]] u32 entityId() const { return m_entityId; }
    [[nodiscard]] u32 linkedEntityId() const { return m_linkedEntityId; }

    void setEntityId(u32 entityId) { m_entityId = entityId; }
    void setLinkedEntityId(u32 linkedEntityId) { m_linkedEntityId = linkedEntityId; }

private:
    u32 m_entityId = 0;
    u32 m_linkedEntityId = 0;
};

} // namespace mc::network
