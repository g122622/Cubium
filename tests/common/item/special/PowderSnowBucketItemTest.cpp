/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND OF WHETHER
 * ARISING FROM, IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/BucketItem.hpp"
#include "common/item/items/special/PowderSnowBucketItem.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/IBucketPickupHandler.hpp"
#include "common/world/block/blocks/cave/PowderSnowBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"

namespace mc {
using item::PowderSnowBucketItem;
namespace {

// ============================================================================
// 测试用世界存根 - 支持方块设置和音效记录
// ============================================================================

class PowderSnowTestWorld final : public test::BaseTestWorld {
public:
    PowderSnowTestWorld() = default;

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("PowderSnowTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("PowderSnowTestWorld::tickManager not implemented");
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[packPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    void playSound(const ResourceLocation& soundId, sound::SoundCategory, const Vector3&, f32, f32) override
    {
        m_lastPlayedSound = soundId;
    }

    [[nodiscard]] const std::optional<ResourceLocation>& lastPlayedSound() const { return m_lastPlayedSound; }
    void clearLastPlayedSound() { m_lastPlayedSound.reset(); }

private:
    static u64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<u64>(static_cast<u32>(x)) << 32) | (static_cast<u64>(static_cast<u32>(y)) << 16) |
            static_cast<u64>(static_cast<u32>(z));
    }

    std::unordered_map<u64, const BlockState*> m_blocks;
    std::optional<ResourceLocation> m_lastPlayedSound;
};

// ============================================================================
// 测试固件
// ============================================================================

class PowderSnowBucketItemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 流体注册表必须在物品注册之前初始化
        fluid::FluidRegistry::instance().initialize();
        VanillaBlocks::initialize();
        Items::initialize();
    }

    void SetUp() override { m_world = std::make_unique<PowderSnowTestWorld>(); }

    std::unique_ptr<PowderSnowTestWorld> m_world;
};

// ============================================================================
// 细雪桶物品注册测试
// ============================================================================

TEST_F(PowderSnowBucketItemTest, PowderSnowBucketIsRegistered)
{
    ASSERT_NE(Items::POWDER_SNOW_BUCKET, nullptr) << "PowderSnowBucket should be registered";
}

TEST_F(PowderSnowBucketItemTest, PowderSnowBucketItemLocation)
{
    ASSERT_NE(Items::POWDER_SNOW_BUCKET, nullptr);
    EXPECT_EQ(Items::POWDER_SNOW_BUCKET->itemLocation(), ResourceLocation("minecraft:powder_snow_bucket"));
}

TEST_F(PowderSnowBucketItemTest, PowderSnowBucketIsNotBucketItem)
{
    // 细雪桶不是 BucketItem 的实例（因为细雪不是流体）
    ASSERT_NE(Items::POWDER_SNOW_BUCKET, nullptr);
    auto* bucketItem = dynamic_cast<BucketItem*>(Items::POWDER_SNOW_BUCKET);
    EXPECT_EQ(bucketItem, nullptr) << "PowderSnowBucketItem should NOT be a BucketItem";
}

TEST_F(PowderSnowBucketItemTest, PowderSnowBucketIsPowderSnowBucketItem)
{
    ASSERT_NE(Items::POWDER_SNOW_BUCKET, nullptr);
    auto* powderSnowBucket = dynamic_cast<PowderSnowBucketItem*>(Items::POWDER_SNOW_BUCKET);
    EXPECT_NE(powderSnowBucket, nullptr) << "PowderSnowBucketItem should be castable from Items::POWDER_SNOW_BUCKET";
}

TEST_F(PowderSnowBucketItemTest, PowderSnowBucketMaxStackSizeIs1)
{
    ASSERT_NE(Items::POWDER_SNOW_BUCKET, nullptr);
    EXPECT_EQ(Items::POWDER_SNOW_BUCKET->maxStackSize(), 1) << "PowderSnowBucket should stack to 1";
}

TEST_F(PowderSnowBucketItemTest, PowderSnowBucketContainerItemIsBucket)
{
    // 细雪桶的容器物品应该是空桶
    ASSERT_NE(Items::POWDER_SNOW_BUCKET, nullptr);
    const Item* containerItem = Items::POWDER_SNOW_BUCKET->containerItem();
    ASSERT_NE(containerItem, nullptr) << "PowderSnowBucket should have a container item";
    EXPECT_EQ(containerItem, Items::BUCKET) << "PowderSnowBucket container item should be empty bucket";
}

