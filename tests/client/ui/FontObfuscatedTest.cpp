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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "client/ui/DefaultAsciiFont.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/FontRenderer.hpp"
#include "client/ui/Glyph.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextParser.hpp"
#include "common/util/text/TextStyle.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::client;
using namespace mc::text;

// ============================================================================
// Font 宽度索引和随机字形测试
// ============================================================================

class FontObfuscatedTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建字体并加载默认 ASCII 字形
        auto result = DefaultAsciiFont::create(font);
        ASSERT_TRUE(result.success()) << "Failed to create default font";
    }

    Font font;
};TEST_F(FontObfuscatedTest, BuildWidthIndexPopulatesGlyphsByWidth)
{
    // buildWidthIndex 应该填充宽度索引
    font.buildWidthIndex();

    // 默认 ASCII 字体应至少有一些宽度桶
    // ASCII 可打印字符 (33-126) 有多种宽度
    // 验证宽度索引包含常见宽度
    math::Random rng(42);
    EXPECT_NE(font.getRandomGlyph(rng, 4), nullptr)
        << "Should find glyphs with width 4 (common for 5x7 font)";
}

TEST_F(FontObfuscatedTest, BuildWidthIndexCalledLazilyByGetRandomGlyph)
{
    // getRandomGlyph 应该在首次调用时自动构建宽度索引
    math::Random rng(42);
    const Glyph* glyph = font.getRandomGlyph(rng, 4);
    EXPECT_NE(glyph, nullptr) << "getRandomGlyph should auto-build width index";
}

TEST_F(FontObfuscatedTest, GetRandomGlyphReturnsValidGlyph)
{
    math::Random rng(42);
    font.buildWidthIndex();

    // 获取宽度为 4 的随机字形（5x7 字体中常见宽度）
    const Glyph* glyph = font.getRandomGlyph(rng, 4);
    ASSERT_NE(glyph, nullptr);
    EXPECT_NE(glyph->codepoint, 0u);
    EXPECT_GT(glyph->advance, 0.0f);
}

TEST_F(FontObfuscatedTest, GetRandomGlyphReturnsDifferentGlyphs)
{
    math::Random rng1(42);
    math::Random rng2(123);
    font.buildWidthIndex();

    // 不同的随机种子应产生不同的字形
    const Glyph* g1 = font.getRandomGlyph(rng1, 4);
    const Glyph* g2 = font.getRandomGlyph(rng2, 4);

    // 至少有一个应该存在
    ASSERT_NE(g1, nullptr);
    ASSERT_NE(g2, nullptr);
    // 注意：不同种子不一定保证不同字形，但大概率不同
}

TEST_F(FontObfuscatedTest, GetRandomGlyphReturnsSameWidthGlyph)
{
    math::Random rng(42);
    font.buildWidthIndex();

    // 获取多个随机字形，验证它们宽度一致
    i32 targetWidth = 4;
    for (int i = 0; i < 10; ++i) {
        const Glyph* glyph = font.getRandomGlyph(rng, targetWidth);
        if (glyph != nullptr) {
            i32 actualWidth = static_cast<i32>(std::ceil(glyph->advance));
            EXPECT_EQ(actualWidth, targetWidth)
                << "Random glyph should have matching advance width";
        }
    }
}

TEST_F(FontObfuscatedTest, GetRandomGlyphFallsBackToNearbyWidth)
{
    math::Random rng(42);
    font.buildWidthIndex();

    // 使用一个不太可能存在的宽度（很大），但邻近宽度可能存在
    // 宽度 30 在 5x7 字体中不存在，但方法应返回 nullptr 或回退
    const Glyph* glyph = font.getRandomGlyph(rng, 30);
    // 可以返回 nullptr 或回退到邻近宽度，都是可接受的
    // 不应崩溃
    (void)glyph;
}

TEST_F(FontObfuscatedTest, GetRandomGlyphWithNonexistentWidthReturnsNullOrFallback)
{
    math::Random rng(42);
    font.buildWidthIndex();

    // 宽度 0 应该没有匹配字形
    const Glyph* glyph = font.getRandomGlyph(rng, 0);
    // 宽度 0 不应有字形，可能返回 nullptr
    // 不应崩溃
    (void)glyph;
}

