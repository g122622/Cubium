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
 * @file DolphinEntityTest.cpp
 * @brief 海豚实体单元测试
 *
 * 参考 MC 1.21.11 net.minecraft.world.entity.animal.dolphin.Dolphin
 *
 * 覆盖场景：
 * 1. isFoodItem: 检查是否是食物（鱼类）
 * 2. 维度和属性测试
 * 3. 宝藏寻找状态（setTreasurePos/clearTreasureTarget/引导玩家）
 * 4. 湿润度（MOISTNESS_LEVEL）业务联动：水/雨回满、离水递减、归零干燥伤害
 * 5. 陆地挣扎：离水且踩地时随机弹跳（向上速度 > 0、脱离地面）
 * 6. 永不溺水：updateAirSupply 覆盖后离水空气不消耗（vanilla handleAirSupply 空实现语义）
 * 7. NBT 保存/加载往返（GotFish/Moistness）
 */

#include "entity/entities/passive/water/DolphinEntity.hpp"
#include "entity/core/AgeableEntity.hpp"
#include "entity/entities/passive/water/AgeableWaterCreatureEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/fluid/Fluids.hpp"

using namespace mc;
using namespace mc::entity::serialization;

namespace {

/**
 * @brief 测试用世界桩：可配置降雨状态与地面碰撞
 *
 * isInRain() 依赖 IWorld::isRaining() 与 canRainAt()，BaseTestWorld 未覆写，
 * 这里显式提供以便测试湿润度的"雨中回满"分支。
 * checkOnGround() 经 IWorld::hasBlockCollision() 探测地面，若恒返回 false 则
 * 每 tick 后 onGround 被重置为 false，无法测试陆地挣扎分支，故这里可配置为 true。
 */
class DolphinTestWorld final : public mc::test::BaseTestWorld {
public:
    void setRaining(bool raining)
    {
        m_raining = raining;
    }

    void setGroundCollision(bool collides)
    {
        m_groundCollision = collides;
    }

    void setWaterImmersion(bool inWater)
    {
        m_waterImmersion = inWater;
    }

    [[nodiscard]] bool isRaining() const override
    {
        return m_raining;
    }

    [[nodiscard]] bool canRainAt(const BlockPos& /*pos*/) const override
    {
        return m_raining;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& /*box*/) const override
    {
        return m_groundCollision;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 /*x*/, i32 /*y*/, i32 /*z*/) const override
    {
        // 浸水时返回水方块流体，使 updateEnvironmentState() 将 m_inWater 置 true。
        if (m_waterImmersion) {
            return &fluid::Fluids::WATER()->defaultState();
        }
        return fluid::Fluids::EMPTY() != nullptr ? &fluid::Fluids::EMPTY()->defaultState() : nullptr;
    }

private:
    bool m_raining = false;
    bool m_groundCollision = false;
    bool m_waterImmersion = false;
};

/**
 * @brief 测试用海豚子类，暴露 protected 方法供单元测试调用
 *
 * addAdditionalSaveData / readAdditionalSaveData 在实体中为 protected（对齐 vanilla），
 * 测试子类通过 using 声明提升为 public 以便直接测试 NBT 序列化。
 */
class TestableDolphinEntity : public DolphinEntity {
public:
    using DolphinEntity::DolphinEntity;

    using DolphinEntity::addAdditionalSaveData;
    using DolphinEntity::interactMob;
    using DolphinEntity::readAdditionalSaveData;
};

} // namespace

// ==================== DolphinEntity Test Fixture ====================

class DolphinEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化物品注册表（喂食交互测试需 Items::COD 等非空）
        Items::ensureInitialized();
        // 创建海豚实体
        dolphin = std::make_unique<TestableDolphinEntity>(EntityInstanceId(0), mc::test::testEcsRegistry());
    }

    void TearDown() override
    {
        dolphin.reset();
    }

    std::unique_ptr<TestableDolphinEntity> dolphin;
};

// ==================== isFoodItem Tests ====================

TEST_F(DolphinEntityTest, IsFoodItem_Cod_ReturnsTrue)
{
    // 鳕鱼是海豚的食物
    if (Items::COD != nullptr) {
        ItemStack codStack(Items::COD, 1);
        EXPECT_TRUE(dolphin->isFoodItem(codStack));
    }
}

TEST_F(DolphinEntityTest, IsFoodItem_Salmon_ReturnsTrue)
{
    // 鲑鱼是海豚的食物
    if (Items::SALMON != nullptr) {
        ItemStack salmonStack(Items::SALMON, 1);
        EXPECT_TRUE(dolphin->isFoodItem(salmonStack));
    }
}

