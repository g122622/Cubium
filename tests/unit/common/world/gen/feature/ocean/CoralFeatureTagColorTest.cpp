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
 * @file CoralFeatureTagColorTest.cpp
 * @brief 珊瑚颜色由 coral_blocks 方块标签抽签决定
 *
 * 珊瑚类特征的配置里没有颜色字段：原版每次放置都从 `BlockTags.CORAL_BLOCKS`
 * 均匀随机取一个珊瑚方块，用它的颜色作为整株珊瑚的颜色。这条抽签必须发生在
 * 任何形状掷骰之前，否则随机流与离原版一致；颜色也不能退回成配置里的固定值。
 *
 * 这里钉住两件事：
 * 1. `BlockTags::CORAL_BLOCKS` 确实由数据包加载、且恰好是五种活珊瑚块；
 * 2. 同一配置、不同种子下放置出的珊瑚底座颜色会出现多种，而不是恒定一种。
 */

#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/tags/BlockTags.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/ocean/CoralFeature.hpp"

#include <array>
#include <memory>
#include <set>
#include <vector>

using namespace mc;

namespace {

/// 五种活珊瑚方块
constexpr std::array<const char*, 5> kCoralBlockNames = {
    "tube_coral_block", "brain_coral_block", "bubble_coral_block", "fire_coral_block", "horn_coral_block"};

/**
 * @brief 只实现珊瑚放置所用到成员的生成器桩
 *
 * 珊瑚放置不推进区块生成状态，只需要能构造出来。
 */
class StubChunkGenerator final : public IChunkGenerator {
public:
    void generateStructureStarts(WorldGenRegion&, ChunkPrimer&) override {}

    void generateStructureReferences(WorldGenRegion&, ChunkPrimer&) override {}

    void generateBiomes(WorldGenRegion&, ChunkPrimer&) override {}

    void generateNoise(WorldGenRegion&, ChunkPrimer&) override {}

    void buildSurface(WorldGenRegion&, ChunkPrimer&) override {}

    void applyCarvers(WorldGenRegion&, ChunkPrimer&) override {}

    void placeFeatures(WorldGenRegion&, ChunkPrimer&) override {}

    [[nodiscard]] i32 spawnInitialMobs(WorldGenRegion&, ChunkPrimer&, std::vector<SpawnedEntityData>&) override
    {
        return 0;
    }

    [[nodiscard]] BiomeId getBiome(i32, i32, i32) const override
    {
        return 0;
    }

    [[nodiscard]] BiomeId getNoiseBiome(i32, i32, i32) const override
    {
        return 0;
    }

    [[nodiscard]] i32 getHeight(i32, i32, HeightmapType) const override
    {
        return 62;
    }

    [[nodiscard]] u64 seed() const override
    {
        return 0;
    }

    [[nodiscard]] const DimensionSettings& settings() const override
    {
        return m_settings;
    }

    [[nodiscard]] i32 seaLevel() const override
    {
        return m_settings.seaLevel;
    }

private:
    DimensionSettings m_settings{};
};

} // namespace

class CoralFeatureTagColorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }
};

/**
 * @test coral_blocks 标签由数据包加载，且恰为五种活珊瑚块
 *
 * 若标签没被加载（或读成了空表），珊瑚放置会直接返回 false、自然生成里整片没有珊瑚，
 * 且不会有任何报错。这里把标签内容本身钉住。
 */
