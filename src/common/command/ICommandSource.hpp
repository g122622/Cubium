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

#include "common/core/Types.hpp"
#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

// 前向声明
class Entity;
class Player;
class ServerPlayer;
class MinecraftServer;
namespace server {
class ServerWorld;
}

// Uuid 类型定义
using Uuid = std::array<u8, 16>;

/**
 * @brief Uuid 哈希函数，用于 std::unordered_set/std::unordered_map。
 */
struct UuidHash {
    std::size_t operator()(const Uuid& uuid) const noexcept
    {
        std::size_t result = 0;
        for (const auto& byte : uuid) {
            result ^= std::hash<u8>{}(byte) + 0x9e3779b9 + (result << 6) + (result >> 2);
        }
        return result;
    }
};

namespace command {

/**
 * @brief 命令源接口
 *
 * 定义命令执行者的基本能力。
 *
 * 不同实现：
 * - ServerCommandSource: 服务端玩家/控制台
 * - ClientCommandSource: 客户端本地命令
 * - CommandBlockSource: 命令方块
 */
class ICommandSource {
public:
    virtual ~ICommandSource() = default;

    /**
     * @brief 发送消息给命令源
     * @param message 消息内容
     * @param senderUuid 发送者UUID（可选）
     */
    virtual void sendMessage(const std::string& message, const std::optional<Uuid>& senderUuid = std::nullopt) = 0;

    /**
     * @brief 发送错误消息给命令源
     * @param message 错误消息内容
     */
    virtual void sendError(const std::string& message) = 0;

    /**
     * @brief 是否应该接收反馈消息
     */
    virtual bool shouldReceiveFeedback() const = 0;

    /**
     * @brief 是否应该接收错误消息
     */
    virtual bool shouldReceiveErrors() const = 0;

    /**
     * @brief 是否允许日志记录
     */
    virtual bool allowLogging() const = 0;
};

/**
 * @brief 空命令源（静默模式）
 *
 * 忽略所有消息，用于不需要反馈的场景
 */
class SilentCommandSource : public ICommandSource {
public:
    void sendMessage(const std::string&, const std::optional<Uuid>&) override {}
    void sendError(const std::string&) override {}

    bool shouldReceiveFeedback() const noexcept override { return false; }
    bool shouldReceiveErrors() const noexcept override { return false; }
    bool allowLogging() const noexcept override { return false; }

    static SilentCommandSource& instance() noexcept
    {
        static SilentCommandSource s_instance;
        return s_instance;
    }
};

} // namespace command
} // namespace mc
