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
 * @brief 设置摄像机实体包（S2C）
 *
 * 服务端向客户端发送此包以设置玩家的摄像机实体。
 * 当玩家进入旁观者模式并使用 /spectate 命令跟踪目标实体时，
 * 服务端发送此包通知客户端切换渲染视角到目标实体。
 *
 * 当 cameraEntityId 为玩家自身的实体ID时，表示恢复正常视角。
 */
class SetCameraPacket : public Packet {
public:
    SetCameraPacket();
    explicit SetCameraPacket(u32 cameraEntityId);

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    /**
     * @brief 获取摄像机实体的ID
     * @return 实体ID
     */
    [[nodiscard]] u32 cameraEntityId() const { return m_cameraEntityId; }

    /**
     * @brief 设置摄像机实体的ID
     * @param id 实体ID
     */
    void setCameraEntityId(u32 id) { m_cameraEntityId = id; }

private:
    u32 m_cameraEntityId = 0;
};

} // namespace mc::network
