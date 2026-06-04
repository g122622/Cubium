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

#include "../../../../core/Constants.hpp"
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
#include <spdlog/spdlog.h>

namespace mc::world::gen::structure {

using namespace mc::Biomes;
using namespace mc::world; // 引入 CHUNK_WIDTH 等常量

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

// 主世界生物群系列表（包含所有变体）
const std::vector<BiomeId> RuinedPortalStructure::s_validBiomes = {Plains,
    Desert,
    DesertHills,
    DesertLakes,
    Forest,
    Taiga,
    Mountains,
    WoodedMountains,
    GravellyMountains,
    MountainEdge,
    SnowyMountains,
    SnowyPlains,
    Swamp,
    SwampHills,
    Badlands,
    WoodedBadlandsPlateau,
    BadlandsPlateau,
    ErodedBadlands,
    ModifiedWoodedBadlandsPlateau,
    ModifiedBadlandsPlateau,
    Jungle,
    JungleHills,
    JungleEdge,
    ModifiedJungle,
    ModifiedJungleEdge,
    BambooJungle,
    BambooJungleHills,
    Savanna,
    SavannaPlateau,
    ShatteredSavanna,
    ShatteredSavannaPlateau,
    BirchForest,
    BirchForestHills,
    TallBirchForest,
    TallBirchHills,
    DarkForest,
    DarkForestHills,
    WoodedHills,
    TaigaHills,
    TaigaMountains,
    SnowyTaiga,
    SnowyTaigaHills,
    SnowyTaigaMountains,
    GiantTreeTaiga,
    GiantTreeTaigaHills,
    GiantSpruceTaiga,
    GiantSpruceTaigaHills,
    River,
    Beach,
    SnowyBeach,
    StoneShore,
    LukewarmOcean,
    DeepLukewarmOcean,
    WarmOcean,
    DeepWarmOcean,
    ColdOcean,
    DeepColdOcean,
    FrozenOcean,
    DeepFrozenOcean,
    Ocean,
    DeepOcean,
    MushroomFields,
    MushroomFieldShore,
    SunflowerPlains,
    FlowerForest,
    IceSpikes};

// ============================================================================
// RuinedPortalPiece
// ============================================================================

RuinedPortalPiece::RuinedPortalPiece(const std::string& templateName,
    const BlockPos& position,
    Rotation rotation,
    Mirror mirror,
    RuinedPortalLocation location,
    const RuinedPortalProperties& properties)
    : StructurePiece(
          StructurePieceTypes::RUINED_PORTAL, position.x, position.y, position.z, position.x, position.y, position.z)
    , m_templateName(templateName)
    , m_rotation(rotation)
    , m_mirror(mirror)
    , m_location(location)
    , m_properties(properties)
    , m_size(1, 1, 1)
{}

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
    // 计算旋转后的尺寸
    i32 sizeX = m_size.x;
    i32 sizeZ = m_size.z;
    if (m_rotation == Rotation::Clockwise90 || m_rotation == Rotation::CounterClockwise90) {
        std::swap(sizeX, sizeZ);
    }

    // 应用镜像后更新边界框
    // 使用中心偏移定位
    m_minX = m_minX - m_centerOffset.x;
    m_minZ = m_minZ - m_centerOffset.z;
    m_maxX = m_minX + sizeX - 1;
    m_maxY = m_minY + m_size.y - 1;
    m_maxZ = m_minZ + sizeZ - 1;
}

void RuinedPortalPiece::generate(
    IWorldWriter& world, math::Random& rng, i32 /*chunkX*/, i32 /*chunkZ*/, const StructureBoundingBox& chunkBounds)
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

    // 创建放置设置
    feature::template_::PlacementSettings settings;
    settings.setRotation(m_rotation);
    settings.setMirror(m_mirror);
    settings.setCenterOffset(m_centerOffset);
    settings.setBoundingBox(&chunkBounds);

    // 添加处理器
    feature::template_::StructureProcessorList processors;

    // 方块忽略处理器
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

    // TODO: 以下处理器待完整实现
    // - RuleStructureProcessor: 金块随机替换为空气（0.2概率）
    // - BlockMossinessProcessor: 石砖随机苔藓化
    // - LavaSubmergingProcessor: 岩浆淹没处理
    // - BlackStoneReplacementProcessor: 下界传送门的黑石替换

    settings.setProcessors(&processors);

    // 放置模板，使用中心偏移，所以需要调整位置
    BlockPos adjustedPos(m_minX, m_minY, m_minZ);
    m_template->place(world, adjustedPos, settings, rng, 18);
}

