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
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BannerItem.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/interactive/BannerEntity.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "util/property/Properties.hpp"
#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <vector>

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
    LayeredCauldronTestEntity(EntityInstanceId id, IWorld* world = nullptr)
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

    LayeredCauldronTestEntity entity(EntityInstanceId(1), &world_);
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

    LayeredCauldronTestEntity entity(EntityInstanceId(1), &world_);
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

    LayeredCauldronTestEntity entity(EntityInstanceId(1), &world_);
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

    LayeredCauldronTestEntity entity(EntityInstanceId(1), &world_);
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

    LayeredCauldronTestEntity entity(EntityInstanceId(1), &world_);
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
        LayeredCauldronTestEntity entity(EntityInstanceId(1), &world_);
        entity.igniteForTicks(100);
        waterCauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);
        EXPECT_EQ(world_.getCauldronLevel(pos), 2);
    }

    // 第二次：水位2 → 1
    {
        LayeredCauldronTestEntity entity(EntityInstanceId(2), &world_);
        entity.igniteForTicks(100);
        waterCauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);
        EXPECT_EQ(world_.getCauldronLevel(pos), 1);
    }

    // 第三次：水位1 → 空炼药锅
    {
        LayeredCauldronTestEntity entity(EntityInstanceId(3), &world_);
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

// ============================================================================
// 细雪炼药锅 (PowderSnowCauldron) 测试
// ============================================================================

/**
 * @brief 细雪炼药锅测试用的世界桩
 *
 * 在 LayeredCauldronTestWorld 基础上增加细雪炼药锅判定辅助方法。
 */
class PowderSnowCauldronTestWorld : public test::BaseTestWorld {
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
        const_cast<PowderSnowCauldronTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    void setRaining(bool raining) { m_isRaining = raining; }
    void setThundering(bool thundering) { m_isThundering = thundering; }

    [[nodiscard]] bool isRaining() const override { return m_isRaining; }
    [[nodiscard]] bool isThundering() const override { return m_isThundering; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    void setRandomSeed(u64 seed) { m_random.setSeed(seed); }

    [[nodiscard]] i32 getCauldronLevel(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return -1;
        }
        return LayeredCauldronBlock::getLevel(*it->second);
    }

    [[nodiscard]] bool isEmptyCauldron(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return false;
        }
        return it->second->is(block_registry::BuildingBlocks::CAULDRON);
    }

    [[nodiscard]] bool isWaterCauldron(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return false;
        }
        return it->second->is(block_registry::BuildingBlocks::WATER_CAULDRON);
    }

    [[nodiscard]] bool isPowderSnowCauldron(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return false;
        }
        return it->second->is(block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON);
    }

    [[nodiscard]] bool isLavaCauldron(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return false;
        }
        return it->second->is(block_registry::BuildingBlocks::LAVA_CAULDRON);
    }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    bool m_isRaining = false;
    bool m_isThundering = false;
};

// ============================================================================
// 细雪炼药锅降水测试
// ============================================================================

class PowderSnowCauldronPrecipTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        fluid::FluidTags::initialize();

        // 使用已注册的 POWDER_SNOW_CAULDRON 方块
        powderSnowCauldron_ = dynamic_cast<LayeredCauldronBlock*>(block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON);
        ASSERT_NE(powderSnowCauldron_, nullptr) << "POWDER_SNOW_CAULDRON should be a LayeredCauldronBlock";
    }

    /// 在指定位置放置指定水位的细雪炼药锅
    void placePowderSnowCauldron(i32 x, i32 y, i32 z, i32 level)
    {
        const BlockState* state = &powderSnowCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), level);
        world_.setBlockAt(BlockPos(x, y, z), state);
    }

    LayeredCauldronBlock* powderSnowCauldron_;
    PowderSnowCauldronTestWorld world_;
};

