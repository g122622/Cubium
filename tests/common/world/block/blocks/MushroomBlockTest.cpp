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
#include "common/world/block/FireInfoRegistry.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/blocks/vegetation/MushroomBlock.hpp"
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
// 测试辅助类
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

} // namespace

// ============================================================================
// MUSHROOM_GROW_BLOCK 标签测试
// ============================================================================

class MushroomGrowBlockTagTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

TEST_F(MushroomGrowBlockTagTest, TagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::MUSHROOM_GROW_BLOCK().getId(), ResourceLocation("minecraft", "mushroom_grow_block"));
}

TEST_F(MushroomGrowBlockTagTest, ContainsMycelium)
{
    if (VanillaBlocks::MYCELIUM == nullptr) {
        GTEST_SKIP() << "MYCELIUM not registered";
    }
    EXPECT_TRUE(BlockTags::MUSHROOM_GROW_BLOCK().contains(*VanillaBlocks::MYCELIUM));
}

TEST_F(MushroomGrowBlockTagTest, ContainsPodzol)
{
    if (VanillaBlocks::PODZOL == nullptr) {
        GTEST_SKIP() << "PODZOL not registered";
    }
    EXPECT_TRUE(BlockTags::MUSHROOM_GROW_BLOCK().contains(*VanillaBlocks::PODZOL));
}

TEST_F(MushroomGrowBlockTagTest, ContainsCrimsonNylium)
{
    if (VanillaBlocks::CRIMSON_NYLIUM == nullptr) {
        GTEST_SKIP() << "CRIMSON_NYLIUM not registered";
    }
    EXPECT_TRUE(BlockTags::MUSHROOM_GROW_BLOCK().contains(*VanillaBlocks::CRIMSON_NYLIUM));
}

TEST_F(MushroomGrowBlockTagTest, ContainsWarpedNylium)
{
    if (VanillaBlocks::WARPED_NYLIUM == nullptr) {
        GTEST_SKIP() << "WARPED_NYLIUM not registered";
    }
    EXPECT_TRUE(BlockTags::MUSHROOM_GROW_BLOCK().contains(*VanillaBlocks::WARPED_NYLIUM));
}

TEST_F(MushroomGrowBlockTagTest, DoesNotContainDirt)
{
    if (VanillaBlocks::DIRT == nullptr) {
        GTEST_SKIP() << "DIRT not registered";
    }
    EXPECT_FALSE(BlockTags::MUSHROOM_GROW_BLOCK().contains(*VanillaBlocks::DIRT));
}

TEST_F(MushroomGrowBlockTagTest, DoesNotContainStone)
{
    if (VanillaBlocks::STONE == nullptr) {
        GTEST_SKIP() << "STONE not registered";
    }
    EXPECT_FALSE(BlockTags::MUSHROOM_GROW_BLOCK().contains(*VanillaBlocks::STONE));
}

TEST_F(MushroomGrowBlockTagTest, DoesNotContainGrassBlock)
{
    if (VanillaBlocks::GRASS_BLOCK == nullptr) {
        GTEST_SKIP() << "GRASS_BLOCK not registered";
    }
    EXPECT_FALSE(BlockTags::MUSHROOM_GROW_BLOCK().contains(*VanillaBlocks::GRASS_BLOCK));
}

// ============================================================================
// MushroomBlock IPlantable 接口测试
// ============================================================================

class MushroomBlockTypeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

TEST_F(MushroomBlockTypeTest, BrownMushroomIsMushroomBlock)
{
    // 验证棕色蘑菇注册为 MushroomBlock 类型（而非 SimpleBlock）
    if (VanillaBlocks::BROWN_MUSHROOM == nullptr) {
        GTEST_SKIP() << "BROWN_MUSHROOM not registered";
    }
    const Block& block = VanillaBlocks::BROWN_MUSHROOM->defaultState().getBlock();
    const auto* mushroomBlock = dynamic_cast<const blocks::MushroomBlock*>(&block);
    EXPECT_NE(mushroomBlock, nullptr) << "BROWN_MUSHROOM should be registered as MushroomBlock";
}

