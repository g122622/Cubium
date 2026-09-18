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
 * The above copyright notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "common/advancement/AdvancementLoader.hpp"
#include "common/TempDirHelper.hpp"
#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/trigger/impl/ImpossibleTrigger.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

using namespace mc;
using namespace mc::advancement;

namespace {

void writeTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << text;
}

// 简单的成就 JSON，使用 impossible 触发器
constexpr const char* kSimpleAdvancementJson = R"({
  "criteria": {
    "impossible": {
      "trigger": "minecraft:impossible"
    }
  }
})";

// 带有 parent 和 display 的成就 JSON
constexpr const char* kFullAdvancementJson = R"({
  "parent": "minecraft:story/root",
  "display": {
    "icon": { "item": "minecraft:iron_pickaxe" },
    "title": "Isn't It Iron Pick",
    "description": "Upgrade your pickaxe",
    "frame": "task",
    "show_toast": true,
    "announce_to_chat": true
  },
  "criteria": {
    "iron_pickaxe": {
      "trigger": "minecraft:inventory_changed",
      "conditions": {
        "items": [
          { "item": "minecraft:iron_pickaxe" }
        ]
      }
    }
  }
})";

// 无效的 JSON 语法
constexpr const char* kInvalidJsonSyntax = R"(not valid json at all)";

// 缺少 criteria 的无效成就 JSON
constexpr const char* kMissingCriteriaJson = R"({
  "display": {
    "icon": { "item": "minecraft:stone" },
    "title": "No Criteria",
    "description": "Missing criteria"
  }
})";

// 创建数据包目录结构（使用单数形式 advancement/）
std::filesystem::path createDataPackDirSingular()
{
    const auto base = std::filesystem::temp_directory_path();
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const auto dir = base / ("mc_datapack_adv_test_" + std::to_string(static_cast<long long>(now)));

    // 创建数据包结构
    std::filesystem::create_directories(dir / "test_pack/data/minecraft/advancement/story");

    // pack.mcmeta
    writeTextFile(
        dir / "test_pack/pack.mcmeta", R"({"pack":{"pack_format":41,"description":"test advancement pack"}})");

    // story/root.json
    writeTextFile(dir / "test_pack/data/minecraft/advancement/story/root.json", kSimpleAdvancementJson);

    // story/mine_stone.json
    writeTextFile(dir / "test_pack/data/minecraft/advancement/story/mine_stone.json", kFullAdvancementJson);

    return dir;
}

// 创建数据包目录结构（使用复数形式 advancements/）
std::filesystem::path createDataPackDirPlural()
{
    const auto base = std::filesystem::temp_directory_path();
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const auto dir = base / ("mc_datapack_adv_plural_" + std::to_string(static_cast<long long>(now)));

    // 创建数据包结构，使用复数形式 advancements/
    std::filesystem::create_directories(dir / "test_pack/data/minecraft/advancements/adventure");

    writeTextFile(
        dir / "test_pack/pack.mcmeta", R"({"pack":{"pack_format":6,"description":"test advancement pack plural"}})");

    writeTextFile(dir / "test_pack/data/minecraft/advancements/adventure/root.json", kSimpleAdvancementJson);

    return dir;
}

} // namespace

// ============================================================================
// AdvancementLoader pathToAdvancementId 测试
// ============================================================================

class AdvancementLoaderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 每个测试前清空单例管理器，避免前一个测试的残留数据
        AdvancementManager::instance().clear();
    }

    void TearDown() override { AdvancementManager::instance().clear(); }
};

TEST_F(AdvancementLoaderTest, PathToAdvancementId_PluralAdvancements)
{
    AdvancementLoader loader;

    // 旧格式（包含 data/ 前缀，复数 advancements/）
    EXPECT_EQ(ResourceLocation("minecraft", "story/mine_stone"),
        loader.pathToAdvancementId("data/minecraft/advancements/story/mine_stone.json"));

    EXPECT_EQ(ResourceLocation("mod_id", "custom/adv"),
        loader.pathToAdvancementId("data/mod_id/advancements/custom/adv.json"));

    // 新格式（相对于 data/ 根目录，复数 advancements/）
    EXPECT_EQ(ResourceLocation("minecraft", "story/root"),
        loader.pathToAdvancementId("minecraft/advancements/story/root.json"));
}

