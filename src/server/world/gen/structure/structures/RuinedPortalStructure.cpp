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

#include "RuinedPortalStructure.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/tags/BlockTags.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/Feature.hpp"
#include "server/world/gen/feature/template/ProtectedBlocksProcessor.hpp"
#include "server/world/gen/feature/template/RuleTest.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/base/StructureBoundingBox.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::world::gen::structure {

using namespace mc::Biomes;
using namespace mc::world; // 引入 CHUNK_WIDTH 等常量

namespace {

// ============================================================================
// 废弃传送门处理器概率常量
// 与 MC 1.21.11 RuinedPortalPiece 中定义一致
// ============================================================================

/// 金块替换为空气的概率（PROBABILITY_OF_GOLD_GONE）
constexpr f32 PROBABILITY_OF_GOLD_GONE = 0.3F;

/// 下界岩替换为岩浆块的概率（PROBABILITY_OF_MAGMA_INSTEAD_OF_NETHERRACK）
constexpr f32 PROBABILITY_OF_MAGMA_INSTEAD_OF_NETHERRACK = 0.07F;

/// 岩浆替换为岩浆块的概率（PROBABILITY_OF_MAGMA_INSTEAD_OF_LAVA）
constexpr f32 PROBABILITY_OF_MAGMA_INSTEAD_OF_LAVA = 0.2F;

/// 构造「输入方块 → 输出方块」的固定替换规则（无概率，纯 BlockMatch）
std::unique_ptr<feature::template_::RuleEntry> makeBlockReplaceRule(const Block* inputBlock, const Block* outputBlock)
{
    if (inputBlock == nullptr || outputBlock == nullptr) {
        return nullptr;
    }
    return std::make_unique<feature::template_::RuleEntry>(
        std::make_unique<feature::template_::BlockMatchRuleTest>(inputBlock),
        std::make_unique<feature::template_::AlwaysTrueRuleTest>(),
        outputBlock->defaultState().stateId());
}

/// 构造「输入方块 按概率 → 输出方块」的随机替换规则（RandomBlockMatch）
std::unique_ptr<feature::template_::RuleEntry> makeRandomBlockReplaceRule(
    const Block* inputBlock, f32 probability, const Block* outputBlock)
{
    if (inputBlock == nullptr || outputBlock == nullptr) {
        return nullptr;
    }
    return std::make_unique<feature::template_::RuleEntry>(
        std::make_unique<feature::template_::RandomBlockMatchRuleTest>(inputBlock, probability),
        std::make_unique<feature::template_::AlwaysTrueRuleTest>(),
        outputBlock->defaultState().stateId());
}

/// 根据垂直放置位置与 cold 属性构造岩浆处理规则
/// 对应 MC 1.21.11 RuinedPortalPiece#getLavaProcessorRule
std::unique_ptr<feature::template_::RuleEntry> makeLavaProcessorRule(RuinedPortalLocation location, bool cold)
{
    if (location == RuinedPortalLocation::OnOceanFloor) {
        // 海底：岩浆固定替换为岩浆块
        return makeBlockReplaceRule(VanillaBlocks::LAVA, VanillaBlocks::MAGMA);
    }
    if (cold) {
        // 寒冷：岩浆固定替换为下界岩
        return makeBlockReplaceRule(VanillaBlocks::LAVA, VanillaBlocks::NETHERRACK);
    }
    // 默认：岩浆以 0.2 概率替换为岩浆块
    return makeRandomBlockReplaceRule(VanillaBlocks::LAVA, PROBABILITY_OF_MAGMA_INSTEAD_OF_LAVA, VanillaBlocks::MAGMA);
}

} // namespace

// ============================================================================
// 静态常量
// ============================================================================

const std::string RuinedPortalStructure::s_name = "ruined_portal";

// 普通传送门模板（10个）
const std::vector<std::string> RuinedPortalStructure::s_normalTemplates = {"ruined_portal/portal_1",
    "ruined_portal/portal_2",
    "ruined_portal/portal_3",
    "ruined_portal/portal_4",
    "ruined_portal/portal_5",
    "ruined_portal/portal_6",
    "ruined_portal/portal_7",
    "ruined_portal/portal_8",
    "ruined_portal/portal_9",
    "ruined_portal/portal_10"};

