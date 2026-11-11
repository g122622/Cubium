#include <gtest/gtest.h>

#include "common/mod/bedrock/addon/pack/PackVersion.hpp"

namespace {

using mc::i32;
using mc::mod::bedrock::addon::PackVersion;

} // namespace

// 基岩版内容的 format_version 是点分字符串，正常形态必须能解析。
TEST(PackVersionFromStringTest, ParsesDottedVersions)
{
    PackVersion v;
    ASSERT_TRUE(PackVersion::fromString("1.21.0", v));
    EXPECT_EQ(v.major, 1);
    EXPECT_EQ(v.minor, 21);
    EXPECT_EQ(v.patch, 0);

    ASSERT_TRUE(PackVersion::fromString("1.16.100", v));
    EXPECT_EQ(v.major, 1);
    EXPECT_EQ(v.minor, 16);
    EXPECT_EQ(v.patch, 100);
}

// 单段与两段也要接受：基岩内容里存在 "1" / "1.20" 这类写法。
TEST(PackVersionFromStringTest, AcceptsFewerThanThreeSegments)
{
    PackVersion v;
    ASSERT_TRUE(PackVersion::fromString("1", v));
    EXPECT_EQ(v.major, 1);
    EXPECT_EQ(v.minor, 0);
    EXPECT_EQ(v.patch, 0);

    ASSERT_TRUE(PackVersion::fromString("1.20", v));
    EXPECT_EQ(v.major, 1);
    EXPECT_EQ(v.minor, 20);
    EXPECT_EQ(v.patch, 0);
}

// 这些都不该被接受。原先只校验 is_string()，垃圾串能直接通过。
TEST(PackVersionFromStringTest, RejectsMalformedInput)
{
    PackVersion v;

    EXPECT_FALSE(PackVersion::fromString("", v)) << "empty string must be rejected";
    EXPECT_FALSE(PackVersion::fromString("abc", v)) << "non-numeric must be rejected";
    EXPECT_FALSE(PackVersion::fromString("1.a.0", v)) << "non-numeric segment must be rejected";
    EXPECT_FALSE(PackVersion::fromString("1..0", v)) << "empty segment must be rejected";
    EXPECT_FALSE(PackVersion::fromString("1.0.", v)) << "trailing dot must be rejected";
    EXPECT_FALSE(PackVersion::fromString(".1.0", v)) << "leading dot must be rejected";
    EXPECT_FALSE(PackVersion::fromString("-1.0.0", v)) << "negative must be rejected";
    EXPECT_FALSE(PackVersion::fromString("1.0.0.0", v)) << "four segments must be rejected";
    EXPECT_FALSE(PackVersion::fromString("1.0 beta", v)) << "whitespace must be rejected";
    EXPECT_FALSE(PackVersion::fromString("99999999999.0.0", v)) << "overflowing segment must be rejected";
}

// 解析结果要能与现有比较运算符协同工作（版本语义矩阵的前提）。
TEST(PackVersionFromStringTest, ParsedVersionsCompareCorrectly)
{
    PackVersion older;
    PackVersion newer;
    ASSERT_TRUE(PackVersion::fromString("1.16.0", older));
    ASSERT_TRUE(PackVersion::fromString("1.21.0", newer));

    EXPECT_LT(older, newer);
    EXPECT_GT(newer, older);
    EXPECT_NE(older, newer);
}

// 失败时不得写坏出参。
TEST(PackVersionFromStringTest, FailureLeavesOutputUntouched)
{
    PackVersion v;
    ASSERT_TRUE(PackVersion::fromString("1.21.0", v));

    EXPECT_FALSE(PackVersion::fromString("garbage", v));
    EXPECT_EQ(v.major, 1);
    EXPECT_EQ(v.minor, 21);
    EXPECT_EQ(v.patch, 0);
}
