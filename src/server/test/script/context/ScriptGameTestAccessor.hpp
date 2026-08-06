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
 * @deprecated 本单例已不再被主路径使用。JS `Test` 方法现从 Test 对象 opaque 携带 `GameTestHelper*`
 * （`ScriptGameTestFunction::run` 创建并传入），经 `ScriptObjectRegistry::unwrap(thisVal, testClassId)`
 * 取回——async 测试体 `await` 挂起后 then-handler resume 时 thisVal 仍携带正确 helper，多 async 并发安全，
 * 不再依赖单例。保留此类供未来无 thisVal 场景（如顶层 `gametest.spawnSimulatedPlayer`）备用。
 *
 * TODO: 若未来确认无复用场景，可连同 .cpp 与两处 CMake 登记一并删除。
 *
 * 历史语义：JS 测试体回调由 `ScriptGameTestFunction::run` 同步触发，`run` 进入时经 `setCurrentHelper`
 * 把 helper 压入单例，JS 体经 `currentHelper()` 取回，退出时清空。helper 生命周期由
 * `MinecraftGameTestInstance` 拥有，测试运行期间稳定。
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
