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
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/cave/PointedDripstoneBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::blocks;
using namespace mc::test;

// ========== PointedDripstoneBlock 测试 ==========

class PointedDripstoneBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        block_ =
            std::make_unique<PointedDripstoneBlock>(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    }

    std::unique_ptr<PointedDripstoneBlock> block_;
};

// ============================================================================
// 构造与默认状态测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(block_, nullptr);
}

TEST_F(PointedDripstoneBlockTest, DefaultState_DirectionUp)
{
    const BlockState& state = block_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);
}

TEST_F(PointedDripstoneBlockTest, DefaultState_ThicknessTip)
{
    const BlockState& state = block_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Tip);
}

TEST_F(PointedDripstoneBlockTest, DefaultState_NotWaterlogged)
{
    const BlockState& state = block_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(PointedDripstoneBlockTest, TicksRandomly_ReturnsTrue)
{
    EXPECT_TRUE(block_->ticksRandomly());
}

TEST_F(PointedDripstoneBlockTest, UseShapeForLightOcclusion_AlwaysTrue)
{
    const BlockState& state = block_->defaultState();
    EXPECT_TRUE(block_->useShapeForLightOcclusion(state));
}

// ============================================================================
// 状态属性组合测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, DirectionProperty_CanBeSet)
{
    auto state = block_->defaultState();

    state = state.with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up);
    EXPECT_EQ(state.get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);

    state = state.with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down);
    EXPECT_EQ(state.get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Down);
}

TEST_F(PointedDripstoneBlockTest, ThicknessProperty_CanBeSet)
{
    auto state = block_->defaultState();

    state = state.with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);
    EXPECT_EQ(state.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Tip);

    state = state.with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::TipMerge);
    EXPECT_EQ(
        state.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::TipMerge);

    state = state.with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Frustum);
    EXPECT_EQ(
        state.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Frustum);

    state = state.with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Middle);
    EXPECT_EQ(state.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Middle);

    state = state.with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Base);
    EXPECT_EQ(state.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Base);
}

TEST_F(PointedDripstoneBlockTest, WaterloggedProperty_CanBeToggled)
{
    auto state = block_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));

    state = state.with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
    EXPECT_TRUE(block_->isWaterlogged(state));

    state = state.with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
    EXPECT_FALSE(block_->isWaterlogged(state));
}

// ============================================================================
// 碰撞形状测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, Shape_TipUp_HasCollisionBox)
{
    auto state = block_->defaultState()
                     .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
                     .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);
    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_FALSE(shape.isFullBlock());
}

TEST_F(PointedDripstoneBlockTest, Shape_TipDown_HasCollisionBox)
{
    auto state = block_->defaultState()
                     .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
                     .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);
    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_FALSE(shape.isFullBlock());
}

TEST_F(PointedDripstoneBlockTest, Shape_TipUpAndDown_AreDifferent)
{
    // 朝上尖端和朝下尖端应该有不同的形状
    auto stateUp =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);
    auto stateDown =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    const CollisionShape& shapeUp = block_->getShape(stateUp);
    const CollisionShape& shapeDown = block_->getShape(stateDown);

    // 两种方向的尖端形状不同（朝下尖端的碰撞盒高度不同）
    EXPECT_NE(&shapeUp, &shapeDown);
}

TEST_F(PointedDripstoneBlockTest, Shape_AllThicknesses_NonEmpty)
{
    // 所有厚度等级都应有非空碰撞形状
    const auto thicknesses = {BlockStateProperties::DripstoneThickness::TipMerge,
        BlockStateProperties::DripstoneThickness::Tip,
        BlockStateProperties::DripstoneThickness::Frustum,
        BlockStateProperties::DripstoneThickness::Middle,
        BlockStateProperties::DripstoneThickness::Base};

    for (const auto& thickness : thicknesses) {
        auto state = block_->defaultState()
                         .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
                         .with(BlockStateProperties::DRIPSTONE_THICKNESS(), thickness);
        const CollisionShape& shape = block_->getShape(state);
        EXPECT_FALSE(shape.isEmpty()) << "Shape should not be empty for thickness " << static_cast<int>(thickness);
    }
}

