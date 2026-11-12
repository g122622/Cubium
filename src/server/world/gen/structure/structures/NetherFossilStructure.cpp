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
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
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
const std::vector<std::string> NetherFossilStructure::s_fossilTemplates = {"minecraft:nether_fossils/fossil_1",
    "minecraft:nether_fossils/fossil_2",
    "minecraft:nether_fossils/fossil_3",
    "minecraft:nether_fossils/fossil_4",
    "minecraft:nether_fossils/fossil_5",
    "minecraft:nether_fossils/fossil_6",
    "minecraft:nether_fossils/fossil_7",
    "minecraft:nether_fossils/fossil_8",
    "minecraft:nether_fossils/fossil_9",
    "minecraft:nether_fossils/fossil_10",
    "minecraft:nether_fossils/fossil_11",
    "minecraft:nether_fossils/fossil_12",
    "minecraft:nether_fossils/fossil_13",
    "minecraft:nether_fossils/fossil_14"};

// ============================================================================
// NetherFossilPiece
// ============================================================================

NetherFossilPiece::NetherFossilPiece(const std::string& templateName, const BlockPos& position, Rotation rotation)
    : StructurePiece(
          StructurePieceTypes::NETHER_FOSSIL, position.x, position.y, position.z, position.x, position.y, position.z)
    , m_templateName(templateName)
    , m_templatePosition(position)
    , m_size(1, 1, 1)
{
    // 旋转与镜像存进基类：子类同名成员会遮蔽基类，`getRotation()` 读的正是基类那个。
    setRotation(rotation);
    setMirror(Mirror::None);
    // 原版 `TemplateStructurePiece` 在构造期就以模板真实包围盒确定几何；等到后处理阶段才展开
    // 会让 STRUCTURE_STARTS 阶段每个构件退化成一个点。
    _loadTemplate();
}

void NetherFossilPiece::_loadTemplate()
{
    if (m_templateManager == nullptr) {
        m_templateManager = &jigsaw::JigsawAssembler::getTemplateManager();
    }
    m_template = m_templateManager->getTemplate(ResourceLocation(m_templateName));
    if (m_template == nullptr) {
        return;
    }
    m_size = m_template->getSize();
    _updateBoundingBox();
}

void NetherFossilPiece::_updateBoundingBox()
{
    feature::template_::PlacementSettings settings;
    settings.setRotation(getRotation());
    settings.setMirror(Mirror::None);
    const StructureBoundingBox box = m_template->getBoundingBox(settings, m_templatePosition);
    m_minX = box.minX();
    m_minY = box.minY();
    m_minZ = box.minZ();
    m_maxX = box.maxX();
    m_maxY = box.maxY();
    m_maxZ = box.maxZ();
}

void NetherFossilPiece::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    // 构造期模板管理器还没装好时补一次加载。
    if (!m_template) {
        _loadTemplate();
    }

    if (!m_template) {
        spdlog::warn("NetherFossilPiece: Failed to load template: {}", m_templateName);
        return;
    }

    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "NetherFossilPiece::generate requires an IWorld");

    // 创建放置设置
    feature::template_::PlacementSettings settings;
    settings.setRotation(getRotation());
    settings.setMirror(Mirror::None);
    settings.setBoundingBox(&chunkBounds);

    // 结构方块（含数据标记）与空气都是模板占位，不写入世界
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

    // 放置模板（原点即模板位置，不是包围盒最小角）
    m_template->placeInWorld(*reader, m_templatePosition, settings, rng, world::BlockUpdateFlags::UPDATE_CLIENTS);

    // 原版 `TemplateStructurePiece.postProcess` 末尾的拼图方块 `final_state` 覆盖。
    m_template->applyJigsawReplacements(*reader, m_templatePosition, settings);

    // 原版 NetherFossilPiece.placeDriedGhast：50% 概率在化石底部放一个随机朝向的干燥恶魂方块。
    // 注意原版 `handleDataMarker` 对本结构是**空实现**（NetherFossilPieces 里没有任何数据标记），
    // 所以这里不存在"数据标记需排在 jigsaw 覆盖之前"的问题。
    _placeDriedGhast(*reader, world, chunkBounds);
}

void NetherFossilPiece::_placeDriedGhast(IWorld& world, IWorldWriter& writer, const StructureBoundingBox& chunkBounds)
{
    // 模板包围盒（原版 placeDriedGhast 的 `boundingbox` 参数）。
    const StructureBoundingBox templateBounds = getBoundingBox();
    if (templateBounds.xSpan() <= 0 || templateBounds.zSpan() <= 0) {
        return;
    }

    // 随机源口径：`RandomSource.create(level.getSeed()).forkPositional().at(模板包围盒中心)`。
    // 它只由世界种子与化石位置决定，与传入 generate 的区块随机源无关——若改用后者，
    // 同一化石在不同区块的后处理里会拿到不同的随机值。
    math::JavaLegacyRandom levelRandom(world.seed());
    math::PositionalRandomFactory factory = levelRandom.forkPositional();
    std::unique_ptr<math::IRandom> ghastRandom =
        factory.at(templateBounds.centerX(), templateBounds.centerY(), templateBounds.centerZ());

    if (ghastRandom->nextFloat() >= 0.5f) {
        return;
    }

    const i32 x = templateBounds.minX() + ghastRandom->nextInt(templateBounds.xSpan());
    const i32 y = templateBounds.minY();
    const i32 z = templateBounds.minZ() + ghastRandom->nextInt(templateBounds.zSpan());
    const BlockPos pos(x, y, z);

    // 只放在空气里，且必须在本次生成的区块包围盒内（避免写到尚未生成的邻居区块）。
    const BlockState* current = world.getBlockState(pos);
    if (current == nullptr || !current->is(VanillaBlocks::AIR)) {
        return;
    }
    if (pos.x < chunkBounds.minX() || pos.x > chunkBounds.maxX() || pos.z < chunkBounds.minZ() ||
        pos.z > chunkBounds.maxZ()) {
        return;
    }

    const BlockState* driedGhast = VanillaBlocks::getState(VanillaBlocks::DRIED_GHAST);
    if (driedGhast == nullptr) {
        spdlog::warn("NetherFossilPiece: dried_ghast state is unavailable; skipping placeDriedGhast");
        return;
    }
    // 原版把朝向交给 `defaultBlockState().rotate(Rotation.getRandom(random))`：
    // Rotation 的四个枚举值按 NONE/CLOCKWISE_90/CLOCKWISE_180/COUNTERCLOCKWISE_90 顺序排列，
    // 因此一次 `nextInt(4)` 就是原版的取值映射。
    const Rotation rotation = static_cast<Rotation>(ghastRandom->nextInt(4));
    const BlockState& rotatedState = driedGhast->getBlock().rotate(*driedGhast, rotation);
    writer.setBlockState(pos.x, pos.y, pos.z, &rotatedState, static_cast<i32>(world::BlockUpdateFlags::UPDATE_CLIENTS));
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
