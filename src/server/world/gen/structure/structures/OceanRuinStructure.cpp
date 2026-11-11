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
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/tags/BlockTags.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/core/LootableContainerBlockEntity.hpp"
#include "common/world/blockentity/interactive/BrushableBlockEntity.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/Feature.hpp"
#include "server/world/gen/feature/template/CappedStructureProcessor.hpp"
#include "server/world/gen/feature/template/RuleTest.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/base/StructureBoundingBox.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
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
// 片段模板选择
// ============================================================================

namespace {

/**
 * @brief 按风格与大小生成一组废墟片段（每层一个，完整度依次递减）
 *
 * 暖海只有一层；冷海是砖 / 裂纹 / 苔藓三层叠在同一个位置。抽取顺序与层数都是原版
 * `OceanRuinPieces.addPiece` 的一部分：集群里的小废墟也要走这里，随机数序列必须同源。
 */
void appendRuinPieces(feature::template_::TemplateManager& templateManager,
    const BlockPos& pos,
    Rotation rotation,
    math::IRandom& rng,
    const OceanRuinConfig& config,
    bool isLarge,
    f32 integrity,
    std::vector<std::unique_ptr<StructurePiece>>& pieces)
{
    const auto make = [&](const std::string& templateName, f32 pieceIntegrity) {
        auto piece =
            std::make_unique<OceanRuinPiece>(templateName, pos, rotation, pieceIntegrity, config.biomeType, isLarge);
        piece->setTemplateManager(&templateManager);
        pieces.push_back(std::move(piece));
    };

    if (config.biomeType == OceanRuinType::Warm) {
        const auto& table = isLarge ? OceanRuinStructure::s_warmBigTemplates : OceanRuinStructure::s_warmTemplates;
        make(table[static_cast<size_t>(rng.nextInt(static_cast<i32>(table.size())))], integrity);
        return;
    }

    // 冷海：抽取范围按**该分支自己的表长**，不能用小废墟的表长再取模。
    const auto& bricks = isLarge ? OceanRuinStructure::s_brickBigTemplates : OceanRuinStructure::s_brickTemplates;
    const auto& cracked = isLarge ? OceanRuinStructure::s_crackedBigTemplates : OceanRuinStructure::s_crackedTemplates;
    const auto& mossy = isLarge ? OceanRuinStructure::s_mossyBigTemplates : OceanRuinStructure::s_mossyTemplates;
    MC_ASSERT_RELEASE(bricks.size() == cracked.size() && bricks.size() == mossy.size());
    const size_t index = static_cast<size_t>(rng.nextInt(static_cast<i32>(bricks.size())));
    make(bricks[index], integrity);
    make(cracked[index], 0.7f);
    make(mossy[index], 0.5f);
}

std::vector<BlockPos> getCandidatePositions(math::IRandom& rng, i32 x, i32 z)
{
    // 生成 8 个候选位置，围绕主废墟
    std::vector<BlockPos> positions;
    positions.reserve(8);

    // Y 坐标固定为 90（后续会调整到海床）
    const i32 y = 90;

    // 【抽取顺序】每次 `offset(dx, 0, dz)` 的两个分量在 Java 里严格从左到右求值，
    // 而 C++ 同一表达式中两次调用的求值顺序是**未指定**的（clang 实际从右往左），
    // 直接写成 `emplace_back(x + rng.nextInt(...), y, z + rng.nextInt(...))` 会让 X/Z 的
    // 抽样顺序颠倒，整组候选位置与后续随机流随之错位。这里逐项取出中间量。
    const auto at = [&](i32 offsetX, i32 offsetZ, i32 xMin, i32 xMax, i32 zMin, i32 zMax) {
        const i32 dx = rng.nextInt(xMin, xMax);
        const i32 dz = rng.nextInt(zMin, zMax);
        positions.emplace_back(x + offsetX + dx, y, z + offsetZ + dz);
    };

    // 北侧偏移
    at(-16, 16, 1, 8, 1, 7);
    at(-16, 0, 1, 8, 1, 7);
    at(-16, -16, 1, 8, 4, 8);

    // 中间偏移
    at(0, 16, 1, 7, 1, 7);
    at(0, -16, 1, 7, 4, 6);

    // 南侧偏移
    at(16, 16, 1, 7, 3, 8);
    at(16, 0, 1, 7, 1, 7);
    at(16, -16, 1, 7, 4, 8);

    return positions;
}

void generateClusterPieces(feature::template_::TemplateManager& templateManager,
    math::IRandom& rng,
    Rotation mainRotation,
    const BlockPos& mainPos,
    const OceanRuinConfig& config,
    std::vector<std::unique_ptr<StructurePiece>>& pieces)
{
    // 主废墟包围盒：以 (15, 0, 15) 的变换结果为另一角（`BoundingBox.fromCorners` 会自己
    // 排好 min/max，旋转到 90/270 度时另一角会落到负方向，不能假设它就是 max）。
    const BlockPos mainCornerA(mainPos.x, 90, mainPos.z);
    const BlockPos mainCornerB = feature::template_::Template::transformBlockPos(
        BlockPos(15, 0, 15), Mirror::None, mainRotation, BlockPos(0, 0, 0));
    const StructureBoundingBox mainBox(std::min(mainCornerA.x, mainPos.x + mainCornerB.x),
        mainCornerA.y,
        std::min(mainCornerA.z, mainPos.z + mainCornerB.z),
        std::max(mainCornerA.x, mainPos.x + mainCornerB.x),
        mainCornerA.y,
        std::max(mainCornerA.z, mainPos.z + mainCornerB.z));
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
        const StructureBoundingBox smallBox(std::min(pos.x, pos.x + smallCorner.x),
            pos.y,
            std::min(pos.z, pos.z + smallCorner.z),
            std::max(pos.x, pos.x + smallCorner.x),
            pos.y,
            std::max(pos.z, pos.z + smallCorner.z));
        if (smallBox.intersects(mainBox)) {
            continue;
        }

        appendRuinPieces(templateManager, pos, rotation, rng, config, false, 0.8f, pieces);
    }
}

} // namespace

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
    , m_templateName(ResourceLocation(templateName).toString())
    , m_integrity(integrity)
    , m_type(type)
    , m_isLarge(isLarge)
    , m_size(1, 1, 1)
{
    // 模板标识用资源位置的完整写法（`minecraft:underwater_ruin/brick_6`）：原版
    // `TemplateStructurePiece.templateName` 存的是 `Identifier.toString()`，字段本身
    // 是片段的对外标识，不是内部查找键。
    m_rotation = rotation;

    // 包围盒必须在构造期按模板真实尺寸算好：`TemplateStructurePiece` 的构造就是这么做的，
    // 后续 move() 只平移盒子。等到 FEATURES 阶段才回填，会让 STRUCTURE_STARTS 阶段的
    // 每个构件都退化成一个点——整座废墟的整体包围盒、集群的重叠判定、差分输出全部失真。
    const feature::template_::Template* templ =
        jigsaw::JigsawAssembler::getTemplateManager().getTemplate(ResourceLocation(m_templateName));
    if (templ == nullptr) {
        return;
    }
    m_size = templ->getSize();
    m_templatePosition = position;
    // 旋转枢轴为模板原点（原版 `OceanRuinPiece.makeSettings` 不设枢轴）。
    feature::template_::PlacementSettings settings;
    settings.setRotation(rotation);
    settings.setMirror(Mirror::None);
    const StructureBoundingBox rect = templ->getBoundingBox(settings, BlockPos(0, 0, 0));
    m_minX = position.x + rect.minX();
    m_minY = position.y + rect.minY();
    m_minZ = position.z + rect.minZ();
    m_maxX = position.x + rect.maxX();
    m_maxY = position.y + rect.maxY();
    m_maxZ = position.z + rect.maxZ();
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

i32 OceanRuinPiece::_scanHeight(const BlockPos& start, const BlockPos& end, IWorld& world)
{
    BlockTag* iceTag = BlockTags::getTag(ResourceLocation("minecraft:ice"));
    const i32 minY = world.getMinBuildHeight();

    const i32 startY = start.y;
    i32 lowest = 512;
    const i32 headingY = startY - 1;
    i32 deepColumns = 0;

    const i32 minX = std::min(start.x, end.x);
    const i32 maxX = std::max(start.x, end.x);
    const i32 minZ = std::min(start.z, end.z);
    const i32 maxZ = std::max(start.z, end.z);

    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 z = minZ; z <= maxZ; ++z) {
            i32 y = headingY;
            // 和原版一致：空气、水、冰都算「未到底」，一直扫到世界底部附近。
            while (y > minY + 1) {
                const BlockState* state = world.getBlockState(x, y, z);
                const fluid::FluidState* fluid = world.getFluidState(x, y, z);
                const bool isAir = state == nullptr || state->isAir();
                const bool isWater = fluid != nullptr && fluid->getFluid().isIn(fluid::FluidTags::WATER());
                const bool isIce = iceTag != nullptr && state != nullptr && iceTag->contains(*state);
                if (!isAir && !isWater && !isIce) {
                    break;
                }
                --y;
            }
            lowest = std::min(lowest, y);
            if (y < headingY - 2) {
                ++deepColumns;
            }
        }
    }

    const i32 spanX = std::abs(start.x - end.x);
    if (headingY - lowest > 2 && deepColumns > spanX - 2) {
        return lowest + 1;
    }
    return startY;
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

    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "OceanRuinPiece::generate requires an IWorld");

    // 贴地：起点阶段的 Y 只是占位，先按海底高度图落到海床，再按模板足迹的水深修正。
    // `IWorld::getHeight` 返回最高方块 Y，原版 `LevelReader.getHeight` 返回首个可放置 Y
    // （最高方块 Y + 1），模板原点按原版语义要落在海床上方一格。
    m_templatePosition.y =
        reader->getHeight(m_templatePosition.x, m_templatePosition.z, HeightmapType::OceanFloorWG) + 1;
    const BlockPos farCorner = feature::template_::Template::transformBlockPos(
        BlockPos(m_size.x - 1, 0, m_size.z - 1), Mirror::None, getRotation(), BlockPos(0, 0, 0));
    const BlockPos scanEnd(
        m_templatePosition.x + farCorner.x, m_templatePosition.y, m_templatePosition.z + farCorner.z);
    m_templatePosition.y = _scanHeight(m_templatePosition, scanEnd, *reader);

    // 创建放置设置
    feature::template_::PlacementSettings settings;
    settings.setRotation(getRotation());
    settings.setMirror(Mirror::None);
    settings.setBoundingBox(&chunkBounds);
    // CappedStructureProcessor 的随机源取世界种子，必须提供读取器
    settings.setWorld(reader);

    // 添加完整度处理器
    feature::template_::StructureProcessorList processors;
    processors.addProcessor(std::make_unique<feature::template_::IntegrityProcessor>(m_integrity));
    // 原版 `BlockIgnoreProcessor.STRUCTURE_AND_AIR`：结构方块（含 DATA 标记）与空气都不放置。
    std::vector<u32> blocksToIgnore;
    if (auto* airState = VanillaBlocks::getState(VanillaBlocks::AIR)) {
        blocksToIgnore.push_back(airState->blockId());
    }
    if (auto* structureState = VanillaBlocks::getState(VanillaBlocks::STRUCTURE_BLOCK)) {
        blocksToIgnore.push_back(structureState->blockId());
    }
    processors.addProcessor(std::make_unique<feature::template_::BlockIgnoreStructureProcessor>(blocksToIgnore));

    // 原版 `COLD/WARM_SUSPICIOUS_BLOCK_PROCESSOR`：每座废墟最多把 5 个沙砾（冷）或
    // 沙子（暖）替换成可疑方块。战利品表在原版由 `AppendLoot` 方块实体修饰器写入
    // （LootTable + LootTableSeed），本结构在放置完成后统一补挂（见下方 _attachArchaeologyLoot）。
    {
        const Block* sourceBlock = m_type == OceanRuinType::Cold ? VanillaBlocks::GRAVEL : VanillaBlocks::SAND;
        const Block* outputBlock =
            m_type == OceanRuinType::Cold ? VanillaBlocks::SUSPICIOUS_GRAVEL : VanillaBlocks::SUSPICIOUS_SAND;
        std::vector<std::unique_ptr<feature::template_::RuleEntry>> rules;
        rules.push_back(std::make_unique<feature::template_::RuleEntry>(
            std::make_unique<feature::template_::BlockMatchRuleTest>(sourceBlock),
            std::make_unique<feature::template_::AlwaysTrueRuleTest>(),
            outputBlock->defaultState().stateId()));
        processors.addProcessor(std::make_unique<feature::template_::CappedStructureProcessor>(
            std::make_unique<feature::template_::RuleStructureProcessor>(std::move(rules)), 5));
    }
    settings.setProcessors(&processors);

    // 放置模板：完整版会在放完后按邻居重算方块形状，并按世界流体补上水浸状态。
    if (!m_template->placeInWorld(
            *reader, m_templatePosition, settings, rng, world::BlockUpdateFlags::UPDATE_CLIENTS)) {
        return;
    }

    // 给刚被替换出来的可疑方块补挂考古战利品表。
    _attachArchaeologyLoot(*reader, rng, settings);

    // 原版 `TemplateStructurePiece#postProcess`：放完模板后再处理 DATA 标记，最后把拼图
    // 方块按 `final_state` 覆盖。
    for (const auto& marker : m_template->getDataMarkers(m_templatePosition, settings)) {
        handleDataMarker(marker.metadata, marker.pos, *reader, rng, chunkBounds);
    }
    m_template->applyJigsawReplacements(*reader, m_templatePosition, settings);
}

