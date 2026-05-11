#include <gtest/gtest.h>

#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/core/Constants.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用 Mock World，支持 getBlockState、getBrightness 等方法
 */
class PathWeightTestWorld final : public IWorld {
public:
    void setBlock(i32 x, i32 y, i32 z, const BlockState* state) {
        m_blocks[BlockPos(x, y, z)] = state;
    }

    void setBrightness(f32 brightness) {
        m_brightness = brightness;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override {
        m_blocks[BlockPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override {
        return fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] f32 getBrightness(const BlockPos& pos) const override {
        (void)pos;
        return m_brightness;
    }

    // Stub implementations
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
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
    [[nodiscard]] DimensionId dimension() const override { return 0; }  // Overworld
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() override { return false; }

    EntityId spawnEntity(std::unique_ptr<Entity>) override { return 0; }

    // Random interface (stubbed for tests)
    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("PathWeightTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("PathWeightTestWorld::tickManager not implemented");
    }

    // WorldBorder interface (stubbed for tests)
    [[nodiscard]] world::border::WorldBorder& worldBorder() override {
        throw std::runtime_error("PathWeightTestWorld::worldBorder not implemented");
    }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override {
        throw std::runtime_error("PathWeightTestWorld::worldBorder not implemented");
    }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
    f32 m_brightness = 1.0f;
    math::Random m_random{12345};  // 固定种子以便测试可重复
};

// 具体的 AnimalEntity 子类用于测试
class TestAnimalEntity : public AnimalEntity {
public:
    TestAnimalEntity(LegacyEntityType type, EntityId id) : AnimalEntity(type, id) {}

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& /*partner*/) override {
        return std::make_unique<TestAnimalEntity>(LegacyEntityType::Pig, 0);
    }
};

// 具体的 MonsterEntity 子类用于测试
class TestMonsterEntity : public MonsterEntity {
public:
    TestMonsterEntity(LegacyEntityType type, EntityId id) : MonsterEntity(type, id) {}
};

// ==================== AnimalEntity::getPathWeight 测试 ====================

TEST(AnimalEntityGetPathWeightTest, ReturnsHighScoreOnGrassBlock) {
    VanillaBlocks::initialize();

    PathWeightTestWorld world;
    world.setBlock(0, 63, 0, &VanillaBlocks::GRASS_BLOCK->defaultState());
    world.setBrightness(1.0f);

    TestAnimalEntity animal(LegacyEntityType::Pig, EntityId(1));
    animal.setWorld(&world);

    // 脚下是草方块，应该返回 10.0F
    f32 weight = animal.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 10.0f);
}

TEST(AnimalEntityGetPathWeightTest, ReturnsBrightnessMinusHalfOnNonGrassBlock) {
    VanillaBlocks::initialize();

    PathWeightTestWorld world;
    world.setBlock(0, 63, 0, &VanillaBlocks::STONE->defaultState());
    world.setBrightness(1.0f);

    TestAnimalEntity animal(LegacyEntityType::Pig, EntityId(1));
    animal.setWorld(&world);

    // 脚下是石头，亮度 1.0，应该返回 1.0 - 0.5 = 0.5F
    f32 weight = animal.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.5f);
}

TEST(AnimalEntityGetPathWeightTest, ReturnsNegativeScoreInDarkness) {
    VanillaBlocks::initialize();

    PathWeightTestWorld world;
    world.setBlock(0, 63, 0, &VanillaBlocks::STONE->defaultState());
    world.setBrightness(0.0f);

    TestAnimalEntity animal(LegacyEntityType::Pig, EntityId(1));
    animal.setWorld(&world);

    // 脚下是石头，亮度 0.0，应该返回 0.0 - 0.5 = -0.5F
    f32 weight = animal.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, -0.5f);
}

TEST(AnimalEntityGetPathWeightTest, ReturnsZeroWhenNoWorld) {
    VanillaBlocks::initialize();

    TestAnimalEntity animal(LegacyEntityType::Pig, EntityId(1));
    // 没有 world，应该返回 0.0f
    f32 weight = animal.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.0f);
}

