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
 * @brief 方块破坏动画数据包
 *
 * 服务端向客户端广播方块的挖掘进度，用于多人游戏中显示其他玩家的挖掘动画。
 *
 * 协议格式：
 * - VarInt: breakerEntityId (挖掘者实体ID)
 * - Position: position (方块位置，x/y/z各i32)
 * - Byte: stage (破坏阶段 0-9，-1表示移除)
 */
class BlockBreakAnimPacket : public Packet {
public:
    BlockBreakAnimPacket();

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    // ========================================================================
    // Getter
    // ========================================================================

    /**
     * @brief 获取挖掘者实体ID
     */
    [[nodiscard]] EntityInstanceId breakerEntityId() const { return m_breakerEntityId; }

    /**
     * @brief 获取方块位置
     */
    [[nodiscard]] const BlockPos& position() const { return m_position; }

    /**
     * @brief 获取破坏阶段
     *
     * @return 0-9 表示破坏阶段，-1 表示移除破坏效果
     */
    [[nodiscard]] i8 stage() const { return m_stage; }

    // ========================================================================
    // Setter
    // ========================================================================

    /**
     * @brief 设置挖掘者实体ID
     */
    void setBreakerEntityId(EntityInstanceId id) { m_breakerEntityId = id; }

    /**
     * @brief 设置方块位置
     */
    void setPosition(const BlockPos& pos) { m_position = pos; }

    /**
     * @brief 设置破坏阶段
     *
     * @param stage 0-9 表示破坏阶段，负数表示移除
     */
    void setStage(i8 stage) { m_stage = stage; }

    /**
     * @brief 设置为移除模式
     */
    void setRemove() { m_stage = -1; }

    // ========================================================================
    // 工厂方法
    // ========================================================================

    /**
     * @brief 创建更新破坏进度的包
     */
    [[nodiscard]] static BlockBreakAnimPacket createUpdate(EntityInstanceId breakerId, const BlockPos& pos, u8 stage)
    {
        BlockBreakAnimPacket packet;
        packet.setBreakerEntityId(breakerId);
        packet.setPosition(pos);
        packet.setStage(static_cast<i8>(stage));
        return packet;
    }

    /**
     * @brief 创建移除破坏效果的包
     */
    [[nodiscard]] static BlockBreakAnimPacket createRemove(EntityInstanceId breakerId, const BlockPos& pos)
    {
        BlockBreakAnimPacket packet;
        packet.setBreakerEntityId(breakerId);
        packet.setPosition(pos);
        packet.setRemove();
        return packet;
    }

    /**
     * @brief 获取预期大小
     */
    size_t expectedSize() const override;

private:
    EntityInstanceId m_breakerEntityId = 0;
    BlockPos m_position;
    i8 m_stage = 0;
};

} // namespace network
} // namespace mc
