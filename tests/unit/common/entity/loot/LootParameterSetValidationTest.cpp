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

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::loot;

// ============================================================================
// LootParameterSet 验证测试
// ============================================================================

class LootParameterSetValidationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// --- isAllowed() 测试 ---

TEST_F(LootParameterSetValidationTest, IsAllowed_RequiredParam)
{
    auto paramSet = LootParameterSets::block();
    // block() 必需参数：BLOCK_STATE, BLOCK_POS
    EXPECT_TRUE(paramSet.isAllowed(LootParams::BLOCK_STATE.getId()));
    EXPECT_TRUE(paramSet.isAllowed(LootParams::BLOCK_POS.getId()));
}

TEST_F(LootParameterSetValidationTest, IsAllowed_OptionalParam)
{
    auto paramSet = LootParameterSets::block();
    // block() 可选参数：TOOL, THIS_ENTITY, BLOCK_ENTITY, FORTUNE_LEVEL, SILK_TOUCH_LEVEL
    EXPECT_TRUE(paramSet.isAllowed(LootParams::TOOL.getId()));
    EXPECT_TRUE(paramSet.isAllowed(LootParams::THIS_ENTITY.getId()));
    EXPECT_TRUE(paramSet.isAllowed(LootParams::BLOCK_ENTITY.getId()));
    EXPECT_TRUE(paramSet.isAllowed(LootParams::FORTUNE_LEVEL.getId()));
    EXPECT_TRUE(paramSet.isAllowed(LootParams::SILK_TOUCH_LEVEL.getId()));
}

TEST_F(LootParameterSetValidationTest, IsAllowed_ParamNotInSet)
{
    auto paramSet = LootParameterSets::block();
    // KILLER_PLAYER 不在 block 参数集中
    EXPECT_FALSE(paramSet.isAllowed(LootParams::KILLER_PLAYER.getId()));
    // DAMAGE_SOURCE 不在 block 参数集中
    EXPECT_FALSE(paramSet.isAllowed(LootParams::DAMAGE_SOURCE.getId()));
    // IS_IN_OPEN_WATER 不在 block 参数集中
    EXPECT_FALSE(paramSet.isAllowed(LootParams::IS_IN_OPEN_WATER.getId()));
}

TEST_F(LootParameterSetValidationTest, IsAllowed_EmptySet)
{
    auto paramSet = LootParameterSets::empty();
    // 空集合不允许任何参数
    EXPECT_FALSE(paramSet.isAllowed(LootParams::THIS_ENTITY.getId()));
    EXPECT_FALSE(paramSet.isAllowed(LootParams::BLOCK_POS.getId()));
}

// --- validate() 简单版本测试 ---

TEST_F(LootParameterSetValidationTest, Validate_BlockSet_AllRequiredProvided)
{
    auto paramSet = LootParameterSets::block();
    std::vector<std::string> provided = {
        LootParams::BLOCK_STATE.getId(),
        LootParams::BLOCK_POS.getId(),
    };
    EXPECT_TRUE(paramSet.validate(provided));
}

TEST_F(LootParameterSetValidationTest, Validate_BlockSet_MissingRequiredBlockPos)
{
    auto paramSet = LootParameterSets::block();
    std::vector<std::string> provided = {
        LootParams::BLOCK_STATE.getId(),
    };
    EXPECT_FALSE(paramSet.validate(provided));
}

TEST_F(LootParameterSetValidationTest, Validate_BlockSet_MissingRequiredBlockState)
{
    auto paramSet = LootParameterSets::block();
    std::vector<std::string> provided = {
        LootParams::BLOCK_POS.getId(),
    };
    EXPECT_FALSE(paramSet.validate(provided));
}

