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

#include "common/core/Result.hpp"
#include <functional>

namespace mc::mod::bedrock::addon {
class IScriptBindingContext;
class AddonDynamicProperties;

/** @brief 属性访问及成功调用后的提交入口；提交回调同时持有临时快照的寿命。 */
struct DynamicPropertyAccess {
    AddonDynamicProperties* properties;
    std::function<Result<void>()> commit;
};

/** @brief 解析当前方法接收者的属性所有者和提交方式；第三个参数标记写操作。 */
using DynamicPropertyAccessor = std::function<Result<DynamicPropertyAccess>(IScriptBindingContext&, void*, bool)>;

/** @brief 注册统一的动态属性方法，访问器管理所属对象的生命周期。 */
void registerDynamicPropertyBindings(IScriptBindingContext& context, void* prototype, DynamicPropertyAccessor accessor);

} // namespace mc::mod::bedrock::addon
