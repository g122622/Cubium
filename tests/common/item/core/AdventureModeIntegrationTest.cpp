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
#include "common/advancement/trigger/conditions/NBTPredicate.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"

using namespace mc;
using namespace mc::nbt;
using namespace mc::nbt::tags;

// ============================================================================
// Player::mayInteract() 冒险模式集成测试
// ============================================================================

namespace {

/**
 * @brief 测试用世界存根，用于 Player::mayInteract 冒险模式测试
 */
class AdventureModeTestWorld final : public mc::test::BaseTestWorld {
public:
    AdventureModeTestWorld() = default;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    void clearState() { m_blocks.clear(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
};

} // namespace

class PlayerMayInteractAdventureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();
    }

    void TearDown() override { m_world.clearState(); }

    AdventureModeTestWorld m_world;
};

// ============================================================================
// 冒险模式基本交互权限
// ============================================================================

TEST_F(PlayerMayInteractAdventureTest, AdventureMode_NoCanPlaceOn_ReturnsFalse)
{
    // 冒险模式下没有 CanPlaceOn 标签的物品不能与方块交互
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);

    // 设置方块
    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    EXPECT_FALSE(player->mayInteract(m_world, pos));
}

TEST_F(PlayerMayInteractAdventureTest, AdventureMode_EmptyHand_ReturnsFalse)
{
    // 冒险模式下空手不能交互
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    EXPECT_FALSE(player->mayInteract(m_world, pos));
}

TEST_F(PlayerMayInteractAdventureTest, SurvivalMode_AlwaysReturnsTrue)
{
    // 生存模式始终允许交互
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Survival);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    EXPECT_TRUE(player->mayInteract(m_world, pos));
}

TEST_F(PlayerMayInteractAdventureTest, CreativeMode_AlwaysReturnsTrue)
{
    // 创造模式始终允许交互
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Creative);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    EXPECT_TRUE(player->mayInteract(m_world, pos));
}

TEST_F(PlayerMayInteractAdventureTest, SpectatorMode_AlwaysReturnsFalse)
{
    // 旁观模式始终禁止交互
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Spectator);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    EXPECT_FALSE(player->mayInteract(m_world, pos));
}

// ============================================================================
// 冒险模式 + CanPlaceOn 精确ID匹配
// ============================================================================

TEST_F(PlayerMayInteractAdventureTest, MainHand_CanPlaceOnExactMatch_ReturnsTrue)
{
    // 主手物品的 CanPlaceOn 标签匹配目标方块
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    // 给主手物品设置 CanPlaceOn: minecraft:stone
    if (Items::DIAMOND_PICKAXE) {
        ItemStack tool(Items::DIAMOND_PICKAXE, 1);
        tool.setCanPlaceOn(AdventureModePredicate({"minecraft:stone"}));
        player->inventory().getSelectedStackRef() = tool;

        EXPECT_TRUE(player->mayInteract(m_world, pos));
    } else {
        GTEST_SKIP() << "DIAMOND_PICKAXE item not registered";
    }
}

TEST_F(PlayerMayInteractAdventureTest, MainHand_CanPlaceOnNoMatch_ReturnsFalse)
{
    // 主手物品的 CanPlaceOn 标签不匹配目标方块
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    // 给主手物品设置 CanPlaceOn: minecraft:dirt（不匹配 stone）
    if (Items::DIAMOND_PICKAXE) {
        ItemStack tool(Items::DIAMOND_PICKAXE, 1);
        tool.setCanPlaceOn(AdventureModePredicate({"minecraft:dirt"}));
        player->inventory().getSelectedStackRef() = tool;

        EXPECT_FALSE(player->mayInteract(m_world, pos));
    } else {
        GTEST_SKIP() << "DIAMOND_PICKAXE item not registered";
    }
}

// ============================================================================
// 冒险模式 + CanPlaceOn 标签引用匹配
// ============================================================================

TEST_F(PlayerMayInteractAdventureTest, MainHand_CanPlaceOnTagMatch_ReturnsTrue)
{
    // 主手物品的 CanPlaceOn 标签使用 #minecraft:dirt 匹配
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::DIRT->defaultState());

    if (Items::DIAMOND_PICKAXE) {
        ItemStack tool(Items::DIAMOND_PICKAXE, 1);
        tool.setCanPlaceOn(AdventureModePredicate({"#minecraft:dirt"}));
        player->inventory().getSelectedStackRef() = tool;

        EXPECT_TRUE(player->mayInteract(m_world, pos));
    } else {
        GTEST_SKIP() << "DIAMOND_PICKAXE item not registered";
    }
}

