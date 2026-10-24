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
#include "common/entity/ai/goal/goals/special/GhastGoals.hpp"
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
#include <unordered_map>
#include <vector>

namespace mc {
namespace {

/**
 * @brief 快乐恶魂挽具测试世界：记录音效与游戏事件
 */
class HarnessTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] bool isClientSide() const override
    {
        return false;
    }

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

    /// 按 UUID 反查实体（供拴绳跟随目标解析）：测试世界只登记少量实体，线性扫描即可。
    [[nodiscard]] Entity* getEntityByUuid(const std::string& uuid) override
    {
        for (Entity* entity : m_knownEntities) {
            if (entity != nullptr && entity->uuid() == uuid) {
                return entity;
            }
        }
        return nullptr;
    }

    void setKnownEntities(std::vector<Entity*> entities)
    {
        m_knownEntities = std::move(entities);
    }

    /// 世界中可见的玩家列表（供快乐恶魂 scanPlayerAbove 扫描）
    [[nodiscard]] std::vector<Entity*> getPlayers() const override
    {
        return m_players;
    }

    void setPlayers(std::vector<Entity*> players)
    {
        m_players = std::move(players);
    }

    /// 该位置是否正在降水（供 continuousHeal 判定）
    [[nodiscard]] world::biome::BiomeClimate::Precipitation precipitationAt(const BlockPos& pos) const override
    {
        (void)pos;
        return m_precipitation;
    }

    void setPrecipitation(world::biome::BiomeClimate::Precipitation precipitation)
    {
        m_precipitation = precipitation;
    }

    [[nodiscard]] bool isInCloudLayer(f64, f64) const override
    {
        return m_inClouds;
    }

    void setInCloudLayer(bool inClouds)
    {
        m_inClouds = inClouds;
    }

    [[nodiscard]] const std::vector<ResourceLocation>& sounds() const
    {
        return m_playedSounds;
    }

    [[nodiscard]] i32 gameEventCount() const
    {
        return static_cast<i32>(m_gameEvents.size());
    }

private:
    std::vector<Entity*> m_knownEntities;
    std::vector<ResourceLocation> m_playedSounds;
    std::vector<const gameevent::GameEvent*> m_gameEvents;
    std::vector<Entity*> m_players;
    world::biome::BiomeClimate::Precipitation m_precipitation = world::biome::BiomeClimate::Precipitation::None;
    bool m_inClouds = false;
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

    void SetUp() override
    {
        m_world = std::make_unique<HarnessTestWorld>();
    }

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

// ============================================================================
// 静止超时（serverStillTimeout）
// ============================================================================

TEST_F(HappyGhastHarnessTest, PlayerAboveGhastIsDetected)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    auto player = makePlayer(EntityInstanceId(2));
    ghast->setPosition(0.0, 64.0f, 0.0);
    ASSERT_FALSE(ghast->scanPlayerAbove()) << "无玩家时不应扫描到上方玩家";

    // 玩家站在快乐恶魂的顶面上方（成体高 4 格，顶面 y=68，扫描区域向上延伸到 70）
    player->setPosition(0.0f, 68.5f, 0.0f);
    m_world->setPlayers({player.get()});
    EXPECT_TRUE(ghast->scanPlayerAbove());

    // 高出扫描区域（顶面 + 2 格以上）则不再命中
    player->setPosition(0.0f, 71.0f, 0.0f);
    EXPECT_FALSE(ghast->scanPlayerAbove());
}

TEST_F(HappyGhastHarnessTest, SpectatorAboveDoesNotCount)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    auto player = makePlayer(EntityInstanceId(2));
    ghast->setPosition(0.0, 64.0f, 0.0);
    player->setPosition(0.0f, 68.5f, 0.0f);
    player->setGameMode(GameMode::Spectator);
    m_world->setPlayers({player.get()});

    // 对齐 vanilla scanPlayerAboveGhast：!player.isSpectator()
    EXPECT_FALSE(ghast->scanPlayerAbove());
}

TEST_F(HappyGhastHarnessTest, StayStillDisablesControllingPassenger)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    ghast->setEquipment(EquipmentSlot::Body, ItemStack(Items::WHITE_HARNESS, 1));
    auto player = makePlayer(EntityInstanceId(2));
    ASSERT_TRUE(player->startRiding(*ghast));
    // startRiding → onPassengerAdded 只在"上方无玩家"时清除静止超时；此处上方有玩家，
    // 按 vanilla 该状态保持为 0（不设置），直到 tick 检测到上方玩家再压到 10。
    ASSERT_EQ(ghast->serverStillTimeout(), 0);

    // 有玩家（未骑乘，根载具即其本身）停在快乐恶魂上方时，静止超时被持续重置为 10，
    // 此时乘客可驾驶。vanilla 扫描的是玩家"根载具"坐标，故用独立玩家实体表达该场景。
    auto stander = makePlayer(EntityInstanceId(3));
    m_world->setPlayers({stander.get()});
    // 快乐恶魂顶面 y=68，扫描区向上延伸 2 格 → 站立玩家落在 68..70 内
    stander->setPosition(0.0f, ghast->boundingBox().maxY + 0.5f, 0.0f);
    ghast->tick();
    EXPECT_TRUE(ghast->isOnStillTimeout()) << "上方有玩家时静止超时应被重置为 10";
    EXPECT_EQ(ghast->getControllingPassenger(), player->id());

    // 玩家离开上方后（tickCount 已过 60），静止超时每 tick 递减直至归零，
    // 归零后快乐恶魂不再接受该乘客操控（vanilla 同序）。
    m_world->setPlayers({});
    stander->setPosition(200.0f, 64.0f, 200.0f);
    i32 ticksUntilReleased = 0;
    while (ghast->isOnStillTimeout() && ticksUntilReleased < 200) {
        ghast->tick();
        ++ticksUntilReleased;
    }
    EXPECT_LT(ticksUntilReleased, 200) << "玩家离开上方后静止超时必须最终归零";
    EXPECT_FALSE(ghast->isOnStillTimeout());
}

