/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
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

#include "world/block/blocks/LayeredCauldronBlock.hpp"
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
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "util/property/Properties.hpp"
#include <gtest/gtest.h>

#include <map>
#include <memory>

using namespace mc;
using namespace mc::blocks;
using namespace mc::world::biome;

// ============================================================================
// 测试用世界桩 - 用于 LayeredCauldronBlock 测试
// ============================================================================

/**
 * @brief LayeredCauldronBlock 测试用的世界桩
 *
 * 继承 BaseTestWorld，提供可控的方块状态存储、天气控制和随机数控制。
 */
class LayeredCauldronTestWorld : public test::BaseTestWorld {
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
        const_cast<LayeredCauldronTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    void setRaining(bool raining) { m_isRaining = raining; }
    void setThundering(bool thundering) { m_isThundering = thundering; }

    [[nodiscard]] bool isRaining() const override { return m_isRaining; }
    [[nodiscard]] bool isThundering() const override { return m_isThundering; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    /// 设置随机数种子以控制 nextFloat() 结果
    void setRandomSeed(u64 seed) { m_random.setSeed(seed); }

    /// 获取指定位置的水炼药锅水位
    [[nodiscard]] i32 getCauldronLevel(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return -1;
        }
        return LayeredCauldronBlock::getLevel(*it->second);
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

    /// 检查指定位置是否为水炼药锅
    [[nodiscard]] bool isWaterCauldron(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return false;
        }
        return it->second->is(block_registry::BuildingBlocks::WATER_CAULDRON);
    }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    bool m_isRaining = false;
    bool m_isThundering = false;
};

// ============================================================================
// LayeredCauldronBlock 基础测试
// ============================================================================

class LayeredCauldronBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        fluid::FluidTags::initialize();

        // 创建水炼药锅（降水类型为 Rain）
        waterCauldron_ = std::make_unique<LayeredCauldronBlock>(
            BlockProperties(Material::IRON).hardness(2.0f).resistance(2.0f).notSolid(),
            BiomeClimate::Precipitation::Rain);
    }

    std::unique_ptr<LayeredCauldronBlock> waterCauldron_;
};

TEST_F(LayeredCauldronBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(waterCauldron_, nullptr);
}

TEST_F(LayeredCauldronBlockTest, GetLevel_DefaultIs1)
{
    // 水炼药锅默认水位为1（LEVEL_1_3 的最小值）
    const auto& state = waterCauldron_->defaultState();
    EXPECT_EQ(LayeredCauldronBlock::getLevel(state), 1);
}

TEST_F(LayeredCauldronBlockTest, GetLevel_AllLevels)
{
    for (i32 level = 1; level <= 3; ++level) {
        const auto& state = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), level);
        EXPECT_EQ(LayeredCauldronBlock::getLevel(state), level) << "Level " << level << " should be readable";
    }
}

TEST_F(LayeredCauldronBlockTest, IsFull_Level3_ReturnsTrue)
{
    const auto& state = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 3);
    EXPECT_TRUE(LayeredCauldronBlock::isFull(state));
}

TEST_F(LayeredCauldronBlockTest, IsFull_Level1And2_ReturnsFalse)
{
    const auto& state1 = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 1);
    EXPECT_FALSE(LayeredCauldronBlock::isFull(state1));

    const auto& state2 = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 2);
    EXPECT_FALSE(LayeredCauldronBlock::isFull(state2));
}

TEST_F(LayeredCauldronBlockTest, HasComparatorInputOverride_ReturnsTrue)
{
    const auto& state = waterCauldron_->defaultState();
    EXPECT_TRUE(waterCauldron_->hasComparatorInputOverride(state));
}

