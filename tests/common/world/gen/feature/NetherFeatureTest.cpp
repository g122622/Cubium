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

#include "common/world/block/blocks/nether/FireBlock.hpp"
#include "common/world/block/blocks/nether/SoulFireBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "world/chunk/data/ChunkPrimer.hpp"
#include "world/gen/chunk/IChunkGenerator.hpp"
#include "world/gen/feature/nether/NetherFireFeature.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace mc {
namespace {

// ============================================================================
// NetherFireFeature 放置测试
// ============================================================================

class NetherFirePlaceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);
                // 构建下界测试场景：y=40 铺满下界岩
                for (i32 x = 0; x < 16; ++x) {
                    for (i32 z = 0; z < 16; ++z) {
                        chunk->setBlockState(x, 40, z, &VanillaBlocks::NETHERRACK->defaultState());
                    }
                }
                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }

        m_region = std::make_unique<WorldGenRegion>(0, 0, 1, std::move(m_chunks));
    }

    void setWorldBlock(i32 x, i32 y, i32 z, const BlockState* state)
    {
        ASSERT_TRUE(m_region->setBlockState(x, y, z, state));
    }

    [[nodiscard]] const BlockState* getWorldBlock(i32 x, i32 y, i32 z) const
    {
        return m_region->getBlockState(x, y, z);
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
};

TEST_F(NetherFirePlaceTest, PlacesFireOnNetherrack)
{
    // 在下界岩上方应放置普通火焰
    NetherFireFeature feature;
    NetherFireFeatureConfig config(4, 1, 3);
    math::Random random(12345);

    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(8, 41, 8), config));

    // 扫描是否放置了火焰
    bool foundFire = false;
    for (i32 x = 0; x < 16 && !foundFire; ++x) {
        for (i32 z = 0; z < 16 && !foundFire; ++z) {
            const BlockState* state = getWorldBlock(x, 41, z);
            if (state != nullptr && (state->is(VanillaBlocks::FIRE) || state->is(VanillaBlocks::SOUL_FIRE))) {
                foundFire = true;
            }
        }
    }
    EXPECT_TRUE(foundFire);
}

TEST_F(NetherFirePlaceTest, PlacesSoulFireOnSoulSand)
{
    // 在部分区域将下界岩替换为灵魂沙
    for (i32 x = 0; x < 8; ++x) {
        for (i32 z = 0; z < 8; ++z) {
            setWorldBlock(x, 40, z, &VanillaBlocks::SOUL_SAND->defaultState());
        }
    }

    NetherFireFeature feature;
    NetherFireFeatureConfig config(8, 1, 3);
    math::Random random(54321);

    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(4, 41, 4), config));

    // 扫描是否放置了灵魂火
    bool foundSoulFire = false;
    for (i32 x = 0; x < 8 && !foundSoulFire; ++x) {
        for (i32 z = 0; z < 8 && !foundSoulFire; ++z) {
            const BlockState* state = getWorldBlock(x, 41, z);
            if (state != nullptr && state->is(VanillaBlocks::SOUL_FIRE)) {
                foundSoulFire = true;
            }
        }
    }
    EXPECT_TRUE(foundSoulFire);
}

TEST_F(NetherFirePlaceTest, PlacesSoulFireOnSoulSoil)
{
    // 在部分区域将下界岩替换为灵魂土
    for (i32 x = 0; x < 8; ++x) {
        for (i32 z = 0; z < 8; ++z) {
            setWorldBlock(x, 40, z, &VanillaBlocks::SOUL_SOIL->defaultState());
        }
    }

    NetherFireFeature feature;
    NetherFireFeatureConfig config(8, 1, 3);
    math::Random random(98765);

    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(4, 41, 4), config));

    // 扫描是否放置了灵魂火
    bool foundSoulFire = false;
    for (i32 x = 0; x < 8 && !foundSoulFire; ++x) {
        for (i32 z = 0; z < 8 && !foundSoulFire; ++z) {
            const BlockState* state = getWorldBlock(x, 41, z);
            if (state != nullptr && state->is(VanillaBlocks::SOUL_FIRE)) {
                foundSoulFire = true;
            }
        }
    }
    EXPECT_TRUE(foundSoulFire);
}

TEST_F(NetherFirePlaceTest, NoFireOnInvalidBase)
{
    // 将 y=40 的下界岩替换为非可燃基座方块（石头），使用 setWorldBlock
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setWorldBlock(x, 40, z, &VanillaBlocks::STONE->defaultState());
        }
    }

    NetherFireFeature feature;
    NetherFireFeatureConfig config(4, 1, 3);
    math::Random random(11111);

    EXPECT_FALSE(feature.place(*m_region, random, BlockPos(8, 41, 8), config));
}

