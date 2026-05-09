#include <gtest/gtest.h>

#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/basic/RabbitEntity.hpp"
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/core/Constants.hpp"

using namespace mc;

namespace {

class SoundCaptureWorld final : public IWorld {
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

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        if (m_supportEnabled && x == 0 && y == 0 && z == 0) {
            return &VanillaBlocks::STONE->defaultState();
        }

        return nullptr;
    }

    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override {
        return fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override {
        if (!m_supportEnabled) {
            return false;
        }

        return box.maxX > 0.0f && box.minX < 1.0f &&
               box.maxY > 0.0f && box.minY < 1.0f &&
               box.maxZ > 0.0f && box.minZ < 1.0f;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override {
        if (!hasBlockCollision(box)) {
            return {};
        }

        return {AxisAlignedBB(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f)};
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() override { return false; }

    void playSound(const ResourceLocation& soundEventId,
                   sound::SoundCategory category,
                   const Vector3& position,
                   f32 volume,
                   f32 pitch) override {
        m_lastSound = SoundRecord{soundEventId, category, position, volume, pitch};
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("SoundCaptureWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("SoundCaptureWorld::tickManager not implemented");
    }

    // Random interface (stubbed for tests)
    [[nodiscard]] math::Random& getRandom() override {
        throw std::runtime_error("SoundCaptureWorld::getRandom not implemented");
    }
    [[nodiscard]] const math::Random& getRandom() const override {
        throw std::runtime_error("SoundCaptureWorld::getRandom not implemented");
    }

    // WorldBorder interface (stubbed for tests)
    [[nodiscard]] world::border::WorldBorder& worldBorder() override {
        throw std::runtime_error("SoundCaptureWorld::worldBorder not implemented");
    }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override {
        throw std::runtime_error("SoundCaptureWorld::worldBorder not implemented");
    }

private:
    bool m_supportEnabled = true;
    std::optional<SoundRecord> m_lastSound;
};

class TestLivingEntity final : public LivingEntity {
public:
    TestLivingEntity() : LivingEntity(LegacyEntityType::Player, 1) {
        registerAttributes();
        setHealth(maxHealth());
    }
};

class TestRabbitEntity final : public RabbitEntity {
public:
    TestRabbitEntity() : RabbitEntity(LegacyEntityType::Rabbit, 2) {
        registerAttributes();
        setHealth(maxHealth());
    }
};

class TestWolfEntity final : public WolfEntity {
public:
    TestWolfEntity() : WolfEntity(LegacyEntityType::Wolf, 3) {
        registerAttributes();
        setHealth(maxHealth());
    }

    using WolfEntity::getAmbientSound;
    using WolfEntity::playShakingSound;
    using WolfEntity::playStepSound;
};

} // namespace

TEST(AnimalSoundTest, RabbitAmbientHurtDeathUseRabbitEvents) {
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

TEST(AnimalSoundTest, RabbitJumpAndAttackUseDedicatedEvents) {
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

TEST(AnimalSoundTest, RabbitNormalRabbitDoesNotPlayAttackSound) {
    SoundCaptureWorld world;
    TestRabbitEntity rabbit;
    rabbit.setWorld(&world);
    rabbit.setTypeId("minecraft:rabbit");
    rabbit.setRabbitType(RabbitEntity::RabbitType::Brown);

    TestLivingEntity target;
    rabbit.playAttackSound(target);
    EXPECT_FALSE(world.hasSoundRecord());
}

TEST(AnimalSoundTest, WolfAmbientSound_UsesVanillaVariants) {
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
        wolf.setId(static_cast<EntityId>(entityId));
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

TEST(AnimalSoundTest, WolfHurtAndDeathUseWolfEvents) {
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

TEST(AnimalSoundTest, WolfStepAndShakeUseDedicatedEvents) {
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
