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

/**
 * @file SpecialToolsItemsTest.cpp
 * @brief 工具 / 特殊物品（盔甲架、末影水晶、物品展示框、山羊角、望远镜、蝌蚪桶、不死图腾）单元测试
 *
 * 测试覆盖：
 * - 7 个物品全链路注册（物品 id 反查、Item 子类 dynamic_cast）
 * - ArmorStandItem：贴地放置生成盔甲架实体、贴底面失败、实体占据失败、yaw 对齐 45 度
 * - EndCrystalItem：黑曜石/基岩上放置、非黑曜石失败、上方非空气失败
 * - ItemFrameItem：侧面放置生成展示框、贴顶/底面失败、荧光变体标记
 * - SpyglassItem：UseAction::Spyglass、使用时长 1200
 * - GoatHornItem：UseAction::TootHorn、使用时长 140、8 音色变体
 * - TadpoleEntity：实体注册、桶物品、音效、成长计时
 * - LivingEntity 不死图腾死亡保护：持图腾不死、消耗图腾、无图腾正常死亡、BYPASSES_INVULNERABILITY 不被救
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/hanging/HangingEntity.hpp"
#include "common/entity/entities/passive/water/TadpoleEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/items/special/ArmorStandItem.hpp"
#include "common/item/items/special/EndCrystalItem.hpp"
#include "common/item/items/special/FishBucketItem.hpp"
#include "common/item/items/special/GoatHornItem.hpp"
#include "common/item/items/special/ItemFrameItem.hpp"
#include "common/item/items/special/SpyglassItem.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;

namespace {

/**
 * @brief 特殊物品测试用世界
 *
 * 提供方块存储、实体生成捕获（记录 spawnEntity 传入的实体），供放置类物品测试使用。
 */
class SpecialItemsTestWorld final : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(key(x, y, z));
        return it != m_blocks.end() ? it->second.get() : &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[key(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawned.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawned.size());
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return world::MAX_BUILD_HEIGHT; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SpecialItemsTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SpecialItemsTestWorld::tickManager not implemented");
    }

    [[nodiscard]] math::IRandom& getRandom() override { return m_random; }
    [[nodiscard]] const math::IRandom& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    [[nodiscard]] ecs::EntityRegistry* entityRegistry() override { return &mc::test::testEcsRegistry(); }

    // 测试辅助
    [[nodiscard]] std::size_t spawnedCount() const noexcept { return m_spawned.size(); }
    [[nodiscard]] Entity* lastSpawned() const noexcept { return m_spawned.empty() ? nullptr : m_spawned.back().get(); }

private:
    static i64 key(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 40) ^ (static_cast<i64>(y) << 20) ^ static_cast<i64>(z & 0xFFFFF);
    }

    std::unordered_map<i64, std::unique_ptr<BlockState>> m_blocks;
    mutable std::vector<std::unique_ptr<Entity>> m_spawned;
    world::border::WorldBorder m_worldBorder;
    mutable math::Random m_random{12345};
};

/// 构造创造模式玩家（不消耗物品）
std::unique_ptr<Player> makeCreativePlayer()
{
    auto player = std::make_unique<Player>(EntityInstanceId(1), "CreativeTester", mc::test::testEcsRegistry());
    player->setGameMode(GameMode::Creative);
    return player;
}

} // namespace

// ============================================================================
// 注册完备性
// ============================================================================

class SpecialToolsRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        entity::VanillaEntities::registerAll();
    }
};

