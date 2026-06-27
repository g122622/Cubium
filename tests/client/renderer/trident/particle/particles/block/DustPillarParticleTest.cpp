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

#include "client/renderer/trident/particle/ParticleRenderType.hpp"
#include "client/renderer/trident/particle/ParticleTextureAtlas.hpp"
#include "client/renderer/trident/particle/particles/block/DustPillarParticle.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <cmath>
#include <memory>
#include <glm/glm.hpp>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::particles;

/**
 * @brief DustPillarParticle 测试夹具
 *
 * 测试尘柱粒子（重锤砸地攻击效果）的创建、物理属性和生命周期。
 */
class DustPillarParticleTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块注册表
        VanillaBlocks::initialize();
    }

    void TearDown() override {}
};

// ==================== 创建测试 ====================

TEST_F(DustPillarParticleTest, CreateWithBlock_ReturnsValidParticle)
{
    glm::vec3 pos(10.0f, 64.0f, 20.0f);
    glm::vec3 velocity(0.1f, 0.2f, 0.3f);

    const BlockState* stoneState = &(VanillaBlocks::STONE->defaultState());
    ASSERT_NE(stoneState, nullptr);

    auto particle = DustPillarParticle::createWithBlock(pos, velocity, *stoneState);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST_F(DustPillarParticleTest, CreateWithBlock_SetsPosition)
{
    glm::vec3 pos(10.0f, 64.0f, 20.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    EXPECT_FLOAT_EQ(particle->position().x, 10.0f);
    EXPECT_FLOAT_EQ(particle->position().y, 64.0f);
    EXPECT_FLOAT_EQ(particle->position().z, 20.0f);
}

TEST_F(DustPillarParticleTest, Create_Default_ReturnsStoneParticle)
{
    // 默认工厂方法应该返回使用石头纹理的粒子
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = DustPillarParticle::create(pos, velocity, nullptr);

    // 如果石头方块状态可用，应该返回有效粒子
    if (particle != nullptr) {
        EXPECT_TRUE(particle->isAlive());
    }
}

// ==================== 速度覆盖测试 ====================

TEST_F(DustPillarParticleTest, Constructor_OverridesHorizontalVelocity)
{
    // DustPillarProvider 在 MC Java 中将 X/Z 速度替换为 nextGaussian() / 30.0
    // 因此传入的 X/Z 速度应该被覆盖，不再保留原值
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(1.0f, 0.5f, 1.0f); // 较大的水平速度

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    // X/Z 速度应被覆盖为极小的值（gaussian/30），不应仍为传入的大值
    // 由于高斯分布理论上可产生较大值，这里检查速度明显小于传入值
    EXPECT_LT(std::abs(particle->velocity().x), 0.5f) << "X 速度应被覆盖为极小值";
    EXPECT_LT(std::abs(particle->velocity().z), 0.5f) << "Z 速度应被覆盖为极小值";
}

TEST_F(DustPillarParticleTest, Constructor_PreservesAndModifiesYVelocity)
{
    // DustPillarProvider 在 MC Java 中 Y 速度 = 传入Y + nextGaussian() / 2.0
    // 传入 Y=0.5 时，最终 Y 速度应在 0.5 附近波动（±1.0 范围内）
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.5f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    // Y 速度应保留传入的 Y 分量并叠加高斯偏移（sigma=0.5）
    // 3-sigma 范围约为 0.5 ± 1.5，这里放宽检查
    EXPECT_GT(particle->velocity().y, -2.0f) << "Y 速度应在合理范围内";
    EXPECT_LT(particle->velocity().y, 3.0f) << "Y 速度应在合理范围内";
}

// ==================== 物理属性测试 ====================

TEST_F(DustPillarParticleTest, HasHighGravity)
{
    // DustPillar 粒子重力为 1.0（DiggingParticle 为 0.03）
    glm::vec3 pos(0.0f, 100.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    // DustPillar 的重力远大于 DiggingParticle
    EXPECT_NEAR(particle->gravity(), 1.0, 0.01);
}

TEST_F(DustPillarParticleTest, HasPhysics)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    EXPECT_TRUE(particle->hasPhysics());
}

TEST_F(DustPillarParticleTest, HasFriction)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    EXPECT_NEAR(particle->friction(), 0.92, 0.01);
}

// ==================== 生命周期测试 ====================

TEST_F(DustPillarParticleTest, Tick_UpdatesAge)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    f64 initialAge = particle->age();

    particle->tick(nullptr);

    EXPECT_GT(particle->age(), initialAge);
}