void OceanRuinPiece::_attachArchaeologyLoot(
    IWorld& world, math::IRandom& rng, const feature::template_::PlacementSettings& settings)
{
    // 原版用 `AppendLoot` 方块实体修饰器在放置期给可疑方块写 LootTable/LootTableSeed。
    // 本项目没有方块实体修饰器层（IWorld::setBlockEntity 是显式调用，不会随
    // setBlockState 自动创建），所以在模板放置完成后扫一遍模板包围盒，
    // 给其中的可疑方块建 BrushableBlockEntity 并挂表。
    // 海洋废墟单个片段很小，扫描范围就是模板自身的包围盒，代价可忽略。
    const StructureBoundingBox box = m_template->getBoundingBox(settings, m_templatePosition);

    const Block* suspiciousGravel = VanillaBlocks::SUSPICIOUS_GRAVEL;
    const Block* suspiciousSand = VanillaBlocks::SUSPICIOUS_SAND;
    if (suspiciousGravel == nullptr || suspiciousSand == nullptr) {
        spdlog::warn("OceanRuinPiece: suspicious blocks are not registered; archaeology loot skipped");
        return;
    }

    // 冷废墟的沙砾换成可疑沙砾、暖废墟的沙子换成可疑沙子，两者战利品表不同。
    static const ResourceLocation coldLoot("minecraft:archaeology/ocean_ruin_cold");
    static const ResourceLocation warmLoot("minecraft:archaeology/ocean_ruin_warm");
    const ResourceLocation& lootTable = m_type == OceanRuinType::Cold ? coldLoot : warmLoot;

    for (i32 y = box.minY(); y <= box.maxY(); ++y) {
        for (i32 z = box.minZ(); z <= box.maxZ(); ++z) {
            for (i32 x = box.minX(); x <= box.maxX(); ++x) {
                const BlockState* state = world.getBlockState(x, y, z);
                if (state == nullptr) {
                    continue;
                }
                if (!state->is(suspiciousGravel) && !state->is(suspiciousSand)) {
                    continue;
                }
                const BlockPos pos(x, y, z);
                // 已有方块实体就跳过（重复放置同一片段时不覆盖既有内容）。
                if (world.getBlockEntity(pos) != nullptr) {
                    continue;
                }
                auto entity = std::make_unique<blockentity::BrushableBlockEntity>(pos);
                entity->setLootTable(lootTable, rng.nextLong());
                world.setBlockEntity(pos, std::move(entity));
            }
        }
    }
}

