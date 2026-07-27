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

#include "item/loot/LootPredicateLoader.hpp"
#include "common/TempDirHelper.hpp"
#include "item/Items.hpp"
#include "item/loot/LootPredicateManager.hpp"
#include "item/loot/conditions/LootConditions.hpp"
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace mc;
using namespace mc::loot;

namespace {

void writeTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << text;
}

constexpr const char* kRandomChancePredicateJson = R"({
  "condition": "minecraft:random_chance",
  "chance": 0.5
})";

constexpr const char* kSurvivesExplosionPredicateJson = R"({
  "condition": "minecraft:survives_explosion"
})";

constexpr const char* kInvalidConditionJson = R"({
  "condition": "minecraft:nonexistent_condition_type",
  "chance": 0.5
})";

constexpr const char* kInvalidJsonSyntax = R"(not valid json at all)";

} // namespace

// ============================================================================
// LootPredicateLoader pathToPredicateId 测试
// ============================================================================

class LootPredicateLoaderTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(LootPredicateLoaderTest, PathToPredicateId_StandardPath)
{
    LootPredicateManager manager;
    LootPredicateLoader loader(manager);

    // 旧格式（包含 data/ 前缀）
    EXPECT_EQ("minecraft:gameplay/raid", loader.pathToPredicateId("data/minecraft/predicates/gameplay/raid.json"));
    EXPECT_EQ("mod_id:custom/predicate", loader.pathToPredicateId("data/mod_id/predicates/custom/predicate.json"));

    // 新格式（相对于 data/ 根目录，不含 data/ 前缀）
    EXPECT_EQ("minecraft:gameplay/raid", loader.pathToPredicateId("minecraft/predicates/gameplay/raid.json"));
    EXPECT_EQ("mod_id:custom/predicate", loader.pathToPredicateId("mod_id/predicates/custom/predicate.json"));
}

TEST_F(LootPredicateLoaderTest, PathToPredicateId_NestedPath)
{
    LootPredicateManager manager;
    LootPredicateLoader loader(manager);

    // 多层嵌套路径
    EXPECT_EQ(
        "minecraft:gameplay/raid/hero", loader.pathToPredicateId("data/minecraft/predicates/gameplay/raid/hero.json"));
}

TEST_F(LootPredicateLoaderTest, PathToPredicateId_NoPredicatesDir)
{
    LootPredicateManager manager;
    LootPredicateLoader loader(manager);

    // 没有 predicates/ 目录时，应使用文件名（去掉 .json）
    std::string result = loader.pathToPredicateId("some/random/file.json");
    EXPECT_EQ("minecraft:file", result);
}

// ============================================================================
// LootPredicateLoader loadJson 测试
// ============================================================================

TEST_F(LootPredicateLoaderTest, LoadJson_ValidRandomChance)
{
    LootPredicateManager manager;
    LootPredicateLoader loader(manager);

    auto result = loader.loadJson("minecraft:test/random_chance", kRandomChancePredicateJson);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), "minecraft:test/random_chance");

    // 验证管理器中有该谓词
    const LootCondition* predicate = manager.getPredicate("minecraft:test/random_chance");
    ASSERT_NE(predicate, nullptr);
    EXPECT_EQ(predicate->getType(), "random_chance");
}

TEST_F(LootPredicateLoaderTest, LoadJson_ValidSurvivesExplosion)
{
    LootPredicateManager manager;
    LootPredicateLoader loader(manager);

    auto result = loader.loadJson("minecraft:test/survives_explosion", kSurvivesExplosionPredicateJson);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), "minecraft:test/survives_explosion");

    const LootCondition* predicate = manager.getPredicate("minecraft:test/survives_explosion");
    ASSERT_NE(predicate, nullptr);
    EXPECT_EQ(predicate->getType(), "survives_explosion");
}

TEST_F(LootPredicateLoaderTest, LoadJson_InvalidConditionType)
{
    LootPredicateManager manager;
    LootPredicateLoader loader(manager);

    auto result = loader.loadJson("minecraft:test/invalid_condition", kInvalidConditionJson);
    EXPECT_FALSE(result.success());

    // 谓词不应注册
    EXPECT_EQ(manager.getPredicate("minecraft:test/invalid_condition"), nullptr);
}