TEST_F(PlayerMayInteractAdventureTest, MainHand_CanPlaceOnTagNoMatch_ReturnsFalse)
{
    // 主手物品的 CanPlaceOn 标签使用 #minecraft:logs 不匹配 stone
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    if (Items::DIAMOND_PICKAXE) {
        ItemStack tool(Items::DIAMOND_PICKAXE, 1);
        tool.setCanPlaceOn(AdventureModePredicate({"#minecraft:logs"}));
        player->inventory().getSelectedStackRef() = tool;

        EXPECT_FALSE(player->mayInteract(m_world, pos));
    } else {
        GTEST_SKIP() << "DIAMOND_PICKAXE item not registered";
    }
}

// ============================================================================
// 冒险模式 + 副手 CanPlaceOn 检查
// ============================================================================

TEST_F(PlayerMayInteractAdventureTest, OffHand_CanPlaceOnMatch_ReturnsTrue)
{
    // 副手物品的 CanPlaceOn 标签匹配目标方块（主手无 CanPlaceOn）
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    // 主手无 CanPlaceOn，副手有 CanPlaceOn: minecraft:stone
    if (Items::DIAMOND_PICKAXE && Items::STONE) {
        ItemStack mainTool(Items::DIAMOND_PICKAXE, 1);
        // 主手物品没有 CanPlaceOn
        player->inventory().getSelectedStackRef() = mainTool;

        ItemStack offTool(Items::STONE, 1);
        offTool.setCanPlaceOn(AdventureModePredicate({"minecraft:stone"}));
        player->inventory().setOffhandItem(offTool);

        EXPECT_TRUE(player->mayInteract(m_world, pos));
    } else {
        GTEST_SKIP() << "Required items not registered";
    }
}

TEST_F(PlayerMayInteractAdventureTest, OffHand_CanPlaceOnNoMatch_MainHandNoCanPlaceOn_ReturnsFalse)
{
    // 副手物品的 CanPlaceOn 不匹配，主手也无 CanPlaceOn
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    if (Items::DIAMOND_PICKAXE && Items::STONE) {
        ItemStack mainTool(Items::DIAMOND_PICKAXE, 1);
        player->inventory().getSelectedStackRef() = mainTool;

        ItemStack offTool(Items::STONE, 1);
        offTool.setCanPlaceOn(AdventureModePredicate({"minecraft:dirt"}));
        player->inventory().setOffhandItem(offTool);

        EXPECT_FALSE(player->mayInteract(m_world, pos));
    } else {
        GTEST_SKIP() << "Required items not registered";
    }
}

TEST_F(PlayerMayInteractAdventureTest, BothHands_CanPlaceOn_MainHandMatches)
{
    // 双手都有 CanPlaceOn，主手匹配
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    if (Items::DIAMOND_PICKAXE && Items::STONE) {
        ItemStack mainTool(Items::DIAMOND_PICKAXE, 1);
        mainTool.setCanPlaceOn(AdventureModePredicate({"minecraft:stone"}));
        player->inventory().getSelectedStackRef() = mainTool;

        ItemStack offTool(Items::STONE, 1);
        offTool.setCanPlaceOn(AdventureModePredicate({"minecraft:dirt"}));
        player->inventory().setOffhandItem(offTool);

        EXPECT_TRUE(player->mayInteract(m_world, pos));
    } else {
        GTEST_SKIP() << "Required items not registered";
    }
}

TEST_F(PlayerMayInteractAdventureTest, BothHands_CanPlaceOn_OffHandMatches)
{
    // 双手都有 CanPlaceOn，主手不匹配但副手匹配
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    if (Items::DIAMOND_PICKAXE && Items::STONE) {
        ItemStack mainTool(Items::DIAMOND_PICKAXE, 1);
        mainTool.setCanPlaceOn(AdventureModePredicate({"minecraft:dirt"}));
        player->inventory().getSelectedStackRef() = mainTool;

        ItemStack offTool(Items::STONE, 1);
        offTool.setCanPlaceOn(AdventureModePredicate({"minecraft:stone"}));
        player->inventory().setOffhandItem(offTool);

        EXPECT_TRUE(player->mayInteract(m_world, pos));
    } else {
        GTEST_SKIP() << "Required items not registered";
    }
}

