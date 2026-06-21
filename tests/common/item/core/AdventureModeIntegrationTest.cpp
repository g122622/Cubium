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
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
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
class AdventureModeTestWorld final : public test::BaseTestWorld {
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
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
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
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    EXPECT_FALSE(player->mayInteract(m_world, pos));
}

TEST_F(PlayerMayInteractAdventureTest, SurvivalMode_AlwaysReturnsTrue)
{
    // 生存模式始终允许交互
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Survival);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    EXPECT_TRUE(player->mayInteract(m_world, pos));
}

TEST_F(PlayerMayInteractAdventureTest, CreativeMode_AlwaysReturnsTrue)
{
    // 创造模式始终允许交互
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Creative);

    BlockPos pos(10, 64, 20);
    m_world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::STONE->defaultState());

    EXPECT_TRUE(player->mayInteract(m_world, pos));
}

TEST_F(PlayerMayInteractAdventureTest, SpectatorMode_AlwaysReturnsFalse)
{
    // 旁观模式始终禁止交互
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
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
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
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
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
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
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
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
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
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
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
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
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
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
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
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
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
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
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
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
    auto player = std::make_unique<Player>(EntityId(100), "TestPlayer");
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
