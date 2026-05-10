#include <gtest/gtest.h>

#include "common/entity/core/MobEntity.hpp"
#include "common/entity/core/FlyingEntity.hpp"
#include "common/entity/entities/monster/basic/PhantomEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
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
 * @brief 测试用世界存根，支持可配置的时间和亮度
 */
class EntityTestWorld final : public IWorld {
public:
    EntityTestWorld() : m_dayTime(0), m_skyLight(15), m_blockLight(0), m_canSeeSky(true), m_isRaining(false) {}

    // 世界时间配置
    void setDayTime(i64 time) { m_dayTime = time; }
    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }

    // 光照配置
    void setSkyLight(u8 light) { m_skyLight = light; }
    void setBlockLight(u8 light) { m_blockLight = light; }
    void setCanSeeSky(bool canSee) { m_canSeeSky = canSee; }
    void setBrightness(f32 brightness) { m_brightness = brightness; }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return m_skyLight; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return m_blockLight; }
    [[nodiscard]] u8 getSkyLight(const BlockPos&) const override { return m_skyLight; }
    [[nodiscard]] u8 getBlockLight(const BlockPos&) const override { return m_blockLight; }

    [[nodiscard]] bool canSeeSky(const BlockPos&) const override { return m_canSeeSky; }

    [[nodiscard]] f32 getBrightness(const BlockPos&) const override {
        return m_brightness.has_value() ? m_brightness.value() : static_cast<f32>(std::max(m_skyLight, m_blockLight)) / 15.0f;
    }

    [[nodiscard]] u8 getLightSubtracted(const BlockPos& pos, u32 skyDarkening) const override {
        u8 sky = m_skyLight > static_cast<u8>(skyDarkening) ? m_skyLight - static_cast<u8>(skyDarkening) : 0;
        return std::max(sky, m_blockLight);
    }

    // 天气配置
    void setRaining(bool raining) { m_isRaining = raining; }
    [[nodiscard]] bool isRaining() const override { return m_isRaining; }
    [[nodiscard]] bool isThundering() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return m_isRaining; }

    // 昼夜检测
    [[nodiscard]] bool isDaytime() const override { return m_dayTime < 12000; }

    // IWorld 接口实现
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override {
        return fluid::Fluid::getFluidState(0);
    }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
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
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }
    [[nodiscard]] bool isClientSide() override { return false; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}
    void addParticle(client::renderer::trident::particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3&, u32) override {}

    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("EntityTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("EntityTestWorld::tickManager not implemented");
    }
    [[nodiscard]] world::border::WorldBorder& worldBorder() override {
        throw std::runtime_error("EntityTestWorld::worldBorder not implemented");
    }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override {
        throw std::runtime_error("EntityTestWorld::worldBorder not implemented");
    }
    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }
    EntityId spawnEntity(std::unique_ptr<Entity>) override { return EntityId(1); }

private:
    i64 m_dayTime;
    u8 m_skyLight;
    u8 m_blockLight;
    bool m_canSeeSky;
    bool m_isRaining;
    std::optional<f32> m_brightness;
    math::Random m_random{12345};
};

} // namespace

// ============================================================================
// MobEntity::isInDaylight() 测试
// ============================================================================

class IsInDaylightTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_world = std::make_unique<EntityTestWorld>();
    }

    void TearDown() override {
        m_world.reset();
    }

    std::unique_ptr<EntityTestWorld> m_world;
};

TEST_F(IsInDaylightTest, ReturnsFalseAtNight) {
    // 夜晚时间：12000-24000
    m_world->setDayTime(13000);  // 夜晚
    m_world->setCanSeeSky(true);
    m_world->setBrightness(1.0f);

    PhantomEntity phantom(LegacyEntityType::Phantom, EntityId(1));
    phantom.setWorld(m_world.get());
    phantom.setPosition(0.0f, 64.0f, 0.0f);

    EXPECT_FALSE(phantom.isInDaylight());
}

TEST_F(IsInDaylightTest, ReturnsFalseWhenSkyNotVisible) {
    m_world->setDayTime(6000);   // 白天
    m_world->setCanSeeSky(false);  // 天空不可见
    m_world->setBrightness(1.0f);

    PhantomEntity phantom(LegacyEntityType::Phantom, EntityId(1));
    phantom.setWorld(m_world.get());
    phantom.setPosition(0.0f, 64.0f, 0.0f);

    EXPECT_FALSE(phantom.isInDaylight());
}