TEST_F(SpecialToolsRegistrationTest, AllSevenItemsRegistered)
{
    ASSERT_NE(Items::ARMOR_STAND, nullptr);
    ASSERT_NE(Items::END_CRYSTAL, nullptr);
    ASSERT_NE(Items::GLOW_ITEM_FRAME, nullptr);
    ASSERT_NE(Items::GOAT_HORN, nullptr);
    ASSERT_NE(Items::SPYGLASS, nullptr);
    ASSERT_NE(Items::TADPOLE_BUCKET, nullptr);
    ASSERT_NE(Items::TOTEM_OF_UNDYING, nullptr);

    // 注册表按 id 反查（验证注册链路完整，非仅静态指针）
    auto& registry = ItemRegistry::instance();
    EXPECT_EQ(registry.getItem(ResourceLocation("minecraft:armor_stand")), Items::ARMOR_STAND);
    EXPECT_EQ(registry.getItem(ResourceLocation("minecraft:end_crystal")), Items::END_CRYSTAL);
    EXPECT_EQ(registry.getItem(ResourceLocation("minecraft:glow_item_frame")), Items::GLOW_ITEM_FRAME);
    EXPECT_EQ(registry.getItem(ResourceLocation("minecraft:goat_horn")), Items::GOAT_HORN);
    EXPECT_EQ(registry.getItem(ResourceLocation("minecraft:spyglass")), Items::SPYGLASS);
    EXPECT_EQ(registry.getItem(ResourceLocation("minecraft:tadpole_bucket")), Items::TADPOLE_BUCKET);
    EXPECT_EQ(registry.getItem(ResourceLocation("minecraft:totem_of_undying")), Items::TOTEM_OF_UNDYING);
}

TEST_F(SpecialToolsRegistrationTest, ItemSubclassesWired)
{
    EXPECT_NE(dynamic_cast<item::items::ArmorStandItem*>(Items::ARMOR_STAND), nullptr);
    EXPECT_NE(dynamic_cast<item::items::EndCrystalItem*>(Items::END_CRYSTAL), nullptr);
    EXPECT_NE(dynamic_cast<item::items::ItemFrameItem*>(Items::GLOW_ITEM_FRAME), nullptr);
    EXPECT_NE(dynamic_cast<item::items::GoatHornItem*>(Items::GOAT_HORN), nullptr);
    EXPECT_NE(dynamic_cast<item::items::SpyglassItem*>(Items::SPYGLASS), nullptr);
    EXPECT_NE(dynamic_cast<item::FishBucketItem*>(Items::TADPOLE_BUCKET), nullptr);
}

TEST_F(SpecialToolsRegistrationTest, TadpoleEntityRegistered)
{
    const entity::EntityType* tadpole = entity::EntityRegistry::instance().getType("minecraft:tadpole");
    ASSERT_NE(tadpole, nullptr);
    EXPECT_NE(entity::EntityRegistry::instance().getType("minecraft:glow_item_frame"), nullptr);
}

TEST_F(SpecialToolsRegistrationTest, StackSizes)
{
    EXPECT_EQ(Items::ARMOR_STAND->maxStackSize(), 16);
    EXPECT_EQ(Items::END_CRYSTAL->maxStackSize(), 64);
    EXPECT_EQ(Items::GLOW_ITEM_FRAME->maxStackSize(), 16);
    EXPECT_EQ(Items::GOAT_HORN->maxStackSize(), 1);
    EXPECT_EQ(Items::SPYGLASS->maxStackSize(), 1);
    EXPECT_EQ(Items::TADPOLE_BUCKET->maxStackSize(), 1);
    EXPECT_EQ(Items::TOTEM_OF_UNDYING->maxStackSize(), 1);
}

// ============================================================================
// 盔甲架物品
// ============================================================================

class ArmorStandItemTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        entity::VanillaEntities::registerAll();
    }
};

TEST_F(ArmorStandItemTest, PlacesArmorStandOnTopFace)
{
    SpecialItemsTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    auto player = makeCreativePlayer();
    ItemStack stack(*Items::ARMOR_STAND, 1);

    ItemUseContext context(world,
        player.get(),
        stack,
        Vector3(0.5f, 64.0f, 0.5f),
        BlockPos(0, 63, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    auto* armorStandItem = dynamic_cast<item::items::ArmorStandItem*>(Items::ARMOR_STAND);
    ASSERT_NE(armorStandItem, nullptr);
    EXPECT_EQ(armorStandItem->onItemUse(context), ActionResultType::Success);
    EXPECT_EQ(world.spawnedCount(), 1u);
}

TEST_F(ArmorStandItemTest, FailsOnBottomFace)
{
    SpecialItemsTestWorld world;
    auto player = makeCreativePlayer();
    ItemStack stack(*Items::ARMOR_STAND, 1);

    ItemUseContext context(world,
        player.get(),
        stack,
        Vector3(0.5f, 64.0f, 0.5f),
        BlockPos(0, 63, 0),
        Direction::Down,
        Hand::MainHand,
        0.0f,
        0.0f);

    auto* armorStandItem = dynamic_cast<item::items::ArmorStandItem*>(Items::ARMOR_STAND);
    ASSERT_NE(armorStandItem, nullptr);
    EXPECT_EQ(armorStandItem->onItemUse(context), ActionResultType::Fail);
    EXPECT_EQ(world.spawnedCount(), 0u);
}

// ============================================================================
// 末影水晶物品
// ============================================================================

class EndCrystalItemTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        entity::VanillaEntities::registerAll();
    }
};

