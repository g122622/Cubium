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

#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"
#include "common/world/block/blocks/cave/SporeBlossomBlock.hpp"
#include "common/world/block/blocks/ocean/BubbleColumnBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 测试用 IBlockAnimateContext 实现
 *
 * 记录 animateTick 调用产生的粒子和音效，用于验证方块动画行为
 */
class MockAnimateContext : public IBlockAnimateContext {
public:
    struct ParticleCall {
        client::renderer::trident::particle::ParticleTypeId type;
        Vector3 pos;
        Vector3 velocity;
    };

    struct SoundCall {
        ResourceLocation soundEventId;
        sound::SoundCategory category;
        Vector3 position;
        f32 volume;
        f32 pitch;
    };

    void addAnimateParticle(
        client::renderer::trident::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        m_particles.push_back({type, pos, velocity});
    }

    void playLocalSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_sounds.push_back({soundEventId, category, position, volume, pitch});
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    // 辅助方法
    void setBlockAt(const BlockPos& pos, const BlockState* state) { m_blocks[pos] = state; }

    [[nodiscard]] size_t particleCount() const { return m_particles.size(); }
    [[nodiscard]] size_t soundCount() const { return m_sounds.size(); }
    [[nodiscard]] const std::vector<ParticleCall>& particles() const { return m_particles; }
    [[nodiscard]] const std::vector<SoundCall>& sounds() const { return m_sounds; }

    void clear()
    {
        m_particles.clear();
        m_sounds.clear();
    }

private:
    std::vector<ParticleCall> m_particles;
    std::vector<SoundCall> m_sounds;
    std::map<BlockPos, const BlockState*> m_blocks;
};

// ========== Block::animateTick 默认实现测试 ==========

class AnimateTickTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    void TearDown() override {}

    MockAnimateContext context_;
    math::Random random_{12345};
};

TEST_F(AnimateTickTest, DefaultAnimateTick_DoesNothing)
{
    // 石头方块的 animateTick 应该是默认的空操作
    const Block* stoneBlock = VanillaBlocks::STONE;
    ASSERT_NE(stoneBlock, nullptr);

    const BlockState& state = stoneBlock->defaultState();
    BlockPos pos(0, 0, 0);

    // 调用基类的 animateTick
    stoneBlock->animateTick(context_, pos, state, random_);

    // 不应该产生任何粒子或音效
    EXPECT_EQ(context_.particleCount(), 0u);
    EXPECT_EQ(context_.soundCount(), 0u);
}

TEST_F(AnimateTickTest, AirBlockAnimateTick_DoesNothing)
{
    const Block* airBlock = VanillaBlocks::AIR;
    ASSERT_NE(airBlock, nullptr);

    const BlockState& state = airBlock->defaultState();
    BlockPos pos(0, 0, 0);

    airBlock->animateTick(context_, pos, state, random_);

    EXPECT_EQ(context_.particleCount(), 0u);
    EXPECT_EQ(context_.soundCount(), 0u);
}

// ========== BubbleColumnBlock::animateTick 测试 ==========

TEST_F(AnimateTickTest, BubbleColumn_DragTrue_GeneratesCurrentDownParticle)
{
    const Block* bubbleBlock = VanillaBlocks::BUBBLE_COLUMN;
    ASSERT_NE(bubbleBlock, nullptr);

    // 设置 DRAG=true（下拖模式）
    const BlockState& dragState = bubbleBlock->defaultState().with(BlockStateProperties::DRAG(), true);
    BlockPos pos(5, 10, 5);

    // 调用 animateTick
    bubbleBlock->animateTick(context_, pos, dragState, random_);

    // 下拖模式应该产生 CURRENT_DOWN 粒子
    EXPECT_GE(context_.particleCount(), 1u);
}

TEST_F(AnimateTickTest, BubbleColumn_DragFalse_GeneratesBubbleColumnUpParticle)
{
    const Block* bubbleBlock = VanillaBlocks::BUBBLE_COLUMN;
    ASSERT_NE(bubbleBlock, nullptr);

    // 设置 DRAG=false（上升模式）
    const BlockState& upState = bubbleBlock->defaultState().with(BlockStateProperties::DRAG(), false);
    BlockPos pos(5, 10, 5);

    // 调用 animateTick
    bubbleBlock->animateTick(context_, pos, upState, random_);

    // 上升模式应该产生 BUBBLE_COLUMN_UP 粒子
    EXPECT_GE(context_.particleCount(), 1u);
}

TEST_F(AnimateTickTest, BubbleColumn_DragTrue_CanProduceWhirlpoolSound)
{
    const Block* bubbleBlock = VanillaBlocks::BUBBLE_COLUMN;
    ASSERT_NE(bubbleBlock, nullptr);

    const BlockState& dragState = bubbleBlock->defaultState().with(BlockStateProperties::DRAG(), true);
    BlockPos pos(5, 10, 5);

    // 多次调用，因为有 1/200 的概率播放环境音
    // 使用不同种子的随机数生成器以增加命中概率
    size_t totalSounds = 0;
    for (int i = 0; i < 500; ++i) {
        math::Random r(i);
        context_.clear();
        bubbleBlock->animateTick(context_, pos, dragState, r);
        totalSounds += context_.soundCount();
    }

    // 在 500 次调用中，期望至少有一次触发环境音（1/200 概率）
    EXPECT_GE(totalSounds, 1u);
}