// ============================================================================
// RuinedPortalStructure
// ============================================================================

RuinedPortalStructure::RuinedPortalStructure()
    : Structure(StructureType::RuinedPortal)
{}

RuinedPortalType RuinedPortalStructure::getPortalType(BiomeId biome)
{
    // 根据生物群系确定传送门类型
    if (biome == Desert || biome == DesertHills || biome == DesertLakes) {
        return RuinedPortalType::Desert;
    }
    if (biome == Jungle || biome == JungleHills || biome == JungleEdge || biome == ModifiedJungle ||
        biome == ModifiedJungleEdge) {
        return RuinedPortalType::Jungle;
    }
    if (biome == Swamp) {
        return RuinedPortalType::Swamp;
    }
    if (biome == Mountains || biome == WoodedMountains || biome == GravellyMountains || biome == MountainEdge ||
        biome == SnowyMountains) {
        return RuinedPortalType::Mountain;
    }
    if (biome == Ocean || biome == DeepOcean || biome == WarmOcean || biome == DeepWarmOcean ||
        biome == LukewarmOcean || biome == DeepLukewarmOcean || biome == ColdOcean || biome == DeepColdOcean ||
        biome == FrozenOcean || biome == DeepFrozenOcean) {
        return RuinedPortalType::Ocean;
    }
    // 下界生物群系由 DimensionType 判断，这里不处理
    return RuinedPortalType::Standard;
}

bool RuinedPortalStructure::canGenerate(
    IWorld& /*world*/, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    // 废弃传送门有概率检查
    // 使用间距设置检查是否应该在此位置生成
    i32 startX, startZ;
    if (!findStructureStart(static_cast<i64>(generator.seed()), chunkX, chunkZ, s_settings, startX, startZ)) {
        return false;
    }

    // 概率检查（约 30% 基础概率，具体由生物群系调整）
    return rng.nextFloat() < 0.3f;
}

RuinedPortalProperties RuinedPortalStructure::configureProperties(
    RuinedPortalType type, math::Random& rng, BiomeId biome) const
{
    RuinedPortalProperties props;

    // 根据类型配置属性
    switch (type) {
        case RuinedPortalType::Desert:
            // 沙漠: 部分掩埋，无空气口袋，无苔藓
            props.airPocket = false;
            props.mossiness = 0.0f;
            break;

        case RuinedPortalType::Jungle:
            // 丛林: 在地表，可能有空气口袋，高苔藓，过度生长，藤蔓
            props.airPocket = rng.nextFloat() < 0.5f;
            props.mossiness = 0.8f;
            props.overgrown = true;
            props.vines = true;
            break;

        case RuinedPortalType::Swamp:
            // 沼泽: 在海底，无空气口袋，中等苔藓，有藤蔓
            props.airPocket = false;
            props.mossiness = 0.5f;
            props.vines = true;
            break;

        case RuinedPortalType::Mountain:
            // 山地: 可能在山中或地表，有空气口袋
            props.airPocket = rng.nextFloat() < 0.5f || rng.nextFloat() < 0.5f;
            break;

        case RuinedPortalType::Ocean:
            // 海洋: 在海底，无空气口袋，高苔藓
            props.airPocket = false;
            props.mossiness = 0.8f;
            break;

        case RuinedPortalType::Nether:
            // 下界: 在下界高度，可能有空气口袋，无苔藓，替换为黑石
            props.airPocket = rng.nextFloat() < 0.5f;
            props.mossiness = 0.0f;
            props.replaceWithBlackstone = true;
            break;

        case RuinedPortalType::Standard:
        default:
            // 标准: 可能在地下或地表，有空气口袋
            props.airPocket = rng.nextFloat() < 0.5f || rng.nextFloat() < 0.5f;
            break;
    }

    // 如果是山地、海洋或标准类型，检查生物群系温度确定是否为寒冷
    if (type == RuinedPortalType::Mountain || type == RuinedPortalType::Ocean || type == RuinedPortalType::Standard) {
        // 简化处理：雪地生物群系为寒冷
        if (biome == SnowyPlains || biome == SnowyMountains || biome == SnowyBeach || biome == FrozenOcean ||
            biome == DeepFrozenOcean || biome == ColdOcean || biome == DeepColdOcean) {
            props.cold = true;
        }
    }

    return props;
}

