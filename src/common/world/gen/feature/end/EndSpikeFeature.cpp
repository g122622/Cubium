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

#include "EndSpikeFeature.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/blocks/nether/FireBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"

namespace mc {

// ============================================================================
// EndSpikeFeatureConfig 实现
// ============================================================================

std::vector<EndSpike> EndSpikeFeatureConfig::generateSpikes(u64 worldSeed)
{
    std::vector<EndSpike> spikes;

    // 10根柱子，使用种子随机打乱高度/半径索引
    math::Random rng(worldSeed);
    u64 cacheKey = rng.nextLong() & 65535ULL;
    math::Random shuffleRng(static_cast<u64>(cacheKey));

    // 创建索引 0-9 并打乱
    std::vector<i32> indices = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::shuffle(indices.begin(), indices.end(), std::mt19937(static_cast<u32>(cacheKey)));

    // 生成10根柱子
    for (i32 i = 0; i < 10; ++i) {
        // 角度计算：均匀分布在圆周上
        f64 angle = 2.0 * (-mc::math::PI_DOUBLE + (mc::math::PI_DOUBLE / 10.0) * static_cast<f64>(i));

        // 使用半径 42 围绕中心分布
        i32 x = static_cast<i32>(std::floor(42.0 * std::cos(angle)));
        i32 z = static_cast<i32>(std::floor(42.0 * std::sin(angle)));

        i32 idx = indices[i];
        // 半径范围：2-5（根据索引计算）
        i32 radius = 2 + idx / 3;
        // 高度范围：76-103（根据索引计算）
        i32 height = 76 + idx * 3;
        // 某些柱子有铁栏杆笼子保护
        bool guarded = (idx == 1 || idx == 2);

        spikes.emplace_back(x, z, radius, height, guarded);
    }

    return spikes;
}

// ============================================================================
// EndSpikeFeature 实现
// ============================================================================

bool EndSpikeFeature::place(
    WorldGenRegion& world, math::Random& random, i32 chunkX, i32 chunkZ, const EndSpikeFeatureConfig& config)
{
    // 获取黑曜石柱列表
    const std::vector<EndSpike>& spikes = config.spikes;

    // 如果配置为销毁模式，先销毁所有柱子
    if (config.destroying) {
        const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);
        for (const auto& spike : spikes) {
            // MC 1.21.11: 只有柱子中心在当前区块时才处理
            if (!spike.isCenterWithinChunk(chunkX, chunkZ)) {
                continue;
            }

            // 销毁柱子区域的所有方块
            for (i32 y = 0; y < spike.height; ++y) {
                for (i32 x = -spike.radius; x <= spike.radius; ++x) {
                    for (i32 z = -spike.radius; z <= spike.radius; ++z) {
                        // 圆形截面
                        if (x * x + z * z <= spike.radius * spike.radius) {
                            world.setBlockState(spike.centerX + x, y, spike.centerZ + z, air);
                        }
                    }
                }
            }
        }
        return true;
    }

    // 生成每根柱子
    for (const auto& spike : spikes) {
        // MC 1.21.11: 只有柱子中心在当前区块时才生成
        // 这确保每个 spike 只在一个区块的 feature 放置阶段生成，不会超出 writeRadius
        if (!spike.isCenterWithinChunk(chunkX, chunkZ)) {
            continue;
        }

        _generateSpike(world, random, spike);
    }

    return true;
}

bool EndSpikeFeature::_canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    // 检查位置是否在末地石上
    const BlockState* state = world.getBlockState(pos.x, pos.y - 1, pos.z);
    return state && &state->getBlock() == VanillaBlocks::END_STONE;
}

void EndSpikeFeature::_generateSpike(WorldGenRegion& world, math::Random& random, const EndSpike& spike)
{
    (void)random;

    const BlockState* obsidian = VanillaBlocks::getState(VanillaBlocks::OBSIDIAN);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    // 柱子中心坐标
    i32 baseX = spike.centerX;
    i32 baseZ = spike.centerZ;
    i32 radius = spike.radius;
    i32 height = spike.height;

    // 遍历整个柱子区域（包括上方10格）
    for (i32 x = -radius; x <= radius; ++x) {
        for (i32 z = -radius; z <= radius; ++z) {
            for (i32 y = 0; y <= height + 10; ++y) {
                // 计算到中心的距离平方
                f64 distSq = static_cast<f64>(x * x + z * z);

                if (distSq <= static_cast<f64>(radius * radius + 1) && y < height) {
                    // 在柱子范围内且低于高度：放置黑曜石
                    world.setBlockState(baseX + x, y, baseZ + z, obsidian);
                } else if (y > 65) {
                    // Y > 65 的区域清除为空气
                    world.setBlockState(baseX + x, y, baseZ + z, air);
                }
            }
        }
    }

    // 如果需要笼子，生成铁栏杆
    if (spike.guarded) {
        BlockPos topPos(baseX, height, baseZ);
        _generateCage(world, topPos, radius);
    }
}

