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

// ============================================================================
// util::json 非抛出访问层测试
//
// 这一层替代了数据包加载路径上的 nlohmann 抛异常用法：
// - 解析失败必须返回 Error（而不是抛 parse_error / 中止进程）
// - 取值时类型不符必须返回 Error（而不是抛 type_error）
// - 可选字段用 findField 判空，缺失不是错误
// ============================================================================

#include "common/util/json/JsonRead.hpp"

#include <sstream>
#include <string>
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// parse
// ============================================================================

TEST(JsonReadTest, ParseValidDocument)
{
    auto parsed = util::json::parse(R"({"a":1,"b":"x"})");
    ASSERT_TRUE(parsed.success());
    EXPECT_EQ(parsed.value().at("a").get<i32>(), 1);
    EXPECT_EQ(parsed.value().at("b").get<std::string>(), "x");
}

TEST(JsonReadTest, ParseInvalidDocumentReturnsError)
{
    // 非法 JSON 必须返回错误，不得抛出
    EXPECT_TRUE(util::json::parse("{invalid").failed());
    EXPECT_TRUE(util::json::parse("").failed());
    EXPECT_TRUE(util::json::parse(R"({"a":})").failed());
}

TEST(JsonReadTest, ParseFromStream)
{
    std::istringstream input(R"({"n":42})");
    auto parsed = util::json::parse(input);
    ASSERT_TRUE(parsed.success());
    EXPECT_EQ(parsed.value().at("n").get<i32>(), 42);
}

TEST(JsonReadTest, ParseFromStreamInvalid)
{
    std::istringstream input("not json");
    EXPECT_TRUE(util::json::parse(input).failed());
}

// ============================================================================
// findField / hasField
// ============================================================================

TEST(JsonReadTest, FindField)
{
    auto parsed = util::json::parse(R"({"present":1})");
    ASSERT_TRUE(parsed.success());
    const auto& document = parsed.value();

    EXPECT_TRUE(util::json::hasField(document, "present"));
    EXPECT_FALSE(util::json::hasField(document, "absent"));
    ASSERT_NE(util::json::findField(document, "present"), nullptr);
    EXPECT_EQ(util::json::findField(document, "absent"), nullptr);
    // 非对象上查找恒为不存在，且不抛异常
    EXPECT_EQ(util::json::findField(nlohmann::json{1}, "x"), nullptr);
}

// ============================================================================
// 类型化取值
// ============================================================================

TEST(JsonReadTest, TypedGettersHappyPath)
{
    auto parsed = util::json::parse(R"({"i":7,"l":8589934592,"f":1.5,"b":true,"s":"hi"})");
    ASSERT_TRUE(parsed.success());
    const auto& document = parsed.value();

    EXPECT_EQ(util::json::getInt32(document, "i").value(), 7);
    EXPECT_EQ(util::json::getInt64(document, "l").value(), 8589934592LL);
    EXPECT_FLOAT_EQ(util::json::getFloat32(document, "f").value(), 1.5f);
    EXPECT_DOUBLE_EQ(util::json::getFloat64(document, "f").value(), 1.5);
    EXPECT_TRUE(util::json::getBool(document, "b").value());
    EXPECT_EQ(util::json::getString(document, "s").value(), "hi");
}

TEST(JsonReadTest, TypedGettersMissingField)
{
    auto parsed = util::json::parse(R"({})");
    ASSERT_TRUE(parsed.success());
    const auto& document = parsed.value();

    EXPECT_TRUE(util::json::getInt32(document, "missing").failed());
    EXPECT_TRUE(util::json::getInt64(document, "missing").failed());
    EXPECT_TRUE(util::json::getFloat32(document, "missing").failed());
    EXPECT_TRUE(util::json::getFloat64(document, "missing").failed());
    EXPECT_TRUE(util::json::getBool(document, "missing").failed());
    EXPECT_TRUE(util::json::getString(document, "missing").failed());
    EXPECT_TRUE(util::json::getObject(document, "missing").failed());
    EXPECT_TRUE(util::json::getArray(document, "missing").failed());
}

TEST(JsonReadTest, TypedGettersTypeMismatch)
{
    // 类型不符必须返回错误（而不是抛 type_error / 隐式转换）
    auto parsed = util::json::parse(R"({"s":"not a number","i":3})");
    ASSERT_TRUE(parsed.success());
    const auto& document = parsed.value();

    EXPECT_TRUE(util::json::getInt32(document, "s").failed());
    EXPECT_TRUE(util::json::getFloat32(document, "s").failed());
    EXPECT_TRUE(util::json::getBool(document, "i").failed());
    EXPECT_TRUE(util::json::getString(document, "i").failed());
    EXPECT_TRUE(util::json::getObject(document, "i").failed());
    EXPECT_TRUE(util::json::getArray(document, "i").failed());
}

TEST(JsonReadTest, FloatGetterAcceptsIntegers)
{
    // 浮点字段在数据包里常写成整数（1 而非 1.0），必须接受
    auto parsed = util::json::parse(R"({"f":2})");
    ASSERT_TRUE(parsed.success());
    EXPECT_FLOAT_EQ(util::json::getFloat32(parsed.value(), "f").value(), 2.0f);
}

