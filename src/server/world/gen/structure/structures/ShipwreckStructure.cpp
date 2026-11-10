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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/core/LootableContainerBlockEntity.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
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
    , m_isBeached(isBeached)
    , m_size(1, 1, 1)
{
    // 原版 `TemplateStructurePiece` 在构造期就用模板的真实包围盒确定几何：模板管理器是全局
    // 共享的，这里直接问它要模板，不必等到 FEATURES 阶段才回填——否则 STRUCTURE_STARTS 阶段
    // 每个构件都是一个点（整座沉船在外层看来只有 1x1x1），差分里表现为「位置对、尺寸全错」。
    m_templatePosition = position;
    // 旋转与镜像存进基类（子类同名成员会遮蔽基类，getRotation() 读的正是基类那个）。
    setRotation(rotation);
    setMirror(Mirror::None);
    const feature::template_::Template* templ =
        jigsaw::JigsawAssembler::getTemplateManager().getTemplate(ResourceLocation(m_templateName));
    if (templ == nullptr) {
        return;
    }
    m_size = templ->getSize();
    _updateBoundingBox();
}

void ShipwreckPiece::_updateBoundingBox()
{
    // 模板绕 `STRUCTURE_OFFSET` 旋转，包围盒由模板尺寸与旋转共同决定。
    feature::template_::PlacementSettings settings;
    settings.setRotation(getRotation());
    settings.setMirror(Mirror::None);
    settings.setRotationPivot(STRUCTURE_OFFSET);
    const StructureBoundingBox rect = jigsaw::JigsawAssembler::getTemplateManager()
                                          .getTemplate(ResourceLocation(m_templateName))
                                          ->getBoundingBox(settings, BlockPos(0, 0, 0));
    m_minX = m_templatePosition.x + rect.minX();
    m_minY = m_templatePosition.y + rect.minY();
    m_minZ = m_templatePosition.z + rect.minZ();
    m_maxX = m_templatePosition.x + rect.maxX();
    m_maxY = m_templatePosition.y + rect.maxY();
    m_maxZ = m_templatePosition.z + rect.maxZ();
}

void ShipwreckPiece::adjustPositionHeight(i32 y)
{
    m_templatePosition = BlockPos(m_templatePosition.x, y, m_templatePosition.z);
    _updateBoundingBox();
}

void ShipwreckPiece::_loadTemplate()
{
    if (!m_templateManager) {
        return;
    }

    ResourceLocation location(m_templateName);
    m_template = m_templateManager->getTemplate(location);

    // 包围盒已在构造期按模板真实尺寸算好，这里不再改写。
}

void ShipwreckPiece::generate(IWorldWriter& world,
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
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "ShipwreckPiece::generate requires an IWorld");

    // 贴地：起点阶段的 Y=90 只是占位，真正的落点在这里按高度图逐区块重算。
    // 超大模板在起点阶段已经贴过地（对应原版 `isTooBigToFitInWorldGenRegion` 分支），
    // 后处理不再重复调整。
    const bool tooBig = m_size.x > 32 || m_size.y > 32;
    if (!tooBig) {
        const HeightmapType heightmapType = m_isBeached ? HeightmapType::WorldSurfaceWG : HeightmapType::OceanFloorWG;
        const i32 columnCount = m_size.x * m_size.z;
        i32 lowest = reader->getMaxBuildHeight();
        i32 average = 0;
        // `IWorld::getHeight` 返回最高方块 Y，原版 `LevelReader.getHeight` 返回首个可放置 Y
        // （最高方块 Y + 1）；模板原点按原版语义贴在海床/地表上方一格。
        if (columnCount == 0) {
            average = reader->getHeight(m_templatePosition.x, m_templatePosition.z, heightmapType) + 1;
        } else {
            for (i32 z = m_templatePosition.z; z < m_templatePosition.z + m_size.z; ++z) {
                for (i32 x = m_templatePosition.x; x < m_templatePosition.x + m_size.x; ++x) {
                    const i32 height = reader->getHeight(x, z, heightmapType) + 1;
                    average += height;
                    lowest = std::min(lowest, height);
                }
            }
            average /= columnCount;
        }

        // 搁浅沉船沿最低列再下沉半高并随机再降 0..2 格；随机数必须在放置模板前消耗。
        const i32 targetY = m_isBeached ? lowest - m_size.y / 2 - rng.nextInt(3) : average;
        adjustPositionHeight(targetY);
    }

    // 创建放置设置
    feature::template_::PlacementSettings settings;
    settings.setRotation(getRotation());
    settings.setMirror(Mirror::None);
    settings.setRotationPivot(STRUCTURE_OFFSET);
    settings.setBoundingBox(&chunkBounds);

    // 原版 `BlockIgnoreProcessor.STRUCTURE_AND_AIR`：结构方块（含 DATA 标记）与空气都不放置。
    std::vector<u32> blocksToIgnore;
    if (auto* airState = VanillaBlocks::getState(VanillaBlocks::AIR)) {
        blocksToIgnore.push_back(airState->blockId());
    }
    if (auto* structureState = VanillaBlocks::getState(VanillaBlocks::STRUCTURE_BLOCK)) {
        blocksToIgnore.push_back(structureState->blockId());
    }
    feature::template_::StructureProcessorList processors;
    processors.addProcessor(std::make_unique<feature::template_::BlockIgnoreStructureProcessor>(blocksToIgnore));
    settings.setProcessors(&processors);

    // 包围盒由模板原点绕枢轴展开，放置位置就是模板原点本身。
    // 用完整版放置：它会在放完后按邻居重算方块形状（楼梯/栅栏等），与原版一致。
    if (!m_template->placeInWorld(
            *reader, m_templatePosition, settings, rng, world::BlockUpdateFlags::UPDATE_CLIENTS)) {
        return;
    }

    // 原版 `TemplateStructurePiece#postProcess`：放完模板后再按 DATA 标记生成箱子内容，
    // 最后把拼图方块按 `final_state` 覆盖。
    for (const auto& marker : m_template->getDataMarkers(m_templatePosition, settings)) {
        handleDataMarker(marker.metadata, marker.pos, *reader, rng, chunkBounds);
    }
    m_template->applyJigsawReplacements(*reader, m_templatePosition, settings);
}