// 巨型传送门模板（3个）
const std::vector<std::string> RuinedPortalStructure::s_giantTemplates = {
    "ruined_portal/giant_portal_1", "ruined_portal/giant_portal_2", "ruined_portal/giant_portal_3"};

// ============================================================================
// RuinedPortalPiece
// ============================================================================

RuinedPortalPiece::RuinedPortalPiece(const std::string& templateName,
    const BlockPos& position,
    Rotation rotation,
    Mirror mirror,
    RuinedPortalLocation location,
    const RuinedPortalProperties& properties,
    const BlockPos& candidatePos)
    : StructurePiece(
          StructurePieceTypes::RUINED_PORTAL, position.x, position.y, position.z, position.x, position.y, position.z)
    , m_templateName(templateName)
    , m_location(location)
    , m_properties(properties)
    , m_candidatePos(candidatePos)
    , m_size(1, 1, 1)
{
    m_rotation = rotation;
    m_mirror = mirror;
    // 模板标识在片段里全程用资源位置的完整写法（`minecraft:ruined_portal/portal_1`）：
    // 原版 `TemplateStructurePiece.templateName` 存的就是 `Identifier.toString()`，
    // 存相对路径会让任何读该字段的地方拿到与资源位置不同的字符串。
    m_templateName = ResourceLocation(templateName).toString();
}

void RuinedPortalPiece::_loadTemplate()
{
    if (!m_templateManager) {
        return;
    }

    ResourceLocation location(m_templateName);
    m_template = m_templateManager->getTemplate(location);

    if (m_template) {
        m_size = m_template->getSize();
        // 中心偏移是模板尺寸的一半
        m_centerOffset = BlockPos(m_size.x / 2, 0, m_size.z / 2);
        _updateBoundingBox();
    }
}

void RuinedPortalPiece::_updateBoundingBox()
{
    // 原版 TemplateStructurePiece 直接把模板的 getBoundingBox(placeSettings, templatePosition)
    // 当包围盒用：templatePosition 是**候选生成点**（区块原点），盒子已经围绕旋转轴心
    // (templateSize/2) 展开。这里再减一次轴心会让整座传送门沿 XZ 平移半个模板边长。
    feature::template_::PlacementSettings boxSettings;
    boxSettings.setRotation(m_rotation);
    boxSettings.setMirror(m_mirror);
    boxSettings.setCenterOffset(m_centerOffset);
    const StructureBoundingBox box = m_template->getBoundingBox(boxSettings, BlockPos(m_minX, m_minY, m_minZ));
    m_minX = box.minX();
    m_minY = box.minY();
    m_minZ = box.minZ();
    m_maxX = box.maxX();
    m_maxY = box.maxY();
    m_maxZ = box.maxZ();
}