TEST_F(PlayerMayInteractAdventureTest, BothHands_CanPlaceOn_NeitherMatches)
{
    // 双手都有 CanPlaceOn，但都不匹配
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    if (Items::DIAMOND_PICKAXE && Items::STONE) {
        ItemStack mainTool(Items::DIAMOND_PICKAXE, 1);
        mainTool.setCanPlaceOn(AdventureModePredicate({"minecraft:dirt"}));
        player->inventory().getSelectedStackRef() = mainTool;

        ItemStack offTool(Items::STONE, 1);
        offTool.setCanPlaceOn(AdventureModePredicate({"#minecraft:logs"}));
        player->inventory().setOffhandItem(offTool);

        EXPECT_FALSE(player->mayInteract(m_world, pos));
    } else {
        GTEST_SKIP() << "Required items not registered";
    }
}

// ============================================================================
// 冒险模式 + 空气方块
// ============================================================================

TEST_F(PlayerMayInteractAdventureTest, AdventureMode_AirBlock_ReturnsFalse)
{
    // 冒险模式下对空气方块交互返回 false（空气不应被交互）
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);

    BlockPos pos(10, 64, 20);
    // 不设置方块，默认为空气

    if (Items::DIAMOND_PICKAXE) {
        ItemStack tool(Items::DIAMOND_PICKAXE, 1);
        tool.setCanPlaceOn(AdventureModePredicate({"minecraft:air"}));
        player->inventory().getSelectedStackRef() = tool;

        // 空气方块即使有 CanPlaceOn 也不允许交互
        EXPECT_FALSE(player->mayInteract(m_world, pos));
    } else {
        GTEST_SKIP() << "DIAMOND_PICKAXE item not registered";
    }
}

// ============================================================================
// ItemStack CanPlaceOn/CanDestroy NBT 序列化往返测试
// ============================================================================

class ItemStackAdventureModeNbtTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ItemStackAdventureModeNbtTest, CanPlaceOnRoundTrip)
{
    // 测试 CanPlaceOn NBT 序列化/反序列化往返
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    if (!diamond) {
        // 注册一个测试物品
        diamond = &ItemRegistry::instance().registerItem(
            ResourceLocation("minecraft", "diamond"), ItemProperties().maxStackSize(64));
    }
    ASSERT_NE(diamond, nullptr);

    ItemStack original(diamond, 1);
    original.setCanPlaceOn(AdventureModePredicate({"minecraft:stone", "minecraft:dirt", "#minecraft:logs"}));

    // 序列化到 NBT
    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    // 反序列化
    auto parsed = ItemStack::fromNbt(tag);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();

    const ItemStack& result = parsed.value();

    // 验证 CanPlaceOn
    EXPECT_TRUE(result.hasCanPlaceOn());
    EXPECT_EQ(result.getCanPlaceOn().getPredicates().size(), 3u);
    EXPECT_EQ(result.getCanPlaceOn().getPredicates()[0], "minecraft:stone");
    EXPECT_EQ(result.getCanPlaceOn().getPredicates()[1], "minecraft:dirt");
    EXPECT_EQ(result.getCanPlaceOn().getPredicates()[2], "#minecraft:logs");

    // 验证 CanDestroy 为空
    EXPECT_FALSE(result.hasCanDestroy());
}

TEST_F(ItemStackAdventureModeNbtTest, CanDestroyRoundTrip)
{
    // 测试 CanDestroy NBT 序列化/反序列化往返
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    if (!diamond) {
        diamond = &ItemRegistry::instance().registerItem(
            ResourceLocation("minecraft", "diamond"), ItemProperties().maxStackSize(64));
    }
    ASSERT_NE(diamond, nullptr);

    ItemStack original(diamond, 1);
    original.setCanDestroy(AdventureModePredicate({"minecraft:stone", "#minecraft:dirt"}));

    // 序列化到 NBT
    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    // 反序列化
    auto parsed = ItemStack::fromNbt(tag);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();

    const ItemStack& result = parsed.value();

    // 验证 CanDestroy
    EXPECT_TRUE(result.hasCanDestroy());
    EXPECT_EQ(result.getCanDestroy().getPredicates().size(), 2u);
    EXPECT_EQ(result.getCanDestroy().getPredicates()[0], "minecraft:stone");
    EXPECT_EQ(result.getCanDestroy().getPredicates()[1], "#minecraft:dirt");

    // 验证 CanPlaceOn 为空
    EXPECT_FALSE(result.hasCanPlaceOn());
}

