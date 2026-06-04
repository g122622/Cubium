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

namespace mc::network {

/**
 * @brief 命令树同步包
 *
 * 服务端在登录完成后发送给客户端，携带当前可用命令树的 JSON 快照。
 * 该包仅序列化包体，外层网络头由 ConnectionManager::encapsulatePacket 统一添加。
 */
class CommandTreePacket : public Packet {
public:
    CommandTreePacket()
        : Packet(PacketType::CommandTree)
    {}

    explicit CommandTreePacket(std::string treeJson)
        : Packet(PacketType::CommandTree)
        , m_treeJson(std::move(treeJson))
    {}

    // 移动操作
    CommandTreePacket(CommandTreePacket&&) noexcept = default;
    CommandTreePacket& operator=(CommandTreePacket&&) noexcept = default;

    // 禁止拷贝（数据包通常不需要拷贝）
    CommandTreePacket(const CommandTreePacket&) = delete;
    CommandTreePacket& operator=(const CommandTreePacket&) = delete;

    /**
     * @brief 获取命令树 JSON 包体
     */
    [[nodiscard]] const std::string& treeJson() const noexcept { return m_treeJson; }

    /**
     * @brief 设置命令树 JSON 包体
     */
    void setTreeJson(std::string treeJson) noexcept { m_treeJson = std::move(treeJson); }

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

private:
    std::string m_treeJson;
};

} // namespace mc::network
