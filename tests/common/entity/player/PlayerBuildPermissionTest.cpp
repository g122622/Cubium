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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND OF EXPRESS OR IMPLIED,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/player/GameModeUtils.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "world/block/BlockTags.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// mayBuild() 测试 — 纯逻辑，无需 world
// ============================================================================

TEST(PlayerMayBuildTest, SurvivalModeCanBuild)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Survival);
    EXPECT_TRUE(player.mayBuild());
}

TEST(PlayerMayBuildTest, CreativeModeCanBuild)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Creative);
    EXPECT_TRUE(player.mayBuild());
}

TEST(PlayerMayBuildTest, AdventureModeCannotBuild)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Adventure);
    EXPECT_FALSE(player.mayBuild());
}

TEST(PlayerMayBuildTest, SpectatorModeCannotBuild)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Spectator);
    EXPECT_FALSE(player.mayBuild());
}

TEST(PlayerMayBuildTest, AllowEditOverrideInSurvival)
{
    // 生存模式下手动关闭 allowEdit（如通过命令）
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Survival);
    EXPECT_TRUE(player.mayBuild());

    player.abilities().allowEdit = false;
    EXPECT_FALSE(player.mayBuild());
}

TEST(PlayerMayBuildTest, SetGameModeResetsAllowEdit)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Survival);
    EXPECT_TRUE(player.mayBuild());

    // 切换到冒险模式后 allowEdit 变为 false
    player.setGameMode(GameMode::Adventure);
    EXPECT_FALSE(player.mayBuild());

    // 切回生存模式后 allowEdit 恢复为 true
    player.setGameMode(GameMode::Survival);
    EXPECT_TRUE(player.mayBuild());
}

// ============================================================================
// GameModeUtils::isBlockPlacingRestricted 测试
// ============================================================================

TEST(GameModeUtilsIsBlockPlacingRestrictedTest, SurvivalNotRestricted)
{
    EXPECT_FALSE(entity::GameModeUtils::isBlockPlacingRestricted(GameMode::Survival));
}

TEST(GameModeUtilsIsBlockPlacingRestrictedTest, CreativeNotRestricted)
{
    EXPECT_FALSE(entity::GameModeUtils::isBlockPlacingRestricted(GameMode::Creative));
}

TEST(GameModeUtilsIsBlockPlacingRestrictedTest, AdventureRestricted)
{
    EXPECT_TRUE(entity::GameModeUtils::isBlockPlacingRestricted(GameMode::Adventure));
}

TEST(GameModeUtilsIsBlockPlacingRestrictedTest, SpectatorRestricted)
{
    EXPECT_TRUE(entity::GameModeUtils::isBlockPlacingRestricted(GameMode::Spectator));
}

// ============================================================================
// 测试 world stub — 用于 mayUseItemAt 和 blockActionRestricted
// ============================================================================

namespace {

class BuildPermissionTestWorld final : public mc::test::BaseTestWorld {
public:
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
        auto blockState = std::make_unique<BlockState>(*state);
        m_blocks[BlockPos(x, y, z)] = std::move(blockState);
        return true;
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    void clearState() { m_blocks.clear(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
};

} // namespace

// ============================================================================
// mayUseItemAt() 测试
// ============================================================================

class PlayerMayUseItemAtTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();

        m_player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
        m_player->setWorld(&m_world);

        // 在 (10, 64, 20) 放置石方块，用于 CanPlaceOn 测试
        m_world.setBlockState(10, 64, 20, &VanillaBlocks::STONE->defaultState());
    }

    void TearDown() override
    {
        m_world.clearState();
        m_player.reset();
    }

    BuildPermissionTestWorld m_world;
    std::unique_ptr<Player> m_player;
};

TEST_F(PlayerMayUseItemAtTest, SurvivalModeAlwaysAllowed)
{
    m_player->setGameMode(GameMode::Survival);
    BlockPos pos(10, 64, 20);
    ItemStack emptyItem;

    EXPECT_TRUE(m_player->mayUseItemAt(m_world, pos, Direction::North, emptyItem));
}

