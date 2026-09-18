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
 * IMPLIED, INCLUDING WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/Constants.hpp"

using namespace mc;

namespace {

// ============================================================================
// 测试辅助类：实现 IGrowable 接口的 GROWER 类型方块
// ============================================================================
class MockGrowerBlock : public IGrowable {
public:
    [[nodiscard]] bool canGrow(IBlockReader& /*world*/,
        const BlockPos& /*pos*/,
        const BlockState& /*state*/,
        bool /*isClientSide*/) const override
    {
        return true;
    }

    [[nodiscard]] bool canUseBonemeal(IWorld& /*world*/,
        math::IRandom& /*random*/,
        const BlockPos& /*pos*/,
        const BlockState& /*state*/) const override
    {
        return true;
    }

    void grow(
        IWorld& /*world*/, math::IRandom& /*random*/, const BlockPos& /*pos*/, const BlockState& /*state*/) override
    {}

    // 默认 getBoneMealType() 返回 GROWER
    // 默认 getParticlePos() 根据 getBoneMealType() 返回 pos
};

// ============================================================================
// 测试辅助类：实现 IGrowable 接口的 NEIGHBOR_SPREADER 类型方块
// ============================================================================
class MockSpreaderBlock : public IGrowable {
public:
    [[nodiscard]] bool canGrow(IBlockReader& /*world*/,
        const BlockPos& /*pos*/,
        const BlockState& /*state*/,
        bool /*isClientSide*/) const override
    {
        return true;
    }

    [[nodiscard]] bool canUseBonemeal(IWorld& /*world*/,
        math::IRandom& /*random*/,
        const BlockPos& /*pos*/,
        const BlockState& /*state*/) const override
    {
        return true;
    }

    void grow(
        IWorld& /*world*/, math::IRandom& /*random*/, const BlockPos& /*pos*/, const BlockState& /*state*/) override
    {}

    [[nodiscard]] BoneMealType getBoneMealType() const override { return BoneMealType::NEIGHBOR_SPREADER; }
};

// ============================================================================
// BoneMealType 枚举测试
// ============================================================================

TEST(IGrowableBoneMealTypeTest, DefaultTypeIsGrower)
{
    MockGrowerBlock grower;
    EXPECT_EQ(grower.getBoneMealType(), IGrowable::BoneMealType::GROWER);
}

TEST(IGrowableBoneMealTypeTest, SpreaderTypeOverridesDefault)
{
    MockSpreaderBlock spreader;
    EXPECT_EQ(spreader.getBoneMealType(), IGrowable::BoneMealType::NEIGHBOR_SPREADER);
}

// ============================================================================
// getParticlePos 测试
// ============================================================================

TEST(IGrowableBoneMealTypeTest, GrowerParticlePosIsSameAsBlockPos)
{
    MockGrowerBlock grower;
    BlockPos pos(10, 20, 30);
    BlockPos particlePos = grower.getParticlePos(pos);

    EXPECT_EQ(particlePos.x, 10);
    EXPECT_EQ(particlePos.y, 20);
    EXPECT_EQ(particlePos.z, 30);
}

TEST(IGrowableBoneMealTypeTest, SpreaderParticlePosIsAboveBlockPos)
{
    MockSpreaderBlock spreader;
    BlockPos pos(10, 20, 30);
    BlockPos particlePos = spreader.getParticlePos(pos);

    EXPECT_EQ(particlePos.x, 10);
    EXPECT_EQ(particlePos.y, 21); // pos.above() = y + 1
    EXPECT_EQ(particlePos.z, 30);
}

TEST(IGrowableBoneMealTypeTest, GrowerParticlePosAtOrigin)
{
    MockGrowerBlock grower;
    BlockPos pos(0, 0, 0);
    BlockPos particlePos = grower.getParticlePos(pos);

    EXPECT_EQ(particlePos.x, 0);
    EXPECT_EQ(particlePos.y, 0);
    EXPECT_EQ(particlePos.z, 0);
}

TEST(IGrowableBoneMealTypeTest, SpreaderParticlePosAtOrigin)
{
    MockSpreaderBlock spreader;
    BlockPos pos(0, 0, 0);
    BlockPos particlePos = spreader.getParticlePos(pos);

    EXPECT_EQ(particlePos.x, 0);
    EXPECT_EQ(particlePos.y, 1);
    EXPECT_EQ(particlePos.z, 0);
}

TEST(IGrowableBoneMealTypeTest, GrowerParticlePosAtMaxHeight)
{
    MockGrowerBlock grower;
    BlockPos pos(100, 319, 100);
    BlockPos particlePos = grower.getParticlePos(pos);

    EXPECT_EQ(particlePos.y, 319);
}

TEST(IGrowableBoneMealTypeTest, SpreaderParticlePosAtMaxHeight)
{
    MockSpreaderBlock spreader;
    BlockPos pos(100, 319, 100);
    BlockPos particlePos = spreader.getParticlePos(pos);

    EXPECT_EQ(particlePos.y, 320);
}

// ============================================================================
// 接口多态测试
// ============================================================================

TEST(IGrowableBoneMealTypeTest, PolymorphicBoneMealType)
{
    MockGrowerBlock grower;
    MockSpreaderBlock spreader;

    IGrowable* igrowable1 = &grower;
    IGrowable* igrowable2 = &spreader;

    EXPECT_EQ(igrowable1->getBoneMealType(), IGrowable::BoneMealType::GROWER);
    EXPECT_EQ(igrowable2->getBoneMealType(), IGrowable::BoneMealType::NEIGHBOR_SPREADER);
}

TEST(IGrowableBoneMealTypeTest, PolymorphicParticlePos)
{
    MockGrowerBlock grower;
    MockSpreaderBlock spreader;

    IGrowable* igrowable1 = &grower;
    IGrowable* igrowable2 = &spreader;

    BlockPos pos(5, 10, 15);
    EXPECT_EQ(igrowable1->getParticlePos(pos), BlockPos(5, 10, 15));
    EXPECT_EQ(igrowable2->getParticlePos(pos), BlockPos(5, 11, 15));
}

} // namespace