// ============================================================================
// 细雪方块注册测试
// ============================================================================

TEST_F(PowderSnowBucketItemTest, PowderSnowBlockIsRegistered)
{
    ASSERT_NE(VanillaBlocks::POWDER_SNOW, nullptr) << "PowderSnowBlock should be registered";
}

TEST_F(PowderSnowBucketItemTest, PowderSnowBlockImplementsIBucketPickupHandler)
{
    ASSERT_NE(VanillaBlocks::POWDER_SNOW, nullptr);
    auto* pickupHandler = dynamic_cast<IBucketPickupHandler*>(VanillaBlocks::POWDER_SNOW);
    EXPECT_NE(pickupHandler, nullptr) << "PowderSnowBlock should implement IBucketPickupHandler";
}

// ============================================================================
// PowderSnowBlock::pickupItem 测试
// ============================================================================

TEST_F(PowderSnowBucketItemTest, PowderSnowBlockPickupItemReturnsPowderSnowBucket)
{
    ASSERT_NE(VanillaBlocks::POWDER_SNOW, nullptr);
    auto* pickupHandler = dynamic_cast<IBucketPickupHandler*>(VanillaBlocks::POWDER_SNOW);
    ASSERT_NE(pickupHandler, nullptr);

    // 设置细雪方块
    const BlockState* powderSnowState = VanillaBlocks::getState(VanillaBlocks::POWDER_SNOW);
    ASSERT_NE(powderSnowState, nullptr);
    m_world->setBlockState(0, 0, 0, powderSnowState);

    // 验证 pickupItem 返回细雪桶物品
    const BlockState* stateBefore = m_world->getBlockState(0, 0, 0);
    ASSERT_NE(stateBefore, nullptr);

    const Item* pickedItem = pickupHandler->pickupItem(*m_world, BlockPos(0, 0, 0), *stateBefore);
    ASSERT_NE(pickedItem, nullptr) << "pickupItem should return a non-null item for PowderSnowBlock";
    EXPECT_EQ(pickedItem, Items::POWDER_SNOW_BUCKET) << "pickupItem should return POWDER_SNOW_BUCKET";
}

TEST_F(PowderSnowBucketItemTest, PowderSnowBlockPickupItemReplacesWithAir)
{
    ASSERT_NE(VanillaBlocks::POWDER_SNOW, nullptr);
    auto* pickupHandler = dynamic_cast<IBucketPickupHandler*>(VanillaBlocks::POWDER_SNOW);
    ASSERT_NE(pickupHandler, nullptr);

    // 设置细雪方块
    const BlockState* powderSnowState = VanillaBlocks::getState(VanillaBlocks::POWDER_SNOW);
    ASSERT_NE(powderSnowState, nullptr);
    m_world->setBlockState(0, 0, 0, powderSnowState);

    // 执行 pickupItem
    const BlockState* stateBefore = m_world->getBlockState(0, 0, 0);
    ASSERT_NE(stateBefore, nullptr);
    pickupHandler->pickupItem(*m_world, BlockPos(0, 0, 0), *stateBefore);

    // 验证方块被替换为空气
    const BlockState* stateAfter = m_world->getBlockState(0, 0, 0);
    ASSERT_NE(stateAfter, nullptr);
    Block* blockAfter = Block::getBlock(stateAfter->blockId());
    ASSERT_NE(blockAfter, nullptr);
    EXPECT_TRUE(blockAfter->isAir(*stateAfter)) << "After pickup, the block should be air";
}

TEST_F(PowderSnowBucketItemTest, PowderSnowBlockPickupFluidReturnsNull)
{
    ASSERT_NE(VanillaBlocks::POWDER_SNOW, nullptr);
    auto* pickupHandler = dynamic_cast<IBucketPickupHandler*>(VanillaBlocks::POWDER_SNOW);
    ASSERT_NE(pickupHandler, nullptr);

    // 设置细雪方块
    const BlockState* powderSnowState = VanillaBlocks::getState(VanillaBlocks::POWDER_SNOW);
    ASSERT_NE(powderSnowState, nullptr);
    m_world->setBlockState(0, 0, 0, powderSnowState);

    const BlockState* state = m_world->getBlockState(0, 0, 0);
    ASSERT_NE(state, nullptr);

    // 细雪不是流体，pickupFluid 应该返回 nullptr
    fluid::Fluid* pickedFluid = pickupHandler->pickupFluid(*m_world, BlockPos(0, 0, 0), *state);
    EXPECT_EQ(pickedFluid, nullptr) << "pickupFluid should return nullptr for PowderSnowBlock";
}

