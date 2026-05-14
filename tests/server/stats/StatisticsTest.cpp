#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "server/stats/Stat.hpp"
#include "server/stats/StatRegistry.hpp"
#include "server/stats/StatType.hpp"
#include "server/stats/StatisticsManager.hpp"

using namespace mc;
using namespace mc::server::stats;

/**
 * @brief 统计系统测试套件
 *
 * 测试统计系统的核心功能：
 * - StatType 枚举和工具函数
 * - Stat 类的基本操作
 * - StatRegistry 注册和查询
 * - StatisticsManager 统计管理
 * - NBT 序列化/反序列化
 */
class StatisticsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块和物品注册表
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();

        // 注册内置统计
        StatRegistry::instance().registerBuiltinStats();
    }

    void TearDown() override
    {
        // 清理统计注册表
        StatRegistry::instance().clear();
    }
};

// ========== StatType 测试 ==========

TEST_F(StatisticsTest, StatType_GetPrefix)
{
    EXPECT_EQ(getStatTypePrefix(StatType::Mined), "mined");
    EXPECT_EQ(getStatTypePrefix(StatType::Crafted), "crafted");
    EXPECT_EQ(getStatTypePrefix(StatType::Used), "used");
    EXPECT_EQ(getStatTypePrefix(StatType::Broken), "broken");
    EXPECT_EQ(getStatTypePrefix(StatType::PickedUp), "picked_up");
    EXPECT_EQ(getStatTypePrefix(StatType::Dropped), "dropped");
    EXPECT_EQ(getStatTypePrefix(StatType::Killed), "killed");
    EXPECT_EQ(getStatTypePrefix(StatType::KilledBy), "killed_by");
    EXPECT_EQ(getStatTypePrefix(StatType::Custom), "custom");
}

TEST_F(StatisticsTest, StatType_ParseStatType)
{
    EXPECT_EQ(parseStatType("mined"), StatType::Mined);
    EXPECT_EQ(parseStatType("crafted"), StatType::Crafted);
    EXPECT_EQ(parseStatType("used"), StatType::Used);
    EXPECT_EQ(parseStatType("broken"), StatType::Broken);
    EXPECT_EQ(parseStatType("picked_up"), StatType::PickedUp);
    EXPECT_EQ(parseStatType("dropped"), StatType::Dropped);
    EXPECT_EQ(parseStatType("killed"), StatType::Killed);
    EXPECT_EQ(parseStatType("killed_by"), StatType::KilledBy);
    EXPECT_EQ(parseStatType("custom"), StatType::Custom);
    EXPECT_EQ(parseStatType("unknown"), std::nullopt);
}

TEST_F(StatisticsTest, StatType_BuildStatLocation)
{
    ResourceLocation blockId("minecraft:stone");
    ResourceLocation location = buildStatLocation(StatType::Mined, blockId);
    EXPECT_EQ(location.toString(), "minecraft.mined:minecraft:stone");

    ResourceLocation itemId("minecraft:diamond_sword");
    location = buildStatLocation(StatType::Crafted, itemId);
    EXPECT_EQ(location.toString(), "minecraft.crafted:minecraft:diamond_sword");

    ResourceLocation entityId("minecraft:zombie");
    location = buildStatLocation(StatType::Killed, entityId);
    EXPECT_EQ(location.toString(), "minecraft.killed:minecraft:zombie");

    ResourceLocation customId("minecraft:play_one_minute");
    location = buildStatLocation(StatType::Custom, customId);
    EXPECT_EQ(location.toString(), "minecraft.custom:minecraft:play_one_minute");
}

// ========== Stat 测试 ==========

TEST_F(StatisticsTest, Stat_Construction)
{
    Stat stat(StatType::Mined, ResourceLocation("minecraft:stone"));
    EXPECT_EQ(stat.getType(), StatType::Mined);
    EXPECT_EQ(stat.getId().toString(), "minecraft:stone");
    EXPECT_EQ(stat.getValue(), 0);
}

TEST_F(StatisticsTest, Stat_Increment)
{
    Stat stat(StatType::Mined, ResourceLocation("minecraft:stone"));

    stat.increment();
    EXPECT_EQ(stat.getValue(), 1);

    stat.increment(5);
    EXPECT_EQ(stat.getValue(), 6);

    stat.increment(100);
    EXPECT_EQ(stat.getValue(), 106);
}

