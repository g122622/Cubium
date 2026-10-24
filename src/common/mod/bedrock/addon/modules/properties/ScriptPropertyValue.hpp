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
#include "common/core/Types.hpp"
#include <nlohmann/json.hpp>

namespace mc::mod::bedrock::addon {
class IScriptBindingContext;

/** @brief 将脚本标量或 Vector3 复制为属性值，null/undefined 表示删除。 */
Result<nlohmann::json> readScriptPropertyValue(IScriptBindingContext& context, void* value);
/** @brief 将已校验的属性值创建为脚本值，调用方拥有返回句柄。 */
void* writeScriptPropertyValue(IScriptBindingContext& context, const nlohmann::json& value);
/** @brief 检查第一个参数是否是非空属性名。 */
bool hasScriptPropertyName(IScriptBindingContext& context, i32 argc, void** args);

/** @brief 优先重新抛出值转换期间产生的脚本异常，否则抛出类型校验错误。 */
void* throwScriptPropertyError(IScriptBindingContext& context, const char* message);

} // namespace mc::mod::bedrock::addon
