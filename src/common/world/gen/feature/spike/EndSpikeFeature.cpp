#include "EndSpikeFeature.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <algorithm>

namespace mc {

namespace {

[[nodiscard]] bool intersectsWorldGenRegion(
    const EndSpike& spike,
    const WorldGenRegion& world,
    i32 centerX,
    i32 centerZ)
{
    const i32 minX = world.minChunkX() * 16;
    const i32 maxX = (world.maxChunkX() + 1) * 16 - 1;
    const i32 minZ = world.minChunkZ() * 16;
    const i32 maxZ = (world.maxChunkZ() + 1) * 16 - 1;

    if (centerX + spike.radius < minX || centerX - spike.radius > maxX) {
        return false;
    }

    if (centerZ + spike.radius < minZ || centerZ - spike.radius > maxZ) {
        return false;
    }

    return true;
}

} // namespace

// ============================================================================
// EndSpikeFeatureConfig 实现
// ============================================================================

std::vector<EndSpike> EndSpikeFeatureConfig::generateSpikes(u64 worldSeed) {
    std::vector<EndSpike> spikes;

    // 参考 MC 1.16.5: SpikeFeature.EndSpike
    // 10根柱子，均匀分布在半径为43的圆上
    // 每根柱子的角度偏移由种子决定

    math::Random rng(worldSeed);

    // 生成随机偏移角度
    f64 angleOffset = rng.nextDouble() * mc::math::PI_DOUBLE * 2.0;

    // 高度和半径数据（MC原版数据）
    static const i32 heights[] = {76, 79, 82, 85, 88, 91, 94, 97, 100, 103};
    static const i32 radii[] = {2, 2, 2, 2, 2, 3, 3, 3, 4, 4};
    static const bool guarded[] = {false, false, false, false, false, false, false, true, true, true};

    // 随机打乱顺序
    std::vector<i32> indices = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (i32 i = 9; i > 0; --i) {
        i32 j = rng.nextInt(i + 1);
        std::swap(indices[i], indices[j]);
    }

    // 生成10根柱子
    for (i32 i = 0; i < 10; ++i) {
        f64 angle = angleOffset + (i * 2.0 * mc::math::PI_DOUBLE / 10.0);

        // 半径43的圆上分布
        i32 x = static_cast<i32>(std::cos(angle) * 43.0);
        i32 z = static_cast<i32>(std::sin(angle) * 43.0);

        i32 idx = indices[i];
        spikes.emplace_back(x, z, radii[idx], heights[idx], guarded[idx]);
    }

    return spikes;
}

// ============================================================================
// EndSpikeFeature 实现
// ============================================================================

bool EndSpikeFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const EndSpikeFeatureConfig& config)
{
    // 获取黑曜石柱列表
    const std::vector<EndSpike>& spikes = config.spikes;

    // 如果配置为销毁模式，先销毁所有柱子
    if (config.destroying) {
        const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);
        for (const auto& spike : spikes) {
            if (!intersectsWorldGenRegion(spike, world, pos.x + spike.centerX, pos.z + spike.centerZ)) {
                continue;
            }

            // 销毁柱子区域的所有方块
            for (i32 y = 0; y < spike.height; ++y) {
                for (i32 x = -spike.radius; x <= spike.radius; ++x) {
                    for (i32 z = -spike.radius; z <= spike.radius; ++z) {
                        // 圆形截面
                        if (x * x + z * z <= spike.radius * spike.radius) {
                            world.setBlock(
                                pos.x + spike.centerX + x,
                                y,
                                pos.z + spike.centerZ + z,
                                air
                            );
                        }
                    }
                }
            }
        }
        return true;
    }

    // 生成每根柱子
    for (const auto& spike : spikes) {
        if (!intersectsWorldGenRegion(spike, world, spike.centerX, spike.centerZ)) {
            continue;
        }

        generateSpike(world, random, spike);
    }

    return true;
}

bool EndSpikeFeature::canPlaceAt(
    WorldGenRegion& world,
    const BlockPos& pos) const
{
    // 检查位置是否在末地石上
    const BlockState* state = world.getBlock(pos.x, pos.y - 1, pos.z);
    return state && &state->getBlock() == VanillaBlocks::END_STONE;
}

