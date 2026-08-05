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

#include "server/test/script/binding/ScriptRegistrationBuilder.hpp"

#include "common/test/framework/registry/GameTestRegistry.hpp"
#include "common/util/Direction.hpp" // Rotation
#include "server/test/script/binding/ScriptGameTestFunction.hpp"

#include <memory>
#include <utility>

namespace mc::test {

ScriptRegistrationBuilder::ScriptRegistrationBuilder(std::string className,
    std::string testName,
    mc::mod::bedrock::addon::IScriptBindingContext* bindingCtx,
    void* jsCallback)
    : m_className(std::move(className))
    , m_testName(std::move(testName))
    , m_bindingCtx(bindingCtx)
    , m_jsCallback(jsCallback)
{
    // JS 回调由调用方 retainValue 后传入；本对象持有，析构时不释放（所有权在 ScriptGameTestFunction）。
}

ScriptRegistrationBuilder::~ScriptRegistrationBuilder() = default;

ScriptRegistrationBuilder& ScriptRegistrationBuilder::batch(std::string name)
{
    m_data.setBatchName(std::move(name));
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::maxAttempts(i32 n) noexcept
{
    m_data.setMaxAttempts(n);
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::maxTicks(i32 n) noexcept
{
    m_data.setMaxTicks(n);
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::padding(i32 n) noexcept
{
    m_data.setPadding(n);
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::required(bool r) noexcept
{
    m_data.setRequired(r);
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::requiredSuccessfulAttempts(i32 n) noexcept
{
    m_data.setRequiredSuccesses(n);
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::rotateTest(bool r) noexcept
{
    // TODO: --verify 旋转压测标记暂存；TestData 无对应字段，复用 manualOnly 占位待 GameTestServer --verify 接线。
    m_data.setManualOnly(r);
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::setupTicks(i32 n) noexcept
{
    m_data.setSetupTicks(n);
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::structureName(std::string name)
{
    m_data.setStructure(std::move(name));
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::structureLocation(std::string name)
{
    // 与原生 builder 一致：structureLocation 等价于 structureName（占位别名）。
    m_data.setStructure(std::move(name));
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::tag(std::string t)
{
    // TestData 无 tag 字段（tag 在 BaseGameTestFunction::addTag）；此处暂存到 batchName 不合适，
    // 故 TODO：tag 需在 registerTest 构造函数后调 function->addTag。当前先忽略并记 TODO。
    // TODO: tag 暂存与 apply（需改 ScriptGameTestFunction 持 tags 或 GameTestRegistry 支持）。
    (void)t;
    return *this;
}

bool ScriptRegistrationBuilder::registerTest(std::string defaultStructure)
{
    if (m_data.structure().empty()) {
        m_data.setStructure(std::move(defaultStructure));
    }

    auto fn = std::make_shared<ScriptGameTestFunction>(
        m_className, m_testName, m_data.structure(), m_data, m_bindingCtx, m_jsCallback);
    return GameTestRegistry::instance().registerTestMethod(m_className, std::move(fn));
}

} // namespace mc::test
