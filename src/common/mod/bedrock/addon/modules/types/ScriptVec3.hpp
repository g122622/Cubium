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
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"

#include <optional>

namespace mc::mod::bedrock::addon {

class IScriptBindingContext;

/**
 * @brief 脚本层Vector3类型
 *
 * 与Bedrock版@minecraft/server的Vector3接口一致。
 * 使用f64（双精度浮点数）以匹配JS的number类型。
 * 同时用于表示BlockPos（整数值的Vector3）。
 */
struct ScriptVec3 {
    f64 x = 0.0;
    f64 y = 0.0;
    f64 z = 0.0;

    ScriptVec3() noexcept = default;
    ScriptVec3(f64 x_, f64 y_, f64 z_) noexcept
        : x(x_)
        , y(y_)
        , z(z_)
    {}

    /// 从游戏Vector3f转换
    explicit ScriptVec3(const math::Vector3f& v) noexcept
        : x(static_cast<f64>(v.x))
        , y(static_cast<f64>(v.y))
        , z(static_cast<f64>(v.z))
    {}

    /// 从游戏Vector3d转换
    explicit ScriptVec3(const math::Vector3d& v) noexcept
        : x(v.x)
        , y(v.y)
        , z(v.z)
    {}

    /// 从BlockPos转换（整数值）
    explicit ScriptVec3(const BlockPos& p) noexcept
        : x(static_cast<f64>(p.x))
        , y(static_cast<f64>(p.y))
        , z(static_cast<f64>(p.z))
    {}

    /// 转换为Vector3f
    [[nodiscard]] math::Vector3f toVec3f() const noexcept
    {
        return {static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)};
    }

    /// 转换为Vector3d
    [[nodiscard]] math::Vector3d toVec3d() const noexcept { return {x, y, z}; }

    /// 转换为BlockPos
    [[nodiscard]] BlockPos toBlockPos() const noexcept
    {
        return {static_cast<BlockCoord>(x), static_cast<BlockCoord>(y), static_cast<BlockCoord>(z)};
    }

    /// 从脚本对象读取Vector3
    static std::optional<ScriptVec3> fromJs(IScriptBindingContext& ctx, void* obj);

    /// 转换为脚本对象（返回的句柄拥有引用所有权，调用者负责releaseValue）
    [[nodiscard]] void* toJs(IScriptBindingContext& ctx) const;
};

} // namespace mc::mod::bedrock::addon
