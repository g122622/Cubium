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

#include "TreeFeature.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/CaveBlocks.hpp"
#include "common/world/block/registry/CherryBlocks.hpp"
#include "common/world/block/registry/PaleGardenBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/FeatureSpread.hpp"
#include "common/world/gen/feature/state/WeightedBlockStateProvider.hpp"
#include "common/world/gen/feature/tree/featuresize/FeatureSize.hpp"
#include "common/world/gen/feature/tree/trunk/BendingTrunkPlacer.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include "foliage/BlobFoliagePlacer.hpp"
#include "foliage/CherryFoliagePlacer.hpp"
#include "foliage/FoliagePlacers.hpp"
#include "foliage/RandomSpreadFoliagePlacer.hpp"
#include "trunk/CherryTrunkPlacer.hpp"
#include "trunk/StraightTrunkPlacer.hpp"
#include "trunk/TrunkPlacers.hpp"
#include <algorithm>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// TreeFeature 实现（保持原有实现）
// ============================================================================

bool TreeFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& startPos, const TreeFeatureConfig& config)
{
    if (config.trunkPlacer == nullptr || config.foliagePlacer == nullptr) {
        return false;
    }

    // 获取树干高度
    i32 trunkHeight = config.trunkPlacer->getHeight(random);

    // 检查高度是否有效
    if (trunkHeight < config.minHeight) {
        return false;
    }

    // 检查起始位置是否在有效范围内
    if (startPos.y < world::MIN_BUILD_HEIGHT + 1 || startPos.y + trunkHeight + 1 >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    // 检查起始位置下方是否是泥土或草地
    if (!isDirtOrFarmlandAt(world, startPos.down())) {
        return false;
    }

    // 检查是否有足够的空间放置树干。
    // forcePlacement=true 时跳过空间约束，用于部分特例树木配置。
    // 对应 MC 1.21.11 TreeFeature.doPlace：
    //   OptionalInt optionalint = minimumSize.minClippedHeight();
    //   int k1 = getMaxFreeTreeHeight(...);
    //   if (k1 >= i || (!optionalint.isEmpty() && k1 >= optionalint.getAsInt())) { ... }
    //   else return false;
    if (!config.forcePlacement) {
        i32 availableHeight = _calculateAvailableHeight(world, trunkHeight, startPos, config);
        if (availableHeight < trunkHeight) {
            // 空间不足：若配置了 minClippedHeight，且实际可用高度 >= 该值，
            // 仍允许以裁剪后的高度生成（用于 fancy_oak 等容忍较矮空间的配置）。
            i32 clippedHeight = availableHeight;
            bool allowClipped = false;
            if (config.minimumSize) {
                auto minClippedOpt = config.minimumSize->minClippedHeight();
                if (minClippedOpt.has_value() && clippedHeight >= minClippedOpt.value()) {
                    allowClipped = true;
                }
            }
            if (!allowClipped) {
                return false;
            }
            // 使用裁剪后的高度继续生成
            trunkHeight = clippedHeight;
            // 裁剪后仍需满足最小高度要求
            if (trunkHeight < config.minHeight) {
                return false;
            }
        }
    }

    // 放置树干
    std::set<BlockPos> trunkBlocks;
    std::vector<FoliagePosition> foliagePositions =
        config.trunkPlacer->placeTrunk(world, random, trunkHeight, startPos, trunkBlocks, config.trunkBlock);

    if (foliagePositions.empty()) {
        return false;
    }

    // 放置树叶
    std::set<BlockPos> foliageBlocks;
    config.foliagePlacer->placeFoliage(world,
        random,
        trunkHeight,
        foliagePositions,
        trunkBlocks,
        trunkHeight - 1,
        config.foliageBlock,
        config.foliageProvider.get(),
        foliageBlocks);

    // 设置树叶距离属性（用于树叶腐烂机制）
    _setFoliageDistance(world, trunkBlocks, foliageBlocks);

    return true;
}

bool TreeFeature::isReplaceableAt(WorldGenRegion& world, const BlockPos& pos)
{
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (state == nullptr || state->isAir()) {
        return true;
    }

    // 检查是否是树叶
    if (state->is(VanillaBlocks::OAK_LEAVES) || state->is(VanillaBlocks::SPRUCE_LEAVES) ||
        state->is(VanillaBlocks::BIRCH_LEAVES) || state->is(VanillaBlocks::JUNGLE_LEAVES) ||
        state->is(VanillaBlocks::ACACIA_LEAVES) || state->is(VanillaBlocks::DARK_OAK_LEAVES) ||
        state->is(block_registry::CherryBlocks::CHERRY_LEAVES)) {
        return true;
    }

    // 检查是否是植被
    if (state->is(VanillaBlocks::SHORT_GRASS) || state->is(VanillaBlocks::TALL_GRASS) ||
        state->is(VanillaBlocks::FERN) || state->is(VanillaBlocks::DANDELION) || state->is(VanillaBlocks::POPPY) ||
        state->is(VanillaBlocks::OAK_SAPLING) || state->is(VanillaBlocks::SPRUCE_SAPLING) ||
        state->is(VanillaBlocks::BIRCH_SAPLING) || state->is(VanillaBlocks::JUNGLE_SAPLING) ||
        state->is(VanillaBlocks::ACACIA_SAPLING) || state->is(VanillaBlocks::DARK_OAK_SAPLING) ||
        state->is(block_registry::CherryBlocks::CHERRY_SAPLING)) {
        return true;
    }

    // 检查是否是水
    if (state->is(VanillaBlocks::WATER)) {
        return true;
    }

    return false;
}

bool TreeFeature::isAirOrLeavesAt(WorldGenRegion& world, const BlockPos& pos)
{
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (state == nullptr || state->isAir()) {
        return true;
    }

    // 检查是否是树叶
    if (state->is(VanillaBlocks::OAK_LEAVES) || state->is(VanillaBlocks::SPRUCE_LEAVES) ||
        state->is(VanillaBlocks::BIRCH_LEAVES) || state->is(VanillaBlocks::JUNGLE_LEAVES) ||
        state->is(VanillaBlocks::ACACIA_LEAVES) || state->is(VanillaBlocks::DARK_OAK_LEAVES) ||
        state->is(block_registry::CherryBlocks::CHERRY_LEAVES)) {
        return true;
    }

    return false;
}

bool TreeFeature::isDirtOrFarmlandAt(WorldGenRegion& world, const BlockPos& pos)
{
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (state == nullptr) {
        return false;
    }

    // 检查是否是泥土类方块或耕地
    return state->is(VanillaBlocks::DIRT) || state->is(VanillaBlocks::GRASS_BLOCK) ||
        state->is(VanillaBlocks::COARSE_DIRT) || state->is(VanillaBlocks::PODZOL) || state->is(VanillaBlocks::FARMLAND);
}

bool TreeFeature::isWaterAt(WorldGenRegion& world, const BlockPos& pos)
{
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (state == nullptr) {
        return false;
    }

    return state->is(VanillaBlocks::WATER);
}

i32 TreeFeature::_calculateAvailableHeight(
    WorldGenRegion& world, i32 maxHeight, const BlockPos& startPos, const TreeFeatureConfig& config) const
{
    // 对应 MC 1.21.11 TreeFeature.getMaxFreeTreeHeight：
    //   for (int i = 0; i <= trunkHeight + 1; i++) {
    //       int j = minimumSize.getSizeAtHeight(trunkHeight, i);
    //       for (k=-j..j) for (l=-j..j) if (!isFree || (!ignoreVines && isVine)) return i - 2;
    //   }
    //   return trunkHeight;
    //
    // 若 minimumSize 为空（不应发生，但做兜底以保持健壮性），使用退化的默认半径规则。
    BlockPos pos;

    for (i32 y = 0; y <= maxHeight + 1; ++y) {
        i32 checkRadius = 0;
        if (config.minimumSize) {
            checkRadius = config.minimumSize->getSizeAtHeight(maxHeight, y);
        } else {
            // 兜底：原 hardcoded 规则（底部 0、中段 1、顶部 2）
            if (y > 0 && y < maxHeight - 1) {
                checkRadius = 1;
            } else if (y >= maxHeight - 1) {
                checkRadius = 2;
            }
        }

        for (i32 dx = -checkRadius; dx <= checkRadius; ++dx) {
            for (i32 dz = -checkRadius; dz <= checkRadius; ++dz) {
                pos.x = startPos.x + dx;
                pos.y = startPos.y + y;
                pos.z = startPos.z + dz;

                if (!isReplaceableAt(world, pos)) {
                    // 找到不可替换的方块，返回当前可用高度
                    return y - 2;
                }
            }
        }
    }

    return maxHeight;
}

void TreeFeature::_setFoliageDistance(
    WorldGenRegion& world, const std::set<BlockPos>& trunkBlocks, const std::set<BlockPos>& foliageBlocks)
{
    // BFS 从树干方块开始，计算每个树叶到最近树干的曼哈顿距离

    if (trunkBlocks.empty() || foliageBlocks.empty()) {
        return;
    }

    // 使用 BFS 计算距离
    // 距离 1 = 直接相邻树干
    // 距离 7+ = 会自然腐烂
    std::map<BlockPos, i32> distances;
    std::queue<BlockPos> queue;

    // 初始化：所有树干方块距离为 0
    for (const auto& pos : trunkBlocks) {
        distances[pos] = 0;
        queue.push(pos);
    }

    // 6 个方向偏移
    static const BlockPos offsets[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

    // BFS 扩散，最大距离 7
    while (!queue.empty()) {
        BlockPos current = queue.front();
        queue.pop();

        i32 currentDist = distances[current];
        if (currentDist >= 7) {
            continue;
        }

        for (const auto& offset : offsets) {
            BlockPos neighbor(current.x + offset.x, current.y + offset.y, current.z + offset.z);

            // 只处理树叶方块
            if (foliageBlocks.find(neighbor) == foliageBlocks.end()) {
                continue;
            }

            i32 newDist = currentDist + 1;
            auto it = distances.find(neighbor);
            if (it == distances.end() || it->second > newDist) {
                distances[neighbor] = newDist;
                queue.push(neighbor);
            }
        }
    }

    // 将计算出的距离设置到树叶方块的 blockstate 中
    for (const auto& pos : foliageBlocks) {
        auto it = distances.find(pos);
        i32 dist = (it != distances.end()) ? it->second : 7;

        // 限制距离在 [1, 7] 范围内（DISTANCE_1_7 属性范围）
        dist = std::max(1, std::min(7, dist));

        const BlockState* currentState = world.getBlockState(pos);
        if (currentState != nullptr && currentState->hasProperty(BlockStateProperties::DISTANCE_1_7())) {
            const BlockState& newState = currentState->with(BlockStateProperties::DISTANCE_1_7(), dist);
            world.setBlockState(pos, &newState);
        }
    }
}

// ============================================================================
// ConfiguredTreeFeature 实现
// ============================================================================

ConfiguredTreeFeature::ConfiguredTreeFeature(std::unique_ptr<TreeFeatureConfig> featureConfig, const char* featureName)
    : m_config(std::move(featureConfig))
    , m_name(featureName)
{}

bool ConfiguredTreeFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    (void)generator;
    (void)chunk;

    if (!m_config) {
        return false;
    }

    // 数据驱动下 pos 已是 placement 链处理后的最终位置，直接放置树木
    if (pos.y < world::MIN_BUILD_HEIGHT + 1 || pos.y >= world::MAX_BUILD_HEIGHT - 1) {
        return false;
    }

    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// TreeFeatures 实现
// ============================================================================

TreeFeatureConfig TreeFeatures::oakConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::OAK_LOG);
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::OAK_LEAVES);
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(4, 2, 0);
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 3);
    config.minHeight = 4;
    return config;
}

