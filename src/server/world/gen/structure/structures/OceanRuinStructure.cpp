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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/Feature.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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
{
    // 包围盒必须在构造期按模板真实尺寸算好：`TemplateStructurePiece` 的构造就是这么做的，
    // 后续 move() 只平移盒子。等到 FEATURES 阶段才回填，会让 STRUCTURE_STARTS 阶段的
    // 每个构件都退化成一个点——整座废墟的整体包围盒、集群的重叠判定、差分输出全部失真。
    const feature::template_::Template* templ =
        jigsaw::JigsawAssembler::getTemplateManager().getTemplate(ResourceLocation(m_templateName));
    if (templ == nullptr) {
        return;
    }
    m_size = templ->getSize();
    const BlockPos cornerA =
        feature::template_::Template::transformBlockPos(BlockPos(0, 0, 0), Mirror::None, rotation, BlockPos(0, 0, 0));
    const BlockPos cornerB = feature::template_::Template::transformBlockPos(
        BlockPos(m_size.x - 1, m_size.y - 1, m_size.z - 1), Mirror::None, rotation, BlockPos(0, 0, 0));
    m_minX = position.x + std::min(cornerA.x, cornerB.x);
    m_minY = position.y + std::min(cornerA.y, cornerB.y);
    m_minZ = position.z + std::min(cornerA.z, cornerB.z);
    m_maxX = position.x + std::max(cornerA.x, cornerB.x);
    m_maxY = position.y + std::max(cornerA.y, cornerB.y);
    m_maxZ = position.z + std::max(cornerA.z, cornerB.z);
}

void OceanRuinPiece::_loadTemplate()
{
    if (!m_templateManager) {
        return;
    }

    ResourceLocation location(m_templateName);
    m_template = m_templateManager->getTemplate(location);

    if (m_template) {
        // 包围盒已在构造期按旋转铺好，这里只缓存尺寸给放置阶段用，
        // **不能**再用「尺寸 - 1」重算——那会把旋转后落在 pos 之前的那部分范围裁掉
        // （90/270 度时 X 落在 [-(size.z-1), 0]）。
        m_size = m_template->getSize();
    }
}

void OceanRuinPiece::generate(IWorldWriter& world,
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
    m_template->place(world,
        BlockPos(m_minX, m_minY, m_minZ),
        settings,
        rng,
        world::BlockUpdateFlags::UPDATE_CLIENTS | world::BlockUpdateFlags::UPDATE_KNOWN_SHAPE);
}

// ============================================================================
// OceanRuinStructure
// ============================================================================

OceanRuinStructure::OceanRuinStructure(ResourceLocation id)
    : Structure(std::move(id))
{}

const biome::BiomeTag* OceanRuinStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_OCEAN_RUIN_COLD();
}

bool OceanRuinStructure::canGenerate([[maybe_unused]] IWorld& world,
    [[maybe_unused]] IChunkGenerator& generator,
    math::IRandom& /*rng*/,
    [[maybe_unused]] i32 chunkX,
    [[maybe_unused]] i32 chunkZ)
{
    // 候选区块已由结构集放置器（间距 + frequency 缩减）确定；原版 OceanRuinStructure
    // 在候选区块上没有任何额外门槛。此前这里再抽一次 nextFloat() < 0.4，等于把频率
    // 又乘 0.4，同时白耗一次世界随机数，后续旋转/大型判定的随机序列整体错位。
    return true;
}

