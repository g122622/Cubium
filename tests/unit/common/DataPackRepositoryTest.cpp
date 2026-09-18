#include "common/resource/repository/DataPackRepository.hpp"
#include "common/TempDirHelper.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace mc::resource;

namespace {

void writeTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << text;
}

void cleanupTempDir(const std::filesystem::path& dir)
{
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

/// 创建包含单个数据包的临时目录
std::filesystem::path createSinglePackDir()
{
    const auto dir = mc::test::makeUniqueTestDir("mc_datapack_list_test_");
    auto packDir = dir / "example_pack";
    std::filesystem::create_directories(packDir / "data" / "minecraft" / "loot_tables" / "blocks");

    writeTextFile(packDir / "pack.mcmeta", R"({"pack":{"pack_format":6,"description":"test"}})");
    writeTextFile(packDir / "data" / "minecraft" / "loot_tables" / "blocks" / "stone.json",
        R"({"type":"minecraft:block","pools":[]})");

    return dir;
}

/// 创建包含两个数据包的临时目录（高优先级包和低优先级包）
/// 低优先级包定义同名标签但内容不同，用于测试多数据包合并
/// 使用显式优先级设置来确保确定性排序（不依赖文件系统迭代顺序）
std::filesystem::path createMultiPackDir()
{
    const auto dir = mc::test::makeUniqueTestDir("mc_datapack_multi_test_");

    // 低优先级数据包（将设置较低的 priority 值）
    auto lowPackDir = dir / "low_priority_pack";
    std::filesystem::create_directories(lowPackDir / "data" / "minecraft" / "tags" / "functions");
    std::filesystem::create_directories(lowPackDir / "data" / "minecraft" / "functions");
    writeTextFile(lowPackDir / "pack.mcmeta", R"({"pack":{"pack_format":41,"description":"low priority pack"}})");
    writeTextFile(lowPackDir / "data" / "minecraft" / "tags" / "functions" / "tick.json",
        R"({"values": ["minecraft:low_func_a", "minecraft:low_func_b"]})");
    writeTextFile(lowPackDir / "data" / "minecraft" / "functions" / "low_func_a.mcfunction", "say low_a");
    writeTextFile(lowPackDir / "data" / "minecraft" / "functions" / "low_func_b.mcfunction", "say low_b");

    // 高优先级数据包（将设置较高的 priority 值）
    auto highPackDir = dir / "high_priority_pack";
    std::filesystem::create_directories(highPackDir / "data" / "minecraft" / "tags" / "functions");
    std::filesystem::create_directories(highPackDir / "data" / "minecraft" / "functions");
    writeTextFile(highPackDir / "pack.mcmeta", R"({"pack":{"pack_format":41,"description":"high priority pack"}})");
    writeTextFile(highPackDir / "data" / "minecraft" / "tags" / "functions" / "tick.json",
        R"({"values": ["minecraft:high_func"]})");
    writeTextFile(highPackDir / "data" / "minecraft" / "functions" / "high_func.mcfunction", "say high");

    return dir;
}

/// 向 DataPackRepository 添加两个数据包并设置显式优先级
/// low_priority_pack 获得 priority=0，high_priority_pack 获得 priority=1
/// getEnabledPackInfos() 按 priority 降序排列，因此 high_priority_pack 排在前面
void addMultiPacksWithExplicitPriority(DataPackRepository& dataPacks, const std::filesystem::path& dir)
{
    auto lowPackPath = dir / "low_priority_pack";
    auto highPackPath = dir / "high_priority_pack";

    // 显式添加并设置优先级，确保排序确定性
    dataPacks.addPack(lowPackPath, true, 0);  // 低优先级
    dataPacks.addPack(highPackPath, true, 1); // 高优先级
}

} // namespace

// ========== 原有测试 ==========

TEST(DataPackRepositoryTest, ScanDirectoryFindsFolderPack)
{
    const auto dir = createSinglePackDir();
    DataPackRepository list;

    const auto result = list.scanDirectory(dir);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), static_cast<size_t>(1));
    EXPECT_EQ(list.packCount(), static_cast<size_t>(1));
}

TEST(DataPackRepositoryTest, ReadResourceUsesServerDataRoot)
{
    const auto dir = createSinglePackDir();
    DataPackRepository list;
    ASSERT_TRUE(list.scanDirectory(dir).success());

    const auto readResult = list.readTextResource("minecraft/loot_tables/blocks/stone.json");
    ASSERT_TRUE(readResult.success());
    EXPECT_NE(readResult.value().find("\"minecraft:block\""), std::string::npos);
}

// ========== readAllResourceVersions 测试 ==========

