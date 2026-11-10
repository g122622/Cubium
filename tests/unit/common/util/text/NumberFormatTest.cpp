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

/**
 * @file NumberFormatTest.cpp
 * @brief 数字格式（NumberFormat）的渲染语义、NBT 编解码与 Java 线格式编码测试
 *
 * 覆盖：
 * - 三变体的 `format(value)` 语义（blank 空文本 / styled 数字套样式 / fixed 固定组件副本）
 * - 类型注册名与 id 的对应（id 就是网络上的注册表 id，顺序不可变）
 * - NBT 存档编码：`{type:..., ...}` 的平铺结构、往返、损坏数据的报错
 * - Java 线格式：`VarInt(id)` + 变体载荷（styled=Style NBT 根标签，fixed=Component NBT 根标签）
 * - Style ↔ NBT 往返（具名色 / 自定义 RGB / 三态标志 / insertion）
 */

#include "common/util/text/numbers/NumberFormat.hpp"
#include "common/network/buffer/ByteBuf.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include "common/util/text/ComponentNbtSerialization.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/StyleNbt.hpp"
#include "common/util/text/TextStyle.hpp"
#include "common/util/text/TranslationTextComponent.hpp"
#include "common/util/text/numbers/NumberFormatCodec.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace mc;
namespace numbers = mc::text::numbers;

namespace {

/// 从复合标签取字符串键（不存在或类型不符返回空串）
std::string getString(const nbt::tags::compound_tag& comp, const std::string& key)
{
    const auto it = comp.value.find(key);
    if (it == comp.value.end()) {
        return {};
    }
    const auto* str = dynamic_cast<const nbt::tags::string_tag*>(it->second.get());
    return str != nullptr ? str->value : std::string{};
}

/// 从复合标签取 byte 键（不存在或类型不符返回 nullopt）
std::optional<std::int8_t> getByte(const nbt::tags::compound_tag& comp, const std::string& key)
{
    const auto it = comp.value.find(key);
    if (it == comp.value.end()) {
        return std::nullopt;
    }
    const auto* tag = dynamic_cast<const nbt::tags::byte_tag*>(it->second.get());
    if (tag == nullptr) {
        return std::nullopt;
    }
    return tag->value;
}

} // namespace

// ============================================================================
// 类型标识
// ============================================================================

TEST(NumberFormatTest, TypeIdsMatchRegistryOrder)
{
    // 枚举值就是原版 NUMBER_FORMAT_TYPE 注册表的 id，线格式直接用它们。
    EXPECT_EQ(static_cast<u8>(numbers::NumberFormatType::Blank), 0);
    EXPECT_EQ(static_cast<u8>(numbers::NumberFormatType::Styled), 1);
    EXPECT_EQ(static_cast<u8>(numbers::NumberFormatType::Fixed), 2);
    EXPECT_EQ(numbers::NUMBER_FORMAT_TYPE_COUNT, 3u);
}

TEST(NumberFormatTest, TypeNamesRoundTrip)
{
    for (const auto type :
        {numbers::NumberFormatType::Blank, numbers::NumberFormatType::Styled, numbers::NumberFormatType::Fixed}) {
        const auto name = numbers::numberFormatTypeName(type);
        const auto parsed = numbers::numberFormatTypeFromName(name);
        ASSERT_TRUE(parsed.has_value()) << "name=" << name;
        EXPECT_EQ(*parsed, type);
    }
    EXPECT_EQ(numbers::numberFormatTypeName(numbers::NumberFormatType::Blank), "minecraft:blank");
    EXPECT_EQ(numbers::numberFormatTypeName(numbers::NumberFormatType::Styled), "minecraft:styled");
    EXPECT_EQ(numbers::numberFormatTypeName(numbers::NumberFormatType::Fixed), "minecraft:fixed");
}

