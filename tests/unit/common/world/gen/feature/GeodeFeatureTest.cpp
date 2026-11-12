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

/**
 * @file GeodeFeatureTest.cpp
 * @brief 晶洞特征测试
 *
 * 覆盖裂缝凿开后的流体补刻：裂缝会连通周围含水方块，原版在凿开时对六个方向上的
 * 流体逐一 `scheduleTick(fluid, 0)`，让水立即流进空腔。生成期调度不执行 tick，
 * 而是写进目标区块的待执行流体刻列表（随区块发布交给世界 TickManager），
 * 因此这里断言「待执行流体刻列表被写入」，而不是断言流体真的流动。
 */

#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/FlatChunkGenerator.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/cave/GeodeFeature.hpp"
#include "server/world/gen/feature/state/BlockStateProvider.hpp"
#include "server/world/gen/settings/FlatLevelGeneratorSettings.hpp"
#include "server/world/gen/valueprovider/IntProvider.hpp"

#include <memory>
#include <vector>

using namespace mc;
using namespace mc::world::gen;
using namespace mc::world::gen::feature::cave;

namespace {

constexpr i32 kOriginX = 8;
constexpr i32 kOriginY = 60;
constexpr i32 kOriginZ = 8;
constexpr i32 kStoneMinY = 40;
constexpr i32 kStoneMaxY = 90;

/// 组装一份能稳定产出裂缝的最小晶洞配置。
///
/// 分布点固定 1 个、偏移全 0，噪声倍率 0，于是 `d6 = 1/sqrt(distSqr)` 只与到原点距离有关；
/// 裂缝概率固定 1.0，三层厚度取到很薄，使原点 5 格内的格子大量落在 `d1` 以内。
/// `baseCrackSize` 取极小值让 `d5` 只由一次 `nextDouble` 决定且仍在裂缝点项数以内
/// （d5 一旦大于裂缝点项数，`d7 >= d5` 永不成立，裂缝分支就成了死代码）。
std::unique_ptr<GeodeConfig> makeCrackedGeodeConfig()
{
    auto config = std::make_unique<GeodeConfig>();
    config->blockSettings.fillingProvider =
        std::make_unique<feature::state::SimpleBlockStateProvider>(&VanillaBlocks::AIR->defaultState());
    config->blockSettings.innerLayerProvider =
        std::make_unique<feature::state::SimpleBlockStateProvider>(&VanillaBlocks::AMETHYST_BLOCK->defaultState());
    config->blockSettings.alternateInnerLayerProvider =
        std::make_unique<feature::state::SimpleBlockStateProvider>(&VanillaBlocks::AMETHYST_BLOCK->defaultState());
    config->blockSettings.middleLayerProvider =
        std::make_unique<feature::state::SimpleBlockStateProvider>(&VanillaBlocks::CALCITE->defaultState());
    config->blockSettings.outerLayerProvider =
        std::make_unique<feature::state::SimpleBlockStateProvider>(&VanillaBlocks::SMOOTH_BASALT->defaultState());
    config->blockSettings.cannotReplace = nullptr;
    config->blockSettings.invalidBlocks = nullptr;

    // 三层厚度很薄 + 分布点只有 1 个：d1/d2/d3/d4 都接近 1，原点附近大量格子进入
    // 「内层以内、外层壁内」的区域（原版数据包里这一带几乎不存在，属测试专用）。
    config->layerSettings.filling = 1.0;
    config->layerSettings.innerLayer = 1.0;
    config->layerSettings.middleLayer = 1.0;
    config->layerSettings.outerLayer = 1.0;

    // 裂缝点必须够远：裂缝判定要求 d7 >= d5（离裂缝点足够近），而晶洞主体要求
    // d6 >= d4；只有当裂缝点远离分布点时两者才可能同时成立。方向取 0/1，
    // 裂缝点落在 (原点.x + j2, 原点.y + 7, 原点.z)，j2 = 2*分布点数 + 1。
    config->crackSettings.generateCrackChance = 1.0;
    config->crackSettings.baseCrackSize = 2.0;
    config->crackSettings.crackPointOffset = 2;

    // 外壁距离必须非 0：d0 = 分布点数 / outerWallDistance 的最大值会进入 d2/d3/d4 的
    // 开方项，取 0 会让它们全部发散成 0，整片区域都被判成「外层壁外」而跳过。
    config->outerWallDistance = valueprovider::ConstantInt::create(8);
    config->distributionPoints = valueprovider::ConstantInt::create(1);
    config->pointOffset = valueprovider::ConstantInt::create(0);
    config->minGenOffset = -6;
    config->maxGenOffset = 6;
    config->noiseMultiplier = 0.0;
    config->invalidBlocksThreshold = 1024;
    config->usePotentialPlacementsChance = 0.0;
    config->useAlternateLayer0Chance = 0.0;
    config->placementsRequireLayer0Alternate = false;
    return config;
}

/**
 * @brief 3x3 生成区域 + 平坦世界生成器
 *
 * 生成器只用来提供 `seed()`（晶洞用它派生 NormalNoise）与 WorldGenRegion 的 tick 容器；
 * 本用例的方块内容由测试自己写入。
 */
class GeodeFeatureTestRegion {
public:
    GeodeFeatureTestRegion()
    {
        VanillaBlocks::initialize();

        m_chunks.reserve(9);
        for (ChunkCoord relZ = -1; relZ <= 1; ++relZ) {
            for (ChunkCoord relX = -1; relX <= 1; ++relX) {
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);
                m_chunks.push_back(chunk.get());
                m_ownedChunks.push_back(std::move(chunk));
            }
        }