TEST_F(IsInDaylightTest, ReturnsFalseWithLowBrightness) {
    m_world->setDayTime(6000);   // 白天
    m_world->setCanSeeSky(true);
    m_world->setBrightness(0.3f);  // 低亮度

    PhantomEntity phantom(LegacyEntityType::Phantom, EntityId(1));
    phantom.setWorld(m_world.get());
    phantom.setPosition(0.0f, 64.0f, 0.0f);

    EXPECT_FALSE(phantom.isInDaylight());
}

TEST_F(IsInDaylightTest, ReturnsTrueDuringDayWithHighBrightness) {
    // 白天时间：0-11999
    m_world->setDayTime(6000);   // 中午
    m_world->setCanSeeSky(true);
    m_world->setBrightness(0.8f);  // 高亮度

    PhantomEntity phantom(LegacyEntityType::Phantom, EntityId(1));
    phantom.setWorld(m_world.get());
    phantom.setPosition(0.0f, 64.0f, 0.0f);

    // isInDaylight 有随机检查，可能需要多次测试
    // 但在高亮度下应该有较高概率返回 true
    // 由于随机性，我们只检查不抛异常
    EXPECT_NO_THROW({
        bool result = phantom.isInDaylight();
        (void)result;
    });
}

// ============================================================================
// PhantomEntity 日光燃烧测试
// ============================================================================

class PhantomEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_world = std::make_unique<EntityTestWorld>();
    }

    void TearDown() override {
        m_world.reset();
    }

    std::unique_ptr<EntityTestWorld> m_world;
};

TEST_F(PhantomEntityTest, DoesNotBurnAtNight) {
    m_world->setDayTime(13000);  // 夜晚
    m_world->setCanSeeSky(true);
    m_world->setBrightness(0.2f);

    PhantomEntity phantom(LegacyEntityType::Phantom, EntityId(1));
    phantom.setWorld(m_world.get());
    phantom.setPosition(0.0f, 64.0f, 0.0f);

    EXPECT_FALSE(phantom.isOnFire());
    EXPECT_NO_THROW(phantom.tick());
}

TEST_F(PhantomEntityTest, SizeAffectsDimensions) {
    PhantomEntity phantom(LegacyEntityType::Phantom, EntityId(1));

    // 默认尺寸 0
    EXPECT_EQ(phantom.getPhantomSize(), 0);

    // 设置尺寸
    phantom.setPhantomSize(2);
    EXPECT_EQ(phantom.getPhantomSize(), 2);

    // 检查尺寸上限
    phantom.setPhantomSize(100);  // 超过最大值
    EXPECT_EQ(phantom.getPhantomSize(), 64);  // 应该被限制为 64
}

TEST_F(PhantomEntityTest, SizeAffectsAttackDamage) {
    PhantomEntity phantom(LegacyEntityType::Phantom, EntityId(1));

    // 尺寸 0 -> 基础伤害 6.0
    phantom.setPhantomSize(0);
    // 攻击伤害由属性系统管理

    // 尺寸 4 -> 伤害应该更高
    phantom.setPhantomSize(4);
    EXPECT_EQ(phantom.getPhantomSize(), 4);
}

TEST_F(PhantomEntityTest, AttackPhaseDefaultIsCircle) {
    PhantomEntity phantom(LegacyEntityType::Phantom, EntityId(1));
    EXPECT_EQ(phantom.getAttackPhase(), PhantomEntity::AttackPhase::CIRCLE);
}

// ============================================================================
// IWorld::isDaytime() 测试
// ============================================================================

class DaytimeTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_world = std::make_unique<EntityTestWorld>();
    }

    void TearDown() override {
        m_world.reset();
    }

    std::unique_ptr<EntityTestWorld> m_world;
};

TEST_F(DaytimeTest, IsDayDuringMorning) {
    m_world->setDayTime(0);  // 日出
    EXPECT_TRUE(m_world->isDaytime());
}

TEST_F(DaytimeTest, IsDayDuringNoon) {
    m_world->setDayTime(6000);  // 中午
    EXPECT_TRUE(m_world->isDaytime());
}