TEST_F(LayeredCauldronBlockTest, GetComparatorInputOverride_ReturnsLevel)
{
    // 比较器信号 = 水位 (1-3)
    for (i32 level = 1; level <= 3; ++level) {
        LayeredCauldronTestWorld world;
        const auto& state = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), level);
        EXPECT_EQ(waterCauldron_->getComparatorInputOverride(state, world, BlockPos(0, 64, 0)), level)
            << "Comparator output for level " << level << " should be " << level;
    }
}

TEST_F(LayeredCauldronBlockTest, TicksRandomly_ReturnsFalse)
{
    EXPECT_FALSE(waterCauldron_->ticksRandomly());
}

TEST_F(LayeredCauldronBlockTest, GetShape_ReturnsValidShape)
{
    const auto& state = waterCauldron_->defaultState();
    const auto& shape = waterCauldron_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(LayeredCauldronBlockTest, GetCollisionShape_ReturnsValidShape)
{
    const auto& state = waterCauldron_->defaultState();
    const auto& shape = waterCauldron_->getCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(LayeredCauldronBlockTest, GetContentShape_AllLevels)
{
    for (i32 level = 1; level <= 3; ++level) {
        const auto& shape = waterCauldron_->getContentShape(level);
        EXPECT_FALSE(shape.isEmpty()) << "Level " << level << " should have content shape";
    }
}

TEST_F(LayeredCauldronBlockTest, GetContentShape_HeightsMatchMCValues)
{
    // MC 原版 LayeredCauldronBlock 内容高度：
    // 水位1: 9像素 (0.5625), 水位2: 12像素 (0.75), 水位3: 15像素 (0.9375)
    // 内容区域从 y=4/16 开始
    constexpr f32 innerMinY = 4.0f / 16.0f;
    constexpr f32 innerX1 = 2.0f / 16.0f;
    constexpr f32 innerX2 = 14.0f / 16.0f;

    // 水位1：y从 4/16 到 9/16
    const auto& shape1 = waterCauldron_->getContentShape(1);
    EXPECT_FALSE(shape1.isEmpty());
    const auto& boxes1 = shape1.boxes();
    ASSERT_FALSE(boxes1.empty());
    EXPECT_FLOAT_EQ(boxes1[0].minY, innerMinY);
    EXPECT_FLOAT_EQ(boxes1[0].maxY, 9.0f / 16.0f);
    EXPECT_FLOAT_EQ(boxes1[0].minX, innerX1);
    EXPECT_FLOAT_EQ(boxes1[0].maxX, innerX2);

    // 水位2：y从 4/16 到 12/16
    const auto& shape2 = waterCauldron_->getContentShape(2);
    const auto& boxes2 = shape2.boxes();
    ASSERT_FALSE(boxes2.empty());
    EXPECT_FLOAT_EQ(boxes2[0].maxY, 12.0f / 16.0f);

    // 水位3：y从 4/16 到 15/16
    const auto& shape3 = waterCauldron_->getContentShape(3);
    const auto& boxes3 = shape3.boxes();
    ASSERT_FALSE(boxes3.empty());
    EXPECT_FLOAT_EQ(boxes3[0].maxY, 15.0f / 16.0f);
}

TEST_F(LayeredCauldronBlockTest, GetEntityInsideCollisionShape_Level1_ReturnsFilledShape)
{
    const auto& state = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 1);
    const auto& shape = waterCauldron_->getEntityInsideCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty()) << "Level 1 should have entity inside collision shape";
    EXPECT_FALSE(shape.isFullBlock()) << "Level 1 should not be full block (smaller fill area)";
}

TEST_F(LayeredCauldronBlockTest, GetEntityInsideCollisionShape_Level2_ReturnsFilledShape)
{
    const auto& state = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 2);
    const auto& shape = waterCauldron_->getEntityInsideCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty()) << "Level 2 should have entity inside collision shape";
    EXPECT_FALSE(shape.isFullBlock()) << "Level 2 should not be full block";
}