TEST_F(StatisticsTest, Stat_SetValue)
{
    Stat stat(StatType::Mined, ResourceLocation("minecraft:stone"));

    stat.setValue(1000);
    EXPECT_EQ(stat.getValue(), 1000);

    stat.setValue(0);
    EXPECT_EQ(stat.getValue(), 0);

    // 测试大数值
    stat.setValue(INT64_MAX);
    EXPECT_EQ(stat.getValue(), INT64_MAX);
}

TEST_F(StatisticsTest, Stat_Reset)
{
    Stat stat(StatType::Mined, ResourceLocation("minecraft:stone"));
    stat.increment(100);
    EXPECT_EQ(stat.getValue(), 100);

    stat.reset();
    EXPECT_EQ(stat.getValue(), 0);
}

TEST_F(StatisticsTest, Stat_GetFullLocation)
{
    Stat stat(StatType::Mined, ResourceLocation("minecraft:stone"));
    ResourceLocation location = stat.getFullLocation();
    EXPECT_EQ(location.toString(), "minecraft.mined:minecraft:stone");
}

TEST_F(StatisticsTest, Stat_Equality)
{
    Stat stat1(StatType::Mined, ResourceLocation("minecraft:stone"));
    Stat stat2(StatType::Mined, ResourceLocation("minecraft:stone"));
    Stat stat3(StatType::Mined, ResourceLocation("minecraft:dirt"));
    Stat stat4(StatType::Crafted, ResourceLocation("minecraft:stone"));

    stat1.increment(100);
    stat2.increment(50); // 不同的值

    // 相同类型和ID应该相等（忽略值）
    EXPECT_EQ(stat1, stat2);
    EXPECT_NE(stat1, stat3); // 不同的ID
    EXPECT_NE(stat1, stat4); // 不同的类型
}

TEST_F(StatisticsTest, Stat_OverflowProtection)
{
    Stat stat(StatType::Custom, ResourceLocation("minecraft:test"));

    // 测试正溢出
    stat.setValue(INT64_MAX);
    stat.increment(1);
    EXPECT_EQ(stat.getValue(), INT64_MAX); // 应该被限制在最大值

    // 测试负溢出
    stat.setValue(INT64_MIN);
    stat.increment(-1);
    EXPECT_EQ(stat.getValue(), INT64_MIN); // 应该被限制在最小值
}

// ========== StatRegistry 测试 ==========

TEST_F(StatisticsTest, Registry_HasBuiltinStats)
{
    // 手动注册一些统计用于测试
    StatRegistry::instance().registerMinedStat(ResourceLocation("minecraft:stone"));
    StatRegistry::instance().registerMinedStat(ResourceLocation("minecraft:dirt"));
    StatRegistry::instance().registerCustomStat(ResourceLocation("minecraft:play_one_minute"));
    StatRegistry::instance().registerCustomStat(ResourceLocation("minecraft:jump"));
    StatRegistry::instance().registerCustomStat(ResourceLocation("minecraft:deaths"));

    // 检查方块挖掘统计
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Mined, ResourceLocation("minecraft:stone")));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Mined, ResourceLocation("minecraft:dirt")));

    // 检查自定义统计
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:play_one_minute")));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:jump")));
    EXPECT_TRUE(StatRegistry::instance().hasStat(StatType::Custom, ResourceLocation("minecraft:deaths")));
}

TEST_F(StatisticsTest, Registry_GetAllStatIds)
{
    // 注册一些统计用于测试
    StatRegistry::instance().registerMinedStat(ResourceLocation("minecraft:stone"));
    StatRegistry::instance().registerMinedStat(ResourceLocation("minecraft:dirt"));

    auto ids = StatRegistry::instance().getAllStatIds();
    EXPECT_FALSE(ids.empty());

    // 检查是否包含一些基本统计
    bool hasStoneMined = false;
    for (const auto& id : ids) {
        if (id.toString() == "minecraft.mined:minecraft:stone") {
            hasStoneMined = true;
            break;
        }
    }
    EXPECT_TRUE(hasStoneMined);
}

TEST_F(StatisticsTest, Registry_GetStatIdsByType)
{
    // 注册一些统计用于测试
    StatRegistry::instance().registerMinedStat(ResourceLocation("minecraft:stone"));
    StatRegistry::instance().registerCustomStat(ResourceLocation("minecraft:jump"));

    auto minedStats = StatRegistry::instance().getStatIdsByType(StatType::Mined);
    EXPECT_FALSE(minedStats.empty());

    auto customStats = StatRegistry::instance().getStatIdsByType(StatType::Custom);
    EXPECT_FALSE(customStats.empty());

    // 检查 mined 统计都以正确的格式开头
    for (const auto& id : minedStats) {
        std::string idStr = id.toString();
        EXPECT_TRUE(idStr.find("minecraft.mined:") == 0);
    }

    // 检查 custom 统计都以正确的格式开头
    for (const auto& id : customStats) {
        std::string idStr = id.toString();
        EXPECT_TRUE(idStr.find("minecraft.custom:") == 0);
    }
}

