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
 * @file test_block_state_provider.cpp
 * @brief BlockStateProvider 多态基类各子类 getState 行为测试
 *
 * 覆盖 Simple / Weighted / RuleBased 的采样行为与 clone 深拷贝独立性，
 * 验证数据驱动迁移后 provider 体系在运行期采样正确。
 */

#include "common/TestWorldHelper.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include "common/world/gen/feature/state/RuleBasedBlockStateProvider.hpp"
#include "common/world/gen/feature/state/SimpleBlockStateProvider.hpp"
#include "common/world/gen/feature/state/WeightedBlockStateProvider.hpp"

#include <gtest/gtest.h>

using namespace mc;

namespace state = mc::world::gen::feature::state;

namespace {
// BaseTestWorld 默认构造为 protected，派生一个 public 构造的测试世界供采样调用。
class ProviderTestWorld : public mc::test::BaseTestWorld {
public:
    ProviderTestWorld() = default;
};
} // namespace

// ============================================================================
// SimpleBlockStateProvider
// ============================================================================

TEST(SimpleBlockStateProviderTest, AlwaysReturnsSameState)
{
    VanillaBlocks::initialize();
    const BlockState* stone = VanillaBlocks::getState(VanillaBlocks::STONE);
    state::SimpleBlockStateProvider provider(stone);

    ProviderTestWorld world;
    math::Random rng(7);

    EXPECT_EQ(provider.getState(world, rng, 0, 0, 0), stone);
    EXPECT_EQ(provider.getState(world, rng, 99, -5, 42), stone);
    EXPECT_EQ(provider.asSingleState(), stone);
}

TEST(SimpleBlockStateProviderTest, CloneProducesIndependentCopy)
{
    VanillaBlocks::initialize();
    const BlockState* stone = VanillaBlocks::getState(VanillaBlocks::STONE);
    state::SimpleBlockStateProvider provider(stone);

    auto cloned = provider.clone();
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->asSingleState(), stone);
}

// ============================================================================
// WeightedBlockStateProvider
// ============================================================================

TEST(WeightedBlockStateProviderTest, SingleEntryAlwaysReturned)
{
    VanillaBlocks::initialize();
    state::WeightedBlockStateProvider provider;
    const BlockState* oak = VanillaBlocks::getState(VanillaBlocks::OAK_LEAVES);
    provider.add(oak, 1);

    ProviderTestWorld world;
    math::Random rng(3);
    for (i32 i = 0; i < 20; ++i) {
        EXPECT_EQ(provider.getState(world, rng, i, 0, 0), oak);
    }
}

TEST(WeightedBlockStateProviderTest, EmptyReturnsNullptr)
{
    state::WeightedBlockStateProvider provider;
    ProviderTestWorld world;
    math::Random rng(3);
    EXPECT_EQ(provider.getState(world, rng, 0, 0, 0), nullptr);
}

TEST(WeightedBlockStateProviderTest, AsSingleStateIsNullptr)
{
    state::WeightedBlockStateProvider provider;
    EXPECT_EQ(provider.asSingleState(), nullptr);
}

// ============================================================================
// RuleBasedBlockStateProvider
// ============================================================================

TEST(RuleBasedBlockStateProviderTest, FallsBackWhenNoRuleMatches)
{
    VanillaBlocks::initialize();
    const BlockState* stone = VanillaBlocks::getState(VanillaBlocks::STONE);
    const BlockState* dirt = VanillaBlocks::getState(VanillaBlocks::DIRT);

    // 无规则：始终回退到 fallback。
    state::RuleBasedBlockStateProvider provider(std::make_unique<state::SimpleBlockStateProvider>(stone), {});

    ProviderTestWorld world;
    math::Random rng(1);
    EXPECT_EQ(provider.getState(world, rng, 0, 0, 0), stone);
    EXPECT_EQ(provider.asSingleState(), nullptr);
}

TEST(RuleBasedBlockStateProviderTest, ClonePreservesFallback)
{
    VanillaBlocks::initialize();
    const BlockState* stone = VanillaBlocks::getState(VanillaBlocks::STONE);

    state::RuleBasedBlockStateProvider provider(std::make_unique<state::SimpleBlockStateProvider>(stone), {});

    auto cloned = provider.clone();
    ASSERT_NE(cloned, nullptr);
    ProviderTestWorld world;
    math::Random rng(5);
    EXPECT_EQ(cloned->getState(world, rng, 0, 0, 0), stone);
}