TEST_F(LootPredicateLoaderTest, LoadJson_InvalidJsonSyntax)
{
    LootPredicateManager manager;
    LootPredicateLoader loader(manager);

    auto result = loader.loadJson("minecraft:test/bad_json", kInvalidJsonSyntax);
    EXPECT_FALSE(result.success());

    // 谓词不应注册
    EXPECT_EQ(manager.getPredicate("minecraft:test/bad_json"), nullptr);
}

TEST_F(LootPredicateLoaderTest, LoadJson_MultiplePredicates)
{
    LootPredicateManager manager;
    LootPredicateLoader loader(manager);

    auto r1 = loader.loadJson("minecraft:test/predicate_a", kRandomChancePredicateJson);
    auto r2 = loader.loadJson("minecraft:test/predicate_b", kSurvivesExplosionPredicateJson);

    ASSERT_TRUE(r1.success());
    ASSERT_TRUE(r2.success());

    EXPECT_NE(manager.getPredicate("minecraft:test/predicate_a"), nullptr);
    EXPECT_NE(manager.getPredicate("minecraft:test/predicate_b"), nullptr);

    // 验证类型不同
    EXPECT_EQ(manager.getPredicate("minecraft:test/predicate_a")->getType(), "random_chance");
    EXPECT_EQ(manager.getPredicate("minecraft:test/predicate_b")->getType(), "survives_explosion");
}

// ============================================================================
// LootPredicateLoader loadFromDirectory 测试
// ============================================================================

TEST_F(LootPredicateLoaderTest, LoadFromDirectory_LoadsJsonFiles)
{
    // 使用 data/<namespace>/predicates/ 目录结构，确保 pathToPredicateId 能正确解析
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_predicate_loader_test");
    writeTextFile(tempRoot / "data/minecraft/predicates/gameplay/test_predicate.json", kRandomChancePredicateJson);

    LootPredicateManager manager;
    LootPredicateLoader loader(manager);
    auto result = loader.loadFromDirectory((tempRoot / "data/minecraft/predicates").string());

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().successCount, 1u);
    EXPECT_EQ(result.value().failedCount, 0u);
    EXPECT_TRUE(manager.hasPredicate("minecraft:gameplay/test_predicate"));

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

TEST_F(LootPredicateLoaderTest, LoadFromDirectory_NonExistentDirectory)
{
    LootPredicateManager manager;
    LootPredicateLoader loader(manager);
    auto result = loader.loadFromDirectory("/nonexistent/directory/path");

    EXPECT_FALSE(result.success());
}

TEST_F(LootPredicateLoaderTest, LoadFromDirectory_EmptyDirectory)
{
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_predicate_loader_test");

    LootPredicateManager manager;
    LootPredicateLoader loader(manager);
    auto result = loader.loadFromDirectory(tempRoot.string());

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().successCount, 0u);
    EXPECT_EQ(result.value().failedCount, 0u);

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

TEST_F(LootPredicateLoaderTest, LoadFromDirectory_MultipleFilesWithFailure)
{
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_predicate_loader_test");
    writeTextFile(tempRoot / "data/minecraft/predicates/good/valid.json", kRandomChancePredicateJson);
    writeTextFile(tempRoot / "data/minecraft/predicates/bad/invalid.json", kInvalidConditionJson);

    LootPredicateManager manager;
    LootPredicateLoader loader(manager);
    auto result = loader.loadFromDirectory((tempRoot / "data/minecraft/predicates").string());

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().successCount, 1u);
    EXPECT_EQ(result.value().failedCount, 1u);
    EXPECT_FALSE(result.value().errors.empty());

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

// ============================================================================
// LootPredicateLoader clearBeforeLoad 测试
// ============================================================================

TEST_F(LootPredicateLoaderTest, ClearBeforeLoad_DefaultTrue)
{
    LootPredicateManager manager;

    // 先注册一个谓词
    manager.registerPredicate("minecraft:pre_existing", std::make_unique<RandomChanceCondition>(1.0f));
    EXPECT_TRUE(manager.hasPredicate("minecraft:pre_existing"));

    // 默认 clearBeforeLoad=true，加载新谓词时应清空已有谓词
    LootPredicateLoader loader(manager);
    auto result = loader.loadJson("minecraft:new_predicate", kRandomChancePredicateJson);

    // 注意：loadJson 不会触发 _clearIfNeeded，只有 loadFromDirectory/loadFromResourcePacks 才会
    // 所以先验证 loadJson 不清空
    EXPECT_TRUE(manager.hasPredicate("minecraft:pre_existing"));
}