void RuinedPortalPiece::generate(IWorldWriter& world,
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
        spdlog::warn("RuinedPortalPiece: Failed to load template: {}", m_templateName);
        return;
    }

    // 检查边界框是否与区块相交
    StructureBoundingBox pieceBounds(m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ);
    if (!pieceBounds.intersects(chunkBounds)) {
        return;
    }

    // 原版 RuinedPortalPiece#postProcess 只有「本区块包含模板盒中心」时才做任何事：
    // 先用 encapsulate 把可写区扩到整个模板盒，再一次性放完整座传送门（否则每个相交
    // 区块都会重复写一遍，而且共享随机源会被多消费）。
    const i32 centerX = m_minX + (m_maxX - m_minX + 1) / 2;
    const i32 centerY = m_minY + (m_maxY - m_minY + 1) / 2;
    const i32 centerZ = m_minZ + (m_maxZ - m_minZ + 1) / 2;
    if (!chunkBounds.isInside(centerX, centerY, centerZ)) {
        return;
    }
    StructureBoundingBox writable = chunkBounds;
    writable.expandTo(pieceBounds);

    // 创建放置设置
    feature::template_::PlacementSettings settings;
    settings.setRotation(m_rotation);
    settings.setMirror(m_mirror);
    settings.setCenterOffset(m_centerOffset);
    settings.setBoundingBox(&writable);

    // 设置世界引用（ProtectedBlocksProcessor / LavaSubmergingProcessor 等需要 IWorld 读取世界方块）
    IWorld* iworld = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(iworld != nullptr, "RuinedPortalPiece::generate requires an IWorld");
    settings.setWorld(iworld);

    // 添加处理器
    feature::template_::StructureProcessorList processors;

    // 1) 方块忽略处理器
    // 如果有空气口袋，只忽略结构方块；否则忽略空气和结构方块
    std::vector<u32> blocksToIgnore;
    if (m_properties.airPocket) {
        // 忽略结构方块（有空气口袋时不覆盖空气）
        if (auto* structureState = VanillaBlocks::getState(VanillaBlocks::STRUCTURE_BLOCK)) {
            blocksToIgnore.push_back(structureState->blockId());
        }
    } else {
        // 忽略空气和结构方块
        if (auto* airState = VanillaBlocks::getState(VanillaBlocks::AIR)) {
            blocksToIgnore.push_back(airState->blockId());
        }
        if (auto* structureState = VanillaBlocks::getState(VanillaBlocks::STRUCTURE_BLOCK)) {
            blocksToIgnore.push_back(structureState->blockId());
        }
    }
    if (!blocksToIgnore.empty()) {
        processors.addProcessor(std::make_unique<feature::template_::BlockIgnoreStructureProcessor>(blocksToIgnore));
    }

    // 2) RuleStructureProcessor：构造替换规则列表
    // 对应 MC 1.21.11 RuinedPortalPiece#makeSettings 中的 ruleProcessor
    // 顺序：金块→空气(0.3) → 岩浆规则(位置/寒冷) → 下界岩→岩浆块(0.07, !cold)
    {
        std::vector<std::unique_ptr<feature::template_::RuleEntry>> rules;

        // 金块以 0.3 概率替换为空气
        if (auto rule =
                makeRandomBlockReplaceRule(VanillaBlocks::GOLD_BLOCK, PROBABILITY_OF_GOLD_GONE, VanillaBlocks::AIR)) {
            rules.push_back(std::move(rule));
        }

        // 岩浆处理规则（依赖垂直放置位置与 cold 属性）
        if (auto rule = makeLavaProcessorRule(m_location, m_properties.cold)) {
            rules.push_back(std::move(rule));
        }

        // 非寒冷时：下界岩以 0.07 概率替换为岩浆块
        if (!m_properties.cold) {
            if (auto rule = makeRandomBlockReplaceRule(
                    VanillaBlocks::NETHERRACK, PROBABILITY_OF_MAGMA_INSTEAD_OF_NETHERRACK, VanillaBlocks::MAGMA)) {
                rules.push_back(std::move(rule));
            }
        }

        if (!rules.empty()) {
            processors.addProcessor(std::make_unique<feature::template_::RuleStructureProcessor>(std::move(rules)));
        }
    }

    // 3) BlockAgeProcessor：石砖随机苔藓化
    processors.addProcessor(std::make_unique<feature::template_::BlockAgeProcessor>(m_properties.mossiness));

    // 4) ProtectedBlocksProcessor：保护 #minecraft:features_cannot_replace 标签方块不被覆盖
    processors.addProcessor(
        std::make_unique<feature::template_::ProtectedBlocksProcessor>(BlockTags::FEATURES_CANNOT_REPLACE().getId()));

    // 5) LavaSubmergingProcessor：岩浆淹没处理（非固体方块在岩浆中替换为岩浆）
    processors.addProcessor(std::make_unique<feature::template_::LavaSubmergingProcessor>());

    // 6) BlackstoneReplacementProcessor：下界传送门将石质方块替换为黑石变体
    if (m_properties.replaceWithBlackstone) {
        processors.addProcessor(std::make_unique<feature::template_::BlackstoneReplacementProcessor>());
    }

    settings.setProcessors(&processors);

    // 放置原点必须是**候选生成点**本身：模板的方块坐标是相对该点、绕 (size/2) 轴心变换后
    // 再相加的。用包围盒的 min 当原点会重复扣掉轴心，整座传送门平移半个模板边长。
    m_template->placeInWorld(*iworld, m_candidatePos, settings, rng, world::BlockUpdateFlags::UPDATE_CLIENTS);

    // 原版 `TemplateStructurePiece.postProcess`：模板放置完成后，把拼图方块按各自 NBT 的
    // `final_state` 覆盖（废弃传送门模板里带 jigsaw 方块，放置阶段会原样写进世界）。
    m_template->applyJigsawReplacements(*iworld, m_candidatePos, settings);

    // ===== 放置后的地狱岩/岩浆块点缀 =====
    //
    // 这些步骤在空世界里也常常会写入方块（模板自身的地狱岩会触发下方垂滴列），
    // 因此必须逐抽样复刻，不能省略。
    _spreadNetherrack(*iworld, rng);
    _addNetherrackDripColumnsBelowPortal(*iworld, rng);
    if (m_properties.vines || m_properties.overgrown) {
        // 迭代顺序必须与原版 `BlockPos.betweenClosedStream(box)` 一致：x 最快、y 次之、z 最慢。
        // 藤蔓与树叶分支都会消费随机数，顺序错了整个随机流就会错位。
        for (i32 z = m_minZ; z <= m_maxZ; ++z) {
            for (i32 y = m_minY; y <= m_maxY; ++y) {
                for (i32 x = m_minX; x <= m_maxX; ++x) {
                    const BlockPos pos(x, y, z);
                    if (m_properties.vines) {
                        _maybeAddVines(*iworld, rng, pos);
                    }
                    if (m_properties.overgrown) {
                        _maybeAddLeavesAbove(*iworld, rng, pos);
                    }
                }
            }
        }
    }
}

