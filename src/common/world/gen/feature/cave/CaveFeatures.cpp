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

#include "CaveFeatures.hpp"
#include "common/core/Constants.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/placement/Placement.hpp"

namespace mc::world::gen::feature::cave {

// ============================================================================
// 辅助函数
// ============================================================================

namespace {

/**
 * @brief 检查方块是否匹配标签
 */
bool matchesTag(const BlockState& state, const std::string& tagName)
{
    auto* tag = mc::BlockTags::getTag(mc::ResourceLocation(tagName));
    return tag != nullptr && tag->contains(state);
}

} // anonymous namespace

// ============================================================================
// SimpleBlockFeature
// ============================================================================

bool SimpleBlockFeature::place(
    WorldGenRegion& region, math::Random& random, const BlockPos& pos, const SimpleBlockConfig& config)
{
    MC_UNUSED(random);

    if (config.toPlace == nullptr) {
        return false;
    }

    const BlockState* currentState = region.getBlockState(pos);
    if (currentState == nullptr) {
        return false;
    }

    // 检查方块是否能在该位置存活
    const Block& block = config.toPlace->getBlock();
    // 简单检查：如果当前位置是空气或可替换，则放置
    if (!currentState->isAir() && !currentState->getMaterial().isReplaceable()) {
        return false;
    }

    region.setBlockState(pos, config.toPlace, 3);
    return true;
}

// ============================================================================
// RandomBooleanSelectorFeature
// ============================================================================

bool RandomBooleanSelectorFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos,
    const RandomBooleanFeatureConfig& config)
{
    FeatureRegistry& registry = FeatureRegistry::instance();

    u32 featureId = random.nextBoolean() ? config.featureTrueId : config.featureFalseId;
    const auto& features = registry.getFeatures(DecorationStage::VegetalDecoration);

    if (featureId >= features.size() || features[featureId] == nullptr) {
        return false;
    }

    return features[featureId]->place(region, chunk, generator, random, pos);
}

// ============================================================================
// SimpleRandomSelectorFeature
// ============================================================================

bool SimpleRandomSelectorFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos,
    const SimpleRandomFeatureConfig& config)
{
    if (config.featureIds.empty()) {
        return false;
    }

    FeatureRegistry& registry = FeatureRegistry::instance();
    const auto& features = registry.getFeatures(DecorationStage::VegetalDecoration);

    u32 index = static_cast<u32>(random.nextInt(static_cast<i32>(config.featureIds.size())));
    u32 featureId = config.featureIds[index];

    if (featureId >= features.size() || features[featureId] == nullptr) {
        return false;
    }

    return features[featureId]->place(region, chunk, generator, random, pos);
}

// ============================================================================
// BlockColumnFeature
// ============================================================================

bool BlockColumnFeature::place(
    WorldGenRegion& region, math::Random& random, const BlockPos& pos, const BlockColumnConfig& config)
{
    if (config.layers.empty()) {
        return false;
    }

    // 计算每层高度
    i32 totalHeight = 0;
    std::vector<i32> layerHeights;
    layerHeights.reserve(config.layers.size());

    for (const auto& layer : config.layers) {
        i32 height = layer.height ? layer.height->sample(random) : 1;
        layerHeights.push_back(height);
        totalHeight += height;
    }

    if (totalHeight == 0) {
        return false;
    }

    // 检查放置位置
    Direction dir = config.direction;
    BlockPos current = pos;
    i32 failPosition = 0;

    // 先验证路径是否可放置
    if (config.allowedPlacement) {
        BlockPos checkPos = pos.offset(dir);
        for (i32 i = 0; i < totalHeight; ++i) {
            if (!config.allowedPlacement->test(region, checkPos)) {
                // 需要截断
                failPosition = i;
                break;
            }
            checkPos = checkPos.offset(dir);
        }
    }

    // 截断超出可放置范围的层
    if (failPosition > 0) {
        i32 toRemove = totalHeight - failPosition;
        if (config.prioritizeTip) {
            // 从底部删除
            for (i32 i = 0; i < static_cast<i32>(layerHeights.size()) && toRemove > 0; ++i) {
                i32 removeFromLayer = std::min(layerHeights[i], toRemove);
                layerHeights[i] -= removeFromLayer;
                toRemove -= removeFromLayer;
            }
        } else {
            // 从顶部删除
            for (i32 i = static_cast<i32>(layerHeights.size()) - 1; i >= 0 && toRemove > 0; --i) {
                i32 removeFromLayer = std::min(layerHeights[i], toRemove);
                layerHeights[i] -= removeFromLayer;
                toRemove -= removeFromLayer;
            }
        }
    }

    // 放置方块
    current = pos;
    bool placedAny = false;

    for (size_t layerIdx = 0; layerIdx < config.layers.size(); ++layerIdx) {
        const BlockState* state = config.layers[layerIdx].state;
        i32 height = layerHeights[layerIdx];

        for (i32 h = 0; h < height; ++h) {
            if (state != nullptr) {
                const BlockState* existing = region.getBlockState(current);
                if (existing == nullptr || existing->isAir() || existing->getMaterial().isReplaceable()) {
                    region.setBlockState(current, state, 3);
                    placedAny = true;
                }
            }
            current = current.offset(dir);
        }
    }

    return placedAny;
}

