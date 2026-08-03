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

#include "common/core/Types.hpp"

#include <bitset>
#include <cstddef>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本引擎能力标志
 *
 * 控制脚本运行时可执行的操作
 */
enum class Capability : u8 {
    AllowEval = 0,  // 允许使用 eval()
    ScriptOnly = 1, // 仅脚本模式，禁止原生模块
};

/**
 * @brief 脚本能力集合
 */
class Capabilities {
public:
    Capabilities() noexcept = default;

    void setCapability(Capability cap, bool enabled) { m_capabilities[static_cast<size_t>(cap)] = enabled; }

    [[nodiscard]] bool hasCapability(Capability cap) const noexcept
    {
        return m_capabilities.test(static_cast<size_t>(cap));
    }

private:
    std::bitset<2> m_capabilities;
};

} // namespace mc::mod::bedrock::addon
