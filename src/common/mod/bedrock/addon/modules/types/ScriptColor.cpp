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

#include "common/mod/bedrock/addon/modules/types/ScriptColor.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include <optional>

namespace mc::mod::bedrock::addon {

std::optional<ScriptColor> ScriptColor::fromJs(IScriptBindingContext& ctx, void* obj)
{
    // 验证输入对象是否有效
    if (!ctx.isObject(obj)) {
        return std::nullopt;
    }

    // 读取颜色分量，red/green/blue为必需字段
    auto rv = ctx.getPropertyFloat(obj, "red");
    auto gv = ctx.getPropertyFloat(obj, "green");
    auto bv = ctx.getPropertyFloat(obj, "blue");

    // 必需字段缺失则返回空
    if (!rv || !gv || !bv) {
        return std::nullopt;
    }

    // alpha为可选字段，默认为1.0
    auto av = ctx.getPropertyFloat(obj, "alpha");

    return ScriptColor(
        static_cast<f32>(*rv), static_cast<f32>(*gv), static_cast<f32>(*bv), av ? static_cast<f32>(*av) : 1.0f);
}

void* ScriptColor::toJs(IScriptBindingContext& ctx) const
{
    // 创建脚本对象并设置颜色属性
    void* obj = ctx.createObject();
    ctx.setPropertyFloat(obj, "red", static_cast<f64>(r));
    ctx.setPropertyFloat(obj, "green", static_cast<f64>(g));
    ctx.setPropertyFloat(obj, "blue", static_cast<f64>(b));
    ctx.setPropertyFloat(obj, "alpha", static_cast<f64>(a));
    return obj;
}

} // namespace mc::mod::bedrock::addon
