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

#include "StructureSet.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "server/world/gen/structure/StructureManager.hpp"
#include "server/world/gen/structure/placement/ConcentricRingsStructurePlacement.hpp"
#include "server/world/gen/structure/placement/RandomSpreadStructurePlacement.hpp"
#include "server/world/gen/structure/placement/StructurePlacement.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::world::gen::structure {

// ============================================================================
// StructureSet
// ============================================================================

StructureSet::StructureSet(ResourceLocation id,
    std::vector<StructureSelectionEntry> entries,
    std::unique_ptr<placement::StructurePlacement> placement)
    : m_id(std::move(id))
    , m_entries(std::move(entries))
    , m_placement(std::move(placement))
{
    MC_ASSERT_RELEASE(m_placement != nullptr);
}

const StructureSelectionEntry* StructureSet::selectEntry(math::IRandom& rng) const
{
    if (m_entries.empty()) {
        return nullptr;
    }

    i32 total = totalWeight();
    if (total <= 0) {
        return nullptr;
    }

    i32 target = rng.nextInt(total);
    i32 accumulated = 0;

    for (const auto& entry : m_entries) {
        accumulated += entry.weight;
        if (target < accumulated) {
            return &entry;
        }
    }

    // 兜底返回最后一个
    return &m_entries.back();
}

i32 StructureSet::totalWeight() const
{
    i32 total = 0;
    for (const auto& entry : m_entries) {
        total += entry.weight;
    }
    return total;
}

// ============================================================================
// StructureSetRegistry
// ============================================================================

StructureSetRegistry& StructureSetRegistry::instance()
{
    static StructureSetRegistry s_instance;
    return s_instance;
}

void StructureSetRegistry::registerSet(std::unique_ptr<StructureSet> set)
{
    if (!set) {
        spdlog::warn("StructureSetRegistry: attempted to register null set");
        return;
    }

    const ResourceLocation id = set->id();
    // spdlog::info("Registering structure set '{}' with {} entries", id.toString(), set->entries().size());

    // 建立结构 ID → 所属集合的反向索引
    for (const auto& entry : set->entries()) {
        m_byStructureId[entry.structureId] = set.get();
    }

    m_byId[id] = set.get();
    m_sets.push_back(std::move(set));
}

const StructureSet* StructureSetRegistry::get(const ResourceLocation& id) const
{
    auto it = m_byId.find(id);
    if (it != m_byId.end()) {
        return it->second;
    }
    return nullptr;
}

const StructureSet* StructureSetRegistry::findByStructure(const ResourceLocation& structureId) const
{
    auto it = m_byStructureId.find(structureId);
    if (it != m_byStructureId.end()) {
        return it->second;
    }
    return nullptr;
}

void StructureSetRegistry::clear()
{
    m_byId.clear();
    m_byStructureId.clear();
    m_sets.clear();
}

void StructureSetRegistry::validateAgainstStructureRegistry() const
{
    // 注册表为空 = 未加载数据包，是合法状态（不生成任何结构），无需校验
    if (m_sets.empty()) {
        return;
    }

    // 【启动期一次性校验】结构集引用的每个结构必须已在 StructureRegistry 注册。
    // 消费侧（_hasBiomesForStructureSet / generateStructureStarts）对该条件取硬断言策略，
    // 若放任到首个区块生成才暴露，故障会以"每区块一次崩溃"的形式爆发；此处一次列全所有
    // 缺失 id，再统一断言，把故障前移到装配完成时刻。
    std::vector<std::string> missing;
    for (const auto& setPtr : m_sets) {
        for (const auto& entry : setPtr->entries()) {
            if (StructureRegistry::get(entry.structureId) == nullptr) {
                missing.push_back(setPtr->id().toString() + " -> " + entry.structureId.toString());
            }
        }
    }
    if (!missing.empty()) {
        for (const auto& m : missing) {
            spdlog::critical("[STRUCT] structure set references unregistered structure: {}", m);
        }
        MC_ASSERT_RELEASE_MSG(false,
            "StructureSetRegistry: structure sets reference unregistered structures; "
            "load worldgen/structure definitions before structure sets (see critical logs above)");
    }
}

} // namespace mc::world::gen::structure