RuinedPortalLocation RuinedPortalStructure::determineLocation(RuinedPortalType type, math::Random& rng) const
{
    // 根据类型确定垂直放置位置
    switch (type) {
        case RuinedPortalType::Desert:
            return RuinedPortalLocation::PartlyBuried;

        case RuinedPortalType::Jungle:
            return RuinedPortalLocation::OnLandSurface;

        case RuinedPortalType::Swamp:
            return RuinedPortalLocation::OnOceanFloor;

        case RuinedPortalType::Mountain:
            // 50% 在山中，50% 在地表
            return rng.nextFloat() < 0.5f ? RuinedPortalLocation::InMountain : RuinedPortalLocation::OnLandSurface;

        case RuinedPortalType::Ocean:
            return RuinedPortalLocation::OnOceanFloor;

        case RuinedPortalType::Nether:
            return RuinedPortalLocation::InNether;

        case RuinedPortalType::Standard:
        default:
            // 50% 地下，50% 地表
            return rng.nextFloat() < 0.5f ? RuinedPortalLocation::Underground : RuinedPortalLocation::OnLandSurface;
    }
}

std::unique_ptr<StructureStart> RuinedPortalStructure::generate(
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    MC_UNUSED(world);

    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算生成位置
    i32 x = chunkX * CHUNK_WIDTH + rng.nextInt(CHUNK_WIDTH);
    i32 z = chunkZ * CHUNK_WIDTH + rng.nextInt(CHUNK_WIDTH);

    // 获取生物群系并确定传送门类型
    BiomeId biome = generator.getBiome(x, SEA_LEVEL, z);
    RuinedPortalType portalType = getPortalType(biome);

    // 确定属性和位置
    RuinedPortalProperties props = configureProperties(portalType, rng, biome);
    RuinedPortalLocation location = determineLocation(portalType, rng);

    // 5% 概率选择巨型传送门
    const std::string* templateName = nullptr;
    if (rng.nextFloat() < 0.05f && !s_giantTemplates.empty()) {
        const size_t index = static_cast<size_t>(rng.nextInt(static_cast<i32>(s_giantTemplates.size())));
        templateName = &s_giantTemplates[index];
    } else {
        const size_t index = static_cast<size_t>(rng.nextInt(static_cast<i32>(s_normalTemplates.size())));
        templateName = &s_normalTemplates[index];
    }

    // 随机旋转和镜像
    Rotation rotation = static_cast<Rotation>(rng.nextInt(4));
    Mirror mirror = rng.nextFloat() < 0.5f ? Mirror::None : Mirror::FrontBack;

    // 确定高度
    i32 y = 0;
    HeightmapType heightmapType =
        (location == RuinedPortalLocation::OnOceanFloor) ? HeightmapType::OceanFloorWG : HeightmapType::WorldSurfaceWG;

    switch (location) {
        case RuinedPortalLocation::InNether:
            // 下界: Y 32-100，大型传送门在 32-100
            if (*templateName == s_giantTemplates[0] || *templateName == s_giantTemplates[1] ||
                *templateName == s_giantTemplates[2]) {
                y = 32 + rng.nextInt(69); // 32-100
            } else if (rng.nextFloat() < 0.5f) {
                y = 27 + rng.nextInt(3); // 27-29
            } else {
                y = 29 + rng.nextInt(72); // 29-100
            }
            break;

        case RuinedPortalLocation::InMountain:
        case RuinedPortalLocation::Underground: {
            i32 surfaceY = generator.getHeight(x, z, heightmapType);
            if (location == RuinedPortalLocation::InMountain) {
                // 山中: 地表高度附近
                y = 70 + rng.nextInt(std::max(1, surfaceY - 70));
            } else {
                // 地下: Y 15 到地表
                y = 15 + rng.nextInt(std::max(1, surfaceY - 15));
            }
        } break;

        case RuinedPortalLocation::PartlyBuried: {
            i32 surfaceY = generator.getHeight(x, z, heightmapType);
            y = surfaceY + rng.nextInt(7) + 2; // 地表上方 2-8 格
        } break;

        case RuinedPortalLocation::OnLandSurface:
        case RuinedPortalLocation::OnOceanFloor:
        default:
            y = generator.getHeight(x, z, heightmapType);
            if (y <= 0) {
                y = generator.seaLevel();
            }
            break;
    }

    // 获取模板管理器
    feature::template_::TemplateManager* templateManager = m_templateManager;
    if (!templateManager) {
        templateManager = &jigsaw::JigsawManager::getTemplateManager();
    }

    // 创建片段
    auto piece =
        std::make_unique<RuinedPortalPiece>(*templateName, BlockPos(x, y, z), rotation, mirror, location, props);
    piece->setTemplateManager(templateManager);

    start->addPiece(std::move(piece));
    start->recalculateStructureSize();
    return start;
}

} // namespace mc::world::gen::structure