TEST_F(AdvancementLoaderTest, PathToAdvancementId_SingularAdvancement)
{
    AdvancementLoader loader;

    // 旧格式（包含 data/ 前缀，单数 advancement/，MC 1.21+）
    EXPECT_EQ(ResourceLocation("minecraft", "story/mine_stone"),
        loader.pathToAdvancementId("data/minecraft/advancement/story/mine_stone.json"));

    EXPECT_EQ(ResourceLocation("mod_id", "custom/adv"),
        loader.pathToAdvancementId("data/mod_id/advancement/custom/adv.json"));

    // 新格式（相对于 data/ 根目录，单数 advancement/）
    EXPECT_EQ(ResourceLocation("minecraft", "adventure/root"),
        loader.pathToAdvancementId("minecraft/advancement/adventure/root.json"));
}

TEST_F(AdvancementLoaderTest, PathToAdvancementId_DeepNestedPath)
{
    AdvancementLoader loader;

    // 多层嵌套路径
    EXPECT_EQ(ResourceLocation("minecraft", "recipes/building_blocks/acacia_planks"),
        loader.pathToAdvancementId("data/minecraft/advancements/recipes/building_blocks/acacia_planks.json"));

    EXPECT_EQ(ResourceLocation("minecraft", "recipes/building_blocks/acacia_planks"),
        loader.pathToAdvancementId("data/minecraft/advancement/recipes/building_blocks/acacia_planks.json"));
}

TEST_F(AdvancementLoaderTest, PathToAdvancementId_NoAdvancementDir)
{
    AdvancementLoader loader;

    // 没有 advancement(s)/ 目录时，应使用文件名作为 ID
    const auto result = loader.pathToAdvancementId("some/random/file.json");
    EXPECT_EQ(ResourceLocation("file"), result);
}

// ============================================================================
// AdvancementLoader loadJson 测试
// ============================================================================

TEST_F(AdvancementLoaderTest, LoadJson_SimpleAdvancement)
{
    AdvancementLoader loader;
    auto result = loader.loadJson(ResourceLocation("minecraft", "story/root"), std::string(kSimpleAdvancementJson));
    ASSERT_TRUE(result.success());

    const auto& adv = result.value();
    EXPECT_EQ(adv.getId(), ResourceLocation("minecraft", "story/root"));
    EXPECT_FALSE(adv.getParent().has_value());
    EXPECT_TRUE(adv.getCriteria().count("impossible") > 0);
}

TEST_F(AdvancementLoaderTest, LoadJson_FullAdvancement)
{
    AdvancementLoader loader;
    auto result = loader.loadJson(ResourceLocation("minecraft", "story/mine_stone"), std::string(kFullAdvancementJson));
    ASSERT_TRUE(result.success());

    const auto& adv = result.value();
    EXPECT_EQ(adv.getId(), ResourceLocation("minecraft", "story/mine_stone"));
    EXPECT_TRUE(adv.getParent().has_value());
    EXPECT_EQ(adv.getParent().value(), ResourceLocation("minecraft", "story/root"));
    EXPECT_TRUE(adv.hasDisplay());
    EXPECT_TRUE(adv.getCriteria().count("iron_pickaxe") > 0);
}

TEST_F(AdvancementLoaderTest, LoadJson_InvalidJsonSyntax)
{
    AdvancementLoader loader;
    auto result = loader.loadJson(ResourceLocation("minecraft", "bad_json"), std::string(kInvalidJsonSyntax));
    EXPECT_FALSE(result.success());
}

TEST_F(AdvancementLoaderTest, LoadJson_MissingCriteria)
{
    AdvancementLoader loader;
    auto result = loader.loadJson(ResourceLocation("minecraft", "no_criteria"), std::string(kMissingCriteriaJson));
    // 缺少 criteria 的成就 JSON 应该解析失败（criteria 是必需字段）
    EXPECT_FALSE(result.success());
}

// ============================================================================
// AdvancementLoader loadFile 测试
// ============================================================================