TEST_F(LayeredCauldronBlockTest, GetEntityInsideCollisionShape_Level3_ReturnsFilledShape)
{
    const auto& state = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 3);
    const auto& shape = waterCauldron_->getEntityInsideCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty()) << "Level 3 should have entity inside collision shape";
    EXPECT_FALSE(shape.isFullBlock()) << "Level 3 should not be full block";
}

TEST_F(LayeredCauldronBlockTest, GetEntityInsideCollisionShape_FilledShapeContainsOuterShape)
{
    // 填充形状应包含外部炼药锅形状
    const auto& outerShape = waterCauldron_->getShape(waterCauldron_->defaultState());
    for (i32 level = 1; level <= 3; ++level) {
        const auto& state = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), level);
        const auto& filledShape = waterCauldron_->getEntityInsideCollisionShape(state);
        EXPECT_GE(filledShape.boxCount(), outerShape.boxCount())
            << "Level " << level << " filled shape should have at least as many boxes as outer shape";
    }
}

TEST_F(LayeredCauldronBlockTest, UseShapeForLightOcclusion_ReturnsTrue)
{
    const auto& state = waterCauldron_->defaultState();
    EXPECT_TRUE(waterCauldron_->useShapeForLightOcclusion(state));
}

// ============================================================================
// LayeredCauldronBlock handlePrecipitation 测试夹具
// ============================================================================

class LayeredCauldronPrecipTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        fluid::FluidTags::initialize();

        // 创建水炼药锅（降水类型为 Rain）
        waterCauldron_ = std::make_unique<LayeredCauldronBlock>(
            BlockProperties(Material::IRON).hardness(2.0f).resistance(2.0f).notSolid(),
            BiomeClimate::Precipitation::Rain);
        world_.setRaining(true);
    }

    /// 在指定位置放置指定水位的水炼药锅
    void placeWaterCauldron(i32 x, i32 y, i32 z, i32 level)
    {
        const BlockState* state = &waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), level);
        world_.setBlockAt(BlockPos(x, y, z), state);
    }

    std::unique_ptr<LayeredCauldronBlock> waterCauldron_;
    LayeredCauldronTestWorld world_;
};

// ============================================================================
// 降水类型 - None（无降水）
// ============================================================================

TEST_F(LayeredCauldronPrecipTest, HandlePrecipitation_NoneType_NoChange)
{
    placeWaterCauldron(0, 64, 0, 1);
    i32 levelBefore = world_.getCauldronLevel(BlockPos(0, 64, 0));
    ASSERT_EQ(levelBefore, 1);

    // 无降水类型不会改变水位
    waterCauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::None);

    EXPECT_EQ(world_.getCauldronLevel(BlockPos(0, 64, 0)), 1);
}

// ============================================================================
// 降水类型 - Rain（雨天，水炼药锅响应）
// ============================================================================

TEST_F(LayeredCauldronPrecipTest, HandlePrecipitation_Rain_IncrementsLevelWhenNotFull)
{
    // 水位1，雨天，5%概率触发水位增加
    placeWaterCauldron(0, 64, 0, 1);

    bool levelChanged = false;
    for (int i = 0; i < 200; ++i) {
        i32 levelBefore = world_.getCauldronLevel(BlockPos(0, 64, 0));
        waterCauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
        i32 levelAfter = world_.getCauldronLevel(BlockPos(0, 64, 0));
        if (levelAfter > levelBefore) {
            levelChanged = true;
            break;
        }
    }

    EXPECT_TRUE(levelChanged) << "Rain precipitation should eventually increment water cauldron level";
}

TEST_F(LayeredCauldronPrecipTest, HandlePrecipitation_Rain_DoesNotExceedMaxLevel)
{
    // 水位3（满），雨天不应增加
    placeWaterCauldron(0, 64, 0, 3);

    for (int i = 0; i < 100; ++i) {
        waterCauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
    }

    EXPECT_EQ(world_.getCauldronLevel(BlockPos(0, 64, 0)), 3);
}

