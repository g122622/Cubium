#pragma once

#include "common/test/native/NativeGameTestFunction.hpp" // NativeGameTestFunction::TestBody（wrapNativeBody 返回类型）
#include "common/test/native/NativeTestRegistrationBuilder.hpp"

#include <string>

// 前向声明：facade 层 GameTestRegistrar/GameTestHelper 在 server/test/facade 定义，包含此头的 TU 须先 include
// server/test/facade/GameTestRegistrar.hpp 与 server/test/facade/GameTestHelper.hpp。mc_test 库本身不实例化此宏；
// 仅在服务器 EXE / 测试 TU（含 facade）内使用。
//
// wrapNativeBody 模板定义在 facade 头 server/test/facade/GameTestHelper.hpp（不在本头），因其内含
// static_cast<GameTestHelper&>（向派生类引用转换）需 GameTestHelper 完整类型；本头属 mc_test 库（仅依赖 mc_common），
// 不可 include facade 头（依赖 server 类型），故 GameTestHelper 在此仅前向声明。TU 经 facade 头取得完整定义 +
// wrapNativeBody 模板，宏展开处实例化时类型完整。
namespace mc::test {
class GameTestHelper;
class GameTestRegistrar;

template <typename Body>
NativeGameTestFunction::TestBody wrapNativeBody(Body body);
} // namespace mc::test

/**
 * @brief 注册原生 GameTest 测试（静态初始化，命名空间作用域）。
 *
 * 用法（在 .cpp 文件全局/命名空间作用域）：
 * ```
 * // 1. 定义测试体（自由函数，签名 void(GameTestHelper&)）
 * static void ExampleTests_alwaysSucceed(mc::test::GameTestHelper& helper)
 * {
 *     helper.startSequence().thenExecute([] { return mc::test::pass(); }).thenSucceed();
 * }
 *
 * // 2. 注册（链式设置元数据 + registerTest()）
 * MC_REGISTER_GAME_TEST("ExampleTests", "alwaysSucceed", ExampleTests_alwaysSucceed)
 *     .structureName("gametest:empty_3x3")
 *     .maxTicks(20)
 *     .required(true)
 *     .registerTest();
 * ```
 *
 * 展开为 `namespace { inline const bool _mc_gt_N = GameTestRegistrar::create(...).chain().registerTest();`，
 * 即一个静态 bool 初始化语句——链式调用修改临时 builder，`registerTest()` 提交到 `GameTestRegistry` 并返回 bool。
 * 静态初始化在 `main` 前执行，`GameTestRegistry` 是 Meyers 单例（函数局部静态），可安全访问。
 *
 * 包含此头的 TU 须先 include `server/test/facade/GameTestRegistrar.hpp`（提供 `GameTestRegistrar::create`）。
 *
 * @param className 套件/类名（如 `"ExampleTests"`，对应 JS `register(testClassName,...)` 首参）。
 * @param testName  测试名（如 `"alwaysSucceed"`）。
 * @param bodyFn    测试体函数名（如 `ExampleTests_alwaysSucceed`，签名 `void(GameTestHelper&)`）。
 */
#define MC_REGISTER_GAME_TEST(className, testName, bodyFn)                                    \
    static const bool _mc_gametest_reg_##__COUNTER__ = ::mc::test::GameTestRegistrar::create( \
        std::string(className), std::string(testName), ::mc::test::wrapNativeBody(bodyFn))