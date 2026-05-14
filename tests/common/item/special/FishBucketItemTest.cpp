#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/item/items/special/FishBucketItem.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/entity/entities/passive/fish/CodEntity.hpp"
#include "common/entity/entities/passive/fish/SalmonEntity.hpp"
#include "common/entity/entities/passive/fish/PufferfishEntity.hpp"
#include "common/entity/entities/passive/fish/TropicalFishEntity.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/core/Constants.hpp"
#include "common/TestWorldHelper.hpp"

namespace mc {
namespace {

/**
 * @brief 测试用世界存根
 */
class FishBucketTestWorld final : public test::BaseTestWorld {
public:
    bool setBlockState(i32, i32, i32, const BlockState*) override { return true; }

    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("FishBucketTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("FishBucketTestWorld::tickManager not implemented");
    }

    [[nodiscard]] EntityId spawnEntity(std::unique_ptr<Entity> entity) override {
        // 保存鱼实体指针用于验证
        if (entity != nullptr) {
            m_spawnedFishCount++;
            // 检查是否设置了 FromBucket
            auto* fish = dynamic_cast<AbstractFishEntity*>(entity.get());
            if (fish != nullptr) {
                m_lastSpawnedFishFromBucket = fish->isFromBucket();
            }
        }
        return ++m_lastEntityId;
    }

    void addParticle(client::renderer::trident::particle::ParticleTypeId,
                     const Vector3&,
                     const Vector3&,
                     const Vector3& = Vector3(0, 0, 0),
                     u32 = 1) override {
        // 测试中忽略粒子效果
    }

    void playSound(const ResourceLocation&,
                   sound::SoundCategory,
                   const Vector3&,
                   f32,
                   f32) override {
        // 测试中忽略音效
    }

    [[nodiscard]] i32 spawnedFishCount() const { return m_spawnedFishCount; }
    [[nodiscard]] std::optional<bool> lastSpawnedFishFromBucket() const { return m_lastSpawnedFishFromBucket; }

    void reset() {
        m_spawnedFishCount = 0;
        m_lastSpawnedFishFromBucket = std::nullopt;
    }

private:
    EntityId m_lastEntityId = 0;
    i32 m_spawnedFishCount = 0;
    std::optional<bool> m_lastSpawnedFishFromBucket;
};

class FishBucketItemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // 流体注册表必须在物品注册之前初始化
        fluid::FluidRegistry::instance().initialize();
        Items::initialize();
    }

    void SetUp() override {
        m_world = std::make_unique<FishBucketTestWorld>();
    }

    std::unique_ptr<FishBucketTestWorld> m_world;
};

// ============================================================================
// 鱼桶物品注册测试
// ============================================================================

TEST_F(FishBucketItemTest, FishBucketItemsAreRegistered) {
    ASSERT_NE(Items::COD_BUCKET, nullptr) << "Cod bucket should be registered";
    ASSERT_NE(Items::SALMON_BUCKET, nullptr) << "Salmon bucket should be registered";
    ASSERT_NE(Items::PUFFERFISH_BUCKET, nullptr) << "Pufferfish bucket should be registered";
    ASSERT_NE(Items::TROPICAL_FISH_BUCKET, nullptr) << "Tropical fish bucket should be registered";
}

TEST_F(FishBucketItemTest, CodBucketItemLocation) {
    ASSERT_NE(Items::COD_BUCKET, nullptr);
    EXPECT_EQ(Items::COD_BUCKET->itemLocation(), ResourceLocation("minecraft:cod_bucket"));
}

TEST_F(FishBucketItemTest, SalmonBucketItemLocation) {
    ASSERT_NE(Items::SALMON_BUCKET, nullptr);
    EXPECT_EQ(Items::SALMON_BUCKET->itemLocation(), ResourceLocation("minecraft:salmon_bucket"));
}

TEST_F(FishBucketItemTest, PufferfishBucketItemLocation) {
    ASSERT_NE(Items::PUFFERFISH_BUCKET, nullptr);
    EXPECT_EQ(Items::PUFFERFISH_BUCKET->itemLocation(), ResourceLocation("minecraft:pufferfish_bucket"));
}

TEST_F(FishBucketItemTest, TropicalFishBucketItemLocation) {
    ASSERT_NE(Items::TROPICAL_FISH_BUCKET, nullptr);
    EXPECT_EQ(Items::TROPICAL_FISH_BUCKET->itemLocation(), ResourceLocation("minecraft:tropical_fish_bucket"));
}

