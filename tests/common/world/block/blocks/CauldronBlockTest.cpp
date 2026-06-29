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
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
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

TEST_F(CauldronBlockTest, GetEntityInsideCollisionShape_EmptyCauldron_ReturnsFullBlock)
{
    // 空炼药锅（水位0）返回完整方块形状，与 MC 原版一致
    const auto& state = cauldron_->defaultState();
    ASSERT_EQ(CauldronBlock::getLevel(state), 0);
    const auto& shape = cauldron_->getEntityInsideCollisionShape(state);
    EXPECT_TRUE(shape.isFullBlock()) << "Empty cauldron should return full block shape";
}

TEST_F(CauldronBlockTest, GetEntityInsideCollisionShape_Level1_ReturnsFilledShape)
{
    const auto& state = cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), 1);
    const auto& shape = cauldron_->getEntityInsideCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty()) << "Level 1 should have entity inside collision shape";
    EXPECT_FALSE(shape.isFullBlock()) << "Level 1 should not be full block (smaller fill area)";
}

TEST_F(CauldronBlockTest, GetEntityInsideCollisionShape_Level2_ReturnsFilledShape)
{
    const auto& state = cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), 2);
    const auto& shape = cauldron_->getEntityInsideCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty()) << "Level 2 should have entity inside collision shape";
    EXPECT_FALSE(shape.isFullBlock()) << "Level 2 should not be full block";
}

TEST_F(CauldronBlockTest, GetEntityInsideCollisionShape_Level3_ReturnsFilledShape)
{
    const auto& state = cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), 3);
    const auto& shape = cauldron_->getEntityInsideCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty()) << "Level 3 should have entity inside collision shape";
    EXPECT_FALSE(shape.isFullBlock()) << "Level 3 should not be full block";
}

TEST_F(CauldronBlockTest, GetEntityInsideCollisionShape_FilledShapeContainsOuterShape)
{
    // 填充形状应包含外部炼药锅形状（碰撞箱不应小于外部形状）
    const auto& outerShape = cauldron_->getShape(cauldron_->defaultState());
    for (i32 level = 1; level <= 3; ++level) {
        const auto& state = cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), level);
        const auto& filledShape = cauldron_->getEntityInsideCollisionShape(state);
        // 填充形状应有更多的碰撞箱（外部形状 + 内容形状）
        EXPECT_GE(filledShape.boxCount(), outerShape.boxCount())
            << "Level " << level << " filled shape should have at least as many boxes as outer shape";
    }
}

TEST_F(CauldronBlockTest, GetContentShape_HeightsMatchMCValues)
{
    // MC 原版 LayeredCauldronBlock 内容高度：
    // 水位1: 9像素 (0.5625), 水位2: 12像素 (0.75), 水位3: 15像素 (0.9375)
    // 内容区域从 y=4/16 开始
    constexpr f32 innerMinY = 4.0f / 16.0f;
    constexpr f32 innerX1 = 2.0f / 16.0f;
    constexpr f32 innerX2 = 14.0f / 16.0f;

    // 水位1：y从 4/16 到 9/16
    const auto& shape1 = cauldron_->getContentShape(1);
    EXPECT_FALSE(shape1.isEmpty());
    const auto& boxes1 = shape1.boxes();
    ASSERT_FALSE(boxes1.empty());
    EXPECT_FLOAT_EQ(boxes1[0].minY, innerMinY);
    EXPECT_FLOAT_EQ(boxes1[0].maxY, 9.0f / 16.0f);
    EXPECT_FLOAT_EQ(boxes1[0].minX, innerX1);
    EXPECT_FLOAT_EQ(boxes1[0].maxX, innerX2);

    // 水位2：y从 4/16 到 12/16
    const auto& shape2 = cauldron_->getContentShape(2);
    const auto& boxes2 = shape2.boxes();
    ASSERT_FALSE(boxes2.empty());
    EXPECT_FLOAT_EQ(boxes2[0].maxY, 12.0f / 16.0f);

    // 水位3：y从 4/16 到 15/16
    const auto& shape3 = cauldron_->getContentShape(3);
    const auto& boxes3 = shape3.boxes();
    ASSERT_FALSE(boxes3.empty());
    EXPECT_FLOAT_EQ(boxes3[0].maxY, 15.0f / 16.0f);
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

// ============================================================================
// Block::getEntityInsideCollisionShape 默认行为测试
// ============================================================================

class BlockEntityInsideCollisionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 使用 SimpleBlock（公开构造的 Block 子类）测试默认行为
        VanillaBlocks::initialize();
        // 使用石头的 BlockState 来测试默认的 getEntityInsideCollisionShape
        block_ = VanillaBlocks::STONE;
    }

    const Block* block_ = nullptr;
};