// ============================================================================
// VegetationPatchFeature
// ============================================================================

std::vector<BlockPos> VegetationPatchFeature::placeGroundPatch(
    WorldGenRegion& region, math::Random& random, const BlockPos& pos, const VegetationPatchConfig& config)
{
    std::vector<BlockPos> groundPositions;

    // 采样XZ半径
    i32 radiusX = config.xzRadius ? config.xzRadius->sample(random) + 1 : 2;
    i32 radiusZ = config.xzRadius ? config.xzRadius->sample(random) + 1 : 2;

    Direction scanDir = getScanDirection(config.surface);
    Direction placeDir = scanDir;                          // 扫描方向
    Direction oppositeDir = Directions::opposite(scanDir); // 放置方向（反方向）

    for (i32 x = -radiusX; x <= radiusX; ++x) {
        for (i32 z = -radiusZ; z <= radiusZ; ++z) {
            // 检查是否在边缘
            bool isEdgeX = (x == -radiusX || x == radiusX);
            bool isEdgeZ = (z == -radiusZ || z == radiusZ);
            bool isCorner = isEdgeX && isEdgeZ;
            bool isEdge = isEdgeX || isEdgeZ;

            // 跳过角落
            if (isCorner) {
                continue;
            }

            // 边缘列有概率跳过
            if (isEdge && random.nextFloat() >= config.extraEdgeColumnChance) {
                continue;
            }

            // 沿扫描方向寻找实心面
            BlockPos scanPos(pos.x + x, pos.y, pos.z + z);
            bool foundSurface = false;

            // 先沿扫描方向找实心
            for (i32 i = 0; i < config.verticalRange; ++i) {
                const BlockState* state = region.getBlockState(scanPos);
                if (state != nullptr && state->isSolid()) {
                    foundSurface = true;
                    break;
                }
                scanPos = scanPos.offset(placeDir);
            }

            if (!foundSurface) {
                continue;
            }

            // 然后沿反方向找空位
            scanPos = scanPos.offset(oppositeDir);
            for (i32 i = 0; i < config.verticalRange; ++i) {
                const BlockState* state = region.getBlockState(scanPos);
                if (state != nullptr && (state->isAir() || state->getMaterial().isReplaceable())) {
                    // 检查扫描方向相邻的方块是否有坚固面
                    const BlockState* supportState = region.getBlockState(scanPos.offset(scanDir));
                    if (supportState != nullptr && supportState->isSolid()) {
                        foundSurface = true;
                        break;
                    }
                }
                scanPos = scanPos.offset(oppositeDir);
            }

            if (!foundSurface) {
                continue;
            }

            // 放置地面方块
            i32 depth = config.depth ? config.depth->sample(random) : 1;
            if (random.nextFloat() < config.extraBottomBlockChance) {
                depth += 1;
            }

            bool placed = placeGround(region, random, scanPos, config, oppositeDir, depth);
            if (placed) {
                groundPositions.push_back(scanPos);
            }
        }
    }

    return groundPositions;
}

bool VegetationPatchFeature::placeGround(WorldGenRegion& region,
    math::Random& random,
    const BlockPos& pos,
    const VegetationPatchConfig& config,
    Direction surfaceDir,
    i32 depth)
{
    MC_UNUSED(random);

    BlockPos current = pos;
    bool placedAny = false;

    for (i32 i = 0; i < depth; ++i) {
        const BlockState* existing = region.getBlockState(current);
        if (existing == nullptr) {
            break;
        }

        // 已经是目标方块则跳过
        if (&existing->getBlock() == &config.groundState->getBlock()) {
            current = current.offset(surfaceDir);
            continue;
        }

        // 检查是否可替换
        if (!matchesTag(*existing, config.replaceableTag)) {
            return placedAny; // i != 0 返回true，i == 0 返回false
        }

        region.setBlockState(current, config.groundState, 3);
        placedAny = true;
        current = current.offset(surfaceDir);
    }

    return placedAny;
}