TEST_F(LayeredCauldronPrecipTest, HandlePrecipitation_Rain_LevelCapsAt3)
{
    // 水位2，多次雨天触发后水位不应超过3
    placeWaterCauldron(0, 64, 0, 2);
    world_.setRandomSeed(12345);

    for (int i = 0; i < 500; ++i) {
        waterCauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
    }

    EXPECT_EQ(world_.getCauldronLevel(BlockPos(0, 64, 0)), 3);
}

// ============================================================================
// 降水类型 - Snow（雪天，水炼药锅不响应雪天降水）
// ============================================================================

TEST_F(LayeredCauldronPrecipTest, HandlePrecipitation_Snow_DoesNotAffectWaterCauldron)
{
    // 水炼药锅（降水类型为 Rain）不响应雪天降水
    // 只有细雪炼药锅（降水类型为 Snow）才响应雪天
    placeWaterCauldron(0, 64, 0, 1);

    for (int i = 0; i < 200; ++i) {
        waterCauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Snow);
    }

    // 水位不应改变
    EXPECT_EQ(world_.getCauldronLevel(BlockPos(0, 64, 0)), 1);
}

// ============================================================================
// 概率测试 - 确保雨天5%的概率行为
// ============================================================================

TEST_F(LayeredCauldronPrecipTest, HandlePrecipitation_Rain_About5PercentChance)
{
    // 统计2000次雨天调用中水位增加的次数，应该在5%左右
    i32 incrementCount = 0;
    constexpr int TOTAL_TRIALS = 2000;

    for (int i = 0; i < TOTAL_TRIALS; ++i) {
        // 重置水位为1
        placeWaterCauldron(0, 64, 0, 1);

        i32 levelBefore = world_.getCauldronLevel(BlockPos(0, 64, 0));
        waterCauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
        i32 levelAfter = world_.getCauldronLevel(BlockPos(0, 64, 0));

        if (levelAfter > levelBefore) {
            incrementCount++;
        }
    }

    // 5% 概率，2000次约100次。允许较宽的范围：30-170（1.5% ~ 8.5%）
    EXPECT_GE(incrementCount, 30) << "Rain increment count too low";
    EXPECT_LE(incrementCount, 170) << "Rain increment count too high";
}

// ============================================================================
// 边界条件测试
// ============================================================================

TEST_F(LayeredCauldronPrecipTest, HandlePrecipitation_NullBlockState_NoCrash)
{
    // 没有放置水炼药锅的位置，getBlockState 返回 nullptr
    // handlePrecipitation 不应崩溃
    waterCauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
    SUCCEED();
}

TEST_F(LayeredCauldronPrecipTest, HandlePrecipitation_FullCauldron_NoIncrementForAnyPrecipitationType)
{
    // 满的水炼药锅不应被任何降水类型增加
    placeWaterCauldron(0, 64, 0, 3);

    world_.setRandomSeed(42);
    for (int i = 0; i < 100; ++i) {
        waterCauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
    }
    EXPECT_EQ(world_.getCauldronLevel(BlockPos(0, 64, 0)), 3);
}

TEST_F(LayeredCauldronPrecipTest, HandlePrecipitation_LevelTwoCanReachMax)
{
    // 水位2可以在雨天达到3
    placeWaterCauldron(0, 64, 0, 2);
    world_.setRandomSeed(42);

    bool reachedMax = false;
    for (int i = 0; i < 200; ++i) {
        waterCauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
        if (world_.getCauldronLevel(BlockPos(0, 64, 0)) == 3) {
            reachedMax = true;
            break;
        }
    }

    EXPECT_TRUE(reachedMax) << "Level 2 water cauldron should be able to reach level 3 with rain";
}

// ============================================================================
// LayeredCauldronBlock::onEntityCollision 测试
// ============================================================================

/**
 * @brief 水炼药锅实体碰撞测试用实体
 */
