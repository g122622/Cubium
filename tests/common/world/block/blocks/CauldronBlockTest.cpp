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
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BannerItem.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/interactive/BannerEntity.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "util/property/Properties.hpp"
#include <gtest/gtest.h>

#include <map>
#include <memory>

using namespace mc;
using namespace mc::blocks;
using namespace mc::world::biome;

// ============================================================================
// 测试用世界桩 - 用于 handlePrecipitation 测试
// ============================================================================

/**
 * @brief CauldronBlock handlePrecipitation 测试用的世界桩
 *
 * 继承 BaseTestWorld，提供可控的方块状态存储、天气控制和随机数控制。
 */
class CauldronPrecipTestWorld : public test::BaseTestWorld {
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
        const_cast<CauldronPrecipTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    void setRaining(bool raining) { m_isRaining = raining; }
    void setThundering(bool thundering) { m_isThundering = thundering; }

    [[nodiscard]] bool isRaining() const override { return m_isRaining; }
    [[nodiscard]] bool isThundering() const override { return m_isThundering; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    /// 设置随机数种子以控制 nextFloat() 结果
    void setRandomSeed(u64 seed) { m_random.setSeed(seed); }

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
        // 创建炼药锅方块
        cauldron_ = std::make_unique<CauldronBlock>(BlockProperties(Material::ROCK).hardness(2.0f).resistance(2.0f));
    }

    std::unique_ptr<CauldronBlock> cauldron_;
};

TEST_F(CauldronBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(cauldron_, nullptr);
}

TEST_F(CauldronBlockTest, GetLevel_ReturnsZeroByDefault)
{
    const auto& state = cauldron_->defaultState();
    EXPECT_EQ(CauldronBlock::getLevel(state), 0);
}

TEST_F(CauldronBlockTest, IsEmpty_ReturnsTrueForDefault)
{
    const auto& state = cauldron_->defaultState();
    EXPECT_TRUE(CauldronBlock::isEmpty(state));
}

TEST_F(CauldronBlockTest, IsFull_ReturnsFalseForDefault)
{
    const auto& state = cauldron_->defaultState();
    EXPECT_FALSE(CauldronBlock::isFull(state));
}

TEST_F(CauldronBlockTest, HasComparatorInputOverride_ReturnsTrue)
{
    const auto& state = cauldron_->defaultState();
    EXPECT_TRUE(cauldron_->hasComparatorInputOverride(state));
}

TEST_F(CauldronBlockTest, TicksRandomly_ReturnsFalse)
{
    // 炼药锅不再使用 randomTick 进行雨填充，
    // 而是通过 handlePrecipitation 在 tickPrecipitation 中处理降水
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

TEST_F(CauldronBlockTest, GetContentShape_ReturnsValidShapeForAllLevels)
{
    for (i32 level = 0; level <= 3; ++level) {
        const auto& shape = cauldron_->getContentShape(level);
        // 内容形状可以为空（水位0）
        if (level > 0) {
            EXPECT_FALSE(shape.isEmpty()) << "Level " << level << " should have content shape";
        }
    }
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

        cauldron_ = std::make_unique<CauldronBlock>(BlockProperties(Material::ROCK).hardness(2.0f).resistance(2.0f));
        world_.setRaining(true);
    }

    /// 在指定位置放置指定水位的炼药锅
    void placeCauldron(i32 x, i32 y, i32 z, i32 level)
    {
        const BlockState* state = &cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), level);
        world_.setBlockAt(BlockPos(x, y, z), state);
    }

    /// 获取指定位置的炼药锅水位
    i32 getCauldronLevel(i32 x, i32 y, i32 z) const
    {
        const BlockState* state = world_.getBlockState(x, y, z);
        if (state == nullptr) {
            return -1;
        }
        return CauldronBlock::getLevel(*state);
    }

    std::unique_ptr<CauldronBlock> cauldron_;
    CauldronPrecipTestWorld world_;
};

// ============================================================================
// 降水类型 - None（无降水）
// ============================================================================

TEST_F(CauldronPrecipTest, HandlePrecipitation_NoneType_NoChange)
{
    placeCauldron(0, 64, 0, 0);
    const BlockState* stateBefore = world_.getBlockState(0, 64, 0);
    i32 levelBefore = CauldronBlock::getLevel(*stateBefore);
    ASSERT_EQ(levelBefore, 0);

    // 无降水类型不会改变水位
    cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::None);

    EXPECT_EQ(getCauldronLevel(0, 64, 0), 0);
}

// ============================================================================
// 降水类型 - Rain（雨天）
// ============================================================================

