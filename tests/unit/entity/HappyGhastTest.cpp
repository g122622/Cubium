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

// 快乐恶魂食物/音效/繁殖门槛对齐 MC Java 1.21.11 测试。
//
// 原版 HappyGhast：
//   - isFood → ItemTags.HAPPY_GHAST_FOOD（雪球）；
//   - canFallInLove = false（不能进入发情状态，幼体只能由干燥恶魂方块孵化）；
//   - getAmbientSound/getHurtSound/getDeathSound 按 isBaby 分流 GHASTLING_* / HAPPY_GHAST_*；
//   - getSoundVolume：幼体 1.0，成体 4.0。
// 此前缺陷：isBreedingItem 恒 false 且三个音效方法恒 nullopt，留下三处 TODO。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/happyghast/HappyGhastEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>
#include <optional>
#include <vector>

using namespace mc;

namespace {

/// BaseTestWorld 构造函数为 protected，派生公开以供 fixture 持有。
class HappyGhastTestWorld final : public mc::test::BaseTestWorld {
public:
    HappyGhastTestWorld() = default;

    void sendPositionSyncToTrackingPlayers(EntityInstanceId id) override { syncedIds.push_back(id); }

    /// 记录服务端要求补发绝对位置快照的实体（快乐恶魂静止超时入口）。
    std::vector<EntityInstanceId> syncedIds;
};

} // namespace

class HappyGhastTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
        Items::initialize();
        // #harnesses / #happy_ghast_tempt_items 等物品标签的成员由 ItemTags::initialize()
        // 生成，Items::initialize() 只注册物品本身。缺少此调用时 TemptItemTagsRegistered
        // 是否通过取决于其它测试套件是否先初始化过物品标签（测试执行顺序依赖）。
        item::tag::ItemTags::initialize();
    }

    HappyGhastTestWorld m_world;
};

// 食物/繁殖物品来自 HAPPY_GHAST_FOOD 标签（雪球）。
TEST_F(HappyGhastTest, IsBreedingItem_UsesHappyGhastFoodTag)
{
    HappyGhastEntity ghast(EntityInstanceId(1), mc::test::testEcsRegistry());
    ghast.setWorld(&m_world);

    ASSERT_NE(Items::SNOWBALL, nullptr);
    ASSERT_NE(Items::WHEAT, nullptr);
    EXPECT_TRUE(ghast.isBreedingItem(ItemStack(Items::SNOWBALL)));
    EXPECT_FALSE(ghast.isBreedingItem(ItemStack(Items::WHEAT)));
    EXPECT_FALSE(ghast.isBreedingItem(ItemStack()));
}

// 快乐恶魂不能繁殖（对齐 vanilla canFallInLove = false）。
TEST_F(HappyGhastTest, CannotBreed)
{
    HappyGhastEntity ghast(EntityInstanceId(2), mc::test::testEcsRegistry());
    ghast.setWorld(&m_world);
    ghast.setGrowingAge(0);

    EXPECT_FALSE(ghast.canBreed());
}

// 幼体使用 GHASTLING_* 音效与 1.0 音量；成体使用 HAPPY_GHAST_* 音效与 4.0 音量。
TEST_F(HappyGhastTest, SoundsDependOnAge)
{
    HappyGhastEntity baby(EntityInstanceId(3), mc::test::testEcsRegistry());
    baby.setWorld(&m_world);
    baby.setChild(true);
    ASSERT_TRUE(baby.isChild());
    EXPECT_EQ(baby.getAmbientSound(), std::optional<ResourceLocation>(SoundEvents::GHASTLING_AMBIENT));
    EXPECT_FLOAT_EQ(baby.getSoundVolume(), 1.0f);

    HappyGhastEntity adult(EntityInstanceId(4), mc::test::testEcsRegistry());
    adult.setWorld(&m_world);
    adult.setChild(false);
    ASSERT_FALSE(adult.isChild());
    EXPECT_EQ(adult.getAmbientSound(), std::optional<ResourceLocation>(SoundEvents::HAPPY_GHAST_AMBIENT));
    EXPECT_FLOAT_EQ(adult.getSoundVolume(), 4.0f);
}

// 幼体与成体的受伤/死亡音效同样按年龄分流。
TEST_F(HappyGhastTest, HurtAndDeathSoundsDependOnAge)
{
    HappyGhastEntity baby(EntityInstanceId(5), mc::test::testEcsRegistry());
    baby.setWorld(&m_world);
    baby.setChild(true);
    auto source = DamageSources::generic();
    EXPECT_EQ(baby.getHurtSound(source), std::optional<ResourceLocation>(SoundEvents::GHASTLING_HURT));
    EXPECT_EQ(baby.getDeathSound(), std::optional<ResourceLocation>(SoundEvents::GHASTLING_DEATH));

    HappyGhastEntity adult(EntityInstanceId(6), mc::test::testEcsRegistry());
    adult.setWorld(&m_world);
    adult.setChild(false);
    EXPECT_EQ(adult.getHurtSound(source), std::optional<ResourceLocation>(SoundEvents::HAPPY_GHAST_HURT));
    EXPECT_EQ(adult.getDeathSound(), std::optional<ResourceLocation>(SoundEvents::HAPPY_GHAST_DEATH));
}

// 进入静止超时（0 → 正数）时向追踪玩家下发一次位置快照（对齐 vanilla setServerStillTimeout）。
TEST_F(HappyGhastTest, EnteringStillTimeoutSendsPositionSync)
{
    m_world.syncedIds.clear();

    HappyGhastEntity ghast(EntityInstanceId(11), mc::test::testEcsRegistry());
    ghast.setWorld(&m_world);

    ghast.setServerStillTimeoutForTest(10);
    ASSERT_EQ(m_world.syncedIds.size(), 1U) << "0 → 正数应补发一次位置快照";
    EXPECT_EQ(m_world.syncedIds.front(), ghast.id());

    // 再次设置正数（非 0 → 正数的跨越）不应重复下发。
    ghast.setServerStillTimeoutForTest(10);
    EXPECT_EQ(m_world.syncedIds.size(), 1U);

    // 归零后再进入静止仍会下发。
    ghast.setServerStillTimeoutForTest(0);
    ghast.setServerStillTimeoutForTest(10);
    EXPECT_EQ(m_world.syncedIds.size(), 2U);
}

// 诱惑物品：成体取 HAPPY_GHAST_TEMPT_ITEMS（雪球 + 挽具），幼体取 HAPPY_GHAST_FOOD。
TEST_F(HappyGhastTest, TemptItemTagsRegistered)
{
    ASSERT_NE(Items::SNOWBALL, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::HAPPY_GHAST_FOOD().contains(Items::SNOWBALL));
    EXPECT_TRUE(item::tag::ItemTags::HAPPY_GHAST_TEMPT_ITEMS().contains(Items::SNOWBALL));
    // 挽具也是诱惑物品（展开自 #harnesses）。
    const Item* whiteHarness = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "white_harness"));
    if (whiteHarness != nullptr) {
        EXPECT_TRUE(item::tag::ItemTags::HAPPY_GHAST_TEMPT_ITEMS().contains(whiteHarness));
        EXPECT_FALSE(item::tag::ItemTags::HAPPY_GHAST_FOOD().contains(whiteHarness));
    }
}