// (1) 雪天降水增加细雪炼药锅水位
TEST_F(PowderSnowCauldronPrecipTest, Snow_IncrementsLevelWhenNotFull)
{
    // 细雪炼药锅水位1，雪天，10%概率触发水位增加
    placePowderSnowCauldron(0, 64, 0, 1);

    bool levelChanged = false;
    for (int i = 0; i < 200; ++i) {
        i32 levelBefore = world_.getCauldronLevel(BlockPos(0, 64, 0));
        powderSnowCauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Snow);
        i32 levelAfter = world_.getCauldronLevel(BlockPos(0, 64, 0));
        if (levelAfter > levelBefore) {
            levelChanged = true;
            break;
        }
    }

    EXPECT_TRUE(levelChanged) << "Snow precipitation should eventually increment powder snow cauldron level";
}

// (2) 雨天降水对细雪炼药锅无影响（类型不匹配）
TEST_F(PowderSnowCauldronPrecipTest, Rain_DoesNotAffectPowderSnowCauldron)
{
    // 细雪炼药锅不响应雨天降水（m_precipitationType == Snow）
    placePowderSnowCauldron(0, 64, 0, 1);

    for (int i = 0; i < 200; ++i) {
        powderSnowCauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);
    }

    // 水位不应改变
    EXPECT_EQ(world_.getCauldronLevel(BlockPos(0, 64, 0)), 1) << "Rain should not affect powder snow cauldron";
}

// (3) 满水位细雪炼药锅雪天不增加
TEST_F(PowderSnowCauldronPrecipTest, Snow_DoesNotExceedMaxLevel)
{
    // 细雪炼药锅水位3（满），雪天不应增加
    placePowderSnowCauldron(0, 64, 0, 3);

    for (int i = 0; i < 100; ++i) {
        powderSnowCauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Snow);
    }

    EXPECT_EQ(world_.getCauldronLevel(BlockPos(0, 64, 0)), 3) << "Full powder snow cauldron should not increase level";
}

// (4) 无降水类型不影响细雪炼药锅
TEST_F(PowderSnowCauldronPrecipTest, NoneType_NoChange)
{
    placePowderSnowCauldron(0, 64, 0, 1);

    powderSnowCauldron_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::None);

    EXPECT_EQ(world_.getCauldronLevel(BlockPos(0, 64, 0)), 1);
}

// ============================================================================
// 细雪炼药锅实体碰撞测试（着火实体 → 细雪转水 + 降低水位）
// ============================================================================

class PowderSnowCauldronEntityCollisionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        // 使用已注册的 POWDER_SNOW_CAULDRON 方块，确保 isPowderSnowCauldron() 等判定正确
        powderSnowCauldron_ = dynamic_cast<LayeredCauldronBlock*>(block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON);
        ASSERT_NE(powderSnowCauldron_, nullptr) << "POWDER_SNOW_CAULDRON should be a LayeredCauldronBlock";
    }

    void placePowderSnowCauldron(const BlockPos& pos, i32 level)
    {
        const BlockState* state = &powderSnowCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), level);
        world_.setBlockAt(pos, state);
    }

    LayeredCauldronBlock* powderSnowCauldron_;
    PowderSnowCauldronTestWorld world_;
};

// (5) 着火实体碰撞细雪炼药锅水位3 → 细雪转为水炼药锅水位2
TEST_F(PowderSnowCauldronEntityCollisionTest, Level3_BurningEntity_ConvertsToWaterCauldronLevel2)
{
    const BlockPos pos(0, 64, 0);
    placePowderSnowCauldron(pos, 3);
    ASSERT_TRUE(world_.isPowderSnowCauldron(pos));
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    LayeredCauldronTestEntity entity(EntityInstanceId(1), &world_);
    entity.igniteForTicks(100);
    ASSERT_TRUE(entity.isOnFire());

    powderSnowCauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);

    // 灭火成功
    EXPECT_FALSE(entity.isOnFire()) << "Powder snow cauldron should extinguish burning entity";
    // 细雪转为水炼药锅，水位降1级：3 → 2
    EXPECT_TRUE(world_.isWaterCauldron(pos))
        << "Burning entity in powder snow cauldron should convert it to water cauldron";
    EXPECT_EQ(world_.getCauldronLevel(pos), 2) << "Water cauldron level should be 2 (3 - 1)";
}

