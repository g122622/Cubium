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
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/blocks/agricultural/StemBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <map>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 测试用 IBlockReader，支持方块存取、光照设置和基本世界接口
 *
 * 继承自 IBlockReader（即 IWorld 的空派生类），提供用于 StemBlock 生长逻辑
 * 测试所需的最小世界模拟：方块读写、光照查询、种子控制。
 */
class StemGrowthTestWorld final : public IBlockReader {
public:
    using IBlockReader::getBlockState;

    // ========== 方块存取 ==========

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

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    // ========== 光照 ==========

    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blockLight.find(pos);
        return it != m_blockLight.end() ? it->second : 0;
    }

    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_skyLight.find(pos);
        return it != m_skyLight.end() ? it->second : 15;
    }

    void setBlockLightAt(const BlockPos& pos, u8 light) { m_blockLight[pos] = light; }
    void setSkyLightAt(const BlockPos& pos, u8 light) { m_skyLight[pos] = light; }

    // ========== 种子控制 ==========

    void setSeed(u64 seed) { m_seed = seed; }
    [[nodiscard]] u64 seed() const override { return m_seed; }

    // ========== 桩实现 ==========

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
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
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 6000; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("StemGrowthTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("StemGrowthTestWorld::tickManager not implemented");
    }

    [[nodiscard]] math::Random& getRandom() override
    {
        throw std::runtime_error("StemGrowthTestWorld::getRandom not implemented");
    }
    [[nodiscard]] const math::Random& getRandom() const override
    {
        throw std::runtime_error("StemGrowthTestWorld::getRandom not implemented");
    }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, u8> m_blockLight;
    std::map<BlockPos, u8> m_skyLight;
    u64 m_seed = 12345;
    world::border::WorldBorder m_worldBorder;
};

} // namespace

// ============================================================================
// CropBlock::getGrowthChance 公共接口测试
// ============================================================================

class StemGrowthLogicTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(StemGrowthLogicTest, GetGrowthChanceIsPublicAndCallable)
{
    // 验证 CropBlock::getGrowthChance 已从 protected 改为 public，
    // StemBlock 等兄弟类可以调用它
    if (VanillaBlocks::MELON_STEM == nullptr) {
        GTEST_SKIP() << "MELON_STEM not registered";
    }

    auto* melonStem = dynamic_cast<const StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr) << "MELON_STEM should be a StemBlock";

    StemGrowthTestWorld world;

    // 在(5, 64, 5)放置茎，在(5, 63, 5)放置耕地
    const BlockState* farmlandState = &VanillaBlocks::FARMLAND->defaultState();
    world.setBlockAt(BlockPos(5, 63, 5), farmlandState);

    // 调用 CropBlock::getGrowthChance（public static 方法）
    f32 growthChance = CropBlock::getGrowthChance(*melonStem, static_cast<IBlockReader&>(world), BlockPos(5, 64, 5));

    // 基础值 1.0 + 中心湿润耕地 1.0 = 2.0（默认未设置 MOISTURE 属性时 moisture=0，所以是干燥耕地 +1.0）
    EXPECT_GE(growthChance, 1.0f) << "Growth chance should be at least 1.0";
    EXPECT_LE(growthChance, 10.0f) << "Growth chance should not exceed 10.0 (max possible with all wet farmland)";
}

TEST_F(StemGrowthLogicTest, GetGrowthChanceWithWetFarmlandIsHigher)
{
    // 湿润耕地应比干燥耕地有更高的生长概率
    if (VanillaBlocks::MELON_STEM == nullptr || VanillaBlocks::FARMLAND == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    auto* melonStem = dynamic_cast<const StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr);

    StemGrowthTestWorld dryWorld;
    StemGrowthTestWorld wetWorld;

    // 干燥耕地（moisture=0）
    const BlockState* dryFarmland = &VanillaBlocks::FARMLAND->defaultState();
    dryWorld.setBlockAt(BlockPos(5, 63, 5), dryFarmland);

    // 湿润耕地（moisture=7）
    const BlockState* wetFarmland =
        &VanillaBlocks::FARMLAND->defaultState().with(BlockStateProperties::MOISTURE_0_7(), 7);
    wetWorld.setBlockAt(BlockPos(5, 63, 5), wetFarmland);

    f32 dryChance = CropBlock::getGrowthChance(*melonStem, static_cast<IBlockReader&>(dryWorld), BlockPos(5, 64, 5));
    f32 wetChance = CropBlock::getGrowthChance(*melonStem, static_cast<IBlockReader&>(wetWorld), BlockPos(5, 64, 5));

    EXPECT_GT(wetChance, dryChance) << "Wet farmland should give higher growth chance than dry farmland";
}