TEST(AnimalEntityGetPathWeightTest, PrefersGrassOverHighBrightness) {
    VanillaBlocks::initialize();

    PathWeightTestWorld world;

    // 位置1: 草方块，低亮度
    world.setBlock(0, 63, 0, &VanillaBlocks::GRASS_BLOCK->defaultState());
    world.setBrightness(0.0f);

    TestAnimalEntity animal(LegacyEntityType::Pig, EntityId(1));
    animal.setWorld(&world);

    f32 grassWeight = animal.getPathWeight(0.0f, 64.0f, 0.0f);

    // 位置2: 石头，高亮度
    world.setBlock(10, 63, 10, &VanillaBlocks::STONE->defaultState());
    world.setBrightness(1.0f);

    f32 stoneWeight = animal.getPathWeight(10.0f, 64.0f, 10.0f);

    // 草方块权重应该高于石头（即使石头在明亮处）
    EXPECT_GT(grassWeight, stoneWeight);
    EXPECT_FLOAT_EQ(grassWeight, 10.0f);
    EXPECT_FLOAT_EQ(stoneWeight, 0.5f);
}

// ==================== MonsterEntity::getPathWeight 测试 ====================

TEST(MonsterEntityGetPathWeightTest, PrefersDarkness) {
    VanillaBlocks::initialize();

    PathWeightTestWorld world;
    world.setBrightness(0.0f);  // 完全黑暗

    TestMonsterEntity monster(LegacyEntityType::Zombie, EntityId(1));
    monster.setWorld(&world);

    // 亮度 0.0，应该返回 0.5 - 0.0 = 0.5F
    f32 weight = monster.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.5f);
}

TEST(MonsterEntityGetPathWeightTest, DislikesBrightness) {
    VanillaBlocks::initialize();

    PathWeightTestWorld world;
    world.setBrightness(1.0f);  // 完全明亮

    TestMonsterEntity monster(LegacyEntityType::Zombie, EntityId(1));
    monster.setWorld(&world);

    // 亮度 1.0，应该返回 0.5 - 1.0 = -0.5F
    f32 weight = monster.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, -0.5f);
}

TEST(MonsterEntityGetPathWeightTest, ReturnsZeroWhenNoWorld) {
    VanillaBlocks::initialize();

    TestMonsterEntity monster(LegacyEntityType::Zombie, EntityId(1));
    // 没有 world，应该返回 0.0f
    f32 weight = monster.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.0f);
}

TEST(MonsterEntityGetPathWeightTest, MediumBrightness) {
    VanillaBlocks::initialize();

    PathWeightTestWorld world;
    world.setBrightness(0.5f);

    TestMonsterEntity monster(LegacyEntityType::Zombie, EntityId(1));
    monster.setWorld(&world);

    // 亮度 0.5，应该返回 0.5 - 0.5 = 0.0F
    f32 weight = monster.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.0f);
}

TEST(MonsterEntityGetPathWeightTest, SlightlyDarkPreferredOverBright) {
    VanillaBlocks::initialize();

    PathWeightTestWorld world;

    TestMonsterEntity monster(LegacyEntityType::Zombie, EntityId(1));
    monster.setWorld(&world);

    // 较暗位置
    world.setBrightness(0.2f);
    f32 darkWeight = monster.getPathWeight(0.0f, 64.0f, 0.0f);

    // 较亮位置
    world.setBrightness(0.8f);
    f32 brightWeight = monster.getPathWeight(10.0f, 64.0f, 10.0f);

    // 怪物应该偏好较暗的位置
    EXPECT_GT(darkWeight, brightWeight);
    EXPECT_FLOAT_EQ(darkWeight, 0.3f);   // 0.5 - 0.2
    EXPECT_FLOAT_EQ(brightWeight, -0.3f);  // 0.5 - 0.8
}

} // anonymous namespace
} // namespace mc