TEST_F(HappyGhastHarnessTest, DismountSetsStillTimeout)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    ghast->setEquipment(EquipmentSlot::Body, ItemStack(Items::WHITE_HARNESS, 1));
    auto player = makePlayer(EntityInstanceId(2));
    ASSERT_TRUE(player->startRiding(*ghast));
    EXPECT_EQ(ghast->serverStillTimeout(), 0);

    // 对齐 vanilla removePassenger：任何乘客下车都把静止超时设为 10
    player->stopRiding();
    EXPECT_EQ(ghast->serverStillTimeout(), 10);
}

// ============================================================================
// 拴绳牵引
// ============================================================================

TEST_F(HappyGhastHarnessTest, LeashedGhastFollowsHolderInsteadOfWandering)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    auto player = makePlayer(EntityInstanceId(2));

    ghast->setPosition(0.0f, 64.0f, 0.0f);
    player->setPosition(0.0f, 64.0f, 30.0f);
    ghast->setLeashedToEntity(player->uuid());
    m_world->setKnownEntities({ghast.get(), player.get()});

    entity::ai::goal::GhastFollowLeashHolderGoal leashGoal(ghast.get());
    entity::ai::goal::GhastRandomFlyGoal wanderGoal(ghast.get());

    EXPECT_TRUE(leashGoal.shouldExecute()) << "离持有者超过跟随距离时应由拴绳牵引";
    // 被拴在实体上时不走随机巡游（vanilla IDLE 的随机步行不接管拴绳实体）
    EXPECT_FALSE(wanderGoal.shouldExecute());

    // 拉近到跟随距离内：放弃牵引
    player->setPosition(0.0f, 64.0f, 2.0f);
    EXPECT_FALSE(leashGoal.shouldExecute());
}

// ============================================================================
// 幼体/成体切换（ageBoundaryReached）
// ============================================================================

TEST_F(HappyGhastHarnessTest, BabyHasNoGoalsAndAdultRestoresThem)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    // 成体：注册了 Float/Tempt/RandomFly 三个目标
    const std::size_t adultGoals = ghast->goalSelector().getAllGoals().size();
    EXPECT_GT(adultGoals, 0u);

    ghast->setChild(true);
    EXPECT_EQ(ghast->goalSelector().getAllGoals().size(), 0u) << "幼体不应持有任何目标（vanilla babyGhastSetup）";
    // 幼体切回成体时恢复目标集合
    ghast->setChild(false);
    EXPECT_EQ(ghast->goalSelector().getAllGoals().size(), adultGoals);
}

TEST_F(HappyGhastHarnessTest, BabyTransitionClearsStillTimeout)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    auto player = makePlayer(EntityInstanceId(2));
    ghast->setEquipment(EquipmentSlot::Body, ItemStack(Items::WHITE_HARNESS, 1));
    ASSERT_TRUE(player->startRiding(*ghast));
    player->stopRiding();
    ASSERT_EQ(ghast->serverStillTimeout(), 10);

    ghast->setChild(true);
    EXPECT_EQ(ghast->serverStillTimeout(), 0) << "转为幼体时静止超时应清零（vanilla babyGhastSetup）";
}

// ============================================================================
// 降水回血
// ============================================================================

TEST_F(HappyGhastHarnessTest, PrecipitationHealsEveryTwentyTicks)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    ghast->setHealth(10.0f);
    m_world->setPrecipitation(world::biome::BiomeClimate::Precipitation::Rain);

    // 对齐 vanilla continuousHeal：降水时 tickCount % 20 == 0 回 1 点
    for (i32 i = 0; i < 20; ++i) {
        ghast->tick();
    }
    EXPECT_GT(ghast->health(), 10.0f);
}

TEST_F(HappyGhastHarnessTest, NoPrecipitationHealsSlowly)
{
    auto ghast = makeGhast(EntityInstanceId(1));
    ghast->setHealth(10.0f);

    // 无云层、无降水：600 tick 才回 1 点，前 100 tick 不应回血
    for (i32 i = 0; i < 100; ++i) {
        ghast->tick();
    }
    EXPECT_FLOAT_EQ(ghast->health(), 10.0f);
}

} // namespace
} // namespace mc