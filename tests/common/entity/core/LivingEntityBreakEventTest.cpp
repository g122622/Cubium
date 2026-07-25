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

#include "common/TestWorldHelper.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/Items.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <optional>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::attribute;

namespace {

// ============================================================================
// 测试用 mock 世界 — 捕获 broadcastEntityStatus 和 playSound 调用
// ============================================================================

class BreakEventTestWorld final : public test::BaseTestWorld {
public:
    struct StatusRecord {
        EntityInstanceId entityId;
        u8 status;
    };

    struct SoundRecord {
        ResourceLocation soundEventId;
        sound::SoundCategory category;
        Vector3 position;
        f32 volume;
        f32 pitch;
    };

    void broadcastEntityStatus(EntityInstanceId entityId, u8 status) override
    {
        m_statusRecords.push_back(StatusRecord{entityId, status});
    }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_soundRecords.push_back(SoundRecord{soundEventId, category, position, volume, pitch});
    }

    [[nodiscard]] const std::vector<StatusRecord>& statusRecords() const { return m_statusRecords; }
    [[nodiscard]] const std::vector<SoundRecord>& soundRecords() const { return m_soundRecords; }
    void clearRecords()
    {
        m_statusRecords.clear();
        m_soundRecords.clear();
    }

    // TickManager 接口（桩实现）
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("BreakEventTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("BreakEventTestWorld::tickManager not implemented");
    }

private:
    std::vector<StatusRecord> m_statusRecords;
    std::vector<SoundRecord> m_soundRecords;
};

// ============================================================================
// 测试用 TestLivingEntity
// ============================================================================

class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(42))
    {
        registerData();
        registerAttributes();
        setHealth(maxHealth());
    }
};

} // namespace

// ============================================================================
// 测试固定装置
// ============================================================================

class LivingEntityBreakEventTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }

    void SetUp() override
    {
        m_world = std::make_unique<BreakEventTestWorld>();
        m_living = std::make_unique<TestLivingEntity>();
        m_living->setWorld(m_world.get());
    }

    void TearDown() override
    {
        m_living.reset();
        m_world.reset();
    }

    std::unique_ptr<BreakEventTestWorld> m_world;
    std::unique_ptr<TestLivingEntity> m_living;
};

// ============================================================================
// broadcastBreakEvent 测试
// ============================================================================

TEST_F(LivingEntityBreakEventTest, BroadcastBreakEventMainHand)
{
    m_living->broadcastBreakEvent(EquipmentSlot::MainHand);
    ASSERT_EQ(m_world->statusRecords().size(), 1u);
    EXPECT_EQ(m_world->statusRecords()[0].entityId, EntityInstanceId(42));
    EXPECT_EQ(m_world->statusRecords()[0].status, static_cast<u8>(network::EntityStatus::EquipmentBreakMainHand));
}

TEST_F(LivingEntityBreakEventTest, BroadcastBreakEventOffHand)
{
    m_living->broadcastBreakEvent(EquipmentSlot::OffHand);
    ASSERT_EQ(m_world->statusRecords().size(), 1u);
    EXPECT_EQ(m_world->statusRecords()[0].status, static_cast<u8>(network::EntityStatus::EquipmentBreakOffHand));
}

TEST_F(LivingEntityBreakEventTest, BroadcastBreakEventHead)
{
    m_living->broadcastBreakEvent(EquipmentSlot::Head);
    ASSERT_EQ(m_world->statusRecords().size(), 1u);
    EXPECT_EQ(m_world->statusRecords()[0].status, static_cast<u8>(network::EntityStatus::EquipmentBreakHead));
}

TEST_F(LivingEntityBreakEventTest, BroadcastBreakEventChest)
{
    m_living->broadcastBreakEvent(EquipmentSlot::Chest);
    ASSERT_EQ(m_world->statusRecords().size(), 1u);
    EXPECT_EQ(m_world->statusRecords()[0].status, static_cast<u8>(network::EntityStatus::EquipmentBreakChest));
}

TEST_F(LivingEntityBreakEventTest, BroadcastBreakEventLegs)
{
    m_living->broadcastBreakEvent(EquipmentSlot::Legs);
    ASSERT_EQ(m_world->statusRecords().size(), 1u);
    EXPECT_EQ(m_world->statusRecords()[0].status, static_cast<u8>(network::EntityStatus::EquipmentBreakLegs));
}

TEST_F(LivingEntityBreakEventTest, BroadcastBreakEventFeet)
{
    m_living->broadcastBreakEvent(EquipmentSlot::Feet);
    ASSERT_EQ(m_world->statusRecords().size(), 1u);
    EXPECT_EQ(m_world->statusRecords()[0].status, static_cast<u8>(network::EntityStatus::EquipmentBreakFeet));
}

TEST_F(LivingEntityBreakEventTest, BroadcastBreakEventNoWorld)
{
    // 无世界时不应崩溃
    m_living->setWorld(nullptr);
    m_living->broadcastBreakEvent(EquipmentSlot::MainHand);
    // 无断言——仅验证不崩溃
}

// ============================================================================
// onEquippedItemBroken 测试
// ============================================================================

TEST_F(LivingEntityBreakEventTest, OnEquippedItemBrokenPlaysSound)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    m_living->onEquippedItemBroken(*Items::IRON_SWORD, EquipmentSlot::MainHand);

    // 应播放 ENTITY_ITEM_BREAK 音效
    ASSERT_EQ(m_world->soundRecords().size(), 1u);
    EXPECT_EQ(m_world->soundRecords()[0].soundEventId, SoundEvents::ENTITY_ITEM_BREAK);
    EXPECT_FLOAT_EQ(m_world->soundRecords()[0].volume, 0.8f);
    // 音调在 [0.8, 1.2] 范围内
    EXPECT_GE(m_world->soundRecords()[0].pitch, 0.8f);
    EXPECT_LE(m_world->soundRecords()[0].pitch, 1.2f);
}

