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

#include "world/block/blocks/CauldronBlock.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "world/block/blocks/LayeredCauldronBlock.hpp"
#include <gtest/gtest.h>

#include <map>
#include <memory>

using namespace mc;
using namespace mc::blocks;
using namespace mc::world::biome;

// ============================================================================
// 测试用世界桩 - 用于 handlePrecipitation / receiveStalactiteDrip 测试
// ============================================================================

/**
 * @brief CauldronBlock 测试用的世界桩
 *
 * 继承 BaseTestWorld，提供可控的方块状态存储、天气控制和随机数控制。
 */
class CauldronTestWorld : public mc::test::BaseTestWorld {
public:
    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

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

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<CauldronTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    void setRaining(bool raining) { m_isRaining = raining; }
    void setThundering(bool thundering) { m_isThundering = thundering; }

    [[nodiscard]] bool isRaining() const override { return m_isRaining; }
    [[nodiscard]] bool isThundering() const override { return m_isThundering; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    /// 设置随机数种子以控制 nextFloat() 结果
    void setRandomSeed(u64 seed) { m_random.setSeed(seed); }

    /// 检查指定位置是否为水炼药锅
    [[nodiscard]] bool isWaterCauldron(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return false;
        }
        return it->second->is(block_registry::BuildingBlocks::WATER_CAULDRON);
    }

    /// 检查指定位置是否为岩浆炼药锅
    [[nodiscard]] bool isLavaCauldron(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return false;
        }
        return it->second->is(block_registry::BuildingBlocks::LAVA_CAULDRON);
    }

    /// 检查指定位置是否为细雪炼药锅
    [[nodiscard]] bool isPowderSnowCauldron(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return false;
        }
        return it->second->is(block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON);
    }

    /// 检查指定位置是否为空炼药锅
    [[nodiscard]] bool isEmptyCauldron(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return false;
        }
        return it->second->is(block_registry::BuildingBlocks::CAULDRON);
    }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    bool m_isRaining = false;
    bool m_isThundering = false;
};

// ============================================================================
// CauldronBlock 基础测试
// ============================================================================

class CauldronBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建空炼药锅方块
        cauldron_ = std::make_unique<CauldronBlock>(BlockProperties(Material::ROCK).hardness(2.0f).resistance(2.0f));
    }

    std::unique_ptr<CauldronBlock> cauldron_;
};

TEST_F(CauldronBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(cauldron_, nullptr);
}

TEST_F(CauldronBlockTest, IsEmpty_AlwaysReturnsTrue)
{
    // 空炼药锅始终为空
    const auto& state = cauldron_->defaultState();
    EXPECT_TRUE(CauldronBlock::isEmpty(state));
}

TEST_F(CauldronBlockTest, IsFull_AlwaysReturnsFalse)
{
    // 空炼药锅永远不满
    const auto& state = cauldron_->defaultState();
    EXPECT_FALSE(CauldronBlock::isFull(state));
}

TEST_F(CauldronBlockTest, HasNoLevelProperty)
{
    // 空炼药锅没有水位属性，defaultState 没有 LEVEL 属性
    const auto& state = cauldron_->defaultState();
    // 尝试获取不存在的 LEVEL 属性不应崩溃
    // 由于 CauldronBlock 不注册 LEVEL_0_3 或 LEVEL_1_3，状态只有默认值
    EXPECT_TRUE(CauldronBlock::isEmpty(state));
    EXPECT_FALSE(CauldronBlock::isFull(state));
}

TEST_F(CauldronBlockTest, TicksRandomly_ReturnsFalse)
{
    // 空炼药锅不使用 randomTick
    EXPECT_FALSE(cauldron_->ticksRandomly());
}

TEST_F(CauldronBlockTest, GetShape_ReturnsValidShape)
{
    const auto& state = cauldron_->defaultState();
    const auto& shape = cauldron_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(CauldronBlockTest, GetCollisionShape_ReturnsValidShape)
{
    const auto& state = cauldron_->defaultState();
    const auto& shape = cauldron_->getCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(CauldronBlockTest, GetEntityInsideCollisionShape_ReturnsFullBlock)
{
    // 空炼药锅返回完整方块形状
    const auto& state = cauldron_->defaultState();
    const auto& shape = cauldron_->getEntityInsideCollisionShape(state);
    EXPECT_TRUE(shape.isFullBlock()) << "Empty cauldron should return full block shape";
}

// ============================================================================
// Block::getEntityInsideCollisionShape 默认行为测试
// ============================================================================

class BlockEntityInsideCollisionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        // 使用石头的 BlockState 来测试默认的 getEntityInsideCollisionShape
        block_ = VanillaBlocks::STONE;
    }

    const Block* block_ = nullptr;
};

