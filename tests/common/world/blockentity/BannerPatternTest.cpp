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

#include "world/blockentity/interactive/BannerPattern.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blockentity;

// ========== BannerPattern 测试 ==========

TEST(BannerPatternTest, ByHashReturnsCorrectPattern)
{
    // 测试常见图案的hash名称查找
    EXPECT_EQ(BannerPatterns::byHash("bs"), BannerPatternType::Base);
    EXPECT_EQ(BannerPatterns::byHash("bl"), BannerPatternType::StripeBottom);
    EXPECT_EQ(BannerPatterns::byHash("tl"), BannerPatternType::StripeTop);
    EXPECT_EQ(BannerPatterns::byHash("br"), BannerPatternType::StripeRight);
    EXPECT_EQ(BannerPatterns::byHash("tr"), BannerPatternType::StripeLeft);
    EXPECT_EQ(BannerPatterns::byHash("mc"), BannerPatternType::Creeper);
    EXPECT_EQ(BannerPatterns::byHash("moj"), BannerPatternType::Mojang);
}

TEST(BannerPatternTest, ByHashReturnsNulloptForInvalid)
{
    auto result = BannerPatterns::byHash("invalid_hash");
    EXPECT_FALSE(result.has_value());
}

TEST(BannerPatternTest, GetHashNameRoundTrip)
{
    // 验证所有图案的hash名称可以round-trip
    for (i32 i = 1; i < static_cast<i32>(BannerPatternType::Count); ++i) {
        auto type = static_cast<BannerPatternType>(i);
        std::string hashName = BannerPatterns::getHashName(type);
        EXPECT_FALSE(hashName.empty()) << "Hash name empty for pattern index " << i;

        auto result = BannerPatterns::byHash(hashName);
        ASSERT_TRUE(result.has_value()) << "byHash failed for hash name: " << hashName;
        EXPECT_EQ(result.value(), type) << "Round-trip failed for pattern index " << i;
    }
}

TEST(BannerPatternTest, HasPatternItem)
{
    // 需要图案物品的6种特殊图案
    EXPECT_TRUE(BannerPatterns::hasPatternItem(BannerPatternType::Creeper));
    EXPECT_TRUE(BannerPatterns::hasPatternItem(BannerPatternType::Skull));
    EXPECT_TRUE(BannerPatterns::hasPatternItem(BannerPatternType::Flower));
    EXPECT_TRUE(BannerPatterns::hasPatternItem(BannerPatternType::Mojang));
    EXPECT_TRUE(BannerPatterns::hasPatternItem(BannerPatternType::Globe));
    EXPECT_TRUE(BannerPatterns::hasPatternItem(BannerPatternType::Piglin));

    // 不需要图案物品的基础图案
    EXPECT_FALSE(BannerPatterns::hasPatternItem(BannerPatternType::Base));
    EXPECT_FALSE(BannerPatterns::hasPatternItem(BannerPatternType::StripeBottom));
    EXPECT_FALSE(BannerPatterns::hasPatternItem(BannerPatternType::Cross));
}

TEST(BannerPatternTest, IsBase)
{
    EXPECT_TRUE(BannerPatterns::isBase(BannerPatternType::Base));
    EXPECT_FALSE(BannerPatterns::isBase(BannerPatternType::StripeBottom));
    EXPECT_FALSE(BannerPatterns::isBase(BannerPatternType::Creeper));
}

TEST(BannerPatternTest, GetFileName)
{
    // 验证文件名不为空
    for (i32 i = 1; i < static_cast<i32>(BannerPatternType::Count); ++i) {
        auto type = static_cast<BannerPatternType>(i);
        std::string fileName = BannerPatterns::getFileName(type);
        EXPECT_FALSE(fileName.empty()) << "File name empty for pattern index " << i;
    }
}
