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
 * @brief 注册 `@minecraft/server-gametest` 模块顶层 API 绑定。
 *
 * 对齐基岩/JS 模块顶层导出：`register`/`registerAsync`/`setBeforeBatchCallback`/
 * `setAfterBatchCallback`/`spawnSimulatedPlayer`。官方 JS API 这些是模块顶层自由函数
 *（`import { register } from "@minecraft/server-gametest"`）。
 *
 * 实现约束：绑定基础设施的 `IScriptBindingContext::exportNativeFunction` 接收裸 `JSCFunction*`
 *（无 user-data），无法承载捕获状态的 `std::function`；而 `register` 等需捕获
 * `registrationBuilderClassId`（运行期值）。故第一阶段骨架采用项目既有模式（与 `@minecraft/server`
 * 的 `system`/`world` 全局对象一致）：注册一个 `GameTest` "类"（命名空间对象），把上述 5 函数作为其
 * 方法（`registerMethod` 接 `std::function`，可捕获状态），导出实例为模块顶层名 `gametest`。
 *
 * TODO（第一阶段偏差）：JS 作者须写 `gametest.register(...)` 而非 `register(...)`，与官方 API 不一致。
 * 待绑定基础设施扩展 `exportNativeFunction` 支持带 user-data 的 `std::function`（或 JSCFunctionData）后
 * 迁回顶层自由函数。
 *
 * `register` 实现：保留 JS 回调（`retainValue`），构造 C++ `ScriptRegistrationBuilder`（`new`），用
 * `ScriptObjectRegistry::wrap`（owned=true，自定义 destroy 回调在 GC 时 `registerTest` 后 delete）包裹为
 * JS `RegistrationBuilder` 对象返回。作者链式设置元数据后，JS 对象 GC 时触发 `registerTest` 提交到
 * `GameTestRegistry`（对齐 JS 文档：链式末尾无显式提交）。
 *
 * @param builder 模块构建器（`exportClass`/`exportValue` 用）。
 * @param ctx 绑定上下文。
 * @param registrationBuilderClassId `RegistrationBuilder` 类 id（`register` 返回值 wrap 用）。
 */
void registerTopLevelFunctions(mc::mod::bedrock::addon::NativeModuleBuilder& builder,
    mc::mod::bedrock::addon::IScriptBindingContext& ctx,
    u64 registrationBuilderClassId);

} // namespace mc::test