TEST(NumberFormatTest, TypeNameWithoutNamespaceIsAccepted)
{
    // 原版 ResourceLocation 解析允许省略 minecraft: 前缀
    EXPECT_EQ(numbers::numberFormatTypeFromName("styled"), numbers::NumberFormatType::Styled);
    EXPECT_EQ(numbers::numberFormatTypeFromName("fixed"), numbers::NumberFormatType::Fixed);
    EXPECT_FALSE(numbers::numberFormatTypeFromName("styledx").has_value());
    EXPECT_FALSE(numbers::numberFormatTypeFromName("").has_value());
}

// ============================================================================
// 渲染语义
// ============================================================================

TEST(NumberFormatTest, BlankRendersEmptyText)
{
    const numbers::NumberFormat format;
    EXPECT_EQ(format.type(), numbers::NumberFormatType::Blank);
    EXPECT_EQ(format.style(), nullptr);
    EXPECT_EQ(format.value(), nullptr);

    const auto rendered = format.format(42);
    ASSERT_NE(rendered, nullptr);
    EXPECT_EQ(rendered->getUnformattedText(), "");
}

TEST(NumberFormatTest, StyledRendersNumberWithStyle)
{
    text::Style style;
    style.setColor(text::TextFormatting::Red);
    style.setBold(true);
    const auto format = numbers::NumberFormat::styled(style);

    EXPECT_EQ(format.type(), numbers::NumberFormatType::Styled);
    ASSERT_NE(format.style(), nullptr);
    EXPECT_EQ(*format.style(), style);
    EXPECT_EQ(format.value(), nullptr);

    const auto rendered = format.format(-7);
    ASSERT_NE(rendered, nullptr);
    // 数字本身按十进制写出（原版 Integer.toString）
    EXPECT_EQ(rendered->getUnformattedText(), "-7");
    EXPECT_EQ(rendered->getStyle(), style);
    // 相邻两次渲染互不影响（每次都是新组件）
    const auto another = format.format(7);
    ASSERT_NE(another, nullptr);
    EXPECT_EQ(another->getUnformattedText(), "7");
}

TEST(NumberFormatTest, FixedRendersComponentCopy)
{
    auto value = std::make_unique<text::StringTextComponent>("MAX");
    text::Style style;
    style.setColor(text::TextFormatting::Gold);
    value->setStyle(style);
    auto child = std::make_unique<text::StringTextComponent>("!");
    value->append(std::move(child));

    const auto format = numbers::NumberFormat::fixed(std::move(value));
    EXPECT_EQ(format.type(), numbers::NumberFormatType::Fixed);
    EXPECT_EQ(format.style(), nullptr);
    ASSERT_NE(format.value(), nullptr);

    // fixed 忽略传入的数字
    const auto rendered = format.format(12345);
    ASSERT_NE(rendered, nullptr);
    EXPECT_EQ(rendered->getUnformattedText(), "MAX!");
    EXPECT_EQ(rendered->getStyle(), style);
    ASSERT_EQ(rendered->getSiblings().size(), 1u);

    // 渲染结果是副本：改它不能影响格式本身
    rendered->append(std::make_unique<text::StringTextComponent>("?"));
    EXPECT_EQ(format.value()->getUnformattedText(), "MAX!");
    EXPECT_EQ(format.value()->getSiblings().size(), 1u);
}

TEST(NumberFormatTest, FixedWithNullComponentRendersEmptyText)
{
    const auto format = numbers::NumberFormat::fixed(nullptr);
    const auto rendered = format.format(9);
    ASSERT_NE(rendered, nullptr);
    EXPECT_EQ(rendered->getUnformattedText(), "");
}

TEST(NumberFormatTest, CopyIsDeep)
{
    auto value = std::make_unique<text::StringTextComponent>("fixed");
    const auto original = numbers::NumberFormat::fixed(std::move(value));
    const numbers::NumberFormat copy = original;

    ASSERT_NE(copy.value(), nullptr);
    ASSERT_NE(original.value(), nullptr);
    // 两个对象持有不同的组件实例
    EXPECT_NE(copy.value(), original.value());
    EXPECT_EQ(*copy.value(), *original.value());
    EXPECT_EQ(copy, original);

    numbers::NumberFormat assigned;
    assigned = original;
    EXPECT_EQ(assigned, original);
    ASSERT_NE(assigned.value(), nullptr);
    EXPECT_NE(assigned.value(), original.value());
}

