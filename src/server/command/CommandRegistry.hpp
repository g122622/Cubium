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

#include "ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandTreeSnapshot.hpp"
#include "common/command/suggestions/Suggestions.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace mc {
namespace command {

/**
 * @brief 命令注册表
 *
 * 管理所有命令的注册和分发。
 * 提供命令注册的统一入口点。
 */
class CommandRegistry {
public:
    using Dispatcher = CommandDispatcher<ServerCommandSource>;

    CommandRegistry();
    ~CommandRegistry() noexcept = default;

    // 禁止复制
    CommandRegistry(const CommandRegistry&) = delete;
    CommandRegistry& operator=(const CommandRegistry&) = delete;

    // ========== 命令分发 ==========

    /**
     * @brief 获取命令分发器
     */
    [[nodiscard]] Dispatcher& dispatcher() noexcept { return m_dispatcher; }
    [[nodiscard]] const Dispatcher& dispatcher() const noexcept { return m_dispatcher; }

    /**
     * @brief 执行命令
     * @param input 命令字符串
     * @param source 命令源
     * @return 执行结果
     */
    [[nodiscard]] Result<i32> execute(const std::string& input, ServerCommandSource& source);

    /**
     * @brief 获取命令建议
     * @param input 命令输入
     * @param source 命令源
     * @return 异步建议结果
     */
    [[nodiscard]] std::future<Suggestions> getSuggestions(const std::string& input, ServerCommandSource& source);

    /**
     * @brief 获取命令树快照
     */
    [[nodiscard]] CommandTreeSnapshot getCommandTreeSnapshot() const;

    /**
     * @brief 获取命令树 JSON
     */
    [[nodiscard]] std::string getCommandTreeJson() const;

    // ========== 命令注册 ==========

    /**
     * @brief 注册所有默认命令
     *
     * 注册以下命令：
     * - /gamemode - 设置游戏模式
     * - /tp - 传送
     * - /give - 给予物品
     * - /time - 时间控制
     * - /kill - 杀死实体
     * - /clear - 清空背包
     * - /seed - 显示种子
     * - /list - 列出玩家
     * - /help - 帮助信息
     */
    void registerDefaults();

    // ========== 命令查询 ==========

    /**
     * @brief 获取所有命令名称
     */
    [[nodiscard]] std::vector<std::string> getCommandNames() const;

    /**
     * @brief 检查命令是否存在
     */
    [[nodiscard]] bool hasCommand(const std::string& name) const noexcept;

    /**
     * @brief 获取全局命令注册表实例
     *
     * 线程安全的单例模式，首次调用时自动注册默认命令。
     */
    [[nodiscard]] static CommandRegistry& getGlobal();

private:
    Dispatcher m_dispatcher;
    bool m_defaultsRegistered = false;
};

} // namespace command
} // namespace mc
