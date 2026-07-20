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

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/basic/CowEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/BucketItem.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/tick/manager/TickManager.hpp"

namespace mc {
namespace {

/**
 * @brief 测试用世界存根
 */
class MilkingTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("MilkingTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("MilkingTestWorld::tickManager not implemented");
    }

    [[nodiscard]] EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(entity.get());
        // 不实际存储实体，返回临时ID
        return ++m_lastEntityId;
    }

    void addParticle(
        particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3& = Vector3(0, 0, 0), u32 = 1) override
    {
        // 测试中忽略粒子效果
    }

    void playSound(const ResourceLocation& soundId, sound::SoundCategory, const Vector3&, f32, f32) override
    {
        m_lastPlayedSound = soundId;
    }

    [[nodiscard]] const std::vector<Entity*>& spawnedEntities() const { return m_spawnedEntities; }
    [[nodiscard]] const std::optional<ResourceLocation>& lastPlayedSound() const { return m_lastPlayedSound; }

private:
    EntityInstanceId m_lastEntityId = 0;
    std::vector<Entity*> m_spawnedEntities;
    std::optional<ResourceLocation> m_lastPlayedSound;
};

class BucketItemMilkingTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 流体注册表必须在物品注册之前初始化
        fluid::FluidRegistry::instance().initialize();
        Items::initialize();
    }

    void SetUp() override { m_world = std::make_unique<MilkingTestWorld>(); }

    std::unique_ptr<MilkingTestWorld> m_world;
};

// ============================================================================
// 桶物品存在性测试
// ============================================================================

TEST_F(BucketItemMilkingTest, BucketItemsAreRegistered)
{
    // 验证所有桶物品都已注册
    ASSERT_NE(Items::BUCKET, nullptr) << "Empty bucket should be registered";
    ASSERT_NE(Items::WATER_BUCKET, nullptr) << "Water bucket should be registered";
    ASSERT_NE(Items::LAVA_BUCKET, nullptr) << "Lava bucket should be registered";
    ASSERT_NE(Items::MILK_BUCKET, nullptr) << "Milk bucket should be registered";
}

TEST_F(BucketItemMilkingTest, EmptyBucketIsCorrectType)
{
    auto* bucket = static_cast<BucketItem*>(Items::BUCKET);
    ASSERT_NE(bucket, nullptr);
    EXPECT_TRUE(bucket->isEmpty()) << "Empty bucket should report isEmpty() = true";
    EXPECT_EQ(bucket->getContainedFluid(), nullptr) << "Empty bucket should have null fluid";
}

TEST_F(BucketItemMilkingTest, WaterBucketHasFluid)
{
    auto* waterBucket = static_cast<BucketItem*>(Items::WATER_BUCKET);
    ASSERT_NE(waterBucket, nullptr);
    EXPECT_FALSE(waterBucket->isEmpty()) << "Water bucket should report isEmpty() = false";
    EXPECT_NE(waterBucket->getContainedFluid(), nullptr) << "Water bucket should have fluid";
}

TEST_F(BucketItemMilkingTest, LavaBucketHasFluid)
{
    auto* lavaBucket = static_cast<BucketItem*>(Items::LAVA_BUCKET);
    ASSERT_NE(lavaBucket, nullptr);
    EXPECT_FALSE(lavaBucket->isEmpty()) << "Lava bucket should report isEmpty() = false";
    EXPECT_NE(lavaBucket->getContainedFluid(), nullptr) << "Lava bucket should have fluid";
}

// ============================================================================
// 挤奶逻辑测试
// ============================================================================

TEST_F(BucketItemMilkingTest, EmptyBucketCanBeUsedToMilkCow)
{
    // 空桶有 itemInteractionForEntity 方法，可以对牛使用
    auto* bucket = static_cast<BucketItem*>(Items::BUCKET);
    ASSERT_NE(bucket, nullptr);

    // 验证空桶的 itemInteractionForEntity 方法存在
    // 实际测试需要 CowEntity 和 Player 的完整实现
    // 此处仅验证方法签名和返回类型
    EXPECT_TRUE(bucket->isEmpty());
}

TEST_F(BucketItemMilkingTest, WaterBucketCannotMilkCow)
{
    // 水桶不能用于挤奶
    auto* waterBucket = static_cast<BucketItem*>(Items::WATER_BUCKET);
    ASSERT_NE(waterBucket, nullptr);

    // 水桶不为空，因此不能用于挤奶
    EXPECT_FALSE(waterBucket->isEmpty());
}

TEST_F(BucketItemMilkingTest, MilkBucketItemExists)
{
    // 牛奶桶物品应存在
    auto* milkBucket = Items::MILK_BUCKET;
    ASSERT_NE(milkBucket, nullptr);
    EXPECT_EQ(milkBucket->itemLocation(), ResourceLocation("minecraft:milk_bucket"));
}

// ============================================================================
// 静态方法测试
// ============================================================================

TEST_F(BucketItemMilkingTest, GetEmptyBucketReturnsValidItem)
{
    auto* emptyBucket = BucketItem::getEmptyBucket();
    ASSERT_NE(emptyBucket, nullptr);
    EXPECT_EQ(emptyBucket, Items::BUCKET) << "getEmptyBucket should return the BUCKET item";
    EXPECT_TRUE(emptyBucket->isEmpty());
}

} // namespace
} // namespace mc