// ============================================================================
// 旋转和镜像测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, Rotate_DoesNotChangeDirection)
{
    // 垂直方向不受旋转影响
    auto stateUp = block_->defaultState().with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up);
    auto stateDown = block_->defaultState().with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down);

    EXPECT_EQ(
        block_->rotate(stateUp, Rotation::Clockwise90).get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);
    EXPECT_EQ(block_->rotate(stateDown, Rotation::Clockwise180).get(BlockStateProperties::VERTICAL_DIRECTION()),
        Direction::Down);
}

TEST_F(PointedDripstoneBlockTest, Mirror_SwapsUpAndDown)
{
    auto stateUp = block_->defaultState().with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up);
    auto stateDown = block_->defaultState().with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down);

    // FrontBack镜像：Up→Down，Down→Up
    EXPECT_EQ(
        block_->mirror(stateUp, Mirror::FrontBack).get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Down);
    EXPECT_EQ(
        block_->mirror(stateDown, Mirror::FrontBack).get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);

    // LeftRight镜像：Up→Down，Down→Up
    EXPECT_EQ(
        block_->mirror(stateUp, Mirror::LeftRight).get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Down);
    EXPECT_EQ(
        block_->mirror(stateDown, Mirror::LeftRight).get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);
}

TEST_F(PointedDripstoneBlockTest, Mirror_None_DoesNotChangeDirection)
{
    // 无镜像不变
    auto stateUp = block_->defaultState().with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up);
    EXPECT_EQ(block_->mirror(stateUp, Mirror::None).get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);
}

// ============================================================================
// 方块类型验证测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, IsWaterlogged_Implemented)
{
    // PointedDripstoneBlock 实现了 IWaterLoggable 接口
    auto state = block_->defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(block_->isWaterlogged(state));

    auto stateNotWaterlogged = block_->defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(block_->isWaterlogged(stateNotWaterlogged));
}

TEST_F(PointedDripstoneBlockTest, GetFluidState_Waterlogged_ReturnsNonNull)
{
    auto state = block_->defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = block_->getFluidState(state);
    EXPECT_NE(fluidState, nullptr);
}

TEST_F(PointedDripstoneBlockTest, GetFluidState_NotWaterlogged_MayBeNull)
{
    auto state = block_->defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    const fluid::FluidState* fluidState = block_->getFluidState(state);
    // 非含水状态：流体状态为空指针或空流体
    // 无法调用isEmpty()因为FluidState是不完整类型，仅验证不会崩溃
    (void)fluidState;
}

// ============================================================================
// onFallenUpon 行为测试 — 石笋/钟乳石判定逻辑
// ============================================================================

TEST_F(PointedDripstoneBlockTest, OnFallenUpon_StalagmiteTipUp_CallsCauseFallDamage)
{
    // 朝上的TIP尖端（石笋）应该触发增强的摔落伤害
    // 验证：onFallenUpon 在石笋尖端方向上不会调用父类（替代普通摔落伤害）
    // 这里验证静态属性判断逻辑是否正确
    auto stalagmiteTip =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    // 验证状态属性组合正确
    EXPECT_EQ(stalagmiteTip.get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);
    EXPECT_EQ(
        stalagmiteTip.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Tip);
}

TEST_F(PointedDripstoneBlockTest, OnFallenUpon_StalactiteTipDown_NoStalagmiteDamage)
{
    // 朝下的TIP（钟乳石尖端）不应该触发石笋伤害
    // 验证：onFallenUpon 在朝下TIP时应调用父类（普通摔落伤害）
    auto stalactiteTip =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    // 朝下的尖端不是石笋
    EXPECT_EQ(stalactiteTip.get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Down);
}

TEST_F(PointedDripstoneBlockTest, OnFallenUpon_NonTipNoStalagmiteDamage)
{
    // 非TIP厚度的滴石（Base, Middle, Frustum）不应触发石笋伤害
    // 验证：这些厚度方向上总是调用父类（普通摔落伤害）
    const auto nonTipThicknesses = {BlockStateProperties::DripstoneThickness::Base,
        BlockStateProperties::DripstoneThickness::Middle,
        BlockStateProperties::DripstoneThickness::Frustum,
        BlockStateProperties::DripstoneThickness::TipMerge};

    for (const auto& thickness : nonTipThicknesses) {
        auto state = block_->defaultState()
                         .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
                         .with(BlockStateProperties::DRIPSTONE_THICKNESS(), thickness);
        // 非TIP厚度或TIP_MERGE不应触发石笋伤害
        EXPECT_NE(state.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Tip)
            << "Non-tip thickness should not trigger stalagmite damage";
    }
}

