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

#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"

#include <memory>

namespace mc::mod::bedrock::addon {

/** @brief 持有独立脚本值句柄，并在所属上下文销毁前主动失效。 */
class ScriptValueHandle {
public:
    ScriptValueHandle() = default;

    /** @brief 复制借用值；原始句柄仍由调用者管理。 */
    ScriptValueHandle(IScriptBindingContext& context, void* value)
        : m_state(std::make_shared<State>(&context, context.dupValue(value), 0))
    {
        // 清理登记不能反向拥有值状态，否则状态与上下文会形成引用环。
        const std::weak_ptr<State> weak = m_state;
        m_state->cleanupId = context.onDispose([weak]() {
            if (const auto state = weak.lock()) {
                state->reset();
            }
        });
    }

    /** @brief 返回借用句柄；上下文销毁后返回 nullptr。 */
    [[nodiscard]] void* value() const
    {
        return m_state ? m_state->value : nullptr;
    }

    /** @brief 返回仍存活的绑定上下文。 */
    [[nodiscard]] IScriptBindingContext* context() const
    {
        return m_state ? m_state->context : nullptr;
    }

private:
    struct State {
        IScriptBindingContext* context;
        void* value;
        u64 cleanupId;

        ~State()
        {
            reset();
        }

        void reset()
        {
            if (context != nullptr) {
                auto* bindings = context;
                void* handle = value;

                // 先使持有器失效，再释放 JS 值，避免对象终结器重入清理时重复释放。
                context = nullptr;
                value = nullptr;

                bindings->removeDisposeCallback(cleanupId);
                bindings->releaseValue(handle);
            }
        }
    };

    std::shared_ptr<State> m_state;
};

} // namespace mc::mod::bedrock::addon
