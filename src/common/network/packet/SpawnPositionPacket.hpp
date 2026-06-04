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
#include "common/world/block/BlockPos.hpp"
#include <vector>

namespace mc::network {

/**
 * @brief 世界出生点包 (服务端 -> 客户端)
 *
 * 告诉客户端世界出生点的位置。客户端使用此位置来确定指南针指向。
 *
 * 发送时机：
 * - 玩家登录时
 * - 执行 /setworldspawn 命令后
 *
 * 协议格式: BlockPos(x, y, z) + angle(f32)
 */
class SpawnPositionPacket : public Packet {
public:
    SpawnPositionPacket();
    explicit SpawnPositionPacket(const BlockPos& pos, f32 angle = 0.0f);
    ~SpawnPositionPacket() override = default;

    // ========== Packet 接口实现 ==========

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========== Getter/Setter ==========

    /**
     * @brief 获取出生点位置
     */
    [[nodiscard]] const BlockPos& position() const { return m_position; }
    void setPosition(const BlockPos& pos) { m_position = pos; }

    /**
     * @brief 获取出生点偏航角
     *
     * 用于指南针指向计算。
     */
    [[nodiscard]] f32 angle() const { return m_angle; }
    void setAngle(f32 angle) { m_angle = angle; }

private:
    BlockPos m_position;
    f32 m_angle = 0.0f; // 出生点偏航角，用于指南针
};

} // namespace mc::network