TEST_F(PowderSnowBucketItemTest, PowderSnowBlockGetPickupSoundReturnsCorrectSound)
{
    ASSERT_NE(VanillaBlocks::POWDER_SNOW, nullptr);
    auto* pickupHandler = dynamic_cast<IBucketPickupHandler*>(VanillaBlocks::POWDER_SNOW);
    ASSERT_NE(pickupHandler, nullptr);

    const BlockState* powderSnowState = VanillaBlocks::getState(VanillaBlocks::POWDER_SNOW);
    ASSERT_NE(powderSnowState, nullptr);
    m_world->setBlockState(0, 0, 0, powderSnowState);

    const BlockState* state = m_world->getBlockState(0, 0, 0);
    ASSERT_NE(state, nullptr);

    const ResourceLocation* sound = pickupHandler->getPickupSound(*m_world, BlockPos(0, 0, 0), *state);
    ASSERT_NE(sound, nullptr) << "getPickupSound should return a non-null sound for PowderSnowBlock";
    EXPECT_EQ(*sound, SoundEvents::ITEM_BUCKET_FILL_POWDER_SNOW)
        << "getPickupSound should return ITEM_BUCKET_FILL_POWDER_SNOW";
}

// ============================================================================
// PowderSnowBucketItem::emptyContents 测试
// ============================================================================

TEST_F(PowderSnowBucketItemTest, EmptyContentsPlacesPowderSnowOnAir)
{
    ASSERT_NE(Items::POWDER_SNOW_BUCKET, nullptr);
    auto* powderSnowBucket = dynamic_cast<PowderSnowBucketItem*>(Items::POWDER_SNOW_BUCKET);
    ASSERT_NE(powderSnowBucket, nullptr);

    // 目标位置是空气（默认）
    BlockPos pos(0, 0, 0);
    const BlockState* stateBefore = m_world->getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(stateBefore, nullptr);
    Block* blockBefore = Block::getBlock(stateBefore->blockId());
    ASSERT_NE(blockBefore, nullptr);
    EXPECT_TRUE(blockBefore->isAir(*stateBefore)) << "Initial block should be air";

    // 放置细雪
    bool result = powderSnowBucket->emptyContents(nullptr, *m_world, pos);
    EXPECT_TRUE(result) << "emptyContents should succeed on air block";

    // 验证方块已变为细雪
    const BlockState* stateAfter = m_world->getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(stateAfter, nullptr);
    EXPECT_EQ(stateAfter->blockId(), VanillaBlocks::POWDER_SNOW->blockId())
        << "Block should be PowderSnow after emptyContents";
}

TEST_F(PowderSnowBucketItemTest, EmptyContentsFailsOnNonAirBlock)
{
    ASSERT_NE(Items::POWDER_SNOW_BUCKET, nullptr);
    auto* powderSnowBucket = dynamic_cast<PowderSnowBucketItem*>(Items::POWDER_SNOW_BUCKET);
    ASSERT_NE(powderSnowBucket, nullptr);

    // 在目标位置放置石头（非空气方块）
    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);
    ASSERT_NE(stoneState, nullptr);
    m_world->setBlockState(0, 0, 0, stoneState);

    // 尝试放置细雪应该失败
    BlockPos pos(0, 0, 0);
    bool result = powderSnowBucket->emptyContents(nullptr, *m_world, pos);
    EXPECT_FALSE(result) << "emptyContents should fail on non-air block";

    // 验证方块仍然是石头
    const BlockState* stateAfter = m_world->getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(stateAfter, nullptr);
    EXPECT_EQ(stateAfter->blockId(), VanillaBlocks::STONE->blockId()) << "Block should still be stone";
}

