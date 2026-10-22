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

// 快乐恶魂挽具（HarnessItem / HappyGhastEntity）交互链路测试：
//   - EntityTypeTags::CAN_EQUIP_HARNESS 标签定义与成员（vanilla happy_ghast）
//   - 右键装备：成体装备挽具并消耗物品；幼体/已装备/非挽具物品均不装备
//   - 装备后才能被玩家骑乘（vanilla HappyGhast.getControllingPassenger 前提）
//   - 剪刀剪下：挽具掉回世界、槽位清空、HARNESS_UNEQUIP 音效与 SHEAR 事件
//   - 乘客上限 4 与上下客音效

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/passive/happyghast/HappyGhastEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gameevent/GameEvents.hpp"

#include <memory>
#include <vector>

namespace mc {
namespace {

/**
 * @brief 快乐恶魂挽具测试世界：记录音效与游戏事件
 */
class HarnessTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] bool isClientSide() const override { return false; }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        (void)category;
        (void)position;
        (void)volume;
        (void)pitch;
        m_playedSounds.push_back(soundEventId);
    }

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        (void)context;
        (void)pos;
        m_gameEvents.push_back(&event);
    }

    [[nodiscard]] bool playedSound(const ResourceLocation& id) const
    {
        for (const auto& sound : m_playedSounds) {
            if (sound == id) {
                return true;
            }
        }
        return false;
    }

    /// 某个音效事件被播放的次数（用于验证"每个乘客只会触发一次"之类的语义）
    [[nodiscard]] i32 playCount(const ResourceLocation& id) const
    {
        i32 count = 0;
        for (const auto& sound : m_playedSounds) {
            if (sound == id) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] const std::vector<ResourceLocation>& sounds() const { return m_playedSounds; }

    [[nodiscard]] i32 gameEventCount() const { return static_cast<i32>(m_gameEvents.size()); }

private:
    std::vector<ResourceLocation> m_playedSounds;
    std::vector<const gameevent::GameEvent*> m_gameEvents;
};

class HappyGhastHarnessTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::initialize();
            VanillaBlocks::initialize();
            entity::VanillaEntities::registerAll();
            // CAN_EQUIP_HARNESS 等实体标签的成员必须在 initialize() 中填充，运行时查询才有内容。
            EntityTypeTags::initialize();
            s_initialized = true;
        }
    }

    void SetUp() override { m_world = std::make_unique<HarnessTestWorld>(); }

    [[nodiscard]] std::unique_ptr<HappyGhastEntity> makeGhast(EntityInstanceId id)
    {
        auto ghast = std::make_unique<HappyGhastEntity>(id, test::testEcsRegistry());
        ghast->setWorld(m_world.get());
        ghast->setTypeId(entity::EntityTypeKeys::HAPPY_GHAST);
        m_world->registerEntityForLookup(ghast.get());
        return ghast;
    }

    [[nodiscard]] std::unique_ptr<Player> makePlayer(EntityInstanceId id)
    {
        auto player = std::make_unique<Player>(id, "HarnessTester", test::testEcsRegistry());
        player->setWorld(m_world.get());
        m_world->registerEntityForLookup(player.get());
        return player;
    }

    std::unique_ptr<HarnessTestWorld> m_world;
};

// ============================================================================
// 标签
// ============================================================================

TEST_F(HappyGhastHarnessTest, CanEquipHarnessTagContainsHappyGhast)
{
    // 对齐 vanilla 1.21.11 EntityTypeTagsProvider.CAN_EQUIP_HARNESS = happy_ghast
    EXPECT_TRUE(EntityTypeTags::CAN_EQUIP_HARNESS().contains(ResourceLocation("minecraft:happy_ghast")));
    // 其它生物不允许装备挽具
    EXPECT_FALSE(EntityTypeTags::CAN_EQUIP_HARNESS().contains(ResourceLocation("minecraft:pig")));
}

// ============================================================================
// 右键装备
// ============================================================================

