/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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
 */

#pragma once

#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"

namespace mc::test {

/**
 * @brief 注册 JS `RegistrationBuilder` 类绑定（包裹 C++ `ScriptRegistrationBuilder`）。
 *
 * 对齐基岩 `ScriptGameTestRegistrationBuilder`：JS `register(suite,name,fn)` 返回 `RegistrationBuilder`
 * 对象，作者链式 `.batch(...).maxTicks(...).structureName(...)...`。每个 JS 链式方法经
 * `ScriptObjectRegistry::unwrap` 取回 C++ `ScriptRegistrationBuilder*`，调对应方法，返回同一 JS 对象
 * （`this`）以维持链式。
 *
 * 终态：JS 链式末尾隐式 `build()`——对齐 JS 文档（`register(...).structureName(...).maxTicks(...)` 末尾
 * 无显式提交，由 GC 析构触发 `registerTest`）。第一阶段在 JS builder 对象析构回调内 `registerTest`，
 * 默认结构 `gametest:empty_3x3`。
 *
 * @param builder 模块构建器。
 * @param ctx 绑定上下文。
 * @return 类 id。
 */
[[nodiscard]] u64 registerRegistrationBuilderClassBinding(
    mc::mod::bedrock::addon::NativeModuleBuilder& builder, mc::mod::bedrock::addon::IScriptBindingContext& ctx);

} // namespace mc::test