TEST_F(DolphinEntityTest, IsFoodItem_Pufferfish_ReturnsTrue)
{
    // 河豚是海豚的食物
    if (Items::PUFFERFISH != nullptr) {
        ItemStack pufferfishStack(Items::PUFFERFISH, 1);
        EXPECT_TRUE(dolphin->isFoodItem(pufferfishStack));
    }
}

TEST_F(DolphinEntityTest, IsFoodItem_TropicalFish_ReturnsTrue)
{
    // 热带鱼是海豚的食物
    if (Items::TROPICAL_FISH != nullptr) {
        ItemStack tropicalFishStack(Items::TROPICAL_FISH, 1);
        EXPECT_TRUE(dolphin->isFoodItem(tropicalFishStack));
    }
}

TEST_F(DolphinEntityTest, IsFoodItem_NonFish_ReturnsFalse)
{
    // 非鱼类物品不是海豚的食物
    if (Items::DIAMOND != nullptr) {
        ItemStack diamondStack(Items::DIAMOND, 1);
        EXPECT_FALSE(dolphin->isFoodItem(diamondStack));
    }
}

// ==================== Attribute Tests ====================

TEST_F(DolphinEntityTest, Attributes_HasCorrectValues)
{
    // 海豚应该有正确的属性值
    EXPECT_FLOAT_EQ(dolphin->maxHealth(), 10.0f);
}

TEST_F(DolphinEntityTest, Dimensions_HasCorrectValues)
{
    EXPECT_FLOAT_EQ(dolphin->width(), 0.9f);
    EXPECT_FLOAT_EQ(dolphin->height(), 0.6f);
}

// ==================== Treasure Finding Tests ====================

TEST_F(DolphinEntityTest, TreasurePos_SetAndGet)
{
    BlockPos pos(100, 64, 200);
    dolphin->setTreasurePos(pos);

    EXPECT_TRUE(dolphin->hasTreasureTarget());
    EXPECT_EQ(dolphin->getTreasurePos(), pos);
}

TEST_F(DolphinEntityTest, TreasurePos_Clear)
{
    BlockPos pos(100, 64, 200);
    dolphin->setTreasurePos(pos);
    EXPECT_TRUE(dolphin->hasTreasureTarget());

    dolphin->clearTreasureTarget();
    EXPECT_FALSE(dolphin->hasTreasureTarget());
}

TEST_F(DolphinEntityTest, GuidingPlayer_SetAndGet)
{
    dolphin->setGuidingPlayer(true, 12345ULL);

    EXPECT_TRUE(dolphin->isGuidingPlayer());
    EXPECT_EQ(dolphin->getGuidedPlayerId(), 12345ULL);
}

TEST_F(DolphinEntityTest, GuidingPlayer_Clear)
{
    dolphin->setGuidingPlayer(true, 12345ULL);
    EXPECT_TRUE(dolphin->isGuidingPlayer());

    dolphin->clearTreasureTarget();
    EXPECT_FALSE(dolphin->isGuidingPlayer());
}

// ==================== Eye Height Tests ====================

TEST_F(DolphinEntityTest, EyeHeight_CorrectValue)
{
    EXPECT_FLOAT_EQ(dolphin->eyeHeight(), 0.3f);
}

// ==================== Moistness (湿润度) Tests ====================

TEST_F(DolphinEntityTest, Moistness_DefaultIsFullLevel)
{
    // vanilla Dolphin MOISTNESS_LEVEL 默认 2400
    EXPECT_EQ(dolphin->getMoistnessLevel(), 2400);
}

TEST_F(DolphinEntityTest, Moistness_SetAndGet)
{
    dolphin->setMoistnessLevel(1234);
    EXPECT_EQ(dolphin->getMoistnessLevel(), 1234);
}

TEST_F(DolphinEntityTest, Moistness_DecrementsWhenOutOfWater)
{
    DolphinTestWorld world;
    dolphin->setWorld(&world);
    dolphin->setMoistnessLevel(2400);

    dolphin->tick();
    EXPECT_EQ(dolphin->getMoistnessLevel(), 2399);

    dolphin->tick();
    EXPECT_EQ(dolphin->getMoistnessLevel(), 2398);
}

TEST_F(DolphinEntityTest, Moistness_RefillsWhenInWater)
{
    DolphinTestWorld world;
    world.setWaterImmersion(true);
    dolphin->setWorld(&world);
    dolphin->setMoistnessLevel(100);

    dolphin->tick();
    EXPECT_EQ(dolphin->getMoistnessLevel(), 2400);
}

