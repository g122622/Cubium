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

#include "common/resource/LanguageManager.hpp"
#include "common/util/text/TranslationTextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc;

// ============================================================================
// LanguageManager 测试
// ============================================================================

class LanguageManagerTest : public ::testing::Test {
protected:
    void SetUp() override { manager.clear(); }

    void TearDown() override { manager.clear(); }

    LanguageManager manager;
};

// ============================================================================
// 基本功能测试
// ============================================================================

TEST_F(LanguageManagerTest, EmptyManager)
{
    EXPECT_EQ(manager.translationCount(), 0u);
    EXPECT_TRUE(manager.currentLanguage().empty());
}

TEST_F(LanguageManagerTest, LoadFromJson)
{
    std::string jsonContent = R"({
        "item.minecraft.diamond": "Diamond",
        "block.minecraft.stone": "Stone",
        "chat.type.text": "<%s> %s"
    })";

    auto result = manager.loadFromJson(jsonContent);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 3u);
    EXPECT_EQ(manager.translationCount(), 3u);
}

TEST_F(LanguageManagerTest, LoadFromJsonInvalid)
{
    std::string invalidJson = "not a json";
    auto result = manager.loadFromJson(invalidJson);
    EXPECT_FALSE(result.success());
}

TEST_F(LanguageManagerTest, LoadFromJsonNotObject)
{
    std::string jsonArray = "[1, 2, 3]";
    auto result = manager.loadFromJson(jsonArray);
    EXPECT_FALSE(result.success());
}

TEST_F(LanguageManagerTest, Clear)
{
    std::string jsonContent = R"({"key": "value"})";
    manager.loadFromJson(jsonContent);
    EXPECT_EQ(manager.translationCount(), 1u);

    manager.clear();
    EXPECT_EQ(manager.translationCount(), 0u);
    EXPECT_TRUE(manager.currentLanguage().empty());
}

// ============================================================================
// 翻译查询测试
// ============================================================================

TEST_F(LanguageManagerTest, GetTranslation)
{
    std::string jsonContent = R"({
        "item.minecraft.diamond": "Diamond",
        "block.minecraft.stone": "Stone"
    })";
    manager.loadFromJson(jsonContent);

    EXPECT_EQ(manager.get("item.minecraft.diamond"), "Diamond");
    EXPECT_EQ(manager.get("block.minecraft.stone"), "Stone");
}

TEST_F(LanguageManagerTest, GetMissingKey)
{
    std::string jsonContent = R"({"key": "value"})";
    manager.loadFromJson(jsonContent);

    // 缺失的键应该返回键本身
    EXPECT_EQ(manager.get("missing.key"), "missing.key");
}

TEST_F(LanguageManagerTest, HasKey)
{
    std::string jsonContent = R"({"key": "value"})";
    manager.loadFromJson(jsonContent);

    EXPECT_TRUE(manager.has("key"));
    EXPECT_FALSE(manager.has("missing.key"));
}

// ============================================================================
// 占位符替换测试
// ============================================================================

TEST_F(LanguageManagerTest, PlaceholderSequential)
{
    std::string jsonContent = R"({
        "chat.type.text": "<%s> %s"
    })";
    manager.loadFromJson(jsonContent);

    std::string result = manager.get("chat.type.text", {"Player", "Hello!"});
    EXPECT_EQ(result, "<Player> Hello!");
}

TEST_F(LanguageManagerTest, PlaceholderPositional)
{
    std::string jsonContent = R"({
        "translation.test.complex": "First: %1$s, Second: %2$s, First again: %1$s"
    })";
    manager.loadFromJson(jsonContent);

    std::string result = manager.get("translation.test.complex", {"A", "B"});
    EXPECT_EQ(result, "First: A, Second: B, First again: A");
}

TEST_F(LanguageManagerTest, PlaceholderEscape)
{
    std::string jsonContent = R"({
        "translation.test.escape": "100%% complete"
    })";
    manager.loadFromJson(jsonContent);

    std::string result = manager.get("translation.test.escape", {});
    EXPECT_EQ(result, "100% complete");
}

TEST_F(LanguageManagerTest, PlaceholderMixed)
{
    std::string jsonContent = R"({
        "test.mixed": "Name: %s, Value: %2$s, Name again: %1$s"
    })";
    manager.loadFromJson(jsonContent);

    std::string result = manager.get("test.mixed", {"Alice", "100"});
    EXPECT_EQ(result, "Name: Alice, Value: 100, Name again: Alice");
}

TEST_F(LanguageManagerTest, PlaceholderMoreParams)
{
    std::string jsonContent = R"({
        "test": "Params: %s, %s"
    })";
    manager.loadFromJson(jsonContent);

    // 提供更多参数
    std::string result = manager.get("test", {"A", "B", "C"});
    EXPECT_EQ(result, "Params: A, B");
}

TEST_F(LanguageManagerTest, PlaceholderFewerParams)
{
    std::string jsonContent = R"({
        "test": "Params: %s, %s, %s"
    })";
    manager.loadFromJson(jsonContent);

    // 提供较少参数
    std::string result = manager.get("test", {"A"});
    // 应该保留未替换的占位符
    EXPECT_TRUE(result.find("A") != std::string::npos);
}

// ============================================================================
// 内置语言列表测试
// ============================================================================

TEST_F(LanguageManagerTest, GetBuiltinLanguages)
{
    auto languages = LanguageManager::getBuiltinLanguages();

    EXPECT_FALSE(languages.empty());

    // 检查是否包含常用语言
    bool hasEnUs = false;
    bool hasZhCn = false;
    for (const auto& lang : languages) {
        if (lang.code == "en_us") hasEnUs = true;
        if (lang.code == "zh_cn") hasZhCn = true;
    }
    EXPECT_TRUE(hasEnUs);
    EXPECT_TRUE(hasZhCn);
}

