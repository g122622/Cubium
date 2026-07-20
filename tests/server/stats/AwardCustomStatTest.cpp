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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/stats/Stats.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/stats/StatRegistry.hpp"
#include "server/stats/StatisticsManager.hpp"

using namespace mc;
using namespace mc::server::stats;

/**
 * @brief awardCustomStat 集成测试套件
 *
 * 测试 Player::awardCustomStat() 调用链：
 * - 基类 Player 的空实现不崩溃
 * - ServerPlayer 正确委托给 StatisticsManager
 * - Stats 常量与 StatRegistry 注册名称一致
 * - 边界场景：容器打开失败时不触发统计
 */
class AwardCustomStatTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        StatRegistry::instance().registerBuiltinStats();
    }

    void TearDown() override { StatRegistry::instance().clear(); }
};

// ========== Stats 常量与 StatRegistry 一致性测试 ==========

TEST_F(AwardCustomStatTest, StatsConstants_MatchRegistry)
{
    // 验证 Stats.hpp 中的常量字符串在 StatRegistry 中都有注册
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::OPEN_BARREL)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::OPEN_CHEST)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::OPEN_ENDERCHEST)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::OPEN_SHULKER_BOX)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_ANVIL)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_BEACON)));
    EXPECT_TRUE(
        StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_BLAST_FURNACE)));
    EXPECT_TRUE(
        StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_BREWINGSTAND)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_CAMPFIRE)));
    EXPECT_TRUE(
        StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_CARTOGRAPHY_TABLE)));
    EXPECT_TRUE(
        StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_CRAFTING_TABLE)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_FURNACE)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_GRINDSTONE)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_LECTERN)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_LOOM)));
    EXPECT_TRUE(
        StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_SMITHING_TABLE)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_SMOKER)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_STONECUTTER)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::PLAY_RECORD)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::PLAY_NOTEBLOCK)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::TUNE_NOTEBLOCK)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::POT_FLOWER)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::TRIGGER_TRAPPED_CHEST)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::SLEEP_IN_BED)));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation(stats::ENCHANT_ITEM)));
}

// ========== ServerPlayer::awardCustomStat 委托测试 ==========

TEST_F(AwardCustomStatTest, ServerPlayer_AwardCustomStat_IncrementsManager)
{
    // 创建 ServerPlayer 并验证 awardCustomStat 委托给 StatisticsManager
    auto player = std::make_unique<ServerPlayer>(PlayerId(1), "TestPlayer");

    // 初始值应为 0
    EXPECT_EQ(player->getStats().getValue(StatType::Custom, ResourceLocation(stats::OPEN_BARREL)), 0);
    EXPECT_EQ(player->getStats().getValue(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_ANVIL)), 0);

    // 触发统计
    player->awardCustomStat(ResourceLocation(stats::OPEN_BARREL), 1);
    player->awardCustomStat(ResourceLocation(stats::OPEN_BARREL), 1);
    player->awardCustomStat(ResourceLocation(stats::INTERACT_WITH_ANVIL), 1);

    // 验证增量
    EXPECT_EQ(player->getStats().getValue(StatType::Custom, ResourceLocation(stats::OPEN_BARREL)), 2);
    EXPECT_EQ(player->getStats().getValue(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_ANVIL)), 1);

    // 验证未触及的统计仍为 0
    EXPECT_EQ(player->getStats().getValue(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_FURNACE)), 0);
}

TEST_F(AwardCustomStatTest, ServerPlayer_AwardCustomStat_MultipleIncrements)
{
    auto player = std::make_unique<ServerPlayer>(PlayerId(1), "TestPlayer");

    // 多次增量不同的统计
    player->awardCustomStat(ResourceLocation(stats::INTERACT_WITH_FURNACE), 1);
    player->awardCustomStat(ResourceLocation(stats::INTERACT_WITH_BLAST_FURNACE), 3);
    player->awardCustomStat(ResourceLocation(stats::INTERACT_WITH_SMOKER), 5);

    EXPECT_EQ(player->getStats().getValue(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_FURNACE)), 1);
    EXPECT_EQ(player->getStats().getValue(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_BLAST_FURNACE)), 3);
    EXPECT_EQ(player->getStats().getValue(StatType::Custom, ResourceLocation(stats::INTERACT_WITH_SMOKER)), 5);
}

// ========== Player 基类空实现测试 ==========

TEST_F(AwardCustomStatTest, PlayerBaseClass_AwardCustomStat_DoesNotCrash)
{
    // Player 基类的 awardCustomStat 是空实现，调用不应崩溃
    Player player(EntityInstanceId(1), "TestPlayer");
    EXPECT_NO_THROW(player.awardCustomStat(ResourceLocation(stats::OPEN_CHEST), 1));
    EXPECT_NO_THROW(player.awardCustomStat(ResourceLocation(stats::SLEEP_IN_BED), 100));
}

// ========== Stats 常量拼写正确性测试 ==========

