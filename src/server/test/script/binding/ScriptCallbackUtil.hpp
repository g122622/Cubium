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

#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/test/base/error/GameTestError.hpp"
#include "common/test/base/error/GameTestErrorType.hpp"
#include "common/test/base/error/GameTestResult.hpp"

#include <functional>
#include <memory>
#include <utility>

namespace mc::test {

/**
 * @brief 把 JS 回调函数句柄包装为 `std::function<GameTestResult()>`。
 *
 * 供 `GameTestSequence::thenWait`/`thenExecute*` 步骤回调、`GameTestHelper::until` 轮询回调等
 * 需要 C++ 侧反复调用 JS 函数的场景复用。调用时经 `callFunction0` 执行 JS 函数，把 JS 异常
 * 映射为 `GameTestError(FailConditionsMet)`（对齐基岩把 JS Error 映射为 GameTestError）。
 *
 * @param ctxPtr 脚本绑定上下文（非拥有，回调执行时须存活）。
 * @param jsCb   JS 函数句柄（trampoline 传入的 arg handle，本函数内部 `dupValue` 取独立副本，
 *               调用方无需 `retainValue`，trampoline 释放 arg handle 后本副本仍有效）。
 * @return `std::function`，每次调用执行 JS 函数并返回 GameTestResult；std::function 析构时
 *         自动 `releaseValue` 释放独立 handle（经 shared_ptr custom deleter）。
 *
 * 生命周期：独立 handle 由 shared_ptr 管理，最后一个引用（std::function 副本）析构时 release。
 * 故 std::function 可自由拷贝/移动，handle 引用计数语义正确。
 *
 * 注意：必须用 `dupValue` 而非 `retainValue`——retainValue 仅 JS_DupValue（refcount+1）不新建
 * handle，trampoline 会 delete arg handle 内存，致悬垂指针（见 ScriptRegister::_doRegister 注释）。
 */
inline std::function<GameTestResult()> wrapJsCallback(
    mc::mod::bedrock::addon::IScriptBindingContext* ctxPtr, void* jsCb)
{
    // dupValue 取独立 handle（refcount+1 + 新建 JSValue* 内存），trampoline 释放 arg handle 不影响。
    void* ownedHandle = ctxPtr->dupValue(jsCb);
    // shared_ptr 持 handle，custom deleter 在引用归零时 releaseValue。std::function 拷贝/移动
    // 共享同一 shared_ptr，保证 handle 在所有副本销毁后才释放。
    auto holder =
        std::make_shared<std::pair<mc::mod::bedrock::addon::IScriptBindingContext*, void*>>(ctxPtr, ownedHandle);
    auto guard = std::shared_ptr<void>(nullptr, [holder](void*) {
        if (holder->first != nullptr && holder->second != nullptr) {
            holder->first->releaseValue(holder->second);
        }
    });

    return [ctxPtr, ownedHandle, guard]() -> GameTestResult {
        void* undef = ctxPtr->createUndefined();
        void* ret = ctxPtr->callFunction0(ownedHandle, undef);
        ctxPtr->releaseValue(undef);

        GameTestResult result = pass();
        if (ctxPtr->isException(ret)) {
            void* exc = ctxPtr->getException();
            auto msg = ctxPtr->getExceptionMessage(exc);
            ctxPtr->releaseValue(exc);
            result = fail(GameTestErrorType::FailConditionsMet, std::string(msg));
        }
        ctxPtr->releaseValue(ret);
        return result;
    };
}

} // namespace mc::test
