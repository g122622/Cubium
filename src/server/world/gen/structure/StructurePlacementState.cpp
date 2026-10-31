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

#include "StructurePlacementState.hpp"

#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/StructureManager.hpp"
#include "server/world/gen/structure/placement/RandomSpreadStructurePlacement.hpp"

namespace mc::world::gen::structure {

namespace {

/**
 * @brief 结构集里是否有结构的群系标签与本维度可能群系相交
 *
 * 原版 `ChunkGeneratorStructureState.hasBiomesForStructureSet`：结构集的所有条目逐个
 * 展开群系，只要其中一个群系在本维度可能出现，该结构集就参与放置。群系标签缺失
 * （结构定义没带 biomes）按空集处理——它在该维度永远不会生成，与"完全不匹配"同义。
 */
bool hasBiomesForStructureSet(const StructureSet& structureSet, const std::vector<BiomeId>& possibleBiomes)
{
    for (const auto& entry : structureSet.entries()) {
        const Structure* structure = StructureRegistry::get(entry.structureId);
        if (structure == nullptr) {
            continue;
        }
        const biome::BiomeTag* tag = structure->biomeTag();
        if (tag == nullptr) {
            continue;
        }
        for (BiomeId biomeId : possibleBiomes) {
            if (tag->contains(biomeId)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

StructurePlacementState::StructurePlacementState(const biome::IBiomeSource& biomeSource, i64 worldSeed)
    : m_biomeSource(biomeSource)
    , m_worldSeed(worldSeed)
{}

void StructurePlacementState::addStructureSet(const StructureSet& structureSet)
{
    if (!hasBiomesForStructureSet(structureSet, m_biomeSource.possibleBiomes())) {
        return;
    }
    m_structureSets.push_back(&structureSet);
    if (const auto* concentric = dynamic_cast<const placement::ConcentricRingsStructurePlacement*>(
            &structureSet.placement())) {
        m_concentric.emplace(&structureSet, concentric);
    }
}

bool StructurePlacementState::isStructureChunk(const StructureSet& structureSet, i32 chunkX, i32 chunkZ) const
{
    return isPlacementChunk(structureSet, chunkX, chunkZ) && passesFrequencyReduction(structureSet, chunkX, chunkZ) &&
        passesExclusionZone(structureSet, chunkX, chunkZ);
}

bool StructurePlacementState::isPlacementChunk(const StructureSet& structureSet, i32 chunkX, i32 chunkZ) const
{
    const auto iter = m_concentric.find(&structureSet);
    if (iter != m_concentric.end()) {
        const auto& positions = ringPositions(*iter->second);
        for (const auto& pos : positions) {
            if (pos.x == chunkX && pos.z == chunkZ) {
                return true;
            }
        }
        return false;
    }
    // 随机散布：候选区块由网格与盐值决定，与该区块是否是候选无关地先算出候选点再比较。
    const auto& spread = static_cast<const placement::RandomSpreadStructurePlacement&>(structureSet.placement());
    const auto candidate = spread.getPotentialStructureChunk(m_worldSeed, chunkX, chunkZ);
    return candidate.x == chunkX && candidate.z == chunkZ;
}

bool StructurePlacementState::passesFrequencyReduction(const StructureSet& structureSet, i32 chunkX, i32 chunkZ) const
{
    return structureSet.placement().applyAdditionalChunkRestrictions(chunkX, chunkZ, m_worldSeed);
}

bool StructurePlacementState::passesExclusionZone(const StructureSet& structureSet, i32 chunkX, i32 chunkZ) const
{
    const auto& zone = structureSet.placement().exclusionZone();
    if (!zone.has_value()) {
        return true;
    }
    for (const auto* other : m_structureSets) {
        if (other->id() != zone->otherSetId) {
            continue;
        }
        // 原版连「本区块自身」也计入范围（中心处的再次落点同样算被占用），这里不额外排除。
        return !hasStructureChunkInRange(*other, chunkX, chunkZ, zone->chunkCount);
    }
    // 排斥区指向的结构集不在本维度：没有落点，不禁用。
    return true;
}

bool StructurePlacementState::hasStructureChunkInRange(
    const StructureSet& structureSet, i32 chunkX, i32 chunkZ, i32 chunkCount) const
{
    for (i32 x = chunkX - chunkCount; x <= chunkX + chunkCount; ++x) {
        for (i32 z = chunkZ - chunkCount; z <= chunkZ + chunkCount; ++z) {
            if (isStructureChunk(structureSet, x, z)) {
                return true;
            }
        }
    }
    return false;
}

const std::vector<world::chunk::ChunkPos>& StructurePlacementState::ringPositions(
    const placement::ConcentricRingsStructurePlacement& concentric) const
{
    return concentric.getRingPositions(m_worldSeed);
}

} // namespace mc::world::gen::structure