// ============================================================================
// RuinedPortalPiece 放置后点缀
// ============================================================================

i32 RuinedPortalPiece::_surfaceY(const IWorld& world, i32 x, i32 z) const
{
    // 原版 getSurfaceY = `level.getHeight(type, x, z) - 1`，即最高方块 Y。本项目的
    // `IWorld::getHeight(x, z, type)` 直接返回最高方块 Y（已替你减过这一步），因此不能再减 1。
    // 海底放置用 OCEAN_FLOOR_WG，其余用 WORLD_SURFACE_WG。
    const HeightmapType type = m_location == RuinedPortalSetupDefinition::Placement::OnOceanFloor
        ? HeightmapType::OceanFloorWG
        : HeightmapType::WorldSurfaceWG;
    return world.getHeight(x, z, type);
}

bool RuinedPortalPiece::_canBeReplacedByNetherrackOrMagma(const IWorld& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || state->isAir()) {
        return false;
    }
    if (state->is(VanillaBlocks::OBSIDIAN)) {
        return false;
    }
    if (BlockTags::FEATURES_CANNOT_REPLACE().contains(state->getBlock())) {
        return false;
    }
    if (m_location != RuinedPortalLocation::InNether && state->is(VanillaBlocks::LAVA)) {
        return false;
    }
    return true;
}

void RuinedPortalPiece::_placeNetherrackOrMagma(IWorld& world, math::IRandom& rng, const BlockPos& pos)
{
    const Block* block = nullptr;
    if (!m_properties.cold && rng.nextFloat() < PROBABILITY_OF_MAGMA_INSTEAD_OF_NETHERRACK) {
        block = VanillaBlocks::MAGMA;
    } else {
        block = VanillaBlocks::NETHERRACK;
    }
    if (block != nullptr) {
        world.setBlockState(pos.x, pos.y, pos.z, &block->defaultState());
    }
}

void RuinedPortalPiece::_addNetherrackDripColumn(IWorld& world, math::IRandom& rng, const BlockPos& pos)
{
    BlockPos cursor = pos;
    _placeNetherrackOrMagma(world, rng, cursor);
    i32 remaining = 8;
    while (remaining > 0 && rng.nextFloat() < 0.5F) {
        cursor = BlockPos(cursor.x, cursor.y - 1, cursor.z);
        --remaining;
        _placeNetherrackOrMagma(world, rng, cursor);
    }
}