class LayeredCauldronTestEntity : public LivingEntity {
public:
    LayeredCauldronTestEntity(EntityId id, IWorld* world = nullptr)
        : LivingEntity(id, world)
    {
        setHealth(20.0f);
    }

    [[nodiscard]] bool isImmuneToFire() const override { return m_immuneToFire; }

    void setImmuneToFire(bool immune) { m_immuneToFire = immune; }

private:
    bool m_immuneToFire = false;
};

class LayeredCauldronEntityCollisionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        // 创建水炼药锅（降水类型为 Rain）
        waterCauldron_ = std::make_unique<LayeredCauldronBlock>(
            BlockProperties(Material::IRON).hardness(2.0f).resistance(2.0f).notSolid(),
            BiomeClimate::Precipitation::Rain);
    }

    /// 在指定位置放置指定水位的水炼药锅
    void placeWaterCauldron(const BlockPos& pos, i32 level)
    {
        const BlockState* state = &waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), level);
        world_.setBlockAt(pos, state);
    }

    std::unique_ptr<LayeredCauldronBlock> waterCauldron_;
    LayeredCauldronTestWorld world_;
};

TEST_F(LayeredCauldronEntityCollisionTest, Level1_BurningEntity_ExtinguishesAndLowersLevel)
{
    // 水位1 + 着火实体 → 灭火 + 水位降为0（替换为空炼药锅）
    const BlockPos pos(0, 64, 0);
    placeWaterCauldron(pos, 1);
    ASSERT_EQ(world_.getCauldronLevel(pos), 1);

    LayeredCauldronTestEntity entity(EntityId(1), &world_);
    entity.igniteForTicks(100);
    ASSERT_TRUE(entity.isOnFire());

    waterCauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);

    // 灭火成功
    EXPECT_FALSE(entity.isOnFire()) << "Water cauldron should extinguish burning entity";
    // 水位降至0，替换为空炼药锅
    EXPECT_TRUE(world_.isEmptyCauldron(pos)) << "Water level 1 after extinguishing should become empty cauldron";
}

TEST_F(LayeredCauldronEntityCollisionTest, Level2_BurningEntity_ExtinguishesAndLowersLevel)
{
    // 水位2 + 着火实体 → 灭火 + 水位降为1
    const BlockPos pos(0, 64, 0);
    placeWaterCauldron(pos, 2);
    ASSERT_EQ(world_.getCauldronLevel(pos), 2);

    LayeredCauldronTestEntity entity(EntityId(1), &world_);
    entity.igniteForTicks(100);
    ASSERT_TRUE(entity.isOnFire());

    waterCauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);

    EXPECT_FALSE(entity.isOnFire()) << "Water cauldron should extinguish burning entity";
    EXPECT_EQ(world_.getCauldronLevel(pos), 1) << "Water level should decrease from 2 to 1";
}

TEST_F(LayeredCauldronEntityCollisionTest, Level3_BurningEntity_ExtinguishesAndLowersLevel)
{
    // 水位3（满）+ 着火实体 → 灭火 + 水位降为2
    const BlockPos pos(0, 64, 0);
    placeWaterCauldron(pos, 3);
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    LayeredCauldronTestEntity entity(EntityId(1), &world_);
    entity.igniteForTicks(100);
    ASSERT_TRUE(entity.isOnFire());

    waterCauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);

    EXPECT_FALSE(entity.isOnFire()) << "Water cauldron should extinguish burning entity";
    EXPECT_EQ(world_.getCauldronLevel(pos), 2) << "Water level should decrease from 3 to 2";
}

TEST_F(LayeredCauldronEntityCollisionTest, NonBurningEntity_NoEffect)
{
    // 水位3 + 未着火实体 → 不灭火 + 水位不变
    const BlockPos pos(0, 64, 0);
    placeWaterCauldron(pos, 3);
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    LayeredCauldronTestEntity entity(EntityId(1), &world_);
    ASSERT_FALSE(entity.isOnFire());

    waterCauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);

    EXPECT_EQ(world_.getCauldronLevel(pos), 3) << "Non-burning entity should not affect water level";
}