TEST_F(PlayerMayUseItemAtTest, CreativeModeAlwaysAllowed)
{
    m_player->setGameMode(GameMode::Creative);
    BlockPos pos(10, 64, 20);
    ItemStack emptyItem;

    EXPECT_TRUE(m_player->mayUseItemAt(m_world, pos, Direction::North, emptyItem));
}

TEST_F(PlayerMayUseItemAtTest, AdventureModeWithEmptyItemNotAllowed)
{
    m_player->setGameMode(GameMode::Adventure);
    BlockPos pos(10, 64, 20);
    ItemStack emptyItem;

    // 冒险模式 + 空手 → mayBuild=false + 没有 CanPlaceOn → 不允许
    EXPECT_FALSE(m_player->mayUseItemAt(m_world, pos, Direction::North, emptyItem));
}

TEST_F(PlayerMayUseItemAtTest, AdventureModeWithCanPlaceOnMatchAllowed)
{
    m_player->setGameMode(GameMode::Adventure);
    BlockPos pos(10, 64, 20);

    // 放置物品在主手，设置 CanPlaceOn 标签为 stone
    ItemStack tool(Items::DIAMOND_PICKAXE, 1);
    tool.setCanPlaceOn(AdventureModePredicate({"minecraft:stone"}));
    m_player->inventory().getSelectedStackRef() = tool;

    // mayUseItemAt 检查 pos.offset(opposite(face)) 处的方块
    // pos=(10,64,20), face=North → opposite=South → offset 到 (10,64,21)
    // (10,64,21) 处没有方块（AIR），所以 canPlaceOn 不匹配
    EXPECT_FALSE(m_player->mayUseItemAt(m_world, pos, Direction::North, tool));

    // 在 (10,64,21) 放置石方块（pos 的 South 方向）
    m_world.setBlockState(10, 64, 21, &VanillaBlocks::STONE->defaultState());

    // 现在 South 方向有石方块，CanPlaceOn 应该匹配
    EXPECT_TRUE(m_player->mayUseItemAt(m_world, pos, Direction::North, tool));
}

TEST_F(PlayerMayUseItemAtTest, AdventureModeWithCanPlaceOnNoMatchNotAllowed)
{
    m_player->setGameMode(GameMode::Adventure);
    BlockPos pos(10, 64, 20);

    // 设置 CanPlaceOn 标签为 dirt（不是 stone）
    ItemStack tool(Items::DIAMOND_PICKAXE, 1);
    tool.setCanPlaceOn(AdventureModePredicate({"minecraft:dirt"}));
    m_player->inventory().getSelectedStackRef() = tool;

    // (10,64,20) 的 North 方向对面是 (10,64,21)，那里是 AIR
    // 即使 (10,64,20) 本身是 stone，mayUseItemAt 检查的是对面的方块
    EXPECT_FALSE(m_player->mayUseItemAt(m_world, pos, Direction::North, tool));
}

TEST_F(PlayerMayUseItemAtTest, SpectatorModeNotAllowed)
{
    m_player->setGameMode(GameMode::Spectator);
    BlockPos pos(10, 64, 20);

    ItemStack tool(Items::DIAMOND_PICKAXE, 1);
    tool.setCanPlaceOn(AdventureModePredicate({"minecraft:stone"}));
    m_player->inventory().getSelectedStackRef() = tool;

    // 旁观者 allowEdit=false，即使有 CanPlaceOn 物品也不允许
    EXPECT_FALSE(m_player->mayUseItemAt(m_world, pos, Direction::North, tool));
}

// ============================================================================
// blockActionRestricted() 测试
// ============================================================================

class PlayerBlockActionRestrictedTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();

        m_player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
        m_player->setWorld(&m_world);

        // 在 (10, 64, 20) 放置石方块，用于 CanDestroy 测试
        m_world.setBlockState(10, 64, 20, &VanillaBlocks::STONE->defaultState());
    }

    void TearDown() override
    {
        m_world.clearState();
        m_player.reset();
    }

    BuildPermissionTestWorld m_world;
    std::unique_ptr<Player> m_player;
};