TEST_F(LivingEntityBreakEventTest, OnEquippedItemBrokenBroadcastsStatus)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    m_living->onEquippedItemBroken(*Items::IRON_SWORD, EquipmentSlot::Head);

    // 应广播装备破损状态码
    ASSERT_EQ(m_world->statusRecords().size(), 1u);
    EXPECT_EQ(m_world->statusRecords()[0].entityId, EntityInstanceId(42));
    EXPECT_EQ(m_world->statusRecords()[0].status, static_cast<u8>(network::EntityStatus::EquipmentBreakHead));
}

TEST_F(LivingEntityBreakEventTest, OnEquippedItemBrokenBothBroadcastAndSound)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    // 调用一次 onEquippedItemBroken 应同时触发广播和音效
    m_living->onEquippedItemBroken(*Items::IRON_SWORD, EquipmentSlot::Chest);

    EXPECT_EQ(m_world->statusRecords().size(), 1u);
    EXPECT_EQ(m_world->soundRecords().size(), 1u);
}

TEST_F(LivingEntityBreakEventTest, OnEquippedItemBrokenSilentEntityNoSound)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    // 静音实体不应播放音效，但仍应广播状态
    m_living->setSilent(true);

    m_living->onEquippedItemBroken(*Items::IRON_SWORD, EquipmentSlot::MainHand);

    // 广播仍然发生
    EXPECT_EQ(m_world->statusRecords().size(), 1u);
    // 音效不应播放
    EXPECT_EQ(m_world->soundRecords().size(), 0u);
}

TEST_F(LivingEntityBreakEventTest, OnEquippedItemBrokenNoWorldNoCrash)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    // 无世界时不应崩溃（不播放音效也不广播状态）
    m_living->setWorld(nullptr);
    m_living->onEquippedItemBroken(*Items::IRON_SWORD, EquipmentSlot::MainHand);
    // 仅验证不崩溃
}

TEST_F(LivingEntityBreakEventTest, OnEquippedItemBrokenAllSlotsCorrectStatus)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    // 验证每个装备槽位对应的正确状态码
    struct TestCase {
        EquipmentSlot slot;
        network::EntityStatus expectedStatus;
    };

    const TestCase testCases[] = {
        {EquipmentSlot::MainHand, network::EntityStatus::EquipmentBreakMainHand},
        {EquipmentSlot::OffHand, network::EntityStatus::EquipmentBreakOffHand},
        {EquipmentSlot::Head, network::EntityStatus::EquipmentBreakHead},
        {EquipmentSlot::Chest, network::EntityStatus::EquipmentBreakChest},
        {EquipmentSlot::Legs, network::EntityStatus::EquipmentBreakLegs},
        {EquipmentSlot::Feet, network::EntityStatus::EquipmentBreakFeet},
    };

    for (const auto& tc : testCases) {
        m_world->clearRecords();
        m_living->onEquippedItemBroken(*Items::IRON_SWORD, tc.slot);

        ASSERT_EQ(m_world->statusRecords().size(), 1u)
            << "Expected 1 status record for slot " << static_cast<int>(tc.slot);
        EXPECT_EQ(m_world->statusRecords()[0].status, static_cast<u8>(tc.expectedStatus))
            << "Wrong status for slot " << static_cast<int>(tc.slot);
    }
}

// ============================================================================
// equipmentBreakStatus 映射测试
// ============================================================================

TEST_F(LivingEntityBreakEventTest, EquipmentBreakStatusMapping)
{
    // 验证 equipmentBreakStatus 的槽位到状态码映射
    using Status = network::EntityStatus;

    EXPECT_EQ(network::equipmentBreakStatus(0), Status::EquipmentBreakMainHand);
    EXPECT_EQ(network::equipmentBreakStatus(1), Status::EquipmentBreakOffHand);
    EXPECT_EQ(network::equipmentBreakStatus(5), Status::EquipmentBreakHead);
    EXPECT_EQ(network::equipmentBreakStatus(4), Status::EquipmentBreakChest);
    EXPECT_EQ(network::equipmentBreakStatus(3), Status::EquipmentBreakLegs);
    EXPECT_EQ(network::equipmentBreakStatus(2), Status::EquipmentBreakFeet);

    // 无效槽位应回退到 MainHand
    EXPECT_EQ(network::equipmentBreakStatus(99), Status::EquipmentBreakMainHand);
}

// ============================================================================
// equipmentBreakStatus 状态码值测试（与 MC 原版一致性）
// ============================================================================

TEST_F(LivingEntityBreakEventTest, EquipmentBreakStatusValuesMatchMC)
{
    // 验证状态码值与 MC 原版一致
    // MC 原版 LivingEntity.entityEventForEquipmentBreak 映射：
    // MainHand=47, OffHand=48, Head=49, Chest=50, Legs=51, Feet=52
    using Status = network::EntityStatus;

    EXPECT_EQ(static_cast<u8>(Status::EquipmentBreakMainHand), 47);
    EXPECT_EQ(static_cast<u8>(Status::EquipmentBreakOffHand), 48);
    EXPECT_EQ(static_cast<u8>(Status::EquipmentBreakHead), 49);
    EXPECT_EQ(static_cast<u8>(Status::EquipmentBreakChest), 50);
    EXPECT_EQ(static_cast<u8>(Status::EquipmentBreakLegs), 51);
    EXPECT_EQ(static_cast<u8>(Status::EquipmentBreakFeet), 52);
}
