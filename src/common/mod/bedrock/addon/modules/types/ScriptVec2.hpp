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
#include "common/util/math/Vector2.hpp"

#include <optional>

namespace mc::mod::bedrock::addon {

class IScriptBindingContext;

/**
 * @brief 脚本层Vector2类型
 *
 * 与Bedrock版@minecraft/server的Vector2接口一致。
 * 使用f64（双精度浮点数）以匹配JS的number类型。
 */
struct ScriptVec2 {
    f64 x = 0.0;
    f64 y = 0.0;

    ScriptVec2() noexcept = default;
    ScriptVec2(f64 x_, f64 y_) noexcept
        : x(x_)
        , y(y_)
    {}

    /// 从游戏Vector2f转换
    explicit ScriptVec2(const math::Vector2f& v) noexcept
        : x(static_cast<f64>(v.x))
        , y(static_cast<f64>(v.y))
    {}

    /// 从游戏Vector2d转换
    explicit ScriptVec2(const math::Vector2d& v) noexcept
        : x(v.x)
        , y(v.y)
    {}

    /// 转换为Vector2f
    [[nodiscard]] math::Vector2f toVec2f() const noexcept { return {static_cast<f32>(x), static_cast<f32>(y)}; }

    /// 转换为Vector2d
    [[nodiscard]] math::Vector2d toVec2d() const noexcept { return {x, y}; }

    /// 从脚本对象读取Vector2
    static std::optional<ScriptVec2> fromJs(IScriptBindingContext& ctx, void* obj);

    /// 转换为脚本对象（返回的句柄拥有引用所有权，调用者负责releaseValue）
    [[nodiscard]] void* toJs(IScriptBindingContext& ctx) const;
};

} // namespace mc::mod::bedrock::addon