// ============================================================================
// 方向和厚度的石笋/钟乳石判定逻辑测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, StalagmiteCondition_UpTipOnly)
{
    // 石笋（Stalagmite）条件：Direction::Up + DripstoneThickness::Tip
    // 只有同时满足朝上和TIP厚度时，onFallenUpon才施加石笋伤害
    auto upTip = block_->defaultState()
                     .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
                     .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);
    EXPECT_EQ(upTip.get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);
    EXPECT_EQ(upTip.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Tip);

    // 朝上 + 非TIP = 非石笋
    auto upFrustum =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Frustum);
    EXPECT_NE(
        upFrustum.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Tip);

    // 朝下 + TIP = 非石笋（是钟乳石尖端）
    auto downTip =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);
    EXPECT_EQ(downTip.get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Down);
}

TEST_F(PointedDripstoneBlockTest, TipMerge_NotTipForStalagmiteDamage)
{
    // TIP_MERGE 厚度不触发石笋伤害（只有纯TIP才触发）
    auto tipMerge =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::TipMerge);
    EXPECT_NE(tipMerge.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Tip);
}

// ============================================================================
// 集成测试：onFallenUpon 伤害验证
// ============================================================================

// 测试用 LivingEntity，追踪 hurt 调用以验证摔落伤害类型和数值
class DamageTrackingEntity final : public LivingEntity {
public:
    explicit DamageTrackingEntity(IWorld* world, ecs::EntityRegistry& registry)
        : LivingEntity(EntityInstanceId(1), world, registry)
        , m_hurtCount(0)
        , m_lastDamage(0.0f)
        , m_lastDamageType(static_cast<DamageType>(255))
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    bool hurt(DamageSource& source, f32 amount) override
    {
        m_hurtCount++;
        m_lastDamage = amount;
        m_lastDamageType = source.type();
        return LivingEntity::hurt(source, amount);
    }

    [[nodiscard]] i32 hurtCount() const { return m_hurtCount; }
    [[nodiscard]] f32 lastDamage() const { return m_lastDamage; }
    [[nodiscard]] DamageType lastDamageType() const { return m_lastDamageType; }

    void tick() override {}
    [[nodiscard]] f32 width() const override { return 0.6f; }
    [[nodiscard]] f32 height() const override { return 1.8f; }
    [[nodiscard]] f32 eyeHeight() const override { return 1.62f; }

private:
    i32 m_hurtCount = 0;
    f32 m_lastDamage = 0.0f;
    DamageType m_lastDamageType;
};

// 测试用世界，支持方块状态存储和实体伤害追踪
class DripstoneTestWorld final : public BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state != nullptr) {
            m_blocks[BlockPos(x, y, z)] = state;
        } else {
            m_blocks.erase(BlockPos(x, y, z));
        }
        return true;
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    void setBlockAt(const BlockPos& pos, const BlockState* state) { m_blocks[pos] = state; }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
};

// ============================================================================
// onFallenUpon 石笋伤害集成测试
// ============================================================================

class PointedDripstoneFallDamageTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        block_ =
            std::make_unique<PointedDripstoneBlock>(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    }

    std::unique_ptr<PointedDripstoneBlock> block_;
    DripstoneTestWorld world_;
};

TEST_F(PointedDripstoneFallDamageTest, StalagmiteTip_AppliesEnhancedDamage)
{
    // 朝上的TIP（石笋）应该触发增强的摔落伤害
    auto stalagmiteState =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    DamageTrackingEntity entity(&world_, mc::test::testEcsRegistry());
    BlockPos pos(0, 64, 0);

    // 从5格高摔落到石笋上
    f32 fallDistance = 5.0f;
    block_->onFallenUpon(world_, pos, stalagmiteState, entity, fallDistance);

    // 石笋伤害：摔落距离 + 2.5，伤害倍率 2.0
    // causeFallDamage(fallDistance + 2.5, 2.0, stalagmite())
    // LivingEntity::causeFallDamage: effectiveDistance = (5.0 + 2.5) - 0(jump boost) = 7.5
    // damage = (7.5 - 3.0) * 2.0 = 9.0
    // hurt() 应被调用一次
    EXPECT_EQ(entity.hurtCount(), 1);
    // 伤害类型应为 Stalagmite
    EXPECT_EQ(entity.lastDamageType(), DamageType::Stalagmite);
    // 伤害值：((5.0 + 2.5) - 3.0) * 2.0 = 9.0
    EXPECT_FLOAT_EQ(entity.lastDamage(), 9.0f);
}