        m_region = std::make_unique<WorldGenRegion>(0, 0, 1, m_chunks);
        m_generator = std::make_unique<FlatChunkGenerator>(12345, FlatLevelGeneratorSettings());
    }

    [[nodiscard]] WorldGenRegion& region() const
    {
        return *m_region;
    }

    [[nodiscard]] IChunkGenerator& generator() const
    {
        return *m_generator;
    }

    /// 用同一方块填满以原点为中心、边长为 17 的立方体。
    void fillCubeCenteredAtOrigin(const BlockState* state)
    {
        for (i32 x = kOriginX - 8; x <= kOriginX + 8; ++x) {
            for (i32 y = kOriginY - 8; y <= kOriginY + 8; ++y) {
                for (i32 z = kOriginZ - 8; z <= kOriginZ + 8; ++z) {
                    m_region->setBlockState(x, y, z, state);
                }
            }
        }
    }

    /// 中心区块（0,0）的待执行流体刻数量。
    [[nodiscard]] size_t pendingFluidTickCount() const
    {
        const auto* primer = dynamic_cast<const ChunkPrimer*>(m_region->getIChunk(0, 0));
        return primer != nullptr ? primer->pendingFluidTicks().size() : 0U;
    }

private:
    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    std::unique_ptr<FlatChunkGenerator> m_generator;
};

} // namespace

TEST(GeodeFeatureTest, CrackSchedulesFluidTicksForSurroundingWater)
{
    GeodeFeatureTestRegion fixture;
    WorldGenRegion& region = fixture.region();

    // 原点周围铺一片水：裂缝凿出的空腔紧挨着水，六个方向的调度必然命中若干次。
    // 注意原点四周必须都是水（而不是只有 17³ 的外壳），否则 d6 可能整片落在
    // 「外层壁外」而被跳过。
    const BlockState* water = VanillaBlocks::getState(VanillaBlocks::WATER);
    ASSERT_NE(water, nullptr);
    fixture.fillCubeCenteredAtOrigin(water);
    ASSERT_EQ(fixture.pendingFluidTickCount(), 0U) << "a fresh region must not carry pending fluid ticks";

    auto config = makeCrackedGeodeConfig();
    GeodeFeature geode;
    math::JavaLegacyRandom random(42);
    ASSERT_TRUE(geode.place(region, fixture.generator(), random, BlockPos(kOriginX, kOriginY, kOriginZ), *config));

    // 裂缝凿出空气格子后，邻居的水必须被调度补刻，否则空腔会一直悬空不进水。
    EXPECT_GT(fixture.pendingFluidTickCount(), 0U)
        << "cracked geode must schedule a zero-delay fluid tick for each neighbouring fluid";
}

TEST(GeodeFeatureTest, CrackWithoutSurroundingFluidSchedulesNothing)
{
    GeodeFeatureTestRegion fixture;

    // 周围全是石头：裂缝邻居没有流体，不应产生任何待执行流体刻。
    fixture.fillCubeCenteredAtOrigin(&VanillaBlocks::STONE->defaultState());

    auto config = makeCrackedGeodeConfig();
    GeodeFeature geode;
    math::JavaLegacyRandom random(42);
    ASSERT_TRUE(geode.place(
        fixture.region(), fixture.generator(), random, BlockPos(kOriginX, kOriginY, kOriginZ), *config));

    EXPECT_EQ(fixture.pendingFluidTickCount(), 0U);
}
