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
#include "common/entity/entities/passive/fish/CodEntity.hpp"
#include "common/entity/entities/passive/fish/PufferfishEntity.hpp"
#include "common/entity/entities/passive/fish/SalmonEntity.hpp"
#include "common/entity/entities/passive/fish/TropicalFishEntity.hpp"
#include "common/entity/entities/passive/water/AxolotlEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/interfaces/BucketableUtils.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/FishBucketItem.hpp"
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
class FishBucketTestWorld final : public mc::test::BaseTestWorld {
public:
    bool setBlockState(i32, i32, i32, const BlockState*) override { return true; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("FishBucketTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("FishBucketTestWorld::tickManager not implemented");
    }

    [[nodiscard]] EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        // 保存鱼实体指针用于验证
        if (entity != nullptr) {
            m_spawnedFishCount++;
            // 检查是否设置了 FromBucket
            auto* fish = dynamic_cast<AbstractFishEntity*>(entity.get());
            if (fish != nullptr) {
                m_lastSpawnedFishFromBucket = fish->isFromBucket();
            }
            // 保留生成的实体以支持放鱼后状态断言（放鱼数据恢复测试）。
            m_lastSpawnedEntity = std::move(entity);
        }
        return ++m_lastEntityId;
    }

    void addParticle(
        particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3& = Vector3(0, 0, 0), u32 = 1) override
    {
        // 测试中忽略粒子效果
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override
    {
        // 测试中忽略音效
    }

    [[nodiscard]] i32 spawnedFishCount() const { return m_spawnedFishCount; }
    [[nodiscard]] std::optional<bool> lastSpawnedFishFromBucket() const { return m_lastSpawnedFishFromBucket; }
    [[nodiscard]] Entity* lastSpawnedEntity() const { return m_lastSpawnedEntity.get(); }

    void reset()
    {
        m_spawnedFishCount = 0;
        m_lastSpawnedFishFromBucket = std::nullopt;
    }

private:
    EntityInstanceId m_lastEntityId = 0;
    i32 m_spawnedFishCount = 0;
    std::optional<bool> m_lastSpawnedFishFromBucket;
    std::unique_ptr<Entity> m_lastSpawnedEntity;
};

class FishBucketItemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 流体注册表必须在物品注册之前初始化
        fluid::FluidRegistry::instance().initialize();
        Items::initialize();
        // 放鱼链路（FishBucketItem::_spawnFish → EntityRegistry::getType）须实体类型已注册
        entity::VanillaEntities::registerAll();
    }

    void SetUp() override { m_world = std::make_unique<FishBucketTestWorld>(); }

    std::unique_ptr<FishBucketTestWorld> m_world;
};

// ============================================================================
// 鱼桶物品注册测试
// ============================================================================

TEST_F(FishBucketItemTest, FishBucketItemsAreRegistered)
{
    ASSERT_NE(Items::COD_BUCKET, nullptr) << "Cod bucket should be registered";
    ASSERT_NE(Items::SALMON_BUCKET, nullptr) << "Salmon bucket should be registered";
    ASSERT_NE(Items::PUFFERFISH_BUCKET, nullptr) << "Pufferfish bucket should be registered";
    ASSERT_NE(Items::TROPICAL_FISH_BUCKET, nullptr) << "Tropical fish bucket should be registered";
}

TEST_F(FishBucketItemTest, CodBucketItemLocation)
{
    ASSERT_NE(Items::COD_BUCKET, nullptr);
    EXPECT_EQ(Items::COD_BUCKET->itemLocation(), ResourceLocation("minecraft:cod_bucket"));
}

TEST_F(FishBucketItemTest, SalmonBucketItemLocation)
{
    ASSERT_NE(Items::SALMON_BUCKET, nullptr);
    EXPECT_EQ(Items::SALMON_BUCKET->itemLocation(), ResourceLocation("minecraft:salmon_bucket"));
}

TEST_F(FishBucketItemTest, PufferfishBucketItemLocation)
{
    ASSERT_NE(Items::PUFFERFISH_BUCKET, nullptr);
    EXPECT_EQ(Items::PUFFERFISH_BUCKET->itemLocation(), ResourceLocation("minecraft:pufferfish_bucket"));
}

TEST_F(FishBucketItemTest, TropicalFishBucketItemLocation)
{
    ASSERT_NE(Items::TROPICAL_FISH_BUCKET, nullptr);
    EXPECT_EQ(Items::TROPICAL_FISH_BUCKET->itemLocation(), ResourceLocation("minecraft:tropical_fish_bucket"));
}

// ============================================================================
// FromBucket 标签测试（在实体上）
// ============================================================================