void RuinedPortalPiece::_addNetherrackDripColumnsBelowPortal(IWorld& world, math::IRandom& rng)
{
    for (i32 x = m_minX + 1; x < m_maxX; ++x) {
        for (i32 z = m_minZ + 1; z < m_maxZ; ++z) {
            const BlockPos pos(x, m_minY, z);
            const BlockState* state = world.getBlockState(pos);
            if (state != nullptr && state->is(VanillaBlocks::NETHERRACK)) {
                _addNetherrackDripColumn(world, rng, BlockPos(x, m_minY - 1, z));
            }
        }
    }
}

void RuinedPortalPiece::_spreadNetherrack(IWorld& world, math::IRandom& rng)
{
    const bool useSurface =
        m_location == RuinedPortalLocation::OnLandSurface || m_location == RuinedPortalLocation::OnOceanFloor;

    const i32 centerX = m_minX + (m_maxX - m_minX + 1) / 2;
    const i32 centerZ = m_minZ + (m_maxZ - m_minZ + 1) / 2;

    // 曼哈顿距离 → 概率衰减表
    static constexpr std::array<f32, 14> RADIAL_PROBABILITY = {
        1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 0.9F, 0.9F, 0.8F, 0.7F, 0.6F, 0.4F, 0.2F};
    constexpr i32 RADIUS = static_cast<i32>(RADIAL_PROBABILITY.size());

    const i32 horizontalSpan = ((m_maxX - m_minX + 1) + (m_maxZ - m_minZ + 1)) / 2;
    const i32 jitter = rng.nextInt(std::max(1, 8 - horizontalSpan / 2));

    for (i32 x = centerX - RADIUS; x <= centerX + RADIUS; ++x) {
        for (i32 z = centerZ - RADIUS; z <= centerZ + RADIUS; ++z) {
            const i32 manhattan = std::abs(x - centerX) + std::abs(z - centerZ);
            const i32 index = std::max(0, manhattan + jitter);
            if (index >= RADIUS) {
                continue;
            }
            if (rng.nextDouble() >= static_cast<f64>(RADIAL_PROBABILITY[index])) {
                continue;
            }

            const i32 surfaceY = _surfaceY(world, x, z);
            const i32 y = useSurface ? surfaceY : std::min(m_minY, surfaceY);
            const BlockPos pos(x, y, z);
            if (std::abs(y - m_minY) > 3) {
                continue;
            }
            if (!_canBeReplacedByNetherrackOrMagma(world, pos)) {
                continue;
            }

            _placeNetherrackOrMagma(world, rng, pos);
            if (m_properties.overgrown) {
                _maybeAddLeavesAbove(world, rng, pos);
            }
            _addNetherrackDripColumn(world, rng, BlockPos(x, y - 1, z));
        }
    }
}

void RuinedPortalPiece::_maybeAddVines(IWorld& world, math::IRandom& rng, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || state->isAir() || state->is(VanillaBlocks::VINE)) {
        return;
    }

    // 水平方向表（NORTH, EAST, SOUTH, WEST）决定 nextInt(4) 的下标映射
    const auto horizontalDirs = Directions::horizontal();
    const Direction direction = horizontalDirs[static_cast<std::size_t>(rng.nextInt(4))];
    const BlockPos adjacent(pos.x + Directions::xOffset(direction),
        pos.y + Directions::yOffset(direction),
        pos.z + Directions::zOffset(direction));

    const BlockState* neighbor = world.getBlockState(adjacent);
    if (neighbor != nullptr && !neighbor->isAir()) {
        return;
    }
    if (!Block::isFaceFull(state->getCollisionShape(), direction)) {
        return;
    }

    const Direction opposite = Directions::opposite(direction);
    const BooleanProperty* property = nullptr;
    switch (opposite) {
        case Direction::North:
            property = &BlockStateProperties::NORTH();
            break;
        case Direction::South:
            property = &BlockStateProperties::SOUTH();
            break;
        case Direction::East:
            property = &BlockStateProperties::EAST();
            break;
        case Direction::West:
            property = &BlockStateProperties::WEST();
            break;
        default:
            return;
    }

    const BlockState& vine = VanillaBlocks::VINE->defaultState().with(*property, true);
    world.setBlockState(adjacent.x, adjacent.y, adjacent.z, &vine);
}

