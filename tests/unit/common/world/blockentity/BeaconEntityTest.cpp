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

#include "world/blockentity/processing/BeaconEntity.hpp"
#include "entity/effect/EffectType.hpp"
#include "world/block/BlockPos.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;
using namespace mc::blockentity;

// ========== BeaconEntity 测试 ==========

class BeaconEntityTest : public ::testing::Test {
protected:
    void SetUp() override { beacon_ = std::make_unique<BeaconEntity>(BlockPos(10, 64, 20)); }

    std::unique_ptr<BeaconEntity> beacon_;
};

TEST_F(BeaconEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(beacon_->getType(), BlockEntityType::Beacon);
}

TEST_F(BeaconEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(beacon_->getPos(), BlockPos(10, 64, 20));
}

TEST_F(BeaconEntityTest, Create_LevelIsZero)
{
    EXPECT_EQ(beacon_->getLevel(), 0);
}

TEST_F(BeaconEntityTest, Create_IsNotActive)
{
    EXPECT_FALSE(beacon_->isActive());
}

TEST_F(BeaconEntityTest, Create_NoPrimaryEffect)
{
    EXPECT_EQ(beacon_->getPrimaryEffect(), nullptr);
}

TEST_F(BeaconEntityTest, Create_NoSecondaryEffect)
{
    EXPECT_EQ(beacon_->getSecondaryEffect(), nullptr);
}

TEST_F(BeaconEntityTest, Create_PaymentItemIsEmpty)
{
    EXPECT_TRUE(beacon_->getPaymentItem().isEmpty());
}

TEST_F(BeaconEntityTest, Create_NoBeam)
{
    EXPECT_FALSE(beacon_->hasBeam());
}

TEST_F(BeaconEntityTest, Create_BeamSegmentsEmpty)
{
    EXPECT_TRUE(beacon_->getBeamSegments().empty());
}

TEST_F(BeaconEntityTest, NeedsTick_ReturnsTrue)
{
    EXPECT_TRUE(beacon_->needsTick());
}

TEST_F(BeaconEntityTest, SetLevel_UpdatesLevel)
{
    beacon_->setLevel(2);
    EXPECT_EQ(beacon_->getLevel(), 2);
}

TEST_F(BeaconEntityTest, SetLevel_ClampsToValidRange)
{
    beacon_->setLevel(-5);
    EXPECT_EQ(beacon_->getLevel(), 0);

    beacon_->setLevel(10);
    EXPECT_EQ(beacon_->getLevel(), 4); // MAX_LEVELS = 4
}

TEST_F(BeaconEntityTest, SetLevel_MarksChanged)
{
    EXPECT_FALSE(beacon_->isChanged());
    beacon_->setLevel(1);
    EXPECT_TRUE(beacon_->isChanged());
}

TEST_F(BeaconEntityTest, SetPrimaryEffect_UpdatesEffect)
{
    using EffectType = BeaconEntity::EffectType;
    const EffectType speed = EffectType::Speed;
    beacon_->setPrimaryEffect(&speed);
    EXPECT_NE(beacon_->getPrimaryEffect(), nullptr);
    EXPECT_EQ(*beacon_->getPrimaryEffect(), EffectType::Speed);
}

TEST_F(BeaconEntityTest, SetPrimaryEffect_NullClearsEffect)
{
    using EffectType = BeaconEntity::EffectType;
    const EffectType speed = EffectType::Speed;
    beacon_->setPrimaryEffect(&speed);
    beacon_->setPrimaryEffect(nullptr);
    EXPECT_EQ(beacon_->getPrimaryEffect(), nullptr);
}

TEST_F(BeaconEntityTest, SetSecondaryEffect_UpdatesEffect)
{
    using EffectType = BeaconEntity::EffectType;
    const EffectType regeneration = EffectType::Regeneration;
    beacon_->setSecondaryEffect(&regeneration);
    EXPECT_NE(beacon_->getSecondaryEffect(), nullptr);
    EXPECT_EQ(*beacon_->getSecondaryEffect(), EffectType::Regeneration);
}

TEST_F(BeaconEntityTest, IsActive_RequiresLevelAndEffect)
{
    using EffectType = BeaconEntity::EffectType;
    const EffectType speed = EffectType::Speed;

    // 无等级、无效果 -> 不激活
    EXPECT_FALSE(beacon_->isActive());

    // 有等级、无效果 -> 不激活
    beacon_->setLevel(1);
    EXPECT_FALSE(beacon_->isActive());

    // 有等级、有效果 -> 激活
    beacon_->setPrimaryEffect(&speed);
    EXPECT_TRUE(beacon_->isActive());

    // 无等级、有效果 -> 不激活
    beacon_->setLevel(0);
    EXPECT_FALSE(beacon_->isActive());
}

TEST_F(BeaconEntityTest, GetEffectRange_ReturnsCorrectValue)
{
    // 公式: level * 10 + 10
    beacon_->setLevel(1);
    EXPECT_EQ(beacon_->getEffectRange(), 20);

    beacon_->setLevel(2);
    EXPECT_EQ(beacon_->getEffectRange(), 30);

    beacon_->setLevel(3);
    EXPECT_EQ(beacon_->getEffectRange(), 40);

    beacon_->setLevel(4);
    EXPECT_EQ(beacon_->getEffectRange(), 50);

    beacon_->setLevel(0);
    EXPECT_EQ(beacon_->getEffectRange(), 10);
}

