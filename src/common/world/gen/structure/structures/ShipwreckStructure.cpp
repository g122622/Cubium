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

#include "ShipwreckStructure.hpp"

#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/feature/template/TemplateLoader.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/jigsaw/JigsawAssembler.hpp"
#include <algorithm>

namespace mc::world::gen::structure {

// ============================================================================
// 静态模板名称
// ============================================================================

// 搁浅沉船变体
const std::vector<std::string> ShipwreckStructure::s_beachedTemplates = {"shipwreck/with_mast",
    "shipwreck/sideways_full",
    "shipwreck/sideways_fronthalf",
    "shipwreck/sideways_backhalf",
    "shipwreck/rightsideup_full",
    "shipwreck/rightsideup_fronthalf",
    "shipwreck/rightsideup_backhalf",
    "shipwreck/with_mast_degraded",
    "shipwreck/rightsideup_full_degraded",
    "shipwreck/rightsideup_fronthalf_degraded",
    "shipwreck/rightsideup_backhalf_degraded"};

// 所有沉船变体
const std::vector<std::string> ShipwreckStructure::s_allTemplates = {"shipwreck/with_mast",
    "shipwreck/upsidedown_full",
    "shipwreck/upsidedown_fronthalf",
    "shipwreck/upsidedown_backhalf",
    "shipwreck/sideways_full",
    "shipwreck/sideways_fronthalf",
    "shipwreck/sideways_backhalf",
    "shipwreck/rightsideup_full",
    "shipwreck/rightsideup_fronthalf",
    "shipwreck/rightsideup_backhalf",
    "shipwreck/with_mast_degraded",
    "shipwreck/upsidedown_full_degraded",
    "shipwreck/upsidedown_fronthalf_degraded",
    "shipwreck/upsidedown_backhalf_degraded",
    "shipwreck/sideways_full_degraded",
    "shipwreck/sideways_fronthalf_degraded",
    "shipwreck/sideways_backhalf_degraded",
    "shipwreck/rightsideup_full_degraded",
    "shipwreck/rightsideup_fronthalf_degraded",
    "shipwreck/rightsideup_backhalf_degraded"};

// ============================================================================
// 常量
// ============================================================================

const std::string ShipwreckStructure::m_name = "shipwreck";
const BlockPos ShipwreckPiece::STRUCTURE_OFFSET{4, 0, 15};

// ============================================================================
// ShipwreckPiece
// ============================================================================

ShipwreckPiece::ShipwreckPiece(const std::string& templateName,
    const BlockPos& position,
    Rotation rotation,
    bool isBeached)
    : StructurePiece(StructurePieceTypes::BURIED_TREASURE, // 复用类型 ID
          position.x,
          position.y,
          position.z,
          position.x,
          position.y,
          position.z)
    , m_templateName(templateName)
    , m_rotation(rotation)
    , m_isBeached(isBeached)
    , m_size(1, 1, 1)
{}

void ShipwreckPiece::_loadTemplate()
{
    if (!m_templateManager) {
        return;
    }

    ResourceLocation location(m_templateName);
    m_template = m_templateManager->getTemplate(location);

    if (m_template) {
        m_size = m_template->getSize();
        // 更新边界框
        // 计算旋转后的尺寸
        i32 sizeX = m_size.x;
        i32 sizeZ = m_size.z;
        if (m_rotation == Rotation::Clockwise90 || m_rotation == Rotation::CounterClockwise90) {
            std::swap(sizeX, sizeZ);
        }
        m_maxX = m_minX + sizeX - 1;
        m_maxY = m_minY + m_size.y - 1;
        m_maxZ = m_minZ + sizeZ - 1;
    }
}

void ShipwreckPiece::generate(IWorldWriter& world,
    math::Random& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    if (!m_templateManager) {
        return;
    }

    // 延迟加载模板
    if (!m_template) {
        _loadTemplate();
    }

    if (!m_template) {
        return;
    }

    // 创建放置设置
    feature::template_::PlacementSettings settings;
    settings.setRotation(m_rotation);
    settings.setMirror(Mirror::None);
    settings.setCenterOffset(STRUCTURE_OFFSET);
    settings.setBoundingBox(&chunkBounds);

    // 添加空气和结构方块忽略处理器
    std::vector<u32> blocksToIgnore;
    if (auto* airState = VanillaBlocks::getState(VanillaBlocks::AIR)) {
        blocksToIgnore.push_back(airState->blockId());
    }
    feature::template_::StructureProcessorList processors;
    processors.addProcessor(std::make_unique<feature::template_::BlockIgnoreStructureProcessor>(blocksToIgnore));
    settings.setProcessors(&processors);

    // 放置模板（使用中心偏移，需要调整位置）
    BlockPos adjustedPos(m_minX - STRUCTURE_OFFSET.x, m_minY, m_minZ - STRUCTURE_OFFSET.z);
    m_template->place(world, adjustedPos, settings, rng, 18);
}

// ============================================================================
// ShipwreckStructure
// ============================================================================

ShipwreckStructure::ShipwreckStructure()
    : Structure(ResourceLocation("minecraft", "shipwreck"))
{}

const biome::BiomeTag* ShipwreckStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_SHIPWRECK();
}