TreeFeatureConfig TreeFeatures::birchConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::BIRCH_LOG);
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::BIRCH_LEAVES);
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(5, 2, 0);
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 2);
    config.minHeight = 5;
    return config;
}

TreeFeatureConfig TreeFeatures::spruceConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::SPRUCE_LOG);
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::SPRUCE_LEAVES);
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(5, 2, 1);
    // 使用云杉树叶放置器生成锥形树冠
    // SpruceFoliagePlacer(radius, offset, height)
    config.foliagePlacer = std::make_unique<SpruceFoliagePlacer>(FeatureSpread::spread(2, 1),
        FeatureSpread::fixed(0),
        2 // height
    );
    config.minHeight = 5;
    return config;
}

TreeFeatureConfig TreeFeatures::jungleConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::JUNGLE_LOG);
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::JUNGLE_LEAVES);
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(4, 8, 0);
    // 使用丛林树叶放置器
    // JungleFoliagePlacer(radius, offset, height)
    config.foliagePlacer = std::make_unique<JungleFoliagePlacer>(FeatureSpread::spread(2, 1),
        FeatureSpread::fixed(0),
        2 // height
    );
    config.minHeight = 4;
    return config;
}

TreeFeatureConfig TreeFeatures::acaciaConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::ACACIA_LOG);
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::ACACIA_LEAVES);
    // 金合欢使用分叉树干
    config.trunkPlacer = std::make_unique<ForkyTrunkPlacer>(5, 2, 1);
    // 金合欢使用伞形树叶
    // AcaciaFoliagePlacer(radius, offset)
    config.foliagePlacer = std::make_unique<AcaciaFoliagePlacer>(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0));
    config.minHeight = 4;
    return config;
}