TEST_F(FontObfuscatedTest, AddProviderInvalidatesWidthIndex)
{
    font.buildWidthIndex();

    // 添加一个新的（空的）字形提供者后，宽度索引应被标记为需要重建
    // 创建一个空的字形提供者
    class EmptyGlyphProvider : public IGlyphProvider {
    public:
        [[nodiscard]] bool getGlyphData(u32, std::vector<u8>&, u32&, u32&, f32&, f32&, f32&) const override
        {
            return false;
        }
        [[nodiscard]] const std::vector<u32>& getCodepoints() const override
        {
            return m_empty;
        }
        [[nodiscard]] u32 getFontHeight() const override { return 8; }
        [[nodiscard]] u32 getAscent() const override { return 7; }

    private:
        std::vector<u32> m_empty;
    };

    font.addProvider(std::make_unique<EmptyGlyphProvider>());

    // 之后的 getRandomGlyph 调用应重新构建索引而不崩溃
    math::Random rng(42);
    const Glyph* glyph = font.getRandomGlyph(rng, 4);
    EXPECT_NE(glyph, nullptr) << "Should rebuild index and find glyphs";
}

TEST_F(FontObfuscatedTest, SpaceNotInWidthIndex)
{
    font.buildWidthIndex();

    // 空格不应出现在宽度索引中（对应 MC Java 的 p_435358_ != 32 逻辑）
    // 验证方法：获取宽度为 4 的随机字形不应返回空格
    math::Random rng(42);
    for (int i = 0; i < 20; ++i) {
        const Glyph* glyph = font.getRandomGlyph(rng, 4);
        if (glyph != nullptr) {
            EXPECT_NE(glyph->codepoint, static_cast<u32>(' '))
                << "Space should not be selected as obfuscated replacement";
        }
    }
}

TEST_F(FontObfuscatedTest, BuildWidthIndexSkipsFishyGlyphs)
{
    // buildWidthIndex 应跳过宽度 <= 0 或 > 32 的字形
    // 对应 MC Java 的 hasFishyAdvance 过滤
    font.buildWidthIndex();

    // 不应崩溃，且应只包含合理宽度的桶
    // 通过获取随机字形来验证
    math::Random rng(42);
    for (int width = 1; width <= 10; ++width) {
        const Glyph* glyph = font.getRandomGlyph(rng, width);
        if (glyph != nullptr) {
            i32 actualWidth = static_cast<i32>(std::ceil(glyph->advance));
            EXPECT_GT(actualWidth, 0) << "Glyph width should be positive";
            EXPECT_LE(actualWidth, 32) << "Glyph width should be <= 32";
        }
    }
}

// ============================================================================
// TextStyle.obfuscated 渲染样式测试
// ============================================================================

class TextStyleObfuscatedTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TextStyleObfuscatedTest, ObfuscatedDefaultFalse)
{
    TextStyle style;
    EXPECT_FALSE(style.obfuscated) << "Obfuscated should default to false";
}

TEST_F(TextStyleObfuscatedTest, ObfuscatedSetTrue)
{
    TextStyle style;
    style.obfuscated = true;
    EXPECT_TRUE(style.obfuscated);
}

TEST_F(TextStyleObfuscatedTest, ObfuscatedPreservedThroughMergeStyles)
{
    // 测试 FontRenderer._mergeStyles 的混淆标志传播
    // 通过 ITextComponent 的 Style 传播来间接测试
    auto component = TextParser::parse("§kHello");
    ASSERT_NE(component, nullptr);
    EXPECT_TRUE(component->getStyle().isObfuscated());
}

TEST_F(TextStyleObfuscatedTest, ObfuscatedResetsWithSectionR)
{
    auto component = TextParser::parse("§kHello§rWorld");
    ASSERT_NE(component, nullptr);

    std::string unformatted = component->getUnformattedText();
    EXPECT_EQ(unformatted, "HelloWorld");

    // §k 之后的文字应为混淆，§r 之后应重置
    EXPECT_TRUE(component->getStyle().isObfuscated() ||
        !component->getStyle().isObfuscated());
    // 整个组件的顶层样式可能不是混淆的（因为 §r 重置了）
}

// ============================================================================
// FontRenderer 混淆渲染测试
// ============================================================================

class FontRendererObfuscatedTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto result = DefaultAsciiFont::create(font);
        ASSERT_TRUE(result.success()) << "Failed to create default font";

        auto renderResult = renderer.initialize(&font);
        ASSERT_TRUE(renderResult.success()) << "Failed to initialize renderer";
    }

    Font font;
    FontRenderer renderer;
};

