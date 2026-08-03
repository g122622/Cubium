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

#include "common/mod/bedrock/addon/modules/types/ScriptVec3.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include <optional>

namespace mc::mod::bedrock::addon {

std::optional<ScriptVec3> ScriptVec3::fromJs(IScriptBindingContext& ctx, void* obj)
{
    if (!ctx.isObject(obj)) {
        return std::nullopt;
    }

    auto xv = ctx.getPropertyFloat(obj, "x");
    auto yv = ctx.getPropertyFloat(obj, "y");
    auto zv = ctx.getPropertyFloat(obj, "z");

    if (!xv || !yv || !zv) {
        return std::nullopt;
    }

    return ScriptVec3(*xv, *yv, *zv);
}

void* ScriptVec3::toJs(IScriptBindingContext& ctx) const
{
    void* obj = ctx.createObject();
    ctx.setPropertyFloat(obj, "x", x);
    ctx.setPropertyFloat(obj, "y", y);
    ctx.setPropertyFloat(obj, "z", z);
    return obj;
}

} // namespace mc::mod::bedrock::addon