TreeFeatureConfig TreeFeatures::darkOakConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::DARK_OAK_LOG);
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::DARK_OAK_LEAVES);
    // 深色橡树使用 2x2 树干
    config.trunkPlacer = std::make_unique<DarkOakTrunkPlacer>(6, 3, 1);
    // 深色橡树使用密集球形树叶
    // DarkOakFoliagePlacer(radius, offset, height)
    config.foliagePlacer = std::make_unique<DarkOakFoliagePlacer>(FeatureSpread::spread(2, 1),
        FeatureSpread::fixed(0),
        4 // height
    );
    config.minHeight = 6;
    return config;
}

TreeFeatureConfig TreeFeatures::giantSpruceConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::SPRUCE_LOG);
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::SPRUCE_LEAVES);
    // 巨型云杉使用 2x2 树干
    config.trunkPlacer = std::make_unique<GiantTrunkPlacer>(13, 5, 3);
    // 巨型云杉使用更大的锥形树叶
    // MegaPineFoliagePlacer(radius, offset, height)
    config.foliagePlacer = std::make_unique<MegaPineFoliagePlacer>(FeatureSpread::spread(3, 2),
        FeatureSpread::fixed(0),
        8 // height
    );
    config.minHeight = 13;
    return config;
}

TreeFeatureConfig TreeFeatures::giantJungleConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::JUNGLE_LOG);
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::JUNGLE_LEAVES);
    // 巨型丛林木使用 2x2 树干
    config.trunkPlacer = std::make_unique<MegaJungleTrunkPlacer>(10, 8, 5);
    // 巨型丛林木使用丛林树叶放置器
    // JungleFoliagePlacer(radius, offset, height)
    config.foliagePlacer = std::make_unique<JungleFoliagePlacer>(FeatureSpread::spread(3, 2),
        FeatureSpread::fixed(0),
        3 // height
    );
    config.minHeight = 10;
    return config;
}

