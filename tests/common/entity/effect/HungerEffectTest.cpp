#include <gtest/gtest.h>

#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/food/FoodStats.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/core/Constants.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::entity::effect;

namespace {

/**
 * @brief 测试用世界存根
 */
class EffectTestWorld final : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override {
        return fluid::Fluid::getFluidState(0);
    }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() override { return false; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}
    void addParticle(client::renderer::trident::particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3&, u32) override {}

    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("EffectTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("EffectTestWorld::tickManager not implemented");
    }
    [[nodiscard]] world::border::WorldBorder& worldBorder() override {
        throw std::runtime_error("EffectTestWorld::worldBorder not implemented");
    }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override {
        throw std::runtime_error("EffectTestWorld::worldBorder not implemented");
    }
    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }
    EntityId spawnEntity(std::unique_ptr<Entity>) override { return 0; }

private:
    math::Random m_random{12345};
};

class HungerEffectTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_world = std::make_unique<EffectTestWorld>();
    }

    void TearDown() override {
        m_world.reset();
    }

    std::unique_ptr<EffectTestWorld> m_world;
};

// ============================================================================
// 饥饿效果测试
// ============================================================================

TEST_F(HungerEffectTest, HungerEffectAddExhaustionToPlayer) {
    // 创建玩家并设置世界
    Player player(EntityId(1), "TestPlayer");
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    // 获取初始饥饿值和饱和度
    FoodStats& foodStats = player.foodStats();
    f32 initialExhaustion = foodStats.exhaustionLevel();

    // 创建饥饿效果 I (amplifier = 0)
    EffectInstance hunger(EffectType::Hunger, 200, 0, false, true, true);

    // 应用效果（每 tick）- 使用 tick 而不是 applyEffect
    hunger.tick(player);

    // 验证饥饿消耗增加
    // MC 1.16.5: exhaustion += 0.005F * (amplifier + 1)
    // 饥饿 I: 0.005 * 1 = 0.005
    f32 expectedExhaustion = initialExhaustion + 0.005f;
    EXPECT_FLOAT_EQ(foodStats.exhaustionLevel(), expectedExhaustion);
}

TEST_F(HungerEffectTest, HungerEffectIIIncreasesExhaustionMore) {
    Player player(EntityId(1), "TestPlayer");
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    f32 initialExhaustion = foodStats.exhaustionLevel();

    // 创建饥饿效果 II (amplifier = 1)
    EffectInstance hunger(EffectType::Hunger, 200, 1, false, true, true);

    hunger.tick(player);

    // 饥饿 II: 0.005 * 2 = 0.01
    f32 expectedExhaustion = initialExhaustion + 0.01f;
    EXPECT_FLOAT_EQ(foodStats.exhaustionLevel(), expectedExhaustion);
}

TEST_F(HungerEffectTest, HungerEffectIIIIncreasesExhaustionMore) {
    Player player(EntityId(1), "TestPlayer");
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    f32 initialExhaustion = foodStats.exhaustionLevel();

    // 创建饥饿效果 III (amplifier = 2)
    EffectInstance hunger(EffectType::Hunger, 200, 2, false, true, true);

    hunger.tick(player);

    // 饥饿 III: 0.005 * 3 = 0.015
    f32 expectedExhaustion = initialExhaustion + 0.015f;
    EXPECT_FLOAT_EQ(foodStats.exhaustionLevel(), expectedExhaustion);
}

TEST_F(HungerEffectTest, HungerEffectDoesNotAffectNonPlayer) {
    // 饥饿效果对非玩家实体不应该做任何事情
    // 这个测试验证 tick 不会崩溃
    MobEntity mob(LegacyEntityType::Zombie, EntityId(2));
    mob.setWorld(m_world.get());
    mob.setPosition(0.0f, 64.0f, 0.0f);

    EffectInstance hunger(EffectType::Hunger, 200, 0, false, true, true);

    // 不应该抛出异常
    EXPECT_NO_THROW(hunger.tick(mob));
}

TEST_F(HungerEffectTest, HungerEffectMultipleTicks) {
    Player player(EntityId(1), "TestPlayer");
    player.setWorld(m_world.get());
    player.setPosition(0.0f, 64.0f, 0.0f);

    FoodStats& foodStats = player.foodStats();
    f32 initialExhaustion = foodStats.exhaustionLevel();

    // 创建饥饿效果 I
    EffectInstance hunger(EffectType::Hunger, 200, 0, false, true, true);

    // 模拟多个 tick
    for (int i = 0; i < 10; ++i) {
        hunger.tick(player);
    }

    // 10 tick 后: 0.005 * 10 = 0.05
    f32 expectedExhaustion = initialExhaustion + 0.05f;
    EXPECT_FLOAT_EQ(foodStats.exhaustionLevel(), expectedExhaustion);
}

} // namespace