TEST_F(ItemStackAdventureModeNbtTest, BothCanPlaceOnAndCanDestroyRoundTrip)
{
    // 测试同时有 CanPlaceOn 和 CanDestroy 的往返
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    if (!diamond) {
        diamond = &ItemRegistry::instance().registerItem(
            ResourceLocation("minecraft", "diamond"), ItemProperties().maxStackSize(64));
    }
    ASSERT_NE(diamond, nullptr);

    ItemStack original(diamond, 1);
    original.setCanPlaceOn(AdventureModePredicate({"minecraft:stone"}));
    original.setCanDestroy(AdventureModePredicate({"minecraft:dirt", "#minecraft:logs"}));

    // 序列化到 NBT
    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    // 反序列化
    auto parsed = ItemStack::fromNbt(tag);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();

    const ItemStack& result = parsed.value();

    // 验证 CanPlaceOn
    EXPECT_TRUE(result.hasCanPlaceOn());
    EXPECT_EQ(result.getCanPlaceOn().getPredicates().size(), 1u);
    EXPECT_EQ(result.getCanPlaceOn().getPredicates()[0], "minecraft:stone");

    // 验证 CanDestroy
    EXPECT_TRUE(result.hasCanDestroy());
    EXPECT_EQ(result.getCanDestroy().getPredicates().size(), 2u);
    EXPECT_EQ(result.getCanDestroy().getPredicates()[0], "minecraft:dirt");
    EXPECT_EQ(result.getCanDestroy().getPredicates()[1], "#minecraft:logs");
}

TEST_F(ItemStackAdventureModeNbtTest, NoAdventureModeTagsRoundTrip)
{
    // 测试没有 CanPlaceOn/CanDestroy 的物品正常序列化/反序列化
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    if (!diamond) {
        diamond = &ItemRegistry::instance().registerItem(
            ResourceLocation("minecraft", "diamond"), ItemProperties().maxStackSize(64));
    }
    ASSERT_NE(diamond, nullptr);

    ItemStack original(diamond, 1);

    // 序列化到 NBT
    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    // 反序列化
    auto parsed = ItemStack::fromNbt(tag);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();

    const ItemStack& result = parsed.value();

    // 不应有冒险模式标签
    EXPECT_FALSE(result.hasCanPlaceOn());
    EXPECT_FALSE(result.hasCanDestroy());
}

TEST_F(ItemStackAdventureModeNbtTest, EqualityAfterRoundTrip)
{
    // 测试序列化/反序列化后物品堆相等性
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    if (!diamond) {
        diamond = &ItemRegistry::instance().registerItem(
            ResourceLocation("minecraft", "diamond"), ItemProperties().maxStackSize(64));
    }
    ASSERT_NE(diamond, nullptr);

    ItemStack original(diamond, 1);
    original.setCanPlaceOn(AdventureModePredicate({"minecraft:stone"}));
    original.setCanDestroy(AdventureModePredicate({"minecraft:dirt"}));

    // 序列化到 NBT
    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    // 反序列化
    auto parsed = ItemStack::fromNbt(tag);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();

    const ItemStack& result = parsed.value();

    // CanPlaceOn 和 CanDestroy 应该相等
    EXPECT_EQ(original.getCanPlaceOn(), result.getCanPlaceOn());
    EXPECT_EQ(original.getCanDestroy(), result.getCanDestroy());
}

TEST_F(ItemStackAdventureModeNbtTest, CanPlaceOnPredicateFunctionalAfterRoundTrip)
{
    // 测试反序列化后的 CanPlaceOn 仍然能正确匹配方块
    VanillaBlocks::initialize();
    BlockTags::initialize();

    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    if (!diamond) {
        diamond = &ItemRegistry::instance().registerItem(
            ResourceLocation("minecraft", "diamond"), ItemProperties().maxStackSize(64));
    }
    ASSERT_NE(diamond, nullptr);

    ItemStack original(diamond, 1);
    original.setCanPlaceOn(AdventureModePredicate({"minecraft:stone", "#minecraft:dirt"}));

    // 序列化到 NBT
    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    // 反序列化
    auto parsed = ItemStack::fromNbt(tag);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();

    const ItemStack& result = parsed.value();

    // 测试匹配功能
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    const BlockState& oakLogState = VanillaBlocks::OAK_LOG->defaultState();

    EXPECT_TRUE(result.canPlaceOnBlockInAdventureMode(stoneState));
    EXPECT_TRUE(result.canPlaceOnBlockInAdventureMode(dirtState));
    EXPECT_FALSE(result.canPlaceOnBlockInAdventureMode(oakLogState));
}

