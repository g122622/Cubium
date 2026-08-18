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
    // 基岩 rotateTest(true) 表示测试在 4 个旋转方向各跑一次（--verify 压测）。项目 GameTestServer
    // 暂未接线 --verify 旋转批生成，TestData 无 rotate 字段。复用 manualOnly 占位（manualOnly=true
    // 的测试不进默认批量跑，需显式触发），语义近似"需特殊触发"。TODO: --verify 旋转批接线后改独立字段。
    m_data.setManualOnly(r);
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::setupTicks(i32 n) noexcept
{
    m_data.setSetupTicks(n);
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::skyAccess(bool s) noexcept
{
    // 对齐基岩 RegistrationBuilder.skyAccess(boolean)：声明测试结构需要露天/天空光照进入。
    // TestData.m_skyAccess 默认 false（封顶隔离光照），现有测试不设此值故行为不变。
    // MinecraftStructurePlacer 在 skyAccess=true 时向上清理 air 列至世界顶部，确保
    // canSeeSky=true（结构埋于 gridStartY=-59 地下 worldgen 中时 sunlight 不可达）。
    m_data.setSkyAccess(s);
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::loadSpawnChunks(bool s) noexcept
{
    // 项目独有（基岩无对应）：声明测试需强制加载结构周围 SPAWN_DISTANCE_CHUNK(8) 区块。
    // GameTestServer 无头门面无玩家区块加载链路，NaturalSpawner._collectSpawnableChunks 仅数到
    // 结构 footprint 区块（3 个），cap=maxInstances*3/289=0 致怪物/动物都不自然生成。开启此标志
    // 让 MinecraftStructurePlacer force 结构中心周围 8 区块（满载 289），对齐原版
    // DistanceManager.getNaturalSpawnChunkCount。仅 NaturalSpawner 类测试启用——副作用：结构外
    // worldgen + 自然生成残留污染，须独立 batch + 区域限定计数隔离。详见 TestData.loadSpawnChunks。
    m_data.setLoadSpawnChunks(s);
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::structureName(std::string name)
{
    m_data.setStructure(std::move(name));
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::structureLocation(std::string name)
{
    // TODO: 基岩 structureLocation 是结构在世界中的放置坐标（BlockPos），非结构名。项目 TestData
    // 无对应字段，当前等价于 structureName（占位别名）。行为包 0 使用 structureLocation，待按需补
    // BlockPos 字段与 TemplateManager 放置偏移接线后修正。
    m_data.setStructure(std::move(name));
    return *this;
}

ScriptRegistrationBuilder& ScriptRegistrationBuilder::tag(std::string t)
{
    // 暂存 tag，registerTest 时调 fn->addTag（对齐 NativeTestRegistrationBuilder 模式）。
    // tag 用于运行期按标签筛选（如 --gametest tag:suite:broken），见 GameTestTags.hpp。
    m_tags.push_back(std::move(t));
    return *this;
}

bool ScriptRegistrationBuilder::registerTest(std::string defaultStructure)
{
    if (m_data.structure().empty()) {
        m_data.setStructure(std::move(defaultStructure));
    }

    auto fn = std::make_shared<ScriptGameTestFunction>(
        m_className, m_testName, m_data.structure(), m_data, m_bindingCtx, m_jsCallback);
    for (auto& tag : m_tags) {
        fn->addTag(std::move(tag));
    }
    return GameTestRegistry::instance().registerTestMethod(m_className, std::move(fn));
}

} // namespace mc::test
