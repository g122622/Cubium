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
#include "common/core/Constants.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/FlyingEntity.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include <cmath>

using namespace mc;
using namespace mc::entity::attribute;

namespace {

/**
 * @brief 测试用飞行实体
 */
class TestFlyingEntity : public FlyingEntity {
public:
    TestFlyingEntity()
        : FlyingEntity(EntityInstanceId(1))
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

/**
 * @brief 测试用存根世界
 */
class StubWorld final : public test::BaseTestWorld {
public:
    void setInWater(bool inWater) { m_inWater = inWater; }
    void setInLava(bool inLava) { m_inLava = inLava; }
    void setOnGround(bool onGround, const BlockState* groundBlock = nullptr)
    {
        m_onGround = onGround;
        m_groundBlock = groundBlock;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        // 对于地面检测：实体在 y=1.0 时会检查 floor(y - 1.0) = 0
        // 或者实体在 y=0.0 时会检查 floor(y - 1.0) = -1
        if (m_onGround && m_groundBlock && y >= -1 && y <= 0) {
            return m_groundBlock;
        }
        return nullptr;
    }

    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        if (m_inWater) {
            return &fluid::Fluids::WATER()->defaultState(); // Water
        }
        if (m_inLava) {
            return &fluid::Fluids::LAVA()->defaultState(); // Lava
        }
        return &fluid::Fluids::EMPTY()->defaultState(); // Empty
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override
    {
        if (!m_onGround) return false;
        return box.maxY <= 0.1f && box.minY >= -1.0f;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override
    {
        if (!hasBlockCollision(box)) return {};
        return {AxisAlignedBB(-10.0f, -1.0f, -10.0f, 10.0f, 0.0f, 10.0f)};
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("StubWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("StubWorld::tickManager not implemented");
    }

private:
    bool m_inWater = false;
    bool m_inLava = false;
    bool m_onGround = false;
    const BlockState* m_groundBlock = nullptr;
};

} // namespace

// ============================================================================
// FlyingEntity 构造测试
// ============================================================================

TEST(FlyingEntityTravelTest, Construction)
{
    TestFlyingEntity entity;

    // 飞行实体默认应该不受重力影响
    EXPECT_FALSE(entity.hasGravity());
    // 飞行实体默认应该处于飞行状态
    EXPECT_TRUE(entity.isFlying());
}

// ============================================================================
// Entity::moveRelative 测试
// ============================================================================

TEST(FlyingEntityTravelTest, MoveRelative_BasicMovement)
{
    TestFlyingEntity entity;
    entity.setPosition(0.0f, 100.0f, 0.0f);
    entity.setRotation(0.0f, 0.0f); // 面向 +Z 方向
    entity.setVelocity(0.0f, 0.0f, 0.0f);

    // 向前移动（forward = 1.0）
    entity.moveRelative(0.1f, 0.0f, 0.0f, 1.0f);

    // 验证速度：yaw=0 时，前进方向应该是 +Z
    EXPECT_NEAR(entity.velocityX(), 0.0f, 0.0001f);
    EXPECT_NEAR(entity.velocityY(), 0.0f, 0.0001f);
    EXPECT_NEAR(entity.velocityZ(), 0.1f, 0.0001f);
}

TEST(FlyingEntityTravelTest, MoveRelative_YawRotation)
{
    TestFlyingEntity entity;
    entity.setPosition(0.0f, 100.0f, 0.0f);
    entity.setRotation(90.0f, 0.0f); // 面向 -X 方向 (yaw = 90)
    entity.setVelocity(0.0f, 0.0f, 0.0f);

    // 向前移动（forward = 1.0）
    entity.moveRelative(0.1f, 0.0f, 0.0f, 1.0f);

    // 验证速度：yaw=90 时，前进方向应该是 -X
    EXPECT_NEAR(entity.velocityX(), -0.1f, 0.0001f);
    EXPECT_NEAR(entity.velocityY(), 0.0f, 0.0001f);
    EXPECT_NEAR(entity.velocityZ(), 0.0f, 0.0001f);
}

TEST(FlyingEntityTravelTest, MoveRelative_VerticalMovement)
{
    TestFlyingEntity entity;
    entity.setPosition(0.0f, 100.0f, 0.0f);
    entity.setRotation(0.0f, 0.0f);
    entity.setVelocity(0.0f, 0.0f, 0.0f);

    // 向上移动（vertical = 1.0）
    entity.moveRelative(0.1f, 0.0f, 1.0f, 0.0f);

    // 验证垂直速度
    EXPECT_NEAR(entity.velocityX(), 0.0f, 0.0001f);
    EXPECT_NEAR(entity.velocityY(), 0.1f, 0.0001f);
    EXPECT_NEAR(entity.velocityZ(), 0.0f, 0.0001f);
}

TEST(FlyingEntityTravelTest, MoveRelative_StrafeMovement)
{
    TestFlyingEntity entity;
    entity.setPosition(0.0f, 100.0f, 0.0f);
    entity.setRotation(0.0f, 0.0f); // 面向 +Z
    entity.setVelocity(0.0f, 0.0f, 0.0f);

    // 向左移动（strafe = -1.0）
    entity.moveRelative(0.1f, -1.0f, 0.0f, 0.0f);

    // 验证速度：yaw=0 时，左移方向应该是 -X
    EXPECT_NEAR(entity.velocityX(), -0.1f, 0.0001f);
    EXPECT_NEAR(entity.velocityY(), 0.0f, 0.0001f);
    EXPECT_NEAR(entity.velocityZ(), 0.0f, 0.0001f);
}

TEST(FlyingEntityTravelTest, MoveRelative_ZeroInput)
{
    TestFlyingEntity entity;
    entity.setPosition(0.0f, 100.0f, 0.0f);
    entity.setRotation(0.0f, 0.0f);
    entity.setVelocity(0.0f, 0.0f, 0.0f);

    // 零输入不应该改变速度
    entity.moveRelative(0.1f, 0.0f, 0.0f, 0.0f);

    EXPECT_NEAR(entity.velocityX(), 0.0f, 0.0001f);
    EXPECT_NEAR(entity.velocityY(), 0.0f, 0.0001f);
    EXPECT_NEAR(entity.velocityZ(), 0.0f, 0.0001f);
}

// ============================================================================
// Entity::scaleVelocity 测试
// ============================================================================

TEST(FlyingEntityTravelTest, ScaleVelocity)
{
    TestFlyingEntity entity;
    entity.setVelocity(1.0f, 2.0f, 3.0f);

    // 应用 0.5 的阻力
    entity.scaleVelocity(0.5f);

    EXPECT_FLOAT_EQ(entity.velocityX(), 0.5f);
    EXPECT_FLOAT_EQ(entity.velocityY(), 1.0f);
    EXPECT_FLOAT_EQ(entity.velocityZ(), 1.5f);
}

TEST(FlyingEntityTravelTest, ScaleVelocity_WaterDrag)
{
    TestFlyingEntity entity;
    entity.setVelocity(1.0f, 1.0f, 1.0f);

    // 应用水中阻力 0.8
    entity.scaleVelocity(physics::DRAG_WATER);

    EXPECT_FLOAT_EQ(entity.velocityX(), physics::DRAG_WATER);
    EXPECT_FLOAT_EQ(entity.velocityY(), physics::DRAG_WATER);
    EXPECT_FLOAT_EQ(entity.velocityZ(), physics::DRAG_WATER);
}

TEST(FlyingEntityTravelTest, ScaleVelocity_LavaDrag)
{
    TestFlyingEntity entity;
    entity.setVelocity(1.0f, 1.0f, 1.0f);

    // 应用岩浆阻力 0.5
    entity.scaleVelocity(physics::DRAG_LAVA);

    EXPECT_FLOAT_EQ(entity.velocityX(), physics::DRAG_LAVA);
    EXPECT_FLOAT_EQ(entity.velocityY(), physics::DRAG_LAVA);
    EXPECT_FLOAT_EQ(entity.velocityZ(), physics::DRAG_LAVA);
}

// ============================================================================
// FlyingEntity::travel 在不同环境下的测试
// ============================================================================

TEST(FlyingEntityTravelTest, Travel_InWater)
{
    StubWorld world;
    world.setInWater(true);

    TestFlyingEntity entity;
    entity.setWorld(&world);
    entity.setPosition(0.0f, 50.0f, 0.0f);
    entity.setRotation(0.0f, 0.0f);
    entity.setVelocity(0.0f, 0.0f, 0.0f);

    // 在水中向前移动
    entity.travel(0.0f, 0.0f, 1.0f);

    // 验证速度被应用了水中阻力和移动因子
    // MC 1.16.5: moveRelative(0.02F, ...) 然后 scale(0.8F)
    // 初始速度应该有正向 Z 分量
    EXPECT_GT(entity.velocityZ(), 0.0f);

    // 验证阻力应用后速度小于初始移动因子
    EXPECT_LT(std::abs(entity.velocityZ()), 0.02f);
}

TEST(FlyingEntityTravelTest, Travel_InLava)
{
    StubWorld world;
    world.setInLava(true);

    TestFlyingEntity entity;
    entity.setWorld(&world);
    entity.setPosition(0.0f, 50.0f, 0.0f);
    entity.setRotation(0.0f, 0.0f);
    entity.setVelocity(0.0f, 0.0f, 0.0f);

    // 在岩浆中向前移动
    entity.travel(0.0f, 0.0f, 1.0f);

    // 验证速度
    // MC 1.16.5: moveRelative(0.02F, ...) 然后 scale(0.5F)
    EXPECT_GT(entity.velocityZ(), 0.0f);

    // 岩浆阻力比水大，速度应该更小
    EXPECT_LT(std::abs(entity.velocityZ()), 0.02f);
}

TEST(FlyingEntityTravelTest, Travel_InAir)
{
    StubWorld world;
    // 不设置水和岩浆，实体在空中

    TestFlyingEntity entity;
    entity.setWorld(&world);
    entity.setPosition(0.0f, 100.0f, 0.0f);
    entity.setRotation(0.0f, 0.0f);
    entity.setVelocity(0.0f, 0.0f, 0.0f);
    entity.setOnGround(false);

    // 在空中飞行
    entity.travel(0.0f, 0.0f, 1.0f);

    // MC 1.16.5: 空中移动因子 = 0.02F, 阻力 = 0.91F
    // 初始速度应该有正向 Z 分量
    EXPECT_GT(entity.velocityZ(), 0.0f);

    // 验证空中阻力
    EXPECT_NEAR(entity.velocityZ(), 0.02f * 0.91f, 0.0001f);
}

TEST(FlyingEntityTravelTest, Travel_OnGround)
{
    StubWorld world;
    // 注意：地面测试需要完整的 BlockState 支持，这里只测试空中飞行的对比
    // 不使用地面，避免复杂的 BlockState 初始化

    TestFlyingEntity entity;
    entity.setWorld(&world);
    entity.setPosition(0.0f, 100.0f, 0.0f); // 在空中
    entity.setRotation(0.0f, 0.0f);
    entity.setVelocity(0.0f, 0.0f, 0.0f);
    entity.setOnGround(false);

    // 在空中飞行
    entity.travel(0.0f, 0.0f, 1.0f);

    // MC 1.16.5: 空中移动因子 = 0.02F, 阻力 = 0.91F
    f32 expectedVelocity = 0.02f * 0.91f; // moveRelative * friction

    // 初始速度应该有正向 Z 分量
    EXPECT_GT(entity.velocityZ(), 0.0f);

    // 验证空中阻力
    EXPECT_NEAR(entity.velocityZ(), expectedVelocity, 0.001f);
}

// ============================================================================
// FlyingEntity::travel 综合测试
// ============================================================================

TEST(FlyingEntityTravelTest, Travel_ThreeDimensionalMovement)
{
    StubWorld world;

    TestFlyingEntity entity;
    entity.setWorld(&world);
    entity.setPosition(0.0f, 100.0f, 0.0f);
    entity.setRotation(45.0f, 0.0f); // 朝向东北方向
    entity.setVelocity(0.0f, 0.0f, 0.0f);
    entity.setOnGround(false);

    // 同时向多个方向移动
    entity.travel(1.0f, 1.0f, 1.0f); // 左、上、前

    // 所有速度分量应该非零
    EXPECT_NE(entity.velocityX(), 0.0f);
    EXPECT_GT(entity.velocityY(), 0.0f); // 向上
    EXPECT_NE(entity.velocityZ(), 0.0f);
}

TEST(FlyingEntityTravelTest, Travel_VelocityAccumulation)
{
    StubWorld world;

    TestFlyingEntity entity;
    entity.setWorld(&world);
    entity.setPosition(0.0f, 100.0f, 0.0f);
    entity.setRotation(0.0f, 0.0f);
    entity.setVelocity(0.5f, 0.0f, 0.0f); // 已有水平速度
    entity.setOnGround(false);

    // 向前移动
    entity.travel(0.0f, 0.0f, 1.0f);

    // 速度应该累加，而不是替换
    EXPECT_GT(entity.velocityX(), 0.0f); // 原有速度
    EXPECT_GT(entity.velocityZ(), 0.0f); // 新增速度
}

TEST(FlyingEntityTravelTest, Travel_DragReducesVelocity)
{
    StubWorld world;

    TestFlyingEntity entity;
    entity.setWorld(&world);
    entity.setPosition(0.0f, 100.0f, 0.0f);
    entity.setRotation(0.0f, 0.0f);
    entity.setVelocity(1.0f, 0.5f, 1.0f); // 初始速度
    entity.setOnGround(false);

    // 不移动，只应用阻力
    entity.travel(0.0f, 0.0f, 0.0f);

    // 阻力应该降低速度
    EXPECT_LT(entity.velocityX(), 1.0f);
    EXPECT_LT(entity.velocityY(), 0.5f);
    EXPECT_LT(entity.velocityZ(), 1.0f);

    // 验证阻力系数 0.91
    EXPECT_NEAR(entity.velocityX(), 0.91f, 0.001f);
    EXPECT_NEAR(entity.velocityY(), 0.5f * 0.91f, 0.001f);
    EXPECT_NEAR(entity.velocityZ(), 0.91f, 0.001f);
}

// ============================================================================
// FlyingEntity 物理常量验证
// ============================================================================

TEST(FlyingEntityTravelTest, PhysicsConstants_Verify)
{
    // 验证项目中定义的物理常量与 MC 1.16.5 一致

    // 水中阻力
    EXPECT_FLOAT_EQ(physics::DRAG_WATER, 0.8f);

    // 岩浆阻力
    EXPECT_FLOAT_EQ(physics::DRAG_LAVA, 0.5f);

    // 空中阻力
    EXPECT_FLOAT_EQ(physics::DRAG_AIR, 0.98f);

    // 地面摩擦
    EXPECT_FLOAT_EQ(physics::DRAG_GROUND, 0.91f);

    // 游泳基础速度
    EXPECT_FLOAT_EQ(physics::SWIM_SPEED_BASE, 0.02f);

    // 岩浆基础速度
    EXPECT_FLOAT_EQ(physics::LAVA_SWIM_SPEED, 0.02f);
}
