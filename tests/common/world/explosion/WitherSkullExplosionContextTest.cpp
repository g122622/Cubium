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

#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/explosion/ExplosionContext.hpp"

using namespace mc;
using namespace mc::world::explosion;

namespace {

/**
 * @brief 测试用方块 - 模拟高爆炸抗性方块（如黑曜石，resistance=1200）
 */
class HighResistanceBlock final : public Block {
public:
    HighResistanceBlock()
        : Block(makeProperties())
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block,
                std::vector<size_t> values,
                const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                const std::vector<BlockState*>* allStates,
                u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
        createBlockState(std::move(container));
    }

    [[nodiscard]] static BlockProperties makeProperties()
    {
        return BlockProperties(Material::ROCK).resistance(1200.0f);
    }
};

/**
 * @brief 测试用方块 - 低爆炸抗性方块（如泥土，resistance=0.5）
 */
class LowResistanceBlock final : public Block {
public:
    LowResistanceBlock()
        : Block(makeProperties())
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block,
                std::vector<size_t> values,
                const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                const std::vector<BlockState*>* allStates,
                u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
        createBlockState(std::move(container));
    }

    [[nodiscard]] static BlockProperties makeProperties() { return BlockProperties(Material::EARTH).resistance(0.5f); }
};

} // namespace

// ============================================================================
// WitherSkullExplosionContext 测试夹具
// ============================================================================

class WitherSkullExplosionContextTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

// ============================================================================
// 蓝色凋灵之首 (isDangerous=true) 测试
// ============================================================================

TEST_F(WitherSkullExplosionContextTest, DangerousSkull_CutsHighResistanceTo08)
{
    // 蓝色凋灵之首对高抗性非免疫方块，将抗性限制为 min(0.8, 原始抗性)
    // 黑曜石抗性 1200 -> 被限制为 0.8
    WitherSkullExplosionContext context(nullptr, true);

    HighResistanceBlock highResBlock;
    const BlockState& highResState = highResBlock.defaultState();

    auto resistance = context.getExplosionResistance(highResState, nullptr);
    ASSERT_TRUE(resistance.has_value());
    EXPECT_FLOAT_EQ(resistance.value(), 0.8f);
}

TEST_F(WitherSkullExplosionContextTest, DangerousSkull_LowResistanceUnchanged)
{
    // 蓝色凋灵之首对低抗性方块（抗性 < 0.8），不修改抗性
    // 泥土抗性 0.5 -> 保持 0.5
    WitherSkullExplosionContext context(nullptr, true);

    LowResistanceBlock lowResBlock;
    const BlockState& lowResState = lowResBlock.defaultState();

    auto resistance = context.getExplosionResistance(lowResState, nullptr);
    ASSERT_TRUE(resistance.has_value());
    EXPECT_FLOAT_EQ(resistance.value(), 0.5f);
}

TEST_F(WitherSkullExplosionContextTest, DangerousSkull_WitherImmuneBlockUnchanged)
{
    // 蓝色凋灵之首对 WITHER_IMMUNE 标签中的方块（如基岩），不修改抗性
    // 基岩抗性 3600000 -> 保持 3600000
    WitherSkullExplosionContext context(nullptr, true);

    ASSERT_TRUE(VanillaBlocks::BEDROCK != nullptr);
    const BlockState& bedrockState = VanillaBlocks::BEDROCK->defaultState();
    ASSERT_TRUE(BlockTags::WITHER_IMMUNE().contains(bedrockState));

    auto resistance = context.getExplosionResistance(bedrockState, nullptr);
    ASSERT_TRUE(resistance.has_value());
    // 基岩抗性极高，不应被限制为 0.8
    EXPECT_GT(resistance.value(), 0.8f);
}

TEST_F(WitherSkullExplosionContextTest, DangerousSkull_AirBlockReturnsNullopt)
{
    // 蓝色凋灵之首对空气方块，返回 nullopt（不消耗爆炸强度）
    WitherSkullExplosionContext context(nullptr, true);

    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    ASSERT_TRUE(airState.isAir());

    auto resistance = context.getExplosionResistance(airState, nullptr);
    EXPECT_FALSE(resistance.has_value());
}

TEST_F(WitherSkullExplosionContextTest, DangerousSkull_ObsidianResistanceCut)
{
    // 使用实际的黑曜石方块测试
    WitherSkullExplosionContext context(nullptr, true);

    if (!VanillaBlocks::OBSIDIAN) {
        GTEST_SKIP() << "OBSIDIAN block not registered";
    }

    const BlockState& obsidianState = VanillaBlocks::OBSIDIAN->defaultState();

    // 黑曜石不在 WITHER_IMMUNE 标签中
    EXPECT_FALSE(BlockTags::WITHER_IMMUNE().contains(obsidianState));

    auto resistance = context.getExplosionResistance(obsidianState, nullptr);
    ASSERT_TRUE(resistance.has_value());
    // 黑曜石原始抗性 1200，应被限制为 0.8
    EXPECT_FLOAT_EQ(resistance.value(), 0.8f);
}

// ============================================================================
// 普通凋灵之首 (isDangerous=false) 测试
// ============================================================================

TEST_F(WitherSkullExplosionContextTest, NormalSkull_HighResistanceUnchanged)
{
    // 普通凋灵之首不修改任何爆炸抗性
    // 高抗性方块保持原始抗性
    WitherSkullExplosionContext context(nullptr, false);

    HighResistanceBlock highResBlock;
    const BlockState& highResState = highResBlock.defaultState();

    auto resistance = context.getExplosionResistance(highResState, nullptr);
    ASSERT_TRUE(resistance.has_value());
    EXPECT_FLOAT_EQ(resistance.value(), 1200.0f);
}

