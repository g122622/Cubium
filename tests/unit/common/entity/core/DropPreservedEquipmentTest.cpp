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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, either EXPRESS OR
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;

namespace {

// 测试用 MobEntity 子类
class TestMobEntity : public MobEntity {
public:
    TestMobEntity()
        : MobEntity(EntityInstanceId(1), mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// 测试用世界实现，支持实体生成
class DropEquipmentTestWorld final : public mc::test::BaseTestWorld {
public:
    DropEquipmentTestWorld() { Items::initialize(); }

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty d) { m_difficulty = d; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size() + 100);
    }

    // 获取生成的 ItemEntity 数量
    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

private:
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    Difficulty m_difficulty = Difficulty::Normal;
};

class DropPreservedEquipmentTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::initialize();
            VanillaBlocks::initialize();
            s_initialized = true;
        }
    }

    void SetUp() override
    {
        m_world = std::make_unique<DropEquipmentTestWorld>();
        m_entity = std::make_unique<TestMobEntity>();
        m_entity->setWorld(m_world.get());
        m_entity->setPosition(0.0, 64.0, 0.0);
    }

    std::unique_ptr<DropEquipmentTestWorld> m_world;
    std::unique_ptr<TestMobEntity> m_entity;
};

// ============================================================================
// dropPreservedEquipment 基础测试
// ============================================================================

TEST_F(DropPreservedEquipmentTest, EmptySlotsReturnNoPreservedSlots)
{
    // 没有装备时，应返回空集合
    auto preservedSlots = m_entity->dropPreservedEquipment();
    EXPECT_TRUE(preservedSlots.empty());
}

TEST_F(DropPreservedEquipmentTest, EmptySlotsWithPredicateReturnNoPreservedSlots)
{
    auto preservedSlots = m_entity->dropPreservedEquipment([](const ItemStack&) { return true; });
    EXPECT_TRUE(preservedSlots.empty());
}

TEST_F(DropPreservedEquipmentTest, NonPreservedEquipmentRemainsOnEntity)
{
    // 默认掉落概率 0.085 的装备，谓词返回 true 时保留在实体上（不处理）
    // 对应 MC Java 的行为：dropPreservedEquipment 只掉落保留装备，
    // 非保留装备留在实体上，由调用者决定后续处理
    ItemStack sword(Items::IRON_SWORD, 1);
    m_entity->setEquipment(EquipmentSlot::MainHand, sword);

    // 默认掉落概率为 0.085，不保留
    EXPECT_FALSE(m_entity->isEquipmentDropPreserved(EquipmentSlot::MainHand));

    auto preservedSlots = m_entity->dropPreservedEquipment();

    // 非保留装备不应在保留槽位中
    EXPECT_TRUE(preservedSlots.empty());

    // 装备仍在实体上（dropPreservedEquipment 不清空非保留装备）
    EXPECT_FALSE(m_entity->getEquipment(EquipmentSlot::MainHand).isEmpty());
}

TEST_F(DropPreservedEquipmentTest, PreservedEquipmentDroppedOnGround)
{
    // 保留状态（掉落概率 > 1.0）的装备应掉落在地上
    ItemStack sword(Items::IRON_SWORD, 1);
    m_entity->setEquipment(EquipmentSlot::MainHand, sword);
    m_entity->setGuaranteedDrop(EquipmentSlot::MainHand);

    EXPECT_TRUE(m_entity->isEquipmentDropPreserved(EquipmentSlot::MainHand));

    size_t entityCountBefore = m_world->spawnedEntityCount();
    auto preservedSlots = m_entity->dropPreservedEquipment();

    // 保留装备不应在保留槽位中（已掉落）
    EXPECT_TRUE(preservedSlots.empty());

    // 装备槽位应被清空（已掉落）
    EXPECT_TRUE(m_entity->getEquipment(EquipmentSlot::MainHand).isEmpty());

    // 应生成了一个物品实体
    EXPECT_EQ(m_world->spawnedEntityCount(), entityCountBefore + 1);
}

