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

#include "../core/Types.hpp"

namespace mc {
namespace util {

/**
 * @brief 三态枚举
 *
 * 用于表示"是 / 否 / 默认"三种状态的场景，常见于环境属性查询。
 * - True：明确为真
 * - False：明确为假
 * - Default：未设置，由调用方提供回退值
 *
 * 参考: net.minecraft.util.TriState
 */
enum class TriState : u8 {
    False = 0,
    True = 1,
    Default = 2,
};

/**
 * @brief 将 TriState 转换为布尔值
 *
 * - True 返回 true
 * - False 返回 false
 * - Default 返回调用方提供的 fallback
 *
 * 这是 TriState 最常见的使用模式：当环境属性为 Default 时，
 * 回退到当前方块/实体的状态，避免无意义的切换。
 *
 * @param value 三态值
 * @param fallback 当 value 为 Default 时返回的回退值
 * @return 对应的布尔值
 */
[[nodiscard]] inline bool triStateToBoolean(TriState value, bool fallback) noexcept
{
    switch (value) {
        case TriState::True:
            return true;
        case TriState::False:
            return false;
        case TriState::Default:
            return fallback;
    }
    return fallback;
}

/**
 * @brief 从布尔值构造 TriState
 *
 * @param value 布尔值
 * @return TriState::True 或 TriState::False
 */
[[nodiscard]] inline TriState triStateFromBoolean(bool value) noexcept
{
    return value ? TriState::True : TriState::False;
}

} // namespace util
} // namespace mc
