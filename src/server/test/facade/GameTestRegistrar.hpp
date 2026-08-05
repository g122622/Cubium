#pragma once

#include "common/test/native/NativeGameTestFunction.hpp"
#include "common/test/native/NativeTestRegistrationBuilder.hpp"

#include <string>

namespace mc::test {

/**
 * @brief 注册门面：`mc::test` 子系统对外暴露的测试注册入口。
 *
 * 对齐基岩/JS `register(testClassName, testName, fn)`：作者经 `MC_REGISTER_GAME_TEST` 宏间接调用
 * `GameTestRegistrar::create()`，获得 `NativeTestRegistrationBuilder`（按值返回），链式设置元数据后
 * `registerTest()` 提交到内部 `GameTestRegistry`。
 *
 * 这是原生测试作者唯一接触的注册 API。脚本测试经 `ScriptRegistrationBuilder`（`script/`）同样汇入
 * `GameTestRegistry`，但不经此门面。
 *
 * 注：方法名用 `create` 而非 `register`——`register` 是 C++ 保留关键字（虽仅在显式 `register` 存储类
 * 声明时禁用，但作为方法名部分编译器仍报错，且易混淆），改用 `create` 表"创建注册 builder"。
 *
 * 门面纪律：`GameTestRegistry`（内部数据容器）不对外，外部仅经此门面访问。
 */
class GameTestRegistrar {
public:
    /**
     * @brief 注册原生测试，返回 builder（按值）。
     *
     * @param className 套件/类名（如 `"ExampleTests"`）。
     * @param testName  测试名（如 `"alwaysSucceed"`）。
     * @param body      测试体（`NativeGameTestFunction::TestBody`，即 `GameTestResult(IGameTestHelper&)`）。
     * @return `NativeTestRegistrationBuilder`，作者链式 `.structureName(...)...registerTest()`。
     */
    [[nodiscard]] static NativeTestRegistrationBuilder create(
        std::string className, std::string testName, NativeGameTestFunction::TestBody body);
};

} // namespace mc::test