TEST_F(BlockEntityInsideCollisionTest, DefaultReturnsFullBlock)
{
    // 默认的 getEntityInsideCollisionShape 应返回完整方块形状
    // 参考 MC 原版: BlockBehaviour.getEntityInsideCollisionShape() 默认返回 Shapes.block()
    ASSERT_NE(block_, nullptr);
    const auto& state = block_->defaultState();
    const auto& shape = block_->getEntityInsideCollisionShape(state);
    EXPECT_TRUE(shape.isFullBlock()) << "Default getEntityInsideCollisionShape should return full block shape";
}

// ============================================================================
// CauldronBlock::onEntityCollision 测试
// ============================================================================

/**
 * @brief 炼药锅实体碰撞测试用实体
 *
 * 简单的 LivingEntity 实现，支持设置着火状态
 */
class CauldronTestEntity : public LivingEntity {
public:
    CauldronTestEntity(EntityId id, IWorld* world = nullptr)
        : LivingEntity(id, world)
    {
        setHealth(20.0f);
    }

    [[nodiscard]] bool isImmuneToFire() const override { return m_immuneToFire; }

    void setImmuneToFire(bool immune) { m_immuneToFire = immune; }

private:
    bool m_immuneToFire = false;
};

/**
 * @brief 炼药锅实体碰撞测试用世界
 *
 * 继承 BaseTestWorld，提供方块状态存储。
 */
class CauldronCollisionTestWorld : public test::BaseTestWorld {
public:
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

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    [[nodiscard]] i32 getCauldronLevel(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return -1;
        }
        return CauldronBlock::getLevel(*it->second);
    }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
};

class CauldronEntityCollisionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        cauldron_ = std::make_unique<CauldronBlock>(BlockProperties(Material::ROCK).hardness(2.0f).resistance(2.0f));
    }

    std::unique_ptr<CauldronBlock> cauldron_;
    CauldronCollisionTestWorld world_;
};

TEST_F(CauldronEntityCollisionTest, EmptyCauldron_NoEffect)
{
    // 空炼药锅（水位0）：onEntityCollision 不执行任何操作
    const BlockPos pos(0, 64, 0);
    const auto& state0 = cauldron_->defaultState(); // 水位0
    world_.setBlockAt(pos, &state0);
    ASSERT_EQ(world_.getCauldronLevel(pos), 0);

    CauldronTestEntity entity(EntityId(1), &world_);
    entity.igniteForTicks(100); // 设置实体着火
    ASSERT_TRUE(entity.isOnFire());

    cauldron_->onEntityCollision(state0, world_, pos, entity);

    // 空炼药锅不灭火，不降低水位
    EXPECT_TRUE(entity.isOnFire()) << "Empty cauldron should not extinguish entity";
    EXPECT_EQ(world_.getCauldronLevel(pos), 0) << "Empty cauldron level should remain 0";
}

TEST_F(CauldronEntityCollisionTest, WaterLevel1_BurningEntity_ExtinguishesAndLowersLevel)
{
    // 水位1 + 着火实体 → 灭火 + 水位降为0
    const BlockPos pos(0, 64, 0);
    const auto& state1 = cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), 1);
    world_.setBlockAt(pos, &state1);
    ASSERT_EQ(world_.getCauldronLevel(pos), 1);

    CauldronTestEntity entity(EntityId(1), &world_);
    entity.igniteForTicks(100);
    ASSERT_TRUE(entity.isOnFire());

    cauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);

    // 灭火成功
    EXPECT_FALSE(entity.isOnFire()) << "Water cauldron should extinguish burning entity";
    // 水位降低1级
    EXPECT_EQ(world_.getCauldronLevel(pos), 0) << "Water level should decrease from 1 to 0";
}

TEST_F(CauldronEntityCollisionTest, WaterLevel2_BurningEntity_ExtinguishesAndLowersLevel)
{
    // 水位2 + 着火实体 → 灭火 + 水位降为1
    const BlockPos pos(0, 64, 0);
    const auto& state2 = cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), 2);
    world_.setBlockAt(pos, &state2);
    ASSERT_EQ(world_.getCauldronLevel(pos), 2);

    CauldronTestEntity entity(EntityId(1), &world_);
    entity.igniteForTicks(100);
    ASSERT_TRUE(entity.isOnFire());

    cauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);

    EXPECT_FALSE(entity.isOnFire()) << "Water cauldron should extinguish burning entity";
    EXPECT_EQ(world_.getCauldronLevel(pos), 1) << "Water level should decrease from 2 to 1";
}

