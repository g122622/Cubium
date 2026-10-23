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

#include "common/mod/bedrock/addon/binding/ScriptValueHandle.hpp"

namespace mc::mod::bedrock::addon {

/** @brief 管理回调的独立句柄及上下文失效，不持有调用方的临时句柄地址。 */
class ScriptCallbackHolder {
public:
    ScriptCallbackHolder() = default;

    /** @brief 复制一个已验证的函数值。 */
    ScriptCallbackHolder(IScriptBindingContext& context, void* function)
        : m_function(context, function)
    {}

    /** @brief 调用带一个参数的回调，返回值由调用者释放。 */
    [[nodiscard]] void* call(void* argument) const
    {
        auto* bindings = context();
        void* receiver = bindings->createUndefined();
        void* result = bindings->callFunction1(func(), receiver, argument);
        bindings->releaseValue(receiver);
        return result;
    }

    /** @brief 调用无参数回调，返回值由调用者释放。 */
    [[nodiscard]] void* call0() const
    {
        auto* bindings = context();
        void* receiver = bindings->createUndefined();
        void* result = bindings->callFunction0(func(), receiver);
        bindings->releaseValue(receiver);
        return result;
    }

    /** @brief 回调及所属上下文是否仍然存活。 */
    [[nodiscard]] bool isValid() const { return m_function.value() != nullptr; }

    /** @brief 获取借用的函数值。 */
    [[nodiscard]] void* func() const { return m_function.value(); }

    /** @brief 获取存活的上下文。 */
    [[nodiscard]] IScriptBindingContext* context() const { return m_function.context(); }

private:
    ScriptValueHandle m_function;
};

} // namespace mc::mod::bedrock::addon