TEST_F(DropPreservedEquipmentTest, PredicateFalseReturnsPreservedSlot)
{
    // 谓词返回 false 的装备应保留在实体上，槽位返回
    ItemStack sword(Items::IRON_SWORD, 1);
    sword.addEnchantment("minecraft:binding_curse", 1);
    m_entity->setEquipment(EquipmentSlot::MainHand, sword);

    // 使用绑定诅咒谓词：有绑定诅咒的物品谓词返回 false
    auto noBindingCurse = [](const ItemStack& stack) -> bool {
        return !item::enchant::EnchantmentHelper::hasBindingCurse(stack);
    };

    auto preservedSlots = m_entity->dropPreservedEquipment(noBindingCurse);

    // 绑定诅咒装备应在保留槽位中
    ASSERT_EQ(preservedSlots.size(), 1u);
    EXPECT_EQ(preservedSlots[0], EquipmentSlot::MainHand);

    // 装备仍在实体上（未被移除或掉落）
    EXPECT_FALSE(m_entity->getEquipment(EquipmentSlot::MainHand).isEmpty());
    EXPECT_TRUE(item::enchant::EnchantmentHelper::hasBindingCurse(m_entity->getEquipment(EquipmentSlot::MainHand)));
}

TEST_F(DropPreservedEquipmentTest, MixedEquipmentHandlesAllSlotsCorrectly)
{
    // 混合场景：绑定诅咒装备、保留装备、非保留装备
    ItemStack cursedHelmet(Items::IRON_HELMET, 1);
    cursedHelmet.addEnchantment("minecraft:binding_curse", 1);
    m_entity->setEquipment(EquipmentSlot::Head, cursedHelmet);

    ItemStack preservedSword(Items::IRON_SWORD, 1);
    m_entity->setEquipment(EquipmentSlot::MainHand, preservedSword);
    m_entity->setGuaranteedDrop(EquipmentSlot::MainHand);

    ItemStack normalChestplate(Items::IRON_CHESTPLATE, 1);
    m_entity->setEquipment(EquipmentSlot::Chest, normalChestplate);
    // 默认 0.085 掉落概率

    auto noBindingCurse = [](const ItemStack& stack) -> bool {
        return !item::enchant::EnchantmentHelper::hasBindingCurse(stack);
    };

    size_t entityCountBefore = m_world->spawnedEntityCount();
    auto preservedSlots = m_entity->dropPreservedEquipment(noBindingCurse);

    // 仅绑定诅咒的头盔在保留槽位中
    ASSERT_EQ(preservedSlots.size(), 1u);
    EXPECT_EQ(preservedSlots[0], EquipmentSlot::Head);

    // 绑定诅咒的头盔仍在实体上
    EXPECT_FALSE(m_entity->getEquipment(EquipmentSlot::Head).isEmpty());

    // 保留的主手剑已掉落（槽位清空）
    EXPECT_TRUE(m_entity->getEquipment(EquipmentSlot::MainHand).isEmpty());

    // 非保留的胸甲仍在实体上（dropPreservedEquipment 不清空非保留装备）
    EXPECT_FALSE(m_entity->getEquipment(EquipmentSlot::Chest).isEmpty());

    // 应生成了一个物品实体（主手剑）
    EXPECT_EQ(m_world->spawnedEntityCount(), entityCountBefore + 1);
}

TEST_F(DropPreservedEquipmentTest, MultipleBindingCurseItemsAllPreserved)
{
    // 多个绑定诅咒装备
    ItemStack cursedHelmet(Items::IRON_HELMET, 1);
    cursedHelmet.addEnchantment("minecraft:binding_curse", 1);
    m_entity->setEquipment(EquipmentSlot::Head, cursedHelmet);

    ItemStack cursedBoots(Items::IRON_BOOTS, 1);
    cursedBoots.addEnchantment("minecraft:binding_curse", 1);
    m_entity->setEquipment(EquipmentSlot::Feet, cursedBoots);

    auto noBindingCurse = [](const ItemStack& stack) -> bool {
        return !item::enchant::EnchantmentHelper::hasBindingCurse(stack);
    };

    auto preservedSlots = m_entity->dropPreservedEquipment(noBindingCurse);

    // 两个绑定诅咒装备都在保留槽位中
    ASSERT_EQ(preservedSlots.size(), 2u);

    // 头盔和靴子仍在实体上
    EXPECT_FALSE(m_entity->getEquipment(EquipmentSlot::Head).isEmpty());
    EXPECT_FALSE(m_entity->getEquipment(EquipmentSlot::Feet).isEmpty());
}

TEST_F(DropPreservedEquipmentTest, DropChanceZeroRemainsOnEntity)
{
    // 掉落概率为 0 的装备（如万圣节南瓜头）：谓词返回 true 但非保留，留在实体上
    ItemStack pumpkin(Items::CARVED_PUMPKIN, 1);
    m_entity->setEquipment(EquipmentSlot::Head, pumpkin);
    m_entity->setEquipmentDropChance(EquipmentSlot::Head, 0.0f);

    EXPECT_FALSE(m_entity->isEquipmentDropPreserved(EquipmentSlot::Head));

    auto preservedSlots = m_entity->dropPreservedEquipment();

    // 不在保留槽位中
    EXPECT_TRUE(preservedSlots.empty());

    // 装备仍在实体上（dropPreservedEquipment 不清空非保留装备）
    EXPECT_FALSE(m_entity->getEquipment(EquipmentSlot::Head).isEmpty());
}

TEST_F(DropPreservedEquipmentTest, AlwaysTruePredicateDropsOnlyPreserved)
{
    // 无谓词过滤版本：所有装备参与判断，仅保留的掉落
    ItemStack sword(Items::IRON_SWORD, 1);
    m_entity->setEquipment(EquipmentSlot::MainHand, sword);
    m_entity->setGuaranteedDrop(EquipmentSlot::MainHand);

    ItemStack helmet(Items::IRON_HELMET, 1);
    m_entity->setEquipment(EquipmentSlot::Head, helmet);
    // 头盔默认 0.085，非保留

    size_t entityCountBefore = m_world->spawnedEntityCount();
    auto preservedSlots = m_entity->dropPreservedEquipment();

    // 无谓词版本：谓词总是 true，没有绑定诅咒装备，保留槽位为空
    EXPECT_TRUE(preservedSlots.empty());

    // 主手保留剑已掉落
    EXPECT_TRUE(m_entity->getEquipment(EquipmentSlot::MainHand).isEmpty());

    // 头盔仍在实体上（dropPreservedEquipment 不清空非保留装备）
    EXPECT_FALSE(m_entity->getEquipment(EquipmentSlot::Head).isEmpty());

    // 仅保留的剑掉落（1个物品实体）
    EXPECT_EQ(m_world->spawnedEntityCount(), entityCountBefore + 1);
}

TEST_F(DropPreservedEquipmentTest, EmptyEquipmentSlotsSkipped)
{
    // 确保空槽位不会引起问题
    auto preservedSlots = m_entity->dropPreservedEquipment([](const ItemStack&) { return false; });

    EXPECT_TRUE(preservedSlots.empty());
}

TEST_F(DropPreservedEquipmentTest, SetGuaranteedDropMakesEquipmentPreserved)
{
    // 验证 setGuaranteedDrop 使 isEquipmentDropPreserved 返回 true
    EXPECT_FALSE(m_entity->isEquipmentDropPreserved(EquipmentSlot::MainHand));

    m_entity->setGuaranteedDrop(EquipmentSlot::MainHand);

    EXPECT_TRUE(m_entity->isEquipmentDropPreserved(EquipmentSlot::MainHand));
    EXPECT_FLOAT_EQ(m_entity->getEquipmentDropChance(EquipmentSlot::MainHand), 2.0f);
}

} // namespace