TEST_F(LayeredCauldronEntityCollisionTest, FireImmuneBurningEntity_NoExtinguishNoLevelChange)
{
    // 着火但免疫火焰的实体（isOnFire 返回 false）→ 不触发灭火
    const BlockPos pos(0, 64, 0);
    placeWaterCauldron(pos, 3);
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    LayeredCauldronTestEntity entity(EntityId(1), &world_);
    entity.setImmuneToFire(true);
    entity.igniteForTicks(100);
    ASSERT_FALSE(entity.isOnFire()) << "Fire-immune entity should not be considered on fire";

    waterCauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);

    EXPECT_EQ(world_.getCauldronLevel(pos), 3) << "Fire-immune entity should not affect water level";
}

TEST_F(LayeredCauldronEntityCollisionTest, MultipleExtinguish_DecreasesLevelEachTime)
{
    // 连续灭火：水位3 → 2 → 1 → 空炼药锅
    const BlockPos pos(0, 64, 0);
    placeWaterCauldron(pos, 3);
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    // 第一次：水位3 → 2
    {
        LayeredCauldronTestEntity entity(EntityId(1), &world_);
        entity.igniteForTicks(100);
        waterCauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);
        EXPECT_EQ(world_.getCauldronLevel(pos), 2);
    }

    // 第二次：水位2 → 1
    {
        LayeredCauldronTestEntity entity(EntityId(2), &world_);
        entity.igniteForTicks(100);
        waterCauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);
        EXPECT_EQ(world_.getCauldronLevel(pos), 1);
    }

    // 第三次：水位1 → 空炼药锅
    {
        LayeredCauldronTestEntity entity(EntityId(3), &world_);
        entity.igniteForTicks(100);
        waterCauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);
        // 水位降至0，替换为空炼药锅
        EXPECT_TRUE(world_.isEmptyCauldron(pos)) << "Water level 1 after extinguishing should become empty cauldron";
    }
}

// ============================================================================
// LayeredCauldronBlock::lowerFillLevel 测试
// ============================================================================

class LayeredCauldronLowerFillTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        waterCauldron_ = std::make_unique<LayeredCauldronBlock>(
            BlockProperties(Material::IRON).hardness(2.0f).resistance(2.0f).notSolid(),
            BiomeClimate::Precipitation::Rain);
    }

    std::unique_ptr<LayeredCauldronBlock> waterCauldron_;
    LayeredCauldronTestWorld world_;
};

TEST_F(LayeredCauldronLowerFillTest, Level3_LowerToLevel2)
{
    const BlockPos pos(0, 64, 0);
    const auto& state3 = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 3);
    world_.setBlockAt(pos, &state3);
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    LayeredCauldronBlock::lowerFillLevel(world_, pos, *world_.getBlockState(pos.x, pos.y, pos.z));
    EXPECT_EQ(world_.getCauldronLevel(pos), 2);
}

TEST_F(LayeredCauldronLowerFillTest, Level2_LowerToLevel1)
{
    const BlockPos pos(0, 64, 0);
    const auto& state2 = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 2);
    world_.setBlockAt(pos, &state2);
    ASSERT_EQ(world_.getCauldronLevel(pos), 2);

    LayeredCauldronBlock::lowerFillLevel(world_, pos, *world_.getBlockState(pos.x, pos.y, pos.z));
    EXPECT_EQ(world_.getCauldronLevel(pos), 1);
}

