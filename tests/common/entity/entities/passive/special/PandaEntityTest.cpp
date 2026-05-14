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

#include "common/core/Types.hpp"
#include "common/entity/entities/passive/special/PandaEntity.hpp"
#include "common/item/Items.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"

namespace mc {
namespace {

// ==================== PandaEntity 性格测试 ====================

TEST(PandaEntityPersonalityTest, RandomizePersonalityGeneratesValidGene)
{
    // 初始化物品系统
    Items::initialize();

    PandaEntity panda(LegacyEntityType::Panda, EntityId(1));

    // 性格应该是有效的
    auto personality = panda.getPersonality();
    EXPECT_GE(static_cast<u8>(personality), 0);
    EXPECT_LE(static_cast<u8>(personality), 7);
}

// ==================== PandaEntity 性格访问器测试 ====================

TEST(PandaEntityPersonalityAccessorsTest, SetAndGetPersonality)
{
    Items::initialize();

    PandaEntity panda(LegacyEntityType::Panda, EntityId(1));

    panda.setPersonality(PandaEntity::Personality::Lazy);
    EXPECT_EQ(panda.getPersonality(), PandaEntity::Personality::Lazy);
    EXPECT_TRUE(panda.isLazy());

    panda.setPersonality(PandaEntity::Personality::Aggressive);
    EXPECT_EQ(panda.getPersonality(), PandaEntity::Personality::Aggressive);
    EXPECT_TRUE(panda.isAggressive());

    panda.setPersonality(PandaEntity::Personality::Playful);
    EXPECT_TRUE(panda.isPlayful());

    panda.setPersonality(PandaEntity::Personality::Worried);
    EXPECT_TRUE(panda.isWorried());

    panda.setPersonality(PandaEntity::Personality::Weak);
    EXPECT_TRUE(panda.isWeak());

    panda.setPersonality(PandaEntity::Personality::Brown);
    EXPECT_TRUE(panda.isBrown());

    panda.setPersonality(PandaEntity::Personality::Normal);
    EXPECT_FALSE(panda.isLazy());
    EXPECT_FALSE(panda.isAggressive());
    EXPECT_FALSE(panda.isPlayful());
    EXPECT_FALSE(panda.isWorried());
    EXPECT_FALSE(panda.isWeak());
    EXPECT_FALSE(panda.isBrown());
}

// ==================== PandaEntity 状态测试 ====================

TEST(PandaEntityStateTest, SetAndGetSneezing)
{
    Items::initialize();

    PandaEntity panda(LegacyEntityType::Panda, EntityId(1));

    EXPECT_FALSE(panda.isSneezing());

    panda.setSneezing(true);
    EXPECT_TRUE(panda.isSneezing());

    panda.setSneezing(false);
    EXPECT_FALSE(panda.isSneezing());
}

TEST(PandaEntityStateTest, SetAndGetSneezeTimer)
{
    Items::initialize();

    PandaEntity panda(LegacyEntityType::Panda, EntityId(1));

    EXPECT_EQ(panda.getSneezeTimer(), 0);

    panda.setSneezeTimer(20);
    EXPECT_EQ(panda.getSneezeTimer(), 20);

    panda.setSneezeTimer(0);
    EXPECT_EQ(panda.getSneezeTimer(), 0);
}

TEST(PandaEntityStateTest, SetAndGetRolling)
{
    Items::initialize();

    PandaEntity panda(LegacyEntityType::Panda, EntityId(1));

    EXPECT_FALSE(panda.isRolling());

    panda.setRolling(true);
    EXPECT_TRUE(panda.isRolling());

    panda.setRolling(false);
    EXPECT_FALSE(panda.isRolling());
}

TEST(PandaEntityStateTest, SetAndGetEating)
{
    Items::initialize();

    PandaEntity panda(LegacyEntityType::Panda, EntityId(1));

    EXPECT_FALSE(panda.isEating());

    panda.setEating(true);
    EXPECT_TRUE(panda.isEating());

    panda.setEating(false);
    EXPECT_FALSE(panda.isEating());
}

TEST(PandaEntityStateTest, SetAndGetLying)
{
    Items::initialize();

    PandaEntity panda(LegacyEntityType::Panda, EntityId(1));

    EXPECT_FALSE(panda.isLying());

    panda.setLying(true);
    EXPECT_TRUE(panda.isLying());

    panda.setLying(false);
    EXPECT_FALSE(panda.isLying());
}

// ==================== PandaEntity 眼睛高度测试 ====================

TEST(PandaEntityEyeHeightTest, AdultHasCorrectEyeHeight)
{
    Items::initialize();

    PandaEntity panda(LegacyEntityType::Panda, EntityId(1));
    panda.setChild(false);

    EXPECT_FLOAT_EQ(panda.eyeHeight(), 1.2f);
}

TEST(PandaEntityEyeHeightTest, ChildHasCorrectEyeHeight)
{
    Items::initialize();

    PandaEntity panda(LegacyEntityType::Panda, EntityId(1));
    panda.setChild(true);

    EXPECT_FLOAT_EQ(panda.eyeHeight(), 0.6f);
}

// ==================== PandaEntity 音效常量测试 ====================

TEST(PandaEntitySoundTest, SoundEventsAreDefined)
{
    // 验证熊猫相关音效事件已定义（检查 ResourceLocation 的字符串表示）
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_AMBIENT.toString(), "minecraft:entity.panda.ambient");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_HURT.toString(), "minecraft:entity.panda.hurt");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_DEATH.toString(), "minecraft:entity.panda.death");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_EAT.toString(), "minecraft:entity.panda.eat");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_SNEEZE.toString(), "minecraft:entity.panda.sneeze");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_PRE_SNEEZE.toString(), "minecraft:entity.panda.pre_sneeze");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_BITE.toString(), "minecraft:entity.panda.bite");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_AGGRESSIVE_AMBIENT.toString(), "minecraft:entity.panda.aggressive_ambient");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_WORRIED_AMBIENT.toString(), "minecraft:entity.panda.worried_ambient");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_CANT_BREED.toString(), "minecraft:entity.panda.cant_breed");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_STEP.toString(), "minecraft:entity.panda.step");
}

// ==================== PandaEntity 性格概率分布测试 ====================

TEST(PandaEntityPersonalityTest, PersonalityDistributionIsValid)
{
    Items::initialize();

    // 测试性格枚举值是否正确
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::Normal), 0);
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::Lazy), 1);
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::Worried), 2);
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::Playful), 3);
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::Aggressive), 4);
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::Weak), 5);
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::Brown), 6);
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::AggressiveLazy), 7);
}

} // anonymous namespace
} // namespace mc
