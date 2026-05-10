#include <gtest/gtest.h>

#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/block/blocks/ocean/SeagrassBlock.hpp"
#include "common/world/block/blocks/ocean/TallSeagrassBlock.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "world/IWorld.hpp"
#include "world/tick/manager/TickManager.hpp"
#include "world/border/WorldBorder.hpp"
#include "core/Constants.hpp"

#include <map>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 海草测试用世界
 *
 * 支持方块状态、流体状态和 TickManager 的测试世界。
 * 继承自 IBlockReader 以支持需要 IBlockReader 参数的方法。
 */
class SeagrassTestWorld final : public IBlockReader {
public:
    SeagrassTestWorld() {
        // 初始化流体注册表
        fluid::FluidRegistry::instance().initialize();
    }

    // 延迟初始化 TickManager（首次调用时初始化）
    void ensureTickManager() {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

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
            m_ownedStates.erase(pos);
            m_fluids.erase(pos);
        } else {
            // 存储 BlockState 的副本
            auto [it, inserted] = m_ownedStates.insert_or_assign(pos, *state);
            m_blocks[pos] = &it->second;

            // 如果方块有流体状态，更新流体记录
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr && !fluidState->isEmpty()) {
                m_fluids[pos] = fluidState;
            } else {
                m_fluids.erase(pos);
            }
        }
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override {
        const BlockPos pos(x, y, z);
        const auto it = m_fluids.find(pos);
        if (it != m_fluids.end()) {
            return it->second;
        }
        // 如果是水方块，返回水源状态
        const BlockState* blockState = getBlockState(x, y, z);
        if (blockState != nullptr && blockState->isLiquid()) {
            // 返回默认水状态
            fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
            if (waterFluid != nullptr) {
                return &waterFluid->defaultState();
            }
        }
        return nullptr;
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() override { return false; }

    void setSeed(u64 seed) { m_seed = seed; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) {
        (void)setBlockState(pos.x, pos.y, pos.z, state);
    }

    void setWaterAt(const BlockPos& pos) {
        // 设置水方块（使用 WATER 方块）
        if (VanillaBlocks::WATER != nullptr) {
            m_blocks[pos] = &VanillaBlocks::WATER->defaultState();
            // 获取水流体并设置其默认状态
            fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
            if (waterFluid != nullptr) {
                m_fluids[pos] = &waterFluid->defaultState();
            }
        }
    }

    void clearWaterAt(const BlockPos& pos) {
        m_fluids.erase(pos);
    }

    [[nodiscard]] bool hasBlockAt(const BlockPos& pos) const {
        return m_blocks.find(pos) != m_blocks.end();
    }

    [[nodiscard]] const BlockState* getBlockAt(const BlockPos& pos) const {
        const auto it = m_blocks.find(pos);
        return it != m_blocks.end() ? it->second : nullptr;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        const_cast<SeagrassTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override {
        return m_random;
    }
    [[nodiscard]] const math::Random& getRandom() const override {
        return m_random;
    }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override {
        return m_worldBorder;
    }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override {
        return m_worldBorder;
    }

    void setCurrentTick(u64 tick) { m_currentTick = tick; }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::map<BlockPos, const fluid::FluidState*> m_fluids;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    world::border::WorldBorder m_worldBorder;
    math::Random m_random{12345};
    u64 m_seed = 12345;
    u64 m_currentTick = 0;
};

/**
 * @brief 海草方块测试夹具
 */
class SeagrassBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }

    SeagrassTestWorld world;
};

// ============================================================================
// SeagrassBlock isValidPosition 测试
// ============================================================================

TEST_F(SeagrassBlockTest, IsValidPosition_RequiresSolidGround) {
    SeagrassBlock seagrass(BlockProperties(Material::SEA_GRASS).noCollision().notSolid());

    const BlockPos pos(5, 10, 5);

    // 没有下方支撑 - 应该无效
    EXPECT_FALSE(seagrass.isValidPosition(seagrass.defaultState(), static_cast<IBlockReader&>(world), pos));

    // 有固体下方支撑
    world.setBlockAt(pos.down(), &VanillaBlocks::STONE->defaultState());

    // 但没有水 - 仍然无效
    EXPECT_FALSE(seagrass.isValidPosition(seagrass.defaultState(), static_cast<IBlockReader&>(world), pos));

    // 添加水源
    world.setWaterAt(pos);

    // 现在应该有效
    EXPECT_TRUE(seagrass.isValidPosition(seagrass.defaultState(), static_cast<IBlockReader&>(world), pos));
}

TEST_F(SeagrassBlockTest, IsValidPosition_RequiresWater) {
    SeagrassBlock seagrass(BlockProperties(Material::SEA_GRASS).noCollision().notSolid());

    const BlockPos pos(5, 10, 5);

    // 设置固体地面
    world.setBlockAt(pos.down(), &VanillaBlocks::SAND->defaultState());

    // 没有水 - 无效
    EXPECT_FALSE(seagrass.isValidPosition(seagrass.defaultState(), static_cast<IBlockReader&>(world), pos));

    // 添加水源
    world.setWaterAt(pos);

    // 现在应该有效
    EXPECT_TRUE(seagrass.isValidPosition(seagrass.defaultState(), static_cast<IBlockReader&>(world), pos));
}

// ============================================================================
// SeagrassBlock IGrowable 接口测试
// ============================================================================

