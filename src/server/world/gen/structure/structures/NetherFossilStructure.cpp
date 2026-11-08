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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/chunk/NoiseColumn.hpp"
#include "server/world/gen/feature/Feature.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/base/StructureBoundingBox.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::world::gen::structure {

using namespace mc::Biomes;

// ============================================================================
// 静态常量
// ============================================================================

const std::string NetherFossilStructure::s_name = "Nether_Fossil";

// 下界化石模板名称（共14个）
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

void NetherFossilPiece::_loadTemplate()
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

void NetherFossilPiece::generate(IWorldWriter& world,
    math::IRandom& rng,
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
        spdlog::warn("NetherFossilPiece: Failed to load template: {}", m_templateName);
        return;
    }

    // 创建放置设置
    feature::template_::PlacementSettings settings;
    settings.setRotation(m_rotation);
    settings.setMirror(Mirror::None);
    settings.setBoundingBox(&chunkBounds);

    // 放置模板
    m_template->place(world,
        BlockPos(m_minX, m_minY, m_minZ),
        settings,
        rng,
        world::BlockUpdateFlags::UPDATE_CLIENTS | world::BlockUpdateFlags::UPDATE_KNOWN_SHAPE);
}

// ============================================================================
// NetherFossilStructure
// ============================================================================

NetherFossilStructure::NetherFossilStructure(ResourceLocation id, std::unique_ptr<valueprovider::HeightProvider> height)
    : Structure(std::move(id))
    , m_height(std::move(height))
{
    MC_ASSERT_RELEASE_MSG(m_height != nullptr, "nether_fossil requires a height provider from the data pack");
}

const biome::BiomeTag* NetherFossilStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_NETHER_FOSSIL();
}

std::unique_ptr<StructureStart> NetherFossilStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = makeStructureStart(chunkX, chunkZ);

    // 原版 `NetherFossilStructure.findGenerationPoint`：先在区块内随机挑一列，再沿着这一列
    // 自上而下找「空气 + 下方可承重」的第一处，作为化石骨架的落点。
    const i32 x = chunkX * world::CHUNK_WIDTH + rng.nextInt(world::CHUNK_WIDTH);
    const i32 z = chunkZ * world::CHUNK_WIDTH + rng.nextInt(world::CHUNK_WIDTH);
    const i32 seaLevel = generator.seaLevel();
    const valueprovider::WorldGenerationContext heightContext(generator.getMinY(), generator.getGenDepth());
    i32 y = m_height->sample(rng, heightContext);

    const NoiseColumn column = generator.getBaseColumn(x, z);
    while (y > seaLevel) {
        const BlockState* current = column.getBlock(y);
        const BlockState* below = column.getBlock(--y);
        const bool currentAir = current == nullptr || current->isAir();
        // 原版 `below.is(Blocks.SOUL_SAND) || below.isFaceSturdy(EmptyBlockGetter, pos, UP)`：
        // FULL 支撑判定只读方块自身的支撑形状，与所在世界无关，因此不需要真的构造一个空世界。
        const bool belowSturdy = below != nullptr &&
            (below->is(VanillaBlocks::SOUL_SAND) ||
                Block::isFaceFull(below->getBlockSupportShapeAt(BlockPos(x, y, z)), Direction::Up));
        if (currentAir && belowSturdy) {
            break;
        }
    }
    if (y <= seaLevel) {
        return start;
    }

    // 生物群系校验落在候选生成点上（原版 `Structure.findValidGenerationPoint` 即在
    // GenerationStub 位置采样），不是区块最小角。
    const BlockPos candidate(x, y, z);
    if (!isValidBiomeAt(generator, candidate)) {
        return start;
    }

    // 随机旋转
    Rotation rotation = static_cast<Rotation>(rng.nextInt(4));

    // 随机选择一个化石模板（共14个）
    const size_t templateIndex = static_cast<size_t>(rng.nextInt(static_cast<i32>(s_fossilTemplates.size())));
    const std::string templateName = s_fossilTemplates[templateIndex];

    // 获取模板管理器
    feature::template_::TemplateManager* templateManager = m_templateManager;
    if (!templateManager) {
        templateManager = &jigsaw::JigsawAssembler::getTemplateManager();
    }

    // 创建片段
    auto piece = std::make_unique<NetherFossilPiece>(templateName, BlockPos(x, y, z), rotation);
    piece->setTemplateManager(templateManager);

    start->addPiece(std::move(piece));
    start->recalculateStructureSize();
    return start;
}

} // namespace mc::world::gen::structure