void VegetationPatchFeature::distributeVegetation(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const std::vector<BlockPos>& groundPositions,
    const VegetationPatchConfig& config)
{
    if (config.vegetationFeatureId == 0 || groundPositions.empty()) {
        return;
    }

    FeatureRegistry& registry = FeatureRegistry::instance();
    const auto& features = registry.getFeatures(DecorationStage::VegetalDecoration);

    if (config.vegetationFeatureId >= features.size() || features[config.vegetationFeatureId] == nullptr) {
        return;
    }

    ConfiguredFeatureBase* feature = features[config.vegetationFeatureId];

    // 植被放置的Y偏移
    i32 yOffset = getVegetationYOffset(config.surface);

    for (const BlockPos& groundPos : groundPositions) {
        if (random.nextFloat() < config.vegetationChance) {
            BlockPos vegetationPos = groundPos.offset(Direction::Up, yOffset > 0 ? yOffset : 0)
                                         .offset(Direction::Down, yOffset < 0 ? -yOffset : 0);
            feature->place(region, chunk, generator, random, vegetationPos);
        }
    }
}

bool VegetationPatchFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos,
    const VegetationPatchConfig& config)
{
    if (config.groundState == nullptr) {
        return false;
    }

    // 放置地面贴片
    std::vector<BlockPos> groundPositions = placeGroundPatch(region, random, pos, config);

    if (groundPositions.empty()) {
        return false;
    }

    // 分布植被
    distributeVegetation(region, chunk, generator, random, groundPositions, config);

    return true;
}

// ============================================================================
// WaterloggedVegetationPatchFeature
// ============================================================================

bool WaterloggedVegetationPatchFeature::isExposed(WorldGenRegion& region, const BlockPos& pos)
{
    // 检查四个水平方向和下方是否有非实心面
    static const Direction checkDirs[] = {
        Direction::North, Direction::East, Direction::South, Direction::West, Direction::Down};

    for (Direction dir : checkDirs) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = region.getBlockState(neighborPos);
        if (neighborState == nullptr || !neighborState->isSolid()) {
            return true;
        }
    }
    return false;
}

bool WaterloggedVegetationPatchFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos,
    const VegetationPatchConfig& config)
{
    if (config.groundState == nullptr) {
        return false;
    }

    // 放置地面贴片
    std::vector<BlockPos> groundPositions = VegetationPatchFeature::placeGroundPatch(region, random, pos, config);

    if (groundPositions.empty()) {
        return false;
    }

    // 将非暴露位置替换为水
    for (const BlockPos& groundPos : groundPositions) {
        if (!isExposed(region, groundPos)) {
            // 内部位置填水
            const BlockState* water = VanillaBlocks::getState(VanillaBlocks::WATER);
            if (water != nullptr) {
                region.setBlockState(groundPos, water, 3);
            }
        }
    }

    // 分布植被（含水版）
    if (config.vegetationFeatureId != 0) {
        FeatureRegistry& registry = FeatureRegistry::instance();
        const auto& features = registry.getFeatures(DecorationStage::VegetalDecoration);

        if (config.vegetationFeatureId < features.size() && features[config.vegetationFeatureId] != nullptr) {
            ConfiguredFeatureBase* feature = features[config.vegetationFeatureId];
            i32 yOffset = getVegetationYOffset(config.surface);

            for (const BlockPos& groundPos : groundPositions) {
                if (random.nextFloat() < config.vegetationChance) {
                    BlockPos vegetationPos = groundPos.offset(Direction::Up, yOffset > 0 ? yOffset : 0)
                                                 .offset(Direction::Down, yOffset < 0 ? -yOffset : 0);

                    bool placed = feature->place(region, chunk, generator, random, vegetationPos);

                    // 如果植被放置成功且位于水中，设置WATERLOGGED属性
                    if (placed) {
                        const BlockState* vegState = region.getBlockState(vegetationPos);
                        if (vegState != nullptr && vegState->hasProperty(BlockStateProperties::WATERLOGGED())) {
                            const BlockState& waterlogged = vegState->with(BlockStateProperties::WATERLOGGED(), true);
                            region.setBlockState(vegetationPos, &waterlogged, 3);
                        }
                    }
                }
            }
        }
    }

    return true;
}

// ============================================================================
// RootSystemFeature
// ============================================================================

