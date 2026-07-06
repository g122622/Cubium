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
 */

#include "LushCavesFeatures.hpp"
#include "CaveFeatureConfigs.hpp"
#include "CaveFeatures.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/FeatureIds.hpp"
#include "common/world/gen/feature/FeatureSpread.hpp"
#include "common/world/gen/feature/predicate/BlockPredicate.hpp"
#include "common/world/gen/feature/state/WeightedBlockStateProvider.hpp"
#include "common/world/gen/feature/tree/TreeFeature.hpp"
#include "common/world/gen/feature/tree/foliage/RandomSpreadFoliagePlacer.hpp"
#include "common/world/gen/feature/tree/trunk/BendingTrunkPlacer.hpp"
#include "common/world/gen/placement/BiomeFilterPlacement.hpp"
#include "common/world/gen/placement/EnvironmentScanPlacement.hpp"
#include "common/world/gen/placement/Placement.hpp"
#include "common/world/gen/placement/PlacementUtils.hpp"
#include "common/world/gen/placement/Placements.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"

namespace mc {

using namespace world::gen::feature::cave;
using namespace world::gen::valueprovider;
using namespace world::gen::feature::predicate;
namespace blockstate = world::gen::feature::state;

std::vector<std::unique_ptr<ConfiguredFeatureBase>> LushCavesFeatures::s_features;

// ============================================================================
// 辅助：创建大型垂滴叶
// ============================================================================

namespace {

std::unique_ptr<ConfiguredFeatureBase> createBigDripleaf(Direction facing, const char* name)
{
    auto config = std::make_unique<BlockColumnConfig>();
    config->direction = Direction::Up;
    config->prioritizeTip = true;

    // stem层 - 高度0~4
    auto stemHeight = std::make_unique<UniformInt>(0, 4);
    const BlockState* stemState = VanillaBlocks::getState(VanillaBlocks::BIG_DRIPLEAF_STEM);
    if (stemState != nullptr && stemState->hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
        stemState = &stemState->with(BlockStateProperties::HORIZONTAL_FACING(), facing);
    }
    config->layers.push_back(BlockColumnLayer(std::move(stemHeight), stemState));

    // leaf层 - 高度1
    auto leafHeight = std::make_unique<ConstantInt>(1);
    const BlockState* leafState = VanillaBlocks::getState(VanillaBlocks::BIG_DRIPLEAF);
    if (leafState != nullptr && leafState->hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
        leafState = &leafState->with(BlockStateProperties::HORIZONTAL_FACING(), facing);
    }
    config->layers.push_back(BlockColumnLayer(std::move(leafHeight), leafState));

    config->allowedPlacement = std::make_unique<OnlyInAirOrWaterPredicate>();

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::make_unique<CountPlacement>(), std::make_unique<CountPlacementConfig>(1));

    return std::make_unique<ConfiguredBlockColumnFeature>(std::move(config), std::move(placement), name);
}

} // anonymous namespace

