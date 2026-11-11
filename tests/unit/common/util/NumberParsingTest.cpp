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
// NumberParsing / StringUtils 校验工具测试
//
// 这些工具替代了 std::sto*（会抛异常）与 std::regex（构造失败会抛 regex_error），
// 是所有「外部输入 → 数值」边界的基础设施，行为必须严格：非法输入返回错误、
// 越界不被静默截断、合法输入必须解析成功。
// ============================================================================

#include "common/util/NumberParsing.hpp"
#include "common/util/StringUtils.hpp"

#include <limits>
#include <string>
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// 整数解析
// ============================================================================

TEST(NumberParsingTest, ParseInt32Valid)
{
    EXPECT_EQ(util::parseInt32("42").value(), 42);
    EXPECT_EQ(util::parseInt32("-42").value(), -42);
    EXPECT_EQ(util::parseInt32("0").value(), 0);
    EXPECT_EQ(util::parseInt32("2147483647").value(), std::numeric_limits<i32>::max());
    EXPECT_EQ(util::parseInt32("-2147483648").value(), std::numeric_limits<i32>::min());
    // 首尾空白必须被容忍（配置文件常见）
    EXPECT_EQ(util::parseInt32("  7  ").value(), 7);
    EXPECT_EQ(util::parseInt32("+13").value(), 13);
}

TEST(NumberParsingTest, ParseInt32RejectsInvalid)
{
    EXPECT_TRUE(util::parseInt32("").failed());
    EXPECT_TRUE(util::parseInt32("   ").failed());
    EXPECT_TRUE(util::parseInt32("abc").failed());
    EXPECT_TRUE(util::parseInt32("42abc").failed());
    EXPECT_TRUE(util::parseInt32("4 2").failed());
    EXPECT_TRUE(util::parseInt32("1.5").failed());
}

TEST(NumberParsingTest, ParseInt32RejectsOverflow)
{
    // 越界绝不能被静默截断
    EXPECT_TRUE(util::parseInt32("2147483648").failed());
    EXPECT_TRUE(util::parseInt32("-2147483649").failed());
    EXPECT_TRUE(util::parseInt32("99999999999999999999").failed());
}

TEST(NumberParsingTest, ParseInt64AndNarrow)
{
    EXPECT_EQ(util::parseInt64("-9223372036854775808").value(), std::numeric_limits<i64>::min());
    EXPECT_EQ(util::parseInt64("9223372036854775807").value(), std::numeric_limits<i64>::max());
    EXPECT_TRUE(util::parseInt64("9223372036854775808").failed());

    EXPECT_EQ(util::parseInt16("-32768").value(), std::numeric_limits<i16>::min());
    EXPECT_TRUE(util::parseInt16("32768").failed());

    EXPECT_EQ(util::parseInt8("-128").value(), std::numeric_limits<i8>::min());
    EXPECT_TRUE(util::parseInt8("128").failed());
}

TEST(NumberParsingTest, ParseUint32RejectsNegativeAndOverflow)
{
    EXPECT_EQ(util::parseUint32("4294967295").value(), std::numeric_limits<u32>::max());
    EXPECT_TRUE(util::parseUint32("4294967296").failed());
    EXPECT_TRUE(util::parseUint32("-1").failed());
}

TEST(NumberParsingTest, ParseUint64Base)
{
    // 十六进制颜色
    EXPECT_EQ(util::parseUint64Base("FF8800", 16).value(), 0xFF8800u);
    EXPECT_EQ(util::parseUint64Base("ff8800", 16).value(), 0xFF8800u);
    EXPECT_EQ(util::parseUint64Base("0", 16).value(), 0u);
    EXPECT_TRUE(util::parseUint64Base("GG", 16).failed());
    EXPECT_TRUE(util::parseUint64Base("", 16).failed());
    // 进制非法
    EXPECT_TRUE(util::parseUint64Base("10", 1).failed());
    EXPECT_TRUE(util::parseUint64Base("10", 37).failed());
}

// ============================================================================
// 浮点解析
// ============================================================================

TEST(NumberParsingTest, ParseFloatValid)
{
    EXPECT_FLOAT_EQ(util::parseFloat32("3.5").value(), 3.5f);
    EXPECT_FLOAT_EQ(util::parseFloat32("-3.5").value(), -3.5f);
    EXPECT_FLOAT_EQ(util::parseFloat32(" 1.25 ").value(), 1.25f);
    EXPECT_FLOAT_EQ(util::parseFloat32("1e3").value(), 1000.0f);
    EXPECT_DOUBLE_EQ(util::parseFloat64("2.5").value(), 2.5);
    EXPECT_DOUBLE_EQ(util::parseFloat64("-1e-3").value(), -1e-3);
}

TEST(NumberParsingTest, ParseFloatRejectsInvalid)
{
    EXPECT_TRUE(util::parseFloat32("").failed());
    EXPECT_TRUE(util::parseFloat32("abc").failed());
    EXPECT_TRUE(util::parseFloat32("1.5x").failed());
    EXPECT_TRUE(util::parseFloat64(".").failed());
}

// ============================================================================
// 字符类校验
// ============================================================================

TEST(StringUtilsTest, AllOfCharacters)
{
    const auto isDigit = [](char c) {
        return c >= '0' && c <= '9';
    };
    EXPECT_TRUE(util::allOfCharacters("0123", isDigit));
    EXPECT_FALSE(util::allOfCharacters("", isDigit));
    EXPECT_FALSE(util::allOfCharacters("01a", isDigit));
}

TEST(StringUtilsTest, IsValidIpv4)
{
    EXPECT_TRUE(util::isValidIpv4("127.0.0.1"));
    EXPECT_TRUE(util::isValidIpv4("255.255.255.255"));
    EXPECT_TRUE(util::isValidIpv4("0.0.0.0"));
    // 与旧正则一致：允许前导零
    EXPECT_TRUE(util::isValidIpv4("192.168.001.001"));

    // 段数不对
    EXPECT_FALSE(util::isValidIpv4("127.0.0"));
    EXPECT_FALSE(util::isValidIpv4("127.0.0.1.5"));
    // 越界
    EXPECT_FALSE(util::isValidIpv4("256.0.0.1"));
    // 非法字符 / 空段
    EXPECT_FALSE(util::isValidIpv4("127.0.0."));
    EXPECT_FALSE(util::isValidIpv4("127..0.1"));
    EXPECT_FALSE(util::isValidIpv4("a.b.c.d"));
    EXPECT_FALSE(util::isValidIpv4(""));
    EXPECT_FALSE(util::isValidIpv4("127.0.0.1:25565"));
}

TEST(StringUtilsTest, TrimWhitespace)
{
    EXPECT_EQ(util::trimWhitespace("  abc\t\n"), "abc");
    EXPECT_EQ(util::trimWhitespace(""), "");
    EXPECT_EQ(util::trimWhitespace("   "), "");
    EXPECT_EQ(util::trimWhitespace("a b"), "a b");
}
