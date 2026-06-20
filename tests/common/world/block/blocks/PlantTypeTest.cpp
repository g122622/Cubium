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

#include "common/item/Items.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <map>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::block_registry;

// ============================================================================
// PlantType 枚举值测试
// ============================================================================

class PlantTypeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(PlantTypeTest, PlantTypeEnumValuesAreDistinct)
{
    EXPECT_NE(PlantType::Plains, PlantType::Desert);
    EXPECT_NE(PlantType::Plains, PlantType::Beach);
    EXPECT_NE(PlantType::Plains, PlantType::Cave);
    EXPECT_NE(PlantType::Plains, PlantType::Water);
    EXPECT_NE(PlantType::Plains, PlantType::Nether);
    EXPECT_NE(PlantType::Plains, PlantType::Crop);
    EXPECT_NE(PlantType::Desert, PlantType::Beach);
    EXPECT_NE(PlantType::Cave, PlantType::Water);
    EXPECT_NE(PlantType::Nether, PlantType::Crop);
}

// ============================================================================
// IPlantable 接口测试 - 通过 VanillaBlocks 验证
// ============================================================================

class IPlantableInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(IPlantableInterfaceTest, BushBlockDerivativesAreIPlantable)
{
    // 验证通过 VanillaBlocks 注册的植物方块实现了 IPlantable 接口
    // 注意：部分植物方块（如 RED_MUSHROOM, LILY_PAD）当前注册为 SimpleBlock 占位，
    // 尚未替换为专门的 Block 子类，因此暂时跳过 dynamic_cast 检查

    if (VanillaBlocks::DANDELION != nullptr) {
        const Block& block = VanillaBlocks::DANDELION->defaultState().getBlock();
        const IPlantable* plant = dynamic_cast<const IPlantable*>(&block);
        EXPECT_NE(plant, nullptr) << "DANDELION should implement IPlantable";
    }

    if (VanillaBlocks::WHEAT != nullptr) {
        const Block& block = VanillaBlocks::WHEAT->defaultState().getBlock();
        const IPlantable* plant = dynamic_cast<const IPlantable*>(&block);
        EXPECT_NE(plant, nullptr) << "WHEAT should implement IPlantable";
    }

    if (VanillaBlocks::CACTUS != nullptr) {
        const Block& block = VanillaBlocks::CACTUS->defaultState().getBlock();
        const IPlantable* plant = dynamic_cast<const IPlantable*>(&block);
        EXPECT_NE(plant, nullptr) << "CACTUS should implement IPlantable";
    }

    if (VanillaBlocks::SUGAR_CANE != nullptr) {
        const Block& block = VanillaBlocks::SUGAR_CANE->defaultState().getBlock();
        const IPlantable* plant = dynamic_cast<const IPlantable*>(&block);
        EXPECT_NE(plant, nullptr) << "SUGAR_CANE should implement IPlantable";
    }

    if (VanillaBlocks::NETHER_WART != nullptr) {
        const Block& block = VanillaBlocks::NETHER_WART->defaultState().getBlock();
        const IPlantable* plant = dynamic_cast<const IPlantable*>(&block);
        EXPECT_NE(plant, nullptr) << "NETHER_WART should implement IPlantable";
    }
}

TEST_F(IPlantableInterfaceTest, NonPlantBlocksAreNotIPlantable)
{
    // 验证非植物方块不实现 IPlantable 接口
    if (VanillaBlocks::STONE != nullptr) {
        const Block& block = VanillaBlocks::STONE->defaultState().getBlock();
        const IPlantable* plant = dynamic_cast<const IPlantable*>(&block);
        EXPECT_EQ(plant, nullptr) << "STONE should NOT implement IPlantable";
    }

    if (VanillaBlocks::DIRT != nullptr) {
        const Block& block = VanillaBlocks::DIRT->defaultState().getBlock();
        const IPlantable* plant = dynamic_cast<const IPlantable*>(&block);
        EXPECT_EQ(plant, nullptr) << "DIRT should NOT implement IPlantable";
    }
}

// ============================================================================
// BlockTags 集成测试
// ============================================================================

class BlockTagsPlantTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(BlockTagsPlantTest, WheatInMaintainsFarmlandTag)
{
    // 小麦方块应在 MAINTAINS_FARMLAND 标签中
    if (VanillaBlocks::WHEAT == nullptr) {
        GTEST_SKIP() << "WHEAT not registered";
    }

    const BlockState& wheatState = VanillaBlocks::WHEAT->defaultState();
    EXPECT_TRUE(BlockTags::MAINTAINS_FARMLAND().contains(wheatState));
}