TEST_F(StatisticsTest, Registry_GetMinedStatId)
{
    ResourceLocation blockId("minecraft:stone");
    ResourceLocation statId = StatRegistry::instance().getMinedStatId(blockId);
    EXPECT_EQ(statId.toString(), "minecraft.mined:minecraft:stone");
}

// ========== StatisticsManager 测试 ==========

TEST_F(StatisticsTest, Manager_GetSet)
{
    StatisticsManager manager;

    // 初始值应该是 0
    EXPECT_EQ(manager.getValue(StatType::Mined, ResourceLocation("minecraft:stone")), 0);

    // 设置值
    manager.setValue(StatType::Mined, ResourceLocation("minecraft:stone"), 100);
    EXPECT_EQ(manager.getValue(StatType::Mined, ResourceLocation("minecraft:stone")), 100);

    // 覆盖值
    manager.setValue(StatType::Mined, ResourceLocation("minecraft:stone"), 200);
    EXPECT_EQ(manager.getValue(StatType::Mined, ResourceLocation("minecraft:stone")), 200);
}

TEST_F(StatisticsTest, Manager_Increment)
{
    StatisticsManager manager;

    manager.increment(StatType::Mined, ResourceLocation("minecraft:stone"));
    EXPECT_EQ(manager.getValue(StatType::Mined, ResourceLocation("minecraft:stone")), 1);

    manager.increment(StatType::Mined, ResourceLocation("minecraft:stone"), 5);
    EXPECT_EQ(manager.getValue(StatType::Mined, ResourceLocation("minecraft:stone")), 6);

    // 负增量
    manager.increment(StatType::Mined, ResourceLocation("minecraft:stone"), -3);
    EXPECT_EQ(manager.getValue(StatType::Mined, ResourceLocation("minecraft:stone")), 3);
}

TEST_F(StatisticsTest, Manager_Decrement)
{
    StatisticsManager manager;

    manager.setValue(StatType::Mined, ResourceLocation("minecraft:stone"), 100);
    manager.decrement(StatType::Mined, ResourceLocation("minecraft:stone"), 10);
    EXPECT_EQ(manager.getValue(StatType::Mined, ResourceLocation("minecraft:stone")), 90);
}

TEST_F(StatisticsTest, Manager_Reset)
{
    StatisticsManager manager;

    manager.setValue(StatType::Mined, ResourceLocation("minecraft:stone"), 100);
    manager.setValue(StatType::Mined, ResourceLocation("minecraft:dirt"), 50);

    // 重置单个统计
    manager.reset(StatType::Mined, ResourceLocation("minecraft:stone"));
    EXPECT_EQ(manager.getValue(StatType::Mined, ResourceLocation("minecraft:stone")), 0);
    EXPECT_EQ(manager.getValue(StatType::Mined, ResourceLocation("minecraft:dirt")), 50);

    // 重置所有
    manager.resetAll();
    EXPECT_EQ(manager.getValue(StatType::Mined, ResourceLocation("minecraft:dirt")), 0);
}

TEST_F(StatisticsTest, Manager_HasStat)
{
    StatisticsManager manager;

    EXPECT_FALSE(manager.hasStat(StatType::Mined, ResourceLocation("minecraft:stone")));

    manager.increment(StatType::Mined, ResourceLocation("minecraft:stone"));
    EXPECT_TRUE(manager.hasStat(StatType::Mined, ResourceLocation("minecraft:stone")));
}

TEST_F(StatisticsTest, Manager_GetAllStats)
{
    StatisticsManager manager;

    manager.setValue(StatType::Mined, ResourceLocation("minecraft:stone"), 100);
    manager.setValue(StatType::Mined, ResourceLocation("minecraft:dirt"), 50);
    manager.setValue(StatType::Custom, ResourceLocation("minecraft:jump"), 25);

    auto allStats = manager.getAllStats();
    EXPECT_EQ(allStats.size(), 3);

    // 检查每个统计
    bool hasStone = false, hasDirt = false, hasJump = false;
    for (const auto& [id, value] : allStats) {
        if (id.toString() == "minecraft.mined:minecraft:stone") {
            hasStone = true;
            EXPECT_EQ(value, 100);
        } else if (id.toString() == "minecraft.mined:minecraft:dirt") {
            hasDirt = true;
            EXPECT_EQ(value, 50);
        } else if (id.toString() == "minecraft.custom:minecraft:jump") {
            hasJump = true;
            EXPECT_EQ(value, 25);
        }
    }
    EXPECT_TRUE(hasStone);
    EXPECT_TRUE(hasDirt);
    EXPECT_TRUE(hasJump);
}

