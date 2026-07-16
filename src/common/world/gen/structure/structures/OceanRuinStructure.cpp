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

#include "OceanRuinStructure.hpp"

#include "common/core/Constants.hpp"
#include "common/resource/ResourceLocation.hpp"
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
#include <algorithm>

namespace mc::world::gen::structure {

// ============================================================================
// 静态模板名称
// ============================================================================

const std::vector<std::string> OceanRuinStructure::s_warmTemplates = {"underwater_ruin/warm_1",
    "underwater_ruin/warm_2",
    "underwater_ruin/warm_3",
    "underwater_ruin/warm_4",
    "underwater_ruin/warm_5",
    "underwater_ruin/warm_6",
    "underwater_ruin/warm_7",
    "underwater_ruin/warm_8"};

const std::vector<std::string> OceanRuinStructure::s_warmBigTemplates = {"underwater_ruin/big_warm_4",
    "underwater_ruin/big_warm_5",
    "underwater_ruin/big_warm_6",
    "underwater_ruin/big_warm_7"};

const std::vector<std::string> OceanRuinStructure::s_brickTemplates = {"underwater_ruin/brick_1",
    "underwater_ruin/brick_2",
    "underwater_ruin/brick_3",
    "underwater_ruin/brick_4",
    "underwater_ruin/brick_5",
    "underwater_ruin/brick_6",
    "underwater_ruin/brick_7",
    "underwater_ruin/brick_8"};

const std::vector<std::string> OceanRuinStructure::s_brickBigTemplates = {"underwater_ruin/big_brick_1",
    "underwater_ruin/big_brick_2",
    "underwater_ruin/big_brick_3",
    "underwater_ruin/big_brick_8"};

const std::vector<std::string> OceanRuinStructure::s_crackedTemplates = {"underwater_ruin/cracked_1",
    "underwater_ruin/cracked_2",
    "underwater_ruin/cracked_3",
    "underwater_ruin/cracked_4",
    "underwater_ruin/cracked_5",
    "underwater_ruin/cracked_6",
    "underwater_ruin/cracked_7",
    "underwater_ruin/cracked_8"};

const std::vector<std::string> OceanRuinStructure::s_crackedBigTemplates = {"underwater_ruin/big_cracked_1",
    "underwater_ruin/big_cracked_2",
    "underwater_ruin/big_cracked_3",
    "underwater_ruin/big_cracked_8"};

const std::vector<std::string> OceanRuinStructure::s_mossyTemplates = {"underwater_ruin/mossy_1",
    "underwater_ruin/mossy_2",
    "underwater_ruin/mossy_3",
    "underwater_ruin/mossy_4",
    "underwater_ruin/mossy_5",
    "underwater_ruin/mossy_6",
    "underwater_ruin/mossy_7",
    "underwater_ruin/mossy_8"};

const std::vector<std::string> OceanRuinStructure::s_mossyBigTemplates = {"underwater_ruin/big_mossy_1",
    "underwater_ruin/big_mossy_2",
    "underwater_ruin/big_mossy_3",
    "underwater_ruin/big_mossy_8"};

// ============================================================================
// 常量
// ============================================================================

const std::string OceanRuinStructure::m_name = "ocean_ruin";

// ============================================================================
// OceanRuinPiece
// ============================================================================

OceanRuinPiece::OceanRuinPiece(const std::string& templateName,
    const BlockPos& position,
    Rotation rotation,
    f32 integrity,
    OceanRuinType type,
    bool isLarge)
    : StructurePiece(StructurePieceTypes::RUINED_PORTAL, // 复用类型 ID
          position.x,
          position.y,
          position.z,
          position.x,
          position.y,
          position.z)
    , m_templateName(templateName)
    , m_rotation(rotation)
    , m_integrity(integrity)
    , m_type(type)
    , m_isLarge(isLarge)
    , m_size(1, 1, 1)
{}

void OceanRuinPiece::_loadTemplate()
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

void OceanRuinPiece::generate(IWorldWriter& world,
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
    settings.setBoundingBox(&chunkBounds);

    // 添加完整度处理器
    feature::template_::StructureProcessorList processors;
    processors.addProcessor(std::make_unique<feature::template_::IntegrityProcessor>(m_integrity));
    // 添加空气忽略处理器
    std::vector<u32> blocksToIgnore;
    if (auto* airState = VanillaBlocks::getState(VanillaBlocks::AIR)) {
        blocksToIgnore.push_back(airState->blockId());
    }
    processors.addProcessor(std::make_unique<feature::template_::BlockIgnoreStructureProcessor>(blocksToIgnore));
    settings.setProcessors(&processors);

    // 放置模板
    m_template->place(world, BlockPos(m_minX, m_minY, m_minZ), settings, rng, 18);
}

// ============================================================================
// OceanRuinStructure
// ============================================================================

OceanRuinStructure::OceanRuinStructure()
    : Structure(ResourceLocation("minecraft", "ocean_ruin"))
{}

const biome::BiomeTag* OceanRuinStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_OCEAN_RUIN_COLD();
}