TreeFeatureConfig TreeFeatures::fancyOakConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::OAK_LOG);
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::OAK_LEAVES);
    config.trunkPlacer = std::make_unique<FancyTrunkPlacer>(3, 11, 0);
    config.foliagePlacer =
        std::make_unique<FancyFoliagePlacer>(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 4);
    config.minHeight = 4;
    return config;
}

TreeFeatureConfig TreeFeatures::pineConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::SPRUCE_LOG);
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::SPRUCE_LEAVES);
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(6, 4, 0);
    config.foliagePlacer = std::make_unique<PineFoliagePlacer>(FeatureSpread::spread(1, 1), FeatureSpread::fixed(1), 4);
    config.minHeight = 6;
    return config;
}

TreeFeatureConfig TreeFeatures::jungleBushConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::JUNGLE_LOG);
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::OAK_LEAVES);
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(1, 0, 0);
    config.foliagePlacer = std::make_unique<BushFoliagePlacer>(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0));
    config.minHeight = 1;
    return config;
}

TreeFeatureConfig TreeFeatures::swampConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::OAK_LOG);
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::OAK_LEAVES);
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(5, 3, 0);
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(FeatureSpread::spread(3, 0), FeatureSpread::fixed(0), 3);
    config.maxWaterDepth = 1;
    config.minHeight = 5;
    return config;
}