TEST_F(DolphinEntityTest, Moistness_RefillsWhenInRain)
{
    DolphinTestWorld world;
    world.setRaining(true);
    dolphin->setWorld(&world);
    dolphin->setMoistnessLevel(100);

    dolphin->tick();
    // 雨中（isInWaterOrRain）回满，不递减
    EXPECT_EQ(dolphin->getMoistnessLevel(), 2400);
}

TEST_F(DolphinEntityTest, Moistness_ZeroCauseDryoutDamage)
{
    DolphinTestWorld world;
    dolphin->setWorld(&world);
    // LivingEntity 构造后 health 为基类默认 20，须先对齐 maxHealth（10）再建立基线
    dolphin->setHealth(dolphin->maxHealth());
    dolphin->setMoistnessLevel(0);

    const f32 healthBefore = dolphin->health();
    dolphin->tick();

    // 湿润度归零后受 dryout 伤害（每次 1 点）
    EXPECT_LT(dolphin->health(), healthBefore);
    EXPECT_FLOAT_EQ(healthBefore - dolphin->health(), 1.0f);
}

TEST_F(DolphinEntityTest, Moistness_AboveZeroNoDamage)
{
    DolphinTestWorld world;
    dolphin->setWorld(&world);
    dolphin->setHealth(dolphin->maxHealth());
    // 递减后仍 > 0（1 > 0）时不造成伤害；vanilla 判定在递减后用 <= 0 门控
    dolphin->setMoistnessLevel(2);

    const f32 healthBefore = dolphin->health();
    dolphin->tick();

    EXPECT_FLOAT_EQ(dolphin->health(), healthBefore);
    EXPECT_EQ(dolphin->getMoistnessLevel(), 1);
}

TEST_F(DolphinEntityTest, Moistness_HitsZeroAfterDecrementCausesDamage)
{
    DolphinTestWorld world;
    dolphin->setWorld(&world);
    dolphin->setHealth(dolphin->maxHealth());
    // level=1 递减到 0，vanilla 在递减后以 <= 0 门控，故本条即触发伤害
    dolphin->setMoistnessLevel(1);

    const f32 healthBefore = dolphin->health();
    dolphin->tick();

    EXPECT_EQ(dolphin->getMoistnessLevel(), 0);
    EXPECT_FLOAT_EQ(healthBefore - dolphin->health(), 1.0f);
}

// ==================== 陆地挣扎（onGround 随机弹跳） Tests ====================

TEST_F(DolphinEntityTest, LandStruggle_OnGroundGivesUpwardVelocity)
{
    DolphinTestWorld world;
    // 世界报告地面碰撞，使 baseTick 的 checkOnGround() 得出 onGround=true
    world.setGroundCollision(true);
    dolphin->setWorld(&world);
    dolphin->setInWater(false);
    dolphin->setMoistnessLevel(2400);
    dolphin->setVelocity(0.0f, 0.0f, 0.0f);

    dolphin->tick();

    // 陆地挣扎：获得向上速度（vanilla +0.5）并脱离地面
    EXPECT_GT(dolphin->velocity().y, 0.0f);
    EXPECT_FALSE(dolphin->onGround());
}

TEST_F(DolphinEntityTest, LandStruggle_AirborneNoExtraJump)
{
    DolphinTestWorld world;
    dolphin->setWorld(&world);
    dolphin->setInWater(false);
    dolphin->setMoistnessLevel(2400);
    // 不在空中踩地时不应施加挣扎弹跳（onGround 为 false）
    dolphin->setOnGround(false);
    dolphin->setVelocity(0.0f, 0.0f, 0.0f);

    dolphin->tick();

    // 空中不主动施加向上速度（重力可能使其为负，但不含 +0.5 弹跳）
    EXPECT_LE(dolphin->velocity().y, 0.0f);
}

// ==================== 永不溺水（updateAirSupply 覆盖） Tests ====================

TEST_F(DolphinEntityTest, AirSupply_NotDepletedOutOfWater)
{
    DolphinTestWorld world;
    dolphin->setWorld(&world);
    dolphin->setInWater(false);
    dolphin->setAir(100);

    // 离水连续 tick，空气不应被消耗（vanilla Dolphin.handleAirSupply 空实现）
    for (i32 i = 0; i < 20; ++i) {
        dolphin->tick();
    }

    EXPECT_EQ(dolphin->air(), dolphin->maxAir());
}