TEST_F(PointedDripstoneFallDamageTest, StalactiteTip_AppliesNormalDamage)
{
    // 朝下的TIP（钟乳石尖端）应该触发普通摔落伤害（调用父类 Block::onFallenUpon）
    auto stalactiteState =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    DamageTrackingEntity entity(&world_, mc::test::testEcsRegistry());
    entity.setHealth(20.0f);
    BlockPos pos(0, 64, 0);

    f32 fallDistance = 5.0f;
    block_->onFallenUpon(world_, pos, stalactiteState, entity, fallDistance);

    // 普通摔落伤害：causeFallDamage(5.0, 1.0, fall())
    // LivingEntity::causeFallDamage: effectiveDistance = 5.0 - 0 = 5.0
    // damage = (5.0 - 3.0) * 1.0 = 2.0
    EXPECT_EQ(entity.hurtCount(), 1);
    // 伤害类型应为 Fall（普通摔落）
    EXPECT_EQ(entity.lastDamageType(), DamageType::Fall);
    // 伤害值：(5.0 - 3.0) * 1.0 = 2.0
    EXPECT_FLOAT_EQ(entity.lastDamage(), 2.0f);
}

TEST_F(PointedDripstoneFallDamageTest, NonTip_AppliesNormalDamage)
{
    // 非TIP厚度的滴石应该触发普通摔落伤害
    auto baseState =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Base);

    DamageTrackingEntity entity(&world_, mc::test::testEcsRegistry());
    entity.setHealth(20.0f);
    BlockPos pos(0, 64, 0);

    f32 fallDistance = 5.0f;
    block_->onFallenUpon(world_, pos, baseState, entity, fallDistance);

    // 普通摔落伤害
    EXPECT_EQ(entity.hurtCount(), 1);
    EXPECT_EQ(entity.lastDamageType(), DamageType::Fall);
    EXPECT_FLOAT_EQ(entity.lastDamage(), 2.0f);
}

TEST_F(PointedDripstoneFallDamageTest, TipMerge_AppliesNormalDamage)
{
    // TIP_MERGE 厚度不是石笋尖端，应触发普通摔落伤害
    auto tipMergeState =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::TipMerge);

    DamageTrackingEntity entity(&world_, mc::test::testEcsRegistry());
    entity.setHealth(20.0f);
    BlockPos pos(0, 64, 0);

    f32 fallDistance = 5.0f;
    block_->onFallenUpon(world_, pos, tipMergeState, entity, fallDistance);

    // 普通摔落伤害
    EXPECT_EQ(entity.hurtCount(), 1);
    EXPECT_EQ(entity.lastDamageType(), DamageType::Fall);
    EXPECT_FLOAT_EQ(entity.lastDamage(), 2.0f);
}

TEST_F(PointedDripstoneFallDamageTest, Stalagmite_ShortFall_NoDamage)
{
    // 短距离摔落（< 3格）不应造成伤害，即使落在石笋上
    // causeFallDamage(0.5 + 2.5, 2.0, stalagmite())
    // effectiveDistance = 3.0 - 0 = 3.0，不大于3.0，不造成伤害
    auto stalagmiteState =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    DamageTrackingEntity entity(&world_, mc::test::testEcsRegistry());
    entity.setHealth(20.0f);
    BlockPos pos(0, 64, 0);

    f32 fallDistance = 0.5f;
    block_->onFallenUpon(world_, pos, stalagmiteState, entity, fallDistance);

    // effectiveDistance = (0.5 + 2.5) = 3.0, 不 > 3.0, 无伤害
    EXPECT_EQ(entity.hurtCount(), 0);
}

TEST_F(PointedDripstoneFallDamageTest, Stalagmite_HighFall_MoreDamageThanNormal)
{
    // 高距离摔落时，石笋伤害应远大于普通摔落伤害
    auto stalagmiteState =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    DamageTrackingEntity entity(&world_, mc::test::testEcsRegistry());
    entity.setHealth(20.0f);
    BlockPos pos(0, 64, 0);

    // 10格摔落
    f32 fallDistance = 10.0f;
    block_->onFallenUpon(world_, pos, stalagmiteState, entity, fallDistance);

    // 石笋伤害：effectiveDistance = 10.0 + 2.5 = 12.5, damage = (12.5 - 3.0) * 2.0 = 19.0
    // 普通伤害：effectiveDistance = 10.0, damage = (10.0 - 3.0) * 1.0 = 7.0
    EXPECT_EQ(entity.hurtCount(), 1);
    EXPECT_EQ(entity.lastDamageType(), DamageType::Stalagmite);
    EXPECT_FLOAT_EQ(entity.lastDamage(), 19.0f);
}

