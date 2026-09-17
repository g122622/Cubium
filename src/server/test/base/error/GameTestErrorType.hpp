#pragma once

#include "common/core/Types.hpp"

namespace mc::test {

/**
 * @brief GameTest 错误类型枚举。
 *
 * 对齐基岩版 `GameTestErrorType`（int 枚举，数值 0-9）。JS 侧 `@minecraft/server-gametest`
 * 暴露为字符串枚举名，由脚本绑定层负责 int↔字符串名映射。
 *
 * `GameTestResult = std::optional<GameTestError>`，nullopt 表示测试通过；任何非 nullopt
 * 值表示测试失败，`GameTestError::type()` 描述失败的具体语义类别。
 */
enum class GameTestErrorType : i32 {
    Unknown = 0,                      // 未知错误
    Waiting = 1,                      // 等待条件超时未满足（thenWait/thenWaitAfter 轮询失败）
    ExhaustedAttempts = 2,            // 重试次数耗尽仍未达到 requiredSuccesses
    AssertAtPosition = 3,             // 带坐标的断言失败（上下文携带 BlockPos）
    MethodNotImplemented = 4,         // 调用了未实现的方法（多为脚本侧占位）
    ExecutionTimeout = 5,             // 测试体执行超过 maxTicks 仍未完成
    LevelStateModificationFailed = 6, // 修改世界状态失败（setBlock/spawn 等返回错误）
    FailConditionsMet = 7,            // failIf 条件命中或主动 fail
    Assert = 8,                       // 通用断言失败（不携带坐标）
    SimulatedPlayerOutOfBounds = 9,   // SimulatedPlayer 移出结构边界
};

/**
 * @brief 将错误类型枚举转为 JS 侧使用的字符串名。
 *
 * 对齐基岩官方 JS 文档 `GameTestErrorType` 字符串枚举，供脚本绑定层与日志输出使用。
 */
[[nodiscard]] const char* gameTestErrorTypeName(GameTestErrorType type) noexcept;

} // namespace mc::test
