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

#include "VegetationPatchFeature.hpp"
#include "CaveSurface.hpp"
#include "common/core/Constants.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/ConfiguredFeatureRegistry.hpp"

namespace mc::world::gen::feature::cave {

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
                if (state != nullptr && state->canBeReplaced()) {
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
    if (!config.vegetationFeatureId.isValid() || groundPositions.empty()) {
        return;
    }

    const ConfiguredFeatureBase* feature = ConfiguredFeatureRegistry::instance().get(config.vegetationFeatureId);

    if (feature == nullptr) {
        return;
    }

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
    if (config.vegetationFeatureId.isValid()) {
        const ConfiguredFeatureBase* feature = ConfiguredFeatureRegistry::instance().get(config.vegetationFeatureId);

        if (feature != nullptr) {
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
// ConfiguredVegetationPatchFeature
// ============================================================================

ConfiguredVegetationPatchFeature::ConfiguredVegetationPatchFeature(
    std::unique_ptr<VegetationPatchConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredVegetationPatchFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    return VegetationPatchFeature::place(region, chunk, generator, random, pos, *m_config);
}

// ============================================================================
// ConfiguredWaterloggedPatchFeature
// ============================================================================

ConfiguredWaterloggedPatchFeature::ConfiguredWaterloggedPatchFeature(
    std::unique_ptr<VegetationPatchConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredWaterloggedPatchFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    return WaterloggedVegetationPatchFeature::place(region, chunk, generator, random, pos, *m_config);
}

// ============================================================================
// 静态工厂方法
// ============================================================================

VegetationPatchConfig VegetationPatchConfig::floorPatch(const std::string& replaceableTag,
    const BlockState* groundState,
    ResourceLocation vegetationFeatureId,
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
    config.vegetationFeatureId = std::move(vegetationFeatureId);
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
    ResourceLocation vegetationFeatureId,
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
    config.vegetationFeatureId = std::move(vegetationFeatureId);
    config.surface = CaveSurface::Ceiling;
    config.depth = std::move(depth);
    config.extraBottomBlockChance = extraBottomBlockChance;
    config.verticalRange = verticalRange;
    config.vegetationChance = vegetationChance;
    config.xzRadius = std::move(xzRadius);
    config.extraEdgeColumnChance = extraEdgeColumnChance;
    return config;
}

} // namespace mc::world::gen::feature::cave
