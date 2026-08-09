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
 */

#pragma once

#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/test/base/error/GameTestErrorType.hpp"
#include "common/test/base/error/GameTestResult.hpp"

#include <string_view>

namespace mc::test {

/**
 * @brief 注册 @minecraft/server-gametest 的错误类与枚举常量绑定（批次3）。
 *
 * 对齐基岩官方 JS 文档，注册三个真 JS Error 子类：
 * - `GameTestError`：测试断言失败信封（type/message/context/params）。prototype 经
 *   `setPrototypeOf` 挂到全局 `Error.prototype`，使 JS 侧 `instanceof Error` 成立。
 * - `GameTestCompletedError`：测试已结束信号（reason/gameTestName/methodName），同挂 Error.prototype。
 * - `GameTestErrorContext`：错误上下文值对象（absolutePosition/relativePosition/tickCount）。
 *
 * 三类均不暴露 JS 构造函数（`exportClass` 仅建原型，不导出 ctor）；实例由绑定层在失败时经
 * `createObjectWithProto` 构造并 `throwValue` 抛出。classId/proto 登记 `ScriptClassRegistry`，
 * 供运行期 helper（`throwGameTestError`/`throwGameTestErrorFromResult`）按名查表构造实例。
 *
 * 同时导出三个枚举常量对象（对齐基岩字符串枚举语义）：
 * - `GameTestErrorType`：Unknown/Waiting/.../SimulatedPlayerOutOfBounds，值=自身 PascalCase 名。
 * - `GameTestCompletedErrorReason`：{"Done":"Done","Cleanup":"Cleanup"}（Cleanup 小写 u 对齐官方）。
 * - `Tags`：suite:all/suite:default/suite:disabled/suite:debug/suite:nextupdate 五个套件标签。
 *
 * @param builder 模块构建器（exportClass/exportValue 用）。
 * @param ctx 绑定上下文（取全局 Error.prototype 建 Error 子类继承链）。
 */
void registerGameTestErrorClasses(
    mc::mod::bedrock::addon::NativeModuleBuilder& builder, mc::mod::bedrock::addon::IScriptBindingContext& ctx);

/**
 * @brief 构造 `GameTestError` JS 实例并作为异常抛出。
 *
 * 供 SimulatedPlayer/模块级方法的 stub（依赖未就绪体系）统一抛对齐基岩
 * `GameTestErrorType::MethodNotImplemented` 的错误。不走 GameTestResult 通道的调用点用此 helper。
 *
 * @param ctx 绑定上下文。
 * @param type 错误类型（多为 MethodNotImplemented）。
 * @param message 错误消息。
 * @return 异常句柄（语义对齐 throwTypeError：引擎实现返回 JS_EXCEPTION 的包装句柄）。
 */
[[nodiscard]] void* throwGameTestError(
    mc::mod::bedrock::addon::IScriptBindingContext& ctx, GameTestErrorType type, std::string_view message);

/**
 * @brief 把 GameTestResult 映射为 JS 行为：通过→undefined；失败→构造 GameTestError JS 实例并 throwValue。
 *
 * 替代 ScriptTestHelper 旧 `_resultToJs`（原用 throwInternalError 抛普通 InternalError，不携带
 * type/context 字段且非 Error 子类）。失败时从 GameTestError 取 type/message/params/context 构造
 * JS GameTestError 实例（经 ScriptClassRegistry 查 classId/proto），throwValue 抛出。
 *
 * 注：GameTestResult = optional<GameTestError>，不承载 GameTestCompletedError——CompletedError 的 JS
 * 抛出路径由调用点预判 `isCompleted()` 触发（见 throwGameTestCompletedError），不在本通道。
 *
 * @param ctx 绑定上下文。
 * @param result 门面返回的 GameTestResult。
 * @return 通过→undefined 句柄；失败→异常句柄（调用方 return）。
 */
[[nodiscard]] void* throwGameTestErrorFromResult(
    mc::mod::bedrock::addon::IScriptBindingContext& ctx, GameTestResult result);

/**
 * @brief 构造 `GameTestCompletedError` JS 实例并作为异常抛出。
 *
 * 供 Test/SimulatedPlayer 方法在测试已结束态调用时抛出（对齐基岩 idle/until/removeSimulatedPlayer/
 * getTestDirection/startSequence 仅抛 GameTestCompletedError）。
 *
 * @param ctx 绑定上下文。
 * @param gameTestName 测试名。
 * @param methodName 被调方法名。
 * @return 异常句柄。
 */
[[nodiscard]] void* throwGameTestCompletedError(
    mc::mod::bedrock::addon::IScriptBindingContext& ctx, std::string_view gameTestName, std::string_view methodName);

} // namespace mc::test