TEST(NumberFormatTest, EqualityComparesPayload)
{
    text::Style red;
    red.setColor(text::TextFormatting::Red);
    text::Style blue;
    blue.setColor(text::TextFormatting::Blue);

    EXPECT_EQ(numbers::NumberFormat::styled(red), numbers::NumberFormat::styled(red));
    EXPECT_NE(numbers::NumberFormat::styled(red), numbers::NumberFormat::styled(blue));
    EXPECT_NE(numbers::NumberFormat(), numbers::NumberFormat::styled(text::Style()));

    auto left = std::make_unique<text::StringTextComponent>("a");
    auto right = std::make_unique<text::StringTextComponent>("b");
    EXPECT_NE(numbers::NumberFormat::fixed(std::move(left)), numbers::NumberFormat::fixed(std::move(right)));
}

TEST(NumberFormatTest, MovedFromBecomesBlank)
{
    auto value = std::make_unique<text::StringTextComponent>("x");
    auto source = numbers::NumberFormat::fixed(std::move(value));
    numbers::NumberFormat sink = std::move(source);

    EXPECT_EQ(sink.type(), numbers::NumberFormatType::Fixed);
    // 被移走的对象不留「类型是 fixed 但载荷为空」的不一致状态
    EXPECT_EQ(source.type(), numbers::NumberFormatType::Blank);
    EXPECT_EQ(source.value(), nullptr);
}

// ============================================================================
// NBT 存档编解码
// ============================================================================

TEST(NumberFormatNbtTest, BlankEncodesTypeOnly)
{
    const auto tag = numbers::numberFormatToNbt(numbers::NumberFormat());
    EXPECT_EQ(getString(tag, "type"), "minecraft:blank");
    EXPECT_EQ(tag.value.size(), 1u);
}

TEST(NumberFormatNbtTest, StyledEncodesStyleFieldsFlat)
{
    text::Style style;
    style.setColor(text::TextFormatting::Red);
    style.setBold(true);
    style.setItalic(false);

    const auto tag = numbers::numberFormatToNbt(numbers::NumberFormat::styled(style));
    EXPECT_EQ(getString(tag, "type"), "minecraft:styled");
    // 样式字段与 type 平铺在同一层（原版 dispatchMap 语义），不嵌套
    EXPECT_EQ(getString(tag, "color"), "red");
    EXPECT_EQ(getByte(tag, "bold"), 1);
    EXPECT_EQ(getByte(tag, "italic"), 0);
    EXPECT_EQ(tag.value.count("style"), 0u);
}

TEST(NumberFormatNbtTest, FixedEncodesComponentUnderValue)
{
    auto value = std::make_unique<text::StringTextComponent>("N/A");
    const auto tag = numbers::numberFormatToNbt(numbers::NumberFormat::fixed(std::move(value)));

    EXPECT_EQ(getString(tag, "type"), "minecraft:fixed");
    const auto it = tag.value.find("value");
    ASSERT_NE(it, tag.value.end());
    // 可折叠纯文本 → StringTag
    const auto* valueTag = dynamic_cast<const nbt::tags::string_tag*>(it->second.get());
    ASSERT_NE(valueTag, nullptr);
    EXPECT_EQ(valueTag->value, "N/A");
}

TEST(NumberFormatNbtTest, RoundTripsAllVariants)
{
    text::Style style;
    style.setTextColor(text::TextColor::fromRgb(0xFF5555));
    style.setUnderlined(true);
    style.setInsertion("hint");

    const std::vector<numbers::NumberFormat> formats = {
        numbers::NumberFormat(),
        numbers::NumberFormat::styled(style),
        numbers::NumberFormat::fixed(std::make_unique<text::StringTextComponent>("fixed text")),
    };

    for (const auto& format : formats) {
        const auto tag = numbers::numberFormatToNbt(format);
        auto parsed = numbers::numberFormatFromNbt(tag);
        ASSERT_TRUE(parsed.success()) << "type=" << numbers::numberFormatTypeName(format.type());
        EXPECT_EQ(parsed.value(), format);
    }
}