TEST_F(CoralFeatureTagColorTest, CoralBlocksTagHoldsFiveLiveCoralBlocks)
{
    ASSERT_TRUE(BlockTags::isInitialized()) << "block tags must be loaded from data packs in tests";

    const auto& blockIds = BlockTags::CORAL_BLOCKS().getBlockIds();
    EXPECT_EQ(blockIds.size(), 5u) << "coral_blocks must contain exactly the five live coral blocks";

    auto& registry = BlockRegistry::instance();
    for (const char* name : kCoralBlockNames) {
        const Block* block = registry.getBlock(ResourceLocation(std::string("minecraft:") + name));
        ASSERT_NE(block, nullptr) << name << " must be registered";
        EXPECT_TRUE(BlockTags::CORAL_BLOCKS().contains(block->defaultState()))
            << name << " must be a member of coral_blocks";
    }

    // 死珊瑚不能出现在标签里：它会让自然生成出现枯死的底座。
    if (VanillaBlocks::DEAD_TUBE_CORAL_BLOCK != nullptr) {
        EXPECT_FALSE(BlockTags::CORAL_BLOCKS().contains(VanillaBlocks::DEAD_TUBE_CORAL_BLOCK->defaultState()))
            << "dead corals must not be in coral_blocks";
    }
}

/**
 * @test 同一配置下，不同种子放置出的珊瑚颜色不止一种
 *
 * 珊瑚特征构造时配置里的颜色是占位值（原版该特征没有配置字段）。若放置逻辑仍沿用
 * 配置颜色，那么无论种子如何都只会得到一种颜色；只有真正从 coral_blocks 抽签，
 * 颜色集合才会随种子变化。这里也顺带确认放置确实发生了（水域 + 海底砂）。
 */
TEST_F(CoralFeatureTagColorTest, ColorIsDrawnFromTagNotFromConfig)
{
    ASSERT_TRUE(BlockTags::isInitialized());

    // 半径 0 的单区块区域：自底向上铺海底砂、其上填水到海平面附近。
    constexpr i32 kFloorY = 50;
    std::vector<std::unique_ptr<ChunkPrimer>> primers;
    primers.push_back(std::make_unique<ChunkPrimer>(0, 0));
    std::vector<IChunk*> ptrs{primers.front().get()};
    WorldGenRegion region(0, 0, 0, ptrs);

    ChunkPrimer& chunk = *primers.front();
    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
            chunk.setBlockState(x, kFloorY, z, &VanillaBlocks::SAND->defaultState());
            for (i32 y = kFloorY + 1; y <= 62; ++y) {
                chunk.setBlockState(x, y, z, &VanillaBlocks::WATER->defaultState());
            }
        }
    }

    // 配置里的颜色给一个确定值；若实现忽略抽签就会恒定输出这一种。
    CoralFeatureConfig config(blocks::CoralColor::Tube);

    // 直接写 setBlockState 不会自动维护高度图，而珊瑚放置要靠 OceanFloorWG 找海底高度；
    // 手动铺完地形后必须重建一次，否则高度图仍是空列，放置会在前置检查处返回 false。
    chunk.updateAllHeightmaps();

    std::set<u32> baseBlockIds;
    for (i32 seed = 1; seed <= 64; ++seed) {
        CoralFeature feature;
        math::Random random(seed);
        // 同一份配置、不同种子重复放置，收集实际写出的底座颜色。
        MC_UNUSED(feature.place(region, random, BlockPos(0, kFloorY, 0), config));

        for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
            for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
                // 珊瑚底座可能被树形/蘑菇形向上叠加若干格，因此整列扫描地板以上部分。
                for (i32 y = kFloorY; y <= 62; ++y) {
                    const BlockState* state = chunk.getBlockState(x, y, z);
                    if (state == nullptr) {
                        continue;
                    }
                    for (const char* name : kCoralBlockNames) {
                        const Block* coralBlock =
                            BlockRegistry::instance().getBlock(ResourceLocation(std::string("minecraft:") + name));
                        if (coralBlock != nullptr && state->blockId() == coralBlock->blockId()) {
                            baseBlockIds.insert(state->blockId());
                        }
                    }
                }
            }
        }
    }

    EXPECT_FALSE(baseBlockIds.empty()) << "coral placement must actually write coral blocks";
    EXPECT_GT(baseBlockIds.size(), 1u)
        << "coral color must be drawn from the coral_blocks tag, not fixed by the config";
}