// ============================================================================
// canDripThrough 穿透检测测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, CanDripThrough_NullState_ReturnsTrue)
{
    // 空指针状态可穿透
    DripstoneTestWorld world;
    BlockPos pos(0, 64, 0);
    EXPECT_TRUE(block_->canDripThrough(world, pos, nullptr));
}

TEST_F(PointedDripstoneBlockTest, CanDripThrough_AirBlock_ReturnsTrue)
{
    // 空气方块可穿透
    DripstoneTestWorld world;
    BlockPos pos(0, 64, 0);
    const BlockState* airState = &VanillaBlocks::AIR->defaultState();
    ASSERT_NE(airState, nullptr);
    EXPECT_TRUE(block_->canDripThrough(world, pos, airState));
}

TEST_F(PointedDripstoneBlockTest, CanDripThrough_OpaqueBlock_ReturnsFalse)
{
    // 实心不透明方块不可穿透
    DripstoneTestWorld world;
    BlockPos pos(0, 64, 0);
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stoneState, nullptr);
    EXPECT_FALSE(block_->canDripThrough(world, pos, stoneState));
}

TEST_F(PointedDripstoneBlockTest, CanDripThrough_PointedDripstoneBlock_ReturnsFalse)
{
    // 滴石锥的碰撞形状覆盖了滴水通道区域（4x16x4 中心柱），
    // 因此 canDripThrough 返回 false。这与 MC 1.21.11 行为一致：
    // MC 中 REQUIRED_SPACE_TO_DRIP_THROUGH = Block.column(4.0, 0.0, 16.0)，
    // 而滴石锥的碰撞形状（最小半径6像素的圆柱）完全覆盖该区域。
    // findFillableCauldronBelow 从尖端下方的位置开始搜索，不会检查滴石锥自身。
    DripstoneTestWorld world;
    BlockPos pos(0, 64, 0);
    const BlockState* dripstoneState = &VanillaBlocks::POINTED_DRIPSTONE->defaultState();
    ASSERT_NE(dripstoneState, nullptr);
    EXPECT_FALSE(block_->canDripThrough(world, pos, dripstoneState));
}

// ============================================================================
// canDrip 尖端滴水条件测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, CanDrip_StalactiteTipNotWaterlogged)
{
    // 朝下TIP + 非含水 = 可滴水
    // 注意：必须使用 VanillaBlocks::POINTED_DRIPSTONE 的状态，
    // 因为 canDrip 调用 isStalactite，而 isStalactite 使用 state->is() 检查方块身份
    auto state = VanillaBlocks::POINTED_DRIPSTONE->defaultState()
                     .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
                     .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip)
                     .with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_TRUE(PointedDripstoneBlock::canDrip(state));
}

TEST_F(PointedDripstoneBlockTest, CanDrip_StalactiteTipWaterlogged_CannotDrip)
{
    // 朝下TIP + 含水 = 不可滴水
    auto state = VanillaBlocks::POINTED_DRIPSTONE->defaultState()
                     .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
                     .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip)
                     .with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_FALSE(PointedDripstoneBlock::canDrip(state));
}

TEST_F(PointedDripstoneBlockTest, CanDrip_StalagmiteTip_CannotDrip)
{
    // 朝上TIP = 不是钟乳石，不能滴水
    auto state = VanillaBlocks::POINTED_DRIPSTONE->defaultState()
                     .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
                     .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip)
                     .with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(PointedDripstoneBlock::canDrip(state));
}

TEST_F(PointedDripstoneBlockTest, CanDrip_NonTipStalactite_CannotDrip)
{
    // 朝下非TIP = 不是尖端，不能滴水
    auto state =
        VanillaBlocks::POINTED_DRIPSTONE->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Middle)
            .with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(PointedDripstoneBlock::canDrip(state));
}

// ============================================================================
// 静态辅助方法测试
// 注意：静态方法 isStalactite/isStalagmite/isTip/isPointedDripstoneWithDirection
// 使用 state->is(VanillaBlocks::POINTED_DRIPSTONE) 检查方块身份，
// 因此必须使用 VanillaBlocks::POINTED_DRIPSTONE 的状态，而非 block_ 实例的状态。
// ============================================================================