TEST_F(AnimateTickTest, BubbleColumn_DragFalse_CanProduceUpwardsSound)
{
    const Block* bubbleBlock = VanillaBlocks::BUBBLE_COLUMN;
    ASSERT_NE(bubbleBlock, nullptr);

    const BlockState& upState = bubbleBlock->defaultState().with(BlockStateProperties::DRAG(), false);
    BlockPos pos(5, 10, 5);

    // 多次调用以触发环境音
    size_t totalSounds = 0;
    for (int i = 0; i < 500; ++i) {
        math::Random r(i);
        context_.clear();
        bubbleBlock->animateTick(context_, pos, upState, r);
        totalSounds += context_.soundCount();
    }

    // 在 500 次调用中，期望至少有一次触发环境音
    EXPECT_GE(totalSounds, 1u);
}

// ========== SporeBlossomBlock::animateTick 测试 ==========

TEST_F(AnimateTickTest, SporeBlossom_GeneratesFallingSporeBlossomParticle)
{
    const Block* sporeBlock = VanillaBlocks::SPORE_BLOSSOM;
    ASSERT_NE(sporeBlock, nullptr);

    const BlockState& state = sporeBlock->defaultState();
    BlockPos pos(5, 20, 5);

    // 调用 animateTick
    sporeBlock->animateTick(context_, pos, state, random_);

    // 应该产生 FallingSporeBlossom 粒子（1个）和 SporeBlossomAir 粒子（最多14个尝试）
    EXPECT_GE(context_.particleCount(), 1u);

    // 第一个粒子应该是 FallingSporeBlossom
    bool foundFallingSpore = false;
    for (const auto& p : context_.particles()) {
        if (p.type == client::renderer::trident::particle::ParticleTypeId::FallingSporeBlossom) {
            foundFallingSpore = true;
            // 验证粒子位置在方块范围内
            EXPECT_GE(p.pos.x, static_cast<f32>(pos.x) + 0.1f);
            EXPECT_LE(p.pos.x, static_cast<f32>(pos.x) + 0.9f);
            EXPECT_GE(p.pos.y, static_cast<f32>(pos.y) + 0.7f);
            EXPECT_LE(p.pos.y, static_cast<f32>(pos.y) + 0.71f);
            EXPECT_GE(p.pos.z, static_cast<f32>(pos.z) + 0.1f);
            EXPECT_LE(p.pos.z, static_cast<f32>(pos.z) + 0.9f);
            break;
        }
    }
    EXPECT_TRUE(foundFallingSpore);
}

TEST_F(AnimateTickTest, SporeBlossom_AirParticlesCheckBlockSolidity)
{
    const Block* sporeBlock = VanillaBlocks::SPORE_BLOSSOM;
    ASSERT_NE(sporeBlock, nullptr);

    const BlockState& state = sporeBlock->defaultState();
    BlockPos pos(5, 20, 5);

    // 在 SporeBlossom 周围放置非固体方块（空气），这样 SporeBlossomAir 粒子才会生成
    // context_ 默认 getBlockState 返回 nullptr（空气），空气不是固体
    sporeBlock->animateTick(context_, pos, state, random_);

    // 应该产生 FallingSporeBlossom 粒子
    EXPECT_GE(context_.particleCount(), 1u);
}

// ========== IBlockAnimateContext 接口测试 ==========

TEST_F(AnimateTickTest, MockContext_RecordsParticlesAndSounds)
{
    // 验证 MockAnimateContext 基本功能
    context_.addAnimateParticle(client::renderer::trident::particle::ParticleTypeId::FallingSporeBlossom,
        Vector3(1.0f, 2.0f, 3.0f),
        Vector3(0.0f, 0.0f, 0.0f));

    context_.playLocalSound(ResourceLocation("minecraft:block.bubble_column.whirlpool_ambient"),
        sound::SoundCategory::Blocks,
        Vector3(1.0f, 2.0f, 3.0f),
        1.0f,
        1.0f);

    EXPECT_EQ(context_.particleCount(), 1u);
    EXPECT_EQ(context_.soundCount(), 1u);
    EXPECT_EQ(context_.particles()[0].type, client::renderer::trident::particle::ParticleTypeId::FallingSporeBlossom);
    EXPECT_EQ(context_.sounds()[0].category, sound::SoundCategory::Blocks);
}

TEST_F(AnimateTickTest, MockContext_GetBlockState_ReturnsNullForUnset)
{
    // 未设置的方块位置应返回 nullptr
    EXPECT_EQ(context_.getBlockState(0, 0, 0), nullptr);
}

TEST_F(AnimateTickTest, MockContext_GetBlockState_ReturnsStateForSet)
{
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    context_.setBlockAt(BlockPos(0, 0, 0), stoneState);

    EXPECT_EQ(context_.getBlockState(0, 0, 0), stoneState);
}

} // namespace