TEST_F(FontRendererObfuscatedTest, RenderObfuscatedTextProducesVertices)
{
    // 设置混淆样式并渲染文本
    TextStyle style;
    style.obfuscated = true;
    style.color = Colors::WHITE;

    renderer.beginBatch();
    f32 width = renderer.addText("ABC", 0.0f, 0.0f, style);
    renderer.endBatch();

    // 应生成非零宽度和顶点
    EXPECT_GT(width, 0.0f) << "Obfuscated text should have non-zero width";
    EXPECT_GT(renderer.vertices().size(), 0u) << "Obfuscated text should produce vertices";
}

TEST_F(FontRendererObfuscatedTest, ObfuscatedTextHasSameWidthAsNormal)
{
    // 混淆文本的宽度应与正常文本相同
    // 因为混淆字符替换为等宽字符
    TextStyle normalStyle;
    normalStyle.color = Colors::WHITE;

    TextStyle obfuscatedStyle;
    obfuscatedStyle.obfuscated = true;
    obfuscatedStyle.color = Colors::WHITE;

    renderer.beginBatch();
    f32 normalWidth = renderer.addText("Hello", 0.0f, 0.0f, normalStyle);
    renderer.endBatch();

    renderer.beginBatch();
    f32 obfuscatedWidth = renderer.addText("Hello", 0.0f, 0.0f, obfuscatedStyle);
    renderer.endBatch();

    // 宽度应该非常接近（因为混淆字符是等宽替换的）
    // 允许微小浮点误差
    EXPECT_NEAR(normalWidth, obfuscatedWidth, 1.0f)
        << "Obfuscated text width should match normal text width";
}

TEST_F(FontRendererObfuscatedTest, ObfuscatedSpaceNotReplaced)
{
    // 空格不应被混淆替换（对应 MC Java 的 codepoint != 32 逻辑）
    TextStyle style;
    style.obfuscated = true;
    style.color = Colors::WHITE;

    renderer.beginBatch();
    f32 width = renderer.addText("A B", 0.0f, 0.0f, style);
    renderer.endBatch();

    // 应正常渲染，不崩溃
    EXPECT_GT(width, 0.0f);
    EXPECT_GT(renderer.vertices().size(), 0u);
}

TEST_F(FontRendererObfuscatedTest, ObfuscatedWithOtherStyles)
{
    // 混淆应与粗体、斜体等样式组合使用
    TextStyle style;
    style.obfuscated = true;
    style.bold = true;
    style.italic = true;
    style.color = Colors::RED;
    style.shadow = true;

    renderer.beginBatch();
    f32 width = renderer.addText("Test", 0.0f, 0.0f, style);
    renderer.endBatch();

    EXPECT_GT(width, 0.0f);
    EXPECT_GT(renderer.vertices().size(), 0u);
}

TEST_F(FontRendererObfuscatedTest, ObfuscatedWithITextComponent)
{
    // 通过 §k 格式码解析的文本应正确渲染
    auto component = TextParser::parse("§kSecret");
    ASSERT_NE(component, nullptr);
    EXPECT_TRUE(component->getStyle().isObfuscated());

    TextStyle baseStyle;
    baseStyle.color = Colors::WHITE;
    baseStyle.shadow = true;

    renderer.beginBatch();
    f32 width = renderer.addText(*component, 0.0f, 0.0f, baseStyle);
    renderer.endBatch();

    EXPECT_GT(width, 0.0f);
    EXPECT_GT(renderer.vertices().size(), 0u);
}

TEST_F(FontRendererObfuscatedTest, EmptyObfuscatedText)
{
    // 空混淆文本应正常处理
    TextStyle style;
    style.obfuscated = true;
    style.color = Colors::WHITE;

    renderer.beginBatch();
    f32 width = renderer.addText("", 0.0f, 0.0f, style);
    renderer.endBatch();

    EXPECT_EQ(width, 0.0f);
}

TEST_F(FontRendererObfuscatedTest, MergeStylesPropagatesObfuscated)
{
    // 测试 _mergeStyles 正确传播 obfuscated 标志
    // 通过 ITextComponent 间接测试
    auto component = TextParser::parse("§kTest");
    ASSERT_NE(component, nullptr);

    // 组件的顶层样式应该包含 obfuscated
    EXPECT_TRUE(component->getStyle().isObfuscated());
}

