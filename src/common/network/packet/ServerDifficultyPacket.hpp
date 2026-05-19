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

namespace mc::network {

/**
 * @brief 难度同步数据包
 *
 * 服务端向客户端同步世界难度和锁定状态。
 * 在以下情况发送：
 * - 玩家登录时
 * - 难度变更时
 * - 难度锁定状态变更时
 *
 * 参考 MC 1.16.5 SServerDifficultyPacket
 *
 * 协议格式:
 * | 字段           | 类型 | 说明                    |
 * |----------------|------|-------------------------|
 * | difficulty     | u8   | 难度等级 (0-3)          |
 * | locked         | bool | 难度是否锁定            |
 */
class ServerDifficultyPacket : public Packet {
public:
    ServerDifficultyPacket();

    /**
     * @brief 构造难度同步包
     *
     * @param difficulty 世界难度
     * @param locked 难度是否锁定（锁定后无法更改）
     */
    ServerDifficultyPacket(Difficulty difficulty, bool locked);

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========== Getters ==========

    /**
     * @brief 获取难度
     */
    [[nodiscard]] Difficulty difficulty() const { return m_difficulty; }

    /**
     * @brief 难度是否锁定
     */
    [[nodiscard]] bool locked() const { return m_locked; }

    // ========== Setters ==========

    /**
     * @brief 设置难度
     */
    void setDifficulty(Difficulty difficulty) { m_difficulty = difficulty; }

    /**
     * @brief 设置锁定状态
     */
    void setLocked(bool locked) { m_locked = locked; }

private:
    Difficulty m_difficulty = Difficulty::Normal;
    bool m_locked = false;
};

} // namespace mc::network