bool OceanRuinStructure::canGenerate([[maybe_unused]] IWorld& world,
    [[maybe_unused]] IChunkGenerator& generator,
    math::Random& rng,
    [[maybe_unused]] i32 chunkX,
    [[maybe_unused]] i32 chunkZ)
{
    return rng.nextFloat() < 0.4f;
}

std::unique_ptr<StructureStart> OceanRuinStructure::generate(
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算基础位置
    const i32 baseX = (chunkX << world::CHUNK_SHIFT) + rng.nextInt(world::CHUNK_WIDTH);
    const i32 baseZ = (chunkZ << world::CHUNK_SHIFT) + rng.nextInt(world::CHUNK_WIDTH);

    // 获取海底高度
    i32 floorY = generator.getHeight(baseX, baseZ, HeightmapType::OceanFloorWG);
    if (floorY <= 0) {
        floorY = generator.seaLevel() - 5;
    }

    // 确定废墟类型（根据生物群系）
    const BiomeId biome = generator.getBiome(baseX, floorY, baseZ);
    const bool warmVariant = _isWarmBiome(biome);

    // 更新配置
    OceanRuinConfig config = m_config;
    config.biomeType = warmVariant ? OceanRuinType::Warm : OceanRuinType::Cold;

    // 获取模板管理器
    feature::template_::TemplateManager* templateManager = m_templateManager;
    if (!templateManager) {
        templateManager = &jigsaw::JigsawAssembler::getTemplateManager();
    }

    // 确定是否生成大型废墟
    const bool isLarge = rng.nextFloat() <= config.largeProbability;
    const f32 integrity = isLarge ? 0.9f : 0.8f;

    // 随机旋转
    const Rotation rotation = static_cast<Rotation>(rng.nextInt(4));

    // 创建片段列表
    std::vector<std::unique_ptr<StructurePiece>> pieces;

    // 生成主要片段
    generatePiece(*templateManager, BlockPos(baseX, floorY, baseZ), rotation, pieces, rng, config, isLarge, integrity);

    // 如果是大型废墟，可能生成集群
    if (isLarge && rng.nextFloat() <= config.clusterProbability) {
        generateClusterPieces(*templateManager, rng, rotation, BlockPos(baseX, floorY, baseZ), config, pieces);
    }

    // 将片段添加到 start
    for (auto& piece : pieces) {
        start->addPiece(std::move(piece));
    }

    start->recalculateStructureSize();
    return start;
}

void OceanRuinStructure::generatePiece(feature::template_::TemplateManager& templateManager,
    const BlockPos& pos,
    Rotation rotation,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    const OceanRuinConfig& config,
    bool isLarge,
    f32 integrity) const
{
    // 根据类型选择模板
    std::string templateName;

    if (config.biomeType == OceanRuinType::Warm) {
        // 暖海废墟
        if (isLarge) {
            const size_t index = static_cast<size_t>(rng.nextInt(static_cast<i32>(s_warmBigTemplates.size())));
            templateName = s_warmBigTemplates[index];
        } else {
            const size_t index = static_cast<size_t>(rng.nextInt(static_cast<i32>(s_warmTemplates.size())));
            templateName = s_warmTemplates[index];
        }

        auto piece =
            std::make_unique<OceanRuinPiece>(templateName, pos, rotation, integrity, config.biomeType, isLarge);
        piece->setTemplateManager(&templateManager);
        pieces.push_back(std::move(piece));
    } else {
        // 冷海废墟 - 生成三层叠加（砖、裂纹、苔藓）
        const i32 index = rng.nextInt(static_cast<i32>(s_brickTemplates.size()));

        std::string brickTemplate, crackedTemplate, mossyTemplate;
        if (isLarge) {
            brickTemplate = s_brickBigTemplates[static_cast<size_t>(index) % s_brickBigTemplates.size()];
            crackedTemplate = s_crackedBigTemplates[static_cast<size_t>(index) % s_crackedBigTemplates.size()];
            mossyTemplate = s_mossyBigTemplates[static_cast<size_t>(index) % s_mossyBigTemplates.size()];
        } else {
            brickTemplate = s_brickTemplates[static_cast<size_t>(index)];
            crackedTemplate = s_crackedTemplates[static_cast<size_t>(index)];
            mossyTemplate = s_mossyTemplates[static_cast<size_t>(index)];
        }

        // 生成三层叠加，不同完整度
        auto brickPiece =
            std::make_unique<OceanRuinPiece>(brickTemplate, pos, rotation, integrity, config.biomeType, isLarge);
        brickPiece->setTemplateManager(&templateManager);
        pieces.push_back(std::move(brickPiece));

        auto crackedPiece =
            std::make_unique<OceanRuinPiece>(crackedTemplate, pos, rotation, 0.7f, config.biomeType, isLarge);
        crackedPiece->setTemplateManager(&templateManager);
        pieces.push_back(std::move(crackedPiece));

        auto mossyPiece =
            std::make_unique<OceanRuinPiece>(mossyTemplate, pos, rotation, 0.5f, config.biomeType, isLarge);
        mossyPiece->setTemplateManager(&templateManager);
        pieces.push_back(std::move(mossyPiece));
    }
}

