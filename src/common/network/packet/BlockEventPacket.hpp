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
#include "common/world/block/BlockPos.hpp"

namespace mc {
namespace network {

/**
 * @brief 方块事件包
 *
 * 服务端广播方块事件给附近客户端，用于同步方块动画和状态变化。
 * 客户端收到后调用 Block::triggerEvent() 处理事件。
 *
 * 协议格式：
 * - Position: x/y/z 各 i32（共12字节）
 * - Byte: paramA（事件参数A，无符号字节）
 * - Byte: paramB（事件参数B，无符号字节）
 * - VarInt: blockStateId（方块状态ID，用于标识方块类型）
 *
 * 参考 MC Java: ClientboundBlockEventPacket
 */
class BlockEventPacket : public Packet {
public:
    BlockEventPacket();

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    // Getters
    [[nodiscard]] const BlockPos& position() const { return m_position; }
    [[nodiscard]] u8 paramA() const { return m_paramA; }
    [[nodiscard]] u8 paramB() const { return m_paramB; }
    [[nodiscard]] u32 blockStateId() const { return m_blockStateId; }

    // Setters
    void setPosition(const BlockPos& pos) { m_position = pos; }
    void setParamA(u8 paramA) { m_paramA = paramA; }
    void setParamB(u8 paramB) { m_paramB = paramB; }
    void setBlockStateId(u32 blockStateId) { m_blockStateId = blockStateId; }

    /**
     * @brief 创建方块事件包
     *
     * @param pos 方块位置
     * @param paramA 事件参数A
     * @param paramB 事件参数B
     * @param blockStateId 方块状态ID
     */
    [[nodiscard]] static BlockEventPacket create(const BlockPos& pos, u8 paramA, u8 paramB, u32 blockStateId)
    {
        BlockEventPacket packet;
        packet.setPosition(pos);
        packet.setParamA(paramA);
        packet.setParamB(paramB);
        packet.setBlockStateId(blockStateId);
        return packet;
    }

    size_t expectedSize() const override;

private:
    BlockPos m_position;
    u8 m_paramA = 0;
    u8 m_paramB = 0;
    u32 m_blockStateId = 0;
};

} // namespace network
} // namespace mc