TEST_F(CauldronPrecipTest, HandlePrecipitation_Rain_IncrementsLevelWhenNotFull)
{
    // 水位0，雨天，5%概率触发
    // 使用随机数种子让 nextFloat() 返回一个小于 0.05 的值
    placeCauldron(0, 64, 0, 0);

    // 反复尝试直到触发（概率5%，种子不同结果不同）
    // 直接使用一个确定性的种子序列
    world_.setRandomSeed(42);

    // 调用多次直到水位增加（雨天5%概率）
    bool levelChanged = false;
    for (int i = 0; i < 200; ++i) {
        i32 levelBefore = getCauldronLevel(0, 64, 0);
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
        i32 levelAfter = getCauldronLevel(0, 64, 0);
        if (levelAfter > levelBefore) {
            levelChanged = true;
            break;
        }
    }

    EXPECT_TRUE(levelChanged) << "Rain precipitation should eventually increment cauldron level";
}

TEST_F(CauldronPrecipTest, HandlePrecipitation_Rain_DoesNotExceedMaxLevel)
{
    // 水位3（满），雨天不应增加
    placeCauldron(0, 64, 0, 3);

    // 多次调用，水位不应超过3
    for (int i = 0; i < 100; ++i) {
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
    }

    EXPECT_EQ(getCauldronLevel(0, 64, 0), 3);
}

TEST_F(CauldronPrecipTest, HandlePrecipitation_Rain_LevelCapsAt3)
{
    // 水位2，多次雨天触发后水位不应超过3
    placeCauldron(0, 64, 0, 2);
    world_.setRandomSeed(12345);

    // 调用足够多次
    for (int i = 0; i < 500; ++i) {
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
    }

    EXPECT_EQ(getCauldronLevel(0, 64, 0), 3);
}

// ============================================================================
// 降水类型 - Snow（雪天）
// ============================================================================

TEST_F(CauldronPrecipTest, HandlePrecipitation_Snow_IncrementsLevelWhenNotFull)
{
    // 雪天10%概率，比雨天更容易触发
    placeCauldron(0, 64, 0, 0);
    world_.setRandomSeed(42);

    bool levelChanged = false;
    for (int i = 0; i < 100; ++i) {
        i32 levelBefore = getCauldronLevel(0, 64, 0);
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Snow);
        i32 levelAfter = getCauldronLevel(0, 64, 0);
        if (levelAfter > levelBefore) {
            levelChanged = true;
            break;
        }
    }

    EXPECT_TRUE(levelChanged) << "Snow precipitation should eventually increment cauldron level";
}

TEST_F(CauldronPrecipTest, HandlePrecipitation_Snow_DoesNotExceedMaxLevel)
{
    // 水位3（满），雪天不应增加
    placeCauldron(0, 64, 0, 3);

    for (int i = 0; i < 100; ++i) {
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Snow);
    }

    EXPECT_EQ(getCauldronLevel(0, 64, 0), 3);
}

TEST_F(CauldronPrecipTest, HandlePrecipitation_Snow_LevelCapsAt3)
{
    // 水位2，多次雪天触发后水位不应超过3
    placeCauldron(0, 64, 0, 2);
    world_.setRandomSeed(12345);

    for (int i = 0; i < 500; ++i) {
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Snow);
    }

    EXPECT_EQ(getCauldronLevel(0, 64, 0), 3);
}

// ============================================================================
// 概率测试 - 确保雨天5%和雪天10%的概率行为
// ============================================================================

TEST_F(CauldronPrecipTest, HandlePrecipitation_Rain_About5PercentChance)
{
    // 统计2000次雨天调用中水位增加的次数，应该在5%左右
    // 空炼药锅，每次增加后重置
    i32 incrementCount = 0;
    constexpr int TOTAL_TRIALS = 2000;

    for (int i = 0; i < TOTAL_TRIALS; ++i) {
        // 重置水位为0
        placeCauldron(0, 64, 0, 0);

        i32 levelBefore = getCauldronLevel(0, 64, 0);
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
        i32 levelAfter = getCauldronLevel(0, 64, 0);

        if (levelAfter > levelBefore) {
            incrementCount++;
        }
    }

    // 5% 概率，2000次约100次。允许较宽的范围：30-170（1.5% ~ 8.5%）
    EXPECT_GE(incrementCount, 30) << "Rain increment count too low";
    EXPECT_LE(incrementCount, 170) << "Rain increment count too high";
}