bool RootSystemFeature::spaceForTree(WorldGenRegion& region, const BlockPos& pos, i32 requiredSpace, i32 allowedWater)
{
    i32 waterCount = 0;
    for (i32 i = 1; i <= requiredSpace; ++i) {
        BlockPos checkPos = pos.offset(Direction::Up, i);
        const BlockState* state = region.getBlockState(checkPos);
        if (state == nullptr) {
            return false;
        }
        if (state->isAir()) {
            continue;
        }
        // 检查是否为水
        if (state->getMaterial().isLiquid()) {
            waterCount++;
            if (waterCount > allowedWater) {
                return false;
            }
            continue;
        }
        // 非空气非水，空间不足
        return false;
    }
    return true;
}

void RootSystemFeature::placeRootedDirtColumn(
    WorldGenRegion& region, math::Random& random, const BlockPos& origin, i32 targetY, const RootSystemConfig& config)
{
    MC_UNUSED(random);

    // 从origin向上填充到targetY
    for (i32 y = origin.y; y <= targetY; ++y) {
        BlockPos dirtPos(origin.x, y, origin.z);
        const BlockState* existing = region.getBlockState(dirtPos);
        if (existing != nullptr && matchesTag(*existing, config.rootReplaceableTag)) {
            region.setBlockState(dirtPos, config.rootState, 3);
        }
    }

    // 在rootRadius范围内随机放置缠根泥土
    for (i32 attempt = 0; attempt < config.rootPlacementAttempts; ++attempt) {
        i32 dx = random.nextInt(config.rootRadius * 2 + 1) - config.rootRadius;
        i32 dz = random.nextInt(config.rootRadius * 2 + 1) - config.rootRadius;
        BlockPos rootPos(origin.x + dx, random.nextInt(targetY - origin.y + 1) + origin.y, origin.z + dz);

        const BlockState* existing = region.getBlockState(rootPos);
        if (existing != nullptr && matchesTag(*existing, config.rootReplaceableTag)) {
            region.setBlockState(rootPos, config.rootState, 3);
        }
    }
}

void RootSystemFeature::placeHangingRoots(
    WorldGenRegion& region, math::Random& random, const BlockPos& rootCenter, const RootSystemConfig& config)
{
    if (config.hangingRootState == nullptr) {
        return;
    }

    for (i32 attempt = 0; attempt < config.hangingRootPlacementAttempts; ++attempt) {
        i32 dx = random.nextInt(config.hangingRootRadius * 2 + 1) - config.hangingRootRadius;
        i32 dy = -random.nextInt(config.hangingRootsVerticalSpan + 1);
        i32 dz = random.nextInt(config.hangingRootRadius * 2 + 1) - config.hangingRootRadius;

        BlockPos rootPos(rootCenter.x + dx, rootCenter.y + dy, rootCenter.z + dz);
        const BlockState* existing = region.getBlockState(rootPos);

        // 只在空气位置放置垂根
        if (existing == nullptr || !existing->isAir()) {
            continue;
        }

        // 检查上方是否有坚固面
        BlockPos abovePos = rootPos.offset(Direction::Up);
        const BlockState* aboveState = region.getBlockState(abovePos);
        if (aboveState != nullptr && aboveState->isSolid()) {
            region.setBlockState(rootPos, config.hangingRootState, 3);
        }
    }
}

bool RootSystemFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos,
    const RootSystemConfig& config)
{
    // 检查起始位置是否为空气
    const BlockState* startState = region.getBlockState(pos);
    if (startState != nullptr && !startState->isAir()) {
        return false;
    }

    // 向上搜索有效树木位置
    for (i32 i = 0; i < config.rootColumnMaxHeight; ++i) {
        BlockPos treePos = pos.offset(Direction::Up, i + 1);

        // 检查是否为空气
        const BlockState* treeState = region.getBlockState(treePos);
        if (treeState == nullptr || !treeState->isAir()) {
            continue;
        }

        // 检查树木是否有足够空间
        if (!spaceForTree(region, treePos, config.requiredVerticalSpaceForTree, config.allowedVerticalWaterForTree)) {
            continue;
        }

        // 检查下方是否有实心支撑
        BlockPos belowTreePos = treePos.offset(Direction::Down);
        const BlockState* belowState = region.getBlockState(belowTreePos);
        if (belowState == nullptr || belowState->isAir()) {
            continue;
        }

        // 检查下方不是熔岩
        if (belowState->getMaterial().isLiquid()) {
            continue;
        }

        // 尝试放置树木
        FeatureRegistry& registry = FeatureRegistry::instance();
        const auto& features = registry.getFeatures(DecorationStage::VegetalDecoration);

        if (config.treeFeatureId < features.size() && features[config.treeFeatureId] != nullptr) {
            bool treePlaced = features[config.treeFeatureId]->place(region, chunk, generator, random, treePos);
            if (treePlaced) {
                // 放置缠根泥土柱
                placeRootedDirtColumn(region, random, pos, treePos.y, config);

                // 放置垂根
                placeHangingRoots(region, random, pos, config);

                return true;
            }
        }
    }

    return false;
}