// ============================================================================
// 全局实例测试
// ============================================================================

TEST_F(LanguageManagerTest, GlobalInstance)
{
    LanguageManager& instance = LanguageManager::instance();
    EXPECT_NE(&instance, nullptr);

    // 全局实例应该是同一个
    LanguageManager& instance2 = LanguageManager::instance();
    EXPECT_EQ(&instance, &instance2);
}

// ============================================================================
// TranslationTextComponent 集成测试
// ============================================================================

class TranslationTextComponentIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置语言管理器
        mc::text::TranslationTextComponent::setLanguageManager(&manager);

        // 加载测试数据
        std::string jsonContent = R"({
            "item.minecraft.diamond": "Diamond",
            "block.minecraft.stone": "Stone",
            "chat.type.text": "<%s> %s",
            "chat.type.announcement": "[%s] %s",
            "translation.test.none": "Hello, world!",
            "translation.test.args": "%s %s"
        })";
        manager.loadFromJson(jsonContent);
    }

    void TearDown() override
    {
        mc::text::TranslationTextComponent::setLanguageManager(nullptr);
        manager.clear();
    }

    LanguageManager manager;
};

TEST_F(TranslationTextComponentIntegrationTest, BasicTranslation)
{
    mc::text::TranslationTextComponent text("item.minecraft.diamond");
    EXPECT_EQ(text.getUnformattedText(), "Diamond");
}

TEST_F(TranslationTextComponentIntegrationTest, TranslationWithParams)
{
    mc::text::TranslationTextComponent text("chat.type.text");
    text.addParam(std::make_unique<mc::text::StringTextComponent>("Player"));
    text.addParam(std::make_unique<mc::text::StringTextComponent>("Hello!"));

    EXPECT_EQ(text.getUnformattedText(), "<Player> Hello!");
}

TEST_F(TranslationTextComponentIntegrationTest, MissingKey)
{
    mc::text::TranslationTextComponent text("missing.key");
    // 缺失的键应该返回键本身
    EXPECT_EQ(text.getUnformattedText(), "missing.key");
}

TEST_F(TranslationTextComponentIntegrationTest, TranslationWithSiblings)
{
    mc::text::TranslationTextComponent text("item.minecraft.diamond");
    text.append(std::make_unique<mc::text::StringTextComponent>(" (rare)"));

    EXPECT_EQ(text.getUnformattedText(), "Diamond (rare)");
}

TEST_F(TranslationTextComponentIntegrationTest, JsonRoundTrip)
{
    mc::text::TranslationTextComponent original("chat.type.announcement");
    original.addParam(std::make_unique<mc::text::StringTextComponent>("Server"));
    original.addParam(std::make_unique<mc::text::StringTextComponent>("Welcome!"));

    nlohmann::json json = original.toJson();
    EXPECT_EQ(json["translate"], "chat.type.announcement");
    EXPECT_TRUE(json.contains("with"));
    EXPECT_EQ(json["with"].size(), 2u);

    auto parsed = mc::text::ITextComponent::fromJson(json);
    auto* transComp = dynamic_cast<mc::text::TranslationTextComponent*>(parsed.get());
    ASSERT_NE(transComp, nullptr);
    EXPECT_EQ(transComp->getKey(), "chat.type.announcement");
    EXPECT_EQ(transComp->getParams().size(), 2u);
}

// ============================================================================
// 从 JSON 内容加载测试（不依赖资源包）
// ============================================================================

TEST_F(LanguageManagerTest, LoadFromJsonContent)
{
    // 模拟从资源包加载的 JSON 内容
    std::string enUsJson = R"({
        "item.minecraft.test": "Test Item",
        "block.minecraft.test": "Test Block",
        "entity.minecraft.test": "Test Entity"
    })";

    auto result = manager.loadFromJson(enUsJson);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 3u);

    // 验证翻译
    EXPECT_EQ(manager.get("item.minecraft.test"), "Test Item");
    EXPECT_EQ(manager.get("block.minecraft.test"), "Test Block");
    EXPECT_EQ(manager.get("entity.minecraft.test"), "Test Entity");
}

TEST_F(LanguageManagerTest, LoadMultipleTimesOverwrites)
{
    // 第一次加载
    std::string json1 = R"({"key1": "value1", "key2": "old_value2"})";
    auto result1 = manager.loadFromJson(json1);
    EXPECT_TRUE(result1.success());

    // 第二次加载（应该覆盖已有的键）
    std::string json2 = R"({"key2": "new_value2", "key3": "value3"})";
    auto result2 = manager.loadFromJson(json2);
    EXPECT_TRUE(result2.success());

    // key2 应该被覆盖，key1 应该保留
    EXPECT_EQ(manager.get("key1"), "value1");
    EXPECT_EQ(manager.get("key2"), "new_value2");
    EXPECT_EQ(manager.get("key3"), "value3");
}

// ============================================================================
// 回调测试
// ============================================================================

TEST_F(LanguageManagerTest, OnLanguageChangedCallback)
{
    bool callbackCalled = false;
    manager.setOnLanguageChanged([&callbackCalled]() { callbackCalled = true; });

    // 通过 loadFromJson 加载一些数据后，手动设置语言并调用回调
    std::string jsonContent = R"({"key": "value"})";
    manager.loadFromJson(jsonContent);

    // 注意：loadFromJson 不会触发回调，只有 loadLanguage 才会
    // 这个测试验证回调可以设置
    EXPECT_FALSE(callbackCalled);
}