// ============================================================================
// 子特征创建
// ============================================================================

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createMossVegetation()
{
    // 苔藓植被 - 简单方块放置
    // MC: WeightedStateProvider(杜鹃花丛4, 开花杜鹃花丛7, 苔藓地毯25, 草50, 高草10)
    // 简化：使用苔藓地毯作为主要植被
    auto config = std::make_unique<SimpleBlockConfig>(VanillaBlocks::getState(VanillaBlocks::MOSS_CARPET));

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::make_unique<CountPlacement>(), std::make_unique<CountPlacementConfig>(1));

    return std::make_unique<ConfiguredSimpleBlockFeature>(std::move(config), std::move(placement), "moss_vegetation");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createCaveVineInMoss()
{
    // 苔藓中的洞穴藤蔓 - 较短的藤蔓
    // MC 1.21.11: body高度 WeightedListInt(UniformInt(0,3) w:5, UniformInt(1,7) w:1)
    // body方块: WeightedStateProvider(CAVE_VINES_PLANT w:4, CAVE_VINES_PLANT[berries=true] w:1)
    auto config = std::make_unique<BlockColumnConfig>();
    config->direction = Direction::Down;
    config->prioritizeTip = true;

    // body层 - 加权随机高度 + 加权随机方块状态（含浆果变体）
    std::vector<WeightedListInt::WeightedEntry> mossBodyEntries;
    mossBodyEntries.push_back({std::make_unique<UniformInt>(0, 3), 5});
    mossBodyEntries.push_back({std::make_unique<UniformInt>(1, 7), 1});
    auto bodyHeight = std::make_unique<WeightedListInt>(std::move(mossBodyEntries));

    auto bodyStates = std::make_unique<blockstate::WeightedBlockStateProvider>();
    bodyStates->add(VanillaBlocks::getState(VanillaBlocks::CAVE_VINES_PLANT), 4);
    bodyStates->add(&VanillaBlocks::CAVE_VINES_PLANT->defaultState().with(BlockStateProperties::BERRIES(), true), 1);
    config->layers.push_back(BlockColumnLayer(std::move(bodyHeight), std::move(bodyStates)));

    // tip层 - 高度1
    auto tipHeight = std::make_unique<ConstantInt>(1);
    config->layers.push_back(
        BlockColumnLayer(std::move(tipHeight), VanillaBlocks::getState(VanillaBlocks::CAVE_VINES)));

    config->allowedPlacement = std::make_unique<OnlyInAirPredicate>();

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::make_unique<CountPlacement>(), std::make_unique<CountPlacementConfig>(1));

    return std::make_unique<ConfiguredBlockColumnFeature>(std::move(config), std::move(placement), "cave_vine_in_moss");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createClayWithDripleaves()
{
    auto config =
        std::make_unique<VegetationPatchConfig>(VegetationPatchConfig::floorPatch("minecraft:lush_ground_replaceable",
            VanillaBlocks::getState(VanillaBlocks::CLAY),
            LushCaveFeatureIds::Dripleaf,
            std::make_unique<ConstantInt>(3),
            0.8f,
            2,
            0.05f,
            std::make_unique<UniformInt>(4, 7),
            0.7f));

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::make_unique<CountPlacement>(), std::make_unique<CountPlacementConfig>(1));

    return std::make_unique<ConfiguredVegetationPatchFeature>(
        std::move(config), std::move(placement), "clay_with_dripleaves");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createClayPoolWithDripleaves()
{
    auto config =
        std::make_unique<VegetationPatchConfig>(VegetationPatchConfig::floorPatch("minecraft:lush_ground_replaceable",
            VanillaBlocks::getState(VanillaBlocks::CLAY),
            LushCaveFeatureIds::Dripleaf,
            std::make_unique<ConstantInt>(3),
            0.8f,
            5,
            0.1f,
            std::make_unique<UniformInt>(4, 7),
            0.7f));

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::make_unique<CountPlacement>(), std::make_unique<CountPlacementConfig>(1));

    return std::make_unique<ConfiguredWaterloggedPatchFeature>(
        std::move(config), std::move(placement), "clay_pool_with_dripleaves");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createSmallDripleaf()
{
    // 小型垂滴叶 - SimpleBlockFeature（可在水中放置）
    auto config = std::make_unique<SimpleBlockConfig>(VanillaBlocks::getState(VanillaBlocks::SMALL_DRIPLEAF));

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::make_unique<CountPlacement>(), std::make_unique<CountPlacementConfig>(1));

    return std::make_unique<ConfiguredSimpleBlockFeature>(std::move(config), std::move(placement), "small_dripleaf");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createBigDripleafNorth()
{
    return createBigDripleaf(Direction::North, "big_dripleaf_north");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createBigDripleafSouth()
{
    return createBigDripleaf(Direction::South, "big_dripleaf_south");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createBigDripleafWest()
{
    return createBigDripleaf(Direction::West, "big_dripleaf_west");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createBigDripleafEast()
{
    return createBigDripleaf(Direction::East, "big_dripleaf_east");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createDripleaf()
{
    auto config = std::make_unique<SimpleRandomFeatureConfig>();
    config->featureIds.push_back(LushCaveFeatureIds::SmallDripleaf);
    config->featureIds.push_back(LushCaveFeatureIds::BigDripleafNorth);
    config->featureIds.push_back(LushCaveFeatureIds::BigDripleafSouth);
    config->featureIds.push_back(LushCaveFeatureIds::BigDripleafWest);
    config->featureIds.push_back(LushCaveFeatureIds::BigDripleafEast);

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::make_unique<CountPlacement>(), std::make_unique<CountPlacementConfig>(1));

    return std::make_unique<ConfiguredSimpleRandomSelectorFeature>(std::move(config), std::move(placement), "dripleaf");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createAzaleaTree()
{
    // 杜鹃树 - 使用 BendingTrunkPlacer + RandomSpreadFoliagePlacer
    // 配置对齐 MC 1.21.11 azalea_tree.json：
    //   - trunk: BendingTrunkPlacer(4, 2, 0, 3, UniformInt(1, 2))，trunk_provider=oak_log
    //   - foliage: RandomSpreadFoliagePlacer(radius=3, offset=0, foliageHeight=2, leafPlacementAttempts=50)
    //   - foliage_provider: WeightedStateProvider(azalea_leaves:3, flowering_azalea_leaves:1)
    //   - force_dirt=true（项目对应 forcePlacement=true），dirt_provider=rooted_dirt（项目暂未实现 dirtProvider）
    auto trunkPlacer = std::make_unique<BendingTrunkPlacer>(4, 2, 0, 3, std::make_unique<UniformInt>(1, 2));
    auto foliagePlacer = std::make_unique<RandomSpreadFoliagePlacer>(FeatureSpread::fixed(3), // radius
        FeatureSpread::fixed(0),                                                              // offset
        std::make_unique<ConstantInt>(2),                                                     // foliageHeight
        50                                                                                    // leafPlacementAttempts
    );

    auto config = std::make_unique<TreeFeatureConfig>();
    config->trunkBlock = VanillaBlocks::getState(VanillaBlocks::OAK_LOG);
    // 加权树叶提供者：杜鹃叶 3 : 开花杜鹃叶 1（每个叶片独立采样）
    config->foliageProvider = std::make_unique<blockstate::WeightedBlockStateProvider>();
    config->foliageProvider->add(VanillaBlocks::getState(VanillaBlocks::AZALEA_LEAVES), 3);
    config->foliageProvider->add(VanillaBlocks::getState(VanillaBlocks::FLOWERING_AZALEA_LEAVES), 1);
    // 同时设置 foliageBlock 作为兜底（provider 为空时使用），保持配置完整性
    config->foliageBlock = VanillaBlocks::getState(VanillaBlocks::AZALEA_LEAVES);
    config->trunkPlacer = std::move(trunkPlacer);
    config->foliagePlacer = std::move(foliagePlacer);
    config->forcePlacement = true;
    config->minHeight = 4;

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::make_unique<CountPlacement>(), std::make_unique<CountPlacementConfig>(1));

    return std::make_unique<ConfiguredTreeFeature>(std::move(config), std::move(placement), "azalea_tree");
}

// ============================================================================
// 主特征创建
// ============================================================================

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createLushCavesVegetation()
{
    // 苔藓地面贴片 - VegetationPatchFeature, FLOOR
    // MC 1.21.11: Count(125) -> Square -> HeightRange -> EnvironmentScan(DOWN, hasSturdyFace(UP), onlyInAir, 12) ->
    // RandomOffset(vertical, +1) -> BiomeFilter
    auto config =
        std::make_unique<VegetationPatchConfig>(VegetationPatchConfig::floorPatch("minecraft:moss_replaceable",
            VanillaBlocks::getState(VanillaBlocks::MOSS_BLOCK),
            LushCaveFeatureIds::MossVegetation,
            std::make_unique<ConstantInt>(1),
            0.0f,
            5,
            0.8f,
            std::make_unique<UniformInt>(4, 7),
            0.3f));

    // Count(125) -> Square -> HeightRange(-64, 320)
    auto placement = PlacementUtils::createCountedHeightPlacement(125, -64, 320);
    // EnvironmentScan(DOWN, hasSturdyFace(UP), onlyInAir, 12) — 向下扫描寻找地面
    placement = PlacementUtils::appendEnvironmentScanDown(std::move(placement), 12);
    // RandomOffset(vertical, +1) — 地面上方1格
    placement = PlacementUtils::appendVerticalOffset(std::move(placement), 1);
    // BiomeFilter — 只在 lush_caves 群系放置
    placement = PlacementUtils::appendBiomeFilter(std::move(placement), LushCaveFeatureIds::LushCavesVegetation);

    return std::make_unique<ConfiguredVegetationPatchFeature>(
        std::move(config), std::move(placement), "lush_caves_vegetation");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createLushCavesCeilingVegetation()
{
    // 苔藓天花板贴片 - VegetationPatchFeature, CEILING
    // MC 1.21.11: Count(125) -> Square -> HeightRange -> EnvironmentScan(UP, hasSturdyFace(DOWN), onlyInAir, 12) ->
    // RandomOffset(vertical, -1) -> BiomeFilter
    auto config =
        std::make_unique<VegetationPatchConfig>(VegetationPatchConfig::ceilingPatch("minecraft:moss_replaceable",
            VanillaBlocks::getState(VanillaBlocks::MOSS_BLOCK),
            LushCaveFeatureIds::CaveVineInMoss,
            std::make_unique<UniformInt>(1, 2),
            0.0f,
            5,
            0.08f,
            std::make_unique<UniformInt>(4, 7),
            0.3f));

    // Count(125) -> Square -> HeightRange(-64, 320)
    auto placement = PlacementUtils::createCountedHeightPlacement(125, -64, 320);
    // EnvironmentScan(UP, hasSturdyFace(DOWN), onlyInAir, 12) — 向上扫描寻找天花板
    placement = PlacementUtils::appendEnvironmentScanUp(std::move(placement), 12);
    // RandomOffset(vertical, -1) — 天花板下方1格
    placement = PlacementUtils::appendVerticalOffset(std::move(placement), -1);
    // BiomeFilter
    placement = PlacementUtils::appendBiomeFilter(std::move(placement), LushCaveFeatureIds::LushCavesCeilingVegetation);

    return std::make_unique<ConfiguredVegetationPatchFeature>(
        std::move(config), std::move(placement), "lush_caves_ceiling_vegetation");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createCaveVines()
{
    // 洞穴藤蔓 - BlockColumnFeature, DOWN
    // MC 1.21.11: body高度 WeightedListInt(UniformInt(0,19) w:2, UniformInt(0,2) w:3, UniformInt(0,6) w:10)
    // body方块: WeightedStateProvider(CAVE_VINES_PLANT w:4, CAVE_VINES_PLANT[berries=true] w:1)
    // Count(188) -> Square -> HeightRange -> EnvironmentScan(UP, hasSturdyFace(DOWN), onlyInAir, 12) ->
    // RandomOffset(vertical, -1) -> BiomeFilter
    auto config = std::make_unique<BlockColumnConfig>();
    config->direction = Direction::Down;
    config->prioritizeTip = true;

    // body层 - 加权随机高度 + 加权随机方块状态（含浆果变体）
    std::vector<WeightedListInt::WeightedEntry> bodyEntries;
    bodyEntries.push_back({std::make_unique<UniformInt>(0, 19), 2});
    bodyEntries.push_back({std::make_unique<UniformInt>(0, 2), 3});
    bodyEntries.push_back({std::make_unique<UniformInt>(0, 6), 10});
    auto bodyHeight = std::make_unique<WeightedListInt>(std::move(bodyEntries));

    auto bodyStates = std::make_unique<blockstate::WeightedBlockStateProvider>();
    bodyStates->add(VanillaBlocks::getState(VanillaBlocks::CAVE_VINES_PLANT), 4);
    bodyStates->add(&VanillaBlocks::CAVE_VINES_PLANT->defaultState().with(BlockStateProperties::BERRIES(), true), 1);
    config->layers.push_back(BlockColumnLayer(std::move(bodyHeight), std::move(bodyStates)));

    // tip层 - 高度1
    auto tipHeight = std::make_unique<ConstantInt>(1);
    config->layers.push_back(
        BlockColumnLayer(std::move(tipHeight), VanillaBlocks::getState(VanillaBlocks::CAVE_VINES)));

    config->allowedPlacement = std::make_unique<OnlyInAirPredicate>();

    // Count(188) -> Square -> HeightRange(-64, 320)
    auto placement = PlacementUtils::createCountedHeightPlacement(188, -64, 320);
    // EnvironmentScan(UP, hasSturdyFace(DOWN), onlyInAir, 12) — 向上扫描寻找天花板坚固面
    placement = PlacementUtils::appendEnvironmentScanUp(std::move(placement), 12);
    // RandomOffset(vertical, -1) — 天花板下方1格
    placement = PlacementUtils::appendVerticalOffset(std::move(placement), -1);
    // BiomeFilter
    placement = PlacementUtils::appendBiomeFilter(std::move(placement), LushCaveFeatureIds::CaveVines);

    return std::make_unique<ConfiguredBlockColumnFeature>(std::move(config), std::move(placement), "cave_vines");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createLushCavesClay()
{
    // 黏土池 - RandomBooleanSelectorFeature
    // MC 1.21.11: Count(62) -> Square -> HeightRange -> EnvironmentScan(DOWN, hasSturdyFace(UP), onlyInAir, 12) ->
    // RandomOffset(vertical, +1) -> BiomeFilter
    auto config = std::make_unique<RandomBooleanFeatureConfig>(
        LushCaveFeatureIds::ClayWithDripleaves, LushCaveFeatureIds::ClayPoolWithDripleaves);

    // Count(62) -> Square -> HeightRange(-64, 320)
    auto placement = PlacementUtils::createCountedHeightPlacement(62, -64, 320);
    // EnvironmentScan(DOWN, hasSturdyFace(UP), onlyInAir, 12) — 向下扫描寻找地面
    placement = PlacementUtils::appendEnvironmentScanDown(std::move(placement), 12);
    // RandomOffset(vertical, +1) — 地面上方1格
    placement = PlacementUtils::appendVerticalOffset(std::move(placement), 1);
    // BiomeFilter
    placement = PlacementUtils::appendBiomeFilter(std::move(placement), LushCaveFeatureIds::LushCavesClay);

    return std::make_unique<ConfiguredRandomBooleanSelectorFeature>(
        std::move(config), std::move(placement), "lush_caves_clay");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createRootedAzaleaTree()
{
    // 杜鹃树根系统 - RootSystemFeature
    // MC 1.21.11: CountWithProvider(UniformInt(1,2)) -> Square -> HeightRange -> EnvironmentScan(UP,
    // hasSturdyFace(DOWN), onlyInAir, 12) -> RandomOffset(vertical, -1) -> BiomeFilter
    auto config = std::make_unique<RootSystemConfig>();
    config->treeFeatureId = LushCaveFeatureIds::AzaleaTree;
    config->requiredVerticalSpaceForTree = 3;
    config->rootRadius = 3;
    config->rootReplaceableTag = "minecraft:azalea_root_replaceable";
    config->rootState = VanillaBlocks::getState(VanillaBlocks::ROOTED_DIRT);
    config->rootPlacementAttempts = 20;
    config->rootColumnMaxHeight = 100;
    config->hangingRootRadius = 3;
    config->hangingRootsVerticalSpan = 2;
    config->hangingRootState = VanillaBlocks::getState(VanillaBlocks::HANGING_ROOTS);
    config->hangingRootPlacementAttempts = 20;
    config->allowedVerticalWaterForTree = 2;

    // CountWithProvider(UniformInt(1,2)) -> Square -> HeightRange(-64, 320)
    auto heightPlacement = std::make_unique<HeightRangePlacement>();
    auto heightConfig = std::make_unique<HeightRangePlacementConfig>(-64, 0, 320);
    auto heightConfigured = std::make_unique<ConfiguredPlacement>(std::move(heightPlacement), std::move(heightConfig));

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(std::move(squarePlacement), std::move(squareConfig));
    squareConfigured->setNext(std::move(heightConfigured));

    auto countProvider = std::make_unique<UniformInt>(1, 2);
    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountWithProviderConfig>(std::move(countProvider));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(std::move(countPlacement), std::move(countConfig));
    countConfigured->setNext(std::move(squareConfigured));

    // EnvironmentScan(UP, hasSturdyFace(DOWN), onlyInAir, 12) — 向上扫描寻找天花板
    auto placement = PlacementUtils::appendEnvironmentScanUp(std::move(countConfigured), 12);
    // RandomOffset(vertical, -1) — 天花板下方1格
    placement = PlacementUtils::appendVerticalOffset(std::move(placement), -1);
    // BiomeFilter
    placement = PlacementUtils::appendBiomeFilter(std::move(placement), LushCaveFeatureIds::RootedAzaleaTree);

    return std::make_unique<ConfiguredRootSystemFeature>(std::move(config), std::move(placement), "rooted_azalea_tree");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createSporeBlossom()
{
    // 孢子花 - SimpleBlockFeature
    // MC 1.21.11: Count(25) -> Square -> HeightRange -> EnvironmentScan(UP, hasSturdyFace(DOWN), onlyInAir, 12) ->
    // RandomOffset(vertical, -1) -> BiomeFilter
    auto config = std::make_unique<SimpleBlockConfig>(VanillaBlocks::getState(VanillaBlocks::SPORE_BLOSSOM));

    // Count(25) -> Square -> HeightRange(-64, 320)
    auto placement = PlacementUtils::createCountedHeightPlacement(25, -64, 320);
    // EnvironmentScan(UP, hasSturdyFace(DOWN), onlyInAir, 12) — 向上扫描寻找天花板
    placement = PlacementUtils::appendEnvironmentScanUp(std::move(placement), 12);
    // RandomOffset(vertical, -1) — 天花板下方1格
    placement = PlacementUtils::appendVerticalOffset(std::move(placement), -1);
    // BiomeFilter
    placement = PlacementUtils::appendBiomeFilter(std::move(placement), LushCaveFeatureIds::SporeBlossom);

    return std::make_unique<ConfiguredSimpleBlockFeature>(std::move(config), std::move(placement), "spore_blossom");
}

std::unique_ptr<ConfiguredFeatureBase> LushCavesFeatures::createClassicVines()
{
    // 经典藤蔓 - SimpleBlockFeature
    // MC 1.21.11: Count(256) -> Square -> HeightRange -> BiomeFilter（ClassicVines 无 EnvironmentScan）
    const BlockState* vineState = VanillaBlocks::getState(VanillaBlocks::VINE);
    auto config = std::make_unique<SimpleBlockConfig>(vineState);

    // Count(256) -> Square -> HeightRange(-64, 320)
    auto placement = PlacementUtils::createCountedHeightPlacement(256, -64, 320);
    // BiomeFilter — 只在 lush_caves 群系放置
    placement = PlacementUtils::appendBiomeFilter(std::move(placement), LushCaveFeatureIds::ClassicVines);

    return std::make_unique<ConfiguredSimpleBlockFeature>(std::move(config), std::move(placement), "classic_vines");
}

// ============================================================================
// 初始化
// ============================================================================

void LushCavesFeatures::initialize()
{
    s_features.clear();

    // 注册顺序必须与 FeatureIds.hpp 中 LushCaveFeatureIds 一致
    // Offset + 0: MossVegetation
    s_features.push_back(createMossVegetation());
    // Offset + 1: CaveVineInMoss
    s_features.push_back(createCaveVineInMoss());
    // Offset + 2: ClayWithDripleaves
    s_features.push_back(createClayWithDripleaves());
    // Offset + 3: ClayPoolWithDripleaves
    s_features.push_back(createClayPoolWithDripleaves());
    // Offset + 4: Dripleaf
    s_features.push_back(createDripleaf());
    // Offset + 5: SmallDripleaf
    s_features.push_back(createSmallDripleaf());
    // Offset + 6: BigDripleafNorth
    s_features.push_back(createBigDripleafNorth());
    // Offset + 7: BigDripleafSouth
    s_features.push_back(createBigDripleafSouth());
    // Offset + 8: BigDripleafWest
    s_features.push_back(createBigDripleafWest());
    // Offset + 9: BigDripleafEast
    s_features.push_back(createBigDripleafEast());
    // Offset + 10: AzaleaTree
    s_features.push_back(createAzaleaTree());
    // Offset + 11: SporeBlossom
    s_features.push_back(createSporeBlossom());
    // Offset + 12: ClassicVines
    s_features.push_back(createClassicVines());
    // Offset + 13: LushCavesVegetation (主特征)
    s_features.push_back(createLushCavesVegetation());
    // Offset + 14: LushCavesCeilingVegetation (主特征)
    s_features.push_back(createLushCavesCeilingVegetation());
    // Offset + 15: CaveVines (主特征)
    s_features.push_back(createCaveVines());
    // Offset + 16: LushCavesClay (主特征)
    s_features.push_back(createLushCavesClay());
    // Offset + 17: RootedAzaleaTree (主特征)
    s_features.push_back(createRootedAzaleaTree());
}

std::vector<std::unique_ptr<ConfiguredFeatureBase>> LushCavesFeatures::getAllFeaturesAndClear()
{
    return std::move(s_features);
}

} // namespace mc