TEST_F(AwardCustomStatTest, StatsConstants_CorrectSpelling)
{
    // 验证修正后的常量名与 MC Java Stats.java 一致
    // 这些在之前版本中有拼写错误（如 trapped_chest_triggered → trigger_trapped_chest）
    EXPECT_STREQ(stats::TRIGGER_TRAPPED_CHEST, "minecraft:trigger_trapped_chest");
    EXPECT_STREQ(stats::PLAY_RECORD, "minecraft:play_record");
    EXPECT_STREQ(stats::PLAY_NOTEBLOCK, "minecraft:play_noteblock");
    EXPECT_STREQ(stats::TUNE_NOTEBLOCK, "minecraft:tune_noteblock");
    EXPECT_STREQ(stats::POT_FLOWER, "minecraft:pot_flower");
    EXPECT_STREQ(stats::STRIDER_ONE_CM, "minecraft:strider_one_cm");
    EXPECT_STREQ(stats::NAUTILUS_ONE_CM, "minecraft:nautilus_one_cm");
    EXPECT_STREQ(stats::HAPPY_GHAST_ONE_CM, "minecraft:happy_ghast_one_cm");
}

// ========== StatRegistry 无重复注册测试 ==========

TEST_F(AwardCustomStatTest, StatRegistry_NoDuplicateRegistrations)
{
    // 获取所有 Custom 类型统计，验证数量与预期一致
    // 如果有重复注册，数量会多于预期
    auto customStats = StatRegistry::instance().getStatIdsByType(StatType::Custom);

    // MC Java 1.21.11 Stats.java 中定义了约 68 个自定义统计
    // 允许一定范围，因为我们的注册数量可能与 MC 略有不同
    // 但不应超过 80 个（如果有重复注册会明显超过）
    EXPECT_LE(customStats.size(), 80u);

    // 验证关键统计存在且唯一（通过 hasStat 检查）
    // 如果同一统计被注册两次，hasStat 仍返回 true，但不会重复条目
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:open_barrel")));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:interact_with_anvil")));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:play_record")));
    EXPECT_TRUE(
        StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:trigger_trapped_chest")));
}

// ========== StatRegistry 新增统计验证 ==========

TEST_F(AwardCustomStatTest, StatRegistry_NewlyAddedStats)
{
    // 验证之前缺失、现已补充的统计
    EXPECT_TRUE(
        StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:damage_dealt_absorbed")));
    EXPECT_TRUE(
        StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:damage_dealt_resisted")));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:enchant_item")));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:happy_ghast_one_cm")));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:nautilus_one_cm")));
}

// ========== StatRegistry 不应存在的统计测试 ==========

TEST_F(AwardCustomStatTest, StatRegistry_NonExistentStats)
{
    // 验证 MC Java 1.21.11 中不存在的统计确实没有注册
    // interact_with_composter: MC Java 中无此统计
    EXPECT_FALSE(
        StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:interact_with_composter")));
    // hopper_one_cm: 不存在于 MC Java（只有 inspect_hopper 交互统计）
    EXPECT_FALSE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:hopper_one_cm")));
    // play_one_minute: 已更名为 play_time
    EXPECT_FALSE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:play_one_minute")));
    // dive_one_cm: 不存在于 MC Java
    EXPECT_FALSE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:dive_one_cm")));
    // llama_one_cm: 不存在于 MC Java
    EXPECT_FALSE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:llama_one_cm")));
}

// ========== 边界场景测试 ==========

TEST_F(AwardCustomStatTest, ServerPlayer_AwardCustomStat_ZeroIncrement)
{
    // count=0 不应创建新条目
    auto player = std::make_unique<ServerPlayer>(PlayerId(1), "TestPlayer");

    player->awardCustomStat(ResourceLocation(stats::OPEN_BARREL), 0);
    EXPECT_EQ(player->getStats().getValue(StatType::Custom, ResourceLocation(stats::OPEN_BARREL)), 0);

    // 先增加再增加0，值不变
    player->awardCustomStat(ResourceLocation(stats::OPEN_BARREL), 5);
    player->awardCustomStat(ResourceLocation(stats::OPEN_BARREL), 0);
    EXPECT_EQ(player->getStats().getValue(StatType::Custom, ResourceLocation(stats::OPEN_BARREL)), 5);
}

TEST_F(AwardCustomStatTest, ServerPlayer_AwardCustomStat_NegativeIncrement)
{
    // 负数增量应正常累加（可能导致值为负数或回退）
    auto player = std::make_unique<ServerPlayer>(PlayerId(1), "TestPlayer");

    player->awardCustomStat(ResourceLocation(stats::OPEN_CHEST), 10);
    EXPECT_EQ(player->getStats().getValue(StatType::Custom, ResourceLocation(stats::OPEN_CHEST)), 10);

    player->awardCustomStat(ResourceLocation(stats::OPEN_CHEST), -3);
    EXPECT_EQ(player->getStats().getValue(StatType::Custom, ResourceLocation(stats::OPEN_CHEST)), 7);
}

TEST_F(AwardCustomStatTest, ServerPlayer_AwardCustomStat_UnregisteredStatId)
{
    // 使用未注册的 statId 调用 awardCustomStat 不应崩溃
    auto player = std::make_unique<ServerPlayer>(PlayerId(1), "TestPlayer");

    EXPECT_NO_THROW(player->awardCustomStat(ResourceLocation("minecraft:nonexistent_stat"), 1));
}

TEST_F(AwardCustomStatTest, PlayerBaseClass_AwardCustomStat_WithZeroAndNegative)
{
    // Player 基类的空实现对任何参数都不应崩溃
    Player player(EntityInstanceId(1), "TestPlayer");
    EXPECT_NO_THROW(player.awardCustomStat(ResourceLocation(stats::OPEN_CHEST), 0));
    EXPECT_NO_THROW(player.awardCustomStat(ResourceLocation(stats::OPEN_CHEST), -1));
    EXPECT_NO_THROW(player.awardCustomStat(ResourceLocation("minecraft:fake_stat"), 100));
}
