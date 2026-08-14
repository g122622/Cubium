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
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/gen/structure/placement/ConcentricRingsStructurePlacement.hpp"
#include "common/world/gen/structure/placement/RandomSpreadStructurePlacement.hpp"
#include "common/world/gen/structure/placement/StructurePlacement.hpp"

#include <memory>
#include <optional>
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

const StructureSelectionEntry* StructureSet::selectEntry(math::Random& rng) const
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
    m_initialized = false;
}

void StructureSetRegistry::initialize()
{
    clear();

    m_initialized = true;

    // ============================================================================
    // 村庄集合（5 种变体，权重各 1）
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "village_plains"), 1);
        entries.emplace_back(ResourceLocation("minecraft", "village_desert"), 1);
        entries.emplace_back(ResourceLocation("minecraft", "village_savanna"), 1);
        entries.emplace_back(ResourceLocation("minecraft", "village_snowy"), 1);
        entries.emplace_back(ResourceLocation("minecraft", "village_taiga"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(34, // spacing
            8,                                                                           // separation
            10387312,                                                                    // salt
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::Default,
            1.0f,                    // frequency
            math::Vector3i(0, 0, 0), // locateOffset
            std::nullopt             // exclusionZone
        );

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "villages"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 沙漠神殿
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "desert_pyramid"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(32,
            8,
            14357617,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "desert_pyramids"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 雪屋
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "igloo"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(32,
            8,
            14357618,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "igloos"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 丛林神庙
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "jungle_pyramid"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(32,
            8,
            14357619,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "jungle_temples"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 沼泽小屋
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "swamp_hut"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(32,
            8,
            14357620,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "swamp_huts"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 掠夺者前哨站（排斥村庄，10 区块范围）
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "pillager_outpost"), 1);

        placement::ExclusionZone exclusionZone{
            ResourceLocation("minecraft", "villages"),
            10 // chunkCount
        };

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(32,
            8,
            165745296,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::LegacyType1,
            0.2f,
            math::Vector3i(0, 0, 0),
            std::move(exclusionZone));

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "pillager_outposts"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 远古城市
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "ancient_city"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(24,
            8,
            20083232,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "ancient_cities"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 海洋纪念碑（三角分布）
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "monument"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(32,
            5,
            10387313,
            placement::RandomSpreadType::Triangular,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "ocean_monuments"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 林地府邸（三角分布）
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "mansion"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(80,
            20,
            10387319,
            placement::RandomSpreadType::Triangular,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "woodland_mansions"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 埋藏宝藏（高频率缩减，偏移 9,0,9）
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "buried_treasure"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(1,
            0,
            0,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::LegacyType2,
            0.01f,
            math::Vector3i(9, 0, 9), // locateOffset
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "buried_treasures"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 废弃矿井（两种变体，LegacyType3 频率缩减）
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "mineshaft"), 1);
        entries.emplace_back(ResourceLocation("minecraft", "mineshaft_mesa"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(1,
            0,
            0,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::LegacyType3,
            0.004f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "mineshafts"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 废弃传送门（7 种变体）
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "ruined_portal"), 1);
        entries.emplace_back(ResourceLocation("minecraft", "ruined_portal_desert"), 1);
        entries.emplace_back(ResourceLocation("minecraft", "ruined_portal_jungle"), 1);
        entries.emplace_back(ResourceLocation("minecraft", "ruined_portal_mountain"), 1);
        entries.emplace_back(ResourceLocation("minecraft", "ruined_portal_ocean"), 1);
        entries.emplace_back(ResourceLocation("minecraft", "ruined_portal_swamp"), 1);
        entries.emplace_back(ResourceLocation("minecraft", "ruined_portal_mountain_cracked"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(40,
            15,
            34222645,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "ruined_portals"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 沉船（2 种变体）
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "shipwreck"), 1);
        entries.emplace_back(ResourceLocation("minecraft", "shipwreck_beached"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(24,
            4,
            165745295,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "shipwrecks"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 海洋废墟（2 种变体）
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "ocean_ruin_cold"), 1);
        entries.emplace_back(ResourceLocation("minecraft", "ocean_ruin_warm"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(20,
            8,
            14357621,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "ocean_ruins"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 下界复合体（堡垒遗迹权重 3，下界要塞权重 2）
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "fortress"), 2);
        entries.emplace_back(ResourceLocation("minecraft", "bastion_remnant"), 3);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(27,
            4,
            30084232,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "nether_complexes"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 下界化石
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "nether_fossil"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(2,
            1,
            14357921,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "nether_fossils"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 末地城（三角分布）
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "end_city"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(20,
            11,
            10387313,
            placement::RandomSpreadType::Triangular,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "end_cities"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 要塞（同心环分布）
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "stronghold"), 1);

        auto placement = std::make_unique<placement::ConcentricRingsStructurePlacement>(32, // distance
            3,                                                                              // spread
            128,                                                                            // count
            std::vector<BiomeId>{}, // preferredBiomes（后续通过标签解析填充）
            0,                      // salt
            math::Vector3i(0, 0, 0) // locateOffset
        );

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "strongholds"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 小径废墟
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "trail_ruins"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(34,
            8,
            83469867,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "trail_ruins"), std::move(entries), std::move(placement)));
    }

    // ============================================================================
    // 试炼密室
    // ============================================================================
    {
        std::vector<StructureSelectionEntry> entries;
        entries.emplace_back(ResourceLocation("minecraft", "trial_chambers"), 1);

        auto placement = std::make_unique<placement::RandomSpreadStructurePlacement>(34,
            12,
            94251327,
            placement::RandomSpreadType::Linear,
            placement::FrequencyReductionMethod::Default,
            1.0f,
            math::Vector3i(0, 0, 0),
            std::nullopt);

        registerSet(std::make_unique<StructureSet>(
            ResourceLocation("minecraft", "trial_chambers"), std::move(entries), std::move(placement)));
    }

    spdlog::info("Initialized {} vanilla structure sets", m_sets.size());
}

} // namespace mc::world::gen::structure
