#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "ServerCommandSource.hpp"
#include <memory>
#include <future>
#include <functional>
#include <vector>
#include <unordered_set>

namespace mc {
namespace command {

/**
 * @brief 命令注册表
 *
 * 管理所有命令的注册和分发。
 * 提供命令注册的统一入口点。
 *
 * 参考 MC 的 CommandDispatcher 和 Commands 类
 */
class CommandRegistry {
public:
    using Dispatcher = CommandDispatcher<ServerCommandSource>;

    CommandRegistry();
    ~CommandRegistry() = default;

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
    [[nodiscard]] Result<i32> execute(const String& input, ServerCommandSource& source);

    /**
     * @brief 获取命令建议
     * @param input 命令输入
     * @param source 命令源
     * @return 异步建议结果
     */
    [[nodiscard]] std::future<Suggestions> getSuggestions(const String& input, ServerCommandSource& source);

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
    [[nodiscard]] std::vector<String> getCommandNames() const;

    /**
     * @brief 检查命令是否存在
     */
    [[nodiscard]] bool hasCommand(const String& name) const;

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