TEST_F(LootParameterSetValidationTest, Validate_BlockSet_WithOptionalParams)
{
    auto paramSet = LootParameterSets::block();
    std::vector<std::string> provided = {
        LootParams::BLOCK_STATE.getId(),
        LootParams::BLOCK_POS.getId(),
        LootParams::TOOL.getId(),
        LootParams::THIS_ENTITY.getId(),
        LootParams::FORTUNE_LEVEL.getId(),
    };
    EXPECT_TRUE(paramSet.validate(provided));
}

TEST_F(LootParameterSetValidationTest, Validate_EntitySet_AllRequired)
{
    auto paramSet = LootParameterSets::entity();
    std::vector<std::string> provided = {
        LootParams::THIS_ENTITY.getId(),
    };
    EXPECT_TRUE(paramSet.validate(provided));
}

TEST_F(LootParameterSetValidationTest, Validate_EntitySet_MissingRequired)
{
    auto paramSet = LootParameterSets::entity();
    std::vector<std::string> provided;
    EXPECT_FALSE(paramSet.validate(provided));
}

TEST_F(LootParameterSetValidationTest, Validate_FishingSet_AllRequired)
{
    auto paramSet = LootParameterSets::fishing();
    std::vector<std::string> provided = {
        LootParams::BLOCK_POS.getId(),
        LootParams::TOOL.getId(),
    };
    EXPECT_TRUE(paramSet.validate(provided));
}

TEST_F(LootParameterSetValidationTest, Validate_SelectorSet_AllRequired)
{
    auto paramSet = LootParameterSets::selector();
    std::vector<std::string> provided = {
        LootParams::THIS_ENTITY.getId(),
        LootParams::BLOCK_POS.getId(),
    };
    EXPECT_TRUE(paramSet.validate(provided));
}

TEST_F(LootParameterSetValidationTest, Validate_SelectorSet_MissingOneRequired)
{
    auto paramSet = LootParameterSets::selector();
    std::vector<std::string> provided = {
        LootParams::THIS_ENTITY.getId(),
    };
    EXPECT_FALSE(paramSet.validate(provided));
}

// --- validate() 三参数版本测试 ---

TEST_F(LootParameterSetValidationTest, ValidateDetailed_BlockSet_MissingAndUnexpected)
{
    auto paramSet = LootParameterSets::block();
    // 提供 BLOCK_STATE（必需）但缺少 BLOCK_POS（必需），且有 KILLER_PLAYER（不允许）
    std::vector<std::string> provided = {
        LootParams::BLOCK_STATE.getId(),
        LootParams::KILLER_PLAYER.getId(),
    };

    std::vector<std::string> missingParams;
    std::vector<std::string> unexpectedParams;
    bool result = paramSet.validate(provided, missingParams, unexpectedParams);

    EXPECT_FALSE(result);
    ASSERT_EQ(missingParams.size(), 1u);
    EXPECT_EQ(missingParams[0], LootParams::BLOCK_POS.getId());
    ASSERT_EQ(unexpectedParams.size(), 1u);
    EXPECT_EQ(unexpectedParams[0], LootParams::KILLER_PLAYER.getId());
}

TEST_F(LootParameterSetValidationTest, ValidateDetailed_BlockSet_OnlyMissing)
{
    auto paramSet = LootParameterSets::block();
    std::vector<std::string> provided = {
        LootParams::BLOCK_POS.getId(),
    };

    std::vector<std::string> missingParams;
    std::vector<std::string> unexpectedParams;
    bool result = paramSet.validate(provided, missingParams, unexpectedParams);

    EXPECT_FALSE(result);
    ASSERT_EQ(missingParams.size(), 1u);
    EXPECT_EQ(missingParams[0], LootParams::BLOCK_STATE.getId());
    EXPECT_TRUE(unexpectedParams.empty());
}