TEST_F(LootPredicateLoaderTest, ClearBeforeLoad_DirectoryLoad)
{
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_predicate_loader_test");
    writeTextFile(tempRoot / "data/minecraft/predicates/gameplay/test.json", kRandomChancePredicateJson);

    LootPredicateManager manager;

    // 先注册一个谓词
    manager.registerPredicate("minecraft:pre_existing", std::make_unique<RandomChanceCondition>(1.0f));
    EXPECT_TRUE(manager.hasPredicate("minecraft:pre_existing"));

    // clearBeforeLoad=true（默认），加载目录时应清空已有谓词
    LootPredicateLoader loader(manager);
    loader.setClearBeforeLoad(true);
    auto result = loader.loadFromDirectory((tempRoot / "data/minecraft/predicates").string());

    ASSERT_TRUE(result.success());
    // 预注册的谓词应被清空
    EXPECT_FALSE(manager.hasPredicate("minecraft:pre_existing"));
    // 新加载的谓词应存在
    EXPECT_TRUE(manager.hasPredicate("minecraft:gameplay/test"));

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

TEST_F(LootPredicateLoaderTest, ClearBeforeLoad_False)
{
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_predicate_loader_test");
    writeTextFile(tempRoot / "data/minecraft/predicates/gameplay/test.json", kRandomChancePredicateJson);

    LootPredicateManager manager;

    // 先注册一个谓词
    manager.registerPredicate("minecraft:pre_existing", std::make_unique<RandomChanceCondition>(1.0f));
    EXPECT_TRUE(manager.hasPredicate("minecraft:pre_existing"));

    // clearBeforeLoad=false，加载目录时不应清空已有谓词
    LootPredicateLoader loader(manager);
    loader.setClearBeforeLoad(false);
    auto result = loader.loadFromDirectory((tempRoot / "data/minecraft/predicates").string());

    ASSERT_TRUE(result.success());
    // 预注册的谓词应保留
    EXPECT_TRUE(manager.hasPredicate("minecraft:pre_existing"));
    // 新加载的谓词也应存在
    EXPECT_TRUE(manager.hasPredicate("minecraft:gameplay/test"));

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

// ============================================================================
// LootPredicateLoader LoadResult 测试
// ============================================================================

TEST_F(LootPredicateLoaderTest, LoadResult_InitialState)
{
    LootPredicateManager manager;
    LootPredicateLoader loader(manager);

    const auto& result = loader.getLastResult();
    EXPECT_EQ(result.successCount, 0u);
    EXPECT_EQ(result.failedCount, 0u);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(LootPredicateLoaderTest, ResetResult)
{
    LootPredicateManager manager;
    LootPredicateLoader loader(manager);

    // loadFromDirectory 会填充 m_lastResult，但我们需要先有文件
    // 使用 loadFromDirectory 来测试 resetResult
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_predicate_loader_test");
    writeTextFile(tempRoot / "data/minecraft/predicates/test/reset.json", kRandomChancePredicateJson);

    auto result = loader.loadFromDirectory((tempRoot / "data/minecraft/predicates").string());
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().successCount, 1u);

    // 重置
    loader.resetResult();
    const auto& resetResult = loader.getLastResult();
    EXPECT_EQ(resetResult.successCount, 0u);
    EXPECT_EQ(resetResult.failedCount, 0u);
    EXPECT_TRUE(resetResult.errors.empty());

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

// ============================================================================
// LootPredicateLoader 进度回调测试
// ============================================================================

TEST_F(LootPredicateLoaderTest, LoadFromDirectory_ProgressCallback)
{
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_predicate_loader_test");
    writeTextFile(tempRoot / "data/minecraft/predicates/gameplay/a.json", kRandomChancePredicateJson);
    writeTextFile(tempRoot / "data/minecraft/predicates/gameplay/b.json", kSurvivesExplosionPredicateJson);

    LootPredicateManager manager;
    LootPredicateLoader loader(manager);

    std::vector<std::string> callbackIds;
    Size lastCurrent = 0;
    Size lastTotal = 0;

    auto callback = [&](Size current, Size total, const std::string& id) {
        lastCurrent = current;
        lastTotal = total;
        if (!id.empty()) {
            callbackIds.push_back(id);
        }
    };

    auto result = loader.loadFromDirectory((tempRoot / "data/minecraft/predicates").string(), callback);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().successCount, 2u);

    // 回调应被调用
    EXPECT_EQ(lastTotal, 2u);
    EXPECT_EQ(callbackIds.size(), 2u);

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}