// ============================================================================
// ConfiguredSimpleBlockFeature
// ============================================================================

ConfiguredSimpleBlockFeature::ConfiguredSimpleBlockFeature(
    std::unique_ptr<SimpleBlockConfig> config, std::unique_ptr<ConfiguredPlacement> placement, const char* featureName)
    : m_config(std::move(config))
    , m_placement(std::move(placement))
    , m_name(featureName)
{}

bool ConfiguredSimpleBlockFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    // 获取放置位置
    std::vector<BlockPos> positions;
    if (m_placement) {
        positions = m_placement->getPositions(region, random, pos);
    } else {
        positions.push_back(pos);
    }

    bool placedAny = false;
    for (const BlockPos& placePos : positions) {
        if (SimpleBlockFeature::place(region, random, placePos, *m_config)) {
            placedAny = true;
        }
    }
    return placedAny;
}

// ============================================================================
// ConfiguredVegetationPatchFeature
// ============================================================================

ConfiguredVegetationPatchFeature::ConfiguredVegetationPatchFeature(std::unique_ptr<VegetationPatchConfig> config,
    std::unique_ptr<ConfiguredPlacement> placement,
    const char* featureName)
    : m_config(std::move(config))
    , m_placement(std::move(placement))
    , m_name(featureName)
{}

bool ConfiguredVegetationPatchFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    std::vector<BlockPos> positions;
    if (m_placement) {
        positions = m_placement->getPositions(region, random, pos);
    } else {
        positions.push_back(pos);
    }

    bool placedAny = false;
    for (const BlockPos& placePos : positions) {
        if (VegetationPatchFeature::place(region, chunk, generator, random, placePos, *m_config)) {
            placedAny = true;
        }
    }
    return placedAny;
}

// ============================================================================
// ConfiguredWaterloggedPatchFeature
// ============================================================================

ConfiguredWaterloggedPatchFeature::ConfiguredWaterloggedPatchFeature(std::unique_ptr<VegetationPatchConfig> config,
    std::unique_ptr<ConfiguredPlacement> placement,
    const char* featureName)
    : m_config(std::move(config))
    , m_placement(std::move(placement))
    , m_name(featureName)
{}

bool ConfiguredWaterloggedPatchFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    std::vector<BlockPos> positions;
    if (m_placement) {
        positions = m_placement->getPositions(region, random, pos);
    } else {
        positions.push_back(pos);
    }

    bool placedAny = false;
    for (const BlockPos& placePos : positions) {
        if (WaterloggedVegetationPatchFeature::place(region, chunk, generator, random, placePos, *m_config)) {
            placedAny = true;
        }
    }
    return placedAny;
}

// ============================================================================
// ConfiguredBlockColumnFeature
// ============================================================================

ConfiguredBlockColumnFeature::ConfiguredBlockColumnFeature(
    std::unique_ptr<BlockColumnConfig> config, std::unique_ptr<ConfiguredPlacement> placement, const char* featureName)
    : m_config(std::move(config))
    , m_placement(std::move(placement))
    , m_name(featureName)
{}

bool ConfiguredBlockColumnFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    MC_UNUSED(chunk);
    MC_UNUSED(generator);

    std::vector<BlockPos> positions;
    if (m_placement) {
        positions = m_placement->getPositions(region, random, pos);
    } else {
        positions.push_back(pos);
    }

    bool placedAny = false;
    for (const BlockPos& placePos : positions) {
        if (BlockColumnFeature::place(region, random, placePos, *m_config)) {
            placedAny = true;
        }
    }
    return placedAny;
}

// ============================================================================
// ConfiguredRootSystemFeature
// ============================================================================

ConfiguredRootSystemFeature::ConfiguredRootSystemFeature(
    std::unique_ptr<RootSystemConfig> config, std::unique_ptr<ConfiguredPlacement> placement, const char* featureName)
    : m_config(std::move(config))
    , m_placement(std::move(placement))
    , m_name(featureName)
{}