TEST_F(LayeredCauldronLowerFillTest, Level1_LowerToEmptyCauldron)
{
    // 水位1降低到0时，应替换为空炼药锅
    const BlockPos pos(0, 64, 0);
    const auto& state1 = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 1);
    world_.setBlockAt(pos, &state1);
    ASSERT_EQ(world_.getCauldronLevel(pos), 1);

    LayeredCauldronBlock::lowerFillLevel(world_, pos, *world_.getBlockState(pos.x, pos.y, pos.z));

    // 应替换为空炼药锅
    EXPECT_TRUE(world_.isEmptyCauldron(pos)) << "Lowering level 1 should replace with empty cauldron";
    EXPECT_FALSE(world_.isWaterCauldron(pos)) << "Should no longer be water cauldron";
}

// ============================================================================
// LayeredCauldronBlock::setLevel 测试
// ============================================================================

class LayeredCauldronSetLevelTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        waterCauldron_ = std::make_unique<LayeredCauldronBlock>(
            BlockProperties(Material::IRON).hardness(2.0f).resistance(2.0f).notSolid(),
            BiomeClimate::Precipitation::Rain);
    }

    std::unique_ptr<LayeredCauldronBlock> waterCauldron_;
    LayeredCauldronTestWorld world_;
};

TEST_F(LayeredCauldronSetLevelTest, SetLevel_From1To3)
{
    const BlockPos pos(0, 64, 0);
    const auto& state1 = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 1);
    world_.setBlockAt(pos, &state1);
    ASSERT_EQ(world_.getCauldronLevel(pos), 1);

    LayeredCauldronBlock::setLevel(world_, pos, *world_.getBlockState(pos.x, pos.y, pos.z), 3);
    EXPECT_EQ(world_.getCauldronLevel(pos), 3);
}

TEST_F(LayeredCauldronSetLevelTest, SetLevel_SameLevel_NoChange)
{
    const BlockPos pos(0, 64, 0);
    const auto& state2 = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 2);
    world_.setBlockAt(pos, &state2);
    ASSERT_EQ(world_.getCauldronLevel(pos), 2);

    LayeredCauldronBlock::setLevel(world_, pos, *world_.getBlockState(pos.x, pos.y, pos.z), 2);
    EXPECT_EQ(world_.getCauldronLevel(pos), 2);
}

// ============================================================================
// LayeredCauldronBlock 旗帜清洗测试
// ============================================================================

class LayeredCauldronBannerCleaningTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();

        waterCauldron_ = std::make_unique<LayeredCauldronBlock>(
            BlockProperties(Material::IRON).hardness(2.0f).resistance(2.0f).notSolid(),
            BiomeClimate::Precipitation::Rain);
    }

    std::unique_ptr<LayeredCauldronBlock> waterCauldron_;
};

TEST_F(LayeredCauldronBannerCleaningTest, BannerItemIsRecognizedAsBanner)
{
    if (Items::WHITE_BANNER == nullptr) {
        GTEST_SKIP() << "WHITE_BANNER not registered";
    }
    const auto* bannerItem = dynamic_cast<const item::BannerItem*>(Items::WHITE_BANNER);
    EXPECT_NE(bannerItem, nullptr) << "WHITE_BANNER should be a BannerItem";
}

TEST_F(LayeredCauldronBannerCleaningTest, BannerWithNoPatternsReturnsPass)
{
    if (Items::WHITE_BANNER == nullptr) {
        GTEST_SKIP() << "WHITE_BANNER not registered";
    }
    ItemStack bannerStack(*Items::WHITE_BANNER, 1);
    EXPECT_EQ(blockentity::BannerEntity::getPatternCount(bannerStack), 0);
}

TEST_F(LayeredCauldronBannerCleaningTest, BannerWithPatternsReturnsCorrectCount)
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

TEST_F(LayeredCauldronBannerCleaningTest, RemoveBannerData_DecreasesPatternCount)
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

