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
 * @file SnbtLiteralTest.cpp
 * @brief SNBT 文本字面量的类型保真测试
 *
 * 覆盖两处与 SNBT 规范不一致的地方：
 *  1. 双精度标签写出时不带 'd' 后缀，导致 3.0 被打印成 "3"，重新解析退化为 IntTag；
 *  2. 布尔字面量 true/false 未被识别，被当作未引用字符串解析为 StringTag，
 *     而 SNBT 规范要求解析为 ByteTag 的 1b/0b。
 */

#include <gtest/gtest.h>

#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"

#include <optional>
#include <sstream>
#include <string>

using namespace mc;

namespace {

/**
 * @brief 以 Mojangson 文本形式序列化标签
 */
std::string _dump(const nbt::tags::tag& tag)
{
    using nbt::operator<<;
    std::ostringstream ss;
    ss << nbt::contexts::mojangson << tag;
    return ss.str();
}

/**
 * @brief 解析 Mojangson 文本为复合标签
 */
std::unique_ptr<nbt::tags::compound_tag> _parse(const std::string& text)
{
    auto result = nbt::parseMojangson(text);
    if (result.failed()) {
        return nullptr;
    }
    return std::move(result).value();
}

/**
 * @brief 从复合标签中按名字取出指定类型的子标签（不存在或类型不符返回 nullptr）
 */
template <typename TagT>
const TagT* _child(const nbt::tags::compound_tag* parent, const std::string& name)
{
    if (parent == nullptr) {
        return nullptr;
    }
    const auto it = parent->value.find(name);
    if (it == parent->value.end()) {
        return nullptr;
    }
    return dynamic_cast<const TagT*>(it->second.get());
}

} // namespace

TEST(SnbtLiteralTest, DoubleAlwaysEmitsSuffix)
{
    // 关键点：取值为整数时也不能省略 'd'，否则类型信息丢失。
    nbt::tags::double_tag tag(3.0);
    EXPECT_EQ(_dump(tag), "3d");

    nbt::tags::double_tag fractional(2.5);
    EXPECT_EQ(_dump(fractional), "2.5d");
}

TEST(SnbtLiteralTest, DoubleRoundTripsThroughText)
{
    nbt::tags::compound_tag original;
    original.put("ratio", 3.0);
    original.put("scale", 2.5);

    const std::string text = _dump(original);
    auto parsed = _parse(text);
    ASSERT_NE(parsed, nullptr);

    const auto* ratio = _child<nbt::tags::double_tag>(parsed.get(), "ratio");
    ASSERT_NE(ratio, nullptr) << "integer-valued double must not decay to int_tag, text=" << text;
    EXPECT_DOUBLE_EQ(ratio->value, 3.0);

    const auto* scale = _child<nbt::tags::double_tag>(parsed.get(), "scale");
    ASSERT_NE(scale, nullptr);
    EXPECT_DOUBLE_EQ(scale->value, 2.5);
}

TEST(SnbtLiteralTest, BooleanLiteralsParseAsByte)
{
    auto parsed = _parse("{enabled:true,disabled:false}");
    ASSERT_NE(parsed, nullptr);

    const auto* enabled = _child<nbt::tags::byte_tag>(parsed.get(), "enabled");
    ASSERT_NE(enabled, nullptr) << "true must parse as byte_tag, not string_tag";
    EXPECT_EQ(enabled->value, 1);

    const auto* disabled = _child<nbt::tags::byte_tag>(parsed.get(), "disabled");
    ASSERT_NE(disabled, nullptr) << "false must parse as byte_tag, not string_tag";
    EXPECT_EQ(disabled->value, 0);
}

TEST(SnbtLiteralTest, BooleanLiteralsAreCaseInsensitive)
{
    auto parsed = _parse("{a:TRUE,b:False}");
    ASSERT_NE(parsed, nullptr);

    const auto* a = _child<nbt::tags::byte_tag>(parsed.get(), "a");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->value, 1);

    const auto* b = _child<nbt::tags::byte_tag>(parsed.get(), "b");
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->value, 0);
}

TEST(SnbtLiteralTest, IdentifierStartingWithTrueStaysString)
{
    // 只有完整的 true/false token 才是布尔；以 true 开头的标识符仍是字符串。
    auto parsed = _parse("{name:trueblue}");
    ASSERT_NE(parsed, nullptr);

    const auto* name = _child<nbt::tags::string_tag>(parsed.get(), "name");
    ASSERT_NE(name, nullptr) << "trueblue must stay string_tag";
    EXPECT_EQ(name->value, "trueblue");
}

TEST(SnbtLiteralTest, QuotedTrueStaysString)
{
    // 加引号的 "true" 是字符串，不是布尔。
    auto parsed = _parse("{name:\"true\"}");
    ASSERT_NE(parsed, nullptr);

    const auto* name = _child<nbt::tags::string_tag>(parsed.get(), "name");
    ASSERT_NE(name, nullptr) << "quoted \"true\" must stay string_tag";
    EXPECT_EQ(name->value, "true");
}

TEST(SnbtLiteralTest, BooleanInByteListParsesAsByte)
{
    // 列表元素也走 deduce_tag，布尔字面量在列表中同样是 byte。
    auto parsed = _parse("{flags:[true,false,true]}");
    ASSERT_NE(parsed, nullptr);

    const auto* flags = _child<nbt::tags::list_tag>(parsed.get(), "flags");
    ASSERT_NE(flags, nullptr);
    ASSERT_EQ(flags->size(), 3u);

    // list_tag 的元素按 unique_ptr<tag> 返回，需要先持有再取地址做类型判定。
    const auto firstOwned = (*flags)[0];
    const auto* first = dynamic_cast<const nbt::tags::byte_tag*>(firstOwned.get());
    ASSERT_NE(first, nullptr) << "list element true must parse as byte_tag";
    EXPECT_EQ(first->value, 1);
}