TEST(NumberFormatNbtTest, RoundTripsStyleWithCustomColor)
{
    // 自定义 RGB 颜色必须走 "#RRGGBB"，不能因为「不是具名色」就丢掉
    text::Style style;
    style.setTextColor(text::TextColor::fromRgb(0x123456));
    const auto tag = numbers::numberFormatToNbt(numbers::NumberFormat::styled(style));
    EXPECT_EQ(getString(tag, "color"), "#123456");

    auto parsed = numbers::numberFormatFromNbt(tag);
    ASSERT_TRUE(parsed.success());
    ASSERT_NE(parsed.value().style(), nullptr);
    EXPECT_EQ(*parsed.value().style(), style);
}

TEST(NumberFormatNbtTest, RejectsMissingType)
{
    nbt::tags::compound_tag tag;
    const auto parsed = numbers::numberFormatFromNbt(tag);
    EXPECT_TRUE(parsed.failed());
}

TEST(NumberFormatNbtTest, RejectsUnknownType)
{
    nbt::tags::compound_tag tag;
    tag.put("type", std::string("minecraft:rainbow"));
    const auto parsed = numbers::numberFormatFromNbt(tag);
    EXPECT_TRUE(parsed.failed());
}

TEST(NumberFormatNbtTest, RejectsFixedWithoutValue)
{
    nbt::tags::compound_tag tag;
    tag.put("type", std::string("minecraft:fixed"));
    const auto parsed = numbers::numberFormatFromNbt(tag);
    EXPECT_TRUE(parsed.failed());
}

TEST(NumberFormatNbtTest, RejectsInvalidStyleColor)
{
    nbt::tags::compound_tag tag;
    tag.put("type", std::string("minecraft:styled"));
    tag.put("color", std::string("not-a-color"));
    const auto parsed = numbers::numberFormatFromNbt(tag);
    EXPECT_TRUE(parsed.failed());
}

TEST(NumberFormatNbtTest, AcceptsFullAndShortTypeNames)
{
    nbt::tags::compound_tag shortName;
    shortName.put("type", std::string("blank"));
    auto parsed = numbers::numberFormatFromNbt(shortName);
    ASSERT_TRUE(parsed.success());
    EXPECT_EQ(parsed.value().type(), numbers::NumberFormatType::Blank);
}

// ============================================================================
// Java 线格式
// ============================================================================

TEST(NumberFormatWireTest, BlankIsSingleTypeIdByte)
{
    const auto bytes = numbers::numberFormatToWireBytes(numbers::NumberFormat());
    ASSERT_EQ(bytes.size(), 1u);
    EXPECT_EQ(bytes[0], 0x00); // VarInt(0)
}

TEST(NumberFormatWireTest, AbsentOptionalEncodesToEmptyVector)
{
    // IR 里空 vector = absent（原版 Optional<NumberFormat> 的 false），与 blank（存在但无载荷）不同
    EXPECT_TRUE(numbers::numberFormatToWireBytes(std::optional<numbers::NumberFormat>()).empty());

    const auto present =
        numbers::numberFormatToWireBytes(std::optional<numbers::NumberFormat>(numbers::NumberFormat()));
    ASSERT_EQ(present.size(), 1u);
    EXPECT_EQ(present[0], 0x00);
}

TEST(NumberFormatWireTest, StyledPayloadIsStyleRootCompound)
{
    const auto bytes = numbers::numberFormatToWireBytes(numbers::NumberFormat::styled(text::Style()));
    // VarInt(1) + CompoundTag(0x0A) + 空 entries + End(0x00)
    ASSERT_EQ(bytes.size(), 3u);
    EXPECT_EQ(bytes[0], 0x01);
    EXPECT_EQ(bytes[1], 0x0A);
    EXPECT_EQ(bytes[2], 0x00);
}