TEST_F(StatisticsTest, Manager_GetStatsByType)
{
    StatisticsManager manager;

    manager.setValue(StatType::Mined, ResourceLocation("minecraft:stone"), 100);
    manager.setValue(StatType::Mined, ResourceLocation("minecraft:dirt"), 50);
    manager.setValue(StatType::Custom, ResourceLocation("minecraft:jump"), 25);

    auto minedStats = manager.getStatsByType(StatType::Mined);
    EXPECT_EQ(minedStats.size(), 2);

    auto customStats = manager.getStatsByType(StatType::Custom);
    EXPECT_EQ(customStats.size(), 1);

    auto craftedStats = manager.getStatsByType(StatType::Crafted);
    EXPECT_EQ(craftedStats.size(), 0); // 没有 crafted 统计
}

TEST_F(StatisticsTest, Manager_ConvenienceMethods)
{
    StatisticsManager manager;

    manager.incrementMined(ResourceLocation("minecraft:stone"));
    EXPECT_EQ(manager.getValue(StatType::Mined, ResourceLocation("minecraft:stone")), 1);

    manager.incrementCrafted(ResourceLocation("minecraft:diamond_sword"), 3);
    EXPECT_EQ(manager.getValue(StatType::Crafted, ResourceLocation("minecraft:diamond_sword")), 3);

    manager.incrementUsed(ResourceLocation("minecraft:diamond_pickaxe"));
    EXPECT_EQ(manager.getValue(StatType::Used, ResourceLocation("minecraft:diamond_pickaxe")), 1);

    manager.incrementBroken(ResourceLocation("minecraft:diamond_pickaxe"));
    EXPECT_EQ(manager.getValue(StatType::Broken, ResourceLocation("minecraft:diamond_pickaxe")), 1);

    manager.incrementPickedUp(ResourceLocation("minecraft:cobblestone"), 10);
    EXPECT_EQ(manager.getValue(StatType::PickedUp, ResourceLocation("minecraft:cobblestone")), 10);

    manager.incrementDropped(ResourceLocation("minecraft:cobblestone"), 5);
    EXPECT_EQ(manager.getValue(StatType::Dropped, ResourceLocation("minecraft:cobblestone")), 5);

    manager.incrementKilled(ResourceLocation("minecraft:zombie"));
    EXPECT_EQ(manager.getValue(StatType::Killed, ResourceLocation("minecraft:zombie")), 1);

    manager.incrementKilledBy(ResourceLocation("minecraft:creeper"));
    EXPECT_EQ(manager.getValue(StatType::KilledBy, ResourceLocation("minecraft:creeper")), 1);

    manager.incrementCustom(ResourceLocation("minecraft:jump"), 50);
    EXPECT_EQ(manager.getValue(StatType::Custom, ResourceLocation("minecraft:jump")), 50);
}

TEST_F(StatisticsTest, Manager_DirtyFlag)
{
    StatisticsManager manager;

    EXPECT_FALSE(manager.isDirty());

    manager.setValue(StatType::Mined, ResourceLocation("minecraft:stone"), 100);
    EXPECT_TRUE(manager.isDirty());

    manager.clearDirty();
    EXPECT_FALSE(manager.isDirty());

    manager.increment(StatType::Mined, ResourceLocation("minecraft:stone"));
    EXPECT_TRUE(manager.isDirty());

    manager.clearDirty();
    manager.reset(StatType::Mined, ResourceLocation("minecraft:stone"));
    EXPECT_TRUE(manager.isDirty());
}

TEST_F(StatisticsTest, Manager_ForEach)
{
    StatisticsManager manager;

    manager.setValue(StatType::Mined, ResourceLocation("minecraft:stone"), 100);
    manager.setValue(StatType::Mined, ResourceLocation("minecraft:dirt"), 50);

    i64 total = 0;
    manager.forEach([&total](const ResourceLocation& id, i64 value) {
        total += value;
        return true;
    });

    EXPECT_EQ(total, 150);
}

