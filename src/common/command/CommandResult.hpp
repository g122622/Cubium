#pragma once

#include "common/core/Types.hpp"
#include <string>
#include <optional>

namespace mc::command {

/**
 * @brief 命令执行结果
 *
 * 包含命令执行的返回值和状态信息
 */
class CommandResult {
public:
    /**
     * @brief 创建成功结果
     * @param result 命令返回值（通常表示影响的实体数量）
     */
    static CommandResult success(i32 result = 1) {
        return CommandResult(true, result, {});
    }

    /**
     * @brief 创建失败结果
     */
    static CommandResult failure() {
        return CommandResult(false, 0, {});
    }

    /**
     * @brief 创建带错误消息的失败结果
     */
    static CommandResult failure(const std::string& error) {
        return CommandResult(false, 0, error);
    }

    [[nodiscard]] bool isSuccess() const noexcept { return m_success; }
    [[nodiscard]] bool isFailure() const noexcept { return !m_success; }
    [[nodiscard]] i32 result() const noexcept { return m_result; }
    [[nodiscard]] const std::optional<std::string>& error() const noexcept { return m_error; }

    // 转换为 bool（成功为 true）
    explicit operator bool() const noexcept { return m_success; }

private:
    CommandResult(bool success, i32 result, std::optional<std::string> error)
        : m_success(success)
        , m_result(result)
        , m_error(std::move(error)) {}

    bool m_success;
    i32 m_result;
    std::optional<std::string> m_error;
};

/**
 * @brief 命令执行消费者
 *
 * 用于在命令执行完成后接收回调
 */
template<typename S>
class ResultConsumer {
public:
    virtual ~ResultConsumer() = default;

    /**
     * @brief 命令完成时调用
     * @param success 是否成功
     * @param result 返回值
     */
    virtual void onCommandComplete(bool success, i32 result) = 0;
};

} // namespace mc::command