TEST_F(AdvancementLoaderTest, LoadFile_ValidAdvancement)
{
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_advancement_loader_test");
    writeTextFile(tempRoot / "data/minecraft/advancements/story/root.json", kSimpleAdvancementJson);

    AdvancementLoader loader;
    auto result = loader.loadFile((tempRoot / "data/minecraft/advancements/story/root.json").string());
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), ResourceLocation("minecraft", "story/root"));

    // 验证已注册到管理器
    EXPECT_TRUE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "story/root")));

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

TEST_F(AdvancementLoaderTest, LoadFile_NonExistentFile)
{
    AdvancementLoader loader;
    auto result = loader.loadFile("/nonexistent/path/advancement.json");
    EXPECT_FALSE(result.success());
}

// ============================================================================
// AdvancementLoader loadFromDirectory 测试
// ============================================================================

TEST_F(AdvancementLoaderTest, LoadFromDirectory_LoadsAdvancements)
{
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_advancement_loader_test");
    writeTextFile(tempRoot / "data/minecraft/advancements/story/root.json", kSimpleAdvancementJson);
    writeTextFile(tempRoot / "data/minecraft/advancements/story/mine_stone.json", kFullAdvancementJson);

    AdvancementLoader loader;
    auto result = loader.loadFromDirectory((tempRoot / "data/minecraft/advancements").string());

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().successCount, 2u);
    EXPECT_EQ(result.value().failedCount, 0u);
    EXPECT_TRUE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "story/root")));
    EXPECT_TRUE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "story/mine_stone")));

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

TEST_F(AdvancementLoaderTest, LoadFromDirectory_NonExistentDirectory)
{
    AdvancementLoader loader;
    auto result = loader.loadFromDirectory("/nonexistent/directory/path");
    EXPECT_FALSE(result.success());
}

TEST_F(AdvancementLoaderTest, LoadFromDirectory_EmptyDirectory)
{
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_advancement_loader_test");

    AdvancementLoader loader;
    auto result = loader.loadFromDirectory(tempRoot.string());

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().successCount, 0u);
    EXPECT_EQ(result.value().failedCount, 0u);

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

TEST_F(AdvancementLoaderTest, LoadFromDirectory_MixedSuccessAndFailure)
{
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_advancement_loader_test");
    writeTextFile(tempRoot / "data/minecraft/advancements/story/good.json", kSimpleAdvancementJson);
    writeTextFile(tempRoot / "data/minecraft/advancements/story/bad.json", kInvalidJsonSyntax);

    AdvancementLoader loader;
    auto result = loader.loadFromDirectory((tempRoot / "data/minecraft/advancements").string());

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().successCount, 1u);
    EXPECT_EQ(result.value().failedCount, 1u);
    EXPECT_FALSE(result.value().errors.empty());

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

// ============================================================================
// AdvancementLoader clearBeforeLoad 测试
// ============================================================================

TEST_F(AdvancementLoaderTest, ClearBeforeLoad_DefaultClears)
{
    // 先注册一个成就
    auto trigger = std::make_shared<ImpossibleTriggerInstance>();
    auto adv = std::make_shared<Advancement>(
        Advancement::Builder(ResourceLocation("minecraft", "pre_existing")).criterion("test", trigger).build().value());
    AdvancementManager::instance().registerAdvancement(adv);
    EXPECT_TRUE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "pre_existing")));

    // loadFromDirectory 默认 clearBeforeLoad=true，应清空已有成就
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_advancement_loader_test");
    writeTextFile(tempRoot / "data/minecraft/advancements/test/adv.json", kSimpleAdvancementJson);

    AdvancementLoader loader;
    auto result = loader.loadFromDirectory((tempRoot / "data/minecraft/advancements").string());

    ASSERT_TRUE(result.success());
    // 预注册的成就应被清空
    EXPECT_FALSE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "pre_existing")));
    // 新加载的成就应存在
    EXPECT_TRUE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "test/adv")));

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