void RuinedPortalPiece::_maybeAddLeavesAbove(IWorld& world, math::IRandom& rng, const BlockPos& pos)
{
    if (rng.nextFloat() >= 0.5F) {
        return;
    }
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || !state->is(VanillaBlocks::NETHERRACK)) {
        return;
    }
    const BlockPos above(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(above);
    if (aboveState != nullptr && !aboveState->isAir()) {
        return;
    }
    const BlockState& leaves =
        VanillaBlocks::JUNGLE_LEAVES->defaultState().with(BlockStateProperties::PERSISTENT(), true);
    world.setBlockState(above.x, above.y, above.z, &leaves);
}

// ============================================================================
// RuinedPortalStructure
// ============================================================================

RuinedPortalStructure::RuinedPortalStructure(ResourceLocation id, std::vector<RuinedPortalSetupDefinition> setups)
    : Structure(std::move(id))
    , m_setups(std::move(setups))
{}

const biome::BiomeTag* RuinedPortalStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_STANDARD();
}

namespace {

/// 在闭区间 [min, max] 上均匀取整（原版 `Mth.randomBetweenInclusive`）
i32 _randomBetweenInclusive(math::IRandom& rng, i32 minValue, i32 maxValue)
{
    return rng.nextInt(maxValue - minValue + 1) + minValue;
}

/**
 * @brief 在 [min, max] 上取整；区间为空时**不消耗随机数**直接返回 max
 *
 * 原版 `getRandomWithinInterval` 的语义：`min < max` 才抽取，否则返回 max。这个"不抽"
 * 分支有可观测后果——空区间出现时少一次 `nextInt`，后续所有抽取都会前移一格。
 */
i32 _randomWithinInterval(math::IRandom& rng, i32 minValue, i32 maxValue)
{
    return minValue < maxValue ? _randomBetweenInclusive(rng, minValue, maxValue) : maxValue;
}

/// 原版 `RuinedPortalStructure.sample`：概率为 0/1 时**不消耗随机数**
bool _sampleProbability(math::IRandom& rng, f32 probability)
{
    if (probability == 0.0f) {
        return false;
    }
    if (probability == 1.0f) {
        return true;
    }
    return rng.nextFloat() < probability;
}

/**
 * @brief 四角扫描求竖直落点（原版 `RuinedPortalStructure.findSuitableY`）
 *
 * 从候选高度`startY` 向下逐层扫描，某一层四个角上**至少三格**是高度图判据下的不透明
 * 方块时就把传送门放在该层——这样它不会悬空或整个埋进山体。扫描下界是 `minY + 15`；
 * 扫到下限仍未凑够三格时返回下限本身。
 *
 * @param boundingBox 模板包围盒（世界坐标）
 */
i32 _findSuitableY(math::IRandom& rng,
    IChunkGenerator& generator,
    WorldGenRegion* region,
    const RuinedPortalSetupDefinition& setup,
    bool airPocket,
    i32 startY,
    const StructureBoundingBox& boundingBox)
{
    const i32 minScanY = ::mc::world::MIN_BUILD_HEIGHT + 15;
    const i32 ySpan = boundingBox.ySpan();
    i32 y = 0;
    switch (setup.placement) {
        case RuinedPortalSetupDefinition::Placement::InNether:
            if (airPocket) {
                y = _randomBetweenInclusive(rng, 32, 100);
            } else if (rng.nextFloat() < 0.5f) {
                y = _randomBetweenInclusive(rng, 27, 29);
            } else {
                y = _randomBetweenInclusive(rng, 29, 100);
            }
            break;
        case RuinedPortalSetupDefinition::Placement::InMountain:
            // 目标区间 [70, 地表-模板高]：整座建筑都要留在地面以上，上界扣掉 Y 跨度。
            y = _randomWithinInterval(rng, 70, startY - ySpan);
            break;
        case RuinedPortalSetupDefinition::Placement::Underground:
            y = _randomWithinInterval(rng, minScanY, startY - ySpan);
            break;
        case RuinedPortalSetupDefinition::Placement::PartlyBuried:
            y = startY - ySpan + _randomBetweenInclusive(rng, 2, 8);
            break;
        case RuinedPortalSetupDefinition::Placement::OnLandSurface:
        case RuinedPortalSetupDefinition::Placement::OnOceanFloor:
        default:
            y = startY;
            break;
    }

    // 四角采样：`getBoundingBox(settings, pos)` 返回的盒子里**已经**包含 pos 偏移，四角就是
    // 盒子自身的 min/max，不能再加一次 origin——多加一次会让采样点整体跑到两倍坐标处，
    // 落点整个失效。
    const i32 minX = boundingBox.minX();
    const i32 maxX = boundingBox.maxX();
    const i32 minZ = boundingBox.minZ();
    const i32 maxZ = boundingBox.maxZ();
    const HeightmapType heightmapType = setup.placement == RuinedPortalSetupDefinition::Placement::OnOceanFloor
        ? HeightmapType::OceanFloorWG
        : HeightmapType::WorldSurfaceWG;

    // 逐层取四条完整地形列：判据要的是"该 Y 处是不是实心地形"，而非"该列最高方块在哪"。
    // 原版 `getBaseColumn(x, z, level, randomState)` 内部构造的 NoiseChunk 用的是
    // `DensityFunctions.BeardifierMarker.INSTANCE`（**不含**任何结构的地形贡献），
    // 这里不能擅自叠加结构贡献：STRUCTURE_STARTS 阶段回读本区块的 STRUCTURE_STARTS
    // 既违反阶段依赖（本区块还没推进到该状态），结果也与原版不同。
    const std::array<NoiseColumn, 4> cornerColumns = {generator.getBaseColumn(minX, minZ),
        generator.getBaseColumn(maxX, minZ),
        generator.getBaseColumn(minX, maxZ),
        generator.getBaseColumn(maxX, maxZ)};

    i32 scanY = y;
    for (; scanY > minScanY; --scanY) {
        i32 opaqueCount = 0;
        for (const auto& column : cornerColumns) {
            if (Heightmap::isOpaqueForType(heightmapType, column.getBlock(scanY))) {
                if (++opaqueCount == 3) {
                    return scanY;
                }
            }
        }
    }
    return scanY;
}

} // namespace