TEST_F(SeagrassBlockTest, CanGrow_ReturnsTrueWhenWaterAbove) {
    SeagrassBlock seagrass(BlockProperties(Material::SEA_GRASS).noCollision().notSolid());

    const BlockPos pos(5, 10, 5);
    const BlockPos abovePos(5, 11, 5);

    // 设置地面和水源
    world.setBlockAt(pos.down(), &VanillaBlocks::SAND->defaultState());
    world.setWaterAt(pos);

    // 上方没有水 - 不能生长
    EXPECT_FALSE(seagrass.canGrow(static_cast<IBlockReader&>(world), pos, seagrass.defaultState(), false));

    // 上方有水源
    world.setWaterAt(abovePos);

    // 现在可以生长
    EXPECT_TRUE(seagrass.canGrow(static_cast<IBlockReader&>(world), pos, seagrass.defaultState(), false));
}

TEST_F(SeagrassBlockTest, CanUseBonemeal_AlwaysReturnsTrue) {
    SeagrassBlock seagrass(BlockProperties(Material::SEA_GRASS).noCollision().notSolid());

    const BlockPos pos(5, 10, 5);
    math::Random random(12345);

    // 骨粉总是有效
    EXPECT_TRUE(seagrass.canUseBonemeal(world, random, pos, seagrass.defaultState()));
}

TEST_F(SeagrassBlockTest, Grow_TransformsToTallSeagrass) {
    SeagrassBlock seagrass(BlockProperties(Material::SEA_GRASS).noCollision().notSolid());

    const BlockPos pos(5, 10, 5);
    const BlockPos abovePos(5, 11, 5);

    // 设置环境：地面 + 水源 + 上方水源
    world.setBlockAt(pos.down(), &VanillaBlocks::SAND->defaultState());
    world.setWaterAt(pos);
    world.setWaterAt(abovePos);

    // 先放置海草
    world.setBlockAt(pos, &seagrass.defaultState());

    math::Random random(12345);

    // 执行生长
    seagrass.grow(world, random, pos, seagrass.defaultState());

    // 检查是否变成高海草
    const BlockState* lowerState = world.getBlockAt(pos);
    ASSERT_NE(lowerState, nullptr) << "Lower block should not be null";
    EXPECT_TRUE(lowerState->is(VanillaBlocks::TALL_SEAGRASS))
        << "Lower block should be tall seagrass";

    // 检查下半部分
    EXPECT_EQ(lowerState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()),
              BlockStateProperties::DoubleBlockHalf::Lower);

    // 检查上半部分
    const BlockState* upperState = world.getBlockAt(abovePos);
    ASSERT_NE(upperState, nullptr) << "Upper block should not be null";
    EXPECT_TRUE(upperState->is(VanillaBlocks::TALL_SEAGRASS))
        << "Upper block should be tall seagrass";
    EXPECT_EQ(upperState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()),
              BlockStateProperties::DoubleBlockHalf::Upper);
}

TEST_F(SeagrassBlockTest, Grow_DoesNothingWithoutWaterAbove) {
    SeagrassBlock seagrass(BlockProperties(Material::SEA_GRASS).noCollision().notSolid());

    const BlockPos pos(5, 10, 5);

    // 设置地面和水源，但上方没有水
    world.setBlockAt(pos.down(), &VanillaBlocks::SAND->defaultState());
    world.setWaterAt(pos);
    world.setBlockAt(pos, &seagrass.defaultState());

    math::Random random(12345);

    // 执行生长 - 不应该做任何事
    seagrass.grow(world, random, pos, seagrass.defaultState());

    // 海草应该还在
    const BlockState* state = world.getBlockAt(pos);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->is(&seagrass)) << "Seagrass should still be there";
}

// ============================================================================
// SeagrassBlock 流体状态测试
// ============================================================================

TEST_F(SeagrassBlockTest, GetFluidState_ReturnsWater) {
    SeagrassBlock seagrass(BlockProperties(Material::SEA_GRASS).noCollision().notSolid());

    const BlockState& state = seagrass.defaultState();
    const fluid::FluidState* fluidState = seagrass.getFluidState(state);

    // 海草应该返回水的流体状态
    ASSERT_NE(fluidState, nullptr) << "Fluid state should not be null";
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()))
        << "Fluid should be water";
}

// ============================================================================
// TallSeagrassBlock 测试
// ============================================================================

TEST_F(SeagrassBlockTest, TallSeagrass_HasCorrectHalfProperty) {
    // 检查高海草是否正确设置了 HALF 属性
    ASSERT_NE(VanillaBlocks::TALL_SEAGRASS, nullptr) << "TALL_SEAGRASS should be initialized";

    const BlockState& defaultState = VanillaBlocks::TALL_SEAGRASS->defaultState();
    EXPECT_EQ(defaultState.get(BlockStateProperties::DOUBLE_BLOCK_HALF()),
              BlockStateProperties::DoubleBlockHalf::Lower);
}

// ============================================================================
// VanillaBlocks 海草注册测试
// ============================================================================

TEST_F(SeagrassBlockTest, VanillaBlocks_SeagrassIsRegistered) {
    ASSERT_NE(VanillaBlocks::SEAGRASS, nullptr) << "SEAGRASS should be registered";
    EXPECT_NE(VanillaBlocks::SEAGRASS->blockId(), 0u) << "SEAGRASS should have non-zero block ID";
}

TEST_F(SeagrassBlockTest, VanillaBlocks_TallSeagrassIsRegistered) {
    ASSERT_NE(VanillaBlocks::TALL_SEAGRASS, nullptr) << "TALL_SEAGRASS should be registered";
    EXPECT_NE(VanillaBlocks::TALL_SEAGRASS->blockId(), 0u) << "TALL_SEAGRASS should have non-zero block ID";
}

} // namespace