std::unique_ptr<StructureStart> OceanRuinStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 生成点固定为「区块最小角 + 90」：X/Z 不抽样（原版 generatePieces 直接用
    // chunkPos.getMinBlockX()/getMinBlockZ()），Y 的 90 只是占位，真实高度由落在
    // 片段包围盒上的海底高度图决定。
    const i32 baseX = chunkX * world::CHUNK_WIDTH;
    const i32 baseZ = chunkZ * world::CHUNK_WIDTH;
    const i32 anchorY = 90;

    // 确定废墟类型（根据生物群系）
    const i32 floorY = generator.getHeight(baseX, baseZ, HeightmapType::OceanFloorWG);
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

    // 随机数顺序不可调换：原版先抽「是否大型」，再在 OceanRuinPieces.addPieces 内部
    // 取旋转（Rotation.getRandom 消耗 1 次），最后才抽集群概率。
    const bool isLarge = rng.nextFloat() <= config.largeProbability;
    const f32 integrity = isLarge ? 0.9f : 0.8f;

    // 随机旋转
    const Rotation rotation = static_cast<Rotation>(rng.nextInt(4));

    // 创建片段列表
    std::vector<std::unique_ptr<StructurePiece>> pieces;

    // 生成主要片段
    generatePiece(*templateManager, BlockPos(baseX, floorY, baseZ), rotation, pieces, rng, config, isLarge, integrity);

    // 如果是大型废墟，可能生成集群。判据与原版一致：用**主废墟变换后的对角点 + 位置**
    // 构造主包围盒，候选簇与它相交就跳过（不生成、不消耗后续随机数）。
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
    math::IRandom& rng,
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
        // 抽取范围按**该分支自己的表长**，不能用小废墟的表长再取模：大型表只有 4 项，
        // 取模会把 big_brick_2/3/8 的选中概率抬到小废墟的两倍，与原版 nextInt(表长) 不符。
        const size_t index = isLarge ? static_cast<size_t>(rng.nextInt(static_cast<i32>(s_brickBigTemplates.size())))
                                     : static_cast<size_t>(rng.nextInt(static_cast<i32>(s_brickTemplates.size())));

        std::string brickTemplate, crackedTemplate, mossyTemplate;
        if (isLarge) {
            brickTemplate = s_brickBigTemplates[index];
            crackedTemplate = s_crackedBigTemplates[index];
            mossyTemplate = s_mossyBigTemplates[index];
        } else {
            brickTemplate = s_brickTemplates[index];
            crackedTemplate = s_crackedTemplates[index];
            mossyTemplate = s_mossyTemplates[index];
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
    math::IRandom& rng,
    [[maybe_unused]] Rotation mainRotation,
    const BlockPos& mainPos,
    const OceanRuinConfig& config,
    std::vector<std::unique_ptr<StructurePiece>>& pieces) const
{
    // 主废墟包围盒：以 (15, 0, 15) 的变换结果为另一角。小废墟用到的是 (5, 0, 6)。
    const BlockPos mainCornerA(mainPos.x, 90, mainPos.z);
    const BlockPos mainCornerB = feature::template_::Template::transformBlockPos(
        BlockPos(15, 0, 15), Mirror::None, mainRotation, BlockPos(0, 0, 0));
    const StructureBoundingBox mainBox(mainCornerA.x,
        mainCornerA.y,
        mainCornerA.z,
        mainPos.x + mainCornerB.x,
        mainCornerA.y,
        mainPos.z + mainCornerB.z);
    const BlockPos gridOrigin(
        std::min(mainCornerA.x, mainPos.x + mainCornerB.x), 90, std::min(mainCornerA.z, mainPos.z + mainCornerB.z));

    // 候选位置集合：**先一次性**生成全部 8 个（每个消耗 2 次随机数），再决定抽几个，
    // 顺序不能颠倒——原版 allPositions 在 nextInt(4, 8) 之前完成全部抽取。
    auto candidatePositions = getCandidatePositions(rng, gridOrigin.x, gridOrigin.z);

    const i32 count = rng.nextInt(4, 8);

    for (i32 i = 0; i < count; ++i) {
        if (candidatePositions.empty()) {
            break;
        }
        // 按当前列表长度抽取并移除（不是从固定 8 个里挑后跳过）
        const size_t index = static_cast<size_t>(rng.nextInt(static_cast<i32>(candidatePositions.size())));
        const BlockPos pos = candidatePositions[index];
        candidatePositions.erase(candidatePositions.begin() + static_cast<ptrdiff_t>(index));

        const Rotation rotation = static_cast<Rotation>(rng.nextInt(4));

        // 小废墟占用 (5, 0, 6) 的对角，与主废墟相交时整段跳过。
        const BlockPos smallCorner = feature::template_::Template::transformBlockPos(
            BlockPos(5, 0, 6), Mirror::None, rotation, BlockPos(0, 0, 0));
        const StructureBoundingBox smallBox(pos.x, pos.y, pos.z, pos.x + smallCorner.x, pos.y, pos.z + smallCorner.z);
        if (smallBox.intersects(mainBox)) {
            continue;
        }

        generatePiece(templateManager, pos, rotation, pieces, rng, config, false, 0.8f);
    }
}

std::vector<BlockPos> OceanRuinStructure::getCandidatePositions(math::IRandom& rng, i32 x, i32 z) const
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