TEST_F(MushroomBlockTypeTest, RedMushroomIsMushroomBlock)
{
    // 验证红色蘑菇注册为 MushroomBlock 类型（而非 SimpleBlock）
    if (VanillaBlocks::RED_MUSHROOM == nullptr) {
        GTEST_SKIP() << "RED_MUSHROOM not registered";
    }
    const Block& block = VanillaBlocks::RED_MUSHROOM->defaultState().getBlock();
    const auto* mushroomBlock = dynamic_cast<const blocks::MushroomBlock*>(&block);
    EXPECT_NE(mushroomBlock, nullptr) << "RED_MUSHROOM should be registered as MushroomBlock";
}

TEST_F(MushroomBlockTypeTest, BrownMushroomReturnsPlantTypeCave)
{
    // 验证蘑菇返回 PlantType::Cave
    if (VanillaBlocks::BROWN_MUSHROOM == nullptr) {
        GTEST_SKIP() << "BROWN_MUSHROOM not registered";
    }
    const Block& block = VanillaBlocks::BROWN_MUSHROOM->defaultState().getBlock();
    const auto* plant = dynamic_cast<const IPlantable*>(&block);
    ASSERT_NE(plant, nullptr) << "BROWN_MUSHROOM should implement IPlantable";

    SustainTestReader reader;
    BlockPos pos(0, 0, 0);
    EXPECT_EQ(plant->getPlantType(reader, pos), PlantType::Cave);
}

TEST_F(MushroomBlockTypeTest, BrownMushroomBlockIsHugeMushroomBlock)
{
    // 验证棕色蘑菇方块注册为 HugeMushroomBlock 类型
    if (VanillaBlocks::BROWN_MUSHROOM_BLOCK == nullptr) {
        GTEST_SKIP() << "BROWN_MUSHROOM_BLOCK not registered";
    }
    const Block& block = VanillaBlocks::BROWN_MUSHROOM_BLOCK->defaultState().getBlock();
    const auto* hugeMushroom = dynamic_cast<const blocks::HugeMushroomBlock*>(&block);
    EXPECT_NE(hugeMushroom, nullptr) << "BROWN_MUSHROOM_BLOCK should be registered as HugeMushroomBlock";
}

TEST_F(MushroomBlockTypeTest, RedMushroomBlockIsHugeMushroomBlock)
{
    // 验证红色蘑菇方块注册为 HugeMushroomBlock 类型
    if (VanillaBlocks::RED_MUSHROOM_BLOCK == nullptr) {
        GTEST_SKIP() << "RED_MUSHROOM_BLOCK not registered";
    }
    const Block& block = VanillaBlocks::RED_MUSHROOM_BLOCK->defaultState().getBlock();
    const auto* hugeMushroom = dynamic_cast<const blocks::HugeMushroomBlock*>(&block);
    EXPECT_NE(hugeMushroom, nullptr) << "RED_MUSHROOM_BLOCK should be registered as HugeMushroomBlock";
}

TEST_F(MushroomBlockTypeTest, MushroomStemIsHugeMushroomBlock)
{
    // 验证蘑菇柄注册为 HugeMushroomBlock 类型
    if (VanillaBlocks::MUSHROOM_STEM == nullptr) {
        GTEST_SKIP() << "MUSHROOM_STEM not registered";
    }
    const Block& block = VanillaBlocks::MUSHROOM_STEM->defaultState().getBlock();
    const auto* hugeMushroom = dynamic_cast<const blocks::HugeMushroomBlock*>(&block);
    EXPECT_NE(hugeMushroom, nullptr) << "MUSHROOM_STEM should be registered as HugeMushroomBlock";
}

TEST_F(MushroomBlockTypeTest, HugeMushroomBlockHasDirectionalProperties)
{
    // 验证巨型蘑菇方块拥有方向属性（6个布尔属性）
    if (VanillaBlocks::BROWN_MUSHROOM_BLOCK == nullptr) {
        GTEST_SKIP() << "BROWN_MUSHROOM_BLOCK not registered";
    }
    const BlockState& state = VanillaBlocks::BROWN_MUSHROOM_BLOCK->defaultState();
    // 默认状态下所有方向都为 true
    EXPECT_TRUE(state.get(BlockStateProperties::DOWN()));
    EXPECT_TRUE(state.get(BlockStateProperties::UP()));
    EXPECT_TRUE(state.get(BlockStateProperties::NORTH()));
    EXPECT_TRUE(state.get(BlockStateProperties::SOUTH()));
    EXPECT_TRUE(state.get(BlockStateProperties::EAST()));
    EXPECT_TRUE(state.get(BlockStateProperties::WEST()));
}

