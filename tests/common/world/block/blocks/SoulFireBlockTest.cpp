#include <gtest/gtest.h>

#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/blocks/nether/FireBlock.hpp"
#include "world/block/blocks/nether/SoulFireBlock.hpp"
#include "world/IWorld.hpp"
#include "world/tick/manager/TickManager.hpp"
#include "world/border/WorldBorder.hpp"
#include "core/Constants.hpp"

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 灵魂火测试用世界
 *
 * 继承 IBlockReader，提供最小化测试环境
 */
class SoulFireTestWorld final : public IBlockReader {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = state;
        }
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
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
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() override { return false; }
    [[nodiscard]] bool isUltraWarm() const override { return false; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) {
        (void)setBlockState(pos.x, pos.y, pos.z, state);
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        const_cast<SoulFireTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    void ensureTickManager() const {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(const_cast<SoulFireTestWorld&>(*this));
        }
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    mutable std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
};

class SoulFireBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

TEST_F(SoulFireBlockTest, SoulFireBaseBlocksTagExists) {
    // 验证 SOUL_FIRE_BASE_BLOCKS 标签已创建
    EXPECT_NO_THROW({
        BlockTag& tag = BlockTags::SOUL_FIRE_BASE_BLOCKS();
        EXPECT_TRUE(true);
    });
}

TEST_F(SoulFireBlockTest, SoulFireBaseBlocksTagContainsSoulSand) {
    ASSERT_NE(VanillaBlocks::SOUL_SAND, nullptr);
    EXPECT_TRUE(BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(VanillaBlocks::SOUL_SAND));
}

TEST_F(SoulFireBlockTest, SoulFireBaseBlocksTagContainsSoulSoil) {
    ASSERT_NE(VanillaBlocks::SOUL_SOIL, nullptr);
    EXPECT_TRUE(BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(VanillaBlocks::SOUL_SOIL));
}

TEST_F(SoulFireBlockTest, SoulFireBaseBlocksTagDoesNotContainStone) {
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    EXPECT_FALSE(BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(VanillaBlocks::STONE));
}

TEST_F(SoulFireBlockTest, IsSoulFireBaseReturnsTrueForSoulSand) {
    ASSERT_NE(VanillaBlocks::SOUL_SAND, nullptr);
    EXPECT_TRUE(SoulFireBlock::isSoulFireBase(VanillaBlocks::SOUL_SAND));
}

TEST_F(SoulFireBlockTest, IsSoulFireBaseReturnsTrueForSoulSoil) {
    ASSERT_NE(VanillaBlocks::SOUL_SOIL, nullptr);
    EXPECT_TRUE(SoulFireBlock::isSoulFireBase(VanillaBlocks::SOUL_SOIL));
}

TEST_F(SoulFireBlockTest, IsSoulFireBaseReturnsFalseForStone) {
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    EXPECT_FALSE(SoulFireBlock::isSoulFireBase(VanillaBlocks::STONE));
}

TEST_F(SoulFireBlockTest, IsSoulFireBaseReturnsFalseForNullptr) {
    EXPECT_FALSE(SoulFireBlock::isSoulFireBase(nullptr));
}

TEST_F(SoulFireBlockTest, SoulFireIsValidPositionOnSoulSand) {
    SoulFireTestWorld world;

    ASSERT_NE(VanillaBlocks::SOUL_SAND, nullptr);
    ASSERT_NE(VanillaBlocks::SOUL_FIRE, nullptr);

    // 在灵魂沙上方放置灵魂火
    const BlockPos firePos(5, 64, 5);
    const BlockPos sandPos(5, 63, 5);

    world.setBlockAt(sandPos, &VanillaBlocks::SOUL_SAND->defaultState());

    const BlockState& fireState = VanillaBlocks::SOUL_FIRE->defaultState();
    EXPECT_TRUE(VanillaBlocks::SOUL_FIRE->isValidPosition(fireState, world, firePos));
}

TEST_F(SoulFireBlockTest, SoulFireIsValidPositionOnSoulSoil) {
    SoulFireTestWorld world;

    ASSERT_NE(VanillaBlocks::SOUL_SOIL, nullptr);
    ASSERT_NE(VanillaBlocks::SOUL_FIRE, nullptr);

    // 在灵魂土上方放置灵魂火
    const BlockPos firePos(7, 64, 7);
    const BlockPos soilPos(7, 63, 7);

    world.setBlockAt(soilPos, &VanillaBlocks::SOUL_SOIL->defaultState());

    const BlockState& fireState = VanillaBlocks::SOUL_FIRE->defaultState();
    EXPECT_TRUE(VanillaBlocks::SOUL_FIRE->isValidPosition(fireState, world, firePos));
}

TEST_F(SoulFireBlockTest, SoulFireNotValidPositionOnStone) {
    SoulFireTestWorld world;

    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    ASSERT_NE(VanillaBlocks::SOUL_FIRE, nullptr);

    // 灵魂火不能放置在石头上方
    const BlockPos firePos(9, 64, 9);
    const BlockPos stonePos(9, 63, 9);

    world.setBlockAt(stonePos, &VanillaBlocks::STONE->defaultState());

    const BlockState& fireState = VanillaBlocks::SOUL_FIRE->defaultState();
    EXPECT_FALSE(VanillaBlocks::SOUL_FIRE->isValidPosition(fireState, world, firePos));
}

TEST_F(SoulFireBlockTest, SoulFireNotValidPositionOnAir) {
    SoulFireTestWorld world;

    ASSERT_NE(VanillaBlocks::SOUL_FIRE, nullptr);

    // 灵魂火不能放置在空气上方
    const BlockPos firePos(11, 64, 11);

    const BlockState& fireState = VanillaBlocks::SOUL_FIRE->defaultState();
    EXPECT_FALSE(VanillaBlocks::SOUL_FIRE->isValidPosition(fireState, world, firePos));
}

TEST_F(SoulFireBlockTest, FireTagContainsBothFireTypes) {
    // 验证 FIRE 标签同时包含普通火和灵魂火
    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    ASSERT_NE(VanillaBlocks::SOUL_FIRE, nullptr);

    EXPECT_TRUE(BlockTags::FIRE().contains(VanillaBlocks::FIRE));
    EXPECT_TRUE(BlockTags::FIRE().contains(VanillaBlocks::SOUL_FIRE));
}

} // namespace