TEST_F(BeaconEntityTest, Clone_CreatesCopy)
{
    using EffectType = BeaconEntity::EffectType;
    const EffectType speed = EffectType::Speed;
    const EffectType regeneration = EffectType::Regeneration;

    beacon_->setLevel(3);
    beacon_->setPrimaryEffect(&speed);
    beacon_->setSecondaryEffect(&regeneration);

    std::unique_ptr<BlockEntity> copy = beacon_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Beacon);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 64, 20));

    auto* beaconCopy = static_cast<BeaconEntity*>(copy.get());
    EXPECT_EQ(beaconCopy->getLevel(), 3);
    EXPECT_NE(beaconCopy->getPrimaryEffect(), nullptr);
    EXPECT_EQ(*beaconCopy->getPrimaryEffect(), EffectType::Speed);
    EXPECT_NE(beaconCopy->getSecondaryEffect(), nullptr);
    EXPECT_EQ(*beaconCopy->getSecondaryEffect(), EffectType::Regeneration);
}

TEST_F(BeaconEntityTest, CanUseEffect_ReturnsTrueForValidEffects)
{
    using EffectType = BeaconEntity::EffectType;

    const EffectType speed = EffectType::Speed;
    const EffectType haste = EffectType::Haste;
    const EffectType resistance = EffectType::Resistance;
    const EffectType jumpBoost = EffectType::JumpBoost;
    const EffectType strength = EffectType::Strength;
    const EffectType regeneration = EffectType::Regeneration;

    EXPECT_TRUE(beacon_->canUseEffect(&speed));
    EXPECT_TRUE(beacon_->canUseEffect(&haste));
    EXPECT_TRUE(beacon_->canUseEffect(&resistance));
    EXPECT_TRUE(beacon_->canUseEffect(&jumpBoost));
    EXPECT_TRUE(beacon_->canUseEffect(&strength));
    EXPECT_TRUE(beacon_->canUseEffect(&regeneration));
}

TEST_F(BeaconEntityTest, CanUseEffect_ReturnsTrueForNull)
{
    // null 表示无效果，是有效的
    EXPECT_TRUE(beacon_->canUseEffect(nullptr));
}

TEST_F(BeaconEntityTest, SaveAndLoad_PreservesData)
{
    using EffectType = BeaconEntity::EffectType;
    const EffectType haste = EffectType::Haste;

    beacon_->setLevel(2);
    beacon_->setPrimaryEffect(&haste);

    nlohmann::json data;
    beacon_->save(data);

    auto loaded = std::make_unique<BeaconEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    EXPECT_EQ(loaded->getLevel(), 2);
    EXPECT_NE(loaded->getPrimaryEffect(), nullptr);
    EXPECT_EQ(*loaded->getPrimaryEffect(), EffectType::Haste);
}

TEST_F(BeaconEntityTest, SaveAndLoad_HandlesNoEffect)
{
    beacon_->setLevel(1);

    nlohmann::json data;
    beacon_->save(data);

    auto loaded = std::make_unique<BeaconEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    EXPECT_EQ(loaded->getLevel(), 1);
    EXPECT_EQ(loaded->getPrimaryEffect(), nullptr);
    EXPECT_EQ(loaded->getSecondaryEffect(), nullptr);
}

// ========== 红石比较器信号测试 ==========
// 信标的比较器信号等于金字塔等级 (0-4)
// 参考 MC 1.16.5: BeaconBlock.getComparatorInputOverride() 返回 levels

TEST_F(BeaconEntityTest, GetLevel_ReturnsZeroWhenInactive)
{
    // 信标未激活时，等级为 0
    EXPECT_EQ(beacon_->getLevel(), 0);
}

TEST_F(BeaconEntityTest, GetLevel_ReturnsOneForLevelOne)
{
    beacon_->setLevel(1);
    EXPECT_EQ(beacon_->getLevel(), 1);
}

TEST_F(BeaconEntityTest, GetLevel_ReturnsTwoForLevelTwo)
{
    beacon_->setLevel(2);
    EXPECT_EQ(beacon_->getLevel(), 2);
}

TEST_F(BeaconEntityTest, GetLevel_ReturnsThreeForLevelThree)
{
    beacon_->setLevel(3);
    EXPECT_EQ(beacon_->getLevel(), 3);
}

TEST_F(BeaconEntityTest, GetLevel_ReturnsFourForLevelFour)
{
    beacon_->setLevel(4);
    EXPECT_EQ(beacon_->getLevel(), 4);
}

TEST_F(BeaconEntityTest, GetLevel_RespectsMaxLevel)
{
    // 等级上限为 4
    beacon_->setLevel(100);
    EXPECT_EQ(beacon_->getLevel(), 4);
}

TEST_F(BeaconEntityTest, GetLevel_RespectsMinLevel)
{
    // 等级下限为 0
    beacon_->setLevel(-10);
    EXPECT_EQ(beacon_->getLevel(), 0);
}

TEST_F(BeaconEntityTest, GetLevel_CanTransitionBetweenLevels)
{
    // 测试等级转换
    beacon_->setLevel(2);
    EXPECT_EQ(beacon_->getLevel(), 2);

    beacon_->setLevel(4);
    EXPECT_EQ(beacon_->getLevel(), 4);

    beacon_->setLevel(0);
    EXPECT_EQ(beacon_->getLevel(), 0);

    beacon_->setLevel(1);
    EXPECT_EQ(beacon_->getLevel(), 1);
}

TEST_F(BeaconEntityTest, MaxLevels_ConstantIsFour)
{
    EXPECT_EQ(BeaconEntity::MAX_LEVELS, 4);
}