TEST_F(DolphinEntityTest, AirSupply_DoesNotDrownOnLand)
{
    DolphinTestWorld world;
    dolphin->setWorld(&world);
    dolphin->setHealth(dolphin->maxHealth());
    dolphin->setAir(0);
    // 湿润度保持满值，使离水期间只有呼吸逻辑可能造成伤害
    dolphin->setMoistnessLevel(2400);

    const f32 healthBefore = dolphin->health();

    // 离水 tick 超过继承溺水阈值（空气 <= -20 约需 20+ tick），
    // 若退化为 WaterMobEntity::updateAirSupply 则此处会触发溺水伤害。
    for (i32 i = 0; i < 40; ++i) {
        dolphin->setMoistnessLevel(2400); // 保持湿润度，隔离变量
        dolphin->tick();
    }

    EXPECT_FLOAT_EQ(dolphin->health(), healthBefore);
    // 空气恒维持满值（vanilla increaseAirSupply 语义）
    EXPECT_EQ(dolphin->air(), dolphin->maxAir());
}

// ==================== NBT 序列化 Tests ====================

TEST_F(DolphinEntityTest, NbtRoundTrip_GotFishAndMoistness)
{
    dolphin->setGotFish(true);
    dolphin->setMoistnessLevel(777);

    nbt::tags::compound_tag tag;
    dolphin->addAdditionalSaveData(tag);

    const auto gotFish = nbt_helper::tryGetBool(tag, nbt_keys::DOLPHIN_GOT_FISH);
    ASSERT_TRUE(gotFish.has_value());
    EXPECT_TRUE(*gotFish);

    const auto moistness = nbt_helper::tryGetInt(tag, nbt_keys::DOLPHIN_MOISTNESS);
    ASSERT_TRUE(moistness.has_value());
    EXPECT_EQ(*moistness, 777);

    TestableDolphinEntity loaded(EntityInstanceId(3), mc::test::testEcsRegistry());
    auto result = loaded.readAdditionalSaveData(tag);
    ASSERT_TRUE(result.success()) << result.error().toString();
    EXPECT_TRUE(loaded.hasGotFish());
    EXPECT_EQ(loaded.getMoistnessLevel(), 777);
}

TEST_F(DolphinEntityTest, NbtRoundTrip_EmptyNbtUsesDefault)
{
    nbt::tags::compound_tag emptyTag;

    TestableDolphinEntity loaded(EntityInstanceId(3), mc::test::testEcsRegistry());
    auto result = loaded.readAdditionalSaveData(emptyTag);
    ASSERT_TRUE(result.success()) << result.error().toString();
    EXPECT_FALSE(loaded.hasGotFish());
    EXPECT_EQ(loaded.getMoistnessLevel(), 2400);
}

// ==================== 同步数据注册 Tests ====================

TEST_F(DolphinEntityTest, Moistness_SyncsToDataManager)
{
    // 先清除脏标记，确保后续断言只反映本次 setMoistnessLevel 的写入
    dolphin->dataManager().clearDirty();

    dolphin->setMoistnessLevel(555);
    EXPECT_EQ(dolphin->getMoistnessLevel(), 555);
    // 元数据同步：setMoistnessLevel 须写入 DATA_MOISTNESS_LEVEL_PARAM（id18）本身的脏值，
    // 而不仅是"存在任意脏数据"。经 getMoistnessLevel 之外的路径校验 id18 的写入结果。
    EXPECT_TRUE(dolphin->dataManager().hasDirtyData());
    const auto rawMoistness = dolphin->dataManager().getRaw(DolphinEntity::getMoistnessLevelParamId());
    ASSERT_TRUE(rawMoistness.has_value());
    EXPECT_EQ(rawMoistness->get<i32>(), 555);
}

// ==================== 幼体语义 Tests（vanilla AgeableWaterCreature→AgeableMob） ====================

TEST_F(DolphinEntityTest, BabyState_DefaultIsAdult)
{
    // 默认是成体（vanilla AgeableMob 起始 age=0）
    EXPECT_FALSE(dolphin->isChild());
    EXPECT_EQ(dolphin->getGrowingAge(), 0);
}

TEST_F(DolphinEntityTest, BabyState_SetChildTrue_BecomesChild)
{
    dolphin->setChild(true);
    EXPECT_TRUE(dolphin->isChild());
    EXPECT_EQ(dolphin->getGrowingAge(), AgeableEntity::BABY_AGE); // -24000

    dolphin->setChild(false);
    EXPECT_FALSE(dolphin->isChild());
    EXPECT_EQ(dolphin->getGrowingAge(), AgeableEntity::MAX_AGE); // 0
}

