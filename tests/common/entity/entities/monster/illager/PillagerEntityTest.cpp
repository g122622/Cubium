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
#include "common/entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/monster/illager/IllagerEntities.hpp"
#include "common/entity/interfaces/ICrossbowUser.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

#include <memory>

namespace mc {
namespace {

/**
 * @brief 测试用世界实现
 */
class PillagerTestWorld final : public test::BaseTestWorld {
public:
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return EntityInstanceId(static_cast<u32>(m_spawnedEntities.size()));
    }

    void advanceTick() { m_currentTick++; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("PillagerTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("PillagerTestWorld::tickManager not implemented");
    }

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

private:
    u64 m_currentTick = 0;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

} // namespace

// ============================================================================
// PillagerEntity 基础测试
// ============================================================================

TEST(PillagerEntityTest, Construction)
{
    PillagerEntity pillager(EntityInstanceId(1));

    // 验证掠夺者尺寸
    EXPECT_FLOAT_EQ(pillager.width(), 0.6f);
    EXPECT_FLOAT_EQ(pillager.height(), 1.95f);

    // 验证默认装填状态
    EXPECT_FALSE(pillager.isCharging());
    EXPECT_FALSE(pillager.isChargingCrossbow());
}

TEST(PillagerEntityTest, Attributes)
{
    PillagerEntity pillager(EntityInstanceId(1));

    // MC 1.16.5 PillagerEntity 属性
    EXPECT_FLOAT_EQ(static_cast<f32>(pillager.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH)), 24.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(pillager.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED)), 0.35f);
    EXPECT_FLOAT_EQ(static_cast<f32>(pillager.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE)), 5.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(pillager.getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE)), 32.0f);
}

TEST(PillagerEntityTest, CrossbowUserInterface)
{
    PillagerEntity pillager(EntityInstanceId(1));

    // 验证 ICrossbowUser 接口
    entity::ICrossbowUser* crossbowUser = dynamic_cast<entity::ICrossbowUser*>(&pillager);
    ASSERT_NE(crossbowUser, nullptr);

    // 测试装填状态设置
    crossbowUser->setChargingCrossbow(true);
    EXPECT_TRUE(crossbowUser->isChargingCrossbow());
    EXPECT_TRUE(pillager.isCharging());

    crossbowUser->setChargingCrossbow(false);
    EXPECT_FALSE(crossbowUser->isChargingCrossbow());
    EXPECT_FALSE(pillager.isCharging());

    // 测试装填时间
    EXPECT_EQ(crossbowUser->getCrossbowChargeTime(), 25);
}

TEST(PillagerEntityTest, RangedAttackInterface)
{
    PillagerEntity pillager(EntityInstanceId(1));

    // 验证 IRangedAttackMob 接口
    entity::IRangedAttackMob* rangedAttacker = dynamic_cast<entity::IRangedAttackMob*>(&pillager);
    ASSERT_NE(rangedAttacker, nullptr);

    // 测试攻击间隔
    EXPECT_EQ(rangedAttacker->getAttackInterval(), 20);

    // 测试可以远程攻击
    EXPECT_TRUE(rangedAttacker->canRangedAttack());
}

TEST(PillagerEntityTest, CreateFactory)
{
    auto entity = PillagerEntity::create(nullptr);
    ASSERT_NE(entity, nullptr);

    // 验证创建的是 PillagerEntity
    auto* pillagerPtr = dynamic_cast<PillagerEntity*>(entity.get());
    EXPECT_NE(pillagerPtr, nullptr);
}

TEST(PillagerEntityTest, ChargingState)
{
    PillagerEntity pillager(EntityInstanceId(1));

    // 默认不装填
    EXPECT_FALSE(pillager.isCharging());

    // 设置装填状态
    pillager.setCharging(true);
    EXPECT_TRUE(pillager.isCharging());
    EXPECT_TRUE(pillager.isChargingCrossbow());

    pillager.setCharging(false);
    EXPECT_FALSE(pillager.isCharging());
    EXPECT_FALSE(pillager.isChargingCrossbow());
}

// ============================================================================
// VindicatorEntity 基础测试
// ============================================================================

TEST(VindicatorEntityTest, Construction)
{
    VindicatorEntity vindicator(EntityInstanceId(1));

    // 验证卫道士尺寸
    EXPECT_FLOAT_EQ(vindicator.width(), 0.6f);
    EXPECT_FLOAT_EQ(vindicator.height(), 1.95f);

    // 验证默认攻击状态
    EXPECT_FALSE(vindicator.isAggressive());
}

TEST(VindicatorEntityTest, Attributes)
{
    VindicatorEntity vindicator(EntityInstanceId(1));

    // MC 1.16.5 VindicatorEntity 属性
    EXPECT_FLOAT_EQ(static_cast<f32>(vindicator.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH)), 24.0f);
    EXPECT_FLOAT_EQ(
        static_cast<f32>(vindicator.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED)), 0.35f);
    EXPECT_FLOAT_EQ(static_cast<f32>(vindicator.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE)), 5.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(vindicator.getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE)), 12.0f);
}

TEST(VindicatorEntityTest, AggressiveState)
{
    VindicatorEntity vindicator(EntityInstanceId(1));

    // 默认不攻击
    EXPECT_FALSE(vindicator.isAggressive());

    // 设置攻击状态
    vindicator.setAggressive(true);
    EXPECT_TRUE(vindicator.isAggressive());

    vindicator.setAggressive(false);
    EXPECT_FALSE(vindicator.isAggressive());
}

TEST(VindicatorEntityTest, CreateFactory)
{
    auto entity = VindicatorEntity::create(nullptr);
    ASSERT_NE(entity, nullptr);

    // 验证创建的是 VindicatorEntity
    auto* vindicatorPtr = dynamic_cast<VindicatorEntity*>(entity.get());
    EXPECT_NE(vindicatorPtr, nullptr);
}

// ============================================================================
// RangedCrossbowAttackGoal 测试
// ============================================================================

TEST(RangedCrossbowAttackGoalTest, CrossbowStateEnum)
{
    // 验证状态枚举值
    EXPECT_EQ(static_cast<u8>(entity::ai::goal::CrossbowState::Uncharged), 0);
    EXPECT_EQ(static_cast<u8>(entity::ai::goal::CrossbowState::Charging), 1);
    EXPECT_EQ(static_cast<u8>(entity::ai::goal::CrossbowState::Charged), 2);
    EXPECT_EQ(static_cast<u8>(entity::ai::goal::CrossbowState::ReadyToAttack), 3);
}

} // namespace mc