void OceanRuinStructure::generateClusterPieces(feature::template_::TemplateManager& templateManager,
    math::Random& rng,
    [[maybe_unused]] Rotation mainRotation,
    const BlockPos& mainPos,
    const OceanRuinConfig& config,
    std::vector<std::unique_ptr<StructurePiece>>& pieces) const
{
    // 生成周围的小废墟群
    // 计算主废墟的变换后角落位置
    const i32 mainX = mainPos.x;
    const i32 mainZ = mainPos.z;

    // 获取候选位置
    auto candidatePositions = getCandidatePositions(rng, mainX, mainZ);

    // 生成 4-8 个小废墟
    const i32 count = rng.nextInt(4, 8);

    for (i32 i = 0; i < count && !candidatePositions.empty(); ++i) {
        // 随机选择一个位置
        const size_t index = static_cast<size_t>(rng.nextInt(static_cast<i32>(candidatePositions.size())));
        BlockPos pos = candidatePositions[index];
        candidatePositions.erase(candidatePositions.begin() + static_cast<ptrdiff_t>(index));

        // 随机旋转
        const Rotation rotation = static_cast<Rotation>(rng.nextInt(4));

        // 生成小废墟（完整度 0.8）
        generatePiece(templateManager, pos, rotation, pieces, rng, config, false, 0.8f);
    }
}

std::vector<BlockPos> OceanRuinStructure::getCandidatePositions(math::Random& rng, i32 x, i32 z) const
{
    // 生成 8 个候选位置，围绕主废墟
    std::vector<BlockPos> positions;
    positions.reserve(8);

    // Y 坐标固定为 90（后续会调整到海床）
    const i32 y = 90;

    // 北侧偏移
    positions.emplace_back(x - 16 + rng.nextInt(1, 8), y, z + 16 + rng.nextInt(1, 7));
    positions.emplace_back(x - 16 + rng.nextInt(1, 8), y, z + rng.nextInt(1, 7));
    positions.emplace_back(x - 16 + rng.nextInt(1, 8), y, z - 16 + rng.nextInt(4, 8));

    // 中间偏移
    positions.emplace_back(x + rng.nextInt(1, 7), y, z + 16 + rng.nextInt(1, 7));
    positions.emplace_back(x + rng.nextInt(1, 7), y, z - 16 + rng.nextInt(4, 6));

    // 南侧偏移
    positions.emplace_back(x + 16 + rng.nextInt(1, 7), y, z + 16 + rng.nextInt(3, 8));
    positions.emplace_back(x + 16 + rng.nextInt(1, 7), y, z + rng.nextInt(1, 7));
    positions.emplace_back(x + 16 + rng.nextInt(1, 7), y, z - 16 + rng.nextInt(4, 8));

    return positions;
}

bool OceanRuinStructure::_isWarmBiome(BiomeId biomeId) const noexcept
{
    return biomeId == Biomes::WarmOcean || biomeId == Biomes::LukewarmOcean || biomeId == Biomes::DeepWarmOcean ||
        biomeId == Biomes::DeepLukewarmOcean;
}

} // namespace mc::world::gen::structure
