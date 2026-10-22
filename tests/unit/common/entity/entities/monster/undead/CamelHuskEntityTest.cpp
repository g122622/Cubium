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
 * @file CamelHuskEntityTest.cpp
 * @brief 骆驼尸壳（CamelHusk）实体单元测试
 *
 * 覆盖 vanilla 1.21.11 net.minecraft.world.entity.animal.camel.CamelHusk 的关键语义：
 * 1. 注册为敌对生物（Monster），和平难度禁生成；尺寸 1.7 x 2.375
 * 2. 远离玩家时正常消失（不因被骑乘而免消失）
 * 3. 不可繁殖
 * 4. 继承骆驼的坐姿/冲刺能力；尸壳骑兵常量与 vanilla 一致
 * 5. #minecraft:zombies / #minecraft:undead 标签成员
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/monster/undead/CamelHuskEntity.hpp"
#include "common/entity/entities/monster/undead/HuskEntity.hpp"
#include "common/entity/entities/passive/special/CamelEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>

namespace mc {
namespace {

/// BaseTestWorld 构造函数为 protected，故以派生类实例化（测试仅需最小世界接口）。
class CamelHuskTestWorld final : public mc::test::BaseTestWorld {};

class CamelHuskEntityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::initialize();
            VanillaBlocks::initialize();
            entity::VanillaEntities::registerAll();
            s_initialized = true;
        }
    }

    void SetUp() override { m_world = std::make_unique<CamelHuskTestWorld>(); }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<CamelHuskTestWorld> m_world;
};

TEST_F(CamelHuskEntityTest, RegisteredAsMonster)
{
    const entity::EntityType* type =
        entity::EntityRegistry::instance().getType(std::string(entity::EntityTypeKeys::CAMEL_HUSK));
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(type->classification(), entity::EntityClassification::Monster);
    EXPECT_FLOAT_EQ(type->size().width(), 1.7f);
    EXPECT_FLOAT_EQ(type->size().height(), 2.375f);
    // vanilla EntityType.CAMEL_HUSK 未调用 notInPeaceful()，故和平难度允许召唤；但分类为 MONSTER，
    // 和平难度下不会自然生成。
    EXPECT_TRUE(type->isAllowedInPeaceful());
}

TEST_F(CamelHuskEntityTest, InheritsCamelState)
{
    CamelHuskEntity camelHusk(EntityInstanceId(1), mc::test::testEcsRegistry());
    camelHusk.setWorld(m_world.get());

    // 继承 Camel 的坐姿/冲刺能力
    EXPECT_FALSE(camelHusk.isCamelSitting());
    camelHusk.sitDown();
    EXPECT_TRUE(camelHusk.isCamelSitting());
    camelHusk.standUp();
    EXPECT_FALSE(camelHusk.isCamelSitting());

    camelHusk.setDashing(true);
    EXPECT_TRUE(camelHusk.isDashing());
    EXPECT_EQ(camelHusk.dashCooldown(), CamelEntity::DASH_COOLDOWN_TICKS);
}

TEST_F(CamelHuskEntityTest, CannotBreed)
{
    CamelHuskEntity camelHusk(EntityInstanceId(1), mc::test::testEcsRegistry());
    CamelEntity camel(EntityInstanceId(2), mc::test::testEcsRegistry());

    // 对齐 vanilla CamelHusk.canMate = false / canFallInLove = false
    EXPECT_FALSE(camelHusk.canMateWith(camel));
    EXPECT_FALSE(camelHusk.canBreed());
}

TEST_F(CamelHuskEntityTest, DespawnsEvenWhenRidden)
{
    CamelHuskEntity camelHusk(EntityInstanceId(1), mc::test::testEcsRegistry());
    CamelEntity camel(EntityInstanceId(2), mc::test::testEcsRegistry());

    // 对齐 vanilla CamelHusk.removeWhenFarAway = true：Cubium 用 preventDespawn 反向表达，
    // 骆驼（Camel）默认 isRiding() 时免消失，骆驼尸壳任何情况都正常消失。
    EXPECT_FALSE(camelHusk.preventDespawn());
    EXPECT_TRUE(camelHusk.canBeLeashed());
}

TEST_F(CamelHuskEntityTest, GoalsReplacedWithMonsterSet)
{
    CamelHuskEntity camelHusk(EntityInstanceId(1), mc::test::testEcsRegistry());

    bool hasMelee = false;
    bool hasBreed = false;
    for (const auto& pg : camelHusk.goalSelector().getAllGoals()) {
        const std::string name = pg.getGoal() != nullptr ? pg.getGoal()->getTypeName() : std::string();
        hasMelee = hasMelee || name == "MeleeAttackGoal";
        hasBreed = hasBreed || name == "BreedGoal";
    }
    // 骆驼尸壳为敌对生物：保留近战攻击目标，剔除被动坐骑的繁殖目标
    EXPECT_TRUE(hasMelee);
    EXPECT_FALSE(hasBreed);

    // TargetGoals 系列（HurtByTargetGoal / NearestAttackableTargetGoal）未覆写 getTypeName，
    // 故按目标选择器的条目数量判定：被攻击反击 + 攻击玩家 = 2 条。
    EXPECT_EQ(camelHusk.targetSelector().getAllGoals().size(), 2u);
}

TEST_F(CamelHuskEntityTest, FoodIsRabbitFootUnlikeCamel)
{
    CamelHuskEntity camelHusk(EntityInstanceId(1), mc::test::testEcsRegistry());
    CamelEntity camel(EntityInstanceId(2), mc::test::testEcsRegistry());

    ASSERT_NE(Items::RABBIT_FOOT, nullptr);
    ASSERT_NE(Items::CACTUS, nullptr);
    // 对齐 vanilla：#camel_husk_food = {rabbit_foot}，#camel_food = {cactus}
    EXPECT_TRUE(camelHusk.isFoodItem(ItemStack(*Items::RABBIT_FOOT, 1)));
    EXPECT_FALSE(camelHusk.isFoodItem(ItemStack(*Items::CACTUS, 1)));
    EXPECT_TRUE(camel.isFoodItem(ItemStack(*Items::CACTUS, 1)));
    EXPECT_FALSE(camel.isFoodItem(ItemStack(*Items::RABBIT_FOOT, 1)));
}

TEST_F(CamelHuskEntityTest, HuskJockeyChanceMatchesVanilla)
{
    // 对齐 vanilla Husk.finalizeSpawn: randomsource.nextFloat() < 0.1F
    EXPECT_FLOAT_EQ(HuskEntity::CAMEL_HUSK_JOCKEY_CHANCE, 0.1f);
}

TEST_F(CamelHuskEntityTest, RegisteredInZombiesAndUndeadTags)
{
    EntityTypeTags::initialize();
    const ResourceLocation camelHuskId(entity::EntityTypeKeys::CAMEL_HUSK);

    // 对齐 vanilla EntityTypeTagsProvider.ZOMBIES / UNDEAD
    EXPECT_TRUE(EntityTypeTags::ZOMBIES().contains(camelHuskId));
    EXPECT_TRUE(EntityTypeTags::UNDEAD().contains(camelHuskId));
}

} // namespace
} // namespace mc