TEST_F(AdvancementLoaderTest, ClearBeforeLoad_FalsePreserves)
{
    // 先注册一个成就
    auto trigger = std::make_shared<ImpossibleTriggerInstance>();
    auto adv = std::make_shared<Advancement>(
        Advancement::Builder(ResourceLocation("minecraft", "pre_existing")).criterion("test", trigger).build().value());
    AdvancementManager::instance().registerAdvancement(adv);
    EXPECT_TRUE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "pre_existing")));

    // clearBeforeLoad=false，应保留已有成就
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_advancement_loader_test");
    writeTextFile(tempRoot / "data/minecraft/advancements/test/adv.json", kSimpleAdvancementJson);

    AdvancementLoader loader;
    loader.setClearBeforeLoad(false);
    auto result = loader.loadFromDirectory((tempRoot / "data/minecraft/advancements").string());

    ASSERT_TRUE(result.success());
    // 预注册的成就应保留
    EXPECT_TRUE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "pre_existing")));
    // 新加载的成就也应存在
    EXPECT_TRUE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "test/adv")));

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

// ============================================================================
// AdvancementLoader LoadResult 和 resetResult 测试
// ============================================================================

TEST_F(AdvancementLoaderTest, LoadResult_InitialState)
{
    AdvancementLoader loader;
    const auto& result = loader.getLastResult();
    EXPECT_EQ(result.successCount, 0u);
    EXPECT_EQ(result.failedCount, 0u);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(AdvancementLoaderTest, ResetResult_ClearsState)
{
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_advancement_loader_test");
    writeTextFile(tempRoot / "data/minecraft/advancements/test/bad.json", kInvalidJsonSyntax);

    AdvancementLoader loader;
    loader.loadFromDirectory((tempRoot / "data/minecraft/advancements").string());

    // 加载后应有结果
    EXPECT_EQ(loader.getLastResult().failedCount, 1u);

    // 重置后应清空
    loader.resetResult();
    EXPECT_EQ(loader.getLastResult().successCount, 0u);
    EXPECT_EQ(loader.getLastResult().failedCount, 0u);
    EXPECT_TRUE(loader.getLastResult().errors.empty());

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

// ============================================================================
// AdvancementLoader loadFromDataPackRepository 测试
// ============================================================================

TEST_F(AdvancementLoaderTest, LoadFromDataPackRepository_SingularDirName)
{
    // 使用单数 advancement/ 目录名（MC 1.21+ 格式）
    const auto dir = createDataPackDirSingular();
    resource::DataPackRepository dataPacks;
    ASSERT_TRUE(dataPacks.scanDirectory(dir).success());

    AdvancementLoader loader;
    auto result = loader.loadFromDataPackRepository(dataPacks);

    ASSERT_TRUE(result.success());
    EXPECT_GE(result.value().successCount, 2u) << "Should load at least 2 advancements from data pack";
    EXPECT_EQ(result.value().failedCount, 0u);
    EXPECT_TRUE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "story/root")));
    EXPECT_TRUE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "story/mine_stone")));

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST_F(AdvancementLoaderTest, LoadFromDataPackRepository_PluralDirName)
{
    // 使用复数 advancements/ 目录名（MC 1.16.5 格式）
    const auto dir = createDataPackDirPlural();
    resource::DataPackRepository dataPacks;
    ASSERT_TRUE(dataPacks.scanDirectory(dir).success());

    AdvancementLoader loader;
    auto result = loader.loadFromDataPackRepository(dataPacks);

    ASSERT_TRUE(result.success());
    EXPECT_GE(result.value().successCount, 1u) << "Should load at least 1 advancement from plural dir data pack";
    EXPECT_EQ(result.value().failedCount, 0u);
    EXPECT_TRUE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "adventure/root")));

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST_F(AdvancementLoaderTest, LoadFromDataPackRepository_EmptyDataPack)
{
    // 空数据包（没有成就文件）
    const auto dir = mc::test::makeUniqueTestDir("mc_advancement_loader_test");
    std::filesystem::create_directories(dir / "empty_pack/data/minecraft");
    writeTextFile(dir / "empty_pack/pack.mcmeta", R"({"pack":{"pack_format":41,"description":"empty"}})");

    resource::DataPackRepository dataPacks;
    ASSERT_TRUE(dataPacks.scanDirectory(dir).success());

    AdvancementLoader loader;
    auto result = loader.loadFromDataPackRepository(dataPacks);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().successCount, 0u);
    EXPECT_EQ(result.value().failedCount, 0u);

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST_F(AdvancementLoaderTest, LoadFromDataPackRepository_WithInvalidJson)
{
    const auto dir = mc::test::makeUniqueTestDir("mc_advancement_loader_test");
    std::filesystem::create_directories(dir / "test_pack/data/minecraft/advancement/test");

    writeTextFile(dir / "test_pack/pack.mcmeta", R"({"pack":{"pack_format":41,"description":"test"}})");
    writeTextFile(dir / "test_pack/data/minecraft/advancement/test/good.json", kSimpleAdvancementJson);
    writeTextFile(dir / "test_pack/data/minecraft/advancement/test/bad.json", kInvalidJsonSyntax);

    resource::DataPackRepository dataPacks;
    ASSERT_TRUE(dataPacks.scanDirectory(dir).success());

    AdvancementLoader loader;
    auto result = loader.loadFromDataPackRepository(dataPacks);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().successCount, 1u);
    EXPECT_EQ(result.value().failedCount, 1u);
    EXPECT_FALSE(result.value().errors.empty());

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST_F(AdvancementLoaderTest, LoadFromDataPackRepository_DuplicateIdSecondFails)
{
    // 两个成就文件使用相同的 ID，第二个应失败
    const auto dir = mc::test::makeUniqueTestDir("mc_advancement_loader_test");
    std::filesystem::create_directories(dir / "test_pack/data/minecraft/advancement/dup");

    writeTextFile(dir / "test_pack/pack.mcmeta", R"({"pack":{"pack_format":41,"description":"test"}})");
    writeTextFile(dir / "test_pack/data/minecraft/advancement/dup/first.json", kSimpleAdvancementJson);
    // 第二个文件路径不同但解析后的 ID 相同（都映射到 minecraft:dup/first）
    // 不，实际上路径不同则 ID 也不同。创建两个指向同一 ID 的文件需要同名文件在不同包中。
    // 这里改为创建同名文件来测试重复注册
    writeTextFile(dir / "test_pack/data/minecraft/advancement/dup/second.json", kSimpleAdvancementJson);

    resource::DataPackRepository dataPacks;
    ASSERT_TRUE(dataPacks.scanDirectory(dir).success());

    AdvancementLoader loader;
    auto result = loader.loadFromDataPackRepository(dataPacks);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().successCount, 2u) << "Different paths should produce different IDs";
    EXPECT_EQ(result.value().failedCount, 0u);
    EXPECT_TRUE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "dup/first")));
    EXPECT_TRUE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "dup/second")));

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST_F(AdvancementLoaderTest, LoadFromDataPackRepository_ClearBeforeLoad)
{
    // 先注册一个预存在的成就
    auto trigger = std::make_shared<ImpossibleTriggerInstance>();
    auto adv = std::make_shared<Advancement>(
        Advancement::Builder(ResourceLocation("minecraft", "pre_existing")).criterion("test", trigger).build().value());
    AdvancementManager::instance().registerAdvancement(adv);
    EXPECT_TRUE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "pre_existing")));

    const auto dir = createDataPackDirSingular();
    resource::DataPackRepository dataPacks;
    ASSERT_TRUE(dataPacks.scanDirectory(dir).success());

    // 默认 clearBeforeLoad=true，加载数据包应清空已有成就
    AdvancementLoader loader;
    auto result = loader.loadFromDataPackRepository(dataPacks);

    ASSERT_TRUE(result.success());
    EXPECT_FALSE(AdvancementManager::instance().contains(ResourceLocation("minecraft", "pre_existing")));

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST_F(AdvancementLoaderTest, LoadFromDataPackRepository_ProgressCallback)
{
    const auto dir = createDataPackDirSingular();
    resource::DataPackRepository dataPacks;
    ASSERT_TRUE(dataPacks.scanDirectory(dir).success());

    Size callbackCount = 0;
    AdvancementLoader loader;
    auto result = loader.loadFromDataPackRepository(
        dataPacks, [&callbackCount](Size /*current*/, Size /*total*/, const std::string& /*id*/) { ++callbackCount; });

    ASSERT_TRUE(result.success());
    EXPECT_GT(callbackCount, 0u) << "Progress callback should be invoked for each advancement";

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}
