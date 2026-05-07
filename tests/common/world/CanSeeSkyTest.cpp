#include <gtest/gtest.h>
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/core/Constants.hpp"

#include <unordered_map>

namespace mc {
namespace test {

namespace {

/**
 * @brief 用于测试 canSeeSky 的 Mock World 实现
 *
 * 该 Mock 允许自定义天空光照返回值，以测试 canSeeSky 的不同场景。
 */
class MockWorldForCanSeeSky final : public IWorld {
public:
    // 设置天空光照返回值
    void setSkyLightValue(u8 value) { m_skyLightValue = value; }

    // 设置是否有天空光照
    void setHasSkyLight(bool value) { m_hasSkyLight = value; }

    // ========== IWorld 接口实现 ==========

    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override {
        return nullptr;
    }

    bool setBlockState(i32, i32, i32, const BlockState*) override {
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override {
        return nullptr;
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override {
        return nullptr;
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override {
        return false;
    }

    [[nodiscard]] i32 getHeight(i32, i32) const override {
        return 0;
    }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override {
        return 0;
    }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override {
        return m_skyLightValue;
    }

    [[nodiscard]] bool hasSkyLight() const override {
        return m_hasSkyLight;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override {
        return {};
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override {
        return {};
    }

    [[nodiscard]] PhysicsEngine* physicsEngine() override {
        return nullptr;
    }

    [[nodiscard]] const PhysicsEngine* physicsEngine() const override {
        return nullptr;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override {
        return {};
    }

    [[nodiscard]] DimensionId dimension() const override {
        return static_cast<DimensionId>(0);
    }

    [[nodiscard]] u64 seed() const override {
        return 0;
    }

    [[nodiscard]] u64 currentTick() const override {
        return 0;
    }

    [[nodiscard]] i64 dayTime() const override {
        return 0;
    }

    [[nodiscard]] bool isHardcore() const override {
        return false;
    }

    [[nodiscard]] Difficulty difficulty() const override {
        return Difficulty::Easy;
    }

    [[nodiscard]] bool isClientSide() override {
        return false;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("MockWorldForCanSeeSky::tickManager not implemented");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("MockWorldForCanSeeSky::tickManager not implemented");
    }

    [[nodiscard]] math::Random& getRandom() override {
        throw std::runtime_error("MockWorldForCanSeeSky::getRandom not implemented");
    }

    [[nodiscard]] const math::Random& getRandom() const override {
        throw std::runtime_error("MockWorldForCanSeeSky::getRandom not implemented");
    }

private:
    u8 m_skyLightValue = 15;
    bool m_hasSkyLight = true;
};

} // namespace

/**
 * @brief canSeeSky 测试套件
 *
 * 测试 IWorld::canSeeSky 的默认实现，验证：
 * 1. 天空光照 = 15 时返回 true
 * 2. 天空光照 < 15 时返回 false
 * 3. 无天空光照的维度返回 false
 */
class CanSeeSkyTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }

    MockWorldForCanSeeSky world;
};

// ========== 基础功能测试 ==========

TEST_F(CanSeeSkyTest, ReturnsTrueWhenSkyLightIsMax) {
    // 天空光照为 15 时，canSeeSky 应返回 true
    world.setSkyLightValue(15);

    EXPECT_TRUE(world.canSeeSky(BlockPos(0, 64, 0)));
    EXPECT_TRUE(world.canSeeSky(BlockPos(100, 100, 100)));
    EXPECT_TRUE(world.canSeeSky(BlockPos(-50, 255, -50)));
}

TEST_F(CanSeeSkyTest, ReturnsFalseWhenSkyLightIsBelowMax) {
    // 天空光照 < 15 时，canSeeSky 应返回 false

    world.setSkyLightValue(14);
    EXPECT_FALSE(world.canSeeSky(BlockPos(0, 64, 0)));

    world.setSkyLightValue(10);
    EXPECT_FALSE(world.canSeeSky(BlockPos(0, 64, 0)));

    world.setSkyLightValue(0);
    EXPECT_FALSE(world.canSeeSky(BlockPos(0, 64, 0)));
}

TEST_F(CanSeeSkyTest, ReturnsFalseWhenNoSkyLight) {
    // 无天空光照的维度（如末地、下界），canSeeSky 应返回 false

    world.setHasSkyLight(false);
    world.setSkyLightValue(15);  // 即使天空光照设为 15

    EXPECT_FALSE(world.canSeeSky(BlockPos(0, 64, 0)));
}

TEST_F(CanSeeSkyTest, ReturnsFalseWhenNoSkyLightRegardlessOfLightValue) {
    // 无天空光照维度，任何光照值都应该返回 false

    world.setHasSkyLight(false);

    for (u8 light = 0; light <= 15; ++light) {
        world.setSkyLightValue(light);
        EXPECT_FALSE(world.canSeeSky(BlockPos(0, 64, 0)))
            << "canSeeSky should be false for light level " << static_cast<int>(light);
    }
}

TEST_F(CanSeeSkyTest, AllPositionsReturnSameResult) {
    // canSeeSky 的结果应该只取决于天空光照，不取决于位置

    world.setSkyLightValue(15);
    EXPECT_TRUE(world.canSeeSky(BlockPos(0, 0, 0)));
    EXPECT_TRUE(world.canSeeSky(BlockPos(100, 50, 100)));
    EXPECT_TRUE(world.canSeeSky(BlockPos(-100, 200, -100)));

    world.setSkyLightValue(0);
    EXPECT_FALSE(world.canSeeSky(BlockPos(0, 0, 0)));
    EXPECT_FALSE(world.canSeeSky(BlockPos(100, 50, 100)));
    EXPECT_FALSE(world.canSeeSky(BlockPos(-100, 200, -100)));
}

// ========== 边界条件测试 ==========

TEST_F(CanSeeSkyTest, SkyLightBoundaryValues) {
    // 测试天空光照边界值

    // 正好 15：应该返回 true
    world.setSkyLightValue(15);
    EXPECT_TRUE(world.canSeeSky(BlockPos(0, 64, 0)));

    // 14：应该返回 false（刚好低于最大值）
    world.setSkyLightValue(14);
    EXPECT_FALSE(world.canSeeSky(BlockPos(0, 64, 0)));

    // 1：最小非零光照
    world.setSkyLightValue(1);
    EXPECT_FALSE(world.canSeeSky(BlockPos(0, 64, 0)));

    // 0：完全黑暗
    world.setSkyLightValue(0);
    EXPECT_FALSE(world.canSeeSky(BlockPos(0, 64, 0)));
}

// ========== MC 1.16.5 对齐测试 ==========

TEST_F(CanSeeSkyTest, AlignsWithMC116Implementation) {
    /**
     * MC 1.16.5 实现参考：
     *
     * // IBlockDisplayReader.java
     * default boolean canSeeSky(BlockPos blockPosIn) {
     *    return this.getLightFor(LightType.SKY, blockPosIn) >= this.getMaxLightLevel();
     * }
     *
     * getMaxLightLevel() 返回 15
     *
     * 这意味着 canSeeSky 检查天空光照是否达到最大值。
     * 如果天空光照 >= 15，说明该位置可以看到天空。
     * 如果天空光照 < 15，说明该位置被遮挡（被方块遮挡或处于室内）。
     */

    // 模拟室外位置：天空光照 = 15
    world.setSkyLightValue(15);
    EXPECT_TRUE(world.canSeeSky(BlockPos(0, 64, 0)));

    // 模拟被遮挡位置：天空光照 < 15
    world.setSkyLightValue(12);  // 被部分遮挡
    EXPECT_FALSE(world.canSeeSky(BlockPos(0, 64, 0)));

    // 模拟室内位置：天空光照 = 0
    world.setSkyLightValue(0);
    EXPECT_FALSE(world.canSeeSky(BlockPos(0, 64, 0)));
}

} // namespace test
} // namespace mc