TEST_F(CauldronEntityCollisionTest, WaterLevel3_BurningEntity_ExtinguishesAndLowersLevel)
{
    // 水位3（满）+ 着火实体 → 灭火 + 水位降为2
    const BlockPos pos(0, 64, 0);
    const auto& state3 = cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), 3);
    world_.setBlockAt(pos, &state3);
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    CauldronTestEntity entity(EntityId(1), &world_);
    entity.igniteForTicks(100);
    ASSERT_TRUE(entity.isOnFire());

    cauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);

    EXPECT_FALSE(entity.isOnFire()) << "Water cauldron should extinguish burning entity";
    EXPECT_EQ(world_.getCauldronLevel(pos), 2) << "Water level should decrease from 3 to 2";
}

TEST_F(CauldronEntityCollisionTest, NonBurningEntity_NoEffect)
{
    // 水位3 + 未着火实体 → 不灭火 + 水位不变
    const BlockPos pos(0, 64, 0);
    const auto& state3 = cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), 3);
    world_.setBlockAt(pos, &state3);
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    CauldronTestEntity entity(EntityId(1), &world_);
    // 默认不着火
    ASSERT_FALSE(entity.isOnFire());

    cauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);

    // 未着火实体不触发灭火和降水位
    EXPECT_EQ(world_.getCauldronLevel(pos), 3) << "Non-burning entity should not affect water level";
}

TEST_F(CauldronEntityCollisionTest, FireImmuneBurningEntity_NoExtinguishNoLevelChange)
{
    // 着火但免疫火焰的实体（isOnFire 返回 false）→ 不触发灭火
    // 因为 isOnFire() = !isImmuneToFire() && m_fire > 0
    // 免疫火焰时 isOnFire() 返回 false
    const BlockPos pos(0, 64, 0);
    const auto& state3 = cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), 3);
    world_.setBlockAt(pos, &state3);
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    CauldronTestEntity entity(EntityId(1), &world_);
    entity.setImmuneToFire(true);
    entity.igniteForTicks(100);
    // 免疫火焰的实体，isOnFire() 返回 false
    ASSERT_FALSE(entity.isOnFire()) << "Fire-immune entity should not be considered on fire";

    cauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);

    // 免疫火焰的实体不触发灭火和降水位
    EXPECT_EQ(world_.getCauldronLevel(pos), 3) << "Fire-immune entity should not affect water level";
}

TEST_F(CauldronEntityCollisionTest, MultipleExtinguish_DecreasesLevelEachTime)
{
    // 连续灭火：水位3 → 2 → 1 → 0
    const BlockPos pos(0, 64, 0);
    const auto& state3 = cauldron_->defaultState().with(BlockStateProperties::LEVEL_0_3(), 3);
    world_.setBlockAt(pos, &state3);
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    // 第一次：水位3 → 2
    {
        CauldronTestEntity entity(EntityId(1), &world_);
        entity.igniteForTicks(100);
        cauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);
        EXPECT_EQ(world_.getCauldronLevel(pos), 2);
    }

    // 第二次：水位2 → 1
    {
        CauldronTestEntity entity(EntityId(2), &world_);
        entity.igniteForTicks(100);
        cauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);
        EXPECT_EQ(world_.getCauldronLevel(pos), 1);
    }

    // 第三次：水位1 → 0
    {
        CauldronTestEntity entity(EntityId(3), &world_);
        entity.igniteForTicks(100);
        cauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);
        EXPECT_EQ(world_.getCauldronLevel(pos), 0);
    }

    // 第四次：水位0，空炼药锅不执行任何操作
    {
        CauldronTestEntity entity(EntityId(4), &world_);
        entity.igniteForTicks(100);
        const BlockState* currentState = world_.getBlockState(pos.x, pos.y, pos.z);
        cauldron_->onEntityCollision(*currentState, world_, pos, entity);
        // 空炼药锅不灭火
        EXPECT_TRUE(entity.isOnFire()) << "Empty cauldron should not extinguish entity";
        EXPECT_EQ(world_.getCauldronLevel(pos), 0);
    }
}
