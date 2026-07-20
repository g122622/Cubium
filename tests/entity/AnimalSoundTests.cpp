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
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/basic/RabbitEntity.hpp"
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

using namespace mc;

namespace {

class SoundCaptureWorld final : public test::BaseTestWorld {
public:
    struct SoundRecord {
        ResourceLocation soundEventId;
        sound::SoundCategory category;
        Vector3 position;
        f32 volume;
        f32 pitch;
    };

    void clearSound() { m_lastSound.reset(); }

    [[nodiscard]] bool hasSoundRecord() const { return m_lastSound.has_value(); }
    [[nodiscard]] const SoundRecord& lastSound() const { return *m_lastSound; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        if (m_supportEnabled && x == 0 && y == 0 && z == 0) {
            return &VanillaBlocks::STONE->defaultState();
        }

        return nullptr;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override
    {
        if (!m_supportEnabled) {
            return false;
        }

        return box.maxX > 0.0f && box.minX < 1.0f && box.maxY > 0.0f && box.minY < 1.0f && box.maxZ > 0.0f &&
            box.minZ < 1.0f;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override
    {
        if (!hasBlockCollision(box)) {
            return {};
        }

        return {AxisAlignedBB(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f)};
    }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_lastSound = SoundRecord{soundEventId, category, position, volume, pitch};
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SoundCaptureWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SoundCaptureWorld::tickManager not implemented");
    }

private:
    bool m_supportEnabled = true;
    std::optional<SoundRecord> m_lastSound;
};

class TestLivingEntity final : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1))
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

class TestRabbitEntity final : public RabbitEntity {
public:
    TestRabbitEntity()
        : RabbitEntity(EntityInstanceId(2))
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

class TestWolfEntity final : public WolfEntity {
public:
    TestWolfEntity()
        : WolfEntity(EntityInstanceId(3))
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    using WolfEntity::getAmbientSound;
    using WolfEntity::playShakingSound;
    using WolfEntity::playStepSound;
};

} // namespace

TEST(AnimalSoundTest, RabbitAmbientHurtDeathUseRabbitEvents)
{
    SoundCaptureWorld world;
    TestRabbitEntity rabbit;
    rabbit.setWorld(&world);
    rabbit.setTypeId("minecraft:rabbit");
    rabbit.setRabbitType(RabbitEntity::RabbitType::Brown);

    rabbit.playAmbientSound();
    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.rabbit.ambient");
    EXPECT_EQ(world.lastSound().category, sound::SoundCategory::Neutral);

    world.clearSound();
    EnvironmentalDamage hurtDamage(DamageType::Generic);
    EXPECT_TRUE(rabbit.hurt(hurtDamage, 1.0f));
    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.rabbit.hurt");
    EXPECT_EQ(world.lastSound().category, sound::SoundCategory::Neutral);

    world.clearSound();
    TestRabbitEntity deathRabbit;
    deathRabbit.setWorld(&world);
    deathRabbit.setTypeId("minecraft:rabbit");
    deathRabbit.setRabbitType(RabbitEntity::RabbitType::Brown);
    deathRabbit.setHealth(5.0f);
    EXPECT_TRUE(deathRabbit.hurt(hurtDamage, 10.0f));
    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.rabbit.death");
    EXPECT_EQ(world.lastSound().category, sound::SoundCategory::Neutral);
}

TEST(AnimalSoundTest, RabbitJumpAndAttackUseDedicatedEvents)
{
    SoundCaptureWorld world;
    TestRabbitEntity rabbit;
    rabbit.setWorld(&world);
    rabbit.setTypeId("minecraft:rabbit");
    rabbit.setRabbitType(RabbitEntity::RabbitType::Killer);

    rabbit.setJumping(true);
    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.rabbit.jump");
    EXPECT_EQ(world.lastSound().category, sound::SoundCategory::Hostile);

    world.clearSound();
    TestLivingEntity target;
    rabbit.playAttackSound(target);
    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.rabbit.attack");
    EXPECT_EQ(world.lastSound().category, sound::SoundCategory::Hostile);
}

TEST(AnimalSoundTest, RabbitNormalRabbitDoesNotPlayAttackSound)
{
    SoundCaptureWorld world;
    TestRabbitEntity rabbit;
    rabbit.setWorld(&world);
    rabbit.setTypeId("minecraft:rabbit");
    rabbit.setRabbitType(RabbitEntity::RabbitType::Brown);

    TestLivingEntity target;
    rabbit.playAttackSound(target);
    EXPECT_FALSE(world.hasSoundRecord());
}

TEST(AnimalSoundTest, WolfAmbientSound_UsesVanillaVariants)
{
    TestWolfEntity wolf;
    wolf.setTypeId("minecraft:wolf");

    wolf.setAngry(true);
    auto angrySound = wolf.getAmbientSound();
    ASSERT_TRUE(angrySound.has_value());
    EXPECT_EQ(angrySound->toString(), "minecraft:entity.wolf.growl");

    wolf.setAngry(false);
    wolf.setTamed(true);
    wolf.setHealth(5.0f);

    bool foundLowHealthAmbient = false;
    for (u64 entityId = 1; entityId <= 512; ++entityId) {
        wolf.setId(static_cast<EntityInstanceId>(entityId));
        auto sound = wolf.getAmbientSound();
        if (!sound.has_value()) {
            continue;
        }

        const std::string soundId = sound->toString();
        if (soundId == "minecraft:entity.wolf.whine" || soundId == "minecraft:entity.wolf.pant") {
            foundLowHealthAmbient = true;
            break;
        }
    }

    EXPECT_TRUE(foundLowHealthAmbient);
}

TEST(AnimalSoundTest, WolfHurtAndDeathUseWolfEvents)
{
    SoundCaptureWorld world;
    EnvironmentalDamage damage(DamageType::Generic);

    TestWolfEntity hurtWolf;
    hurtWolf.setWorld(&world);
    hurtWolf.setTypeId("minecraft:wolf");
    hurtWolf.setHealth(8.0f);
    EXPECT_TRUE(hurtWolf.hurt(damage, 1.0f));
    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.wolf.hurt");

    world.clearSound();
    TestWolfEntity deathWolf;
    deathWolf.setWorld(&world);
    deathWolf.setTypeId("minecraft:wolf");
    deathWolf.setHealth(5.0f);
    EXPECT_TRUE(deathWolf.hurt(damage, 10.0f));
    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.wolf.death");
}

TEST(AnimalSoundTest, WolfStepAndShakeUseDedicatedEvents)
{
    SoundCaptureWorld world;
    TestWolfEntity wolf;
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");

    wolf.playStepSound();
    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.wolf.step");
    EXPECT_EQ(world.lastSound().category, sound::SoundCategory::Neutral);
    EXPECT_FLOAT_EQ(world.lastSound().volume, 0.15f);
    EXPECT_FLOAT_EQ(world.lastSound().pitch, 1.0f);

    world.clearSound();
    wolf.playShakingSound();
    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.wolf.shake");
    EXPECT_EQ(world.lastSound().category, sound::SoundCategory::Neutral);
    EXPECT_FLOAT_EQ(world.lastSound().volume, 0.4f);
}
