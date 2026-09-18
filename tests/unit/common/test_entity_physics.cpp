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

#include <cmath>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "entity/attribute/AttributeMap.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/core/Entity.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "physics/PhysicsConstants.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockPos.hpp"

using namespace mc;

// ============================================================================
// 测试辅助类
// ============================================================================

/**
 * @brief 测试用的简单LivingEntity实现
 */
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity(EntityInstanceId id = 1)
        : LivingEntity(id, nullptr, mc::test::testEcsRegistry())
    {
        // 设置默认属性
        setHealth(20.0f);
    }

    void tick() override { LivingEntity::tick(); }
};

/**
 * @brief 测试用方块
 */
class TestBlock : public Block {
public:
    explicit TestBlock(BlockProperties properties)
        : Block(std::move(properties))
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
};

// ============================================================================
// 重力测试
// ============================================================================

TEST(EntityPhysics, GravityConstant)
{
    // 验证重力常量与 MC 1.16.5 一致
    EXPECT_FLOAT_EQ(physics::GRAVITY, 0.08f);
}

TEST(EntityPhysics, JumpVelocity)
{
    // 验证跳跃初速度与 MC 1.16.5 一致
    EXPECT_FLOAT_EQ(physics::JUMP_VELOCITY, 0.42f);
}

TEST(EntityPhysics, AirDrag)
{
    // 验证空气阻力
    EXPECT_FLOAT_EQ(physics::DRAG_AIR, 0.98f);
}

TEST(EntityPhysics, GroundDrag)
{
    // 验证地面基础阻力系数
    // MC 1.16.5: 地面基础阻力为 0.91，实际阻力 = 滑度 * 0.91
    // 默认滑度 0.6 的方块实际阻力为 0.6 * 0.91 = 0.546
    EXPECT_FLOAT_EQ(physics::DRAG_GROUND, 0.91f);
}

// ============================================================================
// 击退测试
// ============================================================================

TEST(LivingEntityKnockback, BasicKnockback)
{
    TestLivingEntity entity;
    entity.setPosition(0.0f, 0.0f, 0.0f);
    entity.setOnGround(true);

    // 初始速度为0
    auto vel = entity.velocity();
    EXPECT_FLOAT_EQ(vel.x, 0.0f);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
    EXPECT_FLOAT_EQ(vel.z, 0.0f);

    // 应用击退
    entity.applyKnockback(1.0f, 1.0f, 0.0f); // 向-X方向击退

    vel = entity.velocity();
    // 击退后应该有负X方向速度
    EXPECT_LT(vel.x, 0.0f);
    // 在地面上，Y速度应为 min(0.4, 0 + strength)
    EXPECT_GT(vel.y, 0.0f);
    EXPECT_LE(vel.y, 0.4f);
}

TEST(LivingEntityKnockback, KnockbackResistance)
{
    TestLivingEntity entity;
    entity.setPosition(0.0f, 0.0f, 0.0f);
    entity.setOnGround(true);

    // 设置击退抗性为100%
    entity.setAttributeBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 1.0);

    // 应用击退
    entity.applyKnockback(1.0f, 1.0f, 0.0f);

    // 击退抗性100%时，应该不受击退影响
    auto vel = entity.velocity();
    EXPECT_FLOAT_EQ(vel.x, 0.0f);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
    EXPECT_FLOAT_EQ(vel.z, 0.0f);
}

TEST(LivingEntityKnockback, AirKnockback)
{
    TestLivingEntity entity;
    entity.setPosition(0.0f, 100.0f, 0.0f);
    entity.setOnGround(false);

    // 空中击退
    entity.applyKnockback(1.0f, 1.0f, 0.0f);

    auto vel = entity.velocity();
    // 空中击退，Y速度应该保持当前值
    // 由于初始Y速度为0，击退后Y速度应该接近0
    EXPECT_NEAR(vel.y, 0.0f, 0.01f);
}

TEST(LivingEntityKnockback, KnockbackFromEntity)
{
    TestLivingEntity attacker;
    TestLivingEntity victim;

    attacker.setPosition(0.0f, 0.0f, 0.0f);
    victim.setPosition(2.0f, 0.0f, 0.0f);
    victim.setOnGround(true);

    // 从攻击者位置计算击退方向
    victim.applyKnockbackFrom(&attacker, 1.0f);

    auto vel = victim.velocity();
    // 击退方向应该是远离攻击者（+X方向）
    EXPECT_GT(vel.x, 0.0f);
}