TEST_F(HappyGhastHarnessTest, InteractEquipsHarnessOnAdultAndConsumesItem)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    auto player = makePlayer(EntityInstanceId(2));
    player->setGameMode(GameMode::Survival);
    player->getHeldItem(Hand::MainHand) = ItemStack(Items::WHITE_HARNESS, 1);

    EXPECT_FALSE(ghast->hasHarness());

    const ActionResultType result = ghast->processInitialInteract(*player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(ghast->hasHarness());
    ASSERT_FALSE(ghast->harnessStack().isEmpty());
    EXPECT_EQ(ghast->harnessStack().getItem(), Items::WHITE_HARNESS);
    // 生存模式消耗一个挽具
    EXPECT_TRUE(player->getHeldItem(Hand::MainHand).isEmpty());
    // 装备音效（vanilla Equippable.harness: setEquipSound(HARNESS_EQUIP)）
    EXPECT_TRUE(m_world->playedSound(SoundEvents::HARNESS_EQUIP));
}

TEST_F(HappyGhastHarnessTest, InteractInCreativeKeepsHarnessItem)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    auto player = makePlayer(EntityInstanceId(2));
    player->setGameMode(GameMode::Creative);
    player->getHeldItem(Hand::MainHand) = ItemStack(Items::RED_HARNESS, 1);

    EXPECT_EQ(ghast->processInitialInteract(*player, Hand::MainHand), ActionResultType::Success);
    EXPECT_TRUE(ghast->hasHarness());
    // 创造模式不消耗
    EXPECT_FALSE(player->getHeldItem(Hand::MainHand).isEmpty());
}

TEST_F(HappyGhastHarnessTest, BabyCannotEquipHarness)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    ghast->setChild(true);
    auto player = makePlayer(EntityInstanceId(2));
    player->setGameMode(GameMode::Survival);
    player->getHeldItem(Hand::MainHand) = ItemStack(Items::WHITE_HARNESS, 1);

    // vanilla HappyGhast.canUseSlot(BODY) = isAlive() && !isBaby()：幼体拒绝装备
    EXPECT_NE(ghast->processInitialInteract(*player, Hand::MainHand), ActionResultType::Success);
    EXPECT_FALSE(ghast->hasHarness());
    EXPECT_FALSE(player->getHeldItem(Hand::MainHand).isEmpty());
}

TEST_F(HappyGhastHarnessTest, AlreadyEquippedHarnessIsNotReplaced)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    ghast->setEquipment(EquipmentSlot::Body, ItemStack(Items::WHITE_HARNESS, 1));
    auto player = makePlayer(EntityInstanceId(2));
    player->setGameMode(GameMode::Survival);
    player->getHeldItem(Hand::MainHand) = ItemStack(Items::BLACK_HARNESS, 1);

    // 已装备时不替换（vanilla 此时走骑乘分支，而不是重新装备）
    auto* harnessItem = const_cast<Item*>(Items::BLACK_HARNESS);
    EXPECT_FALSE(
        harnessItem->itemInteractionForEntity(player->getHeldItem(Hand::MainHand), *player, *ghast, Hand::MainHand));
    EXPECT_EQ(ghast->harnessStack().getItem(), Items::WHITE_HARNESS);
    EXPECT_FALSE(player->getHeldItem(Hand::MainHand).isEmpty());
}

TEST_F(HappyGhastHarnessTest, NonHarnessEntityRejectsHarnessEquip)
{
    // 猪不在 CAN_EQUIP_HARNESS 标签中：物品侧拒绝装备，也不消耗。
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(3), test::testEcsRegistry());
    pig->setWorld(m_world.get());
    auto player = makePlayer(EntityInstanceId(2));
    player->setGameMode(GameMode::Survival);
    player->getHeldItem(Hand::MainHand) = ItemStack(Items::WHITE_HARNESS, 1);

    auto* harnessItem = const_cast<Item*>(Items::WHITE_HARNESS);
    EXPECT_FALSE(
        harnessItem->itemInteractionForEntity(player->getHeldItem(Hand::MainHand), *player, *pig, Hand::MainHand));
    EXPECT_TRUE(pig->getEquipment(EquipmentSlot::Body).isEmpty());
    EXPECT_FALSE(player->getHeldItem(Hand::MainHand).isEmpty());

    // 实体侧同样不应把挽具当作可装备物品（canEquipItemOnInteract 为实体标签驱动）
    EXPECT_FALSE(pig->canEquipItemOnInteract(ItemStack(Items::WHITE_HARNESS, 1), EquipmentSlot::Body));
}

// ============================================================================
// 骑乘
// ============================================================================

TEST_F(HappyGhastHarnessTest, CannotBeRiddenWithoutHarness)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    auto player = makePlayer(EntityInstanceId(2));

    // 未装挽具：右键不会骑上，也不可被骑
    ghast->processInitialInteract(*player, Hand::MainHand);
    EXPECT_FALSE(player->startRiding(*ghast));
    EXPECT_FALSE(player->isRiding());
}