void EndSpikeFeature::generateSpike(
    WorldGenRegion& world,
    math::Random& random,
    const EndSpike& spike)
{
    const BlockState* obsidian = VanillaBlocks::getState(VanillaBlocks::OBSIDIAN);

    // 柱子中心坐标
    i32 baseX = spike.centerX;
    i32 baseZ = spike.centerZ;
    i32 radius = spike.radius;
    i32 height = spike.height;

    // 生成柱子的基座（从Y=0开始，到目标高度）
    for (i32 y = 0; y < height; ++y) {
        // 圆形截面
        for (i32 x = -radius; x <= radius; ++x) {
            for (i32 z = -radius; z <= radius; ++z) {
                // 检查是否在圆形范围内
                i32 distSq = x * x + z * z;
                if (distSq <= radius * radius) {
                    world.setBlock(baseX + x, y, baseZ + z, obsidian);
                }
            }
        }
    }

    // 如果需要笼子，生成铁栏杆
    if (spike.guarded) {
        BlockPos topPos(baseX, height - 1, baseZ);
        generateCage(world, topPos, radius);
    }
}

void EndSpikeFeature::generateCage(
    WorldGenRegion& world,
    const BlockPos& topPos,
    i32 radius)
{
    // 使用基岩作为笼子材料（因为铁栏杆尚未实现）
    const BlockState* cageBlock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);

    if (!cageBlock) {
        return;
    }

    i32 cageRadius = radius + 2;
    i32 cageHeight = 3;

    // 生成笼子
    for (i32 y = 0; y < cageHeight; ++y) {
        for (i32 x = -cageRadius; x <= cageRadius; ++x) {
            for (i32 z = -cageRadius; z <= cageRadius; ++z) {
                // 只生成边缘的栏杆
                i32 distSq = x * x + z * z;
                bool isEdge = (distSq >= (cageRadius - 1) * (cageRadius - 1) && distSq <= cageRadius * cageRadius);

                // 边缘栏杆
                if (isEdge) {
                    world.setBlock(topPos.x + x, topPos.y + y, topPos.z + z, cageBlock);
                }
            }
        }
    }

    // 笼子顶部
    for (i32 x = -cageRadius; x <= cageRadius; ++x) {
        for (i32 z = -cageRadius; z <= cageRadius; ++z) {
            i32 distSq = x * x + z * z;
            if (distSq <= cageRadius * cageRadius) {
                world.setBlock(topPos.x + x, topPos.y + cageHeight, topPos.z + z, cageBlock);
            }
        }
    }
}

// ============================================================================
// ConfiguredEndSpikeFeature 实现
// ============================================================================

ConfiguredEndSpikeFeature::ConfiguredEndSpikeFeature(
    std::unique_ptr<EndSpikeFeatureConfig> config,
    const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{
}

bool ConfiguredEndSpikeFeature::place(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos)
{
    MC_UNUSED(chunk);

    EndSpikeFeatureConfig runtimeConfig = *m_config;
    if (!runtimeConfig.destroying) {
        runtimeConfig.spikes = EndSpikeFeatureConfig::generateSpikes(generator.seed());
    }

    return m_feature.place(region, random, pos, runtimeConfig);
}

// ============================================================================
// EndSpikeFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredEndSpikeFeature>> EndSpikeFeatures::s_features;

void EndSpikeFeatures::initialize() {
    if (!s_features.empty()) return;

    // 创建标准黑曜石柱配置（使用默认种子）
    // 实际使用时应该传入世界种子
    s_features.push_back(createStandard(0));
}

const std::vector<std::unique_ptr<ConfiguredEndSpikeFeature>>& EndSpikeFeatures::getAllFeatures() {
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredEndSpikeFeature>> EndSpikeFeatures::getAllFeaturesAndClear() {
    auto extracted = std::move(s_features);
    s_features.clear();
    return extracted;
}

std::unique_ptr<ConfiguredEndSpikeFeature> EndSpikeFeatures::createStandard(u64 worldSeed) {
    auto config = std::make_unique<EndSpikeFeatureConfig>(
        EndSpikeFeatureConfig::generateSpikes(worldSeed),
        false
    );
    return std::make_unique<ConfiguredEndSpikeFeature>(std::move(config), "end_spike");
}

} // namespace mc