TEST(JsonReadTest, GetObjectAndArray)
{
    auto parsed = util::json::parse(R"({"o":{"k":1},"a":[1,2,3]})");
    ASSERT_TRUE(parsed.success());
    const auto& document = parsed.value();

    auto object = util::json::getObject(document, "o");
    ASSERT_TRUE(object.success());
    EXPECT_EQ(object.value().at("k").get<i32>(), 1);

    auto array = util::json::getArray(document, "a");
    ASSERT_TRUE(array.success());
    EXPECT_EQ(array.value().size(), 3u);
}

TEST(JsonReadTest, ValueOnlyOverloads)
{
    // 不带 field 的重载作用于已定位的 JSON 值
    auto number = util::json::parse("42");
    ASSERT_TRUE(number.success());
    EXPECT_EQ(util::json::getInt32(number.value()).value(), 42);
    EXPECT_TRUE(util::json::getString(number.value()).failed());

    auto text = util::json::parse(R"("hello")");
    ASSERT_TRUE(text.success());
    EXPECT_EQ(util::json::getString(text.value()).value(), "hello");
}

TEST(JsonReadTest, IntegerGettersHandleUnsignedLiterals)
{
    // 非负整数字面量被 nlohmann 解析为 number_unsigned，不能被误判为类型不符
    auto document = util::json::parse(R"({"positive":7,"negative":-7})");
    ASSERT_TRUE(document.success());

    auto positive = util::json::getInt32(document.value(), "positive");
    ASSERT_TRUE(positive.success());
    EXPECT_EQ(positive.value(), 7);

    auto negative = util::json::getInt32(document.value(), "negative");
    ASSERT_TRUE(negative.success());
    EXPECT_EQ(negative.value(), -7);

    auto wide = util::json::getInt64(document.value(), "positive");
    ASSERT_TRUE(wide.success());
    EXPECT_EQ(wide.value(), 7);
}

TEST(JsonReadTest, IntegerGettersRejectOutOfRangeValues)
{
    // get<i32>() 在越界时会抛 out_of_range，这一层必须转成 Error
    auto document = util::json::parse(R"({"tooBig":3000000000,"tooSmall":-3000000000,"u64Max":18446744073709551615})");
    ASSERT_TRUE(document.success());

    EXPECT_TRUE(util::json::getInt32(document.value(), "tooBig").failed());
    EXPECT_TRUE(util::json::getInt32(document.value(), "tooSmall").failed());
    EXPECT_TRUE(util::json::getInt64(document.value(), "u64Max").failed()); // 超出 int64 范围
}

TEST(JsonReadTest, FloatGetterRejectsOutOfRangeValues)
{
    // 超出 f32 范围的数值不能静默变成 inf
    auto document = util::json::parse(R"({"huge":1e300,"normal":0.5})");
    ASSERT_TRUE(document.success());

    EXPECT_TRUE(util::json::getFloat32(document.value(), "huge").failed());

    auto normal = util::json::getFloat32(document.value(), "normal");
    ASSERT_TRUE(normal.success());
    EXPECT_FLOAT_EQ(normal.value(), 0.5f);

    // f64 仍可读取 1e300，只是不能收敛到 f32
    auto wide = util::json::getFloat64(document.value(), "huge");
    ASSERT_TRUE(wide.success());
    EXPECT_DOUBLE_EQ(wide.value(), 1e300);
}

TEST(JsonReadTest, FallbackGettersUseFallbackOnMissingOrMismatch)
{
    // 可选字段：缺失或类型不符都落到兜底值，且不抛 type_error
    auto document =
        util::json::parse(R"({"int":3,"float":0.5,"bool":true,"text":"x","object":{"k":1},"wrongType":"notANumber"})");
    ASSERT_TRUE(document.success());
    const auto& json = document.value();

    EXPECT_EQ(util::json::getInt32Or(json, "int", -1), 3);
    EXPECT_EQ(util::json::getInt32Or(json, "missing", -1), -1);
    EXPECT_EQ(util::json::getInt32Or(json, "wrongType", -1), -1);

    EXPECT_FLOAT_EQ(util::json::getFloat32Or(json, "float", 0.0f), 0.5f);
    EXPECT_FLOAT_EQ(util::json::getFloat32Or(json, "missing", 0.0f), 0.0f);

    EXPECT_TRUE(util::json::getBoolOr(json, "bool", false));
    EXPECT_FALSE(util::json::getBoolOr(json, "missing", false));

    EXPECT_EQ(util::json::getStringOr(json, "text", "fallback"), "x");
    EXPECT_EQ(util::json::getStringOr(json, "missing", "fallback"), "fallback");

    EXPECT_EQ(util::json::getObjectOrEmpty(json, "object").at("k").get<i32>(), 1);
    EXPECT_TRUE(util::json::getObjectOrEmpty(json, "missing").empty());
    EXPECT_TRUE(util::json::getObjectOrEmpty(json, "wrongType").empty());
}