// (6) 着火实体碰撞细雪炼药锅水位2 → 细雪转为水炼药锅水位1
TEST_F(PowderSnowCauldronEntityCollisionTest, Level2_BurningEntity_ConvertsToWaterCauldronLevel1)
{
    const BlockPos pos(0, 64, 0);
    placePowderSnowCauldron(pos, 2);
    ASSERT_TRUE(world_.isPowderSnowCauldron(pos));

    LayeredCauldronTestEntity entity(EntityInstanceId(1), &world_);
    entity.igniteForTicks(100);

    powderSnowCauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);

    EXPECT_TRUE(world_.isWaterCauldron(pos));
    EXPECT_EQ(world_.getCauldronLevel(pos), 1);
}

// (7) 着火实体碰撞细雪炼药锅水位1 → 细雪转为水炼药锅水位0（空炼药锅）
TEST_F(PowderSnowCauldronEntityCollisionTest, Level1_BurningEntity_ConvertsToEmptyCauldron)
{
    const BlockPos pos(0, 64, 0);
    placePowderSnowCauldron(pos, 1);
    ASSERT_TRUE(world_.isPowderSnowCauldron(pos));

    LayeredCauldronTestEntity entity(EntityInstanceId(1), &world_);
    entity.igniteForTicks(100);

    powderSnowCauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);

    // 细雪转水后水位1-1=0，替换为空炼药锅
    EXPECT_TRUE(world_.isEmptyCauldron(pos))
        << "Powder snow level 1 after conversion to water should become empty cauldron";
}

// (8) 非着火实体不影响细雪炼药锅
TEST_F(PowderSnowCauldronEntityCollisionTest, NonBurningEntity_NoEffect)
{
    const BlockPos pos(0, 64, 0);
    placePowderSnowCauldron(pos, 3);

    LayeredCauldronTestEntity entity(EntityInstanceId(1), &world_);
    ASSERT_FALSE(entity.isOnFire());

    powderSnowCauldron_->onEntityCollision(*world_.getBlockState(pos.x, pos.y, pos.z), world_, pos, entity);

    // 应该保持细雪炼药锅水位3
    EXPECT_TRUE(world_.isPowderSnowCauldron(pos));
    EXPECT_EQ(world_.getCauldronLevel(pos), 3);
}

// ============================================================================
// 细雪炼药锅滴石滴水测试（细雪炼药锅不接收滴石滴水）
// ============================================================================

class PowderSnowCauldronDripTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        fluid::Fluids::initialize();
        fluid::FluidTags::initialize();

        // 使用已注册的 POWDER_SNOW_CAULDRON 方块
        powderSnowCauldron_ = dynamic_cast<LayeredCauldronBlock*>(block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON);
        ASSERT_NE(powderSnowCauldron_, nullptr) << "POWDER_SNOW_CAULDRON should be a LayeredCauldronBlock";
    }

    LayeredCauldronBlock* powderSnowCauldron_;
};

// (9) 细雪炼药锅不接收水滴
TEST_F(PowderSnowCauldronDripTest, CanReceiveStalactiteDrip_Water_ReturnsFalse)
{
    // 细雪炼药锅（Snow类型）不接收水滴（仅水炼药锅可接收）
    EXPECT_FALSE(powderSnowCauldron_->canReceiveStalactiteDrip(*fluid::Fluids::WATER()));
}

// (10) 细雪炼药锅不接收岩浆滴
TEST_F(PowderSnowCauldronDripTest, CanReceiveStalactiteDrip_Lava_ReturnsFalse)
{
    EXPECT_FALSE(powderSnowCauldron_->canReceiveStalactiteDrip(*fluid::Fluids::LAVA()));
}

// ============================================================================
// 细雪炼药锅比较器信号测试
// ============================================================================

class PowderSnowCauldronComparatorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        // 使用已注册的 POWDER_SNOW_CAULDRON 方块
        powderSnowCauldron_ = dynamic_cast<LayeredCauldronBlock*>(block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON);
        ASSERT_NE(powderSnowCauldron_, nullptr) << "POWDER_SNOW_CAULDRON should be a LayeredCauldronBlock";
    }

    LayeredCauldronBlock* powderSnowCauldron_;
    PowderSnowCauldronTestWorld world_;
};

TEST_F(PowderSnowCauldronComparatorTest, HasComparatorInputOverride)
{
    const auto& state = powderSnowCauldron_->defaultState();
    EXPECT_TRUE(powderSnowCauldron_->hasComparatorInputOverride(state));
}

TEST_F(PowderSnowCauldronComparatorTest, ComparatorSignalEqualsLevel)
{
    for (i32 level = 1; level <= 3; ++level) {
        const auto& state = powderSnowCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), level);
        EXPECT_EQ(powderSnowCauldron_->getComparatorInputOverride(state, world_, BlockPos(0, 64, 0)), level)
            << "Comparator output for level " << level << " should be " << level;
    }
}

// ============================================================================
// 炼药锅交互测试用的 Mock 世界（支持 onBlockActivated 测试）
// ============================================================================

/**
 * @brief 炼药锅交互测试用 Mock 世界
 *
 * 继承 BaseTestWorld，实现 IWorld 方法，支持 Player 交互测试。
 * 提供 playSound、gameEvent、spawnEntity 等方法的 stub 实现，
 * 确保 onBlockActivated 不会因为缺少世界方法而崩溃。
 */
class CauldronInteractionTestWorld : public test::BaseTestWorld {
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
        const_cast<CauldronInteractionTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }

    void setClientSide(bool clientSide) { m_isClientSide = clientSide; }

    void setRaining(bool raining) { m_isRaining = raining; }
    void setThundering(bool thundering) { m_isThundering = thundering; }

    [[nodiscard]] bool isRaining() const override { return m_isRaining; }
    [[nodiscard]] bool isThundering() const override { return m_isThundering; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    void setRandomSeed(u64 seed) { m_random.setSeed(seed); }

    // IWorld stub 方法
    void playSound(const ResourceLocation& soundId,
        sound::SoundCategory category,
        const Vector3& pos,
        f32 volume,
        f32 pitch) override
    {
        m_soundPlayed = true;
        m_lastSoundId = soundId;
        MC_UNUSED(category, pos, volume, pitch);
    }

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override { MC_UNUSED(eventId, pos, data); }

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        MC_UNUSED(event, pos, context);
        m_gameEventFired = true;
    }

    [[nodiscard]] bool wasGameEventFired() const { return m_gameEventFired; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        EntityInstanceId id = static_cast<EntityInstanceId>(m_spawnedEntities.size() + 1);
        m_spawnedEntities.push_back(std::move(entity));
        return id;
    }

    // 测试辅助方法
    [[nodiscard]] bool wasSoundPlayed() const { return m_soundPlayed; }
    [[nodiscard]] const ResourceLocation& lastSoundId() const { return m_lastSoundId; }
    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

    [[nodiscard]] i32 getCauldronLevel(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return -1;
        }
        return LayeredCauldronBlock::getLevel(*it->second);
    }

    [[nodiscard]] bool isEmptyCauldron(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return false;
        }
        return it->second->is(block_registry::BuildingBlocks::CAULDRON);
    }

    [[nodiscard]] bool isWaterCauldron(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return false;
        }
        return it->second->is(block_registry::BuildingBlocks::WATER_CAULDRON);
    }

    [[nodiscard]] bool isPowderSnowCauldron(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return false;
        }
        return it->second->is(block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON);
    }

    [[nodiscard]] bool isLavaCauldron(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it == m_blocks.end() || it->second == nullptr) {
            return false;
        }
        return it->second->is(block_registry::BuildingBlocks::LAVA_CAULDRON);
    }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    bool m_isRaining = false;
    bool m_isThundering = false;
    bool m_isClientSide = false;
    bool m_soundPlayed = false;
    bool m_gameEventFired = false;
    ResourceLocation m_lastSoundId;
};