TEST_F(LayeredCauldronBannerCleaningTest, RemoveBannerDataFromShield_WorksSameAsBanner)
{
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

TEST_F(LayeredCauldronBannerCleaningTest, NonBannerNonShieldItem_HasZeroPatterns)
{
    if (Items::BUCKET == nullptr) {
        GTEST_SKIP() << "BUCKET not registered";
    }
    ItemStack bucketStack(*Items::BUCKET, 1);
    EXPECT_EQ(blockentity::BannerEntity::getPatternCount(bucketStack), 0);
}

TEST_F(LayeredCauldronBannerCleaningTest, EmptyStack_HasZeroPatterns)
{
    ItemStack emptyStack;
    EXPECT_EQ(blockentity::BannerEntity::getPatternCount(emptyStack), 0);
    // 清洗空物品不应崩溃
    blockentity::BannerEntity::removeBannerData(emptyStack);
    SUCCEED();
}

TEST_F(LayeredCauldronBannerCleaningTest, WaterCauldronLevelConstants)
{
    // 验证水炼药锅水位范围（1-3）
    const auto& state1 = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 1);
    const auto& state2 = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 2);
    const auto& state3 = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 3);

    EXPECT_EQ(LayeredCauldronBlock::getLevel(state1), 1);
    EXPECT_EQ(LayeredCauldronBlock::getLevel(state2), 2);
    EXPECT_EQ(LayeredCauldronBlock::getLevel(state3), 3);

    EXPECT_FALSE(LayeredCauldronBlock::isFull(state1));
    EXPECT_FALSE(LayeredCauldronBlock::isFull(state2));
    EXPECT_TRUE(LayeredCauldronBlock::isFull(state3));
}

// ============================================================================
// LayeredCauldronBlock 滴石滴水测试
// ============================================================================

class LayeredCauldronDripTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        fluid::Fluids::initialize();
        fluid::FluidTags::initialize();

        waterCauldron_ = std::make_unique<LayeredCauldronBlock>(
            BlockProperties(Material::IRON).hardness(2.0f).resistance(2.0f).notSolid(),
            BiomeClimate::Precipitation::Rain);
    }

    std::unique_ptr<LayeredCauldronBlock> waterCauldron_;
    LayeredCauldronTestWorld world_;
};

TEST_F(LayeredCauldronDripTest, CanReceiveStalactiteDrip_Water_ReturnsTrueWhenNotFull)
{
    // 水炼药锅（水位未满时）可以接收水滴水
    const auto& state1 = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 1);
    // canReceiveStalactiteDrip 检查水位是否未满
    EXPECT_TRUE(waterCauldron_->canReceiveStalactiteDrip(*fluid::Fluids::WATER()));
}

TEST_F(LayeredCauldronDripTest, CanReceiveStalactiteDrip_Lava_ReturnsFalse)
{
    // 水炼药锅不接收岩浆滴水
    EXPECT_FALSE(waterCauldron_->canReceiveStalactiteDrip(*fluid::Fluids::LAVA()));
}

TEST_F(LayeredCauldronDripTest, ReceiveStalactiteDrip_WaterDrip_IncrementsLevel)
{
    // 水滴水增加1级水位
    const BlockPos pos(0, 64, 0);
    const auto& state1 = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 1);
    world_.setBlockAt(pos, &state1);
    ASSERT_EQ(world_.getCauldronLevel(pos), 1);

    waterCauldron_->receiveStalactiteDrip(
        world_, pos, *world_.getBlockState(pos.x, pos.y, pos.z), *fluid::Fluids::WATER());
    EXPECT_EQ(world_.getCauldronLevel(pos), 2);
}

TEST_F(LayeredCauldronDripTest, ReceiveStalactiteDrip_WaterDrip_DoesNotExceedMaxLevel)
{
    // 满水位的水炼药锅不应再增加
    const BlockPos pos(0, 64, 0);
    const auto& state3 = waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), 3);
    world_.setBlockAt(pos, &state3);
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    waterCauldron_->receiveStalactiteDrip(
        world_, pos, *world_.getBlockState(pos.x, pos.y, pos.z), *fluid::Fluids::WATER());
    EXPECT_EQ(world_.getCauldronLevel(pos), 3);
}