void ShipwreckPiece::handleDataMarker(const std::string& metadata,
    const BlockPos& pos,
    IWorld& world,
    math::IRandom& rng,
    const StructureBoundingBox& /*chunkBounds*/)
{
    static const ResourceLocation mapLoot("minecraft:chests/shipwreck_map");
    static const ResourceLocation treasureLoot("minecraft:chests/shipwreck_treasure");
    static const ResourceLocation supplyLoot("minecraft:chests/shipwreck_supply");

    const ResourceLocation* lootTable = nullptr;
    if (metadata == "map_chest") {
        lootTable = &mapLoot;
    } else if (metadata == "treasure_chest") {
        lootTable = &treasureLoot;
    } else if (metadata == "supply_chest") {
        lootTable = &supplyLoot;
    }
    if (lootTable == nullptr) {
        return;
    }

    // 原版 `RandomizableContainer.setBlockEntityLootTable`：只有下方确实是战利品容器时
    // 才消耗随机数并挂表。
    BlockEntity* blockEntity = world.getBlockEntity(BlockPos(pos.x, pos.y - 1, pos.z));
    auto* container = dynamic_cast<blockentity::LootableContainerBlockEntity*>(blockEntity);
    if (container != nullptr) {
        container->setLootTable(*lootTable, rng.nextLong());
    }
}

// ============================================================================
// ShipwreckStructure
// ============================================================================

ShipwreckStructure::ShipwreckStructure(ResourceLocation id)
    : Structure(std::move(id))
{}

const biome::BiomeTag* ShipwreckStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_SHIPWRECK();
}

std::string ShipwreckStructure::_getRandomTemplateName(math::IRandom& rng, bool isBeached) const
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
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = makeStructureStart(chunkX, chunkZ);

    // 搁浅与否由结构定义决定（数据包里的 `is_beached`），不是按生成点群系现判。
    const bool isBeached = m_config.isBeached;

    // 生物群系校验落在候选生成点（区块中心、搁浅取地表 / 否则取海底）上；
    // 不匹配即返回空起点。
    const HeightmapType heightmapType = isBeached ? HeightmapType::WorldSurfaceWG : HeightmapType::OceanFloorWG;
    if (!isValidBiomeAt(generator, onTopOfChunkCenter(generator, chunkX, chunkZ, heightmapType))) {
        return start;
    }

    // 原版 findGenerationPoint 走 onTopOfChunkCenter：它**不消耗随机数**（X/Z 是区块中心的
    // 定值，Y 走高度图），generatePieces 第一件事就是取旋转。这里同样直接从旋转开始。

    // 抽取顺序：旋转 → 模板。原版 addRandomPiece 先取模板再构造构件。
    const Rotation rotation = static_cast<Rotation>(rng.nextInt(4));

    feature::template_::TemplateManager* templateManager = m_templateManager;
    if (!templateManager) {
        templateManager = &jigsaw::JigsawAssembler::getTemplateManager();
    }
    const std::string templateName = _getRandomTemplateName(rng, isBeached);

    const i32 baseX = chunkX * world::CHUNK_WIDTH;
    const i32 baseZ = chunkZ * world::CHUNK_WIDTH;
    auto piece = std::make_unique<ShipwreckPiece>(templateName, BlockPos(baseX, 90, baseZ), rotation, isBeached);
    piece->setTemplateManager(templateManager);

    // 只有「放不进世界生成区域」的超大模板才在起点阶段调整高度（原版
    // `isTooBigToFitInWorldGenRegion` 分支）；其余模板的贴地发生在后处理阶段，
    // 起点阶段的包围盒保持在 Y=90。
    // TODO: 在后处理阶段实现 `ShipwreckPiece.postProcess` 的贴地（赋高与搁浅随机下沉），
    //       目前只保证起点阶段的几何一致。
    const BlockPos size = piece->templateSize();
    if (size.x > 32 || size.y > 32) {
        const auto& bounds = piece->getBoundingBox();
        const HeightmapType adjustType = isBeached ? HeightmapType::WorldSurfaceWG : HeightmapType::OceanFloorWG;
        i32 height;
        if (isBeached) {
            i32 lowest = 0;
            for (i32 z = bounds.minZ(); z <= bounds.maxZ(); ++z) {
                for (i32 x = bounds.minX(); x <= bounds.maxX(); ++x) {
                    lowest = std::min(lowest, generator.getHeight(x, z, adjustType));
                }
            }
            height = lowest - size.y / 2 - rng.nextInt(3);
        } else {
            i64 sum = 0;
            i32 count = 0;
            for (i32 z = bounds.minZ(); z <= bounds.maxZ(); ++z) {
                for (i32 x = bounds.minX(); x <= bounds.maxX(); ++x) {
                    sum += generator.getHeight(x, z, heightmapType);
                    count++;
                }
            }
            height = count == 0 ? 0 : static_cast<i32>(sum / count);
        }
        piece->adjustPositionHeight(height);
    }

    start->addPiece(std::move(piece));
    start->recalculateStructureSize();
    return start;
}

} // namespace mc::world::gen::structure