TEST_F(FishBucketItemTest, FishFromBucketPreventsDespawn)
{
    // 创建鳕鱼并验证 FromBucket 标签影响消失行为
    CodEntity cod(EntityInstanceId(1), mc::test::testEcsRegistry());
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

TEST_F(FishBucketItemTest, FromBucketFishCannotDespawn)
{
    CodEntity cod(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 默认情况下，鱼可以消失（没有自定义名称）
    EXPECT_TRUE(cod.canDespawn(128.0));

    // 设置 FromBucket 后，canDespawn 应该返回 false
    cod.setFromBucket(true);
    EXPECT_FALSE(cod.canDespawn(128.0));
    EXPECT_FALSE(cod.canDespawn(0.0)); // 即使玩家很近

    // 设置自定义名称也阻止消失
    cod.setFromBucket(false);
    cod.setCustomName("Nemo");
    EXPECT_FALSE(cod.canDespawn(128.0));
}

TEST_F(FishBucketItemTest, AllFishTypesSupportFromBucket)
{
    CodEntity cod(EntityInstanceId(1), mc::test::testEcsRegistry());
    SalmonEntity salmon(EntityInstanceId(2), mc::test::testEcsRegistry());
    PufferfishEntity pufferfish(EntityInstanceId(3), mc::test::testEcsRegistry());
    TropicalFishEntity tropicalFish(EntityInstanceId(4), mc::test::testEcsRegistry());

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

TEST_F(FishBucketItemTest, FromBucketAndRidingBothPreventDespawn)
{
    CodEntity cod(EntityInstanceId(1), mc::test::testEcsRegistry());
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

TEST_F(FishBucketItemTest, MilkBucketExists)
{
    ASSERT_NE(Items::MILK_BUCKET, nullptr) << "Milk bucket should be registered";
    EXPECT_EQ(Items::MILK_BUCKET->itemLocation(), ResourceLocation("minecraft:milk_bucket"));
}

TEST_F(FishBucketItemTest, MilkBucketHasCorrectUseDuration)
{
    // 牛奶桶饮用时间应为 32 ticks
    ItemStack milkStack(Items::MILK_BUCKET, 1);
    EXPECT_EQ(Items::MILK_BUCKET->getUseDuration(milkStack), 32);
}

TEST_F(FishBucketItemTest, EmptyBucketExists)
{
    ASSERT_NE(Items::BUCKET, nullptr) << "Empty bucket should be registered";
    EXPECT_EQ(Items::BUCKET->itemLocation(), ResourceLocation("minecraft:bucket"));
}

// ============================================================================
// 鱼桶实体数据（BucketEntityData）保存/恢复测试
//   对齐 vanilla Bucketable.saveDefaultDataToBucketTag / loadDefaultDataFromBucketTag。
// ============================================================================

TEST_F(FishBucketItemTest, SaveDefaultData_WritesBucketEntityDataChildTag)
{
    CodEntity cod(EntityInstanceId(1), mc::test::testEcsRegistry());
    cod.setHealth(2.0f); // 鳕鱼最大生命为 3，取合法值
    cod.setSilent(true);
    cod.setCustomName("Bubbles");

    ItemStack bucket(Items::COD_BUCKET, 1);
    entity::saveDefaultDataToBucketTag(cod, bucket);

    const nlohmann::json* data = bucket.getChildTag(entity::BUCKET_ENTITY_DATA_KEY);
    ASSERT_NE(data, nullptr) << "BucketEntityData child tag should be written";
    EXPECT_EQ((*data)["Health"].get<f32>(), 2.0f);
    EXPECT_TRUE((*data)["Silent"].get<bool>());
    EXPECT_EQ((*data)["CustomName"].get<std::string>(), "Bubbles");
    // 未被设置的状态不应写入（对齐 vanilla 仅写非默认字段）
    EXPECT_FALSE(data->contains("NoAI"));
    EXPECT_FALSE(data->contains("NoGravity"));
}

TEST_F(FishBucketItemTest, LoadDefaultData_RestoresEntityState)
{
    // 构造桶数据并恢复到一个默认状态的新实体。
    nlohmann::json data = nlohmann::json::object();
    data["Health"] = 1.5f; // 鳕鱼最大生命为 3，取合法值
    data["Silent"] = true;
    data["NoAI"] = true;
    data["CustomName"] = "Fishy";

    CodEntity cod(EntityInstanceId(2), mc::test::testEcsRegistry());
    cod.setHealth(cod.maxHealth());
    entity::loadDefaultDataFromBucketTag(cod, data);

    EXPECT_EQ(cod.health(), 1.5f);
    EXPECT_TRUE(cod.isSilent());
    EXPECT_TRUE(cod.isNoAi());
    EXPECT_EQ(cod.customNameText(), "Fishy");
}

TEST_F(FishBucketItemTest, LoadDefaultData_EmptyTag_KeepsDefaults)
{
    CodEntity cod(EntityInstanceId(3), mc::test::testEcsRegistry());
    cod.setHealth(cod.maxHealth());
    const nlohmann::json empty = nlohmann::json::object();
    entity::loadDefaultDataFromBucketTag(cod, empty);

    EXPECT_EQ(cod.health(), cod.maxHealth());
    EXPECT_FALSE(cod.isSilent());
    EXPECT_FALSE(cod.isNoAi());
}

TEST_F(FishBucketItemTest, SaveToBucketTag_AbstractFish_WritesDefaultData)
{
    CodEntity cod(EntityInstanceId(4), mc::test::testEcsRegistry());
    cod.setHealth(2.5f); // 鳕鱼最大生命为 3，取合法值

    ItemStack bucket(Items::COD_BUCKET, 1);
    cod.saveToBucketTag(bucket);

    const nlohmann::json* data = bucket.getChildTag(entity::BUCKET_ENTITY_DATA_KEY);
    ASSERT_NE(data, nullptr);
    EXPECT_EQ((*data)["Health"].get<f32>(), 2.5f);
}

TEST_F(FishBucketItemTest, AxolotlSaveLoad_RoundTripsVariantAndAge)
{
    // 美西螈桶须持久化 Variant/Age（对齐 vanilla Axolotl.saveToBucketTag/loadFromBucketTag）。
    AxolotlEntity adult(EntityInstanceId(10), mc::test::testEcsRegistry());
    adult.setVariant(AxolotlVariant::Blue);
    adult.setChild(true);

    ItemStack bucket(Items::AXOLOTL_BUCKET, 1);
    adult.saveToBucketTag(bucket);

    const nlohmann::json* data = bucket.getChildTag(entity::BUCKET_ENTITY_DATA_KEY);
    ASSERT_NE(data, nullptr);
    EXPECT_EQ((*data)["Variant"].get<i32>(), static_cast<i32>(AxolotlVariant::Blue));
    EXPECT_EQ((*data)["Age"].get<i32>(), adult.getGrowingAge());

    // 恢复到另一个默认状态的美西螈。
    AxolotlEntity restored(EntityInstanceId(11), mc::test::testEcsRegistry());
    ASSERT_FALSE(restored.isChild());
    ASSERT_EQ(restored.getVariant(), AxolotlVariant::Lucy);
    restored.loadFromBucketTag(*data);

    EXPECT_EQ(restored.getVariant(), AxolotlVariant::Blue);
    EXPECT_TRUE(restored.isChild());
    EXPECT_EQ(restored.getGrowingAge(), adult.getGrowingAge());
}

TEST_F(FishBucketItemTest, ReleaseFish_RestoresBucketEntityData)
{
    // 端到端：携带 BucketEntityData 的鳕鱼桶放鱼后，新生成的鱼须恢复 Health/Silent/
    // 自定义名（对齐 vanilla MobBucketItem.spawn → bucketable.loadFromBucketTag）。
    ASSERT_NE(Items::COD_BUCKET, nullptr);
    auto* codBucket = dynamic_cast<item::FishBucketItem*>(Items::COD_BUCKET);
    ASSERT_NE(codBucket, nullptr);

    // 构造带 BucketEntityData 的鱼桶：生命 2、静音、命名 "Saved"。
    ItemStack bucket(Items::COD_BUCKET, 1);
    nlohmann::json& data = bucket.getOrCreateChildTag(entity::BUCKET_ENTITY_DATA_KEY);
    data["Health"] = 2.0f;
    data["Silent"] = true;
    data["CustomName"] = "Saved";

    Player player(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(m_world.get());
    player.setTypeId("minecraft:player");
    player.setInWater(true);
    player.inventory().setItem(0, bucket);
    player.inventory().setSelectedSlot(0);

    const ItemActionResult result = codBucket->onItemRightClick(*m_world, player, Hand::MainHand);
    EXPECT_TRUE(result.isSuccess());

    Entity* spawned = m_world->lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr) << "A cod should have been spawned from the bucket";
    auto* spawnedLiving = dynamic_cast<LivingEntity*>(spawned);
    ASSERT_NE(spawnedLiving, nullptr);
    EXPECT_EQ(spawnedLiving->health(), 2.0f) << "Health should be restored from BucketEntityData";
    EXPECT_TRUE(spawned->isSilent()) << "Silent flag should be restored from BucketEntityData";
    EXPECT_EQ(spawned->customNameText(), "Saved") << "Custom name should be restored from BucketEntityData";
    auto* fish = dynamic_cast<AbstractFishEntity*>(spawned);
    ASSERT_NE(fish, nullptr);
    EXPECT_TRUE(fish->isFromBucket());
}

} // namespace
} // namespace mc