TEST_F(ItemStackAdventureModeNbtTest, EmptyAdventureModePredicateNotSerialized)
{
    // 空 AdventureModePredicate 不应序列化到 NBT
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    if (!diamond) {
        diamond = &ItemRegistry::instance().registerItem(
            ResourceLocation("minecraft", "diamond"), ItemProperties().maxStackSize(64));
    }
    ASSERT_NE(diamond, nullptr);

    ItemStack stack(diamond, 1);
    // 默认空的 CanPlaceOn/CanDestroy 不应产生 NBT 数据

    // needTag() 应该返回 false（没有冒险模式标签）
    EXPECT_FALSE(stack.hasCanPlaceOn());
    EXPECT_FALSE(stack.hasCanDestroy());
}

// ============================================================================
// AdventureModePredicate NBT 匹配测试（带 IWorld + BlockPos）
// ============================================================================

namespace {

/**
 * @brief 支持 BlockEntity 的测试世界，用于 NBT 匹配测试
 */
class NbtMatchingTestWorld final : public mc::test::BaseTestWorld {
public:
    NbtMatchingTestWorld() = default;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_blockEntities.find(pos);
        if (it != m_blockEntities.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_blockEntities.find(pos);
        if (it != m_blockEntities.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    void setBlockEntity(const BlockPos& pos, std::unique_ptr<BlockEntity> entity)
    {
        m_blockEntities[pos] = std::move(entity);
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    void clearState()
    {
        m_blocks.clear();
        m_blockEntities.clear();
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
};

/**
 * @brief 测试用简单方块实体，用于验证 NBT 匹配
 */
class TestBlockEntity : public BlockEntity {
public:
    TestBlockEntity(const BlockPos& pos, nbt::tags::compound_tag customData)
        : BlockEntity(BlockEntityType::Chest, pos)
        , m_customData(std::move(customData))
    {}

    void saveToNBT(nbt::tags::compound_tag& tag) const override
    {
        BlockEntity::saveToNBT(tag);
        // 合并自定义数据到标签中
        for (const auto& [key, value] : m_customData.value) {
            tag.value.emplace(key, value->copy());
        }
    }

    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override
    {
        auto copiedData = nbt::tags::compound_tag();
        for (const auto& [key, value] : m_customData.value) {
            copiedData.value.emplace(key, value->copy());
        }
        return std::make_unique<TestBlockEntity>(m_pos, std::move(copiedData));
    }

private:
    nbt::tags::compound_tag m_customData;
};

} // namespace

class AdventureModeNbtMatchingTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }

    void TearDown() override { m_world.clearState(); }

    NbtMatchingTestWorld m_world;
};

TEST_F(AdventureModeNbtMatchingTest, NBTMatching_NoBlockEntity_NbtPredicateFails)
{
    // 谓词包含 NBT 条件，但没有方块实体时，匹配应失败
    AdventureModePredicate predicate({"minecraft:stone{CustomName:'test'}"});

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());
    const BlockState* state = m_world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(state, nullptr);

    // 没有方块实体，NBT 条件匹配失败
    EXPECT_FALSE(predicate.test(m_world, pos, *state));
}

TEST_F(AdventureModeNbtMatchingTest, NBTMatching_WithBlockEntity_MatchingNbt)
{
    // 谓词包含 NBT 条件，方块实体的 NBT 匹配时应成功
    AdventureModePredicate predicate({"minecraft:stone{CustomName:'test'}"});

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());
    const BlockState* state = m_world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(state, nullptr);

    // 创建带有匹配 NBT 数据的方块实体
    auto customData = nbt::tags::compound_tag();
    customData.value.emplace("CustomName", std::make_unique<nbt::tags::string_tag>("test"));
    m_world.setBlockEntity(pos, std::make_unique<TestBlockEntity>(pos, std::move(customData)));

    // NBT 匹配（子集匹配：谓词的 {CustomName:'test'} 是实际数据的子集）
    EXPECT_TRUE(predicate.test(m_world, pos, *state));
}

