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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR THE DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "IglooStructure.hpp"

#include "common/core/Constants.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/feature/template/TemplateLoader.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/jigsaw/JigsawAssembler.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include <spdlog/spdlog.h>

namespace mc::world::gen::structure {

using namespace mc::Biomes;

// ============================================================================
// 静态常量
// ============================================================================

const std::string IglooStructure::s_name = "Igloo";

// 雪屋模板名称
const std::string IglooStructure::s_topTemplateName = "igloo/top";
const std::string IglooStructure::s_middleTemplateName = "igloo/middle";
const std::string IglooStructure::s_bottomTemplateName = "igloo/bottom";

// ============================================================================
// IglooPiece
// ============================================================================

IglooPiece::IglooPiece(const BlockPos& position, Rotation rotation, bool hasBasement, i32 middleCount)
    : StructurePiece(StructurePieceTypes::IGLOO, position.x, position.y, position.z, position.x, position.y, position.z)
    , m_rotation(rotation)
    , m_hasBasement(hasBasement)
    , m_middleCount(middleCount)
{}

void IglooPiece::_loadTemplates()
{
    if (!m_templateManager) {
        return;
    }

    // 加载地上部分模板
    m_topTemplate = m_templateManager->getTemplate(ResourceLocation(IglooStructure::s_topTemplateName));
    if (m_topTemplate) {
        m_topSize = m_topTemplate->getSize();
    }

    if (m_hasBasement) {
        // 加载中间层模板
        m_middleTemplate = m_templateManager->getTemplate(ResourceLocation(IglooStructure::s_middleTemplateName));
        if (m_middleTemplate) {
            m_middleSize = m_middleTemplate->getSize();
        }

        // 加载底部模板
        m_bottomTemplate = m_templateManager->getTemplate(ResourceLocation(IglooStructure::s_bottomTemplateName));
        if (m_bottomTemplate) {
            m_bottomSize = m_bottomTemplate->getSize();
        }
    }

    // 更新边界框
    _updateBoundingBox();
}

void IglooPiece::_updateBoundingBox()
{
    // igloo/top 的中心偏移是 BlockPos(3, 0, 5)
    // 使用 transformBlockPos 正确处理旋转变换
    BlockPos centerOffset(3, 0, 5);
    BlockPos transformedOffset =
        feature::template_::Template::transformBlockPos(centerOffset, Mirror::None, m_rotation, BlockPos(0, 0, 0));

    // 计算地上部分尺寸（考虑旋转）
    i32 topSizeX = m_topSize.x;
    i32 topSizeZ = m_topSize.z;
    if (m_rotation == Rotation::Clockwise90 || m_rotation == Rotation::CounterClockwise90) {
        std::swap(topSizeX, topSizeZ);
    }

    // 计算总高度
    i32 totalHeight = m_topSize.y;
    if (m_hasBasement) {
        totalHeight += m_middleSize.y * m_middleCount + m_bottomSize.y;
    }

    // 边界框从调整后的位置开始计算
    m_minX = m_minX - transformedOffset.x;
    m_minZ = m_minZ - transformedOffset.z;
    m_maxX = m_minX + topSizeX - 1;
    m_maxY = m_minY + totalHeight - 1;
    m_maxZ = m_minZ + topSizeZ - 1;

    // 如果有地下室，向下扩展边界框
    if (m_hasBasement && m_middleTemplate && m_bottomTemplate) {
        i32 basementHeight = m_middleSize.y * m_middleCount + m_bottomSize.y;
        m_minY = m_minY - basementHeight + m_topSize.y;
    }
}

void IglooPiece::generate(IWorldWriter& world,
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
    if (!m_topTemplate) {
        _loadTemplates();
    }

    if (!m_topTemplate) {
        spdlog::warn("IglooPiece: Failed to load top template");
        return;
    }

    // 检查边界框是否与区块相交
    StructureBoundingBox pieceBounds(m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ);
    if (!pieceBounds.intersects(chunkBounds)) {
        return;
    }

    // 生成地上部分
    _generateTop(world, rng, chunkBounds);

    // 生成地下室（如果有）
    if (m_hasBasement) {
        for (i32 i = 0; i < m_middleCount; ++i) {
            _generateMiddle(world, rng, i, chunkBounds);
        }
        _generateBottom(world, rng, chunkBounds);
    }
}

void IglooPiece::_generateTop(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds)
{
    if (!m_topTemplate) {
        return;
    }

    // 创建放置设置
    feature::template_::PlacementSettings settings;
    settings.setRotation(m_rotation);
    settings.setMirror(Mirror::None);
    settings.setBoundingBox(&chunkBounds);

    // igloo/top 模板的中心偏移是 BlockPos(3, 0, 5)
    // 使用 transformBlockPos 正确处理旋转变换
    BlockPos centerOffset(3, 0, 5);
    BlockPos transformedOffset =
        feature::template_::Template::transformBlockPos(centerOffset, Mirror::None, m_rotation, BlockPos(0, 0, 0));

    // 计算调整后的放置位置
    BlockPos adjustedPos(m_minX - transformedOffset.x, m_minY, m_minZ - transformedOffset.z);

    // 放置模板
    m_topTemplate->place(world, adjustedPos, settings, rng, 18);
}

void IglooPiece::_generateMiddle(
    IWorldWriter& world, math::Random& rng, i32 index, const StructureBoundingBox& chunkBounds)
{
    if (!m_middleTemplate) {
        return;
    }

    // 计算中间层位置（在地下部分）
    // 中间层偏移是 BlockPos(2, 0, 4)，每个中间层向下偏移 3 格
    i32 y = m_minY + m_topSize.y - 3 + index * (-3);

    feature::template_::PlacementSettings settings;
    settings.setRotation(m_rotation);
    settings.setMirror(Mirror::None);
    settings.setBoundingBox(&chunkBounds);

    // 使用 transformBlockPos 正确处理旋转变换
    BlockPos centerOffset(2, 0, 4);
    BlockPos transformedOffset =
        feature::template_::Template::transformBlockPos(centerOffset, Mirror::None, m_rotation, BlockPos(0, 0, 0));

    BlockPos adjustedPos(m_minX - transformedOffset.x, y, m_minZ - transformedOffset.z);

    m_middleTemplate->place(world, adjustedPos, settings, rng, 18);
}

void IglooPiece::_generateBottom(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds)
{
    if (!m_bottomTemplate) {
        return;
    }

    // 计算底部位置
    // 底部偏移是 BlockPos(3, 0, 7)
    i32 y = m_minY + m_topSize.y - 3 - m_middleSize.y * m_middleCount;

    feature::template_::PlacementSettings settings;
    settings.setRotation(m_rotation);
    settings.setMirror(Mirror::None);
    settings.setBoundingBox(&chunkBounds);

    // 使用 transformBlockPos 正确处理旋转变换
    BlockPos centerOffset(3, 0, 7);
    BlockPos transformedOffset =
        feature::template_::Template::transformBlockPos(centerOffset, Mirror::None, m_rotation, BlockPos(0, 0, 0));

    BlockPos adjustedPos(m_minX - transformedOffset.x, y, m_minZ - transformedOffset.z);

    m_bottomTemplate->place(world, adjustedPos, settings, rng, 18);
}

// ============================================================================
// IglooStructure
// ============================================================================

IglooStructure::IglooStructure()
    : Structure(ResourceLocation("minecraft", "igloo"))
{}

const biome::BiomeTag* IglooStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_IGLOO();
}

