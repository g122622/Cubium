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

#include <optional>

namespace mc::mod::bedrock::addon {

class IScriptBindingContext;

/**
 * @brief 脚本层Color类型
 *
 * RGBA颜色，每个通道0.0-1.0范围，
 * 与Bedrock版@minecraft/server的Color接口一致。
 */
struct ScriptColor {
    f32 r = 0.0f; ///< 红色通道 (0.0-1.0)
    f32 g = 0.0f; ///< 绿色通道 (0.0-1.0)
    f32 b = 0.0f; ///< 蓝色通道 (0.0-1.0)
    f32 a = 1.0f; ///< 透明度通道 (0.0-1.0)

    ScriptColor() = default;

    /**
     * @brief 构造指定颜色的ScriptColor
     * @param r_ 红色通道 (0.0-1.0)
     * @param g_ 绿色通道 (0.0-1.0)
     * @param b_ 蓝色通道 (0.0-1.0)
     * @param a_ 透明度通道 (0.0-1.0)，默认为1.0（完全不透明）
     */
    ScriptColor(f32 r_, f32 g_, f32 b_, f32 a_ = 1.0f) noexcept
        : r(r_)
        , g(g_)
        , b(b_)
        , a(a_)
    {}

    /**
     * @brief 从0-255整数范围构建颜色
     * @param ri 红色通道 (0-255)
     * @param gi 绿色通道 (0-255)
     * @param bi 蓝色通道 (0-255)
     * @param ai 透明度通道 (0-255)，默认为255（完全不透明）
     * @return 对应的ScriptColor对象
     */
    static ScriptColor fromRGBA(u8 ri, u8 gi, u8 bi, u8 ai = 255) noexcept
    {
        return ScriptColor(ri / 255.0f, gi / 255.0f, bi / 255.0f, ai / 255.0f);
    }

    /**
     * @brief 从脚本对象读取Color
     * @param ctx 脚本绑定上下文
     * @param obj 脚本对象指针
     * @return 成功返回ScriptColor，失败返回std::nullopt
     */
    static std::optional<ScriptColor> fromJs(IScriptBindingContext& ctx, void* obj);

    /**
     * @brief 转换为脚本对象
     * @param ctx 脚本绑定上下文
     * @return 脚本对象句柄，调用者负责调用releaseValue释放引用
     */
    [[nodiscard]] void* toJs(IScriptBindingContext& ctx) const;
};

} // namespace mc::mod::bedrock::addon
