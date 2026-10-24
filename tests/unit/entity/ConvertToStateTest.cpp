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

// Entity::convertTo 状态复制对齐 vanilla ConversionType.SINGLE.convert + convertCommon 测试。
//
// 原版转化链路（Mob.java #convertTo → ConversionType.convert → convertCommon）会复制：
//   - Entity 级：位置/朝向/速度、身体朝向、摔落距离、落地与鞘翅滑翔标志、自定义名称及其
//     可见性、无敌、静默、无重力、传送冷却、剩余燃烧时间；
//   - 生物级（convertCommon）：吸收伤害、全部状态效果、受击时间、按参数复制的装备与
//     装备掉落率、属性、年龄、能否拾取战利品、AI 开关、持久化标记；
//   - 骑乘关系：乘客与载具迁移到新实体。
// 此前 convertTo 只复制位置/朝向/速度/自定义名称/无敌/静默，其余状态在转化时全部丢失。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/passive/basic/CowEntity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>

using namespace mc;

namespace {

/**
 * @brief 接管生成实体的测试世界
 *
 * spawnEntity 持有新实体的所有权，便于在 convertTo 之后检查目标实体的状态。
 */
class ConvertTestWorld final : public mc::test::BaseTestWorld {
public:
    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawned.push_back(std::move(entity));
        registerEntityForLookup(m_spawned.back().get());
        return m_spawned.back()->id();
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const
    {
        return m_spawned;
    }

private:
    std::vector<std::unique_ptr<Entity>> m_spawned;
};

} // namespace

class ConvertToStateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
        Items::initialize();
    }

    ConvertTestWorld m_world;
};

// Entity 级状态：位置/朝向/速度/身体朝向/摔落距离/自定义名称可见性/无敌/静默/无重力/
// 传送冷却/剩余燃烧时间全部带到新实体。
TEST_F(ConvertToStateTest, CopiesEntityLevelState)
{
    PigEntity pig(EntityInstanceId(101), mc::test::testEcsRegistry());
    pig.setWorld(&m_world);
    pig.setPosition(3.25f, 70.0f, -4.5f);
    pig.setRotation(35.0f, 12.0f);
    pig.setVelocity(Vector3(0.1f, -0.25f, 0.3f));
    pig.setYBodyRot(20.0f);
    pig.setFallDistance(7.5f);
    pig.setCustomName("Bacon");
    pig.setCustomNameVisible(true);
    pig.setInvulnerable(true);
    pig.setSilent(true);
    pig.setNoGravity(true);
    pig.setPortalCooldown(42);
    pig.setRemainingFireTicks(88);

    Entity* converted = pig.convertTo(entity::EntityTypeKeys::COW, ConversionParams::single(pig, false, false));
    ASSERT_NE(converted, nullptr);
    ASSERT_EQ(m_world.spawnedEntities().size(), 1U);

    EXPECT_FLOAT_EQ(converted->x(), 3.25f);
    EXPECT_FLOAT_EQ(converted->y(), 70.0f);
    EXPECT_FLOAT_EQ(converted->z(), -4.5f);
    EXPECT_FLOAT_EQ(converted->yaw(), 35.0f);
    EXPECT_FLOAT_EQ(converted->pitch(), 12.0f);
    EXPECT_FLOAT_EQ(converted->velocity().x, 0.1f);
    EXPECT_FLOAT_EQ(converted->velocity().y, -0.25f);
    EXPECT_FLOAT_EQ(converted->velocity().z, 0.3f);
    EXPECT_FLOAT_EQ(converted->yBodyRot(), 20.0f);
    EXPECT_FLOAT_EQ(converted->fallDistance(), 7.5f);
    EXPECT_EQ(converted->customNameText(), "Bacon");
    EXPECT_TRUE(converted->isCustomNameVisible());
    EXPECT_TRUE(converted->isInvulnerable());
    EXPECT_TRUE(converted->isSilent());
    EXPECT_TRUE(converted->hasNoGravity());
    EXPECT_EQ(converted->portalCooldown(), 42);
    EXPECT_EQ(converted->getRemainingFireTicks(), 88);

    // 原实体被静默移除。
    EXPECT_TRUE(pig.isRemoved());
    EXPECT_FALSE(pig.isAlive());
}

// 生物级状态：吸收伤害、状态效果与受击时间原样复制。
//
// MAX_ABSORPTION 默认值为 0（仅吸收药水提供 +4 修饰符），测试直接给源实体写入修饰符，
// 避免依赖效果系统的属性同步行为。
TEST_F(ConvertToStateTest, CopiesLivingState)
{
    CowEntity cow(EntityInstanceId(102), mc::test::testEcsRegistry());
    cow.setWorld(&m_world);
    cow.attributes().addModifier(entity::attribute::Attributes::MAX_ABSORPTION,
        entity::attribute::AttributeModifier("test.absorption", "test", 4.0, entity::attribute::Operation::Addition));
    cow.setAbsorptionAmount(2.0f);
    cow.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 100, 1));
    cow.setHurtTime(7);

    Entity* converted = cow.convertTo(entity::EntityTypeKeys::COW, ConversionParams::single(cow, false, false));
    ASSERT_NE(converted, nullptr);
    auto* convertedLiving = dynamic_cast<LivingEntity*>(converted);
    ASSERT_NE(convertedLiving, nullptr);

    EXPECT_FLOAT_EQ(cow.absorptionAmount(), 2.0f);
    EXPECT_FLOAT_EQ(convertedLiving->absorptionAmount(), 2.0f);
    EXPECT_EQ(convertedLiving->hurtTime(), 7);
    const entity::effect::EffectInstance* poison =
        convertedLiving->effectManager().getEffect(entity::effect::EffectType::Poison);
    ASSERT_NE(poison, nullptr) << "vanilla convertCommon copies every active effect";
    EXPECT_EQ(poison->amplifier(), 1);
}

// 幼体状态与 AI 开关、持久化标记、能否拾取战利品一并复制。
TEST_F(ConvertToStateTest, CopiesAgeAiAndPersistence)
{
    PigEntity baby(EntityInstanceId(105), mc::test::testEcsRegistry());
    baby.setWorld(&m_world);
    baby.setChild(true);
    baby.setNoAi(true);
    baby.enablePersistence();
    baby.setCanPickUpLoot(true);

    Entity* converted = baby.convertTo(entity::EntityTypeKeys::COW, ConversionParams::single(baby, false, true));
    ASSERT_NE(converted, nullptr);
    EXPECT_TRUE(converted->isChild());
    auto* convertedMob = dynamic_cast<MobEntity*>(converted);
    ASSERT_NE(convertedMob, nullptr);
    EXPECT_TRUE(convertedMob->isNoAi());
    EXPECT_TRUE(convertedMob->isNoDespawnRequired());
    EXPECT_TRUE(convertedMob->canPickUpLoot());
}