TEST_F(BlockEntityInsideCollisionTest, DefaultReturnsFullBlock)
{
    // 默认的 getEntityInsideCollisionShape 应返回完整方块形状
    ASSERT_NE(block_, nullptr);
    const auto& state = block_->defaultState();
    const auto& shape = block_->getEntityInsideCollisionShape(state);
    EXPECT_TRUE(shape.isFullBlock()) << "Default getEntityInsideCollisionShape should return full block shape";
}

// ============================================================================
// CauldronBlock handlePrecipitation 测试夹具
// ============================================================================

class CauldronPrecipTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        fluid::FluidTags::initialize();

        // 使用注册的空炼药锅方块，确保 is(CAULDRON) 等检查能正常工作
        cauldron_ = dynamic_cast<CauldronBlock*>(block_registry::BuildingBlocks::CAULDRON);
        ASSERT_NE(cauldron_, nullptr);
        world_.setRaining(true);
    }

    /// 在指定位置放置空炼药锅（使用注册的方块实例）
    void placeEmptyCauldron(i32 x, i32 y, i32 z)
    {
        const BlockState* state = &cauldron_->defaultState();
        world_.setBlockAt(BlockPos(x, y, z), state);
    }

    CauldronBlock* cauldron_ = nullptr;
    CauldronTestWorld world_;
};

// ============================================================================
// 降水类型 - None（无降水）
// ============================================================================

TEST_F(CauldronPrecipTest, HandlePrecipitation_NoneType_NoChange)
{
    placeEmptyCauldron(0, 64, 0);
    const BlockState* stateBefore = world_.getBlockState(0, 64, 0);
    ASSERT_NE(stateBefore, nullptr);

    // 无降水类型不会改变方块
    cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::None);

    // 空炼药锅不应改变
    EXPECT_TRUE(world_.isEmptyCauldron(BlockPos(0, 64, 0)));
    EXPECT_FALSE(world_.isWaterCauldron(BlockPos(0, 64, 0)));
}

// ============================================================================
// 降水类型 - Rain（雨天）
// ============================================================================

TEST_F(CauldronPrecipTest, HandlePrecipitation_Rain_CreatesWaterCauldron)
{
    // 空炼药锅，雨天，5% 概率替换为水炼药锅（水位1）
    placeEmptyCauldron(0, 64, 0);

    // 反复尝试直到触发（概率5%）
    bool created = false;
    for (int i = 0; i < 200; ++i) {
        // 重置为空炼药锅
        placeEmptyCauldron(0, 64, 0);
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
        if (world_.isWaterCauldron(BlockPos(0, 64, 0))) {
            created = true;
            break;
        }
    }

    EXPECT_TRUE(created) << "Rain precipitation should eventually create water cauldron";
}

TEST_F(CauldronPrecipTest, HandlePrecipitation_Rain_WaterCauldronAtLevel1)
{
    // 空炼药锅被雨天后应创建水位1的水炼药锅
    VanillaBlocks::initialize();

    placeEmptyCauldron(0, 64, 0);

    // 反复尝试直到触发
    for (int i = 0; i < 500; ++i) {
        placeEmptyCauldron(0, 64, 0);
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
        if (world_.isWaterCauldron(BlockPos(0, 64, 0))) {
            // 验证水位为1
            const BlockState* state = world_.getBlockState(0, 64, 0);
            ASSERT_NE(state, nullptr);
            EXPECT_EQ(LayeredCauldronBlock::getLevel(*state), 1)
                << "Water cauldron created by rain should be at level 1";
            return;
        }
    }

    FAIL() << "Rain precipitation should eventually create water cauldron";
}

// ============================================================================
// 降水类型 - Snow（雪天）
// ============================================================================

TEST_F(CauldronPrecipTest, HandlePrecipitation_Snow_CreatesPowderSnowCauldron)
{
    // 空炼药锅，雪天，10% 概率替换为细雪炼药锅（水位1）
    placeEmptyCauldron(0, 64, 0);

    bool created = false;
    for (int i = 0; i < 100; ++i) {
        placeEmptyCauldron(0, 64, 0);
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Snow);
        if (world_.isPowderSnowCauldron(BlockPos(0, 64, 0))) {
            created = true;
            break;
        }
    }

    EXPECT_TRUE(created) << "Snow precipitation should eventually create powder snow cauldron";
}

