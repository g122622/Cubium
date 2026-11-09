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
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/explosion/ExplosionMode.hpp"

namespace mc {

/// @brief 只提供 game rules 的世界桩：本组测试不涉及方块查询。
class BlocklikeEntitiesWorld final : public mc::test::BaseTestWorld {
public:
    BlocklikeEntitiesWorld() = default;
};

/**
 * @brief 验证 Explosion::_shouldAffectBlocklikeEntities 的完整判定
 *
 * 对齐 ServerExplosion.shouldAffectBlocklikeEntities：
 *   flag  = mobGriefing
 *   flag1 = source == null || source 不是风弹
 *   return flag ? flag1 : shouldAffectBlocklikeEntities && flag1
 *
 * 关键点：风弹作为爆源时，无论 mobGriefing 如何都不波及其他方块类实体。
 *
 * 类名必须是 mc::ExplosionBlocklikeEntitiesTest：Explosion 内以
 * `friend class ::mc::ExplosionBlocklikeEntitiesTest` 授予私有访问。
 */
class ExplosionBlocklikeEntitiesTest : public ::testing::Test {
protected:
    void setMobGriefing(bool value)
    {
        m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, value, nullptr);
    }

    /// @brief 用指定模式与爆源构造后直接查询判定结果。
    static bool shouldAffect(IWorld& world, world::explosion::ExplosionMode mode, Entity* source)
    {
        world::explosion::Explosion explosion(world, Vector3(0.5, 64.0, 0.5), 4.0f, mode, false, source);
        return explosion._shouldAffectBlocklikeEntities();
    }

    BlocklikeEntitiesWorld m_world;
};

// mobGriefing 开启：无爆源时影响方块类实体。
TEST_F(ExplosionBlocklikeEntitiesTest, MobGriefingEnabledWithoutSourceReturnsTrue)
{
    setMobGriefing(true);
    EXPECT_TRUE(shouldAffect(m_world, world::explosion::ExplosionMode::None, nullptr));
    EXPECT_TRUE(shouldAffect(m_world, world::explosion::ExplosionMode::Break, nullptr));
}

// mobGriefing 关闭：只有破坏方块的爆炸才影响方块类实体。
TEST_F(ExplosionBlocklikeEntitiesTest, MobGriefingDisabledDependsOnMode)
{
    setMobGriefing(false);
    EXPECT_FALSE(shouldAffect(m_world, world::explosion::ExplosionMode::None, nullptr))
        << "non-destructive explosion must not affect blocklike entities when mobGriefing is off";
    EXPECT_TRUE(shouldAffect(m_world, world::explosion::ExplosionMode::Break, nullptr));
    EXPECT_TRUE(shouldAffect(m_world, world::explosion::ExplosionMode::Destroy, nullptr));
}

// 非风弹爆源不触发排除：保证排除条件按 typeId 生效而不是恒真。
TEST_F(ExplosionBlocklikeEntitiesTest, NonWindChargeSourceIsNotExcluded)
{
    setMobGriefing(true);
    Entity plainEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    EXPECT_TRUE(shouldAffect(m_world, world::explosion::ExplosionMode::None, &plainEntity))
        << "a non-wind-charge source must not be excluded";
}

} // namespace mc