/**
 * @file test_water_movement.cpp
 * @brief 生物（非玩家）水中移动输入因子的单元测试
 *
 * 覆盖 LivingEntity::travel 在液体中的移动输入因子（moveFactor）：
 * 液体中应使用独立的 0.02 基准，而不是空中的 m_jumpMovementFactor；水中还要叠加
 * WATER_MOVEMENT_EFFICIENCY（深海探索者）向 MOVEMENT_SPEED 的插值。
 *
 * 该行为此前缺失：非玩家生物在水中用的是空中因子，深海探索者只影响速度衰减、
 * 不影响输入放大，导致附魔对生物完全无效。
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/attribute/AttributeInstance.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <gtest/gtest.h>

namespace mc::test {
namespace {

/**
 * @brief 最小可构造的生物，用于驱动 travel 的液体分支。
 */
class _WaterTravelMob final : public CreatureEntity {
public:
    _WaterTravelMob()
        : CreatureEntity(EntityInstanceId(1), testEcsRegistry())
    {
        // 构造期虚函数不派发，需显式再调一次以注册 MOB 层属性。
        registerAttributes();
        setHealth(maxHealth());
    }
};

/**
 * @brief 让实体以给定输入 travel 一次，返回水平速度增量。
 *
 * travel(strafing, vertical, forward) 中 strafing/forward 是原始输入，函数内部会归一化
 * 并乘以 moveFactor。这里固定 forward=1、yaw=0，于是水平速度增量就等于 moveFactor
 * 在 +X 方向上的投影（yaw=0 时 sinYaw=0、cosYaw=1，即 moveX = -0 → 实际为 forward 方向）。
 */
f32 _horizontalSpeedGainAfterTravel(_WaterTravelMob& mob)
{
    mob.setVelocity(0.0f, 0.0f, 0.0f);
    // 只给前进输入，yaw=0 时前进方向为 -Z（与 moveRelative 公式一致）。
    mob.travel(0.0f, 0.0f, 1.0f);
    const Vector3 velocity = mob.velocity();
    return std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
}

// ============================================================================
// 液体中的移动输入因子
// ============================================================================

/**
 * 水中默认（无深海探索者）的水平输入增量应等于 0.02 —— 即 SWIM_SPEED_BASE。
 *
 * 若实现错误地沿用空中因子 m_jumpMovementFactor（其默认值同样接近 0.02），这条断言
 * 未必能区分；因此配合下一条"深海探索者必须放大输入"一起构成契约。
 */
TEST(WaterMovementTest, InWaterUsesSwimSpeedBaseWithoutDepthStrider)
{
    _WaterTravelMob mob;
    mob.setInWater(true);

    const f32 gain = _horizontalSpeedGainAfterTravel(mob);

    // travel 先在速度上累加 moveFactor，随后才乘水中阻力，故净增量为两者之积。
    EXPECT_NEAR(gain, physics::SWIM_SPEED_BASE * physics::DRAG_WATER, 0.0005f);
}

/**
 * 深海探索者（WATER_MOVEMENT_EFFICIENCY）必须放大水中输入，而不只是降低阻力。
 *
 * 这是本次修复的核心：属性值 f2 会把输入从 0.02 向 MOVEMENT_SPEED 插值，
 * 因此同等输入下水平速度增量应显著大于无附魔的情形。
 */
TEST(WaterMovementTest, DepthStriderIncreasesWaterInputSpeed)
{
    _WaterTravelMob plain;
    plain.setInWater(true);
    const f32 plainGain = _horizontalSpeedGainAfterTravel(plain);

    _WaterTravelMob enchanted;
    enchanted.setInWater(true);
    // 直接写入属性基值，等价于深海探索者 III 满值。
    enchanted.attributes().setBaseValue(entity::attribute::Attributes::WATER_MOVEMENT_EFFICIENCY, 1.0);
    const f32 enchantedGain = _horizontalSpeedGainAfterTravel(enchanted);

    EXPECT_GT(enchantedGain, plainGain * 2.0f) << "depth strider must scale water input speed, not only reduce drag";
}

/**
 * 岩浆中使用固定的 0.02 输入，不响应深海探索者（岩浆没有对应附魔）。
 */
TEST(WaterMovementTest, LavaUsesSwimSpeedBaseAndIgnoresWaterEfficiency)
{
    _WaterTravelMob lava;
    lava.setInLava(true);
    lava.attributes().setBaseValue(entity::attribute::Attributes::WATER_MOVEMENT_EFFICIENCY, 1.0);

    const f32 gain = _horizontalSpeedGainAfterTravel(lava);

    // 岩浆同样先用常量输入、再乘岩浆阻力；深海探索者不影响这一路。
    EXPECT_NEAR(gain, physics::SWIM_SPEED_BASE * physics::DRAG_LAVA, 0.0005f);
}

} // namespace
} // namespace mc::test
