#pragma once

#include <string_view>

namespace mc::test::tags {

/**
 * @brief GameTest 预定义标签常量。
 *
 * 对齐基岩版 `gametest::tags`（`SuiteAll`/`SuiteDefault`/`SuiteDisabled`）与 Java 版 `Tags`。
 * 标签用于在注册期给测试分组（`RegistrationBuilder.tag(name)`），运行期可按标签筛选。
 *
 * - `SuiteAll`：所有测试的合集标签（`/gametest runall` 默认跑此集合）。
 * - `SuiteDefault`：默认套件（不含 disabled）。
 * - `SuiteDisabled`：禁用套件（仅手动触发，不进 `runall`）。
 * - `SuiteDebug`：调试套件（仅在显式指定时运行，对齐基岩官方 `suite:debug`）。
 * - `SuiteNextUpdate`：下一版本套件（预览/即将变更行为，对齐基岩官方 `suite:nextupdate`）。
 *
 * 用 `std::string_view` 字面量，零拷贝比较。
 */
inline constexpr std::string_view SuiteAll = "suite:all";
inline constexpr std::string_view SuiteDefault = "suite:default";
inline constexpr std::string_view SuiteDisabled = "suite:disabled";
inline constexpr std::string_view SuiteDebug = "suite:debug";
inline constexpr std::string_view SuiteNextUpdate = "suite:nextupdate";

} // namespace mc::test::tags