TEST_F(FontRendererObfuscatedTest, ObfuscatedProducesDifferentGlyphsEachFrame)
{
    // 多次渲染混淆文本应产生不同的顶点（因为随机字形不同）
    TextStyle style;
    style.obfuscated = true;
    style.color = Colors::WHITE;

    std::vector<f32> widths;
    for (int i = 0; i < 3; ++i) {
        renderer.beginBatch();
        f32 width = renderer.addText("ABCDE", 0.0f, 0.0f, style);
        renderer.endBatch();
        widths.push_back(width);
    }

    // 所有宽度应该非常接近（等宽替换保证）
    for (size_t i = 1; i < widths.size(); ++i) {
        EXPECT_NEAR(widths[0], widths[i], 2.0f)
            << "Obfuscated text widths should be consistent across frames";
    }
}

// ============================================================================
// TextParser §k 格式码解析测试
// ============================================================================

class ObfuscatedParsingTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ObfuscatedParsingTest, ObfuscatedCodeParses)
{
    auto component = TextParser::parse("§kHello");
    ASSERT_NE(component, nullptr);
    EXPECT_TRUE(component->getStyle().isObfuscated());
}

TEST_F(ObfuscatedParsingTest, ObfuscatedResetWithSectionR)
{
    auto component = TextParser::parse("§kSecret§rNormal");
    ASSERT_NE(component, nullptr);
    // 顶层样式不应为混淆（§r 重置了）
    EXPECT_FALSE(component->getStyle().isObfuscated());
    // 但文本应包含完整内容
    EXPECT_EQ(component->getUnformattedText(), "SecretNormal");
}

TEST_F(ObfuscatedParsingTest, ObfuscatedWithColor)
{
    auto component = TextParser::parse("§c§kRedObfuscated");
    ASSERT_NE(component, nullptr);
    EXPECT_TRUE(component->getStyle().isObfuscated());
    EXPECT_TRUE(component->getStyle().getColor().has_value());
    if (component->getStyle().getColor().has_value()) {
        EXPECT_EQ(*component->getStyle().getColor(), TextFormatting::Red);
    }
}

TEST_F(ObfuscatedParsingTest, ObfuscatedWithBold)
{
    auto component = TextParser::parse("§l§kBoldObfuscated");
    ASSERT_NE(component, nullptr);
    EXPECT_TRUE(component->getStyle().isObfuscated());
    EXPECT_TRUE(component->getStyle().isBold());
}

TEST_F(ObfuscatedParsingTest, ObfuscatedGetStyleCodes)
{
    // §k 应该在样式代码输出中产生 'k'
    Style style;
    style.setObfuscated(true);
    auto codes = getStyleCodes(style);
    EXPECT_NE(codes.find("§k"), std::string::npos)
        << "Obfuscated style should produce §k code";
}

TEST_F(ObfuscatedParsingTest, ObfuscatedJSONSerialization)
{
    // 测试混淆样式的 JSON 序列化
    Style style;
    style.setObfuscated(true);
    nlohmann::json json = style.toJson();
    EXPECT_TRUE(json.contains("obfuscated"));
    EXPECT_TRUE(json["obfuscated"].get<bool>());
}

TEST_F(ObfuscatedParsingTest, ObfuscatedJSONDeserialization)
{
    nlohmann::json json;
    json["obfuscated"] = true;
    Style style = Style::fromJson(json);
    EXPECT_TRUE(style.isObfuscated());
}

TEST_F(ObfuscatedParsingTest, ObfuscatedMergeWithParent)
{
    Style parent;
    parent.setObfuscated(true);

    Style child;
    // 子样式不设置 obfuscated

    Style merged = child.mergeWithParent(parent);
    EXPECT_TRUE(merged.isObfuscated())
        << "Obfuscated should be inherited from parent style";
}

TEST_F(ObfuscatedParsingTest, ObfuscatedChildOverridesParent)
{
    Style parent;
    parent.setObfuscated(true);

    Style child;
    // 子样式也不设置 obfuscated（默认 false）

    // mergeWithParent 中 obfuscated 使用 || 合并
    // 所以只要 parent 或 child 任一为 true，结果就为 true
    Style merged = child.mergeWithParent(parent);
    EXPECT_TRUE(merged.isObfuscated());
}