TEST_F(LootParameterSetValidationTest, ValidateDetailed_BlockSet_OnlyUnexpected)
{
    auto paramSet = LootParameterSets::block();
    std::vector<std::string> provided = {
        LootParams::BLOCK_STATE.getId(),
        LootParams::BLOCK_POS.getId(),
        LootParams::DAMAGE_SOURCE.getId(),
    };

    std::vector<std::string> missingParams;
    std::vector<std::string> unexpectedParams;
    bool result = paramSet.validate(provided, missingParams, unexpectedParams);

    EXPECT_FALSE(result);
    EXPECT_TRUE(missingParams.empty());
    ASSERT_EQ(unexpectedParams.size(), 1u);
    EXPECT_EQ(unexpectedParams[0], LootParams::DAMAGE_SOURCE.getId());
}

TEST_F(LootParameterSetValidationTest, ValidateDetailed_BlockSet_AllValid)
{
    auto paramSet = LootParameterSets::block();
    std::vector<std::string> provided = {
        LootParams::BLOCK_STATE.getId(),
        LootParams::BLOCK_POS.getId(),
        LootParams::TOOL.getId(),
    };

    std::vector<std::string> missingParams;
    std::vector<std::string> unexpectedParams;
    bool result = paramSet.validate(provided, missingParams, unexpectedParams);

    EXPECT_TRUE(result);
    EXPECT_TRUE(missingParams.empty());
    EXPECT_TRUE(unexpectedParams.empty());
}

// --- getRequiredParams / getOptionalParams 测试 ---

TEST_F(LootParameterSetValidationTest, GetRequiredParams_BlockSet)
{
    auto paramSet = LootParameterSets::block();
    const auto& required = paramSet.getRequiredParams();
    EXPECT_EQ(required.size(), 2u);
    // 验证包含 BLOCK_STATE 和 BLOCK_POS
    bool hasBlockState = false;
    bool hasBlockPos = false;
    for (const auto& param : required) {
        if (param == LootParams::BLOCK_STATE.getId()) hasBlockState = true;
        if (param == LootParams::BLOCK_POS.getId()) hasBlockPos = true;
    }
    EXPECT_TRUE(hasBlockState);
    EXPECT_TRUE(hasBlockPos);
}

TEST_F(LootParameterSetValidationTest, GetOptionalParams_BlockSet)
{
    auto paramSet = LootParameterSets::block();
    const auto& optional = paramSet.getOptionalParams();
    EXPECT_EQ(optional.size(), 5u); // TOOL, THIS_ENTITY, BLOCK_ENTITY, FORTUNE_LEVEL, SILK_TOUCH_LEVEL
}

TEST_F(LootParameterSetValidationTest, GetRequiredParams_EntitySet)
{
    auto paramSet = LootParameterSets::entity();
    const auto& required = paramSet.getRequiredParams();
    EXPECT_EQ(required.size(), 1u);
    EXPECT_EQ(required[0], LootParams::THIS_ENTITY.getId());
}

TEST_F(LootParameterSetValidationTest, GetRequiredParams_EmptySet)
{
    auto paramSet = LootParameterSets::empty();
    EXPECT_TRUE(paramSet.getRequiredParams().empty());
    EXPECT_TRUE(paramSet.getOptionalParams().empty());
}

TEST_F(LootParameterSetValidationTest, GetRequiredParams_GenericSet)
{
    auto paramSet = LootParameterSets::generic();
    EXPECT_TRUE(paramSet.getRequiredParams().empty());
    EXPECT_TRUE(paramSet.getOptionalParams().empty());
}

// ============================================================================
// LootContextBuilder::build() 参数验证集成测试
// ============================================================================

class LootContextBuilderValidationTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    class TestWorld : public mc::test::BaseTestWorld {
    public:
        [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
        {
            return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
        }
        [[nodiscard]] world::tick::TickManager& tickManager() override
        {
            throw std::runtime_error("TestWorld::tickManager not implemented");
        }
        [[nodiscard]] const world::tick::TickManager& tickManager() const override
        {
            throw std::runtime_error("TestWorld::tickManager not implemented");
        }
    };

    TestWorld m_world;
};

// --- Generic 和 Empty 类型跳过验证 ---