// ============================================================================
// BlockTags::DIRT 标签覆盖测试（tryGrowFruit 果实支撑判定）
// ============================================================================

class StemFruitSupportTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(StemFruitSupportTest, FarmlandSupportsFruitGrowth)
{
    // FARMLAND 在 DIRT 标签中，应支撑果实
    if (VanillaBlocks::FARMLAND == nullptr) {
        GTEST_SKIP() << "FARMLAND not registered";
    }
    EXPECT_TRUE(BlockTags::DIRT().contains(VanillaBlocks::FARMLAND->defaultState()));
}

TEST_F(StemFruitSupportTest, DirtSupportsFruitGrowth)
{
    if (VanillaBlocks::DIRT == nullptr) {
        GTEST_SKIP() << "DIRT not registered";
    }
    EXPECT_TRUE(BlockTags::DIRT().contains(VanillaBlocks::DIRT->defaultState()));
}

TEST_F(StemFruitSupportTest, GrassBlockSupportsFruitGrowth)
{
    if (VanillaBlocks::GRASS_BLOCK == nullptr) {
        GTEST_SKIP() << "GRASS_BLOCK not registered";
    }
    EXPECT_TRUE(BlockTags::DIRT().contains(VanillaBlocks::GRASS_BLOCK->defaultState()));
}

TEST_F(StemFruitSupportTest, PodzolSupportsFruitGrowth)
{
    // Podzol 在 DIRT 标签中，原版允许瓜果在 Podzol 上生长
    if (VanillaBlocks::PODZOL == nullptr) {
        GTEST_SKIP() << "PODZOL not registered";
    }
    EXPECT_TRUE(BlockTags::DIRT().contains(VanillaBlocks::PODZOL->defaultState()));
}

TEST_F(StemFruitSupportTest, CoarseDirtSupportsFruitGrowth)
{
    if (VanillaBlocks::COARSE_DIRT == nullptr) {
        GTEST_SKIP() << "COARSE_DIRT not registered";
    }
    EXPECT_TRUE(BlockTags::DIRT().contains(VanillaBlocks::COARSE_DIRT->defaultState()));
}

TEST_F(StemFruitSupportTest, MyceliumSupportsFruitGrowth)
{
    if (VanillaBlocks::MYCELIUM == nullptr) {
        GTEST_SKIP() << "MYCELIUM not registered";
    }
    EXPECT_TRUE(BlockTags::DIRT().contains(VanillaBlocks::MYCELIUM->defaultState()));
}

TEST_F(StemFruitSupportTest, RootedDirtSupportsFruitGrowth)
{
    if (VanillaBlocks::ROOTED_DIRT == nullptr) {
        GTEST_SKIP() << "ROOTED_DIRT not registered";
    }
    EXPECT_TRUE(BlockTags::DIRT().contains(VanillaBlocks::ROOTED_DIRT->defaultState()));
}

TEST_F(StemFruitSupportTest, MossBlockSupportsFruitGrowth)
{
    if (VanillaBlocks::MOSS_BLOCK == nullptr) {
        GTEST_SKIP() << "MOSS_BLOCK not registered";
    }
    EXPECT_TRUE(BlockTags::DIRT().contains(VanillaBlocks::MOSS_BLOCK->defaultState()));
}

TEST_F(StemFruitSupportTest, MudSupportsFruitGrowth)
{
    if (VanillaBlocks::MUD == nullptr) {
        GTEST_SKIP() << "MUD not registered";
    }
    EXPECT_TRUE(BlockTags::DIRT().contains(VanillaBlocks::MUD->defaultState()));
}