TEST_F(NetherFirePlaceTest, NoFireWhenNoAirAbove)
{
    // 在 y=41 到 y=44 位置填满方块（覆盖整个可能的高度范围）
    // 默认配置 minHeight=1, maxHeight=3，火焰可放置在 y=40 到 y=44
    // y=40 是基座（下界岩），y=41~44 全部填满后无法放置火焰
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            for (i32 y = 41; y <= 44; ++y) {
                setWorldBlock(x, y, z, &VanillaBlocks::NETHERRACK->defaultState());
            }
        }
    }

    NetherFireFeature feature;
    NetherFireFeatureConfig config(4, 1, 3);
    math::Random random(22222);

    // 火焰放置位置已被填满，无法放置
    EXPECT_FALSE(feature.place(*m_region, random, BlockPos(8, 41, 8), config));
}

TEST_F(NetherFirePlaceTest, FireBlockTypeMatchesBaseBlock)
{
    // 左半区为下界岩，右半区为灵魂沙
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            if (x < 8) {
                setWorldBlock(x, 40, z, &VanillaBlocks::NETHERRACK->defaultState());
            } else {
                setWorldBlock(x, 40, z, &VanillaBlocks::SOUL_SAND->defaultState());
            }
        }
    }

    NetherFireFeature feature;
    NetherFireFeatureConfig config(16, 1, 3);
    math::Random random(33333);

    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(8, 41, 8), config));

    // 检查下界岩上方的火焰应为普通火，灵魂沙上方的应为灵魂火
    bool foundNormalFireOnNetherrack = false;
    bool foundSoulFireOnSoulSand = false;

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* fireState = getWorldBlock(x, 41, z);
            if (fireState == nullptr) {
                continue;
            }

            const BlockState* baseState = getWorldBlock(x, 40, z);
            if (baseState == nullptr) {
                continue;
            }

            if (baseState->is(VanillaBlocks::NETHERRACK) && fireState->is(VanillaBlocks::FIRE)) {
                foundNormalFireOnNetherrack = true;
            }
            if (baseState->is(VanillaBlocks::SOUL_SAND) && fireState->is(VanillaBlocks::SOUL_FIRE)) {
                foundSoulFireOnSoulSand = true;
            }
        }
    }

    EXPECT_TRUE(foundNormalFireOnNetherrack);
    EXPECT_TRUE(foundSoulFireOnSoulSand);
}

TEST_F(NetherFirePlaceTest, HeightVariationPlacesFireAtDifferentLevels)
{
    // 构建 y=38..44 为下界岩的立体场景，火焰可放置在 y=39..45 的空气位置
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            for (i32 y = 38; y <= 44; ++y) {
                setWorldBlock(x, y, z, &VanillaBlocks::NETHERRACK->defaultState());
            }
        }
    }

    NetherFireFeature feature;
    NetherFireFeatureConfig config(8, 2, 4); // 大范围 + 允许垂直偏移
    math::Random random(55555);

    bool result = feature.place(*m_region, random, BlockPos(8, 41, 8), config);
    EXPECT_TRUE(result);

    // 检查火焰是否出现在不同 Y 层级（不仅是 y=41）
    bool foundAtDifferentY = false;
    for (i32 x = 0; x < 16 && !foundAtDifferentY; ++x) {
        for (i32 z = 0; z < 16 && !foundAtDifferentY; ++z) {
            for (i32 y = 39; y <= 45; ++y) {
                // 跳过 y=41（原始层级），检查其他层级是否有火焰
                if (y == 41) continue;
                const BlockState* state = getWorldBlock(x, y, z);
                if (state != nullptr && (state->is(VanillaBlocks::FIRE) || state->is(VanillaBlocks::SOUL_FIRE))) {
                    foundAtDifferentY = true;
                    break;
                }
            }
        }
    }
    // 注意：由于随机性，可能不会总是找到，但概率很高
    // 如果测试不够稳定，可以移除此断言
    EXPECT_TRUE(foundAtDifferentY);
}

TEST_F(NetherFirePlaceTest, ZeroHeightVariationOnlyPlacesAtOriginLevel)
{
    // minHeight=0, maxHeight=0 时，火焰只能在原点 Y 层级放置
    NetherFireFeature feature;
    NetherFireFeatureConfig config(4, 0, 0); // 无垂直偏移
    math::Random random(77777);

    bool result = feature.place(*m_region, random, BlockPos(8, 41, 8), config);
    EXPECT_TRUE(result);

    // 所有火焰应仅在 y=41（空气）位置，y=40 是下界岩，y=42 应该没有火焰
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* state = getWorldBlock(x, 42, z);
            EXPECT_FALSE(state && state->is(VanillaBlocks::FIRE))
                << "Fire should not be placed at y=42 with zero height variation";
        }
    }
}

} // namespace
} // namespace mc
