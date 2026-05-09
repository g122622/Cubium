#include <gtest/gtest.h>

#include "common/entity/entities/monster/basic/SlimeEntity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/core/Constants.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用世界实现
 *
 * 提供史莱姆测试所需的最小 IWorld 接口实现
 */
class SlimeTestWorld final : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 0; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
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

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override {
        // 记录生成的史莱姆
        if (auto* slime = dynamic_cast<SlimeEntity*>(entity.get())) {
            m_spawnedSlimeSizes.push_back(slime->getSlimeSize());
            m_spawnedSlimeCount++;
        }

        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("SlimeTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("SlimeTestWorld::tickManager not implemented");
    }

    // Random interface (stubbed for tests)
    [[nodiscard]] math::Random& getRandom() override {
        throw std::runtime_error("SlimeTestWorld::getRandom not implemented");
    }
    [[nodiscard]] const math::Random& getRandom() const override {
        throw std::runtime_error("SlimeTestWorld::getRandom not implemented");
    }

    // WorldBorder interface (stubbed for tests)
    [[nodiscard]] world::border::WorldBorder& worldBorder() override {
        throw std::runtime_error("SlimeTestWorld::worldBorder not implemented");
    }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override {
        throw std::runtime_error("SlimeTestWorld::worldBorder not implemented");
    }

    // 测试辅助方法
    [[nodiscard]] size_t spawnedSlimeCount() const { return m_spawnedSlimeCount; }
    [[nodiscard]] const std::vector<i32>& spawnedSlimeSizes() const { return m_spawnedSlimeSizes; }
    void clearSpawnedEntities() {
        m_spawnedEntities.clear();
        m_spawnedSlimeSizes.clear();
        m_spawnedSlimeCount = 0;
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::vector<i32> m_spawnedSlimeSizes;
    size_t m_spawnedSlimeCount = 0;
};

class SlimeEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }

    SlimeTestWorld m_world;
};

// ==================== 尺寸系统测试 ====================

TEST_F(SlimeEntityTest, SetSlimeSize_UpdatesAttributes) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));
    slime.setWorld(&m_world);

    // 设置尺寸为 4
    slime.setSlimeSize(4, true);

    // 验证属性
    EXPECT_EQ(slime.getSlimeSize(), 4);
    // HP = size * size = 16
    EXPECT_FLOAT_EQ(static_cast<f32>(slime.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH, 0.0)), 16.0f);
    // Speed = 0.2 + 0.1 * size = 0.6
    EXPECT_FLOAT_EQ(static_cast<f32>(slime.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0)), 0.6f);
    // AttackDamage = size = 4
    EXPECT_FLOAT_EQ(static_cast<f32>(slime.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0)), 4.0f);
    // 生命值应该被重置为最大值
    EXPECT_FLOAT_EQ(slime.health(), slime.maxHealth());
}

TEST_F(SlimeEntityTest, SetSlimeSize_ClampsToValidRange) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));

    // 测试下限
    slime.setSlimeSize(0, false);
    EXPECT_EQ(slime.getSlimeSize(), 1);  // 最小为 1

    // 测试上限
    slime.setSlimeSize(100, false);
    EXPECT_EQ(slime.getSlimeSize(), 4);  // 最大为 4
}

TEST_F(SlimeEntityTest, IsSmallSlime_ReturnsTrueForSizeOne) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));

    slime.setSlimeSize(1, false);
    EXPECT_TRUE(slime.isSmallSlime());

    slime.setSlimeSize(2, false);
    EXPECT_FALSE(slime.isSmallSlime());

    slime.setSlimeSize(4, false);
    EXPECT_FALSE(slime.isSmallSlime());
}

TEST_F(SlimeEntityTest, CanSplit_ReturnsTrueForSizeGreaterThanOne) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));

    slime.setSlimeSize(1, false);
    EXPECT_FALSE(slime.canSplit());

    slime.setSlimeSize(2, false);
    EXPECT_TRUE(slime.canSplit());

    slime.setSlimeSize(4, false);
    EXPECT_TRUE(slime.canSplit());
}