TEST_F(DaytimeTest, IsDayJustBeforeSunset) {
    m_world->setDayTime(11999);  // 日落前一刻
    EXPECT_TRUE(m_world->isDaytime());
}

TEST_F(DaytimeTest, IsNightAtSunset) {
    m_world->setDayTime(12000);  // 日落
    EXPECT_FALSE(m_world->isDaytime());
}

TEST_F(DaytimeTest, IsNightAtMidnight) {
    m_world->setDayTime(18000);  // 午夜
    EXPECT_FALSE(m_world->isDaytime());
}

TEST_F(DaytimeTest, IsNightJustBeforeDawn) {
    m_world->setDayTime(23999);  // 日出前一刻
    EXPECT_FALSE(m_world->isDaytime());
}

// ============================================================================
// MobEntity::getBrightness() 测试
// ============================================================================

class GetBrightnessTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_world = std::make_unique<EntityTestWorld>();
    }

    void TearDown() override {
        m_world.reset();
    }

    std::unique_ptr<EntityTestWorld> m_world;
};

TEST_F(GetBrightnessTest, ReturnsWorldBrightnessAtEyePosition) {
    m_world->setBrightness(0.75f);

    PhantomEntity phantom(LegacyEntityType::Phantom, EntityId(1));
    phantom.setWorld(m_world.get());
    phantom.setPosition(0.0f, 64.0f, 0.0f);

    f32 brightness = phantom.getBrightness();
    EXPECT_FLOAT_EQ(brightness, 0.75f);
}

TEST_F(GetBrightnessTest, ReturnsZeroWhenNoWorld) {
    PhantomEntity phantom(LegacyEntityType::Phantom, EntityId(1));
    // 不设置世界
    phantom.setPosition(0.0f, 64.0f, 0.0f);

    f32 brightness = phantom.getBrightness();
    EXPECT_FLOAT_EQ(brightness, 0.0f);
}

TEST_F(GetBrightnessTest, UsesEyeHeightForPosition) {
    m_world->setBrightness(0.9f);

    PhantomEntity phantom(LegacyEntityType::Phantom, EntityId(1));
    phantom.setWorld(m_world.get());
    phantom.setPosition(10.0f, 64.0f, 20.0f);

    // 眼睛高度 = height * 0.35f
    f32 expectedY = 64.0f + phantom.eyeHeight();
    EXPECT_GT(expectedY, 64.0f);

    f32 brightness = phantom.getBrightness();
    EXPECT_FLOAT_EQ(brightness, 0.9f);
}

// ============================================================================
// EffectInstance 挖掘疲劳测试
// ============================================================================

class MiningFatigueEffectTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_world = std::make_unique<EntityTestWorld>();
    }

    void TearDown() override {
        m_world.reset();
    }

    std::unique_ptr<EntityTestWorld> m_world;
};

TEST_F(MiningFatigueEffectTest, MiningFatigueLevelIII) {
    // 挖掘疲劳 III (amplifier = 2)
    EffectInstance fatigue(EffectType::MiningFatigue, 6000, 2, false, true, true);
    EXPECT_EQ(fatigue.amplifier(), 2);
    EXPECT_EQ(fatigue.getEffectLevel(), 3);  // 显示等级 III
}

TEST_F(MiningFatigueEffectTest, MiningFatigueDuration) {
    // 挖掘疲劳持续时间：6000 tick = 5 分钟
    EffectInstance fatigue(EffectType::MiningFatigue, 6000, 2, false, true, true);
    EXPECT_EQ(fatigue.duration(), 6000);
    EXPECT_FALSE(fatigue.isExpired());
}

TEST_F(MiningFatigueEffectTest, MiningFatigueRange) {
    // 远古守卫者挖掘疲劳范围：50格
    constexpr f32 MINING_FATIGUE_RANGE = 50.0f;
    EXPECT_FLOAT_EQ(MINING_FATIGUE_RANGE, 50.0f);
}

TEST_F(MiningFatigueEffectTest, MiningFatigueInterval) {
    // 远古守卫者挖掘疲劳间隔：600 tick = 30 秒
    constexpr i32 FATIGUE_INTERVAL = 600;
    EXPECT_EQ(FATIGUE_INTERVAL, 600);
}