// ============================================================================
// 细雪炼药锅 onBlockActivated 交互测试
// ============================================================================

class PowderSnowCauldronInteractionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        fluid::FluidRegistry::instance().initialize();
        fluid::FluidTags::initialize();

        // 使用已注册的 POWDER_SNOW_CAULDRON 方块
        powderSnowCauldron_ = dynamic_cast<LayeredCauldronBlock*>(block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON);
        ASSERT_NE(powderSnowCauldron_, nullptr) << "POWDER_SNOW_CAULDRON should be a LayeredCauldronBlock";

        // 也获取水炼药锅引用用于转换测试
        waterCauldron_ = dynamic_cast<LayeredCauldronBlock*>(block_registry::BuildingBlocks::WATER_CAULDRON);
        ASSERT_NE(waterCauldron_, nullptr) << "WATER_CAULDRON should be a LayeredCauldronBlock";

        world_.setClientSide(false); // 服务端才能执行交互逻辑
    }

    /// 在指定位置放置指定水位的细雪炼药锅
    void placePowderSnowCauldron(const BlockPos& pos, i32 level)
    {
        const BlockState* state = &powderSnowCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), level);
        world_.setBlockAt(pos, state);
    }

    /// 在指定位置放置指定水位的水炼药锅
    void placeWaterCauldron(const BlockPos& pos, i32 level)
    {
        const BlockState* state = &waterCauldron_->defaultState().with(BlockStateProperties::LEVEL_1_3(), level);
        world_.setBlockAt(pos, state);
    }

    LayeredCauldronBlock* powderSnowCauldron_;
    LayeredCauldronBlock* waterCauldron_;
    CauldronInteractionTestWorld world_;
};