TEST_F(PlayerBlockActionRestrictedTest, SurvivalModeNotRestricted)
{
    m_player->setGameMode(GameMode::Survival);
    BlockPos pos(10, 64, 20);

    EXPECT_FALSE(m_player->blockActionRestricted(m_world, pos));
}

TEST_F(PlayerBlockActionRestrictedTest, CreativeModeNotRestricted)
{
    m_player->setGameMode(GameMode::Creative);
    BlockPos pos(10, 64, 20);

    EXPECT_FALSE(m_player->blockActionRestricted(m_world, pos));
}

TEST_F(PlayerBlockActionRestrictedTest, SpectatorModeAlwaysRestricted)
{
    m_player->setGameMode(GameMode::Spectator);
    BlockPos pos(10, 64, 20);

    EXPECT_TRUE(m_player->blockActionRestricted(m_world, pos));
}

TEST_F(PlayerBlockActionRestrictedTest, AdventureModeWithEmptyHandRestricted)
{
    m_player->setGameMode(GameMode::Adventure);
    BlockPos pos(10, 64, 20);

    // 冒险模式 + 空手 → 受限
    EXPECT_TRUE(m_player->blockActionRestricted(m_world, pos));
}

TEST_F(PlayerBlockActionRestrictedTest, AdventureModeWithCanDestroyMatchNotRestricted)
{
    m_player->setGameMode(GameMode::Adventure);
    BlockPos pos(10, 64, 20);

    // 设置 CanDestroy 标签为 stone
    ItemStack tool(Items::DIAMOND_PICKAXE, 1);
    tool.setCanDestroy(AdventureModePredicate({"minecraft:stone"}));
    m_player->inventory().getSelectedStackRef() = tool;

    // CanDestroy 匹配方块 → 不受限
    EXPECT_FALSE(m_player->blockActionRestricted(m_world, pos));
}

TEST_F(PlayerBlockActionRestrictedTest, AdventureModeWithCanDestroyNoMatchRestricted)
{
    m_player->setGameMode(GameMode::Adventure);
    BlockPos pos(10, 64, 20);

    // 设置 CanDestroy 标签为 dirt（不是 stone）
    ItemStack tool(Items::DIAMOND_PICKAXE, 1);
    tool.setCanDestroy(AdventureModePredicate({"minecraft:dirt"}));
    m_player->inventory().getSelectedStackRef() = tool;

    // CanDestroy 不匹配 stone 方块 → 受限
    EXPECT_TRUE(m_player->blockActionRestricted(m_world, pos));
}

TEST_F(PlayerBlockActionRestrictedTest, AdventureModeWithCanPlaceOnNotCanDestroyRestricted)
{
    m_player->setGameMode(GameMode::Adventure);
    BlockPos pos(10, 64, 20);

    // 设置 CanPlaceOn（不是 CanDestroy）→ 不能用于破坏
    ItemStack tool(Items::DIAMOND_PICKAXE, 1);
    tool.setCanPlaceOn(AdventureModePredicate({"minecraft:stone"}));
    m_player->inventory().getSelectedStackRef() = tool;

    // CanPlaceOn 不等于 CanDestroy，没有 CanDestroy 标签 → 受限
    EXPECT_TRUE(m_player->blockActionRestricted(m_world, pos));
}

TEST_F(PlayerBlockActionRestrictedTest, AdventureModeAllowEditOverrideNotRestricted)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Adventure);
    BlockPos pos(10, 64, 20);

    // 冒险模式默认受限
    EXPECT_TRUE(player.blockActionRestricted(m_world, pos));

    // 手动设置 allowEdit = true（如通过命令）→ 不受限
    player.abilities().allowEdit = true;
    EXPECT_FALSE(player.blockActionRestricted(m_world, pos));
}