TreeFeatureConfig TreeFeatures::megaPineConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::SPRUCE_LOG);
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::SPRUCE_LEAVES);
    config.trunkPlacer = std::make_unique<GiantTrunkPlacer>(13, 5, 3);
    config.foliagePlacer =
        std::make_unique<MegaPineFoliagePlacer>(FeatureSpread::spread(3, 2), FeatureSpread::fixed(0), 13);
    config.minHeight = 13;
    return config;
}

TreeFeatureConfig TreeFeatures::tallBirchConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::BIRCH_LOG);
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::BIRCH_LEAVES);
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(5, 2, 6);
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 2);
    config.minHeight = 5;
    return config;
}

TreeFeatureConfig TreeFeatures::cherryConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(block_registry::CherryBlocks::CHERRY_LOG);
    config.foliageBlock = VanillaBlocks::getState(block_registry::CherryBlocks::CHERRY_LEAVES);
    config.trunkPlacer = std::make_unique<CherryTrunkPlacer>(7,
        1,
        0, // baseHeight, heightRandA, heightRandB
        1,
        3, // branchCountMin, branchCountMax
        2,
        4, // branchHorizontalLengthMin, branchHorizontalLengthMax
        -4,
        -3, // branchStartOffsetFromTopMin, branchStartOffsetFromTopMax
        -1,
        0 // branchEndOffsetFromTopMin, branchEndOffsetFromTopMax
    );
    config.foliagePlacer = std::make_unique<CherryFoliagePlacer>(FeatureSpread::fixed(4), // radius
        FeatureSpread::fixed(0),                                                          // offset
        5,                                                                                // height
        0.25f,                                                                            // wideBottomLayerHoleChance
        0.5f,                                                                             // cornerHoleChance
        1.0f / 6.0f,                                                                      // hangingLeavesChance
        1.0f / 3.0f // hangingLeavesExtensionChance
    );
    config.ignoreVines = true;
    config.minHeight = 4;
    return config;
}

TreeFeatureConfig TreeFeatures::paleOakConfig()
{
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::getState(block_registry::PaleGardenBlocks::PALE_OAK_LOG);
    config.foliageBlock = VanillaBlocks::getState(block_registry::PaleGardenBlocks::PALE_OAK_LEAVES);
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(4, 2, 0);
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 3);
    config.minHeight = 4;
    return config;
}

TreeFeatureConfig TreeFeatures::azaleaConfig()
{
    TreeFeatureConfig config;
    // 杜鹃树使用橡木原木作为树干
    config.trunkBlock = VanillaBlocks::getState(VanillaBlocks::OAK_LOG);

    // 树叶使用加权提供者：杜鹃叶（权重 3）与开花杜鹃叶（权重 1）混合
    auto foliageProvider = std::make_unique<world::gen::feature::state::WeightedBlockStateProvider>();
    foliageProvider->add(VanillaBlocks::getState(block_registry::CaveBlocks::AZALEA_LEAVES), 3);
    foliageProvider->add(VanillaBlocks::getState(block_registry::CaveBlocks::FLOWERING_AZALEA_LEAVES), 1);
    config.foliageProvider = std::move(foliageProvider);

    // 弯曲树干放置器：baseHeight=4, heightRandA=2, heightRandB=0,
    // minHeightForLeaves=3, bendLength=UniformInt(1, 2)
    config.trunkPlacer =
        std::make_unique<BendingTrunkPlacer>(4, 2, 0, 3, world::gen::valueprovider::UniformInt::create(1, 2));

    // 随机散布树叶放置器：
    // radius=ConstantInt(3), offset=ConstantInt(0), foliageHeight=ConstantInt(2), leafPlacementAttempts=50
    config.foliagePlacer = std::make_unique<RandomSpreadFoliagePlacer>(
        FeatureSpread::of(world::gen::valueprovider::ConstantInt::create(3)),
        FeatureSpread::of(world::gen::valueprovider::ConstantInt::create(0)),
        world::gen::valueprovider::ConstantInt::create(2),
        50);

    // 两层特征尺寸：limit=1, lowerSize=0, upperSize=1
    config.minimumSize = std::make_unique<TwoLayersFeatureSize>(1, 0, 1);
    config.minHeight = 4;
    return config;
}

} // namespace mc