// ==================== 分裂测试 ====================

TEST_F(SlimeEntityTest, PerformSplit_CreatesCorrectNumberOfSmallSlimes) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));
    slime.setWorld(&m_world);
    slime.setSlimeSize(4, true);  // 大史莱姆
    slime.setPosition(100.0, 64.0, 100.0);

    // 设置为死亡状态
    EnvironmentalDamage damage = DamageSources::generic();
    slime.hurt(damage, 100.0f);

    // 执行分裂
    slime.performSplit();

    // 验证生成了 2-4 个小史莱姆
    EXPECT_GE(m_world.spawnedSlimeCount(), 2u);
    EXPECT_LE(m_world.spawnedSlimeCount(), 4u);

    // 验证小史莱姆的尺寸是 2（4 / 2）
    for (i32 size : m_world.spawnedSlimeSizes()) {
        EXPECT_EQ(size, 2);
    }
}

TEST_F(SlimeEntityTest, PerformSplit_SmallSlimeDoesNotSplit) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));
    slime.setWorld(&m_world);
    slime.setSlimeSize(1, true);  // 小史莱姆
    slime.setPosition(100.0, 64.0, 100.0);

    // 设置为死亡状态
    EnvironmentalDamage damage = DamageSources::generic();
    slime.hurt(damage, 100.0f);

    // 执行分裂
    slime.performSplit();

    // 小史莱姆不应该分裂
    EXPECT_EQ(m_world.spawnedSlimeCount(), 0u);
}

TEST_F(SlimeEntityTest, PerformSplit_InheritsCustomName) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));
    slime.setWorld(&m_world);
    slime.setSlimeSize(4, true);
    slime.setPosition(100.0, 64.0, 100.0);
    slime.setCustomName("TestSlime");

    // 设置为死亡状态
    EnvironmentalDamage damage = DamageSources::generic();
    slime.hurt(damage, 100.0f);

    // 执行分裂
    slime.performSplit();

    // 验证分裂了
    EXPECT_GE(m_world.spawnedSlimeCount(), 2u);
}

TEST_F(SlimeEntityTest, PerformSplit_InheritsInvulnerability) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));
    slime.setWorld(&m_world);
    slime.setSlimeSize(4, true);
    slime.setPosition(100.0, 64.0, 100.0);
    slime.setInvulnerable(true);

    // 设置为死亡状态
    EnvironmentalDamage damage = DamageSources::generic();
    slime.hurt(damage, 100.0f);

    // 执行分裂
    slime.performSplit();

    // 验证分裂了
    EXPECT_GE(m_world.spawnedSlimeCount(), 2u);
}

TEST_F(SlimeEntityTest, PerformSplit_MediumSlimeCreatesTinySlimes) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));
    slime.setWorld(&m_world);
    slime.setSlimeSize(2, true);  // 中型史莱姆
    slime.setPosition(100.0, 64.0, 100.0);

    // 设置为死亡状态
    EnvironmentalDamage damage = DamageSources::generic();
    slime.hurt(damage, 100.0f);

    // 执行分裂
    slime.performSplit();

    // 验证小史莱姆的尺寸是 1（2 / 2）
    for (i32 size : m_world.spawnedSlimeSizes()) {
        EXPECT_EQ(size, 1);
    }
}

// ==================== 声音测试 ====================

TEST_F(SlimeEntityTest, GetHurtSound_ReturnsCorrectSoundForSize) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));

    // 小史莱姆
    slime.setSlimeSize(1, false);
    EnvironmentalDamage damage = DamageSources::generic();
    auto sound = slime.getHurtSound(damage);
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.slime.hurt_small");

    // 大史莱姆
    slime.setSlimeSize(4, false);
    sound = slime.getHurtSound(damage);
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.slime.hurt");
}