// ============================================================================
// canSustainPlant PlantType::Cave 测试（通过 MUSHROOM_GROW_BLOCK 标签）
// ============================================================================

class MushroomCanSustainTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

TEST_F(MushroomCanSustainTest, CavePlantOnMycelium)
{
    // 蘑菇可在菌丝上种植（通过 MUSHROOM_GROW_BLOCK 标签）
    if (VanillaBlocks::BROWN_MUSHROOM == nullptr || VanillaBlocks::MYCELIUM == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }
    const IPlantable* mushroom =
        dynamic_cast<const IPlantable*>(&VanillaBlocks::BROWN_MUSHROOM->defaultState().getBlock());
    ASSERT_NE(mushroom, nullptr) << "BROWN_MUSHROOM should implement IPlantable";

    const BlockState& myceliumState = VanillaBlocks::MYCELIUM->defaultState();
    SustainTestReader reader;
    BlockPos pos(0, 0, 0);

    EXPECT_TRUE(myceliumState.getBlock().canSustainPlant(myceliumState, reader, pos, Direction::Up, *mushroom));
}

TEST_F(MushroomCanSustainTest, CavePlantOnPodzol)
{
    // 蘑菇可在灰化土上种植（通过 MUSHROOM_GROW_BLOCK 标签）
    if (VanillaBlocks::BROWN_MUSHROOM == nullptr || VanillaBlocks::PODZOL == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }
    const IPlantable* mushroom =
        dynamic_cast<const IPlantable*>(&VanillaBlocks::BROWN_MUSHROOM->defaultState().getBlock());
    ASSERT_NE(mushroom, nullptr);

    const BlockState& podzolState = VanillaBlocks::PODZOL->defaultState();
    SustainTestReader reader;
    BlockPos pos(0, 0, 0);

    EXPECT_TRUE(podzolState.getBlock().canSustainPlant(podzolState, reader, pos, Direction::Up, *mushroom));
}

TEST_F(MushroomCanSustainTest, CavePlantOnCrimsonNylium)
{
    // 蘑菇可在绯红菌岩上种植（通过 MUSHROOM_GROW_BLOCK 标签）
    if (VanillaBlocks::BROWN_MUSHROOM == nullptr || VanillaBlocks::CRIMSON_NYLIUM == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }
    const IPlantable* mushroom =
        dynamic_cast<const IPlantable*>(&VanillaBlocks::BROWN_MUSHROOM->defaultState().getBlock());
    ASSERT_NE(mushroom, nullptr);

    const BlockState& nyliumState = VanillaBlocks::CRIMSON_NYLIUM->defaultState();
    SustainTestReader reader;
    BlockPos pos(0, 0, 0);

    EXPECT_TRUE(nyliumState.getBlock().canSustainPlant(nyliumState, reader, pos, Direction::Up, *mushroom));
}

TEST_F(MushroomCanSustainTest, CavePlantOnWarpedNylium)
{
    // 蘑菇可在诡异菌岩上种植（通过 MUSHROOM_GROW_BLOCK 标签）
    if (VanillaBlocks::BROWN_MUSHROOM == nullptr || VanillaBlocks::WARPED_NYLIUM == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }
    const IPlantable* mushroom =
        dynamic_cast<const IPlantable*>(&VanillaBlocks::BROWN_MUSHROOM->defaultState().getBlock());
    ASSERT_NE(mushroom, nullptr);

    const BlockState& nyliumState = VanillaBlocks::WARPED_NYLIUM->defaultState();
    SustainTestReader reader;
    BlockPos pos(0, 0, 0);

    EXPECT_TRUE(nyliumState.getBlock().canSustainPlant(nyliumState, reader, pos, Direction::Up, *mushroom));
}

