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

/**
 * @file BedItemTest.cpp
 * @brief 床物品单元测试
 *
 * 测试 BedItem 的功能：
 * - getStateForPlacement: 方向感知放置，头部位置可替换性检查
 */

#include <gtest/gtest.h>

#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BedItem.hpp"
#include "common/util/Direction.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/functional/BedBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/Constants.hpp"

#include <map>
#include <memory>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief BedItem 测试用世界桩
 *
 * 支持方块状态读写，用于测试 BedItem::getStateForPlacement。
 * 未设置的方块位置返回 nullptr（空气），可被 canBeReplaced() 判定为 true。
 */
class BedItemTestWorld final : public IWorld {
public:
    BedItemTestWorld() = default;

    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
            m_ownedStates.erase(pos);
        } else {
            auto [it, inserted] = m_ownedStates.insert_or_assign(pos, *state);
            m_blocks[pos] = &it->second;
        }
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<BedItemTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    world::border::WorldBorder m_worldBorder;
    math::Random m_random;
};

} // namespace

// ========== getStateForPlacement 测试 ==========

class BedItemPlacementTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        bed_ = std::make_unique<BedBlock>(
            DyeColor::White, BlockProperties(Material::WOOL).hardness(0.2f).resistance(0.2f));
        bedItem_ = std::make_unique<BedItem>(*bed_, ItemProperties().maxStackSize(1));
    }

    void TearDown() override
    {
        bedItem_.reset();
        bed_.reset();
    }

    /**
     * @brief 创建放置上下文
     * @param world 世界引用
     * @param pos 放置位置
     * @param face 点击面方向
     * @param playerYaw 玩家偏航角（0=南，90=西，-90=东，180=北）
     */
    BlockItemUseContext makeContext(
        IWorld& world, const BlockPos& pos, Direction face = Direction::Up, f32 playerYaw = 0.0f)
    {
        Vector3 hitPos(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f);
        ItemStack stack;
        return BlockItemUseContext(world, nullptr, stack, hitPos, pos, face, playerYaw, 0.0f);
    }

    std::unique_ptr<BedBlock> bed_;
    std::unique_ptr<BedItem> bedItem_;
    BedItemTestWorld world;
};

TEST_F(BedItemPlacementTest, ReturnsFootStateWithCorrectFacing)
{
    // 面向南方（yaw=0），头部位置 (5,64,6) 为空气，应成功放置
    auto context = makeContext(world, BlockPos(5, 64, 5), Direction::Up, 0.0f);
    const BlockState* result = bedItem_->getStateForPlacement(context);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
    EXPECT_EQ(result->get(BlockStateProperties::BED_PART()), BlockStateProperties::BedPart::Foot);
}

TEST_F(BedItemPlacementTest, ReturnsNorthFacingWhenPlayerFacesNorth)
{
    // 面向北方（yaw=180），头部位置 (5,64,4) 为空气
    auto context = makeContext(world, BlockPos(5, 64, 5), Direction::Up, 180.0f);
    const BlockState* result = bedItem_->getStateForPlacement(context);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(BedItemPlacementTest, ReturnsEastFacingWhenPlayerFacesEast)
{
    // 面向东方（yaw=-90），头部位置 (6,64,5) 为空气
    auto context = makeContext(world, BlockPos(5, 64, 5), Direction::Up, -90.0f);
    const BlockState* result = bedItem_->getStateForPlacement(context);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
}

TEST_F(BedItemPlacementTest, ReturnsWestFacingWhenPlayerFacesWest)
{
    // 面向西方（yaw=90），头部位置 (4,64,5) 为空气
    auto context = makeContext(world, BlockPos(5, 64, 5), Direction::Up, 90.0f);
    const BlockState* result = bedItem_->getStateForPlacement(context);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::West);
}

TEST_F(BedItemPlacementTest, ReturnsNullptrWhenHeadPositionBlocked)
{
    // 放置脚部位置 (5,64,5) 面朝南，但头部位置 (5,64,6) 被石头方块占据（不可替换）
    // 需要先放一个不可替换方块在头部位置
    auto& stoneState = VanillaBlocks::STONE->defaultState();
    world.setBlockAt(BlockPos(5, 64, 6), &stoneState);

    auto context = makeContext(world, BlockPos(5, 64, 5), Direction::Up, 0.0f);
    const BlockState* result = bedItem_->getStateForPlacement(context);

    // 头部位置被不可替换方块占据，应返回 nullptr
    EXPECT_EQ(result, nullptr);
}

TEST_F(BedItemPlacementTest, ReturnsFootStateWhenHeadPositionIsAir)
{
    // 空气位置可以放置 → 应返回有效状态
    // 默认世界全是 nullptr（空气），不需要额外设置
    auto context = makeContext(world, BlockPos(0, 64, 0), Direction::Up, 0.0f);
    const BlockState* result = bedItem_->getStateForPlacement(context);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->get(BlockStateProperties::BED_PART()), BlockStateProperties::BedPart::Foot);
}

TEST_F(BedItemPlacementTest, AllFourDirectionsSuccess)
{
    // 四个方向均应成功放置（世界为空）
    struct TestCase {
        f32 yaw;
        Direction expectedFacing;
        BlockPos headOffset;
    };

    TestCase cases[] = {
        {0.0f, Direction::South, BlockPos(0, 64, 1)},
        {180.0f, Direction::North, BlockPos(0, 64, -1)},
        {-90.0f, Direction::East, BlockPos(1, 64, 0)},
        {90.0f, Direction::West, BlockPos(-1, 64, 0)},
    };

    for (const auto& tc : cases) {
        BedItemTestWorld dirWorld;
        auto context = makeContext(dirWorld, BlockPos(0, 64, 0), Direction::Up, tc.yaw);
        const BlockState* result = bedItem_->getStateForPlacement(context);

        ASSERT_NE(result, nullptr) << "Failed for yaw=" << tc.yaw;
        EXPECT_EQ(result->get(BlockStateProperties::HORIZONTAL_FACING()), tc.expectedFacing)
            << "Failed for yaw=" << tc.yaw;
        EXPECT_EQ(result->get(BlockStateProperties::BED_PART()), BlockStateProperties::BedPart::Foot)
            << "Failed for yaw=" << tc.yaw;
    }
}

TEST_F(BedItemPlacementTest, PlacementResultHasCorrectBlock)
{
    // 放置结果的状态应该属于正确的 BedBlock
    auto context = makeContext(world, BlockPos(5, 64, 5), Direction::Up, 0.0f);
    const BlockState* result = bedItem_->getStateForPlacement(context);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(&result->getBlock(), bed_.get());
}