// ============================================================================
// 碰撞后速度重置测试
// ============================================================================

TEST(EntityPhysics, VelocityResetAfterCollision)
{
    // 测试碰撞后速度重置
    // 注：完整测试需要物理引擎和碰撞世界
    // 这里只测试常量定义
    EXPECT_FLOAT_EQ(physics::MOTION_THRESHOLD, 0.003f);
}

// ============================================================================
// 方块滑度测试
// ============================================================================

TEST(BlockSlipperiness, DefaultSlipperiness)
{
    TestBlock block{BlockProperties(Material::ROCK)};

    // 默认滑度应为 0.6f
    EXPECT_FLOAT_EQ(block.getSlipperiness(block.defaultState()), 0.6f);
}

TEST(BlockSlipperiness, CustomSlipperiness)
{
    // 创建一个滑度 0.98 的方块（类似冰）
    TestBlock slipperyBlock{BlockProperties(Material::ROCK).slipperiness(0.98f)};

    EXPECT_FLOAT_EQ(slipperyBlock.getSlipperiness(slipperyBlock.defaultState()), 0.98f);
}

TEST(BlockSlipperiness, HoneyBlockSlipperiness)
{
    // 任意自定义滑度(0.5)透传验证（注意：真实蜂蜜块滑度为默认值 0.6，由 speedFactor/jumpFactor 减速）
    TestBlock honeyBlock{BlockProperties(Material::ROCK).slipperiness(0.5f)};

    EXPECT_FLOAT_EQ(honeyBlock.getSlipperiness(honeyBlock.defaultState()), 0.5f);
}

// ============================================================================
// 方块速度因子测试
// ============================================================================

TEST(BlockSpeedFactor, DefaultSpeedFactor)
{
    TestBlock block{BlockProperties(Material::ROCK)};

    // 默认速度因子应为 1.0f
    EXPECT_FLOAT_EQ(block.getSpeedFactor(block.defaultState()), 1.0f);
}

TEST(BlockSpeedFactor, CustomSpeedFactor)
{
    TestBlock slowBlock{BlockProperties(Material::ROCK).speedFactor(0.5f)};

    EXPECT_FLOAT_EQ(slowBlock.getSpeedFactor(slowBlock.defaultState()), 0.5f);
}

// ============================================================================
// 方块跳跃因子测试
// ============================================================================

TEST(BlockJumpFactor, DefaultJumpFactor)
{
    TestBlock block{BlockProperties(Material::ROCK)};

    // 默认跳跃因子应为 1.0f
    EXPECT_FLOAT_EQ(block.getJumpFactor(block.defaultState()), 1.0f);
}

// ============================================================================
// 击退计算公式测试
// ============================================================================

TEST(KnockbackCalculation, GroundKnockbackFormula)
{
    // MC 1.16.5 地面击退公式:
    // Y速度 = min(0.4, 当前Y速度/2 + 击退强度)
    // X/Z速度 = 当前速度/2 - 击退方向 * 击退强度

    TestLivingEntity entity;
    entity.setOnGround(true);
    entity.setVelocity(0.0f, 0.0f, 0.0f);

    entity.applyKnockback(0.5f, 1.0f, 0.0f);

    auto vel = entity.velocity();
    // Y速度 = min(0.4, 0/2 + 0.5) = 0.4
    EXPECT_FLOAT_EQ(vel.y, 0.4f);

    // X速度 = 0/2 - 1.0 * 0.5 = -0.5（归一化后）
    // 由于方向向量被归一化，实际值需要计算
    EXPECT_LT(vel.x, 0.0f);
}

// ============================================================================
// Sleeping姿态碰撞箱测试
// ============================================================================

TEST(PlayerPoseWidth, SleepingWidth)
{
    // Sleeping姿态宽度应为 0.2
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(player.getDimensions(EntityPose::Sleeping).width(), 0.2f);
}

// ============================================================================
// 属性测试
// ============================================================================

TEST(EntityAttributes, KnockbackResistanceAttribute)
{
    // 确认击退抗性属性已定义
    auto attr = entity::attribute::Attributes::knockbackResistance();
    ASSERT_NE(attr, nullptr);
    EXPECT_EQ(attr->registryName(), "generic.knockback_resistance");
    EXPECT_FLOAT_EQ(static_cast<f32>(attr->defaultValue()), 0.0f);
}

