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

#include "EndGatewayFeature.hpp"
#include "../../../../util/math/MathConstants.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../WorldConstants.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// EndGatewayFeature 实现
// ============================================================================

bool EndGatewayFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const EndGatewayFeatureConfig& config)
{
    MC_UNUSED(config);

    // 检查是否可以放置
    if (!_canPlaceAt(world, pos)) {
        return false;
    }

    // 生成折跃门结构
    _generateGateway(world, random, pos);

    return true;
}

BlockPos EndGatewayFeature::calculateTeleportTarget(const BlockPos& currentPos, u64 seed)
{
    MC_UNUSED(currentPos);

    // 传送到1024格外的外岛
    math::Random rng(seed);

    // 随机角度
    f64 angle = rng.nextDouble() * 2.0 * mc::math::PI_DOUBLE;

    // 距离范围：1024-1280格
    f64 distance = 1024.0 + rng.nextDouble() * 256.0;

    // 计算目标位置
    i32 targetX = static_cast<i32>(std::cos(angle) * distance);
    i32 targetZ = static_cast<i32>(std::sin(angle) * distance);

    // Y坐标：在末地岛屿上找一个合适的高度
    i32 targetY = 75; // 外岛通常在这个高度

    return BlockPos(targetX, targetY, targetZ);
}

bool EndGatewayFeature::_canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    // 检查折跃门下方是否有支撑
    // 结构范围从 pos.y - 2 到 pos.y + 2，底部的基岩需要支撑
    // 检查中心列下方和十字臂下方的支撑
    for (i32 x = -1; x <= 1; ++x) {
        for (i32 z = -1; z <= 1; ++z) {
            // 只检查十字形位置（中心 + 四个臂），对角不需要支撑
            if (x != 0 && z != 0) {
                continue;
            }

            const BlockState* state = world.getBlockState(pos.x + x, pos.y - 3, pos.z + z);
            if (!state || state->isAir()) {
                return false;
            }
        }
    }

    return true;
}

void EndGatewayFeature::_generateGateway(WorldGenRegion& world, math::Random& random, const BlockPos& pos)
{
    MC_UNUSED(random);

    // 折跃门结构：3x5x3 的基岩框架，中心为折跃门方块
    // 与 MC Java 的 EndGatewayFeature.place() 一致
    // 结构以 pos 为中心，范围从 pos + (-1, -2, -1) 到 pos + (1, 2, 1)
    //
    // 顶/底盖层（dy = ±2）：仅中心列为基岩
    //   . . .
    //   . B .
    //   . . .
    //
    // 十字臂层（dy = ±1）：十字形基岩框架
    //   . B .
    //   B B B
    //   . B .
    //
    // 中心层（dy = 0）：中心为折跃门方块，其余为空气
    //   . . .
    //   . G .
    //   . . .

    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    const BlockState* endGateway = VanillaBlocks::getState(VanillaBlocks::END_GATEWAY);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    if (!bedrock || !endGateway || !air) {
        return;
    }

    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dy = -2; dy <= 2; ++dy) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                BlockPos blockPos(pos.x + dx, pos.y + dy, pos.z + dz);
                bool sameX = (dx == 0);
                bool sameY = (dy == 0);
                bool sameZ = (dz == 0);
                bool isTopBottomCap = (dy == 2 || dy == -2);

                if (sameX && sameY && sameZ) {
                    // 中心位置：末地折跃门方块
                    world.setBlockState(blockPos, endGateway);
                } else if (sameY) {
                    // 与中心同层但不在中心列：空气（通道）
                    world.setBlockState(blockPos, air);
                } else if (isTopBottomCap && sameX && sameZ) {
                    // 顶/底盖的中心列：基岩
                    world.setBlockState(blockPos, bedrock);
                } else if ((sameX || sameZ) && !isTopBottomCap) {
                    // 侧面十字臂（非顶底盖）：基岩
                    world.setBlockState(blockPos, bedrock);
                } else {
                    // 四角（非中心Y 且非十字臂）：空气
                    world.setBlockState(blockPos, air);
                }
            }
        }
    }
}

// ============================================================================
// ConfiguredEndGatewayFeature 实现
// ============================================================================

ConfiguredEndGatewayFeature::ConfiguredEndGatewayFeature(
    std::unique_ptr<EndGatewayFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredEndGatewayFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    MC_UNUSED(generator);

    // 末地折跃门应稀疏生成，避免每个区块都尝试放置。
    if (!m_config->isExit && random.nextInt(256) != 0) {
        return false;
    }

    const i32 sampleX = chunk.x() * world::CHUNK_WIDTH + random.nextInt(world::CHUNK_WIDTH);
    const i32 sampleZ = chunk.z() * world::CHUNK_WIDTH + random.nextInt(world::CHUNK_WIDTH);
    const i32 topY = region.getTopBlockY(sampleX, sampleZ, HeightmapType::WorldSurfaceWG);

    if (topY <= world::MIN_BUILD_HEIGHT) {
        return false;
    }

    const BlockPos featurePos(sampleX, topY + 1, sampleZ);
    return m_feature.place(region, random, featurePos, *m_config);
}

} // namespace mc