TEST_F(BlockTagsPlantTest, AirNotInMaintainsFarmlandTag)
{
    // 空气不在 MAINTAINS_FARMLAND 标签中
    if (VanillaBlocks::AIR == nullptr) {
        GTEST_SKIP() << "AIR not registered";
    }

    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    EXPECT_FALSE(BlockTags::MAINTAINS_FARMLAND().contains(airState));
}

TEST_F(BlockTagsPlantTest, DirtInDirtTag)
{
    // DIRT 方块应在 DIRT 标签中
    if (VanillaBlocks::DIRT == nullptr) {
        GTEST_SKIP() << "DIRT not registered";
    }

    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    EXPECT_TRUE(BlockTags::DIRT().contains(dirtState));
}

TEST_F(BlockTagsPlantTest, SandInSandTag)
{
    // SAND 方块应在 SAND 标签中
    if (VanillaBlocks::SAND == nullptr) {
        GTEST_SKIP() << "SAND not registered";
    }

    const BlockState& sandState = VanillaBlocks::SAND->defaultState();
    EXPECT_TRUE(BlockTags::SAND().contains(sandState));
}

// ============================================================================
// canSustainPlant 核心逻辑测试
// ============================================================================

namespace {

/**
 * @brief 测试用 IBlockReader，支持方块存取和基本世界接口
 */
class SustainTestReader final : public IBlockReader {
public:
    using IBlockReader::getBlockState;

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
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Peaceful; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SustainTestReader::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SustainTestReader::tickManager not implemented");
    }

    // Random interface (stubbed for tests)
    [[nodiscard]] math::Random& getRandom() override
    {
        throw std::runtime_error("SustainTestReader::getRandom not implemented");
    }
    [[nodiscard]] const math::Random& getRandom() const override
    {
        throw std::runtime_error("SustainTestReader::getRandom not implemented");
    }

    // WorldBorder interface
    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    world::border::WorldBorder m_worldBorder;
};

/**
 * @brief 从 VanillaBlocks 获取 IPlantable 指针，使用 dynamic_cast 处理多重继承
 *
 * 由于 Block 和 IPlantable 是独立的基类，getBlock() 返回 const Block&，
 * 必须使用 dynamic_cast 进行跨类型转换。
 */
const IPlantable* getIPlantable(const Block* blockPtr)
{
    if (blockPtr == nullptr) {
        return nullptr;
    }
    const Block& block = blockPtr->defaultState().getBlock();
    return dynamic_cast<const IPlantable*>(&block);
}

} // namespace

class CanSustainPlantTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(CanSustainPlantTest, PlainsPlantOnDirtTagBlocks)
{
    // Plains 植物（如花草）可在 DIRT 标签方块上种植
    if (VanillaBlocks::DANDELION == nullptr || VanillaBlocks::DIRT == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    const IPlantable* plant = getIPlantable(VanillaBlocks::DANDELION);
    ASSERT_NE(plant, nullptr) << "DANDELION should implement IPlantable";

    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    SustainTestReader reader;
    BlockPos pos(0, 0, 0);

    EXPECT_TRUE(dirtState.getBlock().canSustainPlant(dirtState, reader, pos, Direction::Up, *plant));
}

TEST_F(CanSustainPlantTest, PlainsPlantOnFarmland)
{
    if (VanillaBlocks::DANDELION == nullptr || VanillaBlocks::FARMLAND == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    const IPlantable* plant = getIPlantable(VanillaBlocks::DANDELION);
    ASSERT_NE(plant, nullptr) << "DANDELION should implement IPlantable";

    const BlockState& farmlandState = VanillaBlocks::FARMLAND->defaultState();
    SustainTestReader reader;
    BlockPos pos(0, 0, 0);

    EXPECT_TRUE(farmlandState.getBlock().canSustainPlant(farmlandState, reader, pos, Direction::Up, *plant));
}

TEST_F(CanSustainPlantTest, PlainsPlantOnStoneFails)
{
    if (VanillaBlocks::DANDELION == nullptr || VanillaBlocks::STONE == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    const IPlantable* plant = getIPlantable(VanillaBlocks::DANDELION);
    ASSERT_NE(plant, nullptr) << "DANDELION should implement IPlantable";

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    SustainTestReader reader;
    BlockPos pos(0, 0, 0);

    EXPECT_FALSE(stoneState.getBlock().canSustainPlant(stoneState, reader, pos, Direction::Up, *plant));
}

TEST_F(CanSustainPlantTest, CropPlantOnlyOnFarmland)
{
    // Crop 植物只能在耕地上种植
    if (VanillaBlocks::WHEAT == nullptr || VanillaBlocks::FARMLAND == nullptr || VanillaBlocks::DIRT == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    const IPlantable* crop = getIPlantable(VanillaBlocks::WHEAT);
    ASSERT_NE(crop, nullptr) << "WHEAT should implement IPlantable";

    SustainTestReader reader;

    const BlockState& farmlandState = VanillaBlocks::FARMLAND->defaultState();
    EXPECT_TRUE(
        farmlandState.getBlock().canSustainPlant(farmlandState, reader, BlockPos(0, 0, 0), Direction::Up, *crop));

    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    EXPECT_FALSE(dirtState.getBlock().canSustainPlant(dirtState, reader, BlockPos(1, 0, 0), Direction::Up, *crop));
}

TEST_F(CanSustainPlantTest, DesertPlantOnSandTagBlocks)
{
    if (VanillaBlocks::CACTUS == nullptr || VanillaBlocks::SAND == nullptr || VanillaBlocks::DIRT == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    const IPlantable* cactus = getIPlantable(VanillaBlocks::CACTUS);
    ASSERT_NE(cactus, nullptr) << "CACTUS should implement IPlantable";

    SustainTestReader reader;

    const BlockState& sandState = VanillaBlocks::SAND->defaultState();
    EXPECT_TRUE(sandState.getBlock().canSustainPlant(sandState, reader, BlockPos(0, 0, 0), Direction::Up, *cactus));

    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    EXPECT_FALSE(dirtState.getBlock().canSustainPlant(dirtState, reader, BlockPos(1, 0, 0), Direction::Up, *cactus));
}

TEST_F(CanSustainPlantTest, BeachPlantOnDirtAndSand)
{
    if (VanillaBlocks::SUGAR_CANE == nullptr || VanillaBlocks::DIRT == nullptr || VanillaBlocks::SAND == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    const IPlantable* sugarCane = getIPlantable(VanillaBlocks::SUGAR_CANE);
    ASSERT_NE(sugarCane, nullptr) << "SUGAR_CANE should implement IPlantable";

    SustainTestReader reader;

    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    EXPECT_TRUE(dirtState.getBlock().canSustainPlant(dirtState, reader, BlockPos(0, 0, 0), Direction::Up, *sugarCane));

    const BlockState& sandState = VanillaBlocks::SAND->defaultState();
    EXPECT_TRUE(sandState.getBlock().canSustainPlant(sandState, reader, BlockPos(1, 0, 0), Direction::Up, *sugarCane));
}

TEST_F(CanSustainPlantTest, NetherPlantOnNyliumAndSoulSoil)
{
    if (VanillaBlocks::NETHER_WART == nullptr) {
        GTEST_SKIP() << "NETHER_WART not registered";
    }

    const IPlantable* netherWart = getIPlantable(VanillaBlocks::NETHER_WART);
    ASSERT_NE(netherWart, nullptr) << "NETHER_WART should implement IPlantable";

    SustainTestReader reader;

    if (VanillaBlocks::CRIMSON_NYLIUM != nullptr) {
        const BlockState& nyliumState = VanillaBlocks::CRIMSON_NYLIUM->defaultState();
        EXPECT_TRUE(
            nyliumState.getBlock().canSustainPlant(nyliumState, reader, BlockPos(0, 0, 0), Direction::Up, *netherWart));
    }

    if (VanillaBlocks::SOUL_SOIL != nullptr) {
        const BlockState& soulSoilState = VanillaBlocks::SOUL_SOIL->defaultState();
        EXPECT_TRUE(soulSoilState.getBlock().canSustainPlant(
            soulSoilState, reader, BlockPos(1, 0, 0), Direction::Up, *netherWart));
    }

    // Nether 植物也可在 DIRT 标签方块上种植
    if (VanillaBlocks::DIRT != nullptr) {
        const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
        EXPECT_TRUE(
            dirtState.getBlock().canSustainPlant(dirtState, reader, BlockPos(2, 0, 0), Direction::Up, *netherWart));
    }
}

TEST_F(CanSustainPlantTest, WaterPlantReturnsFalseByDefault)
{
    if (VanillaBlocks::LILY_PAD == nullptr || VanillaBlocks::DIRT == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    // 注意：LILY_PAD 当前注册为 SimpleBlock 占位，可能未实现 IPlantable
    const IPlantable* lilyPad = getIPlantable(VanillaBlocks::LILY_PAD);
    if (lilyPad == nullptr) {
        GTEST_SKIP() << "LILY_PAD not yet implemented as IPlantable";
    }

    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    SustainTestReader reader;
    EXPECT_FALSE(dirtState.getBlock().canSustainPlant(dirtState, reader, BlockPos(0, 0, 0), Direction::Up, *lilyPad));
}

// ============================================================================
// BushBlock::canSustain 回归测试（通过公共 canSustainPlant 接口验证）
// ============================================================================
// 注意：BushBlock::canSustain() 是 protected 方法，无法直接在测试中调用。
// 但 canSustain() 内部委托给 ground.canSustainPlant()，因此通过
// canSustainPlant 的公共接口即可完整验证 canSustain 的行为。
// 以下测试验证了从 isSolid() 检查迁移到 canSustainPlant() 委托后的回归正确性。

class CanSustainPlantRegressionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(CanSustainPlantRegressionTest, FlowerOnDirtIsValid)
{
    // 花朵（Plains 类型）在 DIRT 方块上应可种植
    // 旧版 isSolid() 检查也允许此组合，但新版通过 PlantType::Plains + DIRT 标签验证
    if (VanillaBlocks::DANDELION == nullptr || VanillaBlocks::DIRT == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    const IPlantable* plant = getIPlantable(VanillaBlocks::DANDELION);
    ASSERT_NE(plant, nullptr) << "DANDELION should implement IPlantable";

    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    SustainTestReader reader;

    EXPECT_TRUE(dirtState.getBlock().canSustainPlant(dirtState, reader, BlockPos(0, 0, 0), Direction::Up, *plant));
}

TEST_F(CanSustainPlantRegressionTest, FlowerOnStoneIsInvalid)
{
    // 花朵不应在石头上种植（旧版 isSolid() 允许，新版遵循 MC 原版行为拒绝）
    // 这验证了从 isSolid() 到 canSustainPlant() 迁移的关键行为变更
    if (VanillaBlocks::DANDELION == nullptr || VanillaBlocks::STONE == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    const IPlantable* plant = getIPlantable(VanillaBlocks::DANDELION);
    ASSERT_NE(plant, nullptr) << "DANDELION should implement IPlantable";

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    SustainTestReader reader;

    EXPECT_FALSE(stoneState.getBlock().canSustainPlant(stoneState, reader, BlockPos(0, 0, 0), Direction::Up, *plant));
}

TEST_F(CanSustainPlantRegressionTest, FlowerOnFarmlandIsValid)
{
    // 花朵可在耕地上种植（Plains 类型支持 Farmland）
    if (VanillaBlocks::DANDELION == nullptr || VanillaBlocks::FARMLAND == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    const IPlantable* plant = getIPlantable(VanillaBlocks::DANDELION);
    ASSERT_NE(plant, nullptr) << "DANDELION should implement IPlantable";

    const BlockState& farmlandState = VanillaBlocks::FARMLAND->defaultState();
    SustainTestReader reader;

    EXPECT_TRUE(
        farmlandState.getBlock().canSustainPlant(farmlandState, reader, BlockPos(0, 0, 0), Direction::Up, *plant));
}

TEST_F(CanSustainPlantRegressionTest, CropOnFarmlandIsValid)
{
    // 农作物只能在耕地上种植
    if (VanillaBlocks::WHEAT == nullptr || VanillaBlocks::FARMLAND == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    const IPlantable* crop = getIPlantable(VanillaBlocks::WHEAT);
    ASSERT_NE(crop, nullptr) << "WHEAT should implement IPlantable";

    const BlockState& farmlandState = VanillaBlocks::FARMLAND->defaultState();
    SustainTestReader reader;

    EXPECT_TRUE(
        farmlandState.getBlock().canSustainPlant(farmlandState, reader, BlockPos(0, 0, 0), Direction::Up, *crop));
}

TEST_F(CanSustainPlantRegressionTest, CropOnDirtIsInvalid)
{
    // 农作物不应在 DIRT 上种植（仅 Farmland 支持 Crop 类型）
    if (VanillaBlocks::WHEAT == nullptr || VanillaBlocks::DIRT == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    const IPlantable* crop = getIPlantable(VanillaBlocks::WHEAT);
    ASSERT_NE(crop, nullptr) << "WHEAT should implement IPlantable";

    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    SustainTestReader reader;

    EXPECT_FALSE(dirtState.getBlock().canSustainPlant(dirtState, reader, BlockPos(0, 0, 0), Direction::Up, *crop));
}