void OceanRuinPiece::handleDataMarker(const std::string& metadata,
    const BlockPos& pos,
    IWorld& world,
    math::IRandom& rng,
    const StructureBoundingBox& /*chunkBounds*/)
{
    if (metadata == "chest") {
        const fluid::FluidState* fluid = world.getFluidState(pos);
        const bool waterlogged = fluid != nullptr && fluid->getFluid().isIn(fluid::FluidTags::WATER());
        const BlockState chest =
            VanillaBlocks::CHEST->defaultState().with(BlockStateProperties::WATERLOGGED(), waterlogged);
        world.setBlockState(pos.x, pos.y, pos.z, &chest, 2);

        // 原版 `RandomizableContainer.setBlockEntityLootTable`：只有方块实体确实是战利品容器
        // 时才消耗随机数并挂表。
        BlockEntity* blockEntity = world.getBlockEntity(pos);
        auto* container = dynamic_cast<blockentity::LootableContainerBlockEntity*>(blockEntity);
        if (container != nullptr) {
            static const ResourceLocation lootTable("minecraft:chests/underwater_ruin_big");
            static const ResourceLocation smallLootTable("minecraft:chests/underwater_ruin_small");
            container->setLootTable(m_isLarge ? lootTable : smallLootTable, rng.nextLong());
        }
        return;
    }

    if (metadata != "drowned") {
        return;
    }

    // 原版生成溺尸后按海平面把标记方块替换成空气或水。
    ecs::EntityRegistry* registry = world.entityRegistry();
    if (registry == nullptr) {
        return;
    }
    // 经实体类型注册表创建：`EntityType::create` 会写入 typeId，直接调用
    // `DrownedEntity::create` 得到的实体没有类型标识，网络同步与战利品表都取不到类型。
    const entity::EntityType* drownedType = entity::EntityRegistry::instance().getType(entity::EntityTypeKeys::DROWNED);
    if (drownedType == nullptr) {
        return;
    }
    auto entity = drownedType->create(&world, *registry);
    if (entity == nullptr) {
        return;
    }
    auto* mob = dynamic_cast<MobEntity*>(entity.get());
    if (mob != nullptr) {
        mob->enablePersistence();
        mob->setPosition(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f);
        mob->setRotation(0.0f, 0.0f);
        const entity::combat::DifficultyInstance difficulty = entity::combat::DifficultyInstance::at(world, pos);
        mob->finalizeSpawn(world, difficulty, world::spawn::SpawnReason::Structure);
    }
    world.spawnEntity(std::move(entity));

    const BlockState* replacement = pos.y > world.seaLevel() ? VanillaBlocks::getState(VanillaBlocks::AIR)
                                                             : VanillaBlocks::getState(VanillaBlocks::WATER);
    world.setBlockState(pos.x, pos.y, pos.z, replacement, 2);
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

std::unique_ptr<StructureStart> OceanRuinStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = makeStructureStart(chunkX, chunkZ);

    // 生物群系校验落在候选生成点（区块中心、海底高度）上；不匹配即返回空起点。
    if (!isValidBiomeAt(generator, onTopOfChunkCenter(generator, chunkX, chunkZ, HeightmapType::OceanFloorWG))) {
        return start;
    }

    // 【随机数顺序】原版 findGenerationPoint 走 onTopOfChunkCenter，而它**不消耗随机数**
    // （X/Z 取区块中心是定值，Y 走高度图），generatePieces 的第一件事就是取旋转。这里同样
    // 先取旋转再判大小，中间不得插入任何抽样。
    // 生成点固定为「区块最小角 + 90」：X/Z 不抽样（原版 generatePieces 直接用
    // chunkPos.getMinBlockX()/getMinBlockZ()），Y 恒为 90。
    //
    // 【90 是最终高度，不是占位】原版 OceanRuinPieces.addPieces 不在任何位置回填海床
    // 高度：片段包围盒就是围绕 Y=90 展开的，放置阶段再由模板处理器自行贴地。
    const i32 baseX = chunkX * world::CHUNK_WIDTH;
    const i32 baseZ = chunkZ * world::CHUNK_WIDTH;
    const i32 anchorY = 90;

    // 暖/冷变体来自结构定义（数据包 `biome_temp`），不按生成点群系现判：结构集本身已经
    // 按群系标签挑好了变体，再判一次只会让两者不一致。
    const OceanRuinConfig& config = m_config;

    // 获取模板管理器
    feature::template_::TemplateManager* templateManager = m_templateManager;
    if (!templateManager) {
        templateManager = &jigsaw::JigsawAssembler::getTemplateManager();
    }

    // generatePieces 的第一步就是取旋转（Rotation.getRandom 消耗 1 次）。
    const Rotation rotation = static_cast<Rotation>(rng.nextInt(4));

    // 创建片段列表
    std::vector<std::unique_ptr<StructurePiece>> pieces;

    // OceanRuinPieces.addPieces：先抽「是否大型」，再生成主废墟，最后才抽集群概率。
    const bool isLarge = rng.nextFloat() <= config.largeProbability;
    const f32 integrity = isLarge ? 0.9f : 0.8f;
    appendRuinPieces(
        *templateManager, BlockPos(baseX, anchorY, baseZ), rotation, rng, config, isLarge, integrity, pieces);

    if (isLarge && rng.nextFloat() <= config.clusterProbability) {
        generateClusterPieces(*templateManager, rng, rotation, BlockPos(baseX, anchorY, baseZ), config, pieces);
    }

    // 将片段添加到 start
    for (auto& piece : pieces) {
        start->addPiece(std::move(piece));
    }

    start->recalculateStructureSize();
    return start;
}

bool OceanRuinStructure::_isWarmBiome(BiomeId biomeId) const noexcept
{
    return biomeId == Biomes::WarmOcean || biomeId == Biomes::LukewarmOcean || biomeId == Biomes::DeepWarmOcean ||
        biomeId == Biomes::DeepLukewarmOcean;
}

} // namespace mc::world::gen::structure