TEST_F(DolphinEntityTest, BabyState_SyncsToDataManager)
{
    const u16 babyId = AgeableWaterCreatureEntity::getBabyParamId();
    EXPECT_EQ(babyId, 16u); // vanilla AgeableMob.DATA_BABY id16

    dolphin->dataManager().clearDirty();
    dolphin->setChild(true);
    // 幼体状态写入 DATA_BABY（id16），供客户端渲染幼体模型
    const auto raw = dolphin->dataManager().getRaw(babyId);
    ASSERT_TRUE(raw.has_value());
    EXPECT_TRUE(raw->get<bool>());
}

TEST_F(DolphinEntityTest, BabyState_AgeUpGrowsToAdult)
{
    dolphin->setChild(true);
    ASSERT_TRUE(dolphin->isChild());

    // ageUp(1200) = 24000 tick，恰好抵达成体
    dolphin->ageUp(1200);
    EXPECT_FALSE(dolphin->isChild());
    EXPECT_EQ(dolphin->getGrowingAge(), AgeableEntity::MAX_AGE);
}

TEST_F(DolphinEntityTest, BabyState_NbtRoundTrip)
{
    dolphin->setChild(true);
    const i32 babyAge = dolphin->getGrowingAge();

    nbt::tags::compound_tag tag;
    dolphin->addAdditionalSaveData(tag);

    TestableDolphinEntity loaded(EntityInstanceId(9), mc::test::testEcsRegistry());
    auto result = loaded.readAdditionalSaveData(tag);
    ASSERT_TRUE(result.success()) << result.error().toString();
    EXPECT_TRUE(loaded.isChild());
    EXPECT_EQ(loaded.getGrowingAge(), babyAge);
}

TEST_F(DolphinEntityTest, SpeedUpSecondsWhenFeeding_MatchesVanilla)
{
    // vanilla AgeableMob.getSpeedUpSecondsWhenFeeding(x) = (int)(x / 20 * 0.1F)
    // 满幼体（-24000）→ 24000/20*0.1 = 120 秒
    EXPECT_EQ(AgeableEntity::getSpeedUpSecondsWhenFeeding(24000), 120);
    // 半成长（-12000）→ 12000/20*0.1 = 60 秒
    EXPECT_EQ(AgeableEntity::getSpeedUpSecondsWhenFeeding(12000), 60);
}

TEST_F(DolphinEntityTest, BabyFeeding_ChildDolphinAcceleratesGrowthNotGotFish)
{
    // 交互行为验证（对齐 vanilla Dolphin.mobInteract isBaby() 分支）：
    // 幼体喂鱼 → 加速成长（ageUp），且不置 GOT_FISH。
    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.abilities().creativeMode = false;
    player.inventory().setItem(0, ItemStack(Items::COD, 10));
    player.inventory().setSelectedSlot(0);

    DolphinTestWorld localWorld;
    dolphin->setWorld(&localWorld);
    dolphin->setTypeId("minecraft:dolphin");
    dolphin->setChild(true);
    const i32 beforeAge = dolphin->getGrowingAge(); // -24000
    ASSERT_TRUE(dolphin->isChild());

    const ActionResultType result = dolphin->interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_GT(dolphin->getGrowingAge(), beforeAge) << "Feeding a child dolphin should advance its growing age";
    EXPECT_TRUE(dolphin->isChild()) << "A single feeding should not fully mature the dolphin";
    EXPECT_FALSE(dolphin->hasGotFish()) << "A child dolphin should not be marked GOT_FISH (vanilla isBaby branch)";
    EXPECT_EQ(player.inventory().getItem(0).getCount(), 9) << "One fish should be consumed";
}

TEST_F(DolphinEntityTest, BabyFeeding_AdultDolphinSetsGotFishNotGrowth)
{
    // 对照组（对齐 vanilla else 分支）：成体喂鱼 → setGotFish(true)，年龄不变。
    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player.abilities().creativeMode = false;
    player.inventory().setItem(0, ItemStack(Items::COD, 10));
    player.inventory().setSelectedSlot(0);

    DolphinTestWorld localWorld;
    dolphin->setWorld(&localWorld);
    dolphin->setTypeId("minecraft:dolphin");
    dolphin->setChild(false);
    ASSERT_FALSE(dolphin->isChild());

    const ActionResultType result = dolphin->interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(dolphin->hasGotFish()) << "Feeding an adult dolphin should set GOT_FISH";
    EXPECT_EQ(dolphin->getGrowingAge(), AgeableEntity::MAX_AGE) << "Adult dolphin age should be unchanged";
    EXPECT_EQ(player.inventory().getItem(0).getCount(), 9) << "One fish should be consumed";
}