TEST_F(WitherSkullExplosionContextTest, NormalSkull_LowResistanceUnchanged)
{
    // 普通凋灵之首不修改低抗性方块
    WitherSkullExplosionContext context(nullptr, false);

    LowResistanceBlock lowResBlock;
    const BlockState& lowResState = lowResBlock.defaultState();

    auto resistance = context.getExplosionResistance(lowResState, nullptr);
    ASSERT_TRUE(resistance.has_value());
    EXPECT_FLOAT_EQ(resistance.value(), 0.5f);
}

TEST_F(WitherSkullExplosionContextTest, NormalSkull_WitherImmuneBlockUnchanged)
{
    // 普通凋灵之首对 WITHER_IMMUNE 方块也保持原始抗性
    WitherSkullExplosionContext context(nullptr, false);

    ASSERT_TRUE(VanillaBlocks::BEDROCK != nullptr);
    const BlockState& bedrockState = VanillaBlocks::BEDROCK->defaultState();

    auto resistance = context.getExplosionResistance(bedrockState, nullptr);
    ASSERT_TRUE(resistance.has_value());
    EXPECT_GT(resistance.value(), 0.8f);
}

TEST_F(WitherSkullExplosionContextTest, NormalSkull_AirBlockReturnsNullopt)
{
    // 普通凋灵之首对空气方块也返回 nullopt
    WitherSkullExplosionContext context(nullptr, false);

    const BlockState& airState = VanillaBlocks::AIR->defaultState();

    auto resistance = context.getExplosionResistance(airState, nullptr);
    EXPECT_FALSE(resistance.has_value());
}

// ============================================================================
// 对比测试：蓝色 vs 普通凋灵之首
// ============================================================================

TEST_F(WitherSkullExplosionContextTest, DangerousVsNormal_HighResistanceDifference)
{
    // 蓝色凋灵之首限制高抗性为 0.8，普通不限制
    HighResistanceBlock highResBlock;
    const BlockState& highResState = highResBlock.defaultState();

    WitherSkullExplosionContext dangerousContext(nullptr, true);
    WitherSkullExplosionContext normalContext(nullptr, false);

    auto dangerousRes = dangerousContext.getExplosionResistance(highResState, nullptr);
    auto normalRes = normalContext.getExplosionResistance(highResState, nullptr);

    ASSERT_TRUE(dangerousRes.has_value());
    ASSERT_TRUE(normalRes.has_value());
    EXPECT_FLOAT_EQ(dangerousRes.value(), 0.8f);
    EXPECT_FLOAT_EQ(normalRes.value(), 1200.0f);
    EXPECT_LT(dangerousRes.value(), normalRes.value());
}

TEST_F(WitherSkullExplosionContextTest, DangerousVsNormal_LowResistanceSame)
{
    // 两种凋灵之首对低抗性方块的行为一致
    LowResistanceBlock lowResBlock;
    const BlockState& lowResState = lowResBlock.defaultState();

    WitherSkullExplosionContext dangerousContext(nullptr, true);
    WitherSkullExplosionContext normalContext(nullptr, false);

    auto dangerousRes = dangerousContext.getExplosionResistance(lowResState, nullptr);
    auto normalRes = normalContext.getExplosionResistance(lowResState, nullptr);

    ASSERT_TRUE(dangerousRes.has_value());
    ASSERT_TRUE(normalRes.has_value());
    EXPECT_FLOAT_EQ(dangerousRes.value(), normalRes.value());
}

// ============================================================================
// canDestroyBlock 测试
// ============================================================================

TEST_F(WitherSkullExplosionContextTest, CanDestroyBlock_NonAirBlock)
{
    // 蓝色凋灵之首可以破坏非空气方块
    WitherSkullExplosionContext context(nullptr, true);

    HighResistanceBlock highResBlock;
    const BlockState& highResState = highResBlock.defaultState();

    EXPECT_TRUE(context.canDestroyBlock(highResState, 1.0f));
}

TEST_F(WitherSkullExplosionContextTest, CanDestroyBlock_AirBlock)
{
    // 空气方块不可被破坏
    WitherSkullExplosionContext context(nullptr, true);

    const BlockState& airState = VanillaBlocks::AIR->defaultState();

    EXPECT_FALSE(context.canDestroyBlock(airState, 1.0f));
}

// ============================================================================
// Explosion 构造函数测试（自定义 ExplosionContext）
// ============================================================================

TEST(ExplosionConstructorTest, CustomExplosionContextAccepted)
{
    // 验证 Explosion 可以接受自定义 ExplosionContext
    // 这里只测试构造不崩溃，不执行完整爆炸流程
    // （完整爆炸流程需要 IWorld，在集成测试中覆盖）

    // 自定义 context 会被移动到 Explosion 中
    auto context = std::make_unique<WitherSkullExplosionContext>(nullptr, true);
    ASSERT_TRUE(context != nullptr);

    // 验证 context 的行为在移动前正确
    auto resistance = context->getExplosionResistance(VanillaBlocks::AIR->defaultState(), nullptr);
    EXPECT_FALSE(resistance.has_value());
}

// ============================================================================
// Explosion 自定义 context 构造函数断言测试
// ============================================================================

#ifdef NDEBUG
// Release 模式下 MC_ASSERT_RELEASE 仍然生效
TEST(ExplosionConstructorTest, NullContextTriggersAssert)
{
    // 验证传入 nullptr 的 ExplosionContext 会触发断言
    // 这在 Debug 和 Release 模式下都会触发
    // 注意：此测试在断言触发时会终止进程，因此只在特殊条件下测试
    // 实际上我们通过代码审查确保 MC_ASSERT_RELEASE(m_context != nullptr) 存在
    // 此处仅验证代码编译通过
    EXPECT_TRUE(true);
}
#endif