TEST_F(CauldronPrecipTest, HandlePrecipitation_Snow_About10PercentChance)
{
    // 统计2000次雪天调用中水位增加的次数，应该在10%左右
    i32 incrementCount = 0;
    constexpr int TOTAL_TRIALS = 2000;

    for (int i = 0; i < TOTAL_TRIALS; ++i) {
        // 重置水位为0
        placeCauldron(0, 64, 0, 0);

        i32 levelBefore = getCauldronLevel(0, 64, 0);
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Snow);
        i32 levelAfter = getCauldronLevel(0, 64, 0);

        if (levelAfter > levelBefore) {
            incrementCount++;
        }
    }

    // 10% 概率，2000次约200次。允许较宽的范围：120-280（6% ~ 14%）
    EXPECT_GE(incrementCount, 120) << "Snow increment count too low";
    EXPECT_LE(incrementCount, 280) << "Snow increment count too high";
}

// ============================================================================
// 边界条件测试
// ============================================================================

TEST_F(CauldronPrecipTest, HandlePrecipitation_NullBlockState_NoCrash)
{
    // 没有放置炼药锅的位置，getBlockState 返回 nullptr
    // handlePrecipitation 不应崩溃
    cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
    // 如果到达这里，说明没有崩溃
    SUCCEED();
}

TEST_F(CauldronPrecipTest, HandlePrecipitation_FullCauldron_NoIncrementForAnyPrecipitationType)
{
    // 满的炼药锅不应被任何降水类型增加
    placeCauldron(0, 64, 0, 3);

    world_.setRandomSeed(42);
    for (int i = 0; i < 100; ++i) {
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
    }
    EXPECT_EQ(getCauldronLevel(0, 64, 0), 3);

    for (int i = 0; i < 100; ++i) {
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Snow);
    }
    EXPECT_EQ(getCauldronLevel(0, 64, 0), 3);
}

TEST_F(CauldronPrecipTest, HandlePrecipitation_LevelTwoCanReachMax)
{
    // 水位2可以在雨/雪天达到3
    placeCauldron(0, 64, 0, 2);
    world_.setRandomSeed(42);

    bool reachedMax = false;
    for (int i = 0; i < 200; ++i) {
        cauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
        if (getCauldronLevel(0, 64, 0) == 3) {
            reachedMax = true;
            break;
        }
    }

    EXPECT_TRUE(reachedMax) << "Level 2 cauldron should be able to reach level 3 with rain";
}

// ============================================================================
// 炼药锅旗帜清洗测试（通过 BannerEntity 静态方法间接验证核心逻辑）
// ============================================================================

class CauldronBannerCleaningTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();

        cauldron_ = std::make_unique<CauldronBlock>(BlockProperties(Material::ROCK).hardness(2.0f).resistance(2.0f));
    }

    std::unique_ptr<CauldronBlock> cauldron_;
};

TEST_F(CauldronBannerCleaningTest, BannerItemIsRecognizedAsBanner)
{
    // 验证旗帜物品可以正确通过 dynamic_cast 识别
    if (Items::WHITE_BANNER == nullptr) {
        GTEST_SKIP() << "WHITE_BANNER not registered";
    }
    const auto* bannerItem = dynamic_cast<const item::BannerItem*>(Items::WHITE_BANNER);
    EXPECT_NE(bannerItem, nullptr) << "WHITE_BANNER should be a BannerItem";
}

TEST_F(CauldronBannerCleaningTest, ShieldIsNotBannerButSeparateCheck)
{
    // 验证盾牌不是 BannerItem，需要单独检查
    if (Items::SHIELD == nullptr) {
        GTEST_SKIP() << "SHIELD not registered";
    }
    const auto* bannerItem = dynamic_cast<const item::BannerItem*>(Items::SHIELD);
    EXPECT_EQ(bannerItem, nullptr) << "SHIELD should NOT be a BannerItem";

    // 但盾牌物品应该可以通过 Items::SHIELD 比较
    EXPECT_NE(Items::SHIELD, nullptr);
}

TEST_F(CauldronBannerCleaningTest, BannerWithNoPatternsReturnsPass)
{
    // 没有图案的旗帜不应该被清洗（BannerEntity::getPatternCount 返回 0）
    if (Items::WHITE_BANNER == nullptr) {
        GTEST_SKIP() << "WHITE_BANNER not registered";
    }
    ItemStack bannerStack(*Items::WHITE_BANNER, 1);
    EXPECT_EQ(blockentity::BannerEntity::getPatternCount(bannerStack), 0);
}

TEST_F(CauldronBannerCleaningTest, BannerWithPatternsReturnsCorrectCount)
{
    if (Items::WHITE_BANNER == nullptr) {
        GTEST_SKIP() << "WHITE_BANNER not registered";
    }
    ItemStack bannerStack(*Items::WHITE_BANNER, 1);

    // 添加2个图案
    nlohmann::json& blockEntityTag = bannerStack.getOrCreateChildTag("BlockEntityTag");
    nlohmann::json patterns = nlohmann::json::array();
    patterns.push_back({{"Pattern", "bs"}, {"Color", 14}});
    patterns.push_back({{"Pattern", "cr"}, {"Color", 11}});
    blockEntityTag["Patterns"] = patterns;

    EXPECT_EQ(blockentity::BannerEntity::getPatternCount(bannerStack), 2);
}