TEST_F(StemFruitSupportTest, StoneDoesNotSupportFruitGrowth)
{
    // STONE 不在 DIRT 标签中，不应支撑瓜果
    if (VanillaBlocks::STONE == nullptr) {
        GTEST_SKIP() << "STONE not registered";
    }
    EXPECT_FALSE(BlockTags::DIRT().contains(VanillaBlocks::STONE->defaultState()));
}

TEST_F(StemFruitSupportTest, SandDoesNotSupportFruitGrowth)
{
    // SAND 不在 DIRT 标签中
    if (VanillaBlocks::SAND == nullptr) {
        GTEST_SKIP() << "SAND not registered";
    }
    EXPECT_FALSE(BlockTags::DIRT().contains(VanillaBlocks::SAND->defaultState()));
}

// ============================================================================
// StemBlock::canGrow 测试（骨粉只在未成熟时有效）
// ============================================================================

TEST_F(StemGrowthLogicTest, CanGrowReturnsTrueForImmatureStem)
{
    // 未成熟的茎可以使用骨粉
    if (VanillaBlocks::MELON_STEM == nullptr) {
        GTEST_SKIP() << "MELON_STEM not registered";
    }

    auto* melonStem = dynamic_cast<const StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr);

    StemGrowthTestWorld world;
    const BlockState& age0 = melonStem->defaultState().with(BlockStateProperties::AGE_0_7(), 0);
    EXPECT_TRUE(melonStem->canGrow(world, BlockPos(0, 0, 0), age0, false));
}

TEST_F(StemGrowthLogicTest, CanGrowReturnsFalseForMatureStem)
{
    // 成熟的茎（AGE=7）不能使用骨粉（原版：isValidBonemealTarget 返回 AGE != 7）
    if (VanillaBlocks::MELON_STEM == nullptr) {
        GTEST_SKIP() << "MELON_STEM not registered";
    }

    auto* melonStem = dynamic_cast<const StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr);

    StemGrowthTestWorld world;
    const BlockState& age7 = melonStem->defaultState().with(BlockStateProperties::AGE_0_7(), 7);
    EXPECT_FALSE(melonStem->canGrow(world, BlockPos(0, 0, 0), age7, false));
}

// ============================================================================
// StemBlock 年龄属性测试
// ============================================================================

TEST_F(StemGrowthLogicTest, StemBlockMaxAgeIsSeven)
{
    if (VanillaBlocks::MELON_STEM == nullptr) {
        GTEST_SKIP() << "MELON_STEM not registered";
    }

    auto* melonStem = dynamic_cast<const StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr);
    EXPECT_EQ(melonStem->getMaxAge(), 7) << "StemBlock max age should be 7";
}

TEST_F(StemGrowthLogicTest, StemBlockAgeProgression)
{
    if (VanillaBlocks::MELON_STEM == nullptr) {
        GTEST_SKIP() << "MELON_STEM not registered";
    }

    auto* melonStem = dynamic_cast<const StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr);

    // 验证各年龄阶段的 withAge/getAge 一致性
    for (i32 age = 0; age <= 7; ++age) {
        const BlockState& state = melonStem->withAge(age);
        EXPECT_EQ(melonStem->getAge(state), age) << "Age mismatch at age " << age;
    }

    // 超过最大年龄的 withAge 应该被截断到 7
    const BlockState& clamped = melonStem->withAge(10);
    EXPECT_EQ(melonStem->getAge(clamped), 7) << "Age should be clamped to max 7";
}

TEST_F(StemGrowthLogicTest, IsMaxAgeWorksCorrectly)
{
    if (VanillaBlocks::MELON_STEM == nullptr) {
        GTEST_SKIP() << "MELON_STEM not registered";
    }

    auto* melonStem = dynamic_cast<const StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr);

    EXPECT_FALSE(melonStem->isMaxAge(melonStem->withAge(0)));
    EXPECT_FALSE(melonStem->isMaxAge(melonStem->withAge(3)));
    EXPECT_FALSE(melonStem->isMaxAge(melonStem->withAge(6)));
    EXPECT_TRUE(melonStem->isMaxAge(melonStem->withAge(7)));
}