TEST(NumberFormatWireTest, StyledPayloadCarriesColor)
{
    text::Style style;
    style.setColor(text::TextFormatting::Red);
    const auto bytes = numbers::numberFormatToWireBytes(numbers::NumberFormat::styled(style));

    ASSERT_GE(bytes.size(), 2u);
    EXPECT_EQ(bytes[0], 0x01);
    EXPECT_EQ(bytes[1], 0x0A);

    // 载荷本身是完整的 Style NBT 根标签，可以直接用根 NBT 读取器解析回来
    network::buffer::ByteBuf buf(bytes.data() + 1, bytes.size() - 1);
    auto root = network::buffer::nbt_io::readRootCompound(buf);
    ASSERT_TRUE(root.success());
    const auto tag = std::move(root).value();
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(getString(*tag, "color"), "red");
}

TEST(NumberFormatWireTest, FixedPlainTextPayloadIsStringTag)
{
    auto value = std::make_unique<text::StringTextComponent>("hi");
    const auto bytes = numbers::numberFormatToWireBytes(numbers::NumberFormat::fixed(std::move(value)));

    // VarInt(2) + StringTag(0x08) + U16 长度 + UTF8
    ASSERT_EQ(bytes.size(), 1u + 3u + 2u);
    EXPECT_EQ(bytes[0], 0x02);
    EXPECT_EQ(bytes[1], 0x08);
    EXPECT_EQ(bytes[2], 0x00);
    EXPECT_EQ(bytes[3], 0x02);
    EXPECT_EQ(bytes[4], 'h');
    EXPECT_EQ(bytes[5], 'i');
}

TEST(NumberFormatWireTest, FixedStyledComponentPayloadIsCompound)
{
    auto value = std::make_unique<text::StringTextComponent>("x");
    text::Style style;
    style.setBold(true);
    value->setStyle(style);

    const auto bytes = numbers::numberFormatToWireBytes(numbers::NumberFormat::fixed(std::move(value)));
    ASSERT_GE(bytes.size(), 2u);
    EXPECT_EQ(bytes[0], 0x02);
    // 带 style 的组件不可折叠，走 CompoundTag
    EXPECT_EQ(bytes[1], 0x0A);
}

// ============================================================================
// Style ↔ NBT
// ============================================================================

TEST(StyleNbtTest, RoundTripsAllRepresentableFields)
{
    text::Style style;
    style.setColor(text::TextFormatting::Aqua);
    style.setBold(true);
    style.setItalic(false);
    style.setUnderlined(true);
    style.setStrikethrough(false);
    style.setObfuscated(true);
    style.setInsertion("insert me");

    nbt::tags::compound_tag tag;
    text::styleToNbt(tag, style);

    EXPECT_EQ(getString(tag, "color"), "aqua");
    EXPECT_EQ(getByte(tag, "bold"), 1);
    EXPECT_EQ(getByte(tag, "italic"), 0);
    EXPECT_EQ(getByte(tag, "underlined"), 1);
    EXPECT_EQ(getByte(tag, "strikethrough"), 0);
    EXPECT_EQ(getByte(tag, "obfuscated"), 1);
    EXPECT_EQ(getString(tag, "insertion"), "insert me");

    auto parsed = text::styleFromNbt(tag);
    ASSERT_TRUE(parsed.success());
    EXPECT_EQ(parsed.value(), style);
}

TEST(StyleNbtTest, EmptyStyleWritesNothing)
{
    nbt::tags::compound_tag tag;
    text::styleToNbt(tag, text::Style());
    EXPECT_TRUE(tag.value.empty());

    auto parsed = text::styleFromNbt(tag);
    ASSERT_TRUE(parsed.success());
    EXPECT_TRUE(parsed.value().isEmpty());
}

TEST(StyleNbtTest, NumericFlagsAreAccepted)
{
    // 原版 Codec.BOOL 经 NbtOps 接受任意数值标签（SNBT 里写 {bold:1} 是 IntTag）
    nbt::tags::compound_tag tag;
    tag.put("bold", 1);
    tag.put("italic", 0);
    auto parsed = text::styleFromNbt(tag);
    ASSERT_TRUE(parsed.success());
    EXPECT_EQ(parsed.value().bold(), true);
    EXPECT_EQ(parsed.value().italic(), false);
}