TEST_F(AdventureModeNbtMatchingTest, NBTMatching_WithBlockEntity_NonMatchingNbt)
{
    // 谓词包含 NBT 条件，但方块实体的 NBT 不匹配时应失败
    AdventureModePredicate predicate({"minecraft:stone{CustomName:'expected'}"});

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());
    const BlockState* state = m_world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(state, nullptr);

    // 创建带有不同 NBT 数据的方块实体
    auto customData = nbt::tags::compound_tag();
    customData.value.emplace("CustomName", std::make_unique<nbt::tags::string_tag>("different"));
    m_world.setBlockEntity(pos, std::make_unique<TestBlockEntity>(pos, std::move(customData)));

    // NBT 不匹配
    EXPECT_FALSE(predicate.test(m_world, pos, *state));
}

TEST_F(AdventureModeNbtMatchingTest, NBTMatching_SubsetMatching)
{
    // NBT 使用子集匹配：谓词中的字段在实际NBT中存在且值相等即匹配
    AdventureModePredicate predicate({"minecraft:stone{CustomName:'test'}"});

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());
    const BlockState* state = m_world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(state, nullptr);

    // 创建带有更多字段的方块实体（包含谓词要求的字段 + 额外字段）
    auto customData = nbt::tags::compound_tag();
    customData.value.emplace("CustomName", std::make_unique<nbt::tags::string_tag>("test"));
    customData.value.emplace("ExtraField", std::make_unique<nbt::tags::int_tag>(42));
    m_world.setBlockEntity(pos, std::make_unique<TestBlockEntity>(pos, std::move(customData)));

    // 子集匹配：谓词 {CustomName:'test'} 是实际数据 {CustomName:'test', ExtraField:42} 的子集
    EXPECT_TRUE(predicate.test(m_world, pos, *state));
}

TEST_F(AdventureModeNbtMatchingTest, NBTMatching_NoNbtCondition_BlockEntityIrrelevant)
{
    // 不含 NBT 条件的谓词，不管有没有方块实体都只看方块状态
    AdventureModePredicate predicate({"minecraft:stone"});

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());
    const BlockState* state = m_world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(state, nullptr);

    // 不设置方块实体，但谓词不含 NBT 条件，方块状态匹配即可
    EXPECT_TRUE(predicate.test(m_world, pos, *state));
}

TEST_F(AdventureModeNbtMatchingTest, NBTMatching_WrongBlockId_NbtDoesNotHelp)
{
    // 方块ID不匹配时，即使NBT匹配也不行
    AdventureModePredicate predicate({"minecraft:dirt{CustomName:'test'}"});

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());
    const BlockState* state = m_world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(state, nullptr);

    auto customData = nbt::tags::compound_tag();
    customData.value.emplace("CustomName", std::make_unique<nbt::tags::string_tag>("test"));
    m_world.setBlockEntity(pos, std::make_unique<TestBlockEntity>(pos, std::move(customData)));

    // 方块ID不匹配，即使NBT匹配也失败
    EXPECT_FALSE(predicate.test(m_world, pos, *state));
}

TEST_F(AdventureModeNbtMatchingTest, NBTMatching_PropertyAndNbtBothRequired)
{
    // 带属性和NBT的谓词：两者都要匹配
    AdventureModePredicate predicate({"minecraft:stone{Count:5}"});

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());
    const BlockState* state = m_world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(state, nullptr);

    auto customData = nbt::tags::compound_tag();
    customData.value.emplace("Count", std::make_unique<nbt::tags::int_tag>(5));
    m_world.setBlockEntity(pos, std::make_unique<TestBlockEntity>(pos, std::move(customData)));

    // 方块ID匹配 + NBT匹配
    EXPECT_TRUE(predicate.test(m_world, pos, *state));
}

TEST_F(AdventureModeNbtMatchingTest, NBTMatching_IntTagMismatch)
{
    // NBT中整数标签值不匹配
    AdventureModePredicate predicate({"minecraft:stone{Count:5}"});

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());
    const BlockState* state = m_world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(state, nullptr);

    auto customData = nbt::tags::compound_tag();
    customData.value.emplace("Count", std::make_unique<nbt::tags::int_tag>(10)); // 不同值
    m_world.setBlockEntity(pos, std::make_unique<TestBlockEntity>(pos, std::move(customData)));

    // Count值不匹配
    EXPECT_FALSE(predicate.test(m_world, pos, *state));
}