void EndSpikeFeature::_generateCage(WorldGenRegion& world, const BlockPos& topPos, i32 radius)
{
    (void)radius; // 参数保留用于未来可能的扩展

    // 使用铁栏杆作为笼子材料
    const BlockState* cageBlock = VanillaBlocks::getState(VanillaBlocks::IRON_BARS);

    if (!cageBlock) {
        return;
    }

    // 生成铁栏杆笼子
    // 循环范围: k, l 从 -2 到 2, y 从 0 到 3
    for (i32 k = -2; k <= 2; ++k) {
        for (i32 l = -2; l <= 2; ++l) {
            for (i32 y = 0; y <= 3; ++y) {
                bool isOuterK = std::abs(k) == 2;
                bool isOuterL = std::abs(l) == 2;
                bool isTop = (y == 3);

                if (isOuterK || isOuterL || isTop) {
                    // 计算铁栏杆连接方向
                    bool connectNS = isOuterK || isTop;
                    bool connectWE = isOuterL || isTop;

                    // 设置方向属性
                    const BlockState* barState = cageBlock;
                    barState = &barState->with(BlockStateProperties::NORTH(), connectNS && l != -2);
                    barState = &barState->with(BlockStateProperties::SOUTH(), connectNS && l != 2);
                    barState = &barState->with(BlockStateProperties::WEST(), connectWE && k != -2);
                    barState = &barState->with(BlockStateProperties::EAST(), connectWE && k != 2);

                    world.setBlockState(topPos.x + k, topPos.y + y, topPos.z + l, barState);
                }
            }
        }
    }

    // 在柱子顶部放置基岩作为水晶底座
    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    if (bedrock) {
        world.setBlockState(topPos.x, topPos.y, topPos.z, bedrock);
    }
}

// ============================================================================
// 运行时柱子放置（用于末影龙重生阶段）
// ============================================================================