// ============================================================================
// 概率测试 - 确保雨天5%和雪天10%的概率行为
// ============================================================================

TEST_F(CauldronPrecipTest, HandlePrecipitation_Rain_About5PercentChance)
{
    // 统计2000次雨天调用中方块被替换的次数，应该在5%左右
    i32 replaceCount = 0;
    constexpr int TOTAL_TRIALS = 2000;

    for (int i = 0; i < TOTAL_TRIALS; ++i) {
        placeEmptyCauldron(0, 64, 0);
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);

        if (world_.isWaterCauldron(BlockPos(0, 64, 0))) {
            replaceCount++;
        }
    }

    // 5% 概率，2000次约100次。允许较宽的范围：30-170（1.5% ~ 8.5%）
    EXPECT_GE(replaceCount, 30) << "Rain replace count too low";
    EXPECT_LE(replaceCount, 170) << "Rain replace count too high";
}

TEST_F(CauldronPrecipTest, HandlePrecipitation_Snow_About10PercentChance)
{
    // 统计2000次雪天调用中方块被替换的次数，应该在10%左右
    i32 replaceCount = 0;
    constexpr int TOTAL_TRIALS = 2000;

    for (int i = 0; i < TOTAL_TRIALS; ++i) {
        placeEmptyCauldron(0, 64, 0);
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Snow);

        if (world_.isPowderSnowCauldron(BlockPos(0, 64, 0))) {
            replaceCount++;
        }
    }

    // 10% 概率，2000次约200次。允许较宽的范围：120-280（6% ~ 14%）
    EXPECT_GE(replaceCount, 120) << "Snow replace count too low";
    EXPECT_LE(replaceCount, 280) << "Snow replace count too high";
}

// ============================================================================
// 边界条件测试
// ============================================================================

TEST_F(CauldronPrecipTest, HandlePrecipitation_NullBlockState_NoCrash)
{
    // 没有放置炼药锅的位置，getBlockState 返回 nullptr
    // handlePrecipitation 不应崩溃
    cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
    SUCCEED();
}

// ============================================================================
// CauldronBlock 滴石滴水测试
// ============================================================================

class CauldronDripTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        fluid::Fluids::initialize();
        fluid::FluidTags::initialize();
    }
};

TEST_F(CauldronDripTest, CanReceiveStalactiteDrip_Water_ReturnsTrue)
{
    // 空炼药锅可以接收水滴水
    EXPECT_TRUE(CauldronBlock::canReceiveStalactiteDrip(*fluid::Fluids::WATER()));
}

TEST_F(CauldronDripTest, CanReceiveStalactiteDrip_Lava_ReturnsTrue)
{
    // 空炼药锅可以接收岩浆滴水
    EXPECT_TRUE(CauldronBlock::canReceiveStalactiteDrip(*fluid::Fluids::LAVA()));
}

// ============================================================================
// CauldronBlock::receiveStalactiteDrip 集成测试
// ============================================================================

class CauldronReceiveDripTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        fluid::Fluids::initialize();
        fluid::FluidTags::initialize();

        // 使用注册的空炼药锅方块，确保 is(CAULDRON) 等检查能正常工作
        cauldron_ = dynamic_cast<CauldronBlock*>(block_registry::BuildingBlocks::CAULDRON);
        ASSERT_NE(cauldron_, nullptr);
    }

    CauldronBlock* cauldron_ = nullptr;
    CauldronTestWorld world_;
};

TEST_F(CauldronReceiveDripTest, WaterDrip_CreatesWaterCauldronAtLevel1)
{
    // 空炼药锅 + 水滴水 → 水炼药锅（水位1）
    const BlockPos pos(0, 64, 0);
    const auto& state = cauldron_->defaultState();
    world_.setBlockAt(pos, &state);
    ASSERT_TRUE(world_.isEmptyCauldron(pos));

    CauldronBlock::receiveStalactiteDrip(
        world_, pos, *world_.getBlockState(pos.x, pos.y, pos.z), *fluid::Fluids::WATER());

    // 应该替换为水炼药锅，水位1
    EXPECT_TRUE(world_.isWaterCauldron(pos));
    const BlockState* newState = world_.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(LayeredCauldronBlock::getLevel(*newState), 1) << "Water drip should create water cauldron at level 1";
}