// ============================================================================
// FromBucket 标签测试（在实体上）
// ============================================================================

TEST_F(FishBucketItemTest, FishFromBucketPreventsDespawn) {
    // 创建鳕鱼并验证 FromBucket 标签影响消失行为
    CodEntity cod(LegacyEntityType::Cod, EntityId(1));
    cod.setWorld(m_world.get());

    // 默认情况下不是从桶放出的
    EXPECT_FALSE(cod.isFromBucket());

    // 设置 FromBucket 标签
    cod.setFromBucket(true);
    EXPECT_TRUE(cod.isFromBucket());

    // 从桶放出的鱼不应该消失
    EXPECT_TRUE(cod.preventDespawn());
    EXPECT_FALSE(cod.canDespawn(128.0));
}

TEST_F(FishBucketItemTest, FromBucketFishCannotDespawn) {
    CodEntity cod(LegacyEntityType::Cod, EntityId(1));

    // 默认情况下，鱼可以消失（没有自定义名称）
    EXPECT_TRUE(cod.canDespawn(128.0));

    // 设置 FromBucket 后，canDespawn 应该返回 false
    cod.setFromBucket(true);
    EXPECT_FALSE(cod.canDespawn(128.0));
    EXPECT_FALSE(cod.canDespawn(0.0));  // 即使玩家很近

    // 设置自定义名称也阻止消失
    cod.setFromBucket(false);
    cod.setCustomName("Nemo");
    EXPECT_FALSE(cod.canDespawn(128.0));
}

TEST_F(FishBucketItemTest, AllFishTypesSupportFromBucket) {
    CodEntity cod(LegacyEntityType::Cod, EntityId(1));
    SalmonEntity salmon(LegacyEntityType::Salmon, EntityId(2));
    PufferfishEntity pufferfish(LegacyEntityType::Pufferfish, EntityId(3));
    TropicalFishEntity tropicalFish(LegacyEntityType::TropicalFish, EntityId(4));

    // 所有鱼类默认不是从桶放出的
    EXPECT_FALSE(cod.isFromBucket());
    EXPECT_FALSE(salmon.isFromBucket());
    EXPECT_FALSE(pufferfish.isFromBucket());
    EXPECT_FALSE(tropicalFish.isFromBucket());

    // 设置后都能正确响应
    cod.setFromBucket(true);
    salmon.setFromBucket(true);
    pufferfish.setFromBucket(true);
    tropicalFish.setFromBucket(true);

    EXPECT_TRUE(cod.isFromBucket());
    EXPECT_TRUE(salmon.isFromBucket());
    EXPECT_TRUE(pufferfish.isFromBucket());
    EXPECT_TRUE(tropicalFish.isFromBucket());
}

TEST_F(FishBucketItemTest, FromBucketAndRidingBothPreventDespawn) {
    CodEntity cod(LegacyEntityType::Cod, EntityId(1));
    cod.setWorld(m_world.get());

    // 默认情况下不在骑乘状态，FromBucket 也是 false
    EXPECT_FALSE(cod.preventDespawn());

    // 设置 FromBucket 后阻止消失
    cod.setFromBucket(true);
    EXPECT_TRUE(cod.preventDespawn());

    // 取消 FromBucket 后，如果正在骑乘也应该阻止消失
    cod.setFromBucket(false);
    // 注意：骑乘状态需要通过其他方式设置，这里只测试 FromBucket 的逻辑
    EXPECT_FALSE(cod.preventDespawn());
}

// ============================================================================
// 牛奶桶测试
// ============================================================================

TEST_F(FishBucketItemTest, MilkBucketExists) {
    ASSERT_NE(Items::MILK_BUCKET, nullptr) << "Milk bucket should be registered";
    EXPECT_EQ(Items::MILK_BUCKET->itemLocation(), ResourceLocation("minecraft:milk_bucket"));
}

TEST_F(FishBucketItemTest, MilkBucketHasCorrectUseDuration) {
    // 牛奶桶饮用时间应为 32 ticks
    ItemStack milkStack(Items::MILK_BUCKET, 1);
    EXPECT_EQ(Items::MILK_BUCKET->getUseDuration(milkStack), 32);
}

TEST_F(FishBucketItemTest, EmptyBucketExists) {
    ASSERT_NE(Items::BUCKET, nullptr) << "Empty bucket should be registered";
    EXPECT_EQ(Items::BUCKET->itemLocation(), ResourceLocation("minecraft:bucket"));
}

} // namespace
} // namespace mc
