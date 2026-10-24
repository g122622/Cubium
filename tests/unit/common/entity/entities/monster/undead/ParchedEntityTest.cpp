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

/**
 * @file ParchedEntityTest.cpp
 * @brief 干枯骷髅（Parched）实体单元测试
 *
 * 覆盖 vanilla 1.21.11 net.minecraft.world.entity.monster.skeleton.Parched 的关键语义：
 * 1. 生命值 16（普通骷髅 20），亡灵归类
 * 2. 弓远程攻击：构造期默认持弓、射出的箭附带 30 秒虚弱 I
 * 3. 攻击间隔比普通骷髅慢：困难 50 / 非困难 70
 * 4. 免疫虚弱效果（canBeAffected 对 WEAKNESS 恒 false）
 * 5. 注册与标签：#minecraft:skeletons / #minecraft:undead 成员
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/monster/undead/AbstractSkeletonEntity.hpp"
#include "common/entity/entities/monster/undead/ParchedEntity.hpp"
#include "common/entity/entities/monster/undead/SkeletonEntity.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>

namespace mc {
namespace {

/// BaseTestWorld 构造函数为 protected，故以派生类实例化（测试仅需最小世界接口）。
class ParchedTestWorld final : public mc::test::BaseTestWorld {};

class ParchedEntityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::ensureInitialized();
            VanillaBlocks::initialize();
            entity::VanillaEntities::registerAll();
            s_initialized = true;
        }
    }

    void SetUp() override
    {
        m_world = std::make_unique<ParchedTestWorld>();
    }

    void TearDown() override
    {
        m_world.reset();
    }

    std::unique_ptr<ParchedTestWorld> m_world;
};

TEST_F(ParchedEntityTest, Attributes_MaxHealthIsSixteen)
{
    ParchedEntity parched(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 对齐原版 Parched.createAttributes().add(MAX_HEALTH, 16.0)
    EXPECT_FLOAT_EQ(parched.maxHealth(), ParchedEntity::PARCHED_MAX_HEALTH);
    EXPECT_FLOAT_EQ(parched.maxHealth(), 16.0f);
    // 继承 AbstractSkeleton 的移速 0.25（createAttributes 只覆盖生命值）
    EXPECT_NEAR(parched.attributes().getValue(entity::attribute::Attributes::MOVEMENT_SPEED), 0.25, 1e-6);
}

TEST_F(ParchedEntityTest, CreatureAttribute_IsUndead)
{
    ParchedEntity parched(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(parched.getCreatureAttribute(), CreatureAttribute::Undead);
}

TEST_F(ParchedEntityTest, DefaultMainHand_IsBow)
{
    ParchedEntity parched(EntityInstanceId(1), mc::test::testEcsRegistry());
    ASSERT_NE(Items::BOW, nullptr);
    // setCombatTask 依赖主手持弓选择远程攻击目标（不持弓会退化为近战）
    EXPECT_EQ(parched.getEquipment(EquipmentSlot::MainHand).getItem(), Items::BOW);
}

TEST_F(ParchedEntityTest, CustomizeArrow_AppliesWeaknessThirtySeconds)
{
    ParchedEntity parched(EntityInstanceId(1), mc::test::testEcsRegistry());
    entity::ArrowEntity arrow(EntityInstanceId(2), mc::test::testEcsRegistry());

    parched.customizeArrow(arrow);

    // 对齐原版 Parched.getArrow()：arrow.addEffect(new MobEffectInstance(WEAKNESS, 600))
    const auto& effects = arrow.effects();
    ASSERT_EQ(effects.size(), 1u);
    EXPECT_EQ(effects[0].type(), entity::effect::EffectType::Weakness);
    EXPECT_EQ(effects[0].duration(), 600);
    EXPECT_EQ(effects[0].amplifier(), 0);
    EXPECT_EQ(ParchedEntity::WEAKNESS_DURATION_TICKS, 600);
}

TEST_F(ParchedEntityTest, AttackIntervals_AreSlowerThanSkeleton)
{
    ParchedEntity parched(EntityInstanceId(1), mc::test::testEcsRegistry());
    SkeletonEntity skeleton(EntityInstanceId(2), mc::test::testEcsRegistry());

    // 对齐原版 Parched.getHardAttackInterval()=50 / getAttackInterval()=70
    EXPECT_EQ(parched.getHardAttackInterval(), 50);
    EXPECT_EQ(parched.getAttackInterval(), 70);
    // 普通骷髅为 20 / 40
    EXPECT_EQ(skeleton.getHardAttackInterval(), 20);
    EXPECT_EQ(skeleton.getAttackInterval(), 40);
}

TEST_F(ParchedEntityTest, IsPotionApplicable_RejectsWeakness)
{
    ParchedEntity parched(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 对齐原版 Parched.canBeAffected：WEAKNESS → false
    EXPECT_FALSE(
        parched.isPotionApplicable(entity::effect::EffectInstance(entity::effect::EffectType::Weakness, 600, 0)));
    // 其它效果仍走基类判定（存活亡灵不被通用规则拒绝）
    EXPECT_TRUE(parched.isPotionApplicable(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 100, 0)));
}

TEST_F(ParchedEntityTest, RegisteredWithMonsterClassification)
{
    const entity::EntityType* type =
        entity::EntityRegistry::instance().getType(std::string(entity::EntityTypeKeys::PARCHED));
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(type->classification(), entity::EntityClassification::Monster);
    EXPECT_FLOAT_EQ(type->size().width(), 0.6f);
    EXPECT_FLOAT_EQ(type->size().height(), 1.99f);
    EXPECT_FALSE(type->isAllowedInPeaceful());
}

TEST_F(ParchedEntityTest, RegisteredInSkeletonsAndUndeadTags)
{
    EntityTypeTags::initialize();
    const ResourceLocation parchedId(entity::EntityTypeKeys::PARCHED);

    // 对齐 vanilla EntityTypeTagsProvider.SKELETONS / UNDEAD
    EXPECT_TRUE(EntityTypeTags::SKELETONS().contains(parchedId));
    EXPECT_TRUE(EntityTypeTags::UNDEAD().contains(parchedId));
}

} // namespace
} // namespace mc