std::unique_ptr<StructureStart> RuinedPortalStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    return generate(generator, nullptr, rng, chunkX, chunkZ);
}

std::unique_ptr<StructureStart> RuinedPortalStructure::generate(
    IChunkGenerator& generator, WorldGenRegion* region, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    MC_ASSERT_RELEASE_MSG(!m_setups.empty(),
        fmt::format(
            "ruined_portal '{}' has an empty setups list; datapack must provide at least one setup", id().toString())
            .c_str());

    // ---- 1. 按 weight 抽取 setup（单条目时不消耗随机数）----
    const RuinedPortalSetupDefinition* setup = nullptr;
    if (m_setups.size() > 1) {
        f32 totalWeight = 0.0f;
        for (const auto& candidate : m_setups) {
            totalWeight += candidate.weight;
        }
        f32 roll = rng.nextFloat();
        for (const auto& candidate : m_setups) {
            roll -= candidate.weight / totalWeight;
            if (roll < 0.0f) {
                setup = &candidate;
                break;
            }
        }
        // 浮点累减可能取不到任何项（roll 恰好落在区间右端外）：退回最后一项并告警，
        // 保证不产出空结构。
        if (setup == nullptr) {
            setup = &m_setups.back();
            spdlog::warn("ruined_portal '{}': setup weighted draw did not select any entry; using the last one",
                id().toString());
        }
    } else {
        setup = &m_setups.front();
    }

    // ---- 2. 由 setup 派生属性 ----
    RuinedPortalProperties props;
    props.airPocket = _sampleProbability(rng, setup->airPocketProbability);
    props.mossiness = setup->mossiness;
    props.overgrown = setup->overgrown;
    props.vines = setup->vines;
    props.replaceWithBlackstone = setup->replaceWithBlackstone;

    // ---- 3. 抽模板（5% 巨型）----
    const std::string& templateName = (rng.nextFloat() < 0.05f)
        ? s_giantTemplates[static_cast<size_t>(rng.nextInt(static_cast<i32>(s_giantTemplates.size())))]
        : s_normalTemplates[static_cast<size_t>(rng.nextInt(static_cast<i32>(s_normalTemplates.size())))];

    feature::template_::TemplateManager* templateManager = m_templateManager;
    if (templateManager == nullptr) {
        templateManager = &jigsaw::JigsawAssembler::getTemplateManager();
    }
    const auto* templ = templateManager->getTemplate(ResourceLocation(templateName));
    MC_ASSERT_RELEASE_MSG(
        templ != nullptr, fmt::format("ruined_portal: template '{}' not found in datapacks", templateName).c_str());

    // ---- 4. 旋转 + 镜像 ----
    // 原版 `Util.getRandom(Rotation.values(), random)` 按 [NONE, CLOCKWISE_90, CLOCKWISE_180,
    // COUNTERCLOCKWISE_90] 取下标，与本项目 Rotation 枚举值一致，故直接 `nextInt(4)`。
    const Rotation rotation = static_cast<Rotation>(rng.nextInt(4));
    const Mirror mirror = rng.nextFloat() < 0.5f ? Mirror::None : Mirror::FrontBack;

    // ---- 5. 以区块原点为锚点求模板包围盒，再按包围盒中心取地表高度 ----
    const BlockPos chunkOrigin(chunkX * world::CHUNK_WIDTH, 0, chunkZ * world::CHUNK_WIDTH);
    const BlockPos pivot(templ->getSize().x / 2, 0, templ->getSize().z / 2);
    feature::template_::PlacementSettings pivotSettings;
    pivotSettings.setRotation(rotation);
    pivotSettings.setMirror(mirror);
    pivotSettings.setCenterOffset(pivot);
    const StructureBoundingBox boundingBox = templ->getBoundingBox(pivotSettings, chunkOrigin);

    // 原版 `BoundingBox.getCenter()` 用 `min + span / 2`，跨度按闭区间算（含 +1）。
    const i32 centerX = boundingBox.minX() + boundingBox.xSpan() / 2;
    const i32 centerZ = boundingBox.minZ() + boundingBox.zSpan() / 2;

    const HeightmapType heightmapType = setup->placement == RuinedPortalSetupDefinition::Placement::OnOceanFloor
        ? HeightmapType::OceanFloorWG
        : HeightmapType::WorldSurfaceWG;
    // getHeight 返回"最高方块上方一格"，减 1 得到方块本身 Y。
    const i32 surfaceBlockY = generator.getHeight(centerX, centerZ, heightmapType) - 1;

    const i32 y = _findSuitableY(rng, generator, region, *setup, props.airPocket, surfaceBlockY, boundingBox);

    const BlockPos portalPos(chunkOrigin.x, y, chunkOrigin.z);

    // ---- 6. cold 判定（仅 setup.can_be_cold 时采样，采样点取最终落点）----
    if (setup->canBeCold) {
        const BiomeId biomeId = generator.getBiomeAtBlock(portalPos.x, portalPos.y, portalPos.z);
        const Biome& biome = biome::BiomeRegistry::instance().get(biomeId);
        props.cold = biome.coldEnoughToSnow(portalPos.x, portalPos.y, portalPos.z, generator.seaLevel());
    }

    // ---- 7. 建片段 ----
    auto start = makeStructureStart(chunkX, chunkZ);
    auto piece = std::make_unique<RuinedPortalPiece>(
        templateName, portalPos, rotation, mirror, setup->placement, props, portalPos);
    piece->setTemplateManager(templateManager);
    // 起点包围盒在登记前就要有真实尺寸，否则退化成单点（模板本身由 generate 惰性复用同一份缓存）。
    piece->loadTemplateNow();
    MC_ASSERT_RELEASE_MSG(piece->hasLoadedTemplate(),
        fmt::format("ruined_portal: template '{}' not found in datapacks", templateName).c_str());

    // ---- 8. 生物群系校验（候选点 = 传送门落点）----
    // 原版在候选生成点求出后、构件装配前校验；落点依赖模板尺寸与随机流，
    // 只能在这里补上。不匹配即返回空起点，调用方据此重抽结构集条目。
    if (!isValidBiomeAt(generator, portalPos)) {
        return start;
    }

    start->addPiece(std::move(piece));
    start->recalculateStructureSize();
    return start;
}

} // namespace mc::world::gen::structure