TEST_F(AdventureModeNbtMatchingTest, NBTMatching_MultiplePredicates_OrLogic)
{
    // 多个谓词条目使用 OR 逻辑
    AdventureModePredicate predicate({"minecraft:stone{CustomName:'test'}", "minecraft:dirt{CustomName:'other'}"});

    {
        // 匹配第一个条目
        BlockPos pos1(10, 64, 20);
        m_world.setBlockState(pos1.x, pos1.y, pos1.z, &VanillaBlocks::STONE->defaultState());
        const BlockState* state1 = m_world.getBlockState(pos1.x, pos1.y, pos1.z);
        ASSERT_NE(state1, nullptr);

        auto customData1 = nbt::tags::compound_tag();
        customData1.value.emplace("CustomName", std::make_unique<nbt::tags::string_tag>("test"));
        m_world.setBlockEntity(pos1, std::make_unique<TestBlockEntity>(pos1, std::move(customData1)));

        EXPECT_TRUE(predicate.test(m_world, pos1, *state1));
    }

    {
        // 匹配第二个条目
        BlockPos pos2(20, 64, 30);
        m_world.setBlockState(pos2.x, pos2.y, pos2.z, &VanillaBlocks::DIRT->defaultState());
        const BlockState* state2 = m_world.getBlockState(pos2.x, pos2.y, pos2.z);
        ASSERT_NE(state2, nullptr);

        auto customData2 = nbt::tags::compound_tag();
        customData2.value.emplace("CustomName", std::make_unique<nbt::tags::string_tag>("other"));
        m_world.setBlockEntity(pos2, std::make_unique<TestBlockEntity>(pos2, std::move(customData2)));

        EXPECT_TRUE(predicate.test(m_world, pos2, *state2));
    }
}

TEST_F(AdventureModeNbtMatchingTest, NBTMatching_WithoutBlockPos_FallsBackToStateOnly)
{
    // test(IWorld&, BlockState&) 重载不支持 NBT 匹配，退化为纯方块状态匹配
    AdventureModePredicate predicate({"minecraft:stone{CustomName:'test'}"});

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();

    // 无 BlockPos 版本跳过 NBT 检查，方块ID 匹配即通过
    EXPECT_TRUE(predicate.test(m_world, stoneState));
    EXPECT_FALSE(predicate.test(m_world, dirtState));
}

TEST_F(AdventureModeNbtMatchingTest, NBTMatching_NbtPredicateOnly_NoBlockEntityMatch)
{
    // 纯NBT谓词（无属性）但没有方块实体
    AdventureModePredicate predicate({"minecraft:stone{Level:3}"});

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());
    const BlockState* state = m_world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(state, nullptr);

    // 不设置方块实体，NBT 匹配失败
    EXPECT_FALSE(predicate.test(m_world, pos, *state));

    // 但纯方块状态版本跳过 NBT 检查
    EXPECT_TRUE(predicate.test(*state));
}

TEST_F(AdventureModeNbtMatchingTest, NBTMatching_EmptyCompoundTag_MatchesAnyNbt)
{
    // 空NBT {} 应匹配任何方块实体（空 compound 是任何 compound 的子集）
    AdventureModePredicate predicate({"minecraft:stone{}"});

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());
    const BlockState* state = m_world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(state, nullptr);

    // 创建带有数据的方块实体
    auto customData = nbt::tags::compound_tag();
    customData.value.emplace("SomeField", std::make_unique<nbt::tags::string_tag>("value"));
    m_world.setBlockEntity(pos, std::make_unique<TestBlockEntity>(pos, std::move(customData)));

    // 空 compound 是任何 compound 的子集，应匹配
    EXPECT_TRUE(predicate.test(m_world, pos, *state));
}

TEST_F(AdventureModeNbtMatchingTest, NBTMatching_InvalidNbtFormat_NoNbtCondition)
{
    // 无效NBT格式（如未闭合大括号），parseMojangson返回nullptr
    // hasNbt 为 false，等价于没有NBT条件
    AdventureModePredicate predicate({"minecraft:stone{invalid"});

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());
    const BlockState* state = m_world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(state, nullptr);

    // 无效NBT解析失败，hasNbt为false，不检查NBT
    // 方块ID匹配即通过
    EXPECT_TRUE(predicate.test(m_world, pos, *state));
}