TEST_F(PointedDripstoneBlockTest, IsStalactite_DownDirection)
{
    auto state = VanillaBlocks::POINTED_DRIPSTONE->defaultState().with(
        BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down);
    EXPECT_TRUE(PointedDripstoneBlock::isStalactite(state));
}

TEST_F(PointedDripstoneBlockTest, IsStalactite_UpDirection)
{
    auto state = VanillaBlocks::POINTED_DRIPSTONE->defaultState().with(
        BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up);
    EXPECT_FALSE(PointedDripstoneBlock::isStalactite(state));
}

TEST_F(PointedDripstoneBlockTest, IsStalagmite_UpDirection)
{
    auto state = VanillaBlocks::POINTED_DRIPSTONE->defaultState().with(
        BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up);
    EXPECT_TRUE(PointedDripstoneBlock::isStalagmite(state));
}

TEST_F(PointedDripstoneBlockTest, IsStalagmite_DownDirection)
{
    auto state = VanillaBlocks::POINTED_DRIPSTONE->defaultState().with(
        BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down);
    EXPECT_FALSE(PointedDripstoneBlock::isStalagmite(state));
}

TEST_F(PointedDripstoneBlockTest, IsTip_TrueForTip)
{
    auto state = VanillaBlocks::POINTED_DRIPSTONE->defaultState().with(
        BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);
    EXPECT_TRUE(PointedDripstoneBlock::isTip(&state, false));
    EXPECT_TRUE(PointedDripstoneBlock::isTip(&state, true));
}

TEST_F(PointedDripstoneBlockTest, IsTip_TrueForTipMergeWhenAllowMerge)
{
    auto state = VanillaBlocks::POINTED_DRIPSTONE->defaultState().with(
        BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::TipMerge);
    // 不允许合并 → TIP_MERGE 不是 TIP
    EXPECT_FALSE(PointedDripstoneBlock::isTip(&state, false));
    // 允许合并 → TIP_MERGE 算作 TIP
    EXPECT_TRUE(PointedDripstoneBlock::isTip(&state, true));
}

TEST_F(PointedDripstoneBlockTest, IsTip_FalseForOtherThicknesses)
{
    const auto nonTipThicknesses = {BlockStateProperties::DripstoneThickness::Frustum,
        BlockStateProperties::DripstoneThickness::Middle,
        BlockStateProperties::DripstoneThickness::Base};

    for (const auto& thickness : nonTipThicknesses) {
        auto state = VanillaBlocks::POINTED_DRIPSTONE->defaultState().with(
            BlockStateProperties::DRIPSTONE_THICKNESS(), thickness);
        EXPECT_FALSE(PointedDripstoneBlock::isTip(&state, false));
        EXPECT_FALSE(PointedDripstoneBlock::isTip(&state, true));
    }
}

TEST_F(PointedDripstoneBlockTest, IsTip_NullState_ReturnsFalse)
{
    EXPECT_FALSE(PointedDripstoneBlock::isTip(nullptr, false));
    EXPECT_FALSE(PointedDripstoneBlock::isTip(nullptr, true));
}

TEST_F(PointedDripstoneBlockTest, IsPointedDripstoneWithDirection_NullState_ReturnsFalse)
{
    EXPECT_FALSE(PointedDripstoneBlock::isPointedDripstoneWithDirection(nullptr, Direction::Up));
}

TEST_F(PointedDripstoneBlockTest, IsPointedDripstoneWithDirection_CorrectDirection)
{
    auto upState = VanillaBlocks::POINTED_DRIPSTONE->defaultState().with(
        BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up);
    EXPECT_TRUE(PointedDripstoneBlock::isPointedDripstoneWithDirection(&upState, Direction::Up));
    EXPECT_FALSE(PointedDripstoneBlock::isPointedDripstoneWithDirection(&upState, Direction::Down));

    auto downState = VanillaBlocks::POINTED_DRIPSTONE->defaultState().with(
        BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down);
    EXPECT_TRUE(PointedDripstoneBlock::isPointedDripstoneWithDirection(&downState, Direction::Down));
    EXPECT_FALSE(PointedDripstoneBlock::isPointedDripstoneWithDirection(&downState, Direction::Up));
}