TEST(EntityAttributes, MovementSpeedAttribute)
{
    // 确认移动速度属性已定义
    auto attr = entity::attribute::Attributes::movementSpeed();
    ASSERT_NE(attr, nullptr);
    EXPECT_EQ(attr->registryName(), "generic.movement_speed");
}

TEST(EntityAttributes, EntityGravityAttribute)
{
    // 确认重力属性已定义 (Forge 扩展)
    auto attr = entity::attribute::Attributes::entityGravity();
    ASSERT_NE(attr, nullptr);
    EXPECT_EQ(attr->registryName(), "forge.entity_gravity");
    EXPECT_FLOAT_EQ(static_cast<f32>(attr->defaultValue()), 0.08f);
}

// ============================================================================
// 新增物理常量测试
// ============================================================================

TEST(PhysicsConstants, WaterPhysics)
{
    // MC 1.16.5: 水中重力 = 地面重力 / 16
    EXPECT_FLOAT_EQ(physics::WATER_BUOYANCY, 0.005f);
    EXPECT_FLOAT_EQ(physics::WATER_DRAG, 0.8f);
    EXPECT_FLOAT_EQ(physics::LAVA_DRAG, 0.5f);
}

TEST(PhysicsConstants, SlipperinessConstants)
{
    // MC 1.16.5 滑度值
    EXPECT_FLOAT_EQ(physics::SLIPPERINESS_DEFAULT, 0.6f);
    EXPECT_FLOAT_EQ(physics::SLIPPERINESS_SLIME, 0.8f);
    EXPECT_FLOAT_EQ(physics::SLIPPERINESS_BLUE_ICE, 0.989f);
    // 蜂蜜块跳跃因子（不是滑度，是跳跃因子）
    EXPECT_FLOAT_EQ(physics::HONEY_BLOCK_JUMP_FACTOR, 0.5f);
}

TEST(PhysicsConstants, MovementConstants)
{
    // MC 1.16.5 移动常量
    EXPECT_FLOAT_EQ(physics::FLY_SPEED, 0.05f);
    EXPECT_FLOAT_EQ(physics::WALK_SPEED, 0.1f);
    EXPECT_FLOAT_EQ(physics::JUMP_MOVEMENT_FACTOR, 0.02f);
}

TEST(PhysicsConstants, LadderConstants)
{
    // MC 1.16.5 梯子物理
    EXPECT_FLOAT_EQ(physics::LADDER_SPEED_MAX, 0.15f);
    EXPECT_FLOAT_EQ(physics::LADDER_CLIMB_SPEED, 0.15f);
    EXPECT_FLOAT_EQ(physics::LADDER_SLIDE_SPEED, -0.15f);
}

TEST(PhysicsConstants, SpecialBlockConstants)
{
    // MC 1.16.5 特殊方块
    // 史莱姆块弹跳系数：活体实体 1.0，非活体实体 0.8
    EXPECT_FLOAT_EQ(physics::SLIME_BLOCK_BOUNCE_FACTOR_LIVING, 1.0f);
    EXPECT_FLOAT_EQ(physics::SLIME_BLOCK_BOUNCE_FACTOR_NON_LIVING, 0.8f);
    EXPECT_FLOAT_EQ(physics::HONEY_BLOCK_JUMP_FACTOR, 0.5f);
    // 蜘蛛网减速：XZ 平面 0.25，Y 轴 0.05
    EXPECT_FLOAT_EQ(physics::COBWEB_SLOWDOWN_XZ, 0.25f);
    EXPECT_FLOAT_EQ(physics::COBWEB_SLOWDOWN_Y, 0.05f);
}

TEST(PhysicsConstants, ElytraAndSlowFalling)
{
    // MC 1.16.5 鞘翅和缓降
    EXPECT_FLOAT_EQ(physics::SLOW_FALLING_GRAVITY, 0.01f);
    EXPECT_FLOAT_EQ(physics::ELYTRA_DRAG_HORIZONTAL, 0.99f);
    EXPECT_FLOAT_EQ(physics::ELYTRA_DRAG_VERTICAL, 0.98f);
}

TEST(PhysicsConstants, DolphinGrace)
{
    // MC 1.16.5 海豚的恩惠
    EXPECT_FLOAT_EQ(physics::DOLPHINS_GRACE_WATER_DRAG, 0.96f);
}
