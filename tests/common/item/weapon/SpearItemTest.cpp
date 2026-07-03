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
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/items/weapon/SpearItem.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/item/tier/ItemTiers.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

namespace mc {
namespace {

/**
 * @brief 测试用世界存根
 */
class SpearTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SpearTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SpearTestWorld::tickManager not implemented");
    }

    [[nodiscard]] EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(entity.get());
        m_ownedEntities.push_back(std::move(entity));
        return ++m_lastEntityId;
    }

    void addParticle(
        particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3& = Vector3(0, 0, 0), u32 = 1) override
    {
        // 测试中忽略粒子效果
    }

    [[nodiscard]] const std::vector<Entity*>& spawnedEntities() const { return m_spawnedEntities; }

private:
    EntityId m_lastEntityId = 0;
    std::vector<Entity*> m_spawnedEntities;
    std::vector<std::unique_ptr<Entity>> m_ownedEntities;
};

class SpearItemTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        item::tag::ItemTags::initialize();
    }

    SpearTestWorld m_world;
};

// ============================================================================
// 长矛注册与基础属性测试
// ============================================================================

TEST_F(SpearItemTest, AllSpearsRegistered)
{
    ASSERT_NE(Items::WOODEN_SPEAR, nullptr);
    ASSERT_NE(Items::STONE_SPEAR, nullptr);
    ASSERT_NE(Items::COPPER_SPEAR, nullptr);
    ASSERT_NE(Items::IRON_SPEAR, nullptr);
    ASSERT_NE(Items::GOLDEN_SPEAR, nullptr);
    ASSERT_NE(Items::DIAMOND_SPEAR, nullptr);
    ASSERT_NE(Items::NETHERITE_SPEAR, nullptr);
}

TEST_F(SpearItemTest, WoodenSpear_HasCorrectDurability)
{
    // 木长矛耐久度 = 木层级耐久 = 59
    EXPECT_EQ(Items::WOODEN_SPEAR->maxDamage(), 59);
}

TEST_F(SpearItemTest, StoneSpear_HasCorrectDurability)
{
    // 石长矛耐久度 = 石层级耐久 = 131
    EXPECT_EQ(Items::STONE_SPEAR->maxDamage(), 131);
}

TEST_F(SpearItemTest, CopperSpear_HasCorrectDurability)
{
    // 铜长矛耐久度 = 铜层级耐久 = 190
    EXPECT_EQ(Items::COPPER_SPEAR->maxDamage(), 190);
}

TEST_F(SpearItemTest, IronSpear_HasCorrectDurability)
{
    // 铁长矛耐久度 = 铁层级耐久 = 250
    EXPECT_EQ(Items::IRON_SPEAR->maxDamage(), 250);
}

TEST_F(SpearItemTest, GoldenSpear_HasCorrectDurability)
{
    // 金长矛耐久度 = 金层级耐久 = 32
    EXPECT_EQ(Items::GOLDEN_SPEAR->maxDamage(), 32);
}

TEST_F(SpearItemTest, DiamondSpear_HasCorrectDurability)
{
    // 钻石长矛耐久度 = 钻石层级耐久 = 1561
    EXPECT_EQ(Items::DIAMOND_SPEAR->maxDamage(), 1561);
}

TEST_F(SpearItemTest, NetheriteSpear_HasCorrectDurability)
{
    // 下界合金长矛耐久度 = 下界合金层级耐久 = 2031
    EXPECT_EQ(Items::NETHERITE_SPEAR->maxDamage(), 2031);
}

// ============================================================================
// UseAction 和 UseDuration 测试
// ============================================================================

TEST_F(SpearItemTest, GetUseAction_ReturnsSpear)
{
    auto* spear = dynamic_cast<const item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spear, nullptr);

    ItemStack stack(Items::IRON_SPEAR, 1);
    EXPECT_EQ(spear->getUseAction(stack), UseAction::Spear);
}

TEST_F(SpearItemTest, GetUseDuration_Returns72000)
{
    auto* spear = dynamic_cast<const item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spear, nullptr);

    ItemStack stack(Items::IRON_SPEAR, 1);
    EXPECT_EQ(spear->getUseDuration(stack), 72000);
}

// ============================================================================
// 攻击伤害测试
// ============================================================================