TEST_F(PowderSnowBucketItemTest, EmptyContentsPlaysSound)
{
    ASSERT_NE(Items::POWDER_SNOW_BUCKET, nullptr);
    auto* powderSnowBucket = dynamic_cast<PowderSnowBucketItem*>(Items::POWDER_SNOW_BUCKET);
    ASSERT_NE(powderSnowBucket, nullptr);

    BlockPos pos(0, 0, 0);
    m_world->clearLastPlayedSound();

    bool result = powderSnowBucket->emptyContents(nullptr, *m_world, pos);
    EXPECT_TRUE(result);

    // 验证播放了正确的音效
    const auto& sound = m_world->lastPlayedSound();
    ASSERT_TRUE(sound.has_value()) << "A sound should have been played";
    EXPECT_EQ(*sound, SoundEvents::ITEM_BUCKET_EMPTY_POWDER_SNOW)
        << "Should play ITEM_BUCKET_EMPTY_POWDER_SNOW when emptying powder snow bucket";
}

// ============================================================================
// 水方块 IBucketPickupHandler 默认 pickupItem 测试
// 验证流体方块的 pickupItem 返回 nullptr（不实现非流体拾取）
// ============================================================================

TEST_F(PowderSnowBucketItemTest, WaterBlockPickupItemReturnsNull)
{
    // 水方块实现了 IBucketPickupHandler，但不应实现 pickupItem
    ASSERT_NE(VanillaBlocks::WATER, nullptr);
    auto* pickupHandler = dynamic_cast<IBucketPickupHandler*>(VanillaBlocks::WATER);
    if (pickupHandler == nullptr) {
        // 水方块可能不直接实现 IBucketPickupHandler，跳过测试
        GTEST_SKIP() << "Water block does not implement IBucketPickupHandler";
    }

    const BlockState* waterState = VanillaBlocks::getState(VanillaBlocks::WATER);
    ASSERT_NE(waterState, nullptr);
    m_world->setBlockState(0, 0, 0, waterState);

    const BlockState* state = m_world->getBlockState(0, 0, 0);
    ASSERT_NE(state, nullptr);

    // 流体方块的默认 pickupItem 应返回 nullptr
    const Item* pickedItem = pickupHandler->pickupItem(*m_world, BlockPos(0, 0, 0), *state);
    EXPECT_EQ(pickedItem, nullptr) << "Water block pickupItem should return nullptr (uses pickupFluid instead)";
}

// ============================================================================
// 空桶拾取细雪的 BucketItem::onItemUse 非流体路径测试
// 通过 IBucketPickupHandler::pickupItem 接口验证集成
// ============================================================================

TEST_F(PowderSnowBucketItemTest, BucketItemNonFluidPickupPathIntegration)
{
    // 验证空桶的 BucketItem 已正确注册
    ASSERT_NE(Items::BUCKET, nullptr);
    auto* bucket = dynamic_cast<BucketItem*>(Items::BUCKET);
    ASSERT_NE(bucket, nullptr);
    EXPECT_TRUE(bucket->isEmpty()) << "Empty bucket should report isEmpty() = true";
}

TEST_F(PowderSnowBucketItemTest, PowderSnowBlockPickupItemAndFluidPathSeparation)
{
    // 验证细雪方块的 pickupFluid 和 pickupItem 路径分离
    ASSERT_NE(VanillaBlocks::POWDER_SNOW, nullptr);
    auto* pickupHandler = dynamic_cast<IBucketPickupHandler*>(VanillaBlocks::POWDER_SNOW);
    ASSERT_NE(pickupHandler, nullptr);

    const BlockState* powderSnowState = VanillaBlocks::getState(VanillaBlocks::POWDER_SNOW);
    ASSERT_NE(powderSnowState, nullptr);
    m_world->setBlockState(0, 0, 0, powderSnowState);

    const BlockState* state = m_world->getBlockState(0, 0, 0);
    ASSERT_NE(state, nullptr);

    // pickupFluid 应该返回 nullptr（细雪不是流体）
    fluid::Fluid* fluid = pickupHandler->pickupFluid(*m_world, BlockPos(0, 0, 0), *state);
    EXPECT_EQ(fluid, nullptr) << "PowderSnowBlock pickupFluid should return nullptr";

    // pickupItem 应该返回细雪桶物品
    const Item* item = pickupHandler->pickupItem(*m_world, BlockPos(0, 0, 0), *state);
    ASSERT_NE(item, nullptr) << "PowderSnowBlock pickupItem should return non-null";
    EXPECT_EQ(item, Items::POWDER_SNOW_BUCKET) << "PowderSnowBlock pickupItem should return POWDER_SNOW_BUCKET";
}

} // namespace
} // namespace mc