TEST_F(CauldronReceiveDripTest, LavaDrip_CreatesLavaCauldron)
{
    // 空炼药锅 + 岩浆滴水 → 岩浆炼药锅
    const BlockPos pos(0, 64, 0);
    const auto& state = cauldron_->defaultState();
    world_.setBlockAt(pos, &state);
    ASSERT_TRUE(world_.isEmptyCauldron(pos));

    CauldronBlock::receiveStalactiteDrip(
        world_, pos, *world_.getBlockState(pos.x, pos.y, pos.z), *fluid::Fluids::LAVA());

    // 应该替换为岩浆炼药锅
    EXPECT_TRUE(world_.isLavaCauldron(pos));
    EXPECT_FALSE(world_.isWaterCauldron(pos));
    EXPECT_FALSE(world_.isEmptyCauldron(pos));
}

// ============================================================================
// CauldronBlock 注册测试
// ============================================================================

class CauldronBlockRegistryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

TEST_F(CauldronBlockRegistryTest, CauldronIsRegistered)
{
    EXPECT_NE(block_registry::BuildingBlocks::CAULDRON, nullptr);
}

TEST_F(CauldronBlockRegistryTest, CauldronBlockType)
{
    auto* cauldron = dynamic_cast<CauldronBlock*>(block_registry::BuildingBlocks::CAULDRON);
    EXPECT_NE(cauldron, nullptr);
}

TEST_F(CauldronBlockRegistryTest, CauldronIsInCauldronsTag)
{
    // 空炼药锅应属于 #minecraft:cauldrons 标签
    const BlockState* state = &block_registry::BuildingBlocks::CAULDRON->defaultState();
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(BlockTags::CAULDRONS().contains(*state));
}

TEST_F(CauldronBlockRegistryTest, CauldronDefaultStateIsEmpty)
{
    // 注册的空炼药锅默认状态始终为空
    const auto& state = block_registry::BuildingBlocks::CAULDRON->defaultState();
    EXPECT_TRUE(CauldronBlock::isEmpty(state));
    EXPECT_FALSE(CauldronBlock::isFull(state));
}

TEST_F(CauldronBlockRegistryTest, WaterCauldronIsRegistered)
{
    EXPECT_NE(block_registry::BuildingBlocks::WATER_CAULDRON, nullptr);
}

TEST_F(CauldronBlockRegistryTest, WaterCauldronBlockType)
{
    auto* waterCauldron = dynamic_cast<LayeredCauldronBlock*>(block_registry::BuildingBlocks::WATER_CAULDRON);
    EXPECT_NE(waterCauldron, nullptr);
}

TEST_F(CauldronBlockRegistryTest, WaterCauldronIsInCauldronsTag)
{
    // 水炼药锅应属于 #minecraft:cauldrons 标签
    const BlockState* state = &block_registry::BuildingBlocks::WATER_CAULDRON->defaultState();
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(BlockTags::CAULDRONS().contains(*state));
}

// ============================================================================
// 细雪炼药锅 (PowderSnowCauldron) 注册与基础测试
// ============================================================================

TEST_F(CauldronBlockRegistryTest, PowderSnowCauldronIsRegistered)
{
    EXPECT_NE(block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON, nullptr);
}

TEST_F(CauldronBlockRegistryTest, PowderSnowCauldronBlockType)
{
    // 细雪炼药锅应为 LayeredCauldronBlock 实例
    auto* powderSnowCauldron =
        dynamic_cast<LayeredCauldronBlock*>(block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON);
    EXPECT_NE(powderSnowCauldron, nullptr);
}

TEST_F(CauldronBlockRegistryTest, PowderSnowCauldronIsInCauldronsTag)
{
    // 细雪炼药锅应属于 #minecraft:cauldrons 标签
    const BlockState* state = &block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON->defaultState();
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(BlockTags::CAULDRONS().contains(*state));
}

TEST_F(CauldronBlockRegistryTest, PowderSnowCauldronDefaultState)
{
    // 细雪炼药锅默认水位为1
    const auto& state = block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON->defaultState();
    EXPECT_EQ(LayeredCauldronBlock::getLevel(state), 1);
    EXPECT_FALSE(LayeredCauldronBlock::isFull(state));
}

TEST_F(CauldronBlockRegistryTest, PowderSnowCauldronLevelProperty)
{
    // 细雪炼药锅应支持 LEVEL_1_3 属性
    const auto& state1 = block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON->defaultState();
    EXPECT_EQ(LayeredCauldronBlock::getLevel(state1), 1);

    const BlockState* state3 = &state1.with(BlockStateProperties::LEVEL_1_3(), 3);
    ASSERT_NE(state3, nullptr);
    EXPECT_EQ(LayeredCauldronBlock::getLevel(*state3), 3);
    EXPECT_TRUE(LayeredCauldronBlock::isFull(*state3));
}