// (1) 空桶从满细雪炼药锅取细雪 → 空炼药锅 + 细雪桶
TEST_F(PowderSnowCauldronInteractionTest, EmptyBucket_ExtractsPowderSnowFromFullCauldron)
{
    if (Items::BUCKET == nullptr || Items::POWDER_SNOW_BUCKET == nullptr) {
        GTEST_SKIP() << "BUCKET or POWDER_SNOW_BUCKET not registered";
    }
    const BlockPos pos(0, 64, 0);
    placePowderSnowCauldron(pos, 3);
    ASSERT_TRUE(world_.isPowderSnowCauldron(pos));
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    Player player(EntityInstanceId(100), "TestPlayer");
    player.setWorld(&world_);
    player.abilities().creativeMode = true; // 创造模式避免物品替换逻辑
    player.getHeldItem(Hand::MainHand) = ItemStack(*Items::BUCKET, 1);

    const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = powderSnowCauldron_->onBlockActivated(*state, world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
    // 满细雪炼药锅 + 空桶 → 空炼药锅
    EXPECT_TRUE(world_.isEmptyCauldron(pos))
        << "Full powder snow cauldron with empty bucket should become empty cauldron";
    EXPECT_TRUE(world_.wasSoundPlayed());
}

// (2) 空桶从非满细雪炼药锅取细雪 → 无效果（仅满水位可取）
TEST_F(PowderSnowCauldronInteractionTest, EmptyBucket_NonFullPowderSnowCauldron_NoEffect)
{
    if (Items::BUCKET == nullptr) {
        GTEST_SKIP() << "BUCKET not registered";
    }
    const BlockPos pos(0, 64, 0);
    placePowderSnowCauldron(pos, 2);
    ASSERT_TRUE(world_.isPowderSnowCauldron(pos));

    Player player(EntityInstanceId(100), "TestPlayer");
    player.setWorld(&world_);
    player.abilities().creativeMode = true;
    player.getHeldItem(Hand::MainHand) = ItemStack(*Items::BUCKET, 1);

    const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = powderSnowCauldron_->onBlockActivated(*state, world_, pos, player, Hand::MainHand, hit);

    // 空桶交互总是返回 Success（MC Java 行为），但非满水位不改变方块
    EXPECT_EQ(result, ActionResultType::Success);
    // 非满水位的细雪炼药锅不变
    EXPECT_TRUE(world_.isPowderSnowCauldron(pos));
    EXPECT_EQ(world_.getCauldronLevel(pos), 2);
}

// (3) 细雪桶向细雪炼药锅倒细雪 → 水位满至3
TEST_F(PowderSnowCauldronInteractionTest, PowderSnowBucket_FillsPowderSnowCauldron)
{
    if (Items::POWDER_SNOW_BUCKET == nullptr) {
        GTEST_SKIP() << "POWDER_SNOW_BUCKET not registered";
    }
    const BlockPos pos(0, 64, 0);
    placePowderSnowCauldron(pos, 1);
    ASSERT_TRUE(world_.isPowderSnowCauldron(pos));
    ASSERT_EQ(world_.getCauldronLevel(pos), 1);

    Player player(EntityInstanceId(100), "TestPlayer");
    player.setWorld(&world_);
    player.abilities().creativeMode = true;
    player.getHeldItem(Hand::MainHand) = ItemStack(*Items::POWDER_SNOW_BUCKET, 1);

    const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = powderSnowCauldron_->onBlockActivated(*state, world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(world_.isPowderSnowCauldron(pos)) << "Should still be powder snow cauldron after filling";
    EXPECT_EQ(world_.getCauldronLevel(pos), 3) << "Level should be 3 after filling with powder snow bucket";
}

// (4) 细雪桶对水炼药锅无效（不能向水炼药锅倒细雪）
TEST_F(PowderSnowCauldronInteractionTest, PowderSnowBucket_OnWaterCauldron_NoFill)
{
    if (Items::POWDER_SNOW_BUCKET == nullptr) {
        GTEST_SKIP() << "POWDER_SNOW_BUCKET not registered";
    }
    const BlockPos pos(0, 64, 0);
    placeWaterCauldron(pos, 1);
    ASSERT_TRUE(world_.isWaterCauldron(pos));
    ASSERT_EQ(world_.getCauldronLevel(pos), 1);

    Player player(EntityInstanceId(100), "TestPlayer");
    player.setWorld(&world_);
    player.abilities().creativeMode = true;
    player.getHeldItem(Hand::MainHand) = ItemStack(*Items::POWDER_SNOW_BUCKET, 1);

    const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = waterCauldron_->onBlockActivated(*state, world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
    // 水炼药锅上使用细雪桶：交互已处理但不改变方块类型（仅返回 Success）
    EXPECT_TRUE(world_.isWaterCauldron(pos)) << "Water cauldron should not be affected by powder snow bucket";
    EXPECT_EQ(world_.getCauldronLevel(pos), 1) << "Water cauldron level should not change from powder snow bucket";
}

// (5) 水桶将细雪炼药锅转换为水炼药锅（水位3）
TEST_F(PowderSnowCauldronInteractionTest, WaterBucket_ConvertsPowderSnowToWaterCauldron)
{
    if (Items::WATER_BUCKET == nullptr) {
        GTEST_SKIP() << "WATER_BUCKET not registered";
    }
    const BlockPos pos(0, 64, 0);
    placePowderSnowCauldron(pos, 2);
    ASSERT_TRUE(world_.isPowderSnowCauldron(pos));

    Player player(EntityInstanceId(100), "TestPlayer");
    player.setWorld(&world_);
    player.abilities().creativeMode = true;
    player.getHeldItem(Hand::MainHand) = ItemStack(*Items::WATER_BUCKET, 1);

    const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = powderSnowCauldron_->onBlockActivated(*state, world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
    // 细雪炼药锅 + 水桶 → 水炼药锅（水位3）
    EXPECT_TRUE(world_.isWaterCauldron(pos)) << "Powder snow cauldron + water bucket should convert to water cauldron";
    EXPECT_EQ(world_.getCauldronLevel(pos), 3) << "Water cauldron should be at level 3 after conversion";
}

// (6) 岩浆桶将细雪炼药锅替换为岩浆炼药锅
TEST_F(PowderSnowCauldronInteractionTest, LavaBucket_ConvertsPowderSnowToLavaCauldron)
{
    if (Items::LAVA_BUCKET == nullptr) {
        GTEST_SKIP() << "LAVA_BUCKET not registered";
    }
    const BlockPos pos(0, 64, 0);
    placePowderSnowCauldron(pos, 2);
    ASSERT_TRUE(world_.isPowderSnowCauldron(pos));

    Player player(EntityInstanceId(100), "TestPlayer");
    player.setWorld(&world_);
    player.abilities().creativeMode = true;
    player.getHeldItem(Hand::MainHand) = ItemStack(*Items::LAVA_BUCKET, 1);

    const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = powderSnowCauldron_->onBlockActivated(*state, world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(world_.isLavaCauldron(pos)) << "Powder snow cauldron + lava bucket should convert to lava cauldron";
}

// (7) 玻璃瓶对细雪炼药锅无效（细雪炼药锅不支持瓶类交互）
TEST_F(PowderSnowCauldronInteractionTest, GlassBottle_PowderSnowCauldron_ReturnsPass)
{
    if (Items::GLASS_BOTTLE == nullptr) {
        GTEST_SKIP() << "GLASS_BOTTLE not registered";
    }
    const BlockPos pos(0, 64, 0);
    placePowderSnowCauldron(pos, 3);

    Player player(EntityInstanceId(100), "TestPlayer");
    player.setWorld(&world_);
    player.abilities().creativeMode = true;
    player.getHeldItem(Hand::MainHand) = ItemStack(*Items::GLASS_BOTTLE, 1);

    const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = powderSnowCauldron_->onBlockActivated(*state, world_, pos, player, Hand::MainHand, hit);

    // 细雪炼药锅不支持玻璃瓶交互，应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass) << "Glass bottle should not interact with powder snow cauldron";
    EXPECT_TRUE(world_.isPowderSnowCauldron(pos)) << "Powder snow cauldron should be unchanged";
    EXPECT_EQ(world_.getCauldronLevel(pos), 3);
}

// (8) 水瓶对细雪炼药锅无效
TEST_F(PowderSnowCauldronInteractionTest, WaterBottle_PowderSnowCauldron_ReturnsPass)
{
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION not registered";
    }
    const BlockPos pos(0, 64, 0);
    placePowderSnowCauldron(pos, 2);

    Player player(EntityInstanceId(100), "TestPlayer");
    player.setWorld(&world_);
    player.abilities().creativeMode = true;
    // 创建水瓶
    ItemStack waterBottle = potion::PotionUtils::createPotionItem(potion::Potions::WATER);
    player.getHeldItem(Hand::MainHand) = waterBottle;

    const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = powderSnowCauldron_->onBlockActivated(*state, world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Pass) << "Water bottle should not interact with powder snow cauldron";
    EXPECT_TRUE(world_.isPowderSnowCauldron(pos)) << "Powder snow cauldron should be unchanged";
    EXPECT_EQ(world_.getCauldronLevel(pos), 2);
}

// (9) 空手对细雪炼药锅无交互
TEST_F(PowderSnowCauldronInteractionTest, EmptyHand_PowderSnowCauldron_ReturnsPass)
{
    const BlockPos pos(0, 64, 0);
    placePowderSnowCauldron(pos, 3);

    Player player(EntityInstanceId(100), "TestPlayer");
    player.setWorld(&world_);
    // 空手
    player.getHeldItem(Hand::MainHand) = ItemStack();

    const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = powderSnowCauldron_->onBlockActivated(*state, world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_TRUE(world_.isPowderSnowCauldron(pos));
    EXPECT_EQ(world_.getCauldronLevel(pos), 3);
}

// (10) 细雪桶对满细雪炼药锅无效（水位已是3）
TEST_F(PowderSnowCauldronInteractionTest, PowderSnowBucket_FullPowderSnowCauldron_NoEffect)
{
    if (Items::POWDER_SNOW_BUCKET == nullptr) {
        GTEST_SKIP() << "POWDER_SNOW_BUCKET not registered";
    }
    const BlockPos pos(0, 64, 0);
    placePowderSnowCauldron(pos, 3);
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    Player player(EntityInstanceId(100), "TestPlayer");
    player.setWorld(&world_);
    player.abilities().creativeMode = true;
    player.getHeldItem(Hand::MainHand) = ItemStack(*Items::POWDER_SNOW_BUCKET, 1);

    const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = powderSnowCauldron_->onBlockActivated(*state, world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
    // 满的细雪炼药锅不再增加水位
    EXPECT_TRUE(world_.isPowderSnowCauldron(pos));
    EXPECT_EQ(world_.getCauldronLevel(pos), 3) << "Full powder snow cauldron should not change level";
}

// (11) 水桶对满水炼药锅无效（水位已是3）- 对照测试
TEST_F(PowderSnowCauldronInteractionTest, WaterBucket_FullWaterCauldron_NoEffect)
{
    if (Items::WATER_BUCKET == nullptr) {
        GTEST_SKIP() << "WATER_BUCKET not registered";
    }
    const BlockPos pos(0, 64, 0);
    placeWaterCauldron(pos, 3);
    ASSERT_TRUE(world_.isWaterCauldron(pos));
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    Player player(EntityInstanceId(100), "TestPlayer");
    player.setWorld(&world_);
    player.abilities().creativeMode = true;
    player.getHeldItem(Hand::MainHand) = ItemStack(*Items::WATER_BUCKET, 1);

    const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = waterCauldron_->onBlockActivated(*state, world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(world_.isWaterCauldron(pos));
    EXPECT_EQ(world_.getCauldronLevel(pos), 3) << "Full water cauldron should not change level from water bucket";
}

// (12) 水桶对水位未满的水炼药锅装满到水位3 - 对照测试
TEST_F(PowderSnowCauldronInteractionTest, WaterBucket_FillsWaterCauldronToLevel3)
{
    if (Items::WATER_BUCKET == nullptr) {
        GTEST_SKIP() << "WATER_BUCKET not registered";
    }
    const BlockPos pos(0, 64, 0);
    placeWaterCauldron(pos, 1);
    ASSERT_TRUE(world_.isWaterCauldron(pos));
    ASSERT_EQ(world_.getCauldronLevel(pos), 1);

    Player player(EntityInstanceId(100), "TestPlayer");
    player.setWorld(&world_);
    player.abilities().creativeMode = true;
    player.getHeldItem(Hand::MainHand) = ItemStack(*Items::WATER_BUCKET, 1);

    const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = waterCauldron_->onBlockActivated(*state, world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(world_.isWaterCauldron(pos));
    EXPECT_EQ(world_.getCauldronLevel(pos), 3) << "Water bucket should fill water cauldron to level 3";
}

// (13) 空桶从满水炼药锅取水 → 空炼药锅 + 水桶 - 对照测试
TEST_F(PowderSnowCauldronInteractionTest, EmptyBucket_ExtractsWaterFromFullWaterCauldron)
{
    if (Items::BUCKET == nullptr || Items::WATER_BUCKET == nullptr) {
        GTEST_SKIP() << "BUCKET or WATER_BUCKET not registered";
    }
    const BlockPos pos(0, 64, 0);
    placeWaterCauldron(pos, 3);
    ASSERT_TRUE(world_.isWaterCauldron(pos));
    ASSERT_EQ(world_.getCauldronLevel(pos), 3);

    Player player(EntityInstanceId(100), "TestPlayer");
    player.setWorld(&world_);
    player.abilities().creativeMode = true;
    player.getHeldItem(Hand::MainHand) = ItemStack(*Items::BUCKET, 1);

    const BlockState* state = world_.getBlockState(pos.x, pos.y, pos.z);
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = waterCauldron_->onBlockActivated(*state, world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(world_.isEmptyCauldron(pos)) << "Full water cauldron with empty bucket should become empty cauldron";
    EXPECT_TRUE(world_.wasSoundPlayed());
}
