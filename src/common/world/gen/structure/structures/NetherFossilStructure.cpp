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

#include "NetherFossilStructure.hpp"

#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../feature/template/Template.hpp"
#include "../../feature/template/TemplateLoader.hpp"
#include "../../feature/template/TemplateManager.hpp"
#include "../../jigsaw/JigsawManager.hpp"
#include "../StructureBoundingBox.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc::world::gen::structure {

using namespace mc::Biomes;

// ============================================================================
// 静态常量
// ============================================================================

const std::string NetherFossilStructure::s_name = "Nether_Fossil";

// MC 1.16.5: 下界化石模板名称（共14个）
const std::vector<std::string> NetherFossilStructure::s_fossilTemplates = {"nether_fossils/fossil_1",
    "nether_fossils/fossil_2",
    "nether_fossils/fossil_3",
    "nether_fossils/fossil_4",
    "nether_fossils/fossil_5",
    "nether_fossils/fossil_6",
    "nether_fossils/fossil_7",
    "nether_fossils/fossil_8",
    "nether_fossils/fossil_9",
    "nether_fossils/fossil_10",
    "nether_fossils/fossil_11",
    "nether_fossils/fossil_12",
    "nether_fossils/fossil_13",
    "nether_fossils/fossil_14"};

const std::vector<BiomeId> NetherFossilStructure::s_validBiomes = {SoulSandValley};

// ============================================================================
// NetherFossilPiece
// ============================================================================

NetherFossilPiece::NetherFossilPiece(const std::string& templateName, const BlockPos& position, Rotation rotation)
    : StructurePiece(
          StructurePieceTypes::NETHER_FOSSIL, position.x, position.y, position.z, position.x, position.y, position.z)
    , m_templateName(templateName)
    , m_rotation(rotation)
    , m_size(1, 1, 1)
{}

void NetherFossilPiece::loadTemplate()
{
    if (!m_templateManager) {
        return;
    }

    ResourceLocation location(m_templateName);
    m_template = m_templateManager->getTemplate(location);

    if (m_template) {
        m_size = m_template->getSize();
        // 更新边界框（考虑旋转）
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

void NetherFossilPiece::generate(
    IWorldWriter& world, math::Random& rng, i32 /*chunkX*/, i32 /*chunkZ*/, const StructureBoundingBox& chunkBounds)
{
    if (!m_templateManager) {
        return;
    }

    // 延迟加载模板
    if (!m_template) {
        loadTemplate();
    }

    if (!m_template) {
        spdlog::warn("NetherFossilPiece: Failed to load template: {}", m_templateName);
        return;
    }

    // 创建放置设置
    feature::template_::PlacementSettings settings;
    settings.setRotation(m_rotation);
    settings.setMirror(Mirror::None);
    settings.setBoundingBox(&chunkBounds);

    // 放置模板
    m_template->place(world, BlockPos(m_minX, m_minY, m_minZ), settings, rng, 18);
}

// ============================================================================
// NetherFossilStructure
// ============================================================================

NetherFossilStructure::NetherFossilStructure()
    : Structure(StructureType::Temple)
{}

bool NetherFossilStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(rng);

    // 检查生物群系
    BiomeId biome = generator.getBiome(chunkX * 16 + 8, 64, chunkZ * 16 + 8);
    for (BiomeId valid : s_validBiomes) {
        if (biome == valid) {
            return true;
        }
    }
    return false;
}

std::unique_ptr<StructureStart> NetherFossilStructure::generate(
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    MC_UNUSED(world);

    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算生成位置
    i32 x = chunkX * 16 + rng.nextInt(16);
    i32 z = chunkZ * 16 + rng.nextInt(16);

    // MC 1.16.5: 下界化石在 Y=30-60 之间生成
    // 先选择一个高度范围，然后向下查找合适的位置
    i32 startY = 30 + rng.nextInt(30);
    i32 y = startY;

    // MC 1.16.5: 向下查找灵魂沙上方的空气位置
    // 这里简化处理，直接使用随机高度
    // TODO: 完整实现应该向下查找空气位置

    // 随机旋转
    Rotation rotation = static_cast<Rotation>(rng.nextInt(4));

    // MC 1.16.5: 随机选择一个化石模板（共14个）
    const size_t templateIndex = static_cast<size_t>(rng.nextInt(static_cast<i32>(s_fossilTemplates.size())));
    const std::string templateName = s_fossilTemplates[templateIndex];

    // 获取模板管理器
    feature::template_::TemplateManager* templateManager = m_templateManager;
    if (!templateManager) {
        templateManager = &jigsaw::JigsawManager::getTemplateManager();
    }

    // 创建片段
    auto piece = std::make_unique<NetherFossilPiece>(templateName, BlockPos(x, y, z), rotation);
    piece->setTemplateManager(templateManager);

    start->addPiece(std::move(piece));
    start->recalculateStructureSize();
    return start;
}

} // namespace mc::world::gen::structure