TEST_F(SpearItemTest, WoodenSpear_AttackDamage)
{
    // 木长矛攻击伤害 = 基础值(3) + 木层级加成(0.0) = 3.0
    auto* spear = dynamic_cast<const item::SpearItem*>(Items::WOODEN_SPEAR);
    ASSERT_NE(spear, nullptr);
    EXPECT_FLOAT_EQ(spear->getAttackDamage(), 3.0f);
}

TEST_F(SpearItemTest, StoneSpear_AttackDamage)
{
    // 石长矛攻击伤害 = 基础值(3) + 石层级加成(1.0) = 4.0
    auto* spear = dynamic_cast<const item::SpearItem*>(Items::STONE_SPEAR);
    ASSERT_NE(spear, nullptr);
    EXPECT_FLOAT_EQ(spear->getAttackDamage(), 4.0f);
}

TEST_F(SpearItemTest, IronSpear_AttackDamage)
{
    // 铁长矛攻击伤害 = 基础值(3) + 铁层级加成(2.0) = 5.0
    auto* spear = dynamic_cast<const item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spear, nullptr);
    EXPECT_FLOAT_EQ(spear->getAttackDamage(), 5.0f);
}

TEST_F(SpearItemTest, DiamondSpear_AttackDamage)
{
    // 钻石长矛攻击伤害 = 基础值(3) + 钻石层级加成(3.0) = 6.0
    auto* spear = dynamic_cast<const item::SpearItem*>(Items::DIAMOND_SPEAR);
    ASSERT_NE(spear, nullptr);
    EXPECT_FLOAT_EQ(spear->getAttackDamage(), 6.0f);
}

TEST_F(SpearItemTest, NetheriteSpear_AttackDamage)
{
    // 下界合金长矛攻击伤害 = 基础值(3) + 下界合金层级加成(4.0) = 7.0
    auto* spear = dynamic_cast<const item::SpearItem*>(Items::NETHERITE_SPEAR);
    ASSERT_NE(spear, nullptr);
    EXPECT_FLOAT_EQ(spear->getAttackDamage(), 7.0f);
}

// ============================================================================
// 层级与修复材料测试
// ============================================================================

TEST_F(SpearItemTest, GetTier_ReturnsCorrectTier)
{
    auto* ironSpear = dynamic_cast<const item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(ironSpear, nullptr);
    EXPECT_EQ(&ironSpear->getTier(), &item::tier::ItemTiers::IRON());
}

TEST_F(SpearItemTest, GetItemEnchantability_ReturnsTierEnchantability)
{
    // 金长矛附魔能力 = 22（金层级）
    EXPECT_EQ(Items::GOLDEN_SPEAR->getItemEnchantability(), 22);
}

// ============================================================================
// SPEARS 物品标签测试
// ============================================================================

TEST_F(SpearItemTest, SpearsTag_ContainsAllSpears)
{
    auto& spearsTag = item::tag::ItemTags::SPEARS();
    EXPECT_TRUE(spearsTag.contains(Items::WOODEN_SPEAR));
    EXPECT_TRUE(spearsTag.contains(Items::STONE_SPEAR));
    EXPECT_TRUE(spearsTag.contains(Items::COPPER_SPEAR));
    EXPECT_TRUE(spearsTag.contains(Items::IRON_SPEAR));
    EXPECT_TRUE(spearsTag.contains(Items::GOLDEN_SPEAR));
    EXPECT_TRUE(spearsTag.contains(Items::DIAMOND_SPEAR));
    EXPECT_TRUE(spearsTag.contains(Items::NETHERITE_SPEAR));
}

TEST_F(SpearItemTest, SpearsTag_DoesNotContainTrident)
{
    auto& spearsTag = item::tag::ItemTags::SPEARS();
    EXPECT_FALSE(spearsTag.contains(Items::TRIDENT));
}

TEST_F(SpearItemTest, GoldenSpear_InPiglinLovedTag)
{
    // 金长矛属于 piglin_loved 标签
    auto& piglinLoved = item::tag::ItemTags::PIGLIN_LOVED();
    EXPECT_TRUE(piglinLoved.contains(Items::GOLDEN_SPEAR));
}

} // namespace
} // namespace mc