TEST_F(HappyGhastHarnessTest, EquippedHarnessAllowsRiding)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    ghast->setEquipment(EquipmentSlot::Body, ItemStack(Items::WHITE_HARNESS, 1));
    auto player = makePlayer(EntityInstanceId(2));

    EXPECT_TRUE(ghast->canBeRidden(*player));
    const ActionResultType result = ghast->processInitialInteract(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(player->isRiding());
    EXPECT_EQ(player->getVehicle(), ghast->id());
    EXPECT_EQ(static_cast<i32>(ghast->getPassengers().size()), 1);
    // 首个乘客上车播放护目镜放下音效（vanilla HappyGhast.addPassenger）
    EXPECT_TRUE(m_world->playedSound(SoundEvents::HARNESS_GOGGLES_DOWN));
}

TEST_F(HappyGhastHarnessTest, SneakingPlayerDoesNotMount)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    ghast->setEquipment(EquipmentSlot::Body, ItemStack(Items::WHITE_HARNESS, 1));
    auto player = makePlayer(EntityInstanceId(2));
    player->setSneaking(true);

    // vanilla：isSecondaryUseActive()（潜行）时不骑乘
    ghast->processInitialInteract(*player, Hand::MainHand);
    EXPECT_FALSE(player->isRiding());
}

TEST_F(HappyGhastHarnessTest, MaxPassengersIsFour)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    ghast->setEquipment(EquipmentSlot::Body, ItemStack(Items::WHITE_HARNESS, 1));
    EXPECT_EQ(ghast->getMaxPassengers(), 4);

    std::vector<std::unique_ptr<Player>> players;
    players.reserve(5);
    for (i32 i = 0; i < 5; ++i) {
        auto player = makePlayer(EntityInstanceId(100 + i));
        EXPECT_EQ(player->startRiding(*ghast), i < 4) << "第 " << (i + 1) << " 名乘客的上车结果不符合预期";
        players.push_back(std::move(player));
    }
    EXPECT_EQ(static_cast<i32>(ghast->getPassengers().size()), 4);
}

// ============================================================================
// 剪下挽具
// ============================================================================

TEST_F(HappyGhastHarnessTest, ShearsRemoveHarnessAndPlayUnequipSound)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    ghast->setEquipment(EquipmentSlot::Body, ItemStack(Items::BLUE_HARNESS, 1));
    auto player = makePlayer(EntityInstanceId(2));
    player->setGameMode(GameMode::Survival);
    player->getHeldItem(Hand::MainHand) = ItemStack(Items::SHEARS, 1);

    const ActionResultType result = ghast->processInitialInteract(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_FALSE(ghast->hasHarness());
    // vanilla Equippable.harness: setCanBeSheared(true) + setShearingSound(HARNESS_UNEQUIP)
    EXPECT_TRUE(m_world->playedSound(SoundEvents::HARNESS_UNEQUIP));
    // 挽具被剪下后不再可骑
    EXPECT_FALSE(ghast->canBeRidden(*player));
}

TEST_F(HappyGhastHarnessTest, CanReEquipAfterShearing)
{
    // 回归：剪下挽具后 Body 槽必须真正清空，否则 HarnessItem 会因"已装备"拒绝再次装备
    auto ghast = makeGhast(EntityInstanceId(1));
    ghast->setEquipment(EquipmentSlot::Body, ItemStack(Items::GREEN_HARNESS, 1));
    auto player = makePlayer(EntityInstanceId(2));
    player->setGameMode(GameMode::Survival);

    player->getHeldItem(Hand::MainHand) = ItemStack(Items::SHEARS, 1);
    ASSERT_EQ(ghast->processInitialInteract(*player, Hand::MainHand), ActionResultType::Success);
    ASSERT_FALSE(ghast->hasHarness());
    EXPECT_EQ(m_world->gameEventCount(), 1) << "剪下装备应触发一次 SHEAR 游戏事件";

    player->getHeldItem(Hand::MainHand) = ItemStack(Items::BLACK_HARNESS, 1);
    EXPECT_EQ(ghast->processInitialInteract(*player, Hand::MainHand), ActionResultType::Success);
    EXPECT_EQ(ghast->harnessStack().getItem(), Items::BLACK_HARNESS);
}

} // namespace
} // namespace mc