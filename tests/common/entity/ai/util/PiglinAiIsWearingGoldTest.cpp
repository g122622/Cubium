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
 * @file PiglinAiIsWearingGoldTest.cpp
 * @brief PiglinAi::isWearingGold 行为测试
 *
 * 测试 PiglinAi::isWearingGold 在不同装备场景下的判定：
 * - 无装备时返回 false
 * - 仅非金盔甲（铁盔甲）时返回 false
 * - 任意一件金盔甲时返回 true
 * - 多件金盔甲混合穿戴时返回 true
 * - 金盔甲 + 铁盔甲混合时返回 true
 * - 主手/副手金物品不影响判定（仅检查4个盔甲槽位）
 *
 * 该方法被 PiglinEntity 的 NearestAttackableTargetGoal 谓词调用，
 * 用于过滤掉穿戴金装备的玩家，使其不成为猪灵的攻击目标。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/util/PiglinAi.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/core/ItemStack.hpp"

using namespace mc;

namespace {

/// 测试用世界桩，仅满足 Player 构造所需 IWorld 依赖
class PiglinAiGoldTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("PiglinAiGoldTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("PiglinAiGoldTestWorld::tickManager not implemented");
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}
    void broadcastEntityStatus(EntityInstanceId, u8) override {}
};

} // namespace

/// PiglinAi::isWearingGold 测试夹具
class PiglinAiIsWearingGoldTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::initialize();
            item::armor::ArmorMaterials::initialize();
            s_initialized = true;
        }
    }

    void SetUp() override
    {
        m_world = std::make_unique<PiglinAiGoldTestWorld>();
        m_player = std::make_unique<Player>(static_cast<EntityInstanceId>(1), "GoldArmorTestPlayer");
        m_player->setWorld(m_world.get());
    }

    void TearDown() override
    {
        m_player.reset();
        m_world.reset();
    }

    std::unique_ptr<PiglinAiGoldTestWorld> m_world;
    std::unique_ptr<Player> m_player;
};

// ============================================================================
// 无装备 / 非金装备场景
// ============================================================================

TEST_F(PiglinAiIsWearingGoldTest, NoArmor_ReturnsFalse)
{
    // 玩家无任何装备时，isWearingGold 应返回 false
    EXPECT_FALSE(entity::PiglinAi::isWearingGold(*m_player));
}

TEST_F(PiglinAiIsWearingGoldTest, OnlyIronArmor_ReturnsFalse)
{
    // 玩家穿戴全套铁盔甲时，isWearingGold 应返回 false
    ASSERT_NE(Items::IRON_HELMET, nullptr);
    ASSERT_NE(Items::IRON_CHESTPLATE, nullptr);
    ASSERT_NE(Items::IRON_LEGGINGS, nullptr);
    ASSERT_NE(Items::IRON_BOOTS, nullptr);

    m_player->setEquipment(EquipmentSlot::Head, ItemStack(Items::IRON_HELMET, 1));
    m_player->setEquipment(EquipmentSlot::Chest, ItemStack(Items::IRON_CHESTPLATE, 1));
    m_player->setEquipment(EquipmentSlot::Legs, ItemStack(Items::IRON_LEGGINGS, 1));
    m_player->setEquipment(EquipmentSlot::Feet, ItemStack(Items::IRON_BOOTS, 1));

    EXPECT_FALSE(entity::PiglinAi::isWearingGold(*m_player));
}

// ============================================================================
// 单件金盔甲场景（4 个槽位各测一次）
// ============================================================================

TEST_F(PiglinAiIsWearingGoldTest, GoldHelmetOnly_ReturnsTrue)
{
    // 仅头部穿戴金头盔时，isWearingGold 应返回 true
    ASSERT_NE(Items::GOLDEN_HELMET, nullptr);
    m_player->setEquipment(EquipmentSlot::Head, ItemStack(Items::GOLDEN_HELMET, 1));
    EXPECT_TRUE(entity::PiglinAi::isWearingGold(*m_player));
}

TEST_F(PiglinAiIsWearingGoldTest, GoldChestplateOnly_ReturnsTrue)
{
    // 仅胸部穿戴金胸甲时，isWearingGold 应返回 true
    ASSERT_NE(Items::GOLDEN_CHESTPLATE, nullptr);
    m_player->setEquipment(EquipmentSlot::Chest, ItemStack(Items::GOLDEN_CHESTPLATE, 1));
    EXPECT_TRUE(entity::PiglinAi::isWearingGold(*m_player));
}

TEST_F(PiglinAiIsWearingGoldTest, GoldLeggingsOnly_ReturnsTrue)
{
    // 仅腿部穿戴金护腿时，isWearingGold 应返回 true
    ASSERT_NE(Items::GOLDEN_LEGGINGS, nullptr);
    m_player->setEquipment(EquipmentSlot::Legs, ItemStack(Items::GOLDEN_LEGGINGS, 1));
    EXPECT_TRUE(entity::PiglinAi::isWearingGold(*m_player));
}

TEST_F(PiglinAiIsWearingGoldTest, GoldBootsOnly_ReturnsTrue)
{
    // 仅脚部穿戴金靴子时，isWearingGold 应返回 true
    ASSERT_NE(Items::GOLDEN_BOOTS, nullptr);
    m_player->setEquipment(EquipmentSlot::Feet, ItemStack(Items::GOLDEN_BOOTS, 1));
    EXPECT_TRUE(entity::PiglinAi::isWearingGold(*m_player));
}

// ============================================================================
// 全套金盔甲场景
// ============================================================================

TEST_F(PiglinAiIsWearingGoldTest, FullGoldArmor_ReturnsTrue)
{
    // 全套金盔甲时，isWearingGold 应返回 true
    ASSERT_NE(Items::GOLDEN_HELMET, nullptr);
    ASSERT_NE(Items::GOLDEN_CHESTPLATE, nullptr);
    ASSERT_NE(Items::GOLDEN_LEGGINGS, nullptr);
    ASSERT_NE(Items::GOLDEN_BOOTS, nullptr);

    m_player->setEquipment(EquipmentSlot::Head, ItemStack(Items::GOLDEN_HELMET, 1));
    m_player->setEquipment(EquipmentSlot::Chest, ItemStack(Items::GOLDEN_CHESTPLATE, 1));
    m_player->setEquipment(EquipmentSlot::Legs, ItemStack(Items::GOLDEN_LEGGINGS, 1));
    m_player->setEquipment(EquipmentSlot::Feet, ItemStack(Items::GOLDEN_BOOTS, 1));

    EXPECT_TRUE(entity::PiglinAi::isWearingGold(*m_player));
}

// ============================================================================
// 金 + 铁混合场景
// ============================================================================

TEST_F(PiglinAiIsWearingGoldTest, MixedGoldAndIron_ReturnsTrue)
{
    // 金头盔 + 铁胸甲 + 铁护腿 + 铁靴子，存在金装备即返回 true
    ASSERT_NE(Items::GOLDEN_HELMET, nullptr);
    ASSERT_NE(Items::IRON_CHESTPLATE, nullptr);
    ASSERT_NE(Items::IRON_LEGGINGS, nullptr);
    ASSERT_NE(Items::IRON_BOOTS, nullptr);

    m_player->setEquipment(EquipmentSlot::Head, ItemStack(Items::GOLDEN_HELMET, 1));
    m_player->setEquipment(EquipmentSlot::Chest, ItemStack(Items::IRON_CHESTPLATE, 1));
    m_player->setEquipment(EquipmentSlot::Legs, ItemStack(Items::IRON_LEGGINGS, 1));
    m_player->setEquipment(EquipmentSlot::Feet, ItemStack(Items::IRON_BOOTS, 1));

    EXPECT_TRUE(entity::PiglinAi::isWearingGold(*m_player));
}

// ============================================================================
// 主手/副手金物品不影响判定（仅检查4个盔甲槽位）
// ============================================================================

TEST_F(PiglinAiIsWearingGoldTest, GoldIngotInMainHand_ReturnsFalse)
{
    // 主手持金锭（非盔甲槽位），isWearingGold 应返回 false
    ASSERT_NE(Items::GOLD_INGOT, nullptr);
    m_player->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::GOLD_INGOT, 1));
    EXPECT_FALSE(entity::PiglinAi::isWearingGold(*m_player));
}

TEST_F(PiglinAiIsWearingGoldTest, GoldIngotInOffHand_ReturnsFalse)
{
    // 副手持金锭（非盔甲槽位），isWearingGold 应返回 false
    ASSERT_NE(Items::GOLD_INGOT, nullptr);
    m_player->setEquipment(EquipmentSlot::OffHand, ItemStack(Items::GOLD_INGOT, 1));
    EXPECT_FALSE(entity::PiglinAi::isWearingGold(*m_player));
}

// ============================================================================
// 一致性测试：isWearingGold 与 Player::isWearingGoldArmor 结果一致
// ============================================================================

TEST_F(PiglinAiIsWearingGoldTest, ConsistentWithPlayerIsWearingGoldArmor)
{
    // PiglinAi::isWearingGold 应与 Player::isWearingGoldArmor 返回一致结果
    // 无装备
    EXPECT_EQ(entity::PiglinAi::isWearingGold(*m_player), m_player->isWearingGoldArmor());

    // 金头盔
    ASSERT_NE(Items::GOLDEN_HELMET, nullptr);
    m_player->setEquipment(EquipmentSlot::Head, ItemStack(Items::GOLDEN_HELMET, 1));
    EXPECT_EQ(entity::PiglinAi::isWearingGold(*m_player), m_player->isWearingGoldArmor());

    // 加上铁胸甲
    ASSERT_NE(Items::IRON_CHESTPLATE, nullptr);
    m_player->setEquipment(EquipmentSlot::Chest, ItemStack(Items::IRON_CHESTPLATE, 1));
    EXPECT_EQ(entity::PiglinAi::isWearingGold(*m_player), m_player->isWearingGoldArmor());
}