TEST_F(StatisticsTest, Manager_ForEach_EarlyExit)
{
    StatisticsManager manager;

    manager.setValue(StatType::Mined, ResourceLocation("minecraft:stone"), 100);
    manager.setValue(StatType::Mined, ResourceLocation("minecraft:dirt"), 50);
    manager.setValue(StatType::Mined, ResourceLocation("minecraft:cobblestone"), 25);

    i32 count = 0;
    manager.forEach([&count](const ResourceLocation& id, i64 value) {
        count++;
        return count < 2; // 只遍历前两个
    });

    EXPECT_EQ(count, 2);
}

// ========== NBT 序列化测试 ==========

TEST_F(StatisticsTest, Manager_SerializeDeserialize)
{
    // 注册统计以便序列化可以解析
    StatRegistry::instance().registerMinedStat(ResourceLocation("minecraft:stone"));
    StatRegistry::instance().registerMinedStat(ResourceLocation("minecraft:dirt"));
    StatRegistry::instance().registerCraftedStat(ResourceLocation("minecraft:diamond_sword"));
    StatRegistry::instance().registerCustomStat(ResourceLocation("minecraft:jump"));
    StatRegistry::instance().registerKilledStat(ResourceLocation("minecraft:zombie"));

    StatisticsManager original;
    original.setValue(StatType::Mined, ResourceLocation("minecraft:stone"), 1234);
    original.setValue(StatType::Mined, ResourceLocation("minecraft:dirt"), 567);
    original.setValue(StatType::Crafted, ResourceLocation("minecraft:diamond_sword"), 5);
    original.setValue(StatType::Custom, ResourceLocation("minecraft:jump"), 1500);
    original.setValue(StatType::Killed, ResourceLocation("minecraft:zombie"), 100);

    // 序列化
    nbt::tags::compound_tag nbt = original.toNbt();

    // 反序列化
    auto result = StatisticsManager::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    StatisticsManager& restored = result.value();

    // 验证数据
    EXPECT_EQ(restored.getValue(StatType::Mined, ResourceLocation("minecraft:stone")), 1234);
    EXPECT_EQ(restored.getValue(StatType::Mined, ResourceLocation("minecraft:dirt")), 567);
    EXPECT_EQ(restored.getValue(StatType::Crafted, ResourceLocation("minecraft:diamond_sword")), 5);
    EXPECT_EQ(restored.getValue(StatType::Custom, ResourceLocation("minecraft:jump")), 1500);
    EXPECT_EQ(restored.getValue(StatType::Killed, ResourceLocation("minecraft:zombie")), 100);
}

TEST_F(StatisticsTest, Manager_SerializeEmpty)
{
    StatisticsManager empty;
    nbt::tags::compound_tag nbt = empty.toNbt();

    auto result = StatisticsManager::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    StatisticsManager& restored = result.value();
    EXPECT_EQ(restored.getAllStats().size(), 0);
}

TEST_F(StatisticsTest, Manager_SerializeSkipZeroValues)
{
    // 注册统计
    StatRegistry::instance().registerMinedStat(ResourceLocation("minecraft:stone"));

    StatisticsManager manager;
    manager.setValue(StatType::Mined, ResourceLocation("minecraft:stone"), 100);
    manager.setValue(StatType::Mined, ResourceLocation("minecraft:dirt"), 0);            // 零值
    manager.setValue(StatType::Crafted, ResourceLocation("minecraft:diamond_sword"), 0); // 零值

    nbt::tags::compound_tag nbt = manager.toNbt();

    auto result = StatisticsManager::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    StatisticsManager& restored = result.value();
    EXPECT_EQ(restored.getAllStats().size(), 1); // 只有非零值
    EXPECT_EQ(restored.getValue(StatType::Mined, ResourceLocation("minecraft:stone")), 100);
    EXPECT_EQ(restored.getValue(StatType::Mined, ResourceLocation("minecraft:dirt")), 0);
}

TEST_F(StatisticsTest, Manager_LargeValues)
{
    StatisticsManager manager;
    manager.setValue(StatType::Custom, ResourceLocation("minecraft:play_one_minute"), INT64_MAX);

    nbt::tags::compound_tag nbt = manager.toNbt();
    auto result = StatisticsManager::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    EXPECT_EQ(result.value().getValue(StatType::Custom, ResourceLocation("minecraft:play_one_minute")), INT64_MAX);
}

// main 函数由 gtest_main 库提供
