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
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/illager/VexEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>

namespace mc {
namespace {

/**
 * @brief 测试用世界实现
 *
 * 提供 VexEntity 测试所需的最小 IWorld 接口实现
 */
class VexTestWorld final : public mc::test::BaseTestWorld {
public:
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    EntityInstanceId spawnEntity(std::unique_ptr<Entity>) override { return EntityInstanceId(0); }

    void advanceTick() { m_currentTick++; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("VexTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("VexTestWorld::tickManager not implemented");
    }

private:
    u64 m_currentTick = 0;
};

} // namespace

// ============================================================================
// VexEntity 基础测试
// ============================================================================

TEST(VexEntityTest, Construction)
{
    // 使用 Unknown 类型，因为 Vex 没有在 LegacyEntityType 中定义
    VexEntity vex(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 验证恼鬼尺寸
    EXPECT_FLOAT_EQ(vex.width(), 0.4f);
    EXPECT_FLOAT_EQ(vex.height(), 0.8f);
    EXPECT_FLOAT_EQ(vex.eyeHeight(), 0.4f);

    // 验证默认生命时间
    EXPECT_TRUE(vex.hasLimitedLife());
    EXPECT_EQ(vex.getLifeTime(), 2400);

    // 验证默认状态
    EXPECT_FALSE(vex.isCharging());
    EXPECT_EQ(vex.getOwner(), nullptr);
}

TEST(VexEntityTest, LifeTimeSettings)
{
    VexEntity vex(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 测试生命时间设置
    vex.setLifeTime(100);
    EXPECT_EQ(vex.getLifeTime(), 100);

    // 测试有限生命设置
    vex.setLimitedLife(false);
    EXPECT_FALSE(vex.hasLimitedLife());

    vex.setLimitedLife(true);
    EXPECT_TRUE(vex.hasLimitedLife());
}

TEST(VexEntityTest, OwnerSettings)
{
    VexEntity vex(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 恼鬼可以设置主人（唤魔者）
    // 注意：这里使用 nullptr 作为测试，实际应该使用 EvokerEntity
    EXPECT_EQ(vex.getOwner(), nullptr);

    // 设置主人后应该能获取
    // 在实际游戏中，owner 是唤魔者
    vex.setOwner(nullptr);
    EXPECT_EQ(vex.getOwner(), nullptr);
}

TEST(VexEntityTest, ChargingState)
{
    VexEntity vex(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 默认不充电
    EXPECT_FALSE(vex.isCharging());

    // 设置充电状态
    vex.setCharging(true);
    EXPECT_TRUE(vex.isCharging());

    vex.setCharging(false);
    EXPECT_FALSE(vex.isCharging());
}

TEST(VexEntityTest, Attributes)
{
    VexEntity vex(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 注册属性后检查
    // VexEntity::registerAttributes() 在构造函数中被调用
    // 最大生命值 14.0 (MC 1.16.5)
    // 攻击伤害 4.0 (铁剑伤害)
    EXPECT_FLOAT_EQ(static_cast<f32>(vex.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH)), 14.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(vex.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE)), 4.0f);
}

TEST(VexEntityTest, NoClipDuringTick)
{
    // 这个测试验证 noClip 标志的正确设置
    // 注意：tick() 需要完整的世界初始化，这里只测试标志的设置和获取
    VexEntity vex(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 默认没有 noClip
    EXPECT_FALSE(vex.noClip());

    // 手动设置 noClip
    vex.setNoClip(true);
    EXPECT_TRUE(vex.noClip());

    // 手动清除 noClip
    vex.setNoClip(false);
    EXPECT_FALSE(vex.noClip());
}

TEST(VexEntityTest, NoGravitySetting)
{
    // 这个测试验证 noGravity 标志的正确设置
    VexEntity vex(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 设置 noGravity
    vex.setNoGravity(true);
    EXPECT_TRUE(vex.hasNoGravity());

    // 清除 noGravity
    vex.setNoGravity(false);
    EXPECT_FALSE(vex.hasNoGravity());
}

TEST(VexEntityTest, LifeTimeDecreases)
{
    // 注意：m_limitedLife 和 m_lifeTime 是私有成员
    // 这里只测试公共接口
    VexEntity vex(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 测试生命时间设置
    vex.setLifeTime(100);
    EXPECT_EQ(vex.getLifeTime(), 100);

    vex.setLifeTime(50);
    EXPECT_EQ(vex.getLifeTime(), 50);

    // 测试有限生命设置
    vex.setLimitedLife(true);
    EXPECT_TRUE(vex.hasLimitedLife());

    vex.setLimitedLife(false);
    EXPECT_FALSE(vex.hasLimitedLife());
}

TEST(VexEntityTest, CanFly)
{
    VexEntity vex(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 恼鬼可以飞行
    EXPECT_TRUE(vex.canFly());
}

TEST(VexEntityTest, ShouldNotBurnInDaylight)
{
    VexEntity vex(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 恼鬼不会在日光下燃烧
    EXPECT_FALSE(vex.shouldBurnInDaylight());
}

TEST(VexEntityTest, CreateFactory)
{
    auto entity = VexEntity::create(nullptr, mc::test::testEcsRegistry());
    ASSERT_NE(entity, nullptr);

    // 验证创建的是 VexEntity
    auto* vexPtr = dynamic_cast<VexEntity*>(entity.get());
    EXPECT_NE(vexPtr, nullptr);
}

} // namespace mc