bool IglooStructure::canGenerate(
    IWorld& /*world*/, IChunkGenerator& generator, math::Random& /*rng*/, i32 chunkX, i32 chunkZ)
{
    // 检查区块中心位置的生物群系是否为雪地
    const BiomeId biome = generator.getBiome(chunkX * CHUNK_WIDTH + 8, 64, chunkZ * CHUNK_WIDTH + 8);
    return isValidBiome(biome);
}

std::unique_ptr<StructureStart> IglooStructure::generate(
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算生成位置
    i32 x = chunkX * world::CHUNK_WIDTH + rng.nextInt(world::CHUNK_WIDTH);
    i32 z = chunkZ * world::CHUNK_WIDTH + rng.nextInt(world::CHUNK_WIDTH);

    // 获取地表高度
    i32 y = generator.getHeight(x, z, HeightmapType::WorldSurface);

    // 随机旋转
    Rotation rotation = static_cast<Rotation>(rng.nextInt(4));

    // 雪屋有50%概率有地下室
    bool hasBasement = rng.nextFloat() < 0.5f;

    // 如果有地下室，随机 1-2 层中间层
    i32 middleCount = 0;
    if (hasBasement) {
        middleCount = 1 + rng.nextInt(2); // 1 或 2
    }

    // 获取模板管理器
    feature::template_::TemplateManager* templateManager = m_templateManager;
    if (!templateManager) {
        templateManager = &jigsaw::JigsawAssembler::getTemplateManager();
    }

    // 创建片段
    auto piece = std::make_unique<IglooPiece>(BlockPos(x, y, z), rotation, hasBasement, middleCount);
    piece->setTemplateManager(templateManager);

    start->addPiece(std::move(piece));
    start->recalculateStructureSize();
    return start;
}

} // namespace mc::world::gen::structure
