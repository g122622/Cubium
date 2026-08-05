/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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
 */

#pragma once

#include "common/core/Types.hpp"

namespace mc::test {

class GameTestHelper;
class SimulatedPlayer;

/**
 * @brief 脚本 GameTest 访问器（单例）。
 *
 * 桥接 QuickJS-NG 回调与 C++ 门面：JS 测试体回调由 `ScriptGameTestFunction::run` 同步触发，但 JS 侧
 * 无法直接接收 C++ `GameTestHelper&` 入参（绑定层 `callFunction0` 不透传 C++ 指针）。故 `run` 进入时
 * 经 `setCurrentHelper` 把当前测试的 `GameTestHelper*` 压入单例，JS 体执行期间经 `currentHelper()` 取回，
 * 退出时清空。对齐基岩版 `ScriptGameTestHelper` 经 `WeakEntityRef` 持有 helper 的语义（本项目无实体引用
 * 计数，简化为裸指针——helper 生命周期由 `MinecraftGameTestInstance` 拥有，测试运行期间稳定）。
 *
 * 同时承载 `Test` 类（JS）↔ `GameTestHelper`（C++）的方法转发所需的句柄查询，避免每个绑定回调都捕获
 * `this`（工厂无状态）。
 *
 * 生命周期约束：`currentHelper` 仅在 `ScriptGameTestFunction::run` 栈帧内有效；回调外访问返回 nullptr。
 */
class ScriptGameTestAccessor {
public:
    [[nodiscard]] static ScriptGameTestAccessor& instance() noexcept;

    /**
     * @brief 设置当前正在执行的测试 helper（由 `ScriptGameTestFunction::run` 进入时调用）。
     */
    void setCurrentHelper(GameTestHelper* helper) noexcept;

    /**
     * @brief 取当前 helper（回调外返回 nullptr）。
     */
    [[nodiscard]] GameTestHelper* currentHelper() const noexcept;

    /**
     * @brief 清空当前 helper（`run` 退出时调用）。
     */
    void clearCurrentHelper() noexcept;

private:
    ScriptGameTestAccessor() noexcept = default;

    GameTestHelper* m_currentHelper = nullptr;
};

} // namespace mc::test