void EndSpikeFeature::placeSpike(
    IWorld& world, math::Random& random, const EndSpikeFeatureConfig& config, const EndSpike& spike)
{
    // 对齐 MC 1.21.11 SpikeFeature.placeSpike()
    // 在运行时（非世界生成阶段）放置单根柱子，包括柱体、笼子、基岩底座、末影水晶和底部火焰。
    //
    // 与 _generateSpike（世界生成阶段使用 WorldGenRegion）不同，此方法使用 IWorld& 接口，
    // 由 EndDragonFight 在龙重生序列的 SUMMONING_PILLARS 阶段调用。

    const BlockState* obsidian = VanillaBlocks::getState(VanillaBlocks::OBSIDIAN);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    const i32 baseX = spike.centerX;
    const i32 baseZ = spike.centerZ;
    const i32 radius = spike.radius;
    const i32 height = spike.height;

    // 放置柱体：圆形截面（半径 radius），高度从世界最低 Y 到 spike.height+10
    // MC: for (BlockPos blockpos : BlockPos.betweenClosed(centerX-i, minY, centerZ-i,
    //                                                centerX+i, height+10, centerZ+i))
    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 y = world::MIN_BUILD_HEIGHT; y <= height + 10; ++y) {
                const f64 distSq = static_cast<f64>(dx * dx + dz * dz);
                if (distSq <= static_cast<f64>(radius * radius + 1) && y < height) {
                    // 柱体范围内且低于 height：放置黑曜石
                    world.setBlockState(baseX + dx, y, baseZ + dz, obsidian);
                } else if (y > 65) {
                    // Y > 65 区域清除为空气（顶部清理）
                    world.setBlockState(baseX + dx, y, baseZ + dz, air);
                }
            }
        }
    }

    // 如果有笼子，生成铁栏杆笼子
    if (spike.guarded) {
        BlockPos topPos(baseX, height, baseZ);
        // 复用世界生成阶段的笼子生成逻辑：使用 WorldGenRegion 的 setBlockState。
        // 由于此处只有 IWorld&，内联实现笼子生成逻辑。
        const BlockState* cageBlock = VanillaBlocks::getState(VanillaBlocks::IRON_BARS);
        if (cageBlock != nullptr) {
            for (i32 k = -2; k <= 2; ++k) {
                for (i32 l = -2; l <= 2; ++l) {
                    for (i32 y = 0; y <= 3; ++y) {
                        const bool isOuterK = std::abs(k) == 2;
                        const bool isOuterL = std::abs(l) == 2;
                        const bool isTop = (y == 3);
                        if (isOuterK || isOuterL || isTop) {
                            const bool flag3 = (k == -2) || (k == 2) || isTop;
                            const bool flag4 = (l == -2) || (l == 2) || isTop;
                            const BlockState* barState = cageBlock;
                            barState = &barState->with(BlockStateProperties::NORTH(), flag3 && l != -2);
                            barState = &barState->with(BlockStateProperties::SOUTH(), flag3 && l != 2);
                            barState = &barState->with(BlockStateProperties::WEST(), flag4 && k != -2);
                            barState = &barState->with(BlockStateProperties::EAST(), flag4 && k != 2);
                            world.setBlockState(topPos.x + k, topPos.y + y, topPos.z + l, barState);
                        }
                    }
                }
            }
        }
    }

    // 在柱顶创建末影水晶实体
    // MC: EndCrystal endcrystal = EntityType.END_CRYSTAL.create(level, STRUCTURE);
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* crystalType = registry.getType(entity::EntityTypeKeys::END_CRYSTAL);
    if (crystalType != nullptr) {
        // 通过世界获取 ECS 实体注册表（ServerWorld 持有 m_entityRegistry）
        auto* ecsRegistry = world.entityRegistry();
        if (ecsRegistry != nullptr) {
            std::unique_ptr<Entity> crystalEntity = crystalType->create(&world, *ecsRegistry);
            if (crystalEntity != nullptr) {
                auto* crystal = dynamic_cast<entity::EnderCrystalEntity*>(crystalEntity.get());
                if (crystal != nullptr) {
                    // 设置光束目标（如果配置中有）
                    if (config.crystalBeamTarget.has_value()) {
                        crystal->setBeamTarget(*config.crystalBeamTarget);
                    }
                    // 设置无敌状态（重生阶段中柱顶水晶为无敌）
                    crystal->setInvulnerable(config.crystalInvulnerable);

                    // 设置位置和随机朝向
                    // MC: endcrystal.snapTo(centerX + 0.5, height + 1, centerZ + 0.5,
                    //                       random.nextFloat() * 360.0F, 0.0F);
                    const f32 yaw = random.nextFloat() * 360.0f;
                    crystalEntity->setPosition(Vector3(static_cast<f32>(baseX) + 0.5f,
                        static_cast<f32>(height) + 1.0f,
                        static_cast<f32>(baseZ) + 0.5f));
                    crystalEntity->setRotation(yaw, 0.0f);

                    // 记录水晶位置（用于下方基岩/火焰放置）
                    const BlockPos crystalPos(baseX, height + 1, baseZ);

                    // 加入世界
                    // MC: p_225247_.addFreshEntity(endcrystal);
                    world.spawnEntity(std::move(crystalEntity));

                    // 在水晶下方放置基岩底座
                    // MC: this.setBlock(p_225247_, blockpos1.below(), Blocks.BEDROCK.defaultBlockState());
                    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
                    if (bedrock != nullptr) {
                        world.setBlockState(crystalPos.x, crystalPos.y - 1, crystalPos.z, bedrock);
                    }

                    // 在水晶位置放置火焰
                    // MC: this.setBlock(p_225247_, blockpos1, FireBlock.getState(p_225247_, blockpos1));
                    const BlockState& fireState = blocks::FireBlock::getFireState(world, crystalPos);
                    world.setBlockState(crystalPos.x, crystalPos.y, crystalPos.z, &fireState);
                }
            }
        }
    }
}

// ============================================================================
// ConfiguredEndSpikeFeature 实现
// ============================================================================

ConfiguredEndSpikeFeature::ConfiguredEndSpikeFeature(
    std::unique_ptr<EndSpikeFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredEndSpikeFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    MC_UNUSED(chunk);

    EndSpikeFeatureConfig runtimeConfig = *m_config;
    if (!runtimeConfig.destroying) {
        runtimeConfig.spikes = EndSpikeFeatureConfig::generateSpikes(generator.seed());
    }

    return m_feature.place(region, random, chunk.x(), chunk.z(), runtimeConfig);
}

} // namespace mc
