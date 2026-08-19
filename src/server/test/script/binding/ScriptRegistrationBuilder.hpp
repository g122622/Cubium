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

#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/test/base/data/TestData.hpp"

#include <string>
#include <vector>

namespace mc::test {

/**
 * @brief 脚本测试注册 builder（镜像 `NativeTestRegistrationBuilder` 的 11 链式方法）。
 *
 * 对齐基岩/JS `RegistrationBuilder`：`register(suite,name,fn)` 返回此 builder（按值），作者链式设置
 * 元数据后由绑定层隐式 `registerTest()` 提交。与原生 builder 的差异：
 * - 持 JS 回调句柄 + 绑定上下文（构造期注入），而非 `NativeGameTestFunction::TestBody`。
 * - JS 导出名 `rotateTest`（对齐 JS 文档），原生层方法名 `rotate`（对齐 C++ 风格）。
 * - 终态 `registerTest()` 由绑定层 `register` 回调收尾时调用（JS 侧无显式 `registerTest`）。
 *
 * 链式方法返回 `ScriptRegistrationBuilder&`（除构造/`registerTest`）。字段填充 `TestData`，与原生共用
 * 同一 `GameTestRegistry`，故 `TestData` schema 必须与原生一致。
 */
class ScriptRegistrationBuilder {
public:
    ScriptRegistrationBuilder(std::string className,
        std::string testName,
        mc::mod::bedrock::addon::IScriptBindingContext* bindingCtx,
        void* jsCallback);

    ~ScriptRegistrationBuilder();

    ScriptRegistrationBuilder(const ScriptRegistrationBuilder&) = delete;
    ScriptRegistrationBuilder& operator=(const ScriptRegistrationBuilder&) = delete;
    ScriptRegistrationBuilder(ScriptRegistrationBuilder&&) noexcept = default;
    ScriptRegistrationBuilder& operator=(ScriptRegistrationBuilder&&) noexcept = delete;

    ScriptRegistrationBuilder& batch(std::string name);
    ScriptRegistrationBuilder& maxAttempts(i32 n) noexcept;
    ScriptRegistrationBuilder& maxTicks(i32 n) noexcept;
    ScriptRegistrationBuilder& padding(i32 n) noexcept;
    ScriptRegistrationBuilder& required(bool r) noexcept;
    ScriptRegistrationBuilder& requiredSuccessfulAttempts(i32 n) noexcept;
    ScriptRegistrationBuilder& rotateTest(bool r) noexcept;
    ScriptRegistrationBuilder& setupTicks(i32 n) noexcept;
    ScriptRegistrationBuilder& skyAccess(bool s) noexcept;
    ScriptRegistrationBuilder& loadSpawnChunks(bool s) noexcept;
    ScriptRegistrationBuilder& structureName(std::string name);
    ScriptRegistrationBuilder& structureLocation(std::string name);
    ScriptRegistrationBuilder& tag(std::string t);

    /**
     * @brief 终态：构造 `ScriptGameTestFunction` 提交到 `GameTestRegistry`。
     *
     * @param defaultStructure 默认结构名（`register` 时由绑定层传，JS 未设则用此）。
     * @return true=注册成功，false=同名已存在。
     */
    bool registerTest(std::string defaultStructure);

private:
    std::string m_className;
    std::string m_testName;
    mc::mod::bedrock::addon::IScriptBindingContext* m_bindingCtx;
    void* m_jsCallback;
    TestData m_data;
    /// tag() 暂存，registerTest 时调 fn->addTag（对齐 NativeTestRegistrationBuilder）。
    std::vector<std::string> m_tags;
};

} // namespace mc::test