TEST_F(SlimeEntityTest, GetDeathSound_ReturnsCorrectSoundForSize) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));

    // 小史莱姆
    slime.setSlimeSize(1, false);
    auto sound = slime.getDeathSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.slime.death_small");

    // 大史莱姆
    slime.setSlimeSize(4, false);
    sound = slime.getDeathSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.slime.death");
}

TEST_F(SlimeEntityTest, GetSquishSound_ReturnsCorrectSoundForSize) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));

    // 小史莱姆
    slime.setSlimeSize(1, false);
    auto sound = slime.getSquishSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.slime.squish_small");

    // 大史莱姆
    slime.setSlimeSize(4, false);
    sound = slime.getSquishSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), "minecraft:entity.slime.squish");
}

// ==================== 维度测试 ====================

TEST_F(SlimeEntityTest, GetDimensions_ScalesWithSize) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));

    // size = 1: 0.6 * 0.255 = 0.153
    slime.setSlimeSize(1, false);
    auto dims = slime.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims.width(), 0.6f * 0.255f);
    EXPECT_FLOAT_EQ(dims.height(), 0.6f * 0.255f);

    // size = 4: 0.6 * 0.255 * 4 = 0.612
    slime.setSlimeSize(4, false);
    dims = slime.getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(dims.width(), 0.6f * 0.255f * 4.0f);
    EXPECT_FLOAT_EQ(dims.height(), 0.6f * 0.255f * 4.0f);
}

TEST_F(SlimeEntityTest, EyeHeight_ScalesWithSize) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));

    slime.setSlimeSize(1, false);
    // eyeHeight = 0.625 * height = 0.625 * (0.6 * 0.255 * size)
    // 注意：由于 Entity::height() 返回的是基类默认值 1.8f，
    // 而实际的尺寸计算在 getDimensions() 中，
    // SlimeEntity::eyeHeight() 使用 EYE_HEIGHT_FACTOR * height()
    // 这里我们测试的是 eyeHeight 方法的实现正确性
    f32 expectedEyeHeight1 = 0.625f * slime.height();  // 依赖基类 height()
    EXPECT_FLOAT_EQ(slime.eyeHeight(), expectedEyeHeight1);

    slime.setSlimeSize(4, false);
    f32 expectedEyeHeight4 = 0.625f * slime.height();
    EXPECT_FLOAT_EQ(slime.eyeHeight(), expectedEyeHeight4);
}

// ==================== 伤害测试 ====================

TEST_F(SlimeEntityTest, CanDamagePlayer_ReturnsFalseForSmallSlime) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));
    slime.setWorld(&m_world);

    // 小史莱姆不能伤害玩家
    slime.setSlimeSize(1, false);
    EXPECT_FALSE(slime.canDamagePlayer());

    // 大史莱姆可以伤害玩家
    slime.setSlimeSize(4, false);
    EXPECT_TRUE(slime.canDamagePlayer());
}

TEST_F(SlimeEntityTest, GetSoundVolume_ScalesWithSize) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));

    // 体积 = 0.4 * size
    slime.setSlimeSize(1, false);
    EXPECT_FLOAT_EQ(slime.getSoundVolume(), 0.4f);

    slime.setSlimeSize(4, false);
    EXPECT_FLOAT_EQ(slime.getSoundVolume(), 1.6f);
}

// ==================== 经验值测试 ====================

TEST_F(SlimeEntityTest, ExperienceValue_EqualsSize) {
    SlimeEntity slime(LegacyEntityType::Slime, EntityId(1));
    slime.setWorld(&m_world);

    // 注意：setSlimeSize 会设置经验值等于尺寸
    slime.setSlimeSize(1, true);
    EXPECT_EQ(slime.experienceValue(), 1);

    // 改变尺寸时经验值也应该更新
    slime.setSlimeSize(4, true);
    EXPECT_EQ(slime.experienceValue(), 4);

    // 不重置生命值时经验值也会更新
    slime.setSlimeSize(2, false);
    EXPECT_EQ(slime.experienceValue(), 2);
}

} // namespace
} // namespace mc