TEST(DataPackRepositoryTest, ReadAllResourceVersions_SinglePack)
{
    const auto dir = createSinglePackDir();
    DataPackRepository list;
    ASSERT_TRUE(list.scanDirectory(dir).success());

    const auto result = list.readAllResourceVersions("minecraft/loot_tables/blocks/stone.json");
    ASSERT_TRUE(result.success());

    // 单个数据包，应该只有一个版本
    EXPECT_EQ(result.value().size(), static_cast<size_t>(1));
    EXPECT_NE(result.value()[0].content.find("\"minecraft:block\""), std::string::npos);
    EXPECT_EQ(result.value()[0].packName, "example_pack");
}

TEST(DataPackRepositoryTest, ReadAllResourceVersions_ResourceNotFound)
{
    const auto dir = createSinglePackDir();
    DataPackRepository list;
    ASSERT_TRUE(list.scanDirectory(dir).success());

    const auto result = list.readAllResourceVersions("minecraft/nonexistent/resource.json");
    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.error().code(), mc::ErrorCode::ResourceNotFound);
}

TEST(DataPackRepositoryTest, ReadAllResourceVersions_MultiplePacks_HighPriorityFirst)
{
    const auto dir = createMultiPackDir();
    DataPackRepository list;
    addMultiPacksWithExplicitPriority(list, dir);

    // 两个数据包都定义了 minecraft/tags/functions/tick.json
    const auto result = list.readAllResourceVersions("minecraft/tags/functions/tick.json");
    ASSERT_TRUE(result.success());

    // 应该有两个版本
    EXPECT_EQ(result.value().size(), static_cast<size_t>(2));

    // 高优先级包排在前面（getEnabledPackInfos 按 priority 降序排列）
    EXPECT_EQ(result.value()[0].packName, "high_priority_pack");
    EXPECT_NE(result.value()[0].content.find("minecraft:high_func"), std::string::npos);

    // 低优先级包排在后面
    EXPECT_EQ(result.value()[1].packName, "low_priority_pack");
    EXPECT_NE(result.value()[1].content.find("minecraft:low_func_a"), std::string::npos);
}

// ========== listResourceStacks 测试 ==========

TEST(DataPackRepositoryTest, ListResourceStacks_SinglePack)
{
    const auto dir = createSinglePackDir();
    DataPackRepository list;
    ASSERT_TRUE(list.scanDirectory(dir).success());

    const auto result = list.listResourceStacks("minecraft/loot_tables/blocks", ".json");
    ASSERT_TRUE(result.success());

    // 应该只有一个资源路径
    EXPECT_GE(result.value().size(), static_cast<size_t>(1));

    // 找到 stone.json
    bool foundStone = false;
    for (const auto& [path, versions] : result.value()) {
        if (path.find("stone.json") != std::string::npos) {
            foundStone = true;
            EXPECT_EQ(versions.size(), static_cast<size_t>(1));
            EXPECT_NE(versions[0].content.find("\"minecraft:block\""), std::string::npos);
            break;
        }
    }
    EXPECT_TRUE(foundStone);
}

TEST(DataPackRepositoryTest, ListResourceStacks_EmptyResult)
{
    const auto dir = createSinglePackDir();
    DataPackRepository list;
    ASSERT_TRUE(list.scanDirectory(dir).success());

    // 查询不存在的目录，应该返回空映射（非错误）
    const auto result = list.listResourceStacks("minecraft/nonexistent_dir", ".json");
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().empty());
}

TEST(DataPackRepositoryTest, ListResourceStacks_MultiplePacksMerge_HighPriorityFirst)
{
    const auto dir = createMultiPackDir();
    DataPackRepository list;
    addMultiPacksWithExplicitPriority(list, dir);

    const auto result = list.listResourceStacks("minecraft/tags/functions", ".json");
    ASSERT_TRUE(result.success());

    // 找到 tick.json
    bool foundTick = false;
    for (const auto& [path, versions] : result.value()) {
        if (path.find("tick.json") != std::string::npos) {
            foundTick = true;
            // 两个数据包都定义了 tick.json
            EXPECT_EQ(versions.size(), static_cast<size_t>(2));

            // 高优先级包排在前面
            EXPECT_EQ(versions[0].packName, "high_priority_pack");
            EXPECT_NE(versions[0].content.find("minecraft:high_func"), std::string::npos);

            // 低优先级包排在后面
            EXPECT_EQ(versions[1].packName, "low_priority_pack");
            EXPECT_NE(versions[1].content.find("minecraft:low_func_a"), std::string::npos);
            break;
        }
    }
    EXPECT_TRUE(foundTick);
}