TEST_F(MushroomCanSustainTest, CavePlantOnDirtFails)
{
    // 蘑菇不应通过 canSustainPlant 在普通泥土上种植（光照检查由 MushroomBlock 自身处理）
    if (VanillaBlocks::BROWN_MUSHROOM == nullptr || VanillaBlocks::DIRT == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }
    const IPlantable* mushroom =
        dynamic_cast<const IPlantable*>(&VanillaBlocks::BROWN_MUSHROOM->defaultState().getBlock());
    ASSERT_NE(mushroom, nullptr);

    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    SustainTestReader reader;
    BlockPos pos(0, 0, 0);

    EXPECT_FALSE(dirtState.getBlock().canSustainPlant(dirtState, reader, pos, Direction::Up, *mushroom));
}

TEST_F(MushroomCanSustainTest, CavePlantOnStoneFails)
{
    // 蘑菇不应在石头上种植
    if (VanillaBlocks::BROWN_MUSHROOM == nullptr || VanillaBlocks::STONE == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }
    const IPlantable* mushroom =
        dynamic_cast<const IPlantable*>(&VanillaBlocks::BROWN_MUSHROOM->defaultState().getBlock());
    ASSERT_NE(mushroom, nullptr);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    SustainTestReader reader;
    BlockPos pos(0, 0, 0);

    EXPECT_FALSE(stoneState.getBlock().canSustainPlant(stoneState, reader, pos, Direction::Up, *mushroom));
}

// ============================================================================
// FireInfoRegistry 补充火焰参数测试
// ============================================================================

class FireInfoRegistryStairsSlabsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        blocks::FireInfoRegistry::instance().initializeVanillaFireInfos();
    }
};

TEST_F(FireInfoRegistryStairsSlabsTest, SpruceStairsHasFireInfo)
{
    if (VanillaBlocks::SPRUCE_STAIRS == nullptr) {
        GTEST_SKIP() << "SPRUCE_STAIRS not registered";
    }
    EXPECT_GT(blocks::FireInfoRegistry::instance().getFlammability(VanillaBlocks::SPRUCE_STAIRS->blockId()), 0);
    EXPECT_GT(blocks::FireInfoRegistry::instance().getEncouragement(VanillaBlocks::SPRUCE_STAIRS->blockId()), 0);
}

TEST_F(FireInfoRegistryStairsSlabsTest, BirchStairsHasFireInfo)
{
    if (VanillaBlocks::BIRCH_STAIRS == nullptr) {
        GTEST_SKIP() << "BIRCH_STAIRS not registered";
    }
    EXPECT_GT(blocks::FireInfoRegistry::instance().getFlammability(VanillaBlocks::BIRCH_STAIRS->blockId()), 0);
    EXPECT_GT(blocks::FireInfoRegistry::instance().getEncouragement(VanillaBlocks::BIRCH_STAIRS->blockId()), 0);
}

TEST_F(FireInfoRegistryStairsSlabsTest, JungleStairsHasFireInfo)
{
    if (VanillaBlocks::JUNGLE_STAIRS == nullptr) {
        GTEST_SKIP() << "JUNGLE_STAIRS not registered";
    }
    EXPECT_GT(blocks::FireInfoRegistry::instance().getFlammability(VanillaBlocks::JUNGLE_STAIRS->blockId()), 0);
    EXPECT_GT(blocks::FireInfoRegistry::instance().getEncouragement(VanillaBlocks::JUNGLE_STAIRS->blockId()), 0);
}

TEST_F(FireInfoRegistryStairsSlabsTest, AcaciaStairsHasFireInfo)
{
    if (VanillaBlocks::ACACIA_STAIRS == nullptr) {
        GTEST_SKIP() << "ACACIA_STAIRS not registered";
    }
    EXPECT_GT(blocks::FireInfoRegistry::instance().getFlammability(VanillaBlocks::ACACIA_STAIRS->blockId()), 0);
    EXPECT_GT(blocks::FireInfoRegistry::instance().getEncouragement(VanillaBlocks::ACACIA_STAIRS->blockId()), 0);
}

TEST_F(FireInfoRegistryStairsSlabsTest, DarkOakStairsHasFireInfo)
{
    if (VanillaBlocks::DARK_OAK_STAIRS == nullptr) {
        GTEST_SKIP() << "DARK_OAK_STAIRS not registered";
    }
    EXPECT_GT(blocks::FireInfoRegistry::instance().getFlammability(VanillaBlocks::DARK_OAK_STAIRS->blockId()), 0);
    EXPECT_GT(blocks::FireInfoRegistry::instance().getEncouragement(VanillaBlocks::DARK_OAK_STAIRS->blockId()), 0);
}

