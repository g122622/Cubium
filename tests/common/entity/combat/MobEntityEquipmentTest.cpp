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
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::entity::combat;
using namespace mc::entity::serialization;

// ============================================================================
// 测试世界 - 支持 finalizeSpawn 所需的 IWorld 接口
// ============================================================================

class MobEquipmentTestWorld final : public test::BaseTestWorld {
public:
    MobEquipmentTestWorld()
    {
        Items::initialize();
        VanillaBlocks::initialize();
    }

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty d) { m_difficulty = d; }

private:
    Difficulty m_difficulty = Difficulty::Normal;
};

// ============================================================================
// 测试夹具
// ============================================================================

class MobEntityEquipmentTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            entity::VanillaEntities::registerAll();
            s_initialized = true;
        }
    }

    void SetUp() override { m_world = std::make_unique<MobEquipmentTestWorld>(); }

    std::unique_ptr<MobEquipmentTestWorld> m_world;
};

// ============================================================================
// getEquipmentForSlot 测试 - 需要 Items 注册表初始化
// ============================================================================

TEST_F(MobEntityEquipmentTest, GetEquipmentForSlot_LeatherArmor)
{
    // armorLevel 0 = 皮革护甲
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Head, 0), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Chest, 0), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Legs, 0), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Feet, 0), nullptr);
}

TEST_F(MobEntityEquipmentTest, GetEquipmentForSlot_IronArmor)
{
    // armorLevel 4 = 铁护甲
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Head, 4), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Chest, 4), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Legs, 4), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Feet, 4), nullptr);
}

TEST_F(MobEntityEquipmentTest, GetEquipmentForSlot_DiamondArmor)
{
    // armorLevel 5 = 钻石护甲
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Head, 5), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Chest, 5), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Legs, 5), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Feet, 5), nullptr);
}

TEST_F(MobEntityEquipmentTest, GetEquipmentForSlot_GoldArmor)
{
    // armorLevel 2 = 金护甲
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Head, 2), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Chest, 2), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Legs, 2), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Feet, 2), nullptr);
}

TEST_F(MobEntityEquipmentTest, GetEquipmentForSlot_ChainmailArmor)
{
    // armorLevel 3 = 锁链护甲
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Head, 3), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Chest, 3), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Legs, 3), nullptr);
    EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Feet, 3), nullptr);
}

TEST_F(MobEntityEquipmentTest, GetEquipmentForSlot_CopperArmor)
{
    // armorLevel 1 = 铜护甲（MC 1.21.11 新增）
    const Item* copperHelmet = MobEntity::getEquipmentForSlot(EquipmentSlot::Head, 1);
    const Item* ironHelmet = MobEntity::getEquipmentForSlot(EquipmentSlot::Head, 4);
    EXPECT_NE(copperHelmet, nullptr);
    EXPECT_NE(ironHelmet, nullptr);
    // 铜护甲与铁护甲是不同物品
    EXPECT_NE(copperHelmet, ironHelmet);

    const Item* copperChest = MobEntity::getEquipmentForSlot(EquipmentSlot::Chest, 1);
    const Item* ironChest = MobEntity::getEquipmentForSlot(EquipmentSlot::Chest, 4);
    EXPECT_NE(copperChest, ironChest);

    const Item* copperLegs = MobEntity::getEquipmentForSlot(EquipmentSlot::Legs, 1);
    const Item* copperFeet = MobEntity::getEquipmentForSlot(EquipmentSlot::Feet, 1);
    EXPECT_NE(copperLegs, nullptr);
    EXPECT_NE(copperFeet, nullptr);
}

TEST_F(MobEntityEquipmentTest, GetEquipmentForSlot_MainHandAndOffHandReturnNull)
{
    // 主手和副手槽位不返回护甲
    EXPECT_EQ(MobEntity::getEquipmentForSlot(EquipmentSlot::MainHand, 0), nullptr);
    EXPECT_EQ(MobEntity::getEquipmentForSlot(EquipmentSlot::OffHand, 0), nullptr);
    EXPECT_EQ(MobEntity::getEquipmentForSlot(EquipmentSlot::MainHand, 5), nullptr);
    EXPECT_EQ(MobEntity::getEquipmentForSlot(EquipmentSlot::OffHand, 5), nullptr);
}

TEST_F(MobEntityEquipmentTest, GetEquipmentForSlot_InvalidLevelReturnsNull)
{
    // armorLevel < 0 或 > 5 返回 nullptr
    EXPECT_EQ(MobEntity::getEquipmentForSlot(EquipmentSlot::Head, -1), nullptr);
    EXPECT_EQ(MobEntity::getEquipmentForSlot(EquipmentSlot::Head, 6), nullptr);
    EXPECT_EQ(MobEntity::getEquipmentForSlot(EquipmentSlot::Chest, -1), nullptr);
    EXPECT_EQ(MobEntity::getEquipmentForSlot(EquipmentSlot::Legs, 100), nullptr);
    EXPECT_EQ(MobEntity::getEquipmentForSlot(EquipmentSlot::Feet, -5), nullptr);
}

TEST_F(MobEntityEquipmentTest, GetEquipmentForSlot_AllArmorLevelsPerSlot)
{
    // 验证所有护甲等级 0-5 对每个槽位都返回有效物品
    for (i32 level = 0; level <= 5; ++level) {
        EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Head, level), nullptr)
            << "Head armor level " << level << " returned null";
        EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Chest, level), nullptr)
            << "Chest armor level " << level << " returned null";
        EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Legs, level), nullptr)
            << "Legs armor level " << level << " returned null";
        EXPECT_NE(MobEntity::getEquipmentForSlot(EquipmentSlot::Feet, level), nullptr)
            << "Feet armor level " << level << " returned null";
    }
}

TEST_F(MobEntityEquipmentTest, GetEquipmentForSlot_DifferentArmorTypesAreDistinct)
{
    // 不同护甲等级的相同槽位应返回不同物品
    const Item* leather = MobEntity::getEquipmentForSlot(EquipmentSlot::Head, 0);
    const Item* copper = MobEntity::getEquipmentForSlot(EquipmentSlot::Head, 1);
    const Item* gold = MobEntity::getEquipmentForSlot(EquipmentSlot::Head, 2);
    const Item* chainmail = MobEntity::getEquipmentForSlot(EquipmentSlot::Head, 3);
    const Item* iron = MobEntity::getEquipmentForSlot(EquipmentSlot::Head, 4);
    const Item* diamond = MobEntity::getEquipmentForSlot(EquipmentSlot::Head, 5);

    EXPECT_NE(leather, copper);
    EXPECT_NE(leather, gold);
    EXPECT_NE(leather, chainmail);
    EXPECT_NE(leather, iron);
    EXPECT_NE(leather, diamond);
    EXPECT_NE(copper, gold);
    EXPECT_NE(copper, chainmail);
    EXPECT_NE(copper, iron);
    EXPECT_NE(copper, diamond);
    EXPECT_NE(gold, chainmail);
    EXPECT_NE(gold, iron);
    EXPECT_NE(gold, diamond);
    EXPECT_NE(chainmail, diamond);
    EXPECT_NE(iron, diamond);
}

// ============================================================================
// DifficultyInstance 概率测试
// ============================================================================

TEST_F(MobEntityEquipmentTest, PeacefulDifficulty_NoEquipmentProbability)
{
    // Peaceful: specialMultiplier = 0.0 → 所有概率为 0
    DifficultyInstance peaceful(Difficulty::Peaceful);
    EXPECT_FLOAT_EQ(peaceful.getSpecialMultiplier(), 0.0f);
    EXPECT_FLOAT_EQ(0.15f * peaceful.getSpecialMultiplier(), 0.0f); // 护甲
    EXPECT_FLOAT_EQ(0.25f * peaceful.getSpecialMultiplier(), 0.0f); // 武器附魔
    EXPECT_FLOAT_EQ(0.5f * peaceful.getSpecialMultiplier(), 0.0f);  // 护甲附魔
    EXPECT_FLOAT_EQ(0.55f * peaceful.getSpecialMultiplier(), 0.0f); // 拾取物品
}

TEST_F(MobEntityEquipmentTest, EasyDifficulty_NoEquipmentProbability)
{
    // Easy: effectiveDifficulty = 0.75 < 2.0, specialMultiplier = 0.0
    DifficultyInstance easy(Difficulty::Easy);
    EXPECT_FLOAT_EQ(easy.getSpecialMultiplier(), 0.0f);
}

TEST_F(MobEntityEquipmentTest, NormalSimplified_NoEquipmentProbability)
{
    // Normal 简化构造: effectiveDifficulty = 2.0, specialMultiplier = 0.0
    DifficultyInstance normal(Difficulty::Normal);
    EXPECT_FLOAT_EQ(normal.getSpecialMultiplier(), 0.0f);
}

TEST_F(MobEntityEquipmentTest, HardSimplified_EquipmentProbability)
{
    // Hard 简化构造: effectiveDifficulty = 3.0, specialMultiplier = 0.5
    DifficultyInstance hard(Difficulty::Hard);
    EXPECT_FLOAT_EQ(hard.getSpecialMultiplier(), 0.5f);
    EXPECT_FLOAT_EQ(0.15f * hard.getSpecialMultiplier(), 0.075f); // 护甲 7.5%
    EXPECT_FLOAT_EQ(0.25f * hard.getSpecialMultiplier(), 0.125f); // 武器附魔 12.5%
    EXPECT_FLOAT_EQ(0.5f * hard.getSpecialMultiplier(), 0.25f);   // 护甲附魔 25%
    EXPECT_FLOAT_EQ(0.55f * hard.getSpecialMultiplier(), 0.275f); // 拾取物品 27.5%
    EXPECT_FLOAT_EQ(hard.getSpecialMultiplier() * 0.1f, 0.05f);   // 破门 5%
}

TEST_F(MobEntityEquipmentTest, FullConstructor_MaxDifficulty_MaxSpecialMultiplier)
{
    // 所有因子拉满时 specialMultiplier = 1.0
    DifficultyInstance maxHard(Difficulty::Hard, 1440000, 3600000, 1.0f);
    EXPECT_FLOAT_EQ(maxHard.getSpecialMultiplier(), 1.0f);
    EXPECT_FLOAT_EQ(0.15f * maxHard.getSpecialMultiplier(), 0.15f); // 护甲 15%
    EXPECT_FLOAT_EQ(0.25f * maxHard.getSpecialMultiplier(), 0.25f); // 武器附魔 25%
    EXPECT_FLOAT_EQ(0.5f * maxHard.getSpecialMultiplier(), 0.5f);   // 护甲附魔 50%
    EXPECT_FLOAT_EQ(0.55f * maxHard.getSpecialMultiplier(), 0.55f); // 拾取物品 55%
}

// ============================================================================
// DifficultyHelper 集成测试
// ============================================================================

TEST_F(MobEntityEquipmentTest, DifficultyHelper_RegionalDifficultyBase)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::getRegionalDifficultyBase(Difficulty::Peaceful), 0.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getRegionalDifficultyBase(Difficulty::Easy), 0.75f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getRegionalDifficultyBase(Difficulty::Normal), 1.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getRegionalDifficultyBase(Difficulty::Hard), 1.0f);
}

TEST_F(MobEntityEquipmentTest, DifficultyHelper_MobDamageAdjustment)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(Difficulty::Peaceful), 0.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(Difficulty::Easy), -2.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(Difficulty::Normal), 0.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(Difficulty::Hard), 2.0f);
}

TEST_F(MobEntityEquipmentTest, DifficultyHelper_PlayerDamageAdjustment)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Peaceful, 10.0f), 0.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Easy, 10.0f), 6.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Normal, 10.0f), 10.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Hard, 10.0f), 15.0f);

    // 边界：小伤害（Easy min(damage/2 + 1, damage)）
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Easy, 1.0f), 1.0f);
    // 边界：大伤害
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Easy, 20.0f), 11.0f);
    // 边界：零伤害
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Hard, 0.0f), 0.0f);
    // Peaceful 任何伤害都为零
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Peaceful, 100.0f), 0.0f);
}

TEST_F(MobEntityEquipmentTest, DifficultyHelper_CanZombieReinforce)
{
    EXPECT_FALSE(DifficultyHelper::canZombieReinforce(Difficulty::Peaceful));
    EXPECT_FALSE(DifficultyHelper::canZombieReinforce(Difficulty::Easy));
    EXPECT_FALSE(DifficultyHelper::canZombieReinforce(Difficulty::Normal));
    EXPECT_TRUE(DifficultyHelper::canZombieReinforce(Difficulty::Hard));
}

// ============================================================================
// canPickUpLoot NBT 序列化测试
// ============================================================================

TEST_F(MobEntityEquipmentTest, CanPickUpLoot_SerializeAndDeserialize)
{
    auto entity = std::make_unique<ZombieEntity>(EntityInstanceId(1));
    entity->setWorld(m_world.get());

    // 默认值应为 false
    EXPECT_FALSE(entity->canPickUpLoot());

    // 设置为 true
    entity->setCanPickUpLoot(true);
    EXPECT_TRUE(entity->canPickUpLoot());

    // 序列化
    nbt::tags::compound_tag tag;
    entity->addAdditionalSaveData(tag);

    // 验证 CanPickUpLoot 键存在且为 true
    auto val = nbt_helper::tryGetBool(tag, nbt_keys::CAN_PICK_UP_LOOT);
    ASSERT_TRUE(val.has_value());
    EXPECT_TRUE(*val);

    // 反序列化到新实体
    auto entity2 = std::make_unique<ZombieEntity>(EntityInstanceId(2));
    entity2->setWorld(m_world.get());
    EXPECT_FALSE(entity2->canPickUpLoot()); // 默认 false
    auto result = entity2->readAdditionalSaveData(tag);
    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_TRUE(entity2->canPickUpLoot()); // 从 NBT 读取为 true
}

TEST_F(MobEntityEquipmentTest, CanPickUpLoot_SerializeFalse)
{
    auto entity = std::make_unique<ZombieEntity>(EntityInstanceId(1));
    entity->setWorld(m_world.get());

    // 默认 false
    EXPECT_FALSE(entity->canPickUpLoot());

    // 序列化 false
    nbt::tags::compound_tag tag;
    entity->addAdditionalSaveData(tag);

    auto val = nbt_helper::tryGetBool(tag, nbt_keys::CAN_PICK_UP_LOOT);
    ASSERT_TRUE(val.has_value());
    EXPECT_FALSE(*val);

    // 反序列化到已设为 true 的新实体，应覆盖为 false
    auto entity2 = std::make_unique<ZombieEntity>(EntityInstanceId(2));
    entity2->setWorld(m_world.get());
    entity2->setCanPickUpLoot(true);
    EXPECT_TRUE(entity2->canPickUpLoot());
    auto result = entity2->readAdditionalSaveData(tag);
    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_FALSE(entity2->canPickUpLoot());
}

TEST_F(MobEntityEquipmentTest, CanPickUpLoot_MissingKeyKeepsDefault)
{
    // 当 NBT 中没有 CanPickUpLoot 键时，应保持默认值
    auto entity = std::make_unique<ZombieEntity>(EntityInstanceId(1));
    entity->setWorld(m_world.get());

    // 空的 compound_tag 没有 CanPickUpLoot
    nbt::tags::compound_tag emptyTag;
    auto result = entity->readAdditionalSaveData(emptyTag);
    // canPickUpLoot 应保持默认 false
    EXPECT_FALSE(entity->canPickUpLoot());
}

// ============================================================================
// finalizeSpawn canPickUpLoot 概率测试
// ============================================================================

TEST_F(MobEntityEquipmentTest, FinalizeSpawn_PeacefulNeverPicksUpLoot)
{
    // Peaceful: specialMultiplier = 0.0, 0.55 * 0.0 = 0.0 → 永远不拾取
    DifficultyInstance peaceful(Difficulty::Peaceful);

    for (int i = 0; i < 100; ++i) {
        auto entity = std::make_unique<ZombieEntity>(EntityInstanceId(static_cast<u64>(i + 1)));
        entity->setWorld(m_world.get());
        entity->finalizeSpawn(*m_world, peaceful, world::spawn::SpawnReason::Natural);
        EXPECT_FALSE(entity->canPickUpLoot()) << "Zombie should never pick up loot on Peaceful, iteration " << i;
    }
}

TEST_F(MobEntityEquipmentTest, FinalizeSpawn_EasyNeverPicksUpLoot)
{
    // Easy: specialMultiplier = 0.0, 同 Peaceful
    DifficultyInstance easy(Difficulty::Easy);

    for (int i = 0; i < 100; ++i) {
        auto entity = std::make_unique<ZombieEntity>(EntityInstanceId(static_cast<u64>(i + 1)));
        entity->setWorld(m_world.get());
        entity->finalizeSpawn(*m_world, easy, world::spawn::SpawnReason::Natural);
        EXPECT_FALSE(entity->canPickUpLoot()) << "Zombie should never pick up loot on Easy, iteration " << i;
    }
}

TEST_F(MobEntityEquipmentTest, FinalizeSpawn_NormalSimplifiedNeverPicksUpLoot)
{
    // Normal 简化构造: specialMultiplier = 0.0
    DifficultyInstance normal(Difficulty::Normal);

    for (int i = 0; i < 100; ++i) {
        auto entity = std::make_unique<ZombieEntity>(EntityInstanceId(static_cast<u64>(i + 1)));
        entity->setWorld(m_world.get());
        entity->finalizeSpawn(*m_world, normal, world::spawn::SpawnReason::Natural);
        EXPECT_FALSE(entity->canPickUpLoot())
            << "Zombie should never pick up loot on Normal (simplified), iteration " << i;
    }
}

TEST_F(MobEntityEquipmentTest, FinalizeSpawn_HardCanPickUpLootStatistically)
{
    // Hard 简化构造: specialMultiplier = 0.5, 拾取概率 = 0.55 * 0.5 = 0.275
    // 200 次测试中应该有部分拾取、部分不拾取
    DifficultyInstance hard(Difficulty::Hard);
    int pickUpCount = 0;
    constexpr int iterations = 200;

    for (int i = 0; i < iterations; ++i) {
        auto entity = std::make_unique<ZombieEntity>(EntityInstanceId(static_cast<u64>(i + 1)));
        entity->setWorld(m_world.get());
        entity->finalizeSpawn(*m_world, hard, world::spawn::SpawnReason::Natural);
        if (entity->canPickUpLoot()) {
            ++pickUpCount;
        }
    }

    // 概率 27.5%，200 次中期望约 55 次
    // 允许较大波动范围（20-90 次），确保不是全 0 或全 1
    EXPECT_GT(pickUpCount, 20) << "Too few pick-ups, expected ~55 out of 200";
    EXPECT_LT(pickUpCount, 90) << "Too many pick-ups, expected ~55 out of 200";
}

// ============================================================================
// populateDefaultEquipmentSlots 护甲生成概率测试
// ============================================================================

TEST_F(MobEntityEquipmentTest, PopulateDefaultEquipment_PeacefulNoArmor)
{
    // Peaceful: specialMultiplier = 0.0, 护甲概率 = 0.15 * 0.0 = 0 → 无护甲
    DifficultyInstance peaceful(Difficulty::Peaceful);

    for (int i = 0; i < 50; ++i) {
        auto entity = std::make_unique<ZombieEntity>(EntityInstanceId(static_cast<u64>(i + 1)));
        entity->setWorld(m_world.get());
        entity->finalizeSpawn(*m_world, peaceful, world::spawn::SpawnReason::Natural);

        // Peaceful 不应生成任何护甲
        EXPECT_TRUE(entity->getEquipment(EquipmentSlot::Head).isEmpty())
            << "Zombie should not have head armor on Peaceful, iteration " << i;
        EXPECT_TRUE(entity->getEquipment(EquipmentSlot::Chest).isEmpty())
            << "Zombie should not have chest armor on Peaceful, iteration " << i;
        EXPECT_TRUE(entity->getEquipment(EquipmentSlot::Legs).isEmpty())
            << "Zombie should not have legs armor on Peaceful, iteration " << i;
        EXPECT_TRUE(entity->getEquipment(EquipmentSlot::Feet).isEmpty())
            << "Zombie should not have feet armor on Peaceful, iteration " << i;
    }
}

TEST_F(MobEntityEquipmentTest, PopulateDefaultEquipment_HardHasArmorStatistically)
{
    // Hard 简化构造: specialMultiplier = 0.5, 护甲概率 = 0.15 * 0.5 = 0.075
    // 测试 200 次，期望约 15 次有护甲
    DifficultyInstance hard(Difficulty::Hard);
    int armorCount = 0;
    constexpr int iterations = 200;

    for (int i = 0; i < iterations; ++i) {
        auto entity = std::make_unique<ZombieEntity>(EntityInstanceId(static_cast<u64>(i + 1)));
        entity->setWorld(m_world.get());
        entity->finalizeSpawn(*m_world, hard, world::spawn::SpawnReason::Natural);

        bool hasArmor = !entity->getEquipment(EquipmentSlot::Head).isEmpty() ||
            !entity->getEquipment(EquipmentSlot::Chest).isEmpty() ||
            !entity->getEquipment(EquipmentSlot::Legs).isEmpty() ||
            !entity->getEquipment(EquipmentSlot::Feet).isEmpty();
        if (hasArmor) {
            ++armorCount;
        }
    }

    // 护甲概率 7.5%，200 次中期望约 15 次
    EXPECT_GT(armorCount, 0) << "Hard difficulty should sometimes generate armor";
}

// ============================================================================
// ZombieEntity 破门能力测试
// ============================================================================

TEST_F(MobEntityEquipmentTest, ZombieBreakDoorAbility_PeacefulNever)
{
    // Peaceful: specialMultiplier * 0.1 = 0.0 → 永远不会破门
    DifficultyInstance peaceful(Difficulty::Peaceful);

    for (int i = 0; i < 100; ++i) {
        auto entity = std::make_unique<ZombieEntity>(EntityInstanceId(static_cast<u64>(i + 1)));
        entity->setWorld(m_world.get());
        entity->finalizeSpawn(*m_world, peaceful, world::spawn::SpawnReason::Natural);
        EXPECT_FALSE(entity->canBreakDoors()) << "Zombie should never break doors on Peaceful, iteration " << i;
    }
}

TEST_F(MobEntityEquipmentTest, ZombieBreakDoorAbility_HardCanBreak)
{
    // Hard 简化构造: specialMultiplier * 0.1 = 0.05 → 5% 概率
    DifficultyInstance hard(Difficulty::Hard);
    int breakDoorCount = 0;
    constexpr int iterations = 500;

    for (int i = 0; i < iterations; ++i) {
        auto entity = std::make_unique<ZombieEntity>(EntityInstanceId(static_cast<u64>(i + 1)));
        entity->setWorld(m_world.get());
        entity->finalizeSpawn(*m_world, hard, world::spawn::SpawnReason::Natural);
        if (entity->canBreakDoors()) {
            ++breakDoorCount;
        }
    }

    // 概率 5%，500 次中期望约 25 次
    EXPECT_GT(breakDoorCount, 5) << "Hard difficulty should sometimes enable door breaking";
    EXPECT_LT(breakDoorCount, 60) << "Door breaking probability should be ~5%";
}

// ============================================================================
// ZombieEntity 武器生成逻辑测试
// Hard: 5% 概率生成铁剑/铁锹，其他: 1%
// ============================================================================

TEST_F(MobEntityEquipmentTest, ZombieWeapon_PeacefulNoWeapon)
{
    // Peaceful: 护甲概率为 0，但 Zombie 的武器逻辑独立于护甲
    // Zombie 的武器概率 = Hard ? 0.05 : 0.01
    // 注意：即使 Peaceful 的 specialMultiplier=0 不影响武器概率，
    // 但武器概率不依赖 specialMultiplier，仅依赖难度是否为 Hard
    // Peaceful 时 difficulty != Hard → 概率 1%
    // 这里验证 Peaceful 下僵尸有极小概率持有武器
    DifficultyInstance peaceful(Difficulty::Peaceful);
    int weaponCount = 0;
    constexpr int iterations = 500;

    for (int i = 0; i < iterations; ++i) {
        auto entity = std::make_unique<ZombieEntity>(EntityInstanceId(static_cast<u64>(i + 1)));
        entity->setWorld(m_world.get());
        entity->finalizeSpawn(*m_world, peaceful, world::spawn::SpawnReason::Natural);
        if (!entity->getEquipment(EquipmentSlot::MainHand).isEmpty()) {
            ++weaponCount;
        }
    }

    // 概率 1%，500 次中期望约 5 次
    // 由于概率极低，可能为 0，但不应超过 15 次
    EXPECT_LT(weaponCount, 15) << "Peaceful zombie weapon probability should be ~1%";
}

TEST_F(MobEntityEquipmentTest, ZombieWeapon_HardHigherProbability)
{
    // Hard: 武器概率 = 5%（0.05）
    DifficultyInstance hard(Difficulty::Hard);
    int weaponCount = 0;
    constexpr int iterations = 500;

    for (int i = 0; i < iterations; ++i) {
        auto entity = std::make_unique<ZombieEntity>(EntityInstanceId(static_cast<u64>(i + 1)));
        entity->setWorld(m_world.get());
        entity->finalizeSpawn(*m_world, hard, world::spawn::SpawnReason::Natural);
        if (!entity->getEquipment(EquipmentSlot::MainHand).isEmpty()) {
            ++weaponCount;
        }
    }

    // 概率 5%，500 次中期望约 25 次
    EXPECT_GT(weaponCount, 5) << "Hard difficulty should generate zombie weapons more often";
    EXPECT_LT(weaponCount, 60) << "Zombie weapon probability on Hard should be ~5%";
}

// ============================================================================
// DifficultyInstance 完整构造函数测试
// ============================================================================

TEST_F(MobEntityEquipmentTest, FullConstructor_MidDifficulty_MidSpecialMultiplier)
{
    // effectiveDifficulty = 3.0 时 specialMultiplier = 0.5
    DifficultyInstance hard(Difficulty::Hard);
    EXPECT_FLOAT_EQ(hard.getSpecialMultiplier(), 0.5f);
    EXPECT_FLOAT_EQ(0.15f * hard.getSpecialMultiplier(), 0.075f); // 护甲 7.5%
    EXPECT_FLOAT_EQ(0.25f * hard.getSpecialMultiplier(), 0.125f); // 武器附魔 12.5%
    EXPECT_FLOAT_EQ(0.5f * hard.getSpecialMultiplier(), 0.25f);   // 护甲附魔 25%
}

// ============================================================================
// canPickUpLoot 概率验证（所有难度级别）
// ============================================================================

TEST_F(MobEntityEquipmentTest, CanPickUpLoot_ProbabilityByDifficulty)
{
    // canPickUpLoot = random.nextFloat() < 0.55 * specialMultiplier
    DifficultyInstance peaceful(Difficulty::Peaceful);
    EXPECT_FLOAT_EQ(0.55f * peaceful.getSpecialMultiplier(), 0.0f);

    DifficultyInstance easy(Difficulty::Easy);
    EXPECT_FLOAT_EQ(0.55f * easy.getSpecialMultiplier(), 0.0f);

    DifficultyInstance normal(Difficulty::Normal);
    EXPECT_FLOAT_EQ(0.55f * normal.getSpecialMultiplier(), 0.0f);

    DifficultyInstance hard(Difficulty::Hard);
    EXPECT_FLOAT_EQ(0.55f * hard.getSpecialMultiplier(), 0.275f);
}
