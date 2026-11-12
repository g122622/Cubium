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
#include "common/util/text/ITextComponent.hpp"
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
     * @brief 发送成功反馈
     *
     * 反馈内容是文本组件而非字符串：面向玩家的文本必须是翻译组件（translatable），
     * 由客户端按自己的语言表本地化；控制台出口才把组件字符串化。
     *
     * @param message 反馈组件
     * @param broadcastToAdmins 是否同时广播给其他管理员（原版 `sendSuccess` 的 silent 语义取反）
     */
    virtual void sendSuccess(const text::ITextComponent& message, bool broadcastToAdmins) = 0;

    /**
     * @brief 发送错误反馈（仅发给命令源自己）
     * @param message 错误组件
     */
    virtual void sendFailure(const text::ITextComponent& message) = 0;

    /**
     * @brief 发送成功反馈（组件所有权转移的便捷重载）
     *
     * 允许调用点直接写 `sendSuccess(text::literal("..."), false)`，无需手动解引用。
     *
     * @param message 反馈组件（所有权转移）
     * @param broadcastToAdmins 是否同时广播给其他管理员
     */
    void sendSuccess(std::unique_ptr<text::ITextComponent> message, bool broadcastToAdmins)
    {
        sendSuccess(*message, broadcastToAdmins);
    }

    /**
     * @brief 发送错误反馈（组件所有权转移的便捷重载）
     * @param message 错误组件（所有权转移）
     */
    void sendFailure(std::unique_ptr<text::ITextComponent> message)
    {
        sendFailure(*message);
    }

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
    void sendSuccess(const text::ITextComponent&, bool) override {}

    void sendFailure(const text::ITextComponent&) override {}

    bool shouldReceiveFeedback() const noexcept override
    {
        return false;
    }

    bool shouldReceiveErrors() const noexcept override
    {
        return false;
    }

    bool allowLogging() const noexcept override
    {
        return false;
    }

    static SilentCommandSource& instance() noexcept
    {
        static SilentCommandSource s_instance;
        return s_instance;
    }
};

} // namespace command
} // namespace mc