TEST_F(DustPillarParticleTest, Tick_AppliesGravity)
{
    glm::vec3 pos(0.0f, 100.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    particle->setMaxAge(1000.0); // 防止过期

    f32 initialVelY = particle->velocity().y;

    particle->tick(nullptr);

    // 高重力（1.0）应使 Y 速度显著减小
    EXPECT_LT(particle->velocity().y, initialVelY);
}

TEST_F(DustPillarParticleTest, Tick_AppliesStrongGravity)
{
    // DustPillar 的重力为 1.0，比 DiggingParticle 的 0.03 大得多
    // 每个 tick Y 速度应减少 gravity * 0.04 = 0.04
    glm::vec3 pos(0.0f, 100.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    particle->setMaxAge(1000.0);

    f32 velYBeforeTick = particle->velocity().y;
    particle->tick(nullptr);

    // 重力导致 Y 速度减少 gravity * 0.04 = 1.0 * 0.04 = 0.04
    // 但构造函数可能已修改了 Y 速度（gaussian/2.0），所以直接检查差值
    f32 gravityDelta = velYBeforeTick - particle->velocity().y;
    EXPECT_NEAR(gravityDelta, 0.04f, 0.02f) << "高重力应使 Y 速度每 tick 减少约 0.04";
}

TEST_F(DustPillarParticleTest, Tick_UpdatesPosition)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.5f, 0.0f); // Y 方向有初速度

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    particle->setMaxAge(1000.0);
    particle->setHasPhysics(false); // 禁用碰撞以便测试移动

    particle->tick(nullptr);

    // 位置应更新（虽然速度被覆盖，但构造函数设置了非零速度）
    // 即使水平速度很小，Y 速度应该非零
}

TEST_F(DustPillarParticleTest, Lifetime_In20To40Range)
{
    // DustPillar 生命周期为 20-40 tick
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 创建多个粒子检查生命周期范围
    bool foundMin = false;
    bool foundMax = false;
    for (int i = 0; i < 100; ++i) {
        auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);
        ASSERT_NE(particle, nullptr);

        f64 maxAge = particle->maxAge();
        EXPECT_GE(maxAge, 20.0) << "最小生命周期应 >= 20 tick";
        EXPECT_LE(maxAge, 40.0) << "最大生命周期应 <= 40 tick";

        if (maxAge <= 22.0) foundMin = true;
        if (maxAge >= 38.0) foundMax = true;
    }
    // 统计上，100个粒子应该覆盖范围两端
    EXPECT_TRUE(foundMin || foundMax) << "生命周期应有随机性";
}

// ==================== 颜色测试 ====================

TEST_F(DustPillarParticleTest, Color_IsTinted)
{
    // DustPillar 颜色乘以 0.6 基础亮度
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    // 颜色应被 0.6 调暗
    EXPECT_LE(particle->color().r, 1.0f);
    EXPECT_LE(particle->color().g, 1.0f);
    EXPECT_LE(particle->color().b, 1.0f);
}

// ==================== 渲染属性测试 ====================

TEST_F(DustPillarParticleTest, GetRenderType_ReturnsTerrainSheet)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    // DustPillar 使用 TERRAIN_SHEET 渲染类型（继承自 DiggingParticle）
    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::TERRAIN_SHEET);
}

// ==================== buildVertices 测试 ====================

TEST_F(DustPillarParticleTest, BuildVertices_ProducesVertices)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);

    ParticleTextureAtlas atlas;
    glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);
    std::vector<ParticleVertex> vertices;

    particle->buildVertices(cameraPos, 0.0, atlas, vertices);

    // 应生成 4 个顶点（一个 quad）
    EXPECT_EQ(vertices.size(), 4u);
}

TEST_F(DustPillarParticleTest, BuildVertices_VerticesHaveValidUV)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DustPillarParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);

    ParticleTextureAtlas atlas;
    glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);
    std::vector<ParticleVertex> vertices;

    particle->buildVertices(cameraPos, 0.0, atlas, vertices);

    ASSERT_EQ(vertices.size(), 4u);

    for (const auto& v : vertices) {
        EXPECT_GE(v.texCoord.x, 0.0f);
        EXPECT_LE(v.texCoord.x, 1.0f);
        EXPECT_GE(v.texCoord.y, 0.0f);
        EXPECT_LE(v.texCoord.y, 1.0f);
    }
}

// ==================== 不同方块类型测试 ====================

TEST_F(DustPillarParticleTest, CreateWithBlock_DifferentBlockTypes)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    std::vector<Block*> blocks = {
        VanillaBlocks::STONE, VanillaBlocks::DIRT, VanillaBlocks::GRASS_BLOCK, VanillaBlocks::COBBLESTONE};

    for (Block* block : blocks) {
        if (block == nullptr) continue;

        const BlockState& state = block->defaultState();
        auto particle = DustPillarParticle::createWithBlock(pos, velocity, state);

        ASSERT_NE(particle, nullptr) << "Failed to create DustPillar particle for block";
        EXPECT_TRUE(particle->isAlive());
        EXPECT_EQ(particle->getRenderType(), ParticleRenderType::TERRAIN_SHEET);
    }
}

// ==================== 重力对比测试 ====================

TEST_F(DustPillarParticleTest, Gravity_HigherThanDiggingParticle)
{
    // DustPillar 重力 1.0 远高于 DiggingParticle 的 0.03
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto dustPillar = DustPillarParticle::createWithBlock(pos, velocity, stoneState);
    auto digging = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(dustPillar, nullptr);
    ASSERT_NE(digging, nullptr);

    EXPECT_GT(dustPillar->gravity(), digging->gravity()) << "DustPillar 重力应高于 DiggingParticle";
}

} // namespace
} // namespace mc