TEST_F(FireInfoRegistryStairsSlabsTest, SpruceSlabHasFireInfo)
{
    if (VanillaBlocks::SPRUCE_SLAB == nullptr) {
        GTEST_SKIP() << "SPRUCE_SLAB not registered";
    }
    EXPECT_GT(blocks::FireInfoRegistry::instance().getFlammability(VanillaBlocks::SPRUCE_SLAB->blockId()), 0);
    EXPECT_GT(blocks::FireInfoRegistry::instance().getEncouragement(VanillaBlocks::SPRUCE_SLAB->blockId()), 0);
}

TEST_F(FireInfoRegistryStairsSlabsTest, BirchSlabHasFireInfo)
{
    if (VanillaBlocks::BIRCH_SLAB == nullptr) {
        GTEST_SKIP() << "BIRCH_SLAB not registered";
    }
    EXPECT_GT(blocks::FireInfoRegistry::instance().getFlammability(VanillaBlocks::BIRCH_SLAB->blockId()), 0);
    EXPECT_GT(blocks::FireInfoRegistry::instance().getEncouragement(VanillaBlocks::BIRCH_SLAB->blockId()), 0);
}

TEST_F(FireInfoRegistryStairsSlabsTest, WoodStairsHaveCorrectFireValues)
{
    // 木质楼梯的火焰参数应为 ignite=5, burn=20
    if (VanillaBlocks::OAK_STAIRS == nullptr) {
        GTEST_SKIP() << "OAK_STAIRS not registered";
    }
    EXPECT_EQ(blocks::FireInfoRegistry::instance().getEncouragement(VanillaBlocks::OAK_STAIRS->blockId()), 5);
    EXPECT_EQ(blocks::FireInfoRegistry::instance().getFlammability(VanillaBlocks::OAK_STAIRS->blockId()), 20);
}

TEST_F(FireInfoRegistryStairsSlabsTest, WoodSlabsHaveCorrectFireValues)
{
    // 木质台阶的火焰参数应为 ignite=5, burn=20
    if (VanillaBlocks::OAK_SLAB == nullptr) {
        GTEST_SKIP() << "OAK_SLAB not registered";
    }
    EXPECT_EQ(blocks::FireInfoRegistry::instance().getEncouragement(VanillaBlocks::OAK_SLAB->blockId()), 5);
    EXPECT_EQ(blocks::FireInfoRegistry::instance().getFlammability(VanillaBlocks::OAK_SLAB->blockId()), 20);
}

TEST_F(FireInfoRegistryStairsSlabsTest, StoneStairsHasNoFireInfo)
{
    // 石头楼梯不应有火焰参数
    if (VanillaBlocks::STONE_STAIRS == nullptr) {
        GTEST_SKIP() << "STONE_STAIRS not registered";
    }
    EXPECT_EQ(blocks::FireInfoRegistry::instance().getFlammability(VanillaBlocks::STONE_STAIRS->blockId()), 0);
    EXPECT_EQ(blocks::FireInfoRegistry::instance().getEncouragement(VanillaBlocks::STONE_STAIRS->blockId()), 0);
}

TEST_F(FireInfoRegistryStairsSlabsTest, BrownMushroomBlockHasFireInfo)
{
    // 蘑菇方块（BROWN_MUSHROOM_BLOCK）在 MC 原版中注册了火焰信息
    // ignite=5(IGNITE_HARD), burn=20(BURN_MEDIUM)
    if (VanillaBlocks::BROWN_MUSHROOM_BLOCK == nullptr) {
        GTEST_SKIP() << "BROWN_MUSHROOM_BLOCK not registered";
    }
    EXPECT_EQ(blocks::FireInfoRegistry::instance().getEncouragement(VanillaBlocks::BROWN_MUSHROOM_BLOCK->blockId()), 5);
    EXPECT_EQ(blocks::FireInfoRegistry::instance().getFlammability(VanillaBlocks::BROWN_MUSHROOM_BLOCK->blockId()), 20);
}