TEST_F(CauldronBannerCleaningTest, RemoveBannerDataFromBanner_DecreasesPatternCount)
{
    if (Items::WHITE_BANNER == nullptr) {
        GTEST_SKIP() << "WHITE_BANNER not registered";
    }
    ItemStack bannerStack(*Items::WHITE_BANNER, 1);

    // 添加3个图案
    nlohmann::json& blockEntityTag = bannerStack.getOrCreateChildTag("BlockEntityTag");
    nlohmann::json patterns = nlohmann::json::array();
    patterns.push_back({{"Pattern", "bs"}, {"Color", 14}});
    patterns.push_back({{"Pattern", "cr"}, {"Color", 11}});
    patterns.push_back({{"Pattern", "mc"}, {"Color", 0}});
    blockEntityTag["Patterns"] = patterns;

    ASSERT_EQ(blockentity::BannerEntity::getPatternCount(bannerStack), 3);

    // 清洗一次：移除最顶层图案
    blockentity::BannerEntity::removeBannerData(bannerStack);
    EXPECT_EQ(blockentity::BannerEntity::getPatternCount(bannerStack), 2);

    // 再清洗一次
    blockentity::BannerEntity::removeBannerData(bannerStack);
    EXPECT_EQ(blockentity::BannerEntity::getPatternCount(bannerStack), 1);

    // 再清洗一次：移除最后一个图案
    blockentity::BannerEntity::removeBannerData(bannerStack);
    EXPECT_EQ(blockentity::BannerEntity::getPatternCount(bannerStack), 0);
}

TEST_F(CauldronBannerCleaningTest, RemoveBannerDataFromShield_WorksSameAsBanner)
{
    // 盾牌使用与旗帜相同的 BlockEntityTag.Patterns 结构
    if (Items::SHIELD == nullptr) {
        GTEST_SKIP() << "SHIELD not registered";
    }
    ItemStack shieldStack(*Items::SHIELD, 1);

    // 给盾牌添加图案
    nlohmann::json& blockEntityTag = shieldStack.getOrCreateChildTag("BlockEntityTag");
    nlohmann::json patterns = nlohmann::json::array();
    patterns.push_back({{"Pattern", "bs"}, {"Color", 14}});
    blockEntityTag["Patterns"] = patterns;

    ASSERT_EQ(blockentity::BannerEntity::getPatternCount(shieldStack), 1);

    // 清洗盾牌
    blockentity::BannerEntity::removeBannerData(shieldStack);
    EXPECT_EQ(blockentity::BannerEntity::getPatternCount(shieldStack), 0);
}

TEST_F(CauldronBannerCleaningTest, NonBannerNonShieldItem_HasZeroPatterns)
{
    // 普通物品（如水桶）没有图案
    if (Items::BUCKET == nullptr) {
        GTEST_SKIP() << "BUCKET not registered";
    }
    ItemStack bucketStack(*Items::BUCKET, 1);
    EXPECT_EQ(blockentity::BannerEntity::getPatternCount(bucketStack), 0);
}

TEST_F(CauldronBannerCleaningTest, EmptyStack_HasZeroPatterns)
{
    ItemStack emptyStack;
    EXPECT_EQ(blockentity::BannerEntity::getPatternCount(emptyStack), 0);
    // 清洗空物品不应崩溃
    blockentity::BannerEntity::removeBannerData(emptyStack);
    SUCCEED();
}

TEST_F(CauldronBannerCleaningTest, CauldronLevelConstants)
{
    // 验证炼药锅水位范围（0-3）
    const auto& state0 = cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), 0);
    const auto& state1 = cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), 1);
    const auto& state2 = cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), 2);
    const auto& state3 = cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), 3);

    EXPECT_EQ(CauldronBlock::getLevel(state0), 0);
    EXPECT_EQ(CauldronBlock::getLevel(state1), 1);
    EXPECT_EQ(CauldronBlock::getLevel(state2), 2);
    EXPECT_EQ(CauldronBlock::getLevel(state3), 3);

    EXPECT_TRUE(CauldronBlock::isEmpty(state0));
    EXPECT_FALSE(CauldronBlock::isEmpty(state1));
    EXPECT_TRUE(CauldronBlock::isFull(state3));
    EXPECT_FALSE(CauldronBlock::isFull(state2));
}