// 未知键一律忽略。注意不能拿 `shadow_color` / `font` 当样本：这两个字段已经实现，
// 现在会被解码出来（此前正是因为未实现才被当作"未知键"的代表）。
TEST(StyleNbtTest, UnknownKeysAreIgnored)
{
    nbt::tags::compound_tag tag;
    tag.put("unrecognized_style_key", 0);
    tag.put("another_unknown_key", std::string("value"));
    auto parsed = text::styleFromNbt(tag);
    ASSERT_TRUE(parsed.success());
    EXPECT_TRUE(parsed.value().isEmpty());
}

// ============================================================================
// Component ↔ NBT 标签（数字格式 fixed 载荷依赖）
// ============================================================================

TEST(ComponentNbtTagTest, RoundTripsPlainText)
{
    const text::StringTextComponent text("hello");
    const auto tag = text::componentToNbtTag(&text);
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->id(), nbt::TagId::String);

    auto parsed = text::componentFromNbtTag(*tag);
    ASSERT_TRUE(parsed.success());
    // 【重要】Result<std::unique_ptr<T>>::value() 是转移所有权语义：只能取一次，必须先落到局部变量
    std::unique_ptr<text::ITextComponent> restored = parsed.value();
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->getUnformattedText(), "hello");
}

TEST(ComponentNbtTagTest, RoundTripsStyledComponentWithSiblings)
{
    auto root = std::make_unique<text::StringTextComponent>("root");
    text::Style style;
    style.setColor(text::TextFormatting::Green);
    style.setBold(true);
    root->setStyle(style);
    root->append(std::make_unique<text::StringTextComponent>("child"));

    const auto tag = text::componentToNbtTag(root.get());
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->id(), nbt::TagId::Compound);

    auto parsed = text::componentFromNbtTag(*tag);
    ASSERT_TRUE(parsed.success());
    // value() 只能取一次（转移所有权），先落到局部变量
    std::unique_ptr<text::ITextComponent> restored = parsed.value();
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->getUnformattedText(), "rootchild");
    EXPECT_EQ(restored->getStyle(), style);
    ASSERT_EQ(restored->getSiblings().size(), 1u);
    EXPECT_EQ(restored->getSiblings().front()->getUnformattedText(), "child");
}

TEST(ComponentNbtTagTest, RoundTripsTranslationComponent)
{
    std::vector<std::unique_ptr<text::ITextComponent>> params;
    params.push_back(std::make_unique<text::StringTextComponent>("Steve"));
    const text::TranslationTextComponent text("chat.type.text", std::move(params));

    const auto tag = text::componentToNbtTag(&text);
    ASSERT_NE(tag, nullptr);
    auto parsed = text::componentFromNbtTag(*tag);
    ASSERT_TRUE(parsed.success());
    // value() 只能取一次（转移所有权），先落到局部变量
    std::unique_ptr<text::ITextComponent> restored = parsed.value();
    ASSERT_NE(restored, nullptr);

    const auto* translatable = dynamic_cast<const text::TranslationTextComponent*>(restored.get());
    ASSERT_NE(translatable, nullptr);
    EXPECT_EQ(translatable->getKey(), "chat.type.text");
    ASSERT_EQ(translatable->getParams().size(), 1u);
    EXPECT_EQ(translatable->getParams().front()->getUnformattedText(), "Steve");
}

TEST(ComponentNbtTagTest, RejectsCompoundWithoutContents)
{
    nbt::tags::compound_tag tag;
    tag.put("color", std::string("red"));
    const auto parsed = text::componentFromNbtTag(tag);
    EXPECT_TRUE(parsed.failed());
}

TEST(ComponentNbtTagTest, WireBytesMatchRootTagEncoding)
{
    // componentToNbtBytes 与 componentToNbtTag + 根标签编码必须一致
    const text::StringTextComponent text("abc");
    const auto tag = text::componentToNbtTag(&text);
    const auto expected = network::buffer::nbt_io::serializeRootTagToBytes(*tag);
    EXPECT_EQ(text::componentToNbtBytes(&text), expected);
}
