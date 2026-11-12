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
 * @file PathEncodingTest.cpp
 * @brief UTF-8 与宽字符 / 文件系统路径之间的转换
 *
 * Windows 上路径与命令行都是 UTF-16，而项目内部一律用 UTF-8。若在两者之间直接
 * 用窄字符串中转，非 ASCII 字符会按 ANSI 代码页解释并失真（中文路径在英文系统上
 * 直接变 '?'）。这里钉住转换是可逆的、且路径构造走的是 UTF-8 语义。
 */

#include <gtest/gtest.h>

#include "common/util/PathEncoding.hpp"

#include <filesystem>
#include <string>

using namespace mc;

/**
 * @test UTF-8 ↔ 宽字符串往返无损
 *
 * 覆盖 ASCII、两字节（é）、三字节（中文）、四字节（emoji，需代理对）四种宽度。
 */
TEST(PathEncodingTest, Utf8WideRoundTrip)
{
    const std::string samples[] = {
        "plain_ascii_path",
        "caf\u00e9",   // é：U+00E9
        "\u4e2d\u6587\u76ee\u5f55", // 中文目录
        "\U0001F600_emoji", // 😀：U+1F600，UTF-16 下是代理对
    };

    for (const std::string& sample : samples) {
        const std::wstring wide = util::utf8ToWide(sample);
#if defined(_WIN32)
        ASSERT_FALSE(wide.empty()) << "utf8ToWide failed for: " << sample;
#endif
        EXPECT_EQ(util::wideToUtf8(wide), sample) << "round trip broke for: " << sample;
    }
}

/**
 * @test 空串转换保持为空
 */
TEST(PathEncodingTest, EmptyInputsStayEmpty)
{
    EXPECT_TRUE(util::utf8ToWide("").empty());
    EXPECT_TRUE(util::wideToUtf8(L"").empty());
    EXPECT_TRUE(util::pathFromUtf8("").empty());
}

/**
 * @test 含非 ASCII 的 UTF-8 路径往返后与原文一致
 *
 * 这是修复的核心：`std::filesystem::path(std::string)` 在 Windows 上按 ANSI
 * 代码页解释，往返会得到不同的字节；pathFromUtf8/pathToUtf8 必须保真。
 */
TEST(PathEncodingTest, Utf8PathRoundTripPreservesNonAscii)
{
    const std::string samples[] = {
        "C:/saves/world",
        "/home/user/saves/\u4e2d\u6587\u4e16\u754c",
        "D:/MC\u7814\u7a76/saves/caf\u00e9",
    };

    for (const std::string& sample : samples) {
        const std::filesystem::path path = util::pathFromUtf8(sample);
        EXPECT_FALSE(path.empty()) << "pathFromUtf8 returned empty for: " << sample;
        EXPECT_EQ(util::pathToUtf8(path), sample) << "round trip broke for: " << sample;
    }
}

/**
 * @test Windows 上路径内部的宽字符表示就是原文
 *
 * 直接比较 `path.wstring()` 与 `utf8ToWide(原文)`：若实现走了 ANSI 中转，
 * 两步都会退化成同一个错误值而"往返通过"，因此必须和原文比。
 */
TEST(PathEncodingTest, PathWideFormMatchesSourceUtf8)
{
#if defined(_WIN32)
    const std::string sample = "C:/saves/\u4e2d\u6587\u4e16\u754c";
    const std::filesystem::path path = util::pathFromUtf8(sample);
    EXPECT_EQ(path.wstring(), util::utf8ToWide(sample))
        << "path must be built from the UTF-8 source without an ANSI detour";
#else
    GTEST_SKIP() << "wide path backing store only exists on Windows";
#endif
}

/**
 * @test 非法 UTF-8 不会产生垃圾宽串
 *
 * MB_ERR_INVALID_CHARS 让转换失败返回空串，而不是把非法字节替换成 '?' 后静默继续。
 */
TEST(PathEncodingTest, InvalidUtf8IsRejected)
{
#if defined(_WIN32)
    // 0xFF 不是合法 UTF-8 序列的首字节。
    const std::string invalid = "\xFF\xFE";
    EXPECT_TRUE(util::utf8ToWide(invalid).empty()) << "invalid UTF-8 must not be silently converted";
    EXPECT_TRUE(util::pathFromUtf8(invalid).empty()) << "invalid UTF-8 must not become a path";
#else
    GTEST_SKIP() << "strict validation is implemented through MB_ERR_INVALID_CHARS on Windows";
#endif
}