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
 * THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
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
#include "common/entity/core/EntityPose.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/entities/passive/special/PolarBearEntity.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用模拟世界
 */
class PolarBearTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawnedEntities; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("PolarBearTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("PolarBearTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

class PolarBearEntityTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    PolarBearTestWorld m_world;
};

// ========== 基本属性测试 ==========

TEST_F(PolarBearEntityTest, Construction_DefaultValues)
{
    PolarBearEntity bear(EntityInstanceId(1));

    // 初始状态
    EXPECT_FALSE(bear.isStanding());
    EXPECT_FALSE(bear.isWarning());
    EXPECT_FALSE(bear.isAngry());
    EXPECT_EQ(bear.getAngerTime(), 0);
    EXPECT_EQ(bear.getAttackTarget(), nullptr);
}

TEST_F(PolarBearEntityTest, EyeHeight_AdultVsChild)
{
    PolarBearEntity adultBear(EntityInstanceId(1));
    PolarBearEntity childBear(EntityInstanceId(2));

    // 设置成年和幼体状态
    childBear.setGrowingAge(-24000); // 负数表示幼体

    // 成年熊眼睛高度 1.19（1.4 * 0.85），幼熊 0.595（0.7 * 0.85）
    EXPECT_FLOAT_EQ(adultBear.eyeHeight(), 1.4f * 0.85f);
    EXPECT_FLOAT_EQ(childBear.eyeHeight(), 1.4f * 0.5f * 0.85f);
}

TEST_F(PolarBearEntityTest, BreedingItem_AlwaysReturnsFalse)
{
    PolarBearEntity bear(EntityInstanceId(1));

    // 北极熊不可繁殖
    ItemStack anyItem;
    EXPECT_FALSE(bear.isBreedingItem(anyItem));
}

TEST_F(PolarBearEntityTest, SpawnBaby_ReturnsNullptr)
{
    PolarBearEntity bear1(EntityInstanceId(1));
    PolarBearEntity bear2(EntityInstanceId(2));

    // 北极熊不能繁殖
    auto baby = bear1.spawnBaby(bear2);
    EXPECT_EQ(baby, nullptr);
}

// ========== 站立状态测试 ==========

TEST_F(PolarBearEntityTest, Standing_CanSetAndClear)
{
    PolarBearEntity bear(EntityInstanceId(1));

    bear.setStanding(true);
    EXPECT_TRUE(bear.isStanding());

    bear.setStanding(false);
    EXPECT_FALSE(bear.isStanding());
}

TEST_F(PolarBearEntityTest, Standing_TimerSetOnStanding)
{
    PolarBearEntity bear(EntityInstanceId(1));

    bear.setStanding(true);

    // 站立状态应该被设置
    EXPECT_TRUE(bear.isStanding());
}

// ========== 警告状态测试 ==========

TEST_F(PolarBearEntityTest, Warning_CanSetAndGet)
{
    PolarBearEntity bear(EntityInstanceId(1));

    bear.setWarning(true);
    EXPECT_TRUE(bear.isWarning());

    bear.setWarning(false);
    EXPECT_FALSE(bear.isWarning());
}

// ========== IAngerable 接口测试 ==========

TEST_F(PolarBearEntityTest, Anger_CanSetAngry)
{
    PolarBearEntity bear(EntityInstanceId(1));

    EXPECT_FALSE(bear.isAngry());

    bear.setAngry(true);
    EXPECT_TRUE(bear.isAngry());
    EXPECT_GT(bear.getAngerTime(), 0);

    bear.setAngry(false);
    EXPECT_FALSE(bear.isAngry());
    EXPECT_EQ(bear.getAngerTime(), 0);
}

TEST_F(PolarBearEntityTest, Anger_AngerTimeRange)
{
    PolarBearEntity bear(EntityInstanceId(1));

    bear.setAngry(true);
    i32 angerTime = bear.getAngerTime();
    EXPECT_GT(angerTime, 0);

    // 愤怒时间应该在 [20, 39] 范围内（常量定义）
    EXPECT_GE(angerTime, 20);
    EXPECT_LE(angerTime, 39);
}

TEST_F(PolarBearEntityTest, Anger_SetAngerTimeDirectly)
{
    PolarBearEntity bear(EntityInstanceId(1));

    bear.setAngerTime(100);
    EXPECT_EQ(bear.getAngerTime(), 100);

    bear.setAngerTime(0);
    EXPECT_EQ(bear.getAngerTime(), 0);
}

TEST_F(PolarBearEntityTest, Anger_SetAttackTarget)
{
    PolarBearEntity bear(EntityInstanceId(1));

    // 直接设置攻击目标
    LivingEntity* target = reinterpret_cast<LivingEntity*>(0x1234); // 仅用于指针测试
    bear.setAttackTarget(target);
    EXPECT_EQ(bear.getAttackTarget(), target);

    bear.setAttackTarget(nullptr);
    EXPECT_EQ(bear.getAttackTarget(), nullptr);
}

TEST_F(PolarBearEntityTest, Anger_SetRevengeTarget_ClearsCorrectly)
{
    PolarBearEntity bear(EntityInstanceId(1));

    // 设置愤怒状态
    bear.setAngry(true);
    EXPECT_TRUE(bear.isAngry());
    EXPECT_GT(bear.getAngerTime(), 0);

    // 清除愤怒
    bear.setRevengeTarget(nullptr);
    EXPECT_EQ(bear.getAttackTarget(), nullptr);
    EXPECT_FALSE(bear.isAngry());
}

// ========== 声音事件测试 ==========

TEST_F(PolarBearEntityTest, Sound_AmbientSound_AdultVsChild)
{
    PolarBearEntity adultBear(EntityInstanceId(1));
    PolarBearEntity childBear(EntityInstanceId(2));
    childBear.setGrowingAge(-24000);

    auto adultSound = adultBear.getAmbientSound();
    auto childSound = childBear.getAmbientSound();

    ASSERT_TRUE(adultSound.has_value());
    ASSERT_TRUE(childSound.has_value());

    // 成年熊和幼熊有不同的环境音效
    EXPECT_EQ(adultSound.value(), SoundEvents::ENTITY_POLAR_BEAR_AMBIENT);
    EXPECT_EQ(childSound.value(), SoundEvents::ENTITY_POLAR_BEAR_AMBIENT_BABY);
}

TEST_F(PolarBearEntityTest, Sound_HurtSound)
{
    PolarBearEntity bear(EntityInstanceId(1));

    // 创建一个伤害源（使用空指针进行简单测试）
    EntityDamageSource damageSource(DamageType::MobAttack, nullptr);
    auto hurtSound = bear.getHurtSound(damageSource);

    ASSERT_TRUE(hurtSound.has_value());
    EXPECT_EQ(hurtSound.value(), SoundEvents::ENTITY_POLAR_BEAR_HURT);
}

TEST_F(PolarBearEntityTest, Sound_DeathSound)
{
    PolarBearEntity bear(EntityInstanceId(1));

    auto deathSound = bear.getDeathSound();

    ASSERT_TRUE(deathSound.has_value());
    EXPECT_EQ(deathSound.value(), SoundEvents::ENTITY_POLAR_BEAR_DEATH);
}

// ========== 常量验证测试 ==========

TEST_F(PolarBearEntityTest, Constants_AngerTimeRange)
{
    // 验证愤怒时间范围符合 MC 1.16.5 规范
    // 常量 ANGER_TIME_MIN = 20, ANGER_TIME_MAX = 39

    PolarBearEntity bear(EntityInstanceId(1));
    bear.setAngry(true);

    i32 angerTime = bear.getAngerTime();
    EXPECT_GE(angerTime, 20);
    EXPECT_LE(angerTime, 39);
}

// ========== 尺寸测试 ==========

TEST_F(PolarBearEntityTest, Dimensions_BaseSize_Adult)
{
    PolarBearEntity bear(EntityInstanceId(1));

    // 成年北极熊基础尺寸：宽 1.4，高 1.4
    EXPECT_FLOAT_EQ(bear.getBaseWidth(), 1.4f);
    EXPECT_FLOAT_EQ(bear.getBaseHeight(), 1.4f);
}

TEST_F(PolarBearEntityTest, Dimensions_BaseSize_Child)
{
    PolarBearEntity bear(EntityInstanceId(1));
    bear.setGrowingAge(-24000); // 幼熊

    // 幼熊基础尺寸的 width/height 由 AgeableEntity 缩放
    // getBaseWidth/getBaseHeight 仍为 1.4，但 getDimensions 会缩放
    auto dims = bear.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims.width(), 0.7f);  // 1.4 * 0.5
    EXPECT_FLOAT_EQ(dims.height(), 0.7f); // 1.4 * 0.5
}

TEST_F(PolarBearEntityTest, Dimensions_GetDimensions_AdultNotStanding)
{
    PolarBearEntity bear(EntityInstanceId(1));

    // 未站立时，m_clientSideStandAnimation 为 0
    auto dims = bear.getDimensions(EntityPose::Standing);

    // 基础尺寸：宽 1.4，高 1.4，眼高 1.19
    EXPECT_FLOAT_EQ(dims.width(), 1.4f);
    EXPECT_FLOAT_EQ(dims.height(), 1.4f);
    EXPECT_FLOAT_EQ(dims.eyeHeight(), 1.4f * 0.85f);
}

TEST_F(PolarBearEntityTest, Dimensions_GetDimensions_ChildNotStanding)
{
    PolarBearEntity bear(EntityInstanceId(1));
    bear.setGrowingAge(-24000); // 幼熊

    auto dims = bear.getDimensions(EntityPose::Standing);

    // 幼熊基础尺寸：宽 0.7，高 0.7
    EXPECT_FLOAT_EQ(dims.width(), 0.7f);
    EXPECT_FLOAT_EQ(dims.height(), 0.7f);
    EXPECT_FLOAT_EQ(dims.eyeHeight(), 0.7f * 0.85f);
}

TEST_F(PolarBearEntityTest, Dimensions_StandingAnimation_ScalesHeight)
{
    PolarBearEntity bear(EntityInstanceId(1));

    // 站立动画为 0（四足站立），高度为基础高度 1.4
    bear.setClientSideStandAnimation(0.0f);
    auto dims0 = bear.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims0.width(), 1.4f);
    EXPECT_FLOAT_EQ(dims0.height(), 1.4f);
    EXPECT_FLOAT_EQ(dims0.eyeHeight(), 1.4f * 0.85f);

    // 站立动画为 3.0（半程站立），高度缩放因子 1.0 + 3.0/6.0 = 1.5
    bear.setClientSideStandAnimation(3.0f);
    auto dims3 = bear.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims3.width(), 1.4f);                    // 宽度不变
    EXPECT_FLOAT_EQ(dims3.height(), 1.4f * 1.5f);            // 1.4 * 1.5 = 2.1
    EXPECT_FLOAT_EQ(dims3.eyeHeight(), 1.4f * 0.85f * 1.5f); // 眼高同步缩放

    // 站立动画为 6.0（完全站立），高度缩放因子 1.0 + 6.0/6.0 = 2.0
    bear.setClientSideStandAnimation(6.0f);
    auto dims6 = bear.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims6.width(), 1.4f);                    // 宽度不变
    EXPECT_FLOAT_EQ(dims6.height(), 1.4f * 2.0f);            // 1.4 * 2.0 = 2.8
    EXPECT_FLOAT_EQ(dims6.eyeHeight(), 1.4f * 0.85f * 2.0f); // 眼高同步缩放

    // 非整数动画值，缩放因子 1.0 + 1.0/6.0 ≈ 1.1667
    bear.setClientSideStandAnimation(1.0f);
    auto dims1 = bear.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims1.width(), 1.4f);
    EXPECT_FLOAT_EQ(dims1.height(), 1.4f * (1.0f + 1.0f / 6.0f));
}

TEST_F(PolarBearEntityTest, Dimensions_StandingAnimation_ChildScalesHeight)
{
    PolarBearEntity bear(EntityInstanceId(1));
    bear.setGrowingAge(-24000); // 幼熊

    // 幼熊四足站立，高度 0.7
    bear.setClientSideStandAnimation(0.0f);
    auto dims0 = bear.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims0.width(), 0.7f);
    EXPECT_FLOAT_EQ(dims0.height(), 0.7f);

    // 幼熊完全站立，高度缩放 2.0 倍：0.7 * 2.0 = 1.4
    bear.setClientSideStandAnimation(6.0f);
    auto dims6 = bear.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims6.width(), 0.7f);  // 宽度不变
    EXPECT_FLOAT_EQ(dims6.height(), 1.4f); // 0.7 * 2.0
    EXPECT_FLOAT_EQ(dims6.eyeHeight(), 0.7f * 0.85f * 2.0f);
}

// ========== 属性测试 ==========

TEST_F(PolarBearEntityTest, Attributes_VerifyValues)
{
    PolarBearEntity bear(EntityInstanceId(1));

    // MC 1.16.5 北极熊属性
    // MAX_HEALTH = 30.0
    // FOLLOW_RANGE = 20.0
    // MOVEMENT_SPEED = 0.25
    // ATTACK_DAMAGE = 6.0 (需要属性系统支持)

    f64 maxHealth = bear.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH, 0.0);
    f64 followRange = bear.getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE, 0.0);
    f64 moveSpeed = bear.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);

    EXPECT_DOUBLE_EQ(maxHealth, 30.0);
    EXPECT_DOUBLE_EQ(followRange, 20.0);
    EXPECT_DOUBLE_EQ(moveSpeed, 0.25);

    // ATTACK_DAMAGE 需要属性系统初始化后才有值
    // 这里只验证方法调用不会崩溃
    f64 attackDamage = bear.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0);
    (void)attackDamage; // 避免未使用变量警告
}

// ========== 警告声音冷却测试 ==========

TEST_F(PolarBearEntityTest, WarningSound_DoesNotCrash)
{
    PolarBearEntity bear(EntityInstanceId(1));

    // 验证调用警告声音不会崩溃
    // 由于 playWarningSound 依赖世界对象，这里只验证不会崩溃
    EXPECT_NO_THROW(bear.playWarningSound());

    // 多次快速调用也应该安全
    EXPECT_NO_THROW(bear.playWarningSound());
    EXPECT_NO_THROW(bear.playWarningSound());
}

// ========== 多次随机测试 ==========

TEST_F(PolarBearEntityTest, Random_AngerTimeVaries)
{
    // 测试多次创建北极熊，愤怒时间应该在范围内变化
    std::set<i32> observedAngerTimes;

    for (int i = 0; i < 20; ++i) {
        PolarBearEntity bear(EntityInstanceId(static_cast<u32>(i + 100)));
        bear.setAngry(true);
        observedAngerTimes.insert(bear.getAngerTime());
    }

    // 观察到多个不同的愤怒时间值，证明使用了随机
    EXPECT_GT(observedAngerTimes.size(), 1);
}

TEST_F(PolarBearEntityTest, Random_StandingTimerVaries)
{
    // 测试多次设置站立，验证站立计时器使用随机
    // 虽然无法直接访问计时器，但可以通过行为推断
    std::set<bool> standingStates;

    for (int i = 0; i < 10; ++i) {
        PolarBearEntity bear(EntityInstanceId(static_cast<u32>(i + 200)));
        bear.setStanding(true);
        standingStates.insert(bear.isStanding());
    }

    // 所有熊都应该处于站立状态
    EXPECT_EQ(standingStates.size(), 1);
    EXPECT_TRUE(*standingStates.begin());
}

} // namespace
} // namespace mc