bool ConfiguredRootSystemFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    std::vector<BlockPos> positions;
    if (m_placement) {
        positions = m_placement->getPositions(region, random, pos);
    } else {
        positions.push_back(pos);
    }

    bool placedAny = false;
    for (const BlockPos& placePos : positions) {
        if (RootSystemFeature::place(region, chunk, generator, random, placePos, *m_config)) {
            placedAny = true;
        }
    }
    return placedAny;
}

// ============================================================================
// 静态工厂方法
// ============================================================================

VegetationPatchConfig VegetationPatchConfig::floorPatch(const std::string& replaceableTag,
    const BlockState* groundState,
    u32 vegetationFeatureId,
    std::unique_ptr<valueprovider::IntProvider> depth,
    f32 extraBottomBlockChance,
    i32 verticalRange,
    f32 vegetationChance,
    std::unique_ptr<valueprovider::IntProvider> xzRadius,
    f32 extraEdgeColumnChance)
{
    VegetationPatchConfig config;
    config.replaceableTag = replaceableTag;
    config.groundState = groundState;
    config.vegetationFeatureId = vegetationFeatureId;
    config.surface = CaveSurface::Floor;
    config.depth = std::move(depth);
    config.extraBottomBlockChance = extraBottomBlockChance;
    config.verticalRange = verticalRange;
    config.vegetationChance = vegetationChance;
    config.xzRadius = std::move(xzRadius);
    config.extraEdgeColumnChance = extraEdgeColumnChance;
    return config;
}

VegetationPatchConfig VegetationPatchConfig::ceilingPatch(const std::string& replaceableTag,
    const BlockState* groundState,
    u32 vegetationFeatureId,
    std::unique_ptr<valueprovider::IntProvider> depth,
    f32 extraBottomBlockChance,
    i32 verticalRange,
    f32 vegetationChance,
    std::unique_ptr<valueprovider::IntProvider> xzRadius,
    f32 extraEdgeColumnChance)
{
    VegetationPatchConfig config;
    config.replaceableTag = replaceableTag;
    config.groundState = groundState;
    config.vegetationFeatureId = vegetationFeatureId;
    config.surface = CaveSurface::Ceiling;
    config.depth = std::move(depth);
    config.extraBottomBlockChance = extraBottomBlockChance;
    config.verticalRange = verticalRange;
    config.vegetationChance = vegetationChance;
    config.xzRadius = std::move(xzRadius);
    config.extraEdgeColumnChance = extraEdgeColumnChance;
    return config;
}

// ============================================================================
// ConfiguredRandomBooleanSelectorFeature
// ============================================================================

ConfiguredRandomBooleanSelectorFeature::ConfiguredRandomBooleanSelectorFeature(
    std::unique_ptr<RandomBooleanFeatureConfig> config,
    std::unique_ptr<ConfiguredPlacement> placement,
    const char* featureName)
    : m_config(std::move(config))
    , m_placement(std::move(placement))
    , m_name(featureName)
{}

bool ConfiguredRandomBooleanSelectorFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    std::vector<BlockPos> positions;
    if (m_placement) {
        positions = m_placement->getPositions(region, random, pos);
    } else {
        positions.push_back(pos);
    }

    bool placedAny = false;
    for (const BlockPos& placePos : positions) {
        if (RandomBooleanSelectorFeature::place(region, chunk, generator, random, placePos, *m_config)) {
            placedAny = true;
        }
    }
    return placedAny;
}

// ============================================================================
// ConfiguredSimpleRandomSelectorFeature
// ============================================================================

ConfiguredSimpleRandomSelectorFeature::ConfiguredSimpleRandomSelectorFeature(
    std::unique_ptr<SimpleRandomFeatureConfig> config,
    std::unique_ptr<ConfiguredPlacement> placement,
    const char* featureName)
    : m_config(std::move(config))
    , m_placement(std::move(placement))
    , m_name(featureName)
{}

bool ConfiguredSimpleRandomSelectorFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    std::vector<BlockPos> positions;
    if (m_placement) {
        positions = m_placement->getPositions(region, random, pos);
    } else {
        positions.push_back(pos);
    }

    bool placedAny = false;
    for (const BlockPos& placePos : positions) {
        if (SimpleRandomSelectorFeature::place(region, chunk, generator, random, placePos, *m_config)) {
            placedAny = true;
        }
    }
    return placedAny;
}

} // namespace mc::world::gen::feature::cave