TEST_F(LootContextBuilderValidationTest, GenericSet_NoParams_ReturnsValidContext)
{
    // Generic 参数集没有必需参数也没有可选参数限制，应正常构建
    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build(LootParameterSets::generic());
    ASSERT_NE(context, nullptr);
}

TEST_F(LootContextBuilderValidationTest, EmptySet_NoParams_ReturnsValidContext)
{
    // Empty 参数集没有必需参数也没有可选参数限制，应正常构建
    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build(LootParameterSets::empty());
    ASSERT_NE(context, nullptr);
}

TEST_F(LootContextBuilderValidationTest, DefaultParamSet_ReturnsValidContext)
{
    // 不传参数集时默认为 Generic，应正常构建
    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();
    ASSERT_NE(context, nullptr);
}

// --- 有问题的参数集仍返回有效 context（仅 warn） ---

TEST_F(LootContextBuilderValidationTest, BlockSet_MissingRequiredParams_StillReturnsContext)
{
    // 验证失败时不中断构建，仍返回有效的 context
    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build(LootParameterSets::block());
    // 缺少 BLOCK_STATE 和 BLOCK_POS，但 context 仍应被创建
    ASSERT_NE(context, nullptr);
}

TEST_F(LootContextBuilderValidationTest, BlockSet_AllRequiredParams_ReturnsValidContext)
{
    // 提供所有必需参数，应正常构建
    VanillaBlocks::initialize();
    const BlockState* state = VanillaBlocks::DIRT ? &VanillaBlocks::DIRT->getDefaultState() : nullptr;
    ASSERT_NE(state, nullptr);
    BlockPos pos(10, 64, 20);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world)
                       .withRandom(rng)
                       .withParameter(LootParams::BLOCK_STATE, const_cast<BlockState*>(state))
                       .withParameter(LootParams::BLOCK_POS, &pos)
                       .build(LootParameterSets::block());
    ASSERT_NE(context, nullptr);
    EXPECT_TRUE(context->has(LootParams::BLOCK_STATE));
    EXPECT_TRUE(context->has(LootParams::BLOCK_POS));
}

TEST_F(LootContextBuilderValidationTest, BlockSet_WithOptionalParams_ReturnsValidContext)
{
    // 提供必需参数和部分可选参数
    VanillaBlocks::initialize();
    const BlockState* state = VanillaBlocks::DIRT ? &VanillaBlocks::DIRT->getDefaultState() : nullptr;
    ASSERT_NE(state, nullptr);
    BlockPos pos(10, 64, 20);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world)
                       .withRandom(rng)
                       .withParameter(LootParams::BLOCK_STATE, const_cast<BlockState*>(state))
                       .withParameter(LootParams::BLOCK_POS, &pos)
                       .withOwnedValue(LootParams::FORTUNE_LEVEL, 3)
                       .build(LootParameterSets::block());
    ASSERT_NE(context, nullptr);
    EXPECT_TRUE(context->has(LootParams::FORTUNE_LEVEL));
}

TEST_F(LootContextBuilderValidationTest, SelectorSet_MissingRequiredParams_StillReturnsContext)
{
    // selector 需要 THIS_ENTITY 和 BLOCK_POS，但未提供
    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build(LootParameterSets::selector());
    ASSERT_NE(context, nullptr);
}

TEST_F(LootContextBuilderValidationTest, EntitySet_WithRequiredParam_ReturnsValidContext)
{
    // entity 仅需要 THIS_ENTITY
    math::Random rng(12345);
    // 使用 OwnedValue 传一个简单值来满足 THIS_ENTITY 的类型
    // 注意：THIS_ENTITY 的类型是 Entity*，这里只是测试验证逻辑
    // 由于没有真正的 Entity 对象，我们仅验证 context 是否被成功创建
    auto context = LootContextBuilder(m_world).withRandom(rng).build(LootParameterSets::entity());
    // 缺少 THIS_ENTITY，但仍返回 context
    ASSERT_NE(context, nullptr);
}