bool ShipwreckStructure::canGenerate(
    IWorld& /*world*/, IChunkGenerator& /*generator*/, math::Random& /*rng*/, i32 /*chunkX*/, i32 /*chunkZ*/)
{
    // 沉船不像海洋废墟那样有随机概率检查，直接由间距设置控制生成频率
    return true;
}

std::string ShipwreckStructure::_getRandomTemplateName(math::Random& rng, bool isBeached) const
{
    if (isBeached) {
        const size_t index = static_cast<size_t>(rng.nextInt(static_cast<i32>(s_beachedTemplates.size())));
        return s_beachedTemplates[index];
    } else {
        const size_t index = static_cast<size_t>(rng.nextInt(static_cast<i32>(s_allTemplates.size())));
        return s_allTemplates[index];
    }
}

std::unique_ptr<StructureStart> ShipwreckStructure::generate(
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算基础位置（使用区块坐标转换）
    const i32 baseX = (chunkX << world::CHUNK_SHIFT) + rng.nextInt(world::CHUNK_WIDTH);
    const i32 baseZ = (chunkZ << world::CHUNK_SHIFT) + rng.nextInt(world::CHUNK_WIDTH);

    // 确定是否为搁浅沉船（检查是否在海滩生物群系）
    const BiomeId biome = generator.getBiome(baseX, 64, baseZ); // 使用固定高度检查生物群系
    const bool isBeached = (biome == Biomes::Beach || biome == Biomes::SnowyBeach);

    // 获取高度
    const HeightmapType heightmapType = isBeached ? HeightmapType::WorldSurfaceWG : HeightmapType::OceanFloorWG;
    i32 height = generator.getHeight(baseX, baseZ, heightmapType);

    // 根据是否搁浅调整高度
    // 搁浅沉船: height = minHeight - templateHeight/2 - random(0, 2)
    // 水下沉船: height = averageHeight
    // 这里简化处理，直接使用查询到的高度
    if (height <= 0) {
        height = generator.seaLevel() - 5;
    }

    // 随机旋转
    const Rotation rotation = static_cast<Rotation>(rng.nextInt(4));

    // 获取模板管理器
    feature::template_::TemplateManager* templateManager = m_templateManager;
    if (!templateManager) {
        templateManager = &jigsaw::JigsawAssembler::getTemplateManager();
    }

    // 选择随机模板
    const std::string templateName = _getRandomTemplateName(rng, isBeached);

    // 创建片段
    auto piece = std::make_unique<ShipwreckPiece>(templateName, BlockPos(baseX, height, baseZ), rotation, isBeached);
    piece->setTemplateManager(templateManager);

    start->addPiece(std::move(piece));
    start->recalculateStructureSize();
    return start;
}

} // namespace mc::world::gen::structure