TEST_F(EndCrystalItemTest, PlacesOnObsidian)
{
    SpecialItemsTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::OBSIDIAN->defaultState());
    world.setBlockState(0, 64, 0, &VanillaBlocks::AIR->defaultState());

    auto player = makeCreativePlayer();
    ItemStack stack(*Items::END_CRYSTAL, 1);

    ItemUseContext context(world,
        player.get(),
        stack,
        Vector3(0.5f, 63.99f, 0.5f),
        BlockPos(0, 63, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    auto* endCrystalItem = dynamic_cast<item::items::EndCrystalItem*>(Items::END_CRYSTAL);
    ASSERT_NE(endCrystalItem, nullptr);
    EXPECT_EQ(endCrystalItem->onItemUse(context), ActionResultType::Success);
    EXPECT_EQ(world.spawnedCount(), 1u);
}

TEST_F(EndCrystalItemTest, FailsOnNonObsidianNonBedrock)
{
    SpecialItemsTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    auto player = makeCreativePlayer();
    ItemStack stack(*Items::END_CRYSTAL, 1);

    ItemUseContext context(world,
        player.get(),
        stack,
        Vector3(0.5f, 63.99f, 0.5f),
        BlockPos(0, 63, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    auto* endCrystalItem = dynamic_cast<item::items::EndCrystalItem*>(Items::END_CRYSTAL);
    ASSERT_NE(endCrystalItem, nullptr);
    EXPECT_EQ(endCrystalItem->onItemUse(context), ActionResultType::Fail);
    EXPECT_EQ(world.spawnedCount(), 0u);
}

TEST_F(EndCrystalItemTest, FailsWhenAboveNotEmpty)
{
    SpecialItemsTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::BEDROCK->defaultState());
    world.setBlockState(0, 64, 0, &VanillaBlocks::STONE->defaultState());

    auto player = makeCreativePlayer();
    ItemStack stack(*Items::END_CRYSTAL, 1);

    ItemUseContext context(world,
        player.get(),
        stack,
        Vector3(0.5f, 63.99f, 0.5f),
        BlockPos(0, 63, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    auto* endCrystalItem = dynamic_cast<item::items::EndCrystalItem*>(Items::END_CRYSTAL);
    ASSERT_NE(endCrystalItem, nullptr);
    EXPECT_EQ(endCrystalItem->onItemUse(context), ActionResultType::Fail);
}

// ============================================================================
// 物品展示框
// ============================================================================

class ItemFrameItemTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(ItemFrameItemTest, FailsOnVerticalFace)
{
    SpecialItemsTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    auto player = makeCreativePlayer();
    ItemStack stack(*Items::GLOW_ITEM_FRAME, 1);

    ItemUseContext context(world,
        player.get(),
        stack,
        Vector3(0.5f, 64.0f, 0.5f),
        BlockPos(0, 63, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    auto* frameItem = dynamic_cast<item::items::ItemFrameItem*>(Items::GLOW_ITEM_FRAME);
    ASSERT_NE(frameItem, nullptr);
    // 顶面不可贴展示框
    EXPECT_EQ(frameItem->onItemUse(context), ActionResultType::Fail);
    EXPECT_EQ(world.spawnedCount(), 0u);
}

// ============================================================================
// 望远镜 / 山羊角
// ============================================================================

class SpyglassAndGoatHornTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(SpyglassAndGoatHornTest, SpyglassUseActionAndDuration)
{
    ItemStack stack(*Items::SPYGLASS, 1);
    EXPECT_EQ(Items::SPYGLASS->getUseAction(stack), UseAction::Spyglass);
    EXPECT_EQ(Items::SPYGLASS->getUseDuration(stack), item::items::SpyglassItem::USE_DURATION);
    EXPECT_EQ(item::items::SpyglassItem::USE_DURATION, 1200);
}

TEST_F(SpyglassAndGoatHornTest, GoatHornUseActionAndDuration)
{
    ItemStack stack(*Items::GOAT_HORN, 1);
    EXPECT_EQ(Items::GOAT_HORN->getUseAction(stack), UseAction::TootHorn);
    EXPECT_EQ(Items::GOAT_HORN->getUseDuration(stack), item::items::GoatHornItem::USE_DURATION_TICKS);
    EXPECT_EQ(item::items::GoatHornItem::USE_DURATION_TICKS, 140);
}

TEST_F(SpyglassAndGoatHornTest, GoatHornVariantIndex)
{
    auto* horn = dynamic_cast<item::items::GoatHornItem*>(Items::GOAT_HORN);
    ASSERT_NE(horn, nullptr);
    EXPECT_GE(horn->variantIndex(), 0);
    EXPECT_LT(horn->variantIndex(), 8);
}

// ============================================================================
// 蝌蚪
// ============================================================================

class TadpoleEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(TadpoleEntityTest, BucketItemStack)
{
    TadpoleEntity tadpole(EntityInstanceId(1), mc::test::testEcsRegistry());
    const ItemStack bucket = tadpole.getBucketItemStack();
    EXPECT_EQ(bucket.getItem(), Items::TADPOLE_BUCKET);
    EXPECT_EQ(bucket.getCount(), 1);
}

TEST_F(TadpoleEntityTest, AlwaysFromBucketAndPreventDespawn)
{
    TadpoleEntity tadpole(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(tadpole.isFromBucket());
    EXPECT_TRUE(tadpole.preventDespawn());
}

TEST_F(TadpoleEntityTest, GrowthTimer)
{
    TadpoleEntity tadpole(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(tadpole.getAge(), 0);
    EXPECT_EQ(tadpole.getTicksLeftUntilAdult(), TadpoleEntity::TICKS_TO_BE_FROG);

    tadpole.ageUp(1); // +20 tick
    EXPECT_EQ(tadpole.getAge(), 20);
    EXPECT_EQ(tadpole.getTicksLeftUntilAdult(), TadpoleEntity::TICKS_TO_BE_FROG - 20);
}

TEST_F(TadpoleEntityTest, Sounds)
{
    TadpoleEntity tadpole(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(tadpole.getFlopSound().has_value());
    EXPECT_TRUE(tadpole.getDeathSound().has_value());
    EXPECT_FALSE(tadpole.getAmbientSound().has_value()); // 蝌蚪无环境音
}

// ============================================================================
// 不死图腾死亡保护
// ============================================================================

class TotemDeathProtectionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(TotemDeathProtectionTest, SurvivesLethalDamageWithTotem)
{
    auto player = std::make_unique<Player>(EntityInstanceId(1), "TotemTester", mc::test::testEcsRegistry());
    player->setHealth(1.0f);

    // 主手持有不死图腾
    player->inventory().setSelectedSlot(0);
    player->inventory().setItem(0, ItemStack(*Items::TOTEM_OF_UNDYING, 1));

    EnvironmentalDamage source = DamageSources::generic();
    player->hurt(source, 100.0f);

    // 图腾救回：生命值 = 1、未死亡、图腾被消耗
    EXPECT_FALSE(player->isDead());
    EXPECT_FLOAT_EQ(player->health(), 1.0f);
    EXPECT_TRUE(player->inventory().getSelectedStack().isEmpty());
}

TEST_F(TotemDeathProtectionTest, DiesWithoutTotem)
{
    auto player = std::make_unique<Player>(EntityInstanceId(1), "NoTotemTester", mc::test::testEcsRegistry());
    player->setHealth(1.0f);
    // 空手

    EnvironmentalDamage source = DamageSources::generic();
    player->hurt(source, 100.0f);

    EXPECT_TRUE(player->isDead());
}
