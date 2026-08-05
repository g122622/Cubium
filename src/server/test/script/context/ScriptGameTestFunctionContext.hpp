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

#include "common/test/framework/function/IGameTestFunctionContext.hpp"

namespace mc::test {

/**
 * @brief 脚本测试函数运行上下文。
 *
 * 对齐基岩 `ScriptGameTestFunctionContext`：JS 测试体无额外上下文状态（异步 Future 轮询留 TODO），故
 * 此类是 `IGameTestFunctionContext` 的空具体实现，与 `EmptyGameTestFunctionContext` 等价但单独命名以
 * 便于未来扩展（如持 JS Promise 句柄、异步断言注册表）。
 *
 * 由 `ScriptGameTestFunction::createContext` 构造，传给 `BaseGameTestFunction::run(helper, ctx)`。
 */
class ScriptGameTestFunctionContext final : public IGameTestFunctionContext {
public:
    ScriptGameTestFunctionContext() = default;
    ~ScriptGameTestFunctionContext() override = default;
};

} // namespace